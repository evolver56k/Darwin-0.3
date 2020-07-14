/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *	objc-sel.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 *	Public functions:		
 *	--------------
 *
 *	sel_getName
 *	sel_getUid
 *	sel_isMapped
 *	sel_registerName
 *	_strhash			(undocumented)
 *
 *	Private functions:
 *	---------------
 *
 *	_sel_init			(private extern)
 *	_sel_unloadSelectors		(private extern)
 *
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"

/* Number of hashtable buckets to malloc in a chunk */

#define HASH_ALLOC_LIST_SIZE 40

/* Size of the dynamic and freeze-dried hashtables */

#define SIZEHASHTABLE 821

   
typedef struct _hashEntry
{
  struct _hashEntry *next;
  const char *sel;
} HASH, *PHASH;

typedef struct _hashTable
{
  const struct mach_header *header;
  unsigned int count;
  unsigned int entries;
  const char *min;
  const char *max;
  PHASH *list;
  struct _hashTable *next;
} hashTable;

/* The dynamic hashtable contains all selectors which are not present in
   any freeze-dried hashtable.  Space for it is allocated only on demand.
   We use a sentinel to avoid extra tests for NULL. */

static const PHASH sentinel = NULL;

static hashTable dynamicHashTable =
{
  NULL,
  1,		// count
  0,		// entries
  NULL,		// Range checking does not work for the dynamic hashtable.
  NULL,
  (PHASH *) &sentinel,
  NULL,		// next
};


/* The list of all hashtables.  The freeze-dried hashtables are placed on
   the list in order of size, and the dynamic hashtable is always last. */

static hashTable *hashTableChain = &dynamicHashTable;

#ifndef FREEZE
/* Lock for all static data in objc-sel.m */

static OBJC_DECLARE_LOCK (selectorLock);
#endif

/* This is a duplicate of a static function in objc-class.m!  We should
   make this a private extern. */

static void *objc_malloc (unsigned int size)
{
  NXZone *zone;
  void *space;
  
#ifdef FREEZE
  zone = NXDefaultMallocZone ();
#else
  zone = _objc_create_zone ();
#endif
  
  space = NXZoneMalloc (zone, size);
  
  if (space == 0 && size != 0)
    __S(_objc_fatal) ("unable to allocate space");

  return space;
}


/* For compatibility only.  This function is public even though it never
   should have been. */

#ifndef FREEZE
unsigned int _strhash (const unsigned char *s)
{
  return _objc_strhash (s);
}
#endif

/* Allocate and initialize a new bucket for the dynamic hashtable.
   The selector lock is assumed to be already taken out. */

static inline PHASH opthashNew (const char *key, PHASH link)
{
  static PHASH HASH_alloc_list = 0;
  static int HASH_alloc_index = 0;
  PHASH new;

  if (!HASH_alloc_list || HASH_alloc_index >= HASH_ALLOC_LIST_SIZE)
    {
      HASH_alloc_list = objc_malloc (sizeof (HASH) * HASH_ALLOC_LIST_SIZE);
      HASH_alloc_index = 0;
    }
  
  new = &HASH_alloc_list[HASH_alloc_index++];
  new->next = link;
  new->sel = key;
  return new;
}


#ifndef FREEZE
/* This is no longer used.  We should remove this from the API. */

BOOL sel_isMapped (SEL sel)
{
  hashTable *table;
  
  if (sel == (SEL) 0) 
    return NO;
  
  OBJC_LOCK (&selectorLock);
  
  for (table = hashTableChain; table; table = table->next)
    {
      if ((const char *) sel >= table->min && (const char *) sel < table->max)
        {
	  OBJC_UNLOCK (&selectorLock);
	  return YES;
	}
      else if (table == &dynamicHashTable)
        {
	  unsigned int slot;
	  
	  for (slot = 0; slot < table->count; slot++)
	    {
	      PHASH target;
	      
	      for (target = table->list[slot]; target; target = target->next)
		if (sel == (SEL)target->sel)
		  {
		    OBJC_UNLOCK (&selectorLock);
		    return YES;
		  }
	    }
	}
    }
  
  OBJC_UNLOCK (&selectorLock);
  
  return NO;
}
#endif

#ifndef FREEZE
/* This is now very fast!
   Note that previously we would return NULL if passed an invalid selector. */

const char *sel_getName (SEL sel)
{
  return (const char *) sel;
}
#endif

/* Add a new selector if not already present.  Formerly _sel_registerName(). 
   Now again _sel_registerName(), and a public safe variant added. */

SEL __S(_sel_registerName) (const char *key)
{
  unsigned int hash;
  hashTable *table;
  
  if (!key) 
    return (SEL) 0;
  
  hash = _objc_strhash (key);
  
  OBJC_LOCK (&selectorLock);
  
  for (table = hashTableChain; table; table = table->next)
    {
      if (key >= table->min && key < table->max)
        {
	  OBJC_UNLOCK (&selectorLock);
	  return (SEL) key;
	}
      else
        {
	  PHASH target;
	  unsigned int slot = hash % table->count;
	  
	  for (target = table->list[slot]; target; target = target->next)
	    if (key[0] == target->sel[0] &&
		strcmp (key, target->sel) == 0)
	      {
		OBJC_UNLOCK (&selectorLock);
		return (SEL) target->sel;
	      }
	  
	  /* If we have reached the dynamic hashtable, insert a new entry. */
	  if (table == &dynamicHashTable)
	    {
	      table->entries++;
	      
	      /* Allocate hashtable if needed. */
	      if (table->list == &sentinel)
		{
		  /* Allocate a new empty array. */
		  table->count = SIZEHASHTABLE;
		  table->list = objc_malloc (table->count * sizeof (PHASH));
		  bzero (table->list, table->count * sizeof (PHASH));
		  slot = hash % table->count;
		}
	      
	      table->list[slot] = opthashNew (key, table->list[slot]);
	      OBJC_UNLOCK (&selectorLock);
	      return (SEL) key;
	    }
	}
    }
  
  /* We searched all hashtables without finding the dynamic hashtable! */
  abort ();
}

/*
 *  Public version of _sel_registerName, which assures that the
 *  argument key is copied if nessecary.
 */

#ifndef FREEZE
SEL sel_registerName (const char *key)
{
   SEL s = sel_getUid (key);
   if (s) 
     return s;
   else
     return __S(_sel_registerName) (NXUniqueString (key));
}
#endif

/* Remove all of the selectors associated with a dynamically loaded module.
  We currently leak the removed entries (because they are allocated in
  chunks).  We could instead build a free chain out of them to reuse. */

#ifndef FREEZE
void __S(_sel_unloadSelectors) (const char *min, const char *max)
{
  hashTable *table = &dynamicHashTable;
  unsigned int slot;
  
  OBJC_LOCK (&selectorLock);
  
  for (slot = 0; slot < table->count; slot++)
    {
      PHASH *ptr = &table->list[slot];
      
      while (*ptr)
        {
	  PHASH target = *ptr;
	  
	  if (target->sel >= min && target->sel < max)
	    *ptr = target->next;	/* Remove this entry */
	  else
	    ptr = &target->next;	/* Skip */
        }
    }
  
  OBJC_UNLOCK (&selectorLock);
}
#endif

/* Search each hashtable in the chain for the string. */

#ifndef FREEZE
SEL sel_getUid (const char *key)
{
  unsigned int hash;
  hashTable *table;
  
  if (!key) 
    return (SEL) 0;
  
  hash = _objc_strhash (key);
  
  OBJC_LOCK (&selectorLock);
  
  for (table = hashTableChain; table; table = table->next)
    {
      if (key >= table->min && key < table->max)
        {
	  OBJC_UNLOCK (&selectorLock);
	  return (SEL) key;
	}
      else
        {
	  PHASH target;
	  unsigned int slot = hash % table->count;
	  
	  for (target = table->list[slot]; target; target = target->next)
	    if (key[0] == target->sel[0] &&
		strcmp (key, target->sel) == 0)
	      {
		OBJC_UNLOCK (&selectorLock);
		return (SEL) target->sel;
	      }
	}
    }
  
  OBJC_UNLOCK (&selectorLock);
  
  return (SEL) 0;
}
#endif

/* Register a freeze-dried hashtable. */

void __S(_sel_init) (const struct mach_header *header,
		const char *strings,
		unsigned int stringSize,
		void *frozenHashTable)
{
  hashTable *frozenTable = NXZoneMalloc (NXDefaultMallocZone(), sizeof (hashTable));
  hashTable **tablePtr;
  
  frozenTable->header = header;
  frozenTable->count = SIZEHASHTABLE;
  frozenTable->entries = 0;
  frozenTable->min = strings;
  frozenTable->max = strings + stringSize;
  frozenTable->list = frozenHashTable;
  
  /* Insert the freeze-dried hashtable at the end of the hashtable chain. */
  
  for (tablePtr = &hashTableChain; *tablePtr; tablePtr = &(*tablePtr)->next)
    if (*tablePtr == &dynamicHashTable)
      {
        frozenTable->next = &dynamicHashTable;
	*tablePtr = frozenTable;
	break;
      }
}


#if  !defined(KERNEL) && !defined(FREEZE)

void _sel_printHashTable (void)
{
  hashTable *table;
  
  for (table = hashTableChain; table; table = table->next)
    {
      unsigned int slot;
      
      printf ("%s selector hashtable for %s:\n",
	      (table == &dynamicHashTable) ? "dynamic" : "freeze-dried",
	      __S(_nameForHeader) (table->header));
      
      printf ("\n");
      
      for (slot = 0; slot < table->count; slot++)
	{
	  PHASH target;
	  
	  printf ("slot[%u]: ", slot);
	  
	  for (target = table->list[slot]; target; target = target->next)
	    printf (" %s", target->sel);
	  
	  printf ("\n");
	}
      
      printf ("\n");
    }
}


void _sel_printStatistics (void)
{
  hashTable *table;
  unsigned int totalCount = 0;
  unsigned int totalEntries = 0;
  unsigned int totalUniqueEntries = 0;
  unsigned int totalEmptySlots = 0;
  unsigned int grandTotalChain = 0;
  unsigned int grandTotalMissChain = 0;
  unsigned int maxMaxChain = 0;
  unsigned int maxMaxMissChain = 0;
  unsigned int totalMallocedData = 0;
  unsigned int totalSharedData = 0;
  unsigned int depth = 0;
  unsigned int totalDepth = 0;
  
  for (table = hashTableChain; table; table = table->next)
    {
      unsigned int slot;
      unsigned int entries = 0;
      unsigned int uniqueEntries = 0;
      unsigned int emptySlots = 0;
      unsigned int totalChain = 0;
      unsigned int totalMissChain = 0;
      unsigned int maxChain = 0;
      unsigned int maxMissChain = 0;
      unsigned int mallocedData;
      unsigned int sharedData;
      
      for (slot = 0; slot < table->count; slot++)
	{
	  PHASH target;
	  unsigned int chain = 0;
	  
	  for (target = table->list[slot]; target; target = target->next)
	    {
	      entries++;
	      
	      if (sel_getUid (target->sel) == (SEL) target->sel)
		{
		  uniqueEntries++;
	      
		  chain++;
		  
		  if (chain > maxChain)
		    maxChain = chain;
		  
		  totalChain += chain;
		}
	    }
	  
	  if (chain > maxMissChain)
	    maxMissChain = chain;
	  
	  totalMissChain += chain;
	  
	  if (chain == 0)
	    emptySlots++;
	}
      
      printf ("%s selector hashtable for %s (depth = %u):\n",
	      (table == &dynamicHashTable) ? "dynamic" : "freeze-dried",
	      __S(_nameForHeader) (table->header),
	      depth);
      
      printf ("%u entries in %u slots (%.2f entries/slot)\n",
	      entries, table->count, (float) entries / (float) table->count);
      printf ("%u empty slots (%u%%)\n",
	      emptySlots, (100 * emptySlots + 50) / table->count);
      printf ("%u redundant entries (%u%%)\n",
	      entries - uniqueEntries,
	      (100 * (entries - uniqueEntries) + 50) / entries);
      printf ("%.2f average lookup chain (%u maximum)\n",
	      (float) totalChain / (float) uniqueEntries, maxChain);
      printf ("%.2f average miss chain (%u maximum)\n",
	      (float) totalMissChain / (float) table->count, maxMissChain);
      if (table == &dynamicHashTable)
	{
	  unsigned int chunks = (entries + HASH_ALLOC_LIST_SIZE - 1) /
				HASH_ALLOC_LIST_SIZE;
	  
	  mallocedData = table->count * sizeof (PHASH) +
			 chunks * HASH_ALLOC_LIST_SIZE * sizeof (HASH);
	  sharedData = 0;
	}
      else
        {
	  mallocedData = sizeof (hashTable);
	  sharedData = table->count * sizeof (PHASH) + entries * sizeof (HASH);
        }
      
      if (sharedData)
	printf ("%u bytes malloced data + %u bytes shared data\n",
		mallocedData, sharedData);
      else
	printf ("%u bytes malloced data\n", mallocedData);
      
      totalCount += table->count;
      totalEntries += entries;
      totalUniqueEntries += uniqueEntries;
      totalEmptySlots += emptySlots;
      grandTotalChain += totalChain;
      grandTotalMissChain += totalMissChain;
      if (maxChain > maxMaxChain)
        maxMaxChain = maxChain;
      if (maxMissChain > maxMaxMissChain)
        maxMaxMissChain = maxMissChain;
      totalMallocedData += mallocedData;
      totalSharedData += sharedData;
      totalDepth += depth * uniqueEntries;
      depth++;
      
      printf ("\n");
    }
  
  printf ("cummulative selector hashtable statistics:\n");
  
  printf ("%u entries in %u slots (%.2f entries/slot)\n",
	  totalEntries, totalCount, (float) totalEntries / (float) totalCount);
  printf ("%u empty slots (%u%%)\n",
	  totalEmptySlots, (100 * totalEmptySlots + 50) / totalCount);
  printf ("%u redundant entries (%u%%)\n",
	  totalEntries - totalUniqueEntries,
	  (100 * (totalEntries - totalUniqueEntries) + 50) / totalEntries);
  printf ("%.2f average lookup depth (%u maximum)\n",
	  (float) totalDepth / (float) totalUniqueEntries,
	  depth - 1);
  printf ("%.2f average lookup chain (%u maximum)\n",
	  (float) grandTotalChain / (float) totalUniqueEntries,
	  maxMaxChain);
  printf ("%u miss depth\n", depth - 1);
  printf ("%.2f average miss chain (%u maximum)\n",
	  (float) grandTotalMissChain / (float) totalCount,
	  maxMaxMissChain);
  
  if (totalSharedData)
    printf ("%u bytes malloced data + %u bytes shared data\n",
	    totalMallocedData, totalSharedData);
  else
    printf ("%u bytes malloced data\n", totalMallocedData);
}

#endif /* KERNEL */

#ifdef FREEZE

void volatile __S(_objc_fatal) (const char *msg)
{
  printf ("objc fatal: %s\n", msg);
  exit (1);
}


/* used by `objcopt' to freeze dry a hash table */

void __S(_sel_writeHashTable) (int start_addr, 
			  char *myAddressSpace,
			  char *shlibAddressSpace,
			  void **addr, int *size)
{
  hashTable *table = &dynamicHashTable;
  unsigned int frozenSize = table->count * sizeof (PHASH) +
			    table->entries * sizeof (HASH);
  PHASH *frozenTable = NXZoneMalloc (NXDefaultMallocZone(), frozenSize);
  PHASH frozenEntries = (PHASH) (frozenTable + table->count);
  PHASH vm_hashEntryList = (PHASH) (start_addr +
				    table->count * sizeof (PHASH));
  unsigned int slot;
  
  bzero (frozenTable, frozenSize);
  
  for (slot = 0; slot < table->count; slot++)
    if (table->list[slot])
      {
	PHASH target;
	
	frozenTable[slot] = vm_hashEntryList;
	
	for (target = table->list[slot]; target; target = target->next)
	  {
	    frozenEntries->sel = (target->sel - myAddressSpace) +
				 shlibAddressSpace;
	    if (target->next)
	      frozenEntries->next = vm_hashEntryList + 1;
	    
	    frozenEntries++;
	    vm_hashEntryList++;
	  }
      }
  
  *addr = frozenTable;
  *size = frozenSize;
}

const char *__S(_nameForHeader) (const struct mach_header *header)
{
  return "objcopt";
}

#endif /* FREEZE */
