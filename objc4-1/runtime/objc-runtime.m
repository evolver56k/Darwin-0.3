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
 * objc-runtime.m
 * Copyright 1988-1996, NeXT Software, Inc.
 * Author:	s. naroff
 *
 **********************************************************************/

/***********************************************************************
 * Imports.
 **********************************************************************/

#if defined(WIN32)
#include <winnt-pdo.h>
#endif

#if defined(NeXT_PDO)
#import <pdo.h>		// for pdo_malloc and pdo_free defines
#elif defined(__MACH__)
#include <mach-o/ldsyms.h>
#include <mach-o/dyld.h>
#include <mach/vm_statistics.h>
#endif

#import <objc/objc-class.h>
#import <objc/objc-runtime.h>
#import <objc/hashtable2.h>
#import "maptable.h"
#import "objc-private.h"
#import "objc-sel.h"
#import <objc/Object.h>
#import <objc/Protocol.h>

#if !defined(WIN32)
#include <sys/time.h>
#include <sys/resource.h>
#endif

OBJC_EXPORT Class		_objc_getNonexistentClass	(void);

#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
OBJC_EXPORT void		(*load_class_callback)		(Class, Category);
OBJC_EXPORT unsigned int	_objc_goff_headerCount		(void);
OBJC_EXPORT header_info *	_objc_goff_headerVector		(void);
#endif 

OBJC_EXPORT Class		getOriginalClassForPosingClass	(Class);

#if defined(WIN32) || defined(__svr4__)
// Current Module Header
extern objcModHeader *		CMH;
#endif

/***********************************************************************
 * Constants and macros internal to this module.
 **********************************************************************/

/* Turn on support for literal string objects. */
#define LITERAL_STRING_OBJECTS

/* Work around bug in dyld. */
#define DYLD_MODULE_INFO_WRONG	1

/***********************************************************************
 * Types internal to this module.
 **********************************************************************/

typedef struct _objc_unresolved_category
{
	struct _objc_unresolved_category *	next;
	struct objc_category *			cat;
	long					version;
} _objc_unresolved_category;

typedef struct _PendingClass
{
	Class *			ref;
	Class			classToSetUp;
	const char *		nameof_superclass;
	int			version;
	struct _PendingClass *	next;
} PendingClass;

/***********************************************************************
 * Exports.
 **********************************************************************/

// Mask which specifies whether we are multi-threaded or not.
// A value of (-1) means single-threaded, 0 means multi-threaded. 
int		_objc_multithread_mask = (-1);

// Function to call when message sent to nil object.
void		(*_objc_msgNil)(id, SEL) = NULL;

// Function called after class has been fixed up (MACH only)
void		(*callbackFunction)(Class, Category) = 0;

// Prototype for function passed to 
typedef void (*NilObjectMsgCallback) (id nilObject, SEL selector);

// Lock for class hashtable
OBJC_DECLARE_LOCK (classLock);

// Condition for logging load progress
BOOL		rocketLaunchingDebug = NO;

/***********************************************************************
 * Function prototypes internal to this module.
 **********************************************************************/

static unsigned			classHash							(void * info, Class data);
static int				classIsEqual						(void * info, Class name, Class cls);
#if defined(BUG67896)
static void				_NXNoEffectFree						(const void * info, void * data);
#endif
static int				_objc_defaultClassHandler			(const char * clsName);
static void				_objcTweakMethodListPointerForClass	(Class cls);
static void				__objc_add_category					(struct objc_category * category, int version);
static void				_objc_resolve_categories_for_class	(Class cls);
static void				_objc_register_category				(struct objc_category *	cat, long version);
static void				_objc_add_categories_from_image		(header_info * hi);
#if defined(__MACH__)
static const header_info * _headerForClass					(Class cls);
static void				pend_map_selectors					(void);
#endif
static void				checkForPendingClassReferences		(Class cls);
static PendingClass *	newPending							(void);
static NXMapTable *		pendingClassRefsMapTable			(void);
static NXHashTable *	_objc_get_classes_from_image		(NXHashTable * clsHash, header_info * hi);
static void				_objc_fixup_string_objects_for_image(header_info * hi);
static void				_objc_map_class_refs_for_image		(header_info * hi);
static void				map_selrefs							(SEL * sels, unsigned int cnt);
static void				map_methods							(struct objc_method_list * methods);
static void				map_method_descs					(struct objc_method_description_list * methods);
static void				_objc_fixup_protocol_objects_for_image	(header_info * hi);
#if defined(__MACH__)
static int				qsortHeaders						(const void * x1, const void * x2);
#if defined(DYLD_MODULE_INFO_WRONG)
static void				_objc_registerMethodListFromCategory(Category cat);
static void				_objc_registerMethodListFromClass	(Class cls);
#endif
#endif
static const char *	libraryNameForMachHeader				(const headerType * themh);
#if defined(__MACH__)
static void				undo_preuniqing_for_image			(const header_info * hi);
#endif
static void				_objc_fixup_selector_refs			(const header_info * hi);
#if defined(__MACH__)
static int				sortedRefsSize						(const header_info * info);
static int				qsortBackrefsSectionSize			(const void * v1, const void * v2);
static void				do_pended_map_selectors				(void);
#endif
#if defined(NeXT_PDO)
static void				_objc_map_selectors_from_image		(header_info * hi);
#endif
static void				_objc_call_loads_for_image			(header_info * header);
#if defined(__MACH__)
static void				_objc_map_image_callback			(headerType * mh, unsigned long vmaddr_slide);
static void				_objc_link_module_callback			(NSModule mod);
static void				_objc_unlink_module_callback		(NSModule mod);
#endif

/***********************************************************************
 * Static data internal to this module.
 **********************************************************************/

// System vectors created at runtime by reading the `__OBJC' segments 
// that are a part of the application.
// We do not lock these variables, since they are only set during startup. 
// NOTE: module_count and module_vector are now obsolete.  They have
// been replaced by header_vector and header_count.
static unsigned int		module_count;
static Module *			module_vector;
static header_info *	header_vector = 0;
static unsigned int		header_count = 0;

// Hash table of classes
static NXHashTable *		class_hash = 0;
static NXHashTablePrototype	classHashPrototype = 
{
	(unsigned (*) (const void *, const void *))			classHash, 
	(int (*)(const void *, const void *, const void *))	classIsEqual, 
#if defined(BUG67896)
	_NXNoEffectFree, 0
#else
	NXNoEffectFree, 0
#endif
};

// Function pointer objc_getClass calls through when class is not found
static int			(*objc_classHandler) (const char *) = _objc_defaultClassHandler;

// Category and class registries
static NXMapTable *		category_hash = NULL;
#if defined(DYLD_MODULE_INFO_WRONG)
static NXMapTable *		categoryMethodMap = NULL;
#endif
static NXMapTable *		classMethodMap = NULL;

#if defined(__MACH__)
static BOOL				rocketLaunching = NO;
#endif


enum
{
	pended_map_selectors_growth	= 20
};
static struct header_info **	pended_map_selectors		= NULL;
static int						pended_map_selectors_size	= 0;
static int						pended_map_selectors_count	= 0;
static int						map_selectors_pended		= 0;

static NXMapTable *		pendingClassRefsMap = 0;

/***********************************************************************
 * objc_dump_class_hash.  Log names of all known classes.
 **********************************************************************/
void	objc_dump_class_hash	       (void)
{
	NXHashTable *	table;
	unsigned		count;
	Class	*		data;
	NXHashState		state;
	
	table = objc_getClasses ();
	count = 0;
	state = NXInitHashState (table);
	while (NXNextHashState (table, &state, (void **) &data))
		_NXLogError ("class %d: %s\n", ++count, (*data)->name);
}

/***********************************************************************
 * classHash.
 **********************************************************************/
static unsigned		classHash	       (void *		info,
										Class		data) 
{
	// Nil classes hash to zero
	if (!data)
		return 0;
	
	// Call through to real hash function
	return _objc_strhash ((unsigned char *) ((Class) data)->name);
}

/***********************************************************************
 * classIsEqual.  Returns whether the class names match.  If we ever
 * check more than the name, routines like objc_lookUpClass have to
 * change as well.
 **********************************************************************/
static int		classIsEqual	       (void *		info,
										Class		name,
										Class		cls) 
{
	// Standard string comparison
	return ((name->name[0] == cls->name[0]) &&
		(strcmp (name->name, cls->name) == 0));
}

#if defined(BUG67896)
/***********************************************************************
 * _NXNoEffectFree.  Cover function for NXNoEffectFree.
 **********************************************************************/
static void	_NXNoEffectFree	       (const void *	info,
									void *			data)
{
	// Call through
	return NXNoEffectFree (info, data);
}
#endif

/***********************************************************************
 * _objc_init_class_hash.  Return the class lookup table, create it if
 * necessary.
 **********************************************************************/
void	_objc_init_class_hash	       (void)
{
	// Do nothing if class hash table already exists
	if (class_hash)
		return;
	
	// Provide a generous initial capacity to cut down on rehashes
	// at launch time.  A smallish Foundation+AppKit program will have
	// about 520 classes.  Larger apps (like IB or WOB) have more like
	// 800 classes.  Some customers have massive quantities of classes.
	// Foundation-only programs aren't likely to notice the ~6K loss.
	class_hash = NXCreateHashTableFromZone (classHashPrototype,
						1024,
						nil,
						_objc_create_zone ());
}

/***********************************************************************
 * objc_getClasses.  Return class lookup table.
 *
 * NOTE: This function is very dangerous, since you cannot safely use
 * the hashtable without locking it, and the lock is private! 
 **********************************************************************/
NXHashTable *		objc_getClasses	       (void)
{
#if defined(NeXT_PDO)
	// Make sure a hash table exists
	if (!class_hash)
		_objc_init_class_hash ();
#endif

	// Return the class lookup hash table
	return class_hash;
}

/***********************************************************************
 * _objc_defaultClassHandler.  Default objc_classHandler.  Does nothing.
 **********************************************************************/
static int	_objc_defaultClassHandler      (const char *	clsName)
{
	// Return zero so objc_getClass doesn't bother re-searching
	return 0;
}

/***********************************************************************
 * objc_setClassHandler.  Set objc_classHandler to the specified value.
 *
 * NOTE: This should probably deal with userSuppliedHandler being NULL,
 * because the objc_classHandler caller does not check... it would bus
 * error.  It would make sense to handle NULL by restoring the default
 * handler.  Is anyone hacking with this, though?
 **********************************************************************/
void	objc_setClassHandler	(int	(*userSuppliedHandler) (const char *))
{
	objc_classHandler = userSuppliedHandler;
}

/***********************************************************************
 * objc_getClass.  Return the id of the named class.  If the class does
 * not exist, call the objc_classHandler routine with the class name.
 * If the objc_classHandler returns a non-zero value, try once more to
 * find the class.  Default objc_classHandler always returns zero.
 * objc_setClassHandler is how someone can install a non-default routine.
 **********************************************************************/
id		objc_getClass	       (const char *	aClassName)
{ 
	struct objc_class	cls;
	id					ret;

	// Synchronize access to hash table
	OBJC_LOCK (&classLock);
	
	// Check the hash table
	cls.name = aClassName;
	ret = (id) NXHashGet (objc_getClasses (), &cls);
	OBJC_UNLOCK (&classLock);
	
	// If not found, go call objc_classHandler and try again
	if (!ret && (*objc_classHandler)(aClassName))
	{
		OBJC_LOCK (&classLock);
		ret = (id) NXHashGet (objc_getClasses (), &cls);
		OBJC_UNLOCK (&classLock);
	}

	return ret;
}

/***********************************************************************
 * objc_lookUpClass.  Return the id of the named class.
 *
 * Formerly objc_getClassWithoutWarning ()
 **********************************************************************/
id		objc_lookUpClass       (const char *	aClassName)
{ 
	struct objc_class	cls;
	id					ret;
	
	// Synchronize access to hash table
	OBJC_LOCK (&classLock);

	// Check the hash table
	cls.name = aClassName;
	ret = (id) NXHashGet (objc_getClasses (), &cls);
	
	// Desynchronize
	OBJC_UNLOCK (&classLock);
	return ret;
}

/***********************************************************************
 * objc_getMetaClass.  Return the id of the meta class the named class.
 **********************************************************************/
id		objc_getMetaClass       (const char *	aClassName) 
{ 
	Class	cls;
	
	cls = objc_getClass (aClassName);
	if (!cls)
	{
		_objc_inform ("class `%s' not linked into application", aClassName);
		return Nil;
	}

	return cls->isa;
}

/***********************************************************************
 * objc_addClass.  Add the specified class to the table of known classes,
 * after doing a little verification and fixup.
 **********************************************************************/
void		objc_addClass		(Class		cls) 
{
	// Synchronize access to hash table
	OBJC_LOCK (&classLock);
	
	// Make sure both the class and the metaclass have caches!
	// Clear all bits of the info fields except CLS_CLASS and CLS_META.
	// Normally these bits are already clear but if someone tries to cons
	// up their own class on the fly they might need to be cleared.
	if (cls->cache == NULL)
	{
		cls->cache = (Cache) &emptyCache;
		cls->info = CLS_CLASS;
	}
	
	if (cls->isa->cache == NULL)
	{
		cls->isa->cache = (Cache) &emptyCache;
		cls->isa->info = CLS_META;
	}
	
	// Add the class to the table
	(void) NXHashInsert (objc_getClasses (), cls);

	// Desynchronize
	OBJC_UNLOCK (&classLock);
}

/***********************************************************************
 * _objc_removeClass.  Remove the specified class from the table of
 * known classes.
 *
 * Private extern used by objc_unloadModules ()
 **********************************************************************/
void		_objc_removeClass	       (Class	cls)
{
	// Synchronize access to hash table
	OBJC_LOCK (&classLock);

	NXHashRemove (objc_getClasses (), cls);

	// Desynchronize
	OBJC_UNLOCK (&classLock);
}

/***********************************************************************
 * objc_getModules.  Create and/or return the vector of known modules.
 *
 * NOTE: This only returns the statically linked modules.  I do not
 * believe this is used now.  Replaced by _objc_headerVector ().
 **********************************************************************/
Module *	objc_getModules		       (void)
{
	Module			mods;
	unsigned int	hidx;
	unsigned int	midx;
	Module *		mvp;

	// Return what we already have
	if (module_vector)
		return module_vector;

	// Set overall module count as some of all mod_count's
	for (hidx = 0; hidx < header_count; hidx += 1)
		module_count += header_vector[hidx].mod_count;
	
	// Allocate a vector for that many entries (plus a nil one)
	module_vector = NXZoneMalloc (_objc_create_zone (), sizeof(Module) * (module_count + 1));
	if (!module_vector)
		_objc_fatal ("unable to allocate module vector");
	
	// Initialize vector entries to point to each module
	// Major loop - process every header
	mvp = module_vector;
	for (hidx = 0; hidx < header_count; hidx += 1)
	{
		// Minor loop - process every module in given header
		mods = header_vector[hidx].mod_ptr;
		for (midx = 0;
		     midx < header_vector[hidx].mod_count;
		     midx += 1, mvp += 1)
			*mvp = mods + midx;
	}
	
	// Place end entry
	*mvp = 0;
	
	return module_vector;
}

/***********************************************************************
 * _objc_remove_category.
 *
 * NOTE: Assumes that the category's pointers to the method lists and
 * protocol list are the same as when the category was installed.
 *
 * Private extern used by objc_unloadModules ()
 **********************************************************************/
void	_objc_remove_category	       (struct objc_category *	category,
										int						version)
{
	Class		cls;
	
	// Look up the class this category augments
	cls = objc_getClass (category->class_name);
	if (!cls)
	{
		_objc_inform ("unable to remove category %s...\n", category->category_name);
		_objc_inform ("class `%s' not linked into application\n", category->class_name);
		return;
	}

	// Remove instance methods added by this category
	if (category->instance_methods)
		class_removeMethods (cls, category->instance_methods);

	// Remove class methods added by this category
	if (category->class_methods)
		class_removeMethods (cls->isa, category->class_methods);
	
	// Remove protocols added by this category
	if ((version >= 5) && (category->protocols))
		_class_removeProtocols (cls, category->protocols);
}

/***********************************************************************
 * _objcTweakMethodListPointerForClass.
 **********************************************************************/
static void	_objcTweakMethodListPointerForClass     (Class	cls)
{
	struct objc_method_list *	originalList;
	const int					initialEntries = 4;
	int							mallocSize;
	struct objc_method_list **	ptr;
	
	// Remember existing list
	originalList = (struct objc_method_list *) cls->methodLists;
	
	// Allocate and zero a method list array
	mallocSize   = sizeof(struct objc_method_list *) * initialEntries;
	ptr	     = (struct objc_method_list **) NXZoneMalloc (_objc_create_zone (), mallocSize);
	bzero (ptr, mallocSize);
	
	// Insert the existing list into the array
	ptr[initialEntries - 1] = END_OF_METHODS_LIST;
	ptr[0] = originalList;
	
	// Replace existing list with array
	cls->methodLists = ptr;
	cls->info |= CLS_METHOD_ARRAY;
	
	// Do the same thing to the meta-class
	if (((cls->info & CLS_CLASS) != 0) && cls->isa)
		_objcTweakMethodListPointerForClass (cls->isa);
}

/***********************************************************************
 * _objc_insertMethods.
 **********************************************************************/
void	_objc_insertMethods    (struct objc_method_list *	mlist,
								struct objc_method_list ***	list)
{
	struct objc_method_list **			ptr;
	volatile struct objc_method_list **	tempList;
	int									endIndex;
	int									oldSize;
	int									newSize;
	
	// Locate unused entry for insertion point
	ptr = *list;
	while ((*ptr != 0) && (*ptr != END_OF_METHODS_LIST))
		ptr += 1;
	
	// If array is full, double it
	if (*ptr == END_OF_METHODS_LIST)
	{
		// Calculate old and new dimensions
		endIndex = ptr - *list;
		oldSize  = (endIndex + 1) * sizeof(void *);
		newSize  = oldSize * 2;
		
		// Replace existing array with copy twice its size
		tempList = (struct objc_method_list **) NXZoneRealloc ((NXZone *) _objc_create_zone (),
								      						   (void *) *list,
														       (size_t) newSize);
		*list = tempList;
		
		// Zero out addition part of new array
		bzero (&((*list)[endIndex]), newSize - oldSize);
		
		// Place new end marker
		(*list)[(newSize/sizeof(void *)) - 1] = END_OF_METHODS_LIST;
		
		// Insertion point corresponds to old array end
		ptr = &((*list)[endIndex]);
	}
	
	// Right shift existing entries by one 
	bcopy (*list, (*list) + 1, ((void *) ptr) - ((void *) *list));
	
	// Insert at method list at beginning of array
	**list = mlist;
}

/***********************************************************************
 * _objc_removeMethods.
 **********************************************************************/
void	_objc_removeMethods    (struct objc_method_list *	mlist,
								struct objc_method_list ***	list)
{
	struct objc_method_list **	ptr;
 
	// Locate list in the array
	ptr = *list;
	while (*ptr != mlist)
		ptr += 1;
	
	// Remove this entry
	*ptr = 0;
	
	// Left shift the following entries 
	while (*(++ptr) != END_OF_METHODS_LIST)
		*(ptr-1) = *ptr;
		*(ptr-1) = 0;
}

/***********************************************************************
 * __objc_add_category.  Install the specified category's methods and
 * protocols into the class it augments.
 **********************************************************************/
static inline void	__objc_add_category    (struct objc_category *	category,
											int						version)
{
	Class	cls;
	
	// Locate the class that the category will extend
	cls = (Class) objc_getClass (category->class_name);
	if (!cls)
	{
		_objc_inform ("unable to add category %s...\n", category->category_name);
		_objc_inform ("class `%s' not linked into application\n", category->class_name);
		return;
	}

	// Augment instance methods
	if (category->instance_methods)
		_objc_insertMethods (category->instance_methods, &cls->methodLists);

	// Augment class methods
	if (category->class_methods)
		_objc_insertMethods (category->class_methods, &cls->isa->methodLists);

	// Augment protocols
	if ((version >= 5) && category->protocols)
	{
		if (cls->isa->version >= 5)
		{
			category->protocols->next = cls->protocols;
			cls->protocols	          = category->protocols;
			cls->isa->protocols       = category->protocols;
		}
		else
		{
			_objc_inform ("unable to add protocols from category %s...\n", category->category_name);
			_objc_inform ("class `%s' must be recompiled\n", category->class_name);
		}
	}
	
#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
	// Call back
	if (load_class_callback)
		(*load_class_callback) (cls, 0);
	
	// Call +finishLoading:: from the category's method list
	send_load_message_to_category (category, (void *) header_vector[0].mhdr);
#endif
}

/***********************************************************************
 * _objc_add_category.  Install the specified category's methods into
 * the class it augments, and flush the class' method cache.
 *
 * Private extern used by objc_loadModules ()
 **********************************************************************/
void	_objc_add_category     (struct objc_category *	category,
								int						version)
{
	// Install the category's methods into its intended class
	__objc_add_category (category, version);
	
	// Flush caches so category's methods can get called
	_objc_flush_caches (objc_lookUpClass (category->class_name));
}

/***********************************************************************
 * _objc_resolve_categories_for_class.  Install all categories intended
 * for the specified class, in reverse order from the order in which we
 * found the categories in the image.
 **********************************************************************/
static void	_objc_resolve_categories_for_class  (Class	cls)
{
	_objc_unresolved_category *	cat;
	_objc_unresolved_category *	next;
	
	// Nothing to do if there are no categories at all
	if (!category_hash)
		return;
	
	// Locate and remove first element in category list
	// associated with this class
	cat = NXMapRemove (category_hash, cls->name);
	
	// Traverse the list of categories, if any, registered for this class
	while (cat)
	{
		// Install the category
		_objc_add_category (cat->cat, cat->version);
		
		// Delink and reclaim this registration
		next = cat->next;
		free (cat);
		cat = next;
	}
}

/***********************************************************************
 * _objc_register_category.  Add the specified category to the registry
 * of categories to be installed later (once we know for sure which
 * classes we have).  If there are multiple categories on a given class,
 * they will be processed in reverse order from the order in which they
 * were found in the image.
 **********************************************************************/
static void _objc_register_category    (struct objc_category *	cat,
										long					version)
{
	_objc_unresolved_category *	new_cat;
	_objc_unresolved_category *	old;
	
	
	// If the category's class exists, just add the category
	if (objc_lookUpClass (cat->class_name))
	{
		_objc_add_category (cat, version);
		return;
	}
	
	// Create category lookup table if needed
	if (!category_hash)
		category_hash = NXCreateMapTableFromZone (NXStrValueMapPrototype,
							  128,
							  _objc_create_zone ());
	
	// Locate an existing category, if any, for the class.  This is linked
	// after the new entry, so list is LIFO.
	old = NXMapGet (category_hash, cat->class_name);
	
	// Register the category to be fixed up later
	new_cat = NXZoneMalloc (_objc_create_zone (),
				sizeof(_objc_unresolved_category));
	new_cat->next    = old;
	new_cat->cat     = cat;
	new_cat->version = version;
	(void) NXMapInsert (category_hash, cat->class_name , new_cat);
}

/***********************************************************************
 * _objc_add_categories_from_image.
 **********************************************************************/
static void _objc_add_categories_from_image (header_info *  hi)
#if defined(DYLD_MODULE_INFO_WRONG)
{
	Module		mods;
	unsigned int	midx;
	
	// Major loop - process all modules in the header
	mods = (Module) ((unsigned long) hi->mod_ptr + hi->image_slide);
	for (midx = 0; midx < hi->mod_count; midx += 1)
	{
		unsigned int	index; 
		unsigned int	total; 
		
		// Nothing to do for a module without a symbol table
		if (mods[midx].symtab == NULL)
			continue;
		
		// Total entries in symbol table (class entries followed
		// by category entries)
		total = mods[midx].symtab->cls_def_cnt +
			mods[midx].symtab->cat_def_cnt;
		
#if defined(__MACH__)
		if ((hi->mhdr->filetype == MH_DYLIB) ||
		    (hi->mhdr->filetype == MH_BUNDLE))
		{
			void **	defs;
			
			// Register method lists from all classes in this module
			defs = mods[midx].symtab->defs;
			for (index = 0; index < mods[midx].symtab->cls_def_cnt; index += 1)
				_objc_registerMethodListFromClass (defs[index]);
			
			// Register method lists from all categories in this module
			for (index = mods[midx].symtab->cls_def_cnt; index < total; index += 1)
				_objc_registerMethodListFromCategory (defs[index]);
		}
#endif 
		
		// Minor loop - register all categories from given module
		for (index = mods[midx].symtab->cls_def_cnt; index < total; index += 1)
		{
			_objc_register_category	(mods[midx].symtab->defs[index],
						 mods[midx].version);
		}
	}
}
#else
{
	Module			mods;
	unsigned int	midx;
	
	// Major loop - process all modules in the header
	mods = (Module) ((unsigned long) hi->mod_ptr + hi->image_slide);
	for (midx = 0; midx < hi->mod_count; midx += 1)
	{
		unsigned int	index; 
		unsigned int	total; 
		
		// Nothing to do for a module without a symbol table
		if (mods[midx].symtab == NULL)
			continue;
		
		// Total entries in symbol table (class entries followed
		// by category entries)
		total = mods[midx].symtab->cls_def_cnt +
				mods[midx].symtab->cat_def_cnt;
		
#if defined(__MACH__)
		if ((!_dyld_launched_prebound ()) &&
			((hi->mhdr->filetype == MH_DYLIB) || (hi->mhdr->filetype == MH_BUNDLE)))
			{
			void **		defs;
			Module		mod;

			// Get easy grip on oft-needed data
			mod  = &(mods[midx]);
			defs = mods[midx].symtab->defs;

			// Create classMethodMap if needed
			if (classMethodMap == NULL)
			{
			    classMethodMap = NXCreateMapTableFromZone  (NXPtrValueMapPrototype,
															128,
															_objc_create_zone ());
			}

			// Register method lists from all classes in this module
			for (index = 0; index < mods[midx].symtab->cls_def_cnt; index += 1)
			{
				void *	methodLists;

				methodLists = (void *) (((Class) defs[index])->methodLists);
			    if (methodLists)
					NXMapInsert (classMethodMap, methodLists, (void *) mod);

				methodLists = (void *) (((Class) defs[index])->isa->methodLists);
				if (methodLists)
					NXMapInsert (classMethodMap, methodLists, (void *) mod);
			}

			// Register methods from all categories in this module
			for (index = mods[midx].symtab->cls_def_cnt; index < total; index += 1)
			{
				void *	methods;

				methods = (void *) (((Category) defs[index])->instance_methods);
			    if (methods)
					NXMapInsert (classMethodMap, methods, (void *) mod);

				methods = (void *) (((Category) defs[index])->class_methods);
			    if (methods)
					NXMapInsert (classMethodMap, methods,  (void *) mod);
			}
		}
#endif 
		
		// Minor loop - register all categories from given module
		for (index = mods[midx].symtab->cls_def_cnt; index < total; index += 1)
		{
			_objc_register_category	(mods[midx].symtab->defs[index],
									 mods[midx].version);
		}
	}
}
#endif

#if defined(__MACH__)
/***********************************************************************
 * _headerForClass.
 **********************************************************************/
static const header_info *  _headerForClass     (Class	cls)
{
	const struct segment_command *	objcSeg;
	unsigned int					hidx;
	unsigned int					size;
	unsigned long					vmaddrPlus;
	
	// Check all headers in the vector
	for (hidx = 0; hidx < header_count; hidx += 1)
	{
		// Locate header data, if any
		objcSeg = _getObjcHeaderData ((headerType *) header_vector[hidx].mhdr, &size);
		if (!objcSeg)
			continue;

		// Is the class in this header?
		vmaddrPlus = (unsigned long) objcSeg->vmaddr + header_vector[hidx].image_slide;
		if ((vmaddrPlus <= (unsigned long) cls) &&
		    ((unsigned long) cls < (vmaddrPlus + size)))
			return &(header_vector[hidx]);
	}
	
	// Not found
	return 0;
}
#endif // __MACH__

/***********************************************************************
 * _nameForHeader.
 **********************************************************************/
const char *	_nameForHeader	       (const headerType *	header)
{
	return _getObjcHeaderName ((headerType *) header);
}

#if defined(__MACH__)
/***********************************************************************
 * pend_map_selectors. 
 **********************************************************************/
static void	pend_map_selectors	       (void)
{
	map_selectors_pended = 1;
}
#endif

/***********************************************************************
 * checkForPendingClassReferences.  Complete any fixups registered for
 * this class.
 **********************************************************************/
static void	checkForPendingClassReferences	       (Class	cls)
{
	PendingClass *	pending;

	// Nothing to do if there are no pending classes
	if (!pendingClassRefsMap)
		return;
	
	// Get pending list for this class
	pending = NXMapGet (pendingClassRefsMap, cls->name);
	if (!pending)
		return;
	
	// Remove the list from the table
	(void) NXMapRemove (pendingClassRefsMap, cls->name);
	
	// Process all elements in the list
	while (pending)
	{
		PendingClass *	next;
		
		// Remember follower for loop
		next = pending->next;
		
		// Fill in a pointer to Class 
		// (satisfies caller of objc_pendClassReference)
		if (pending->ref)
			*pending->ref = objc_getClass (cls->name);

		// Fill in super, isa, cache, and version for the class
		// and its meta-class
		// (satisfies caller of objc_pendClassInstallation)
		// NOTE: There must be no more than one of these for
		// any given classToSetUp
		if (pending->classToSetUp)
		{
			Class	fixCls;
		
			// Locate the Class to be fixed up
			fixCls = pending->classToSetUp;
			
			// Set up super class fields with names to be replaced by pointers
			fixCls->super_class      = (Class) pending->nameof_superclass;
			fixCls->isa->super_class = (Class) pending->nameof_superclass;
			
			// Fix up class pointers, version, and cache pointers
			_class_install_relationships (fixCls, pending->version);
		}
		
		// Reclaim the element
		free (pending);
		
		// Move on
		pending = next;
	}
}

/***********************************************************************
 * newPending.  Allocate and zero a PendingClass structure.
 **********************************************************************/
static inline PendingClass *	newPending	       (void)
{
	PendingClass *	pending;
	
	pending = (PendingClass *) NXZoneMalloc (_objc_create_zone (), sizeof(PendingClass));
	bzero (pending, sizeof(PendingClass));
	
	return pending;
}

/***********************************************************************
 * pendingClassRefsMapTable.  Return a pointer to the lookup table for
 * pending classes.
 **********************************************************************/
static inline NXMapTable *	pendingClassRefsMapTable    (void)
{
	// Allocate table if needed
	if (!pendingClassRefsMap)
		pendingClassRefsMap = NXCreateMapTableFromZone (NXStrValueMapPrototype, 10, _objc_create_zone ());
	
	// Return table pointer
	return pendingClassRefsMap;
}

/***********************************************************************
 * objc_pendClassReference.  Register the specified class pointer (ref)
 * to be filled in later with a pointer to the class having the specified
 * name.
 **********************************************************************/
void	objc_pendClassReference	       (const char *	className,
										Class *		ref)
{
	NXMapTable *		table;
	PendingClass *		pending;
	
	// Create and/or locate pending class lookup table
	table = pendingClassRefsMapTable ();

	// Create entry containing the class reference
	pending = newPending ();
	pending->ref = ref;
	
	// Link new entry into head of list of entries for this class
	pending->next = NXMapGet (pendingClassRefsMap, className);
	
	// (Re)place entry list in the table
	(void) NXMapInsert (table, className, pending);
}

/***********************************************************************
 * objc_pendClassInstallation.  Register the specified class to have its
 * super class pointers filled in later because the superclass is not
 * yet found.
 **********************************************************************/
void	objc_pendClassInstallation     (Class	cls,
										int		version)
{
	NXMapTable *		table;
	PendingClass *		pending;
	
	// Create and/or locate pending class lookup table
	table = pendingClassRefsMapTable ();

	// Create entry referring to this class
	pending = newPending ();
	pending->classToSetUp	   = cls;
	pending->nameof_superclass = (const char *) cls->super_class;
	pending->version	   = version;
	
	// Link new entry into head of list of entries for this class
	pending->next		   = NXMapGet (pendingClassRefsMap, cls->super_class);
	
	// (Re)place entry list in the table
	(void) NXMapInsert (table, cls->super_class, pending);
}

/***********************************************************************
 * _objc_get_classes_from_image.  Install all classes contained in the
 * specified image.
 **********************************************************************/
static NXHashTable *	_objc_get_classes_from_image   (NXHashTable *	clsHash,
														header_info *	hi)
{
	unsigned int	index;
	unsigned int	midx;
	Module			mods;
	
	// Major loop - process all modules in the image
	mods = (Module) ((unsigned long) hi->mod_ptr + hi->image_slide);
	for (midx = 0; midx < hi->mod_count; midx += 1)
	{
		// Skip module containing no classes
		if (mods[midx].symtab == NULL)
			continue;
		
		// Minor loop - process all the classes in given module
		for (index = 0; index < mods[midx].symtab->cls_def_cnt; index += 1)
		{
		Class	oldCls;
		Class	newCls;
			
			// Locate the class description pointer
			newCls = mods[midx].symtab->defs[index];
			
			// Convert old style method list to the new style
			_objcTweakMethodListPointerForClass (newCls);

			oldCls = NXHashInsert (clsHash, newCls);

			// Non-Nil oldCls is a class that NXHashInsert just
			// bumped from table because it has the same name
			// as newCls
			if (oldCls)
			{
#if defined(__MACH__)
				const header_info *	oldHeader;
				const header_info *	newHeader;
				const char *		oldName;
				const char *		newName;

				// Log the duplication
				oldHeader = _headerForClass (oldCls);
				newHeader = _headerForClass (newCls);
				oldName   = _nameForHeader  (oldHeader->mhdr);
				newName   = _nameForHeader  (newHeader->mhdr);
				_objc_inform ("Both %s and %s have implementations of class %s.",
								oldName, newName, oldCls->name);				   
				_objc_inform ("Using implementation from %s.", newName);
#endif

				// Use the chosen class
				// NOTE: Isn't this a NOP?
				newCls = objc_lookUpClass (oldCls->name);
			}
			
			// Unless newCls was a duplicate, and we chose the
			// existing one instead, set the version in the meta-class
			if (newCls != oldCls)
				newCls->isa->version = mods[midx].version;

			// Install new categories intended for this class
			// NOTE: But, if we displaced an existing "isEqual"
			// class, the categories have already been installed
			// on an old class and are gone from the registry!!
			_objc_resolve_categories_for_class (newCls);
			
			// Resolve (a) pointers to the named class, and/or
			// (b) the super_class, cache, and version
			// fields of newCls and its meta-class
			// NOTE: But, if we displaced an existing "isEqual"
			// class, this has already been done... with an
			// old-now-"unused" class!!
			checkForPendingClassReferences (newCls);
			
#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
			// Invoke registered callback
			if (load_class_callback)
				(*load_class_callback) (newCls, 0);
			
			// Call +finishLoading:: from the class' method list
			send_load_message_to_class (newCls, (headerType *) hi->mhdr);
#endif
		}
	}
	
	// Return the table the caller passed
	return clsHash;
}

/***********************************************************************
 * _objc_fixup_string_objects_for_image.  Initialize the isa pointers
 * of all NSConstantString objects.
 **********************************************************************/
static void	_objc_fixup_string_objects_for_image   (header_info *	hi)
{
	unsigned int				size;
	OBJC_CONSTANT_STRING_PTR	section;
	Class						constantStringClass;
	unsigned int				index;
	
	// Locate section holding string objects
	section = _getObjcStringObjects ((headerType *) hi->mhdr, &size);
	if (!section || !size)
		return;
	section = (OBJC_CONSTANT_STRING_PTR) ((unsigned long) section + hi->image_slide);
#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
	if (!(*section))
		return;
#endif

	// Luckily NXConstantString is the same size as NSConstantString
	constantStringClass = objc_getClass ("NSConstantString");
	
	// Process each string object in the section
	for (index = 0; index < size; index += 1)
	{
		Class *		isaptr;
		
		isaptr = (Class *) OBJC_CONSTANT_STRING_DEREF section[index];
		if (*isaptr == 0)
			*isaptr = constantStringClass;
	}
}

/***********************************************************************
 * _objc_map_class_refs_for_image.  Convert the class ref entries from
 * a class name string pointer to a class pointer.  If the class does
 * not yet exist, the reference is added to a list of pending references
 * to be fixed up at a later date.
 **********************************************************************/
static void _objc_map_class_refs_for_image (header_info * hi)
{
	Class *			cls_refs;
	unsigned int	size;
	unsigned int	index;
	
	// Locate class refs in image
	cls_refs = _getObjcClassRefs ((headerType *) hi->mhdr, &size);
	if (!cls_refs)
		return;
	cls_refs = (Class *) ((unsigned long) cls_refs + hi->image_slide);
	
	// Process each class ref
	for (index = 0; index < size; index += 1)
	{
		const char *	ref;
		Class		cls;
		
		// Get ref to convert from name string to class pointer
		ref = (const char *) cls_refs[index];
		
		// Get pointer to class of this name
		cls = objc_lookUpClass (ref);
		
		// If class isn't there yet, use pending mechanism
		if (!cls)
		{
			// Register this ref to be set later
			objc_pendClassReference (ref, &cls_refs[index]);
			
			// Use place-holder class
			cls_refs[index] = _objc_getNonexistentClass ();
		}
		
		// Replace name string pointer with class pointer
		else
			cls_refs[index] = cls;
	}
}

/***********************************************************************
 * map_selrefs.  Register each selector in the specified array.  If a
 * given selector is already registered, update this array to point to
 * the registered selector string.
 **********************************************************************/
static inline void	map_selrefs    (SEL *			sels,
									unsigned int	cnt)
{ 
	unsigned int	index;
	
	// Process each selector
	for (index = 0; index < cnt; index += 1)
	{
		SEL	sel;
		
		// Lookup pointer to uniqued string
		sel = _sel_registerName ((const char *) sels[index]);
		
		// Replace this selector with uniqued one (avoid
		// modifying the VM page if this would be a NOP)
		if (sels[index] != sel)
			sels[index] = sel;
	}
}

/***********************************************************************
 * map_methods.
 **********************************************************************/
static inline void  map_methods  (struct objc_method_list *	methods)
{
	unsigned int	index;
	
	// Nothing to do if there are no methods
	if (!methods)
		return;
	
	// Process each method
	for (index = 0; index < methods->method_count; index += 1)
	{
		Method	method;
		SEL	sel;
		
		// Get method entry to fix up
		method = &methods->method_list[index];
		
		// Lookup pointer to uniqued string
		sel = _sel_registerName ((const char *) method->method_name);

		// Replace this selector with uniqued one (avoid
		// modifying the VM page if this would be a NOP)
		if (method->method_name != sel)
			method->method_name = sel;
	}		
}

/***********************************************************************
 * map_method_descs.  For each method in the specified method list,
 * replace the name pointer with a uniqued selector.
 **********************************************************************/
static inline void  map_method_descs (struct objc_method_description_list * methods)
{
	unsigned int	index;
	
	// Process each method
	for (index = 0; index < methods->count; index += 1)
	{
		struct objc_method_description *	method;
		SEL					sel;
		
		// Get method entry to fix up
		method = &methods->list[index];

		// Lookup pointer to uniqued string
		sel = _sel_registerName ((const char *) method->name);

		// Replace this selector with uniqued one (avoid
		// modifying the VM page if this would be a NOP)
		if (method->name != sel)
			method->name = sel;
	}		  
}

/***********************************************************************
 * _fixup.
 **********************************************************************/
@interface Protocol(RuntimePrivate)
+ _fixup: (OBJC_PROTOCOL_PTR)protos numElements: (int) nentries;
@end

/***********************************************************************
 * _objc_fixup_protocol_objects_for_image.  For each protocol in the
 * specified image, selectorize the method names and call +_fixup.
 **********************************************************************/
static void _objc_fixup_protocol_objects_for_image (header_info * hi)
{
	unsigned int		size;
	OBJC_PROTOCOL_PTR	protos;
	unsigned int		index;
	
	// Locate protocals in the image
	protos = (OBJC_PROTOCOL_PTR) _getObjcProtocols ((headerType *) hi->mhdr, &size);
	if (!protos)
		return;
	
	// Apply the slide bias
	protos = (OBJC_PROTOCOL_PTR) ((unsigned long) protos + hi->image_slide);
	
	// Process each protocol
	// NOTE: Can't use map_methods, the sizes differ
	for (index = 0; index < size; index += 1)
	{
		// Selectorize the instance methods
		if (protos[index] OBJC_PROTOCOL_DEREF instance_methods)
			map_method_descs (protos[index] OBJC_PROTOCOL_DEREF instance_methods);
		
		// Selectorize the class methods
		if (protos[index] OBJC_PROTOCOL_DEREF class_methods)
			map_method_descs (protos[index] OBJC_PROTOCOL_DEREF class_methods);
	}
	
	// Invoke Protocol class method to fix up the protocol
	[Protocol _fixup:(OBJC_PROTOCOL_PTR)protos numElements:size];
}

#if defined(__MACH__)
/***********************************************************************
 * qsortHeaders.  Comparison function for qsort'ing headers.  Place
 * all headers with freeze-dried hashtables before any without.
 * Otherwise sort by decreasing size.
 **********************************************************************/
static int	qsortHeaders	   (const void *	x1,
								const void *	x2)
{
	const header_info *	header1 = x1;
	const header_info *	header2 = x2;
	
	// Table with frozen table is always less than one without
	if ((header1->frozenTable) && (!header2->frozenTable))
		return (-1);
	
	if ((header2->frozenTable) && (!header1->frozenTable))
		return 1;
	
	// Smaller is less, bigger is more. same is same
	return (header2->objcSize - header1->objcSize);
}
#endif // __MACH__

/***********************************************************************
 * _objc_headerVector.  Build the header vector, sorting it as
 * _objc_map_selectors () expects.
 **********************************************************************/
header_info *	_objc_headerVector (const headerType * const *	machhdrs)
{
	unsigned int	hidx;
	header_info *	hdrVec;
	
#if defined(__MACH__) // not GENERIC_OBJ_FILE
	// Take advatage of our previous work
	if (header_vector)
		return header_vector;
#else // GENERIC_OBJ_FILE
	// If no headers specified, vectorize generically
	if (!machhdrs)
		return _objc_goff_headerVector ();
	
	// Start from scratch
	header_count = 0;
#endif
	
	// Count headers
	for (hidx = 0; machhdrs[hidx]; hidx += 1)
		header_count += 1;
	
	// Allocate vector large enough to have entries for all of them
	hdrVec = NXZoneMalloc  (_objc_create_zone (),
				header_count * sizeof(header_info));
	if (!hdrVec)
		_objc_fatal ("unable to allocate module vector");
	
	// Fill vector entry for each header
	for (hidx = 0; hidx < header_count; hidx += 1)
	{
		int	size;
#if defined(__MACH__)
		const struct segment_command *	objcSeg = NULL;
#endif
	
		hdrVec[hidx].mhdr	 = machhdrs[hidx];
		hdrVec[hidx].image_slide = 0;
		hdrVec[hidx].mod_ptr	 = _getObjcModules ((headerType *) machhdrs[hidx], &size);
		hdrVec[hidx].mod_count	 = size;
		hdrVec[hidx].objcSize    = 0;
	
#if defined(__MACH__) // not GENERIC_OBJ_FILE
		hdrVec[hidx].frozenTable = _getObjcFrozenTable ((headerType *) machhdrs[hidx]);
		objcSeg = (struct segment_command *) _getObjcHeaderData ((headerType *) machhdrs[hidx], &size);
		if (objcSeg)
			hdrVec[hidx].objcSize = ((struct segment_command *) objcSeg)->filesize;
#endif
	}
	
#if defined(__MACH__)
	// Sort the headers for _objc_map_selectors_from_image (), such that all
	// headers without freeze-dried tables come first, and otherwise 
	// in decreasing size
	qsort (hdrVec, header_count, sizeof(header_info), &qsortHeaders);
#endif
	
	return hdrVec;
}

#if defined(DYLD_MODULE_INFO_WRONG)
#if defined(__MACH__)
/***********************************************************************
 * _objc_registerMethodListFromCategory.  Register all the instance and
 * class methods from the specified category.
 **********************************************************************/
static void  _objc_registerMethodListFromCategory   (Category	cat)
{
	// Create categoryMethodMap if needed
	if (!categoryMethodMap)
		categoryMethodMap = NXCreateMapTableFromZone (NXPtrValueMapPrototype,
							      128,
							      _objc_create_zone ());
	
	// Register instance methods
	if (cat->instance_methods)
		(void) NXMapInsert (categoryMethodMap, (void *) cat->instance_methods, (void *) cat);
	
	// Register class methods
	if (cat->class_methods)
		(void) NXMapInsert (categoryMethodMap, (void *) cat->class_methods, (void *) cat);
}

/***********************************************************************
 * _objc_registerMethodListFromClass.  Register all the instance and
 * class methods from the specified class.
 **********************************************************************/
static void _objc_registerMethodListFromClass (Class cls)
{
	struct objc_method_list *	mList;

	// Create classMethodMap if needed
	if (!classMethodMap)
		classMethodMap = NXCreateMapTableFromZone (NXPtrValueMapPrototype,
							   128,
							   _objc_create_zone ());
	
	// Register instance methods
	if (cls->methodLists)
	{
		mList = (struct objc_method_list *) cls->methodLists;
		(void) NXMapInsert (classMethodMap, (void *) mList, (void *) cls);
	}
	
	// Register class methods
	if (cls->isa->methodLists)
	{
		mList = (struct objc_method_list *) cls->isa->methodLists;
		(void) NXMapInsert (classMethodMap, (void *) mList, (void *) cls);
	}
}
#endif // __MACH__
#endif
	
/***********************************************************************
 * _objc_bindModuleContainingList.  Tell the dynamic loader about the
 * class or category that contributed the specified method list.  Called
 * whenever a method is looked up in a method list.
 **********************************************************************/
void _objc_bindModuleContainingList	   (struct objc_method_list *	methodList)
#if defined(DYLD_MODULE_INFO_WRONG)
{
	Class			cls;
	Category		cat;
#if defined(__MACH__)
	const char *	class_name;
	char *			category_name;
	char *			name;
#endif // MACH
	
	cls = 0;
	cat = 0;
	
	// Remove methodList from method lists registered from categories
	if (categoryMethodMap &&
	   ((cat = (Category) NXMapGet (categoryMethodMap, (void *) methodList)) != 0))
		(void) NXMapRemove (categoryMethodMap, (void *) methodList);

	// Remove methodList from method lists registered from classes
	else if (classMethodMap &&
		((cls = (Class) NXMapGet (classMethodMap, (void *) methodList)) != 0))
		(void) NXMapRemove (classMethodMap, (void *) methodList);
	
	// Bind method list from class
	if (cls)
	{
		// Use the real class behind the poser
		if (CLS_GETINFO (cls, CLS_POSING))
			cls = getOriginalClassForPosingClass (cls);

#if defined(__MACH__)
		// Bind ".objc_class_name_<classname>", where <classname>
		// is the class name with the leading '%'s stripped.
		class_name = cls->name;
		name	   = (char *) alloca (strlen (class_name) + 20);
		while (*class_name == '%')
			class_name += 1;
		strcpy (name, ".objc_class_name_");
		strcat (name, class_name);
		_dyld_lookup_and_bind_objc (name, 0, 0);
	}
		
	// Bind method list from category
	else if (cat)
	{
		// Bind ".objc_category_name_<classname>_<categoryname>",
		// where <classname> is the class name with the leading
		// '%'s stripped.
		class_name    = cat->class_name;
		category_name = cat->category_name;
		name	      = alloca (strlen (class_name) 
					+ strlen (category_name)
					+ 30);
		while (*class_name == '%')
			class_name += 1;
		strcpy (name, ".objc_category_name_");
		strcat (name, class_name);
		strcat (name, "_");
		strcat (name, category_name);
		_dyld_lookup_and_bind_objc (name, 0, 0);
#endif // MACH
	}
}
#else
{
	Module		mod;

    // classMethodMap is only initialized on __MACH__
    // and !_dyld_launched_prebound ()
    if (classMethodMap == NULL)
		return;

	// Locate the module containing the method list
	mod = NXMapGet (classMethodMap, (void *) methodList);
	
	// Return if odule has already been bound and removed 
	if (mod == NULL)
		return;
	
	// Bind module and remove it from classMethodMap
#if defined(__MACH__)
	_dyld_bind_objc_module (mod);
#endif
	NXMapRemove (classMethodMap, (void *) methodList);
}
#endif

/***********************************************************************
 * _objc_headerCount.  Return the currently known number of `__OBJC'
 * segments that are a part of the application
 **********************************************************************/
unsigned int	_objc_headerCount	       (void)
{
#if defined(__MACH__) // not GENERIC_OBJ_FILE
	return header_count;
#else
	return _objc_goff_headerCount ();
#endif
}

/***********************************************************************
 * _objc_addHeader.
 *
 * NOTE: Yet another wildly inefficient routine.
 **********************************************************************/
void	_objc_addHeader	       (const headerType *	header,
								unsigned long		vmaddr_slide)
{
	// Account for addition
	header_count += 1;
	
	// Create vector table if needed
	if (header_vector == 0)
	{
		header_vector = NXZoneMalloc (_objc_create_zone (), 
					      header_count * sizeof(header_info));
#if defined(WIN32) || defined(__svr4__)
		bzero (header_vector, (header_count * sizeof(header_info)));
#endif
	}
	
	
	// Malloc a new vector table one bigger than before
	else
	{
		void *	old;
		
		old = (void *) header_vector;
		header_vector = NXZoneMalloc (_objc_create_zone (),
					      header_count * sizeof(header_info));
	
#if defined(WIN32) || defined(__svr4__)
		bzero (header_vector, (header_count * sizeof(header_info)));
#endif
		memcpy ((void *) header_vector, old, (header_count - 1) * sizeof(header_info));
		NXZoneFree (_objc_create_zone (), old);
	}
	
	// Set up the new vector entry
	header_vector[header_count - 1].mhdr		= header;
	header_vector[header_count - 1].mod_ptr		= NULL;
	header_vector[header_count - 1].mod_count	= 0;
	header_vector[header_count - 1].frozenTable	= NULL;
	header_vector[header_count - 1].image_slide	= vmaddr_slide;
	header_vector[header_count - 1].objcSize	= 0;
}

/***********************************************************************
 * _objc_removeHeader.
 *
 * NOTE: Yet another wildly inefficient routine.
 **********************************************************************/
void	_objc_removeHeader	(const headerType *	header)
{
	unsigned int			index;
	unsigned int			index2;
	volatile header_info *	tempHeader;
	
	// Major loop - remove vector entry pointing to this header
	// Should just stop when one entry is found, no?
	for (index = 0; index < header_count; index += 1)
	{
		// Skip entry not pointing to specified header
		if (header_vector[index].mhdr != header)
			continue;
		
		// Left shift remaining entries
		for (index2 = index; index2 < header_count - 1; index2 += 1)
			header_vector[index2] = header_vector[index2 + 1];
	}
	
	// Account for the loss
	header_count -= 1;
	
	// Replace the vector with a smaller copy
	tempHeader = (header_info *) NXZoneRealloc ((NXZone *) _objc_create_zone (),
						    (void *) header_vector,
						    (size_t) (header_count * sizeof(header_info)));
	header_vector = (header_info *) tempHeader;
}

/***********************************************************************
 * libraryNameForMachHeader.
**********************************************************************/
static const char *	libraryNameForMachHeader  (const headerType * themh)
{
#if defined(NeXT_PDO)
	return "";
#else
	unsigned long	index;
	unsigned long	imageCount;
	headerType *	mh;
	
	// Search images for matching type
	imageCount = _dyld_image_count ();
	for (index = 0; index < imageCount ; index += 1)
	{
		// Return name of image with matching type
		mh = _dyld_get_image_header (index);
		if (mh == themh)
			return _dyld_get_image_name (index);
	}
	
	// Not found
	return 0;
#endif
}

#if defined(__MACH__)
/***********************************************************************
 * undo_preuniqing_for_image.  Use the __sel_backref section to restore
 * the image's selectors to their original values (i.e. so they once again
 * point into the image's own __meth_varnames section, rather than into
 * some loaded library).  After this routine is called. the selectors
 * will get added to a dynamic hash table.
 **********************************************************************/
static void undo_preuniqing_for_image (const header_info *  hi)
{
	unsigned int	size;
	void *			backrefs;
	FixupEntry *	entry;
	unsigned int	unused;
	void *			base;
	
	// Locate the BackRefs in the image
	backrefs = _getObjcBackRefs ((headerType *) hi->mhdr, &size);
	if (!backrefs)
		return;

	// Locate base
	base = (void *) (((struct segment_command *) _getObjcHeaderData ((headerType *) hi->mhdr, &unused))->vmaddr + hi->image_slide);
	
	// Apply the slide bias to the BackRefs
	backrefs = (void *) ((unsigned long) backrefs + hi->image_slide);
	entry    = backrefs;
	
	if (rocketLaunchingDebug)
		_NXLogError ("undoing preuniquing for %s\n", libraryNameForMachHeader (hi->mhdr));
	
	// Process each BackRef
	while (entry < (FixupEntry *) (backrefs + size))
	{
		void **		selPtr;
		void *		sel;
		
		selPtr = (void **) (base + entry->addressOffset);
		sel    = (void  *) (base + entry->selectorOffset);
		if (*selPtr != sel)
		{
			if (rocketLaunchingDebug)
				_NXLogError ("bashing 0x%x with sel (0x%x) '%s'\n", (unsigned int) selPtr, (unsigned int) sel, (char *) sel);
			*selPtr = sel;
		}
		
		entry += 1;
	}
}
#endif // __MACH__

/***********************************************************************
 * _objc_fixup_selector_refs.  Register all of the selectors in each
 * image, and fix them all up.
 *
 * We must register all images which have freeze-dried hashtables before any
 * which don't, since the freeze-dried hashtables are searched before the
 * dynamic hashtable.  We also want to register the freeze-dried hashtables
 * in decreasing order of size to minimize selector lookup time.  We don't
 * need to fixup the selectors in the first image we register, since those
 * strings will always be used.  If the first image has a freeze-dried
 * hashtable, then we don't need to run over its selectors at all, since
 * we can register them all at once.
 **********************************************************************/
static void _objc_fixup_selector_refs   (const header_info *	hi)
{
	unsigned int		midx;
	unsigned int		size;
	OBJC_PROTOCOL_PTR	protos;
	Module				mods;
	unsigned int		index;
#if defined(__MACH__)
	SEL *				messages_refs;
#endif // __MACH__
	
	mods = (Module) ((unsigned long) hi->mod_ptr + hi->image_slide);

#if defined(__MACH__)
	undo_preuniqing_for_image (hi);
#endif

	if (rocketLaunchingDebug)
	{
		_NXLogError ("uniquing selectors for %s\n", libraryNameForMachHeader(hi->mhdr));
		_NXLogError ("   uniquing message_refs\n");
	}
	
#if defined(__MACH__)
	// Fix up message refs
	messages_refs = (SEL *) _getObjcMessageRefs ((headerType *) hi->mhdr, &size);
	if (messages_refs)
	{
		messages_refs = (SEL *) ((unsigned long) messages_refs + hi->image_slide);
		map_selrefs (messages_refs, size);
	}
#endif // __MACH__

	if (rocketLaunchingDebug)
		_NXLogError ("   uniquing method lists\n");
	
	// Major loop - process all modules in this image
	for (midx = 0; midx < hi->mod_count; midx += 1)
	{
		// Skip module containing no classes or categories
		if (mods[midx].symtab == NULL)
			continue;
		
		// Fix up object instance and class method names
		for (index = 0; index < mods[midx].symtab->cls_def_cnt; index += 1)
		{
			Class	cls;
			
			cls = mods[midx].symtab->defs[index];
			map_methods (get_base_method_list (cls));
			map_methods (get_base_method_list (cls->isa));
		}
		
		// Fix up category instance and class method names
		for (index = mods[midx].symtab->cls_def_cnt;
		     index < (mods[midx].symtab->cls_def_cnt + mods[midx].symtab->cat_def_cnt);
		     index += 1)
		{
			Category	cat;
			
			cat = mods[midx].symtab->defs[index];
			
			// Fix up instance methods
			if (cat->instance_methods)
				map_methods (cat->instance_methods);
			
			// Fix up class methods
			if (cat->class_methods)
				map_methods (cat->class_methods);
		}
	}
	
	// Fix up protocols
	protos = (OBJC_PROTOCOL_PTR) _getObjcProtocols ((headerType *) hi->mhdr, &size);
	if (protos)
	{
		protos = (OBJC_PROTOCOL_PTR)((unsigned long)protos + hi->image_slide);
		
		for (index = 0; index < size; index += 1)
		{
			// Fix up instance method names
			if (protos[index] OBJC_PROTOCOL_DEREF instance_methods)
				map_method_descs (protos[index] OBJC_PROTOCOL_DEREF instance_methods);
			
			// Fix up class method names
			if (protos[index] OBJC_PROTOCOL_DEREF class_methods)
				map_method_descs (protos[index] OBJC_PROTOCOL_DEREF class_methods);
		}
	}
}

#if defined(__MACH__)
/***********************************************************************
 * sortedRefsSize.
 **********************************************************************/
static int  sortedRefsSize	(const header_info *	info)
{
	unsigned int size;
	
	if (_getObjcBackRefs ((headerType *)info->mhdr, &size))
		return size;

	return 0;
}

/***********************************************************************
 * qsortBackrefsSectionSize.
 **********************************************************************/
static int qsortBackrefsSectionSize    (const void *	v1,
										const void *	v2)
{
	unsigned int	size1;
	unsigned int	size2;
	
	size1 = sortedRefsSize (*(header_info **) v1);
	size2 = sortedRefsSize (*(header_info **) v2);
	
	if (size1 == size2)
		return 0;

	return size1 > size2 ? (-1) : 1;
}

/***********************************************************************
 * do_pended_map_selectors.  Process the array of mach-o headers that
 * we collected from the image.  This processing was postponed til now
 * iff we expected preuniquing, so that frozen selector tables can be
 * installed properly.
 *
 * If it turned out that one or more images are not prebound (i.e.
 * have no frozen selector table), preuniquing is disabled and all
 * selectors are registered in a massive dynamic hash table.
 *
 * This "pending" mechanism is applied only the set of images loaded
 * at objc_init time.  Images loaded later have their selectors added
 * to the dynamic table by the image load callback.
 **********************************************************************/
static void	do_pended_map_selectors	       (void)
{
	int				loop;
	headerType *	machhdr;
	BOOL			mainModuleObjC;
	BOOL			haveConflictsToResolve;
	
	// Log our presence
	if (rocketLaunchingDebug)
		_NXLogError ("rocketLaunching == %s\n", (rocketLaunching ? "YES" : "NO"));
	
	// Sort the headers according to size of backRefs section
	qsort  (pended_map_selectors,
			pended_map_selectors_count,
			sizeof(header_info *),
			qsortBackrefsSectionSize);
	
	// Turn off rocket launching if some library was not prebound or
	// the main module has no ObjC segment to contain a conflict
	// even though the dependant libraries have conflicts.
	mainModuleObjC		   = YES;
	haveConflictsToResolve = NO;
	for (loop = 0; rocketLaunching && (loop < pended_map_selectors_count); loop += 1)
	{
		header_info *	hi;
		unsigned int	unused;
		SEL **			conflicts;
		int				size;
		
		// Locate the mach header for this module.
		hi		= pended_map_selectors [loop];
		machhdr	= (headerType *) hi->mhdr;

		// Check for ObjC segment existence
		if (_getObjcHeaderData (machhdr, &unused))
		{
			// Turn off rocket launching if this module
			// has an ObjC segment, but no backrefs section
			// (meaning it was not prebound).
			if (_getObjcBackRefs (machhdr, &unused) == NULL)
			{
				if (rocketLaunchingDebug)
				{
					_NXLogError ("backing off because no backref table for: %s\n",
						libraryNameForMachHeader (machhdr));
				}
		
				rocketLaunching = NO;
			}

			// Record the fact that this header
			// has a conflict section
			conflicts = _getObjcConflicts(machhdr, &size);
			if (size != 0)
				haveConflictsToResolve = YES;
		}

		// Record the fact that the main module has
		// no ObjC segment
		else if (machhdr->filetype == MH_EXECUTE)
			mainModuleObjC = NO;
	}

	// Must disable rocket launching if the main module has no ObjC
	// segment to contain the compendium of conflicts from the
	// dependant libraries.
	if (haveConflictsToResolve && (mainModuleObjC == NO))
	{
		if (rocketLaunchingDebug)
			_NXLogError ("backing off because main module is missing necessary conflict section\n");
		
		rocketLaunching = NO;
	}
	
	// Iff preuniquing is still enabled, install backrefs sections
	// as freeze-dried selector lookup tables.
	for (loop = 0; loop < pended_map_selectors_count; loop += 1)
	{
		header_info *					hi;
		unsigned int					size;
		const struct segment_command *	objcSeg;
		unsigned long					vmaddrPlus;
		void *							backrefs;
		
		// Locate the segment_command
		hi      = pended_map_selectors [loop];
		machhdr	= (headerType *) hi->mhdr;
		objcSeg = (struct segment_command *) _getObjcHeaderData (machhdr, &size);
		if (!objcSeg || !rocketLaunching)
			continue;

		// Resolve prebinding conflicts.  Need do this only on the main image,
		// since its conflict list is the sum of conflict lists from all
		// libraries.
		if (machhdr->filetype == MH_EXECUTE)
		{
			if (rocketLaunchingDebug)
				_NXLogError ("resolving conflicts in: %s\n", libraryNameForMachHeader (machhdr));
			_sel_resolve_conflicts (machhdr, hi->image_slide);
		}

		// Locate the backrefs (which we now know to exist, no?)
		vmaddrPlus = (unsigned long) objcSeg->vmaddr + hi->image_slide;
		backrefs = _getObjcBackRefs (machhdr, &size);
		if (backrefs)
			backrefs = (void *) ((unsigned long) backrefs + hi->image_slide);
		
		// Install header as frozen lookup table at end of chain
		if (rocketLaunchingDebug)
			_NXLogError ("installing backref table for: %s\n", libraryNameForMachHeader (machhdr));
		_sel_initsorted (machhdr, backrefs, (void *) vmaddrPlus, size);
	}

	// Apply dynamic selector uniquing for any non-prebound or
	// prebound-disabled image.
	for (loop = 0; loop < pended_map_selectors_count; loop += 1)
	{
		unsigned int		unused;
		const header_info *	hi;
		const headerType *	mhdr;
		
		unused = 0;
		hi     = pended_map_selectors [loop];
		mhdr   = hi->mhdr;

		// Register/unique any selectors in non-prebound image
		if (!rocketLaunching ||
		    ((mhdr->flags & MH_PREBOUND) == 0) || 
		    (!_getObjcBackRefs ((headerType *) mhdr, &unused)))
		{
			// Unique all selectors in the image
			if (_getObjcHeaderData ((headerType *) mhdr, &unused))
				_objc_fixup_selector_refs (hi);
		}

		// Remove and reclaim the header_info
		pended_map_selectors [loop] = 0;
		NXZoneFree (_objc_create_zone (), (void *) hi);
	}
	
	// Reset map_selector data
	if (pended_map_selectors != NULL)
		NXZoneFree (_objc_create_zone (), pended_map_selectors);
	pended_map_selectors		= NULL;
	pended_map_selectors_count	= 0;
	pended_map_selectors_size	= 0;

	// Libraries are added non-lazily from now on.
	map_selectors_pended		= 0;
	
}
#endif // __MACH__

/***********************************************************************
 * _objc_map_selectors_from_image.
 **********************************************************************/
#if defined(NeXT_PDO)
static 
#endif
void	_objc_map_selectors_from_image  (header_info *	hi)
{
	// If not pending, fix up the selectors and be done
	if (!map_selectors_pended)
	{
		// Unique all selectors in the image
		_objc_fixup_selector_refs (hi);
		return;
	}

	// Make room for the new header pointer.  Note that NXZoneRealloc
	// "acts like NXMalloc" when the original pointer is NULL
	// (i.e. the first time we're called.
	if (pended_map_selectors_count >= pended_map_selectors_size)
	{
		pended_map_selectors_size += pended_map_selectors_growth;

		pended_map_selectors = (struct header_info **) NXZoneRealloc (
				(NXZone *) _objc_create_zone (),
				(void *) pended_map_selectors,
				(size_t) (pended_map_selectors_size * sizeof(header_info *)));
	}

	// Copy the header_info into new pended_map_selectors entry.
	pended_map_selectors [pended_map_selectors_count] =
		NXZoneMalloc (_objc_create_zone (), sizeof(*hi));
	memcpy (pended_map_selectors [pended_map_selectors_count], hi, sizeof(*hi));
	pended_map_selectors_count += 1;
}

/***********************************************************************
 * _objc_call_loads_for_image.
 **********************************************************************/
static void _objc_call_loads_for_image (header_info * header)
{
	Class						cls;
	Class *						pClass;
	Category *					pCategory;
	IMP							load_method;
	struct objc_method_list **	methods;
	unsigned int				nModules;
	unsigned int				nClasses;
	unsigned int				nCategories;
	struct objc_symtab *		symtab;
	struct objc_module *		module;
	
	// Major loop - process all modules named in header
	module = (struct objc_module *) ((unsigned long) header->mod_ptr + header->image_slide);
	for (nModules = header->mod_count; nModules; nModules -= 1, module += 1)
	{
		symtab = module->symtab;
		if (symtab == NULL)
			continue;
		
		// Minor loop - call the +load from each class in the given module
		for (nClasses = symtab->cls_def_cnt, pClass = (Class *) symtab->defs;
		     nClasses;
		     nClasses -= 1, pClass += 1)
		{
			cls = *pClass;
			methods = cls->isa->methodLists;
			if (methods)
			{
				// Look up the method manually (vs messaging the class) to bypass
				// +initialize and cache fill on class that is not even loaded yet
				load_method = class_lookupMethodInMethodList (get_base_method_list (cls->isa),
									      @selector(load));
				if (load_method)
					(*load_method) ((id) cls, @selector(load));
			}
		}
		
		// Minor loop - call the +load from augmented class of
		// each category in the given module
		for (nCategories = symtab->cat_def_cnt,
			pCategory = (Category *) &symtab->defs[symtab->cls_def_cnt];
		     nCategories;
		     nCategories -= 1, pCategory += 1)
		{
			struct objc_method_list *	methods;
			
			cls = objc_getClass ((*pCategory)->class_name);
			methods = (*pCategory)->class_methods;
			if (methods)
			{
				load_method = class_lookupMethodInMethodList (methods, @selector(load));
				if (load_method)
					(*load_method) ((id) cls, @selector(load));
			}
		}
	}
}

/***********************************************************************
 * objc_setMultithreaded.
 **********************************************************************/
void objc_setMultithreaded (BOOL flag)
{
	if (flag == YES)
		_objc_multithread_mask = 0;
	else
		_objc_multithread_mask = (-1);
}

/***********************************************************************
 * _objcInit.
 **********************************************************************/
void	_objcInit	       (void)
{
	int				hidx;
#if defined(NeXT_PDO)
	const headerType * const *	headers;

	rocketLaunchingDebug = (getenv ("OBJC_UNIQUE_DEBUG") != 0);

	// Get architecture dependent module headers
	headers = (const headerType * const *) _getObjcHeaders ();
	if (headers)
	{
		// Create vector from these headers
		header_vector = _objc_headerVector (headers);
		if (header_vector) 
		{
			// Load classes from all images in the vector
			for (hidx = 0; hidx < header_count; hidx += 1)
				(void) _objc_get_classes_from_image (objc_getClasses (), &header_vector[hidx]);
		}
	}

#elif defined(__MACH__)
	struct rusage			r1;
	struct rusage			r2;
	struct vm_statistics	stats1;
	struct vm_statistics	stats2;
	
	// Create the class lookup table
	_objc_init_class_hash ();
	
	// Get our configuration
	rocketLaunching	     = _dyld_launched_prebound () && (getenv ("OBJC_DISABLE_OBJCUNIQUE") == 0);
	rocketLaunchingDebug = (getenv ("OBJC_UNIQUE_DEBUG") != 0);
	if (getenv ("OBJC_INIT_TIME"))
	{
		getrusage (RUSAGE_SELF, &r1);
		vm_statistics (task_self (), &stats1);
	}
	
	if (_dyld_present ())
	{
		headerType **	machheaders;
		int				index;
		
		// Cause headers to accumulate in pended_map_selectors array for
		// delayed processing
		pend_map_selectors ();

		// Register our image mapping routine with dyld so it
		// gets invoked when an image is added.  This also invokes
		// the callback right now on any images already present.
		_dyld_register_func_for_add_image (&_objc_map_image_callback);
		
		// Process the headers that accumulated
		do_pended_map_selectors ();
		
		// Register module link callback with dyld
		_dyld_register_func_for_link_module (&_objc_link_module_callback);
#endif // MACH

		// Install relations on classes that were found
		for (hidx = 0; hidx < header_count; hidx += 1)
		{
			int						nModules;
			int						index;
			struct objc_module *	module;
			Class					cls;
			
			module = (struct objc_module *) ((unsigned long) header_vector[hidx].mod_ptr + header_vector[hidx].image_slide);
			for (nModules = header_vector[hidx].mod_count; nModules; nModules -= 1)
			{
				for (index = 0; index < module->symtab->cls_def_cnt; index += 1)
				{
					cls = (Class) module->symtab->defs[index];
					_class_install_relationships (cls, module->version);
				}

				module += 1;
			}
		}
		
		for (hidx = 0; hidx < header_count; hidx += 1)
		{
#if !defined(__MACH__)
			(void)_objc_add_categories_from_image (&header_vector[hidx]);
			(void)_objc_map_selectors_from_image (&header_vector[hidx]);
#endif
			// Initialize the isa pointers of all NXConstantString objects
			(void)_objc_fixup_string_objects_for_image (&header_vector[hidx]);

			// Convert class refs from name pointers to ids
			(void)_objc_map_class_refs_for_image (&header_vector[hidx]);
		}
#if defined(__MACH__)
		if (getenv ("OBJC_DUMP_CLASSES"))
			objc_dump_class_hash ();
		
		// Register callback with dyld
		_dyld_register_func_for_unlink_module (&_objc_unlink_module_callback);
#endif
		
		// For each image selectorize the method names and +_fixup each of
		// protocols in the image
		for (hidx = 0; hidx < header_count; hidx += 1)
			_objc_fixup_protocol_objects_for_image (&header_vector[hidx]);
		
		// Call +load methods that weren't done in the original add-image callbacks
		// because _objc_first_add_image_callback was set
		for (hidx = 0; hidx < header_count; hidx += 1)
			_objc_call_loads_for_image (&header_vector[hidx]);
		
#if defined(__MACH__)
	}
	
	if (getenv ("OBJC_INIT_TIME"))
	{
		getrusage (RUSAGE_SELF, &r2);
		vm_statistics (task_self (), &stats2);
		{
			int		usecs	= r2.ru_utime.tv_usec - r1.ru_utime.tv_usec;
			int		secs	= r2.ru_utime.tv_sec - r1.ru_utime.tv_sec;
			float	seconds = secs + (((float) usecs) / 1000000.0);
			
			_NXLogError ("_objcInit took %f seconds\n", seconds);
			_NXLogError ("cow_faults: %d\n", stats2.cow_faults-stats1.cow_faults);
			_NXLogError ("dynamic hash entries = %d\n", _objc_dynamic_hash_count ());
		}
	}
#endif // MACH
#if defined(WIN32) || defined(__svr4__)
	CMH = (objcModHeader *) 0;
#endif
}

#if defined(__MACH__)
/***********************************************************************
 * _objc_map_image_callback.
 **********************************************************************/
static void	_objc_map_image_callback       (headerType *	mh,
											unsigned long	vmaddr_slide)
{
	header_info *					hInfo;
	const struct segment_command *	objcSeg;
	unsigned int					size;
	
	// Add this header to the header_vector
	_objc_addHeader (mh, vmaddr_slide);
	
	// Touch up the vector entry we just added (yuck)
	hInfo = &(header_vector[header_count-1]);
	hInfo->mod_ptr	   = (Module) _getObjcModules ((headerType *) hInfo->mhdr, &size);
	hInfo->mod_count   = size;
	hInfo->frozenTable = _getObjcFrozenTable ((headerType *) hInfo->mhdr);
	objcSeg = (struct segment_command *) _getObjcHeaderData ((headerType *) mh, &size);
	if (objcSeg)
		hInfo->objcSize = objcSeg->filesize;
	else
		hInfo->objcSize = 0;
	
	// Register any categories and/or classes and/or selectors this image contains
	_objc_add_categories_from_image (hInfo);
	class_hash = _objc_get_classes_from_image (objc_getClasses (), hInfo);
	_objc_map_selectors_from_image (hInfo);
	
	// Log all known class names, if asked
	if (getenv ("OBJC_DUMP_CLASSES"))
	{
		_NXLogError ("classes...\n");
		objc_dump_class_hash ();
	}
	
	if (_dyld_present () && !map_selectors_pended)
	{
		int			nModules;
		int			index;
		struct objc_module *	module;
		
		// Major loop - process each module
		module = (struct objc_module *) ((unsigned long) hInfo->mod_ptr + hInfo->image_slide);
		for (nModules = hInfo->mod_count; nModules; nModules -= 1)
		{
			// Minor loop - process each class in a given module
			for (index = 0; index < module->symtab->cls_def_cnt; index += 1)
			{
				Class cls;
				
				// Locate the class description
				cls = (Class) module->symtab->defs[index];
				
				// If there is no superclass or the superclass can be found,
				// install this class, and invoke the expected callback
				if (!cls->super_class || objc_lookUpClass ((char *) cls->super_class))
				{
					_class_install_relationships (cls, module->version);
					if (callbackFunction)
						(*callbackFunction) (cls, 0);
				}
				
				// Super class can not be found yet, arrange for this class to
				// be filled in later
				else
				{
					objc_pendClassInstallation (cls, module->version);
					cls->super_class      = _objc_getNonexistentClass ();
					cls->isa->super_class = _objc_getNonexistentClass ();
				}
			}

			// Move on
			module += 1;
		}
		
		// Initialize the isa pointers of all NXConstantString objects
		_objc_fixup_string_objects_for_image (hInfo);

		// Convert class refs from name pointers to ids
		_objc_map_class_refs_for_image (hInfo);

		// Selectorize the method names and +_fixup each of
		// protocols in the image
		_objc_fixup_protocol_objects_for_image (hInfo);

		// Call +load on all classes and categorized classes
		_objc_call_loads_for_image (hInfo);
	}
	
	// Sort vector such that all headers without freeze-dried tables
	// come first, and otherwise in decreasing size
	qsort (header_vector, header_count, sizeof(header_info), &qsortHeaders);
}
#endif // __MACH__

#if defined(__MACH__)
/***********************************************************************
 * _objc_link_module_callback.  Callback installed with
 * _dyld_register_func_for_link_module.
 *
 * NOTE: Why does this exist?  The old comment said "This will install
 * class relations for the executable and dylibs."  Hmm.
 **********************************************************************/
static void	_objc_link_module_callback     (NSModule	mod)
{
}

/***********************************************************************
 * _objc_unlink_module_callback.  Callback installed with
 * _dyld_register_func_for_unlink_module.
 **********************************************************************/
static void	_objc_unlink_module_callback   (NSModule	mod)
{
	_objc_fatal ("unlinking is not supported in this version of Objective C\n");
}
#endif // __MACH__

#if defined(WIN32)
#import <stdlib.h>
/***********************************************************************
 * NSRootDirectory.  Returns the value of the root directory that the
 * product was installed to.
 **********************************************************************/
const char *	NSRootDirectory	       (void)
{
	const char * root;

	root = (char *) getenv ("NEXT_ROOT");
	if (!root)
		root = "";

	return root;
}
#endif 

/***********************************************************************
 * objc_setNilObjectMsgHandler.
 **********************************************************************/
void  objc_setNilObjectMsgHandler   (NilObjectMsgCallback  nilObjMsgCallback)
{
	_objc_msgNil = nilObjMsgCallback;
}

/***********************************************************************
 * objc_getNilObjectMsgHandler.
 **********************************************************************/
NilObjectMsgCallback  objc_getNilObjectMsgHandler   (void)
{
	return _objc_msgNil;
}

#if defined(NeXT_PDO)
// so we have a symbol for libgcc2.c when running PDO
arith_t		_objcInit_addr = (arith_t) _objcInit;
#endif
