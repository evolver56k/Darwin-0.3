/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/***********************************************************************
 *	objc-sel.m
 *
 *	Utilities for registering and looking up selectors.  The sole
 *	purpose of the selector tables is a registry whereby there is
 *	exactly one address (selector) associated with a given string
 *	(method name).
 *
 *	Copyright 1988-1997, NeXT Software, Inc.
 *
 *	Author:	s. naroff
 *
 *	Public functions:		
 *	--------------
 *	sel_getName
 *	sel_getUid
 *	sel_isMapped
 *	sel_registerName
 *	_strhash				(undocumented)
 *
 *	Private functions:
 *	---------------
 *	_sel_init				(private extern)
 *	_sel_unloadSelectors	(private extern)
 **********************************************************************/

/***********************************************************************
 * Imports.
 **********************************************************************/

#ifdef 	NeXT_PDO
#import <pdo.h>
#endif	/* NeXT_PDO */

#import "objc-private.h"

// Needed imports not in any header file
OBJC_EXPORT BOOL	rocketLaunchingDebug;

/***********************************************************************
 * Exports.
 **********************************************************************/

const char *	__S(checkSel) = 0;

/***********************************************************************
 * Constants and macros internal to this module.
 **********************************************************************/

enum HashTableConfiguration {
	// Number of hashtable entries to malloc in a chunk
	HASH_ALLOC_LIST_SIZE	= 40,

	// Number of slots (buckets) in a hash table
	SIZEHASHTABLE			= 821
};

/***********************************************************************
 * Types internal to this module.
 **********************************************************************/
 
// Hash table entry.  A hash table consists of SIZEHASHTABLE slots
// (buckets) which each a head of a list _hashEntry structures.  The
// hash value of a given string is masked to make an index into the
// slot array.  The list rooted at a given slot is composed of a
// _hashEntry for each distinct string that hashed/masked to that slot.
// The sel field doubles as the lookup key (i.e. the content of the
// string it points to is what is matched against) as well as the data
// (this string address IS the unique one chosen to represent all
// occurrences of the given string).  No other data is needed.
typedef struct _hashEntry
{
	struct _hashEntry *	next;
	const char *		sel;
} HASH, *PHASH;

// Selector table.  There are two basic kinds of table, frozen and dynamic,
// depending on whether selectors can be added/removed.  Further, there
// are two organizations for the frozen tables: sorted and hashed.  The
// dynamic table is always hashed.
// 
// sorted frozen - the backrefs array is bsearch'd
// -------------
// count		== number of elements in backrefs
// backrefs		== array of FixupEntry structures sorted alphabetically
// backrefbase	== where the selectors start (each FixupEntry contains
//					offset into this area for its given selector)
//
// entries == min == max == list == 0
// 
// hashed frozen - selectors stored between min and max, hash table is in list
// -------------
// count	== number of slots (hash buckets) in table (SIZEHASHTABLE)
// min		== selector string base
// max		== selector string bound
// list		== pointer to array of slots (PHASH)
// 
// entries == backrefs == backrefbase == 0
// 
// dynamic hash table - hash table is in list
// ------------------
// count	== number of slots (hash buckets) in table
//				(empty table has one nil slot, a non-empty
//				table has SIZEHASHTABLE slots)
// entries	== number of registered selectors
// list		== pointer to array of slots (PHASH)
// 
// min == max == backrefs == backrefbase == 0
//
typedef struct _hashTable
{
	const headerType *	header;			// associated header
	unsigned int		count;			// capacity
	unsigned int		entries;		// occupancy of dynamic table 
	const char *		min;			// lowest SEL
	const char *		max;			// end of highest SEL
	PHASH *				list;			// dynamic entries
	FixupEntry *		backrefs;		// backrefs
	const char *		backrefBase;	// lowest SEL
	struct _hashTable *	next;			// link to next table
} hashTable;

/***********************************************************************
 * Function prototypes internal to this module.
 **********************************************************************/
static void *		objc_malloc					(unsigned int size);
static inline PHASH	makeHashEntry				(const char * key, PHASH link);
static int			comparator					(const void * v1, const void * v2);
static FixupEntry *	fixupEntryForSelector		(const char * key, hashTable * table);
static const void *	selRefForFixupEntryInTable	(FixupEntry * entry, hashTable * ht);

/***********************************************************************
 * Static data internal to this module.
 **********************************************************************/

// The dynamic hashtable contains all selectors which are not present in
// any frozen table.  Entries for it are allocated only on demand.
// We use a sentinel to avoid extra tests for NULL.
static const PHASH	sentinel = NULL;

static hashTable	dynamicHashTable =
{
	NULL,					// header
	1,						// count
	0,						// entries
	NULL,					// min : no range check for dynamic table
	NULL,					// max : no range check for dynamic table
	(PHASH *) &sentinel,	// list
	NULL,					// backrefs
	NULL,					// backrefBase
	NULL					// next
};

// The list of all hashtables.  The frozen tables are placed in
// chronological order, and the dynamic hashtable is always last.
static hashTable *	hashTableChain = &dynamicHashTable;

#ifndef FREEZE
// Lock for all static data in objc-sel.m */
static OBJC_DECLARE_LOCK (selectorLock);
#endif

// Additional parameter to bsearch comparator
static const void *	relativeTo = NULL;

// Where dynamic hash table entries are allocated
static PHASH		HASH_alloc_list  = 0;
static int		HASH_alloc_index = 0;

/***********************************************************************
 * _objc_dynamic_hash_count.  Return the number of selectors mapped in
 * the dynamic hash table.
 **********************************************************************/
int	_objc_dynamic_hash_count       (void)
{
	return dynamicHashTable.entries;
}

/***********************************************************************
 * objc_malloc.   Return a newly allocated chunk of memory.
 *
 * This is a duplicate of a static function in objc-class.m!  We should
 * make this a private extern.
 **********************************************************************/
static void *	objc_malloc	       (unsigned int	size)
{
	NXZone *	zone;
	void *		space;
	
#ifdef FREEZE
	zone = NXDefaultMallocZone ();
#else
	zone = _objc_create_zone ();
#endif
	
	space = NXZoneMalloc (zone, size);
	
	if ((space == NULL) && (size != 0))
		__S(_objc_fatal) ("unable to allocate space");
	
#ifdef WIN32
	bzero(space, size);
#endif
	
	return space;
}

#ifndef FREEZE
/***********************************************************************
 * _strhash.  Return the hash value for the given string.
 *
 * NOTE: For compatibility only.  This function is public even though it never
 * should have been.  Unused internally.
 **********************************************************************/
uarith_t	_strhash       (const unsigned char *	str)
{
	return _objc_strhash (str);
}
#endif

/***********************************************************************
 * makeHashEntry.  Allocate and initialize a new entry for the
 * dynamic hashtable.
 *
 * NOTE: The selector lock is assumed to be already taken out.
 **********************************************************************/
static inline PHASH	makeHashEntry  (const char *	key,
									PHASH			link)
{
	PHASH	hashEntry;
	
	// Allocate HASH_alloc_list if don't yet have one, or the
	// one we have is full
	if (!HASH_alloc_list || HASH_alloc_index >= HASH_ALLOC_LIST_SIZE)
	{
		HASH_alloc_list  = objc_malloc (sizeof(HASH) * HASH_ALLOC_LIST_SIZE);
		HASH_alloc_index = 0;
	}
	
	// Use the next available bucket
	hashEntry = &HASH_alloc_list[HASH_alloc_index++];
	hashEntry->next = link;
	hashEntry->sel  = key;
	
	return hashEntry;
}

#ifndef FREEZE
/***********************************************************************
 * sel_isMapped.  Return whether the specified selector is mapped in one
 * of the installed selector tables.
 *
 * NOTE: This is no longer used.  We should remove this from the API.
 * In fact, is it erroneous because it skips the sorted frozen tables.
 **********************************************************************/
BOOL	sel_isMapped	       (SEL	sel)
{
	hashTable *	table;
	unsigned int	slot;
	
	// No such thing as a NULL selector
	if (sel == NULL) 
		return NO;
	
	// Synchronize
	OBJC_LOCK (&selectorLock);
	
	// Major loop - check each hash table
	for (table = hashTableChain; table; table = table->next)
	{
		// Try simple range check iff the table provides for it
		// (only the unsorted frozen tables have non-zero min and max)
		if (((const char *) sel >= table->min) && ((const char *) sel < table->max))
		{
			OBJC_UNLOCK (&selectorLock);
			return YES;
		}
		
		// Not found.  Try next table unless we're at the
		// dynamic table.
		// NOTE: This skips the sorted frozen tables.
		if (table != &dynamicHashTable)
			continue;
		
		// The dynamic table is searched as a hash table
		// NOTE: This would be faster if we chained the
		// malloc'd blocks of entries and made range checks
		// against each of them.  
		// Middle loop - check each slot in the dynamic table
		for (slot = 0; slot < table->count; slot += 1)
		{
			PHASH	target;
			
			// Minor loop - check each selector in the slot
			for (target = table->list[slot]; target; target = target->next)
			{
				if (sel == (SEL) target->sel)
				{
					OBJC_UNLOCK (&selectorLock);
					return YES;
				}
			}
		}
	}
	
	// De-synchronize
	OBJC_UNLOCK (&selectorLock);
	return NO;
}

/***********************************************************************
 * sel_getName.  Return a pointer to the string associated with the
 * specified selector.
 *
 * This is now very fast!
 *
 * NOTE: Previously we would return NULL if passed an invalid selector.
 **********************************************************************/
const char *	sel_getName    (SEL sel)
{
	return (const char *) sel;
}
#endif

/***********************************************************************
 * comparator.  Action-proc for bsearch when looking for a string in
 * a sorted table.
 *
 * The bsearch caller must set the relativeTo global to the value of the
 * table's backrefBase.
 **********************************************************************/
static int	comparator     (const void *	v1,
							const void *	v2)
{
	FixupEntry *	fixup;
	const char *	key;
	const char *	fixupSel;
	
	key		 = (const char *) v1;
	fixup	 = (FixupEntry *) v2;
	fixupSel = (const char *) relativeTo + fixup->selectorOffset;
	
	// Return whether the strings match
	return strcmp (key, fixupSel);
}

/***********************************************************************
 * fixupEntryForSelector.  Return the FixupEntry from the specified
 * sorted table for the specified key.
 *
 * NOTE: The selector lock is assumed to be already taken out.
 **********************************************************************/
static FixupEntry *	fixupEntryForSelector  (const char *	key,
											hashTable *		table)
{
	// Pass additional parameter to comparator
	// Thread-safe because the selector lock is held
	relativeTo = table->backrefBase;
	
	// Use bsearch to locate the FixupEntry
	return (FixupEntry *) bsearch  (key,
									table->backrefs,
									table->count,
									sizeof(FixupEntry),
									comparator);
}

/***********************************************************************
 * selRefForFixupEntryInTable.  Return the address of the selector
 * associated with the specified FixupEntry.
 **********************************************************************/
static const void *	selRefForFixupEntryInTable (FixupEntry *	entry,
												hashTable *		ht)
{
	return (const void *) ht->backrefBase + entry->addressOffset;
}

/***********************************************************************
 * _sel_registerName.  Register the specified string as a selector if
 * not already present.  Return the selector.
 *
 * NOTE: Formerly _sel_registerName().  Now again _sel_registerName(),
 * and a public safe variant added.
 **********************************************************************/
SEL	__S(_sel_registerName)	       (const char *	key)
{
	uarith_t	hash;
	hashTable *	table;
	
	// NULL selector for NULL key
	if (!key) 
		return (SEL) 0;
	
	// Calculate hash value for the key
	hash = _objc_strhash (key);
	
	// Synchronize
	OBJC_LOCK (&selectorLock);
	
	// Log occurrence of interesting key
	if (__S(checkSel) && !strcmp(key, __S(checkSel)))
		_NXLogError("found %s\n", __S(checkSel));

	// First check whether the key is in the dynamic hash table
	table = &dynamicHashTable;
	if (table->list != &sentinel)
	{
		unsigned int	slot;
		PHASH			target;
	
		// Search the slot corresponding to the hash value
		slot = hash % table->count;
		for (target = table->list[slot]; target; target = target->next)
		{
			// Check for string equality
			if ((key[0] == target->sel[0]) && (strcmp (key, target->sel) == 0))
			{
				OBJC_UNLOCK (&selectorLock);
				if (rocketLaunchingDebug)
					_NXLogError("Z: (%p) %s ==> (%p) %s\n", key, key, target->sel, target->sel);
				
				return (SEL) target->sel;
			}
		}
	}
	
	// Second, check whether the key is in one of the frozen tables
	for (table = hashTableChain; table; table = table->next)
	{
		// Try quick range check
		if ((key >= table->min) && (key < table->max))
		{
			OBJC_UNLOCK (&selectorLock);
			if (rocketLaunchingDebug)
				_NXLogError("A: (%p) %s ==> (%p) %s\n", key, key, key, key);
			return (SEL) key;
		}
		
		// Try dynamic list
		if (table->list)
		{
			PHASH			target;
			unsigned int	slot;
			
			// Search the list
			slot = hash % table->count;
			for (target = table->list[slot]; target; target = target->next)
			{
				if ((key[0] == target->sel[0]) && (strcmp (key, target->sel) == 0))
				{
					OBJC_UNLOCK (&selectorLock);
					if (rocketLaunchingDebug)
						_NXLogError("B: (%p) %s ==> (%p) %s\n", key, key, target->sel, target->sel);
					return (SEL) target->sel;
				}
			}
			
			// Not in list.  If we've gotten all the way to the
			// dynamic hashtable, the key is not registered.
			// Insert it as the selector in the dynamic hashtable.
			if (table == &dynamicHashTable)
			{
				table->entries += 1;
				
				// Allocate hashtable if needed.
				if (table->list == &sentinel)
				{
					/* Allocate a new empty array. */
					table->count = SIZEHASHTABLE;
					table->list  = objc_malloc (table->count * sizeof(PHASH));
#ifdef __osf__
					bzero ((char *)table->list, table->count * sizeof(PHASH));
#else
					bzero (table->list, table->count * sizeof(PHASH));
#endif
					slot = hash % table->count;
				}
				
				// Create and install a new bucket at head of list
				table->list[slot] = makeHashEntry (key, table->list[slot]);
				if (rocketLaunchingDebug)
					_NXLogError("C: (%p) %s ==> (%p) %s\n", key, key, key, key);
				
				// Desynchronize
				OBJC_UNLOCK (&selectorLock);
				return (SEL) key;
			}
		}
		
		// Try the backrefs
		else if (table->backrefs)
		{
			FixupEntry *	entry;
			const char *	sel;
			
			// Look for a matching entry
			entry = fixupEntryForSelector (key, table);
			sel = entry ? *(const char **) selRefForFixupEntryInTable (entry, table) : NULL;
			
			// Return the selector if an entry was found and it has a non-nil one
			// NOTE: Why would an entry have a NULL selector?
			if (sel)
			{
				if (strcmp (sel, key))
					_NXLogError ("bsearch failure...\n");
				if (rocketLaunchingDebug)
					_NXLogError("D: (%p) %s ==> (%p) %s\n", key, key, sel, sel);
				
				OBJC_UNLOCK (&selectorLock);
				return (SEL) sel;
			}
		}
	}
	
	// We searched all hashtables without finding the dynamic hashtable!
	abort ();
}

#ifndef FREEZE
/***********************************************************************
 * sel_registerName.  Public version of _sel_registerName, which assures
 * that the argument key is copied if nessecary.
 **********************************************************************/
SEL	sel_registerName	       (const char *	key)
{
	SEL	sel;
	
	sel = sel_getUid (key);
	if (sel) 
		return sel;
	
	return __S(_sel_registerName) (NXUniqueString (key));
}

/***********************************************************************
 * _sel_unloadSelectors.  Remove selectors in the specified memory range
 * from the dynamic hash table.  The range is from min to (max-1).
 *
 * NOTE: We currently leak the removed entries (because they are
 * allocated in chunks).  We could instead build a free chain out of
 * them to reuse.
 **********************************************************************/
void	__S(_sel_unloadSelectors)      (const char *	min,
										const char *	max)
{
	hashTable *	table;
	unsigned int	slot;
	PHASH *		ptr;
	PHASH		target;
	
	table = &dynamicHashTable;
	
	OBJC_LOCK (&selectorLock);
	
	for (slot = 0; slot < table->count; slot += 1)
	{
		ptr = &table->list[slot];
		while (*ptr)
		{			
			target = *ptr;
			if ((target->sel >= min) && (target->sel < max))
				*ptr = target->next;	// remove this entry
			else
				ptr  = &target->next;	// skip this entry
		}
	}
	
	OBJC_UNLOCK (&selectorLock);
}

/***********************************************************************
 * sel_getUid.  Return the selector for the specified string.  Returns
 * NULL if the string is not mapped.
 **********************************************************************/
SEL	sel_getUid	       (const char *	key)
{
	uarith_t	hash;
	hashTable *	table;
	
	// NULL selector for an NULL key
	if (!key) 
		return (SEL) 0;
	
	// Calculate hash value for the key
	hash = _objc_strhash (key);
	
	// Synchronize
	OBJC_LOCK (&selectorLock);
	
	// First check whether the key is in the dynamic hash table
	table = &dynamicHashTable;
	if (table->list != &sentinel)
	{
		unsigned int	slot;
		PHASH		target;

		// Search the slot corresponding to the hash value
		slot  = hash % table->count;
		for (target = table->list[slot]; target; target = target->next)
		{
			// Check for string equality
			if ((key[0] == target->sel[0]) && (strcmp (key, target->sel) == 0))
			{
				OBJC_UNLOCK (&selectorLock);
				if (rocketLaunchingDebug)
					_NXLogError("Z: (%p) %s ==> (%p) %s\n", key, key, target->sel, target->sel);
				
				return (SEL) target->sel;
			}
		}
	}
	
	// Second, check whether the key is in one of the frozen tables
	for (table = hashTableChain; table; table = table->next)
	{
		// Try quick range check
		if ((key >= table->min) && (key < table->max))
		{
			OBJC_UNLOCK (&selectorLock);
			return (SEL) key;
		}
		
		// Try dynamic list
		if (table->list)
		{
			PHASH		target;
			unsigned int	slot;
			
			slot = hash % table->count;
			for (target = table->list[slot]; target; target = target->next)
			{
				if ((key[0] == target->sel[0]) && (strcmp (key, target->sel) == 0))
				{
					OBJC_UNLOCK (&selectorLock);
					return (SEL) target->sel;
				}
			}
		}
		
		// Try the backrefs
		else if (table->backrefs)
		{
			FixupEntry *	entry;
			const char *	sel;
			
			// Look for a matching entry
			entry = (FixupEntry *) fixupEntryForSelector (key, table);
			sel   = entry ? *(const char **) selRefForFixupEntryInTable (entry, table) : NULL;
			
			// Return the selector if an entry was found and it has a non-nil one
			// NOTE: Why would an entry have a nil selector?
			if (sel)
			{
				if (strcmp (sel, key))
					_NXLogError("bsearch failure...\n");
				OBJC_UNLOCK (&selectorLock);
				return (SEL) sel;
			}
		}
	}
	
	OBJC_UNLOCK (&selectorLock);
	
	return (SEL) 0;
}
#endif

/***********************************************************************
 * _sel_initsorted.  Register a frozen table which is a sorted
 * array of FixupEntry elements.  This table will be bsearch'd rather
 * than treated as a true hash table.
 **********************************************************************/
void	__S(_sel_initsorted)   (const headerType *	header,
								void *				backrefs,
								void *				relativeTo,
								unsigned int		sectionSize)
{
	hashTable *		frozenTable;
	hashTable **	tablePtr;
	
	// Create a new frozen table with the specified characteristics
	frozenTable = NXZoneMalloc (NXDefaultMallocZone(), sizeof(hashTable));
	frozenTable->header		 = header;
	frozenTable->count		 = sectionSize / sizeof(FixupEntry);
	frozenTable->entries	 = 0;
	frozenTable->min		 = 0;
	frozenTable->max		 = 0;
	frozenTable->list		 = NULL;
	frozenTable->backrefs	 = backrefs;
	frozenTable->backrefBase = relativeTo;
	
	// Insert the frozen table at the end of the chain.
	for (tablePtr = &hashTableChain; *tablePtr; tablePtr = &(*tablePtr)->next)
	{
		// Check whether the next table is the dynamic one
		if (*tablePtr == &dynamicHashTable)
		{
			frozenTable->next = &dynamicHashTable;
			*tablePtr = frozenTable;
			break;
		}
	}
}

/***********************************************************************
 * _sel_init.  Register a frozen hash table.
 **********************************************************************/
void	__S(_sel_init) (const headerType *	header,
						const char *		strings,
						unsigned int		stringSize,
						void *				frozenHashTable)
{
	hashTable *		frozenTable;
	hashTable **	tablePtr;
	
	// Create a new frozen table with the specified characteristics
	frozenTable = NXZoneMalloc (NXDefaultMallocZone(), sizeof(hashTable));
	frozenTable->header		 = header;
	frozenTable->count		 = SIZEHASHTABLE;
	frozenTable->entries	 = 0;
	frozenTable->min		 = strings;
	// NOTE: strings + stringSize will be zero if the strings are
	// up against the end of the address space... can this happen?
	frozenTable->max		 = strings + stringSize;
	frozenTable->list		 = frozenHashTable;
	frozenTable->backrefs	 = NULL;
	frozenTable->backrefBase = 0;
	
	// Insert the frozen hashtable at the end of the chain.
	for (tablePtr = &hashTableChain; *tablePtr; tablePtr = &(*tablePtr)->next)
	{
		// Check whether the next table is the dynamic one
		if (*tablePtr == &dynamicHashTable)
		{
			frozenTable->next = &dynamicHashTable;
			*tablePtr = frozenTable;
			break;
		}
	}
}

#if !defined(KERNEL) && !defined(FREEZE)
/***********************************************************************
 * _sel_printHashTable.  Display the contents of all installed has
 * tables.
 *
 * NOTE: This crashes if there is a sorted table installed.
 **********************************************************************/
void	_sel_printHashTable	       (void)
{
	hashTable *	table;
	
	for (table = hashTableChain; table; table = table->next)
	{
		unsigned int	slot;
		
		_NXLogError ("%s selector hashtable for %s:\n",
				(table == &dynamicHashTable) ? "dynamic" : "freeze-dried",
				__S(_nameForHeader) (table->header));
		
		_NXLogError ("\n");
		
		for (slot = 0; slot < table->count; slot += 1)
		{
			PHASH target;
			
			_NXLogError ("slot[%u]: ", slot);
			
			for (target = table->list[slot]; target; target = target->next)
				_NXLogError (" %s", target->sel);
			
			_NXLogError ("\n");
		}
		
		_NXLogError ("\n");
	}
}

/***********************************************************************
 * _sel_resolve_conflicts.  Fixup the preuniquing conflicts that could
 * not be resolved in the binary.
 **********************************************************************/
void	_sel_resolve_conflicts		   (headerType *	header,
										unsigned long	slide)
{
	SEL **		conflicts;
	int			size;
	int			numConflicts;
	int			index;
	SEL			pCon1;
	SEL			pCon2;
	
	// Load the section listing the conflicts
	conflicts = _getObjcConflicts(header, &size);
	if (size == 0)
		return;
	
	// Calculate true address and entry count
	conflicts	 = (SEL **) ((unsigned long) conflicts + slide);
	numConflicts = size / sizeof(SEL *);
	
	// Loop over array of (SEL*)
	for (index = 1; index < numConflicts; index += 1)
	{
		pCon1 = *(conflicts[index-1]);
		pCon2 = *(conflicts[index]);
		if ((pCon1 != pCon2) && (strcmp((char *) pCon1, (char *) pCon2) == 0)) {
			if (rocketLaunchingDebug)
				_NXLogError("H: resolve sel 0x%.8X/0x%.8X from 0x%.8X/0x%.8X (%s)\n", conflicts[index], pCon2, conflicts[index-1], pCon1, pCon1);
			*(conflicts[index]) = pCon1;
		}
	}
}		

/***********************************************************************
 * _sel_printStatistics.  Summarize the usage of the hash tables.
 *
 * NOTE: This crashes if there is a sorted table installed.
 **********************************************************************/
void	_sel_printStatistics	       (void)
{
	hashTable *	table;
	unsigned int	totalCount			= 0;
	unsigned int	totalEntries		= 0;
	unsigned int	totalUniqueEntries	= 0;
	unsigned int	totalEmptySlots		= 0;
	unsigned int	grandTotalChain		= 0;
	unsigned int	grandTotalMissChain	= 0;
	unsigned int	maxMaxChain			= 0;
	unsigned int	maxMaxMissChain		= 0;
	unsigned int	totalMallocedData	= 0;
	unsigned int	totalSharedData		= 0;
	unsigned int	depth				= 0;
	unsigned int	totalDepth			= 0;
  
	for (table = hashTableChain; table; table = table->next)
	{
		unsigned int	slot;
		unsigned int	entries			= 0;
		unsigned int	uniqueEntries	= 0;
		unsigned int	emptySlots		= 0;
		unsigned int	totalChain		= 0;
		unsigned int	totalMissChain	= 0;
		unsigned int	maxChain		= 0;
		unsigned int	maxMissChain	= 0;
		unsigned int	mallocedData;
		unsigned int	sharedData;
		
		for (slot = 0; slot < table->count; slot += 1)
		{
			PHASH		target;
			unsigned int	chain = 0;
			
			for (target = table->list[slot]; target; target = target->next)
			{
				entries += 1;
				
				if (sel_getUid (target->sel) == (SEL) target->sel)
				{
					uniqueEntries += 1;
					
					chain += 1;
					
					if (chain > maxChain)
					maxChain = chain;
					
					totalChain += chain;
				}
			}
			
			if (chain > maxMissChain)
				maxMissChain = chain;
			
			totalMissChain += chain;
			
			if (chain == 0)
				emptySlots += 1;
		}
		
		_NXLogError ("%s selector hashtable for %s (depth = %u):\n",
				(table == &dynamicHashTable) ? "dynamic" : "freeze-dried",
				__S(_nameForHeader) (table->header),
				depth);
		
		_NXLogError ("%u entries in %u slots (%.2f entries/slot)\n",
				entries, table->count, (float) entries / (float) table->count);
		_NXLogError ("%u empty slots (%u%%)\n",
				emptySlots, (100 * emptySlots + 50) / table->count);
		_NXLogError ("%u redundant entries (%u%%)\n",
				entries - uniqueEntries,
				(100 * (entries - uniqueEntries) + 50) / entries);
		_NXLogError ("%.2f average lookup chain (%u maximum)\n",
				(float) totalChain / (float) uniqueEntries, maxChain);
		_NXLogError ("%.2f average miss chain (%u maximum)\n",
				(float) totalMissChain / (float) table->count, maxMissChain);
		
		if (table == &dynamicHashTable)
		{
			unsigned int	chunks;
			
			chunks	     = (entries + HASH_ALLOC_LIST_SIZE - 1) / HASH_ALLOC_LIST_SIZE;
			mallocedData = table->count * sizeof(PHASH) +
				       chunks * HASH_ALLOC_LIST_SIZE * sizeof(HASH);
			sharedData   = 0;
		}
		else
		{
			mallocedData = sizeof(hashTable);
			sharedData = table->count * sizeof(PHASH) + entries * sizeof(HASH);
		}
		
		if (sharedData)
			_NXLogError ("%u bytes malloced data + %u bytes shared data\n", mallocedData, sharedData);
		else
			_NXLogError ("%u bytes malloced data\n", mallocedData);
		
		totalCount		+= table->count;
		totalEntries		+= entries;
		totalUniqueEntries	+= uniqueEntries;
		totalEmptySlots		+= emptySlots;
		grandTotalChain		+= totalChain;
		grandTotalMissChain	+= totalMissChain;
		if (maxChain > maxMaxChain)
			maxMaxChain = maxChain;
		if (maxMissChain > maxMaxMissChain)
			maxMaxMissChain = maxMissChain;
		totalMallocedData	+= mallocedData;
		totalSharedData		+= sharedData;
		totalDepth		+= depth * uniqueEntries;
		depth			+= 1;
		
		_NXLogError ("\n");
	}
  
	_NXLogError ("cumulative selector hashtable statistics:\n");
	
	_NXLogError ("%u entries in %u slots (%.2f entries/slot)\n",
			totalEntries, totalCount, (float) totalEntries / (float) totalCount);
	_NXLogError ("%u empty slots (%u%%)\n",
			totalEmptySlots, (100 * totalEmptySlots + 50) / totalCount);
	_NXLogError ("%u redundant entries (%u%%)\n",
			totalEntries - totalUniqueEntries,
			(100 * (totalEntries - totalUniqueEntries) + 50) / totalEntries);
	_NXLogError ("%.2f average lookup depth (%u maximum)\n",
			(float) totalDepth / (float) totalUniqueEntries, depth - 1);
	_NXLogError ("%.2f average lookup chain (%u maximum)\n",
			(float) grandTotalChain / (float) totalUniqueEntries, maxMaxChain);
	_NXLogError ("%u miss depth\n", depth - 1);
	_NXLogError ("%.2f average miss chain (%u maximum)\n",
			(float) grandTotalMissChain / (float) totalCount, maxMaxMissChain);
	
	if (totalSharedData)
		_NXLogError ("%u bytes malloced data + %u bytes shared data\n", totalMallocedData, totalSharedData);
	else
		_NXLogError ("%u bytes malloced data\n", totalMallocedData);
}
#endif /* !defined(KERNEL) && !defined(FREEZE) */

#ifdef FREEZE
/***********************************************************************
 * _objc_fatal.  Log the specified message and quit the application.
 **********************************************************************/
void volatile	__S(_objc_fatal)       (const char *	msg)
{
	_NXLogError ("objc fatal: %s\n", msg);
	exit (1);
}

/***********************************************************************
 * _sel_writeHashTable.  Used by `objcopt' to freeze dry a hash table.
 **********************************************************************/
void	__S(_sel_writeHashTable)   (int			start_addr, 
									char *		myAddressSpace,
									char *		shlibAddressSpace,
									void **		addr,
									int *		size)
{
	hashTable *	table;
	unsigned int	frozenSize;
	PHASH *		frozenTable;
	PHASH		frozenEntry;
	PHASH		vm_hashEntry;
	unsigned int	slot;

	table = &dynamicHashTable;
	
	// Allocate zeroed table
	frozenSize	= table->count * sizeof(PHASH) + table->entries * sizeof(HASH);
	frozenTable	= NXZoneMalloc (NXDefaultMallocZone(), frozenSize);
	bzero (frozenTable, frozenSize);
	
	// The hash entries follow the slot heads.  Calculate the address of
	// the first entry in the the new frozenTable and in the caller's buffer.
	frozenEntry	= (PHASH) (frozenTable + table->count);
	vm_hashEntry	= (PHASH) (start_addr + (table->count * sizeof(PHASH)));
	
	// Major loop - process each slot in the hash table
	for (slot = 0; slot < table->count; slot += 1)
	{
		PHASH	target;
		
		// Skip unused slot... frozenTable[slot] remains zero
		if (!(table->list[slot]))
			continue;

		// Point slot head at next available entry in caller's buffer
		frozenTable[slot] = vm_hashEntry;
		
		// Minor loop - process each entry in the given slot
		// At the top of this loop:
		//	a) target is the first/next entry from the given slot
		//	b) frozenEntry is the next new entry to use
		//	c) vm_hashEntry is the next entry to use in
		//	   in the caller's buffer
		for (target = table->list[slot]; target; target = target->next)
		{
			// Copy selector address biased for caller's space
			frozenEntry->sel = (target->sel - myAddressSpace) +
						shlibAddressSpace;
			
			// If the current element has a successor, link it
			// the frozen entry to the buffer entry that will
			// be used for that successor
			if (target->next)
				frozenEntry->next = vm_hashEntry + 1;
			
			// Advance the entry pointers
			frozenEntry  += 1;
			vm_hashEntry += 1;
		}
	}
	
	// Return the table and its size
	*addr = frozenTable;
	*size = frozenSize;
}

/***********************************************************************
 * _nameForHeader.  Return the name string associated with the
 * specified header.
 *
 * NOTE: This function appears to be unused.  It is under the FREEZE
 * conditional, but the only calls to _nameForHeader are in !FREEZE
 * (which use a non-bogus function from objc-runtime.m).
 **********************************************************************/
const char *__S(_nameForHeader) (const headerType *	header)
{
	return "objcopt";
}
#endif /* FREEZE */
