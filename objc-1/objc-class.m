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
 *	objc-class.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */

static int _class_uncache = 1;
static int _class_slow_grow = 1;

#import <mach/mach_interface.h>

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include <mach-o/ldsyms.h>
#import "objc-private.h"
#ifdef RUNTIME_DYLD
#include <mach-o/dyld.h>
#endif
#import "objc-runtime.h"
#import "objc.h"
#import "Object.h"
#import "maptable.h"

#ifdef RUNTIME_DYLD
void *getsectdatafromheaderinfo(const struct header_info * info, 
				char * segname, 
				char * sectname, 
				int * size);
void _objc_bindModuleContainingList (struct objc_method_list*);
#endif
void *getsectdatafromheader(const struct mach_header *, char *, char *, int *);

#ifdef KERNEL

#undef	bzero(x,y)
#define	vm_page_size	8192

#else /* KERNEL */

#import <mach/mach.h>
#import <mach/thread_status.h>

#endif /* KERNEL */

/* CACHE_SIZE and META_CACHE_SIZE must be a power of two */
#define CACHE_SIZE		4	// let's try 4 instead (was 16)
#define META_CACHE_SIZE		4	// let's try 4 instead (8)

#define ISCLASS(cls)		((cls)->info & CLS_CLASS)
#define ISMETA(cls)		((cls)->info & CLS_META) 
#define GETMETA(cls)		(ISMETA(cls) ? cls : cls->isa)

#define ISINITIALIZED(cls)	(GETMETA(cls)->info & CLS_INITIALIZED)
#define MARKINITIALIZED(cls)	(GETMETA(cls)->info |= CLS_INITIALIZED)

static void _cache_fill(Class, Method);
static Cache _cache_expand(Class);
static void _cache_flush (Class);

#ifdef OBJC_COLLECTING_CACHE
static void _cache_collect_free(void *data, BOOL tryCollect);
#endif

// Error Messages
static const char
	_errNoMem[] = "failed -- out of memory(%s, %u)",
	_errAllocNil[] = "allocating nil object",
	_errFreedObject[] = "message %s sent to freed object=0x%lx",
#if RUNTIME_DYLD
        _errNonExistentObject[] = "message %s sent to non-existent object=0x%lx",
#endif
	_errBadSel[] = "invalid selector %s",
#if 0
	_errDoesntRecognize[] = "Does not recognize selector %s",
#endif
	_errNotSuper[]	= "[%s poseAs:%s]: target not immediate superclass",
	_errNewVars[]	= "[%s poseAs:%s]: %s defines new instance variables";

/* Information about multithtread support:
   
   Since we do not lock many operations which walk the superclass, method
   and ivar chains, these chains must remain intact once a class is published
   by inserting it into the class hashtable.  All modifications must be
   atomic so that someone walking these chains will always geta valid
   result. */

/* Lock for messaging. (Private extern) */

#ifdef OBJC_COLLECTING_CACHE
static OBJC_DECLARE_LOCK (cacheUpdateLock);
static OBJC_DECLARE_LOCK (cacheCollectionLock);
#endif

/* This is the read lock */
OBJC_DECLARE_LOCK (messageLock);


static void *objc_malloc (int size);


/* A static empty cache.  All classes initially point at this cache.
   When the first message is sent it misses in the cache, and when
   the cache is grown it checks for this case and uses malloc rather
   than realloc.  This avoids the need to check for NULL caches in the
   messenger. */

const struct objc_cache emptyCache =
{
  0,				/* mask */
  0,				/* occupied */
#ifdef OBJC_COPY_CACHE
  { 0, 0 }			/* buckets */
#else
  { NULL }			/* buckets */
#endif
};

/* Freed objects have their isa set to point to this dummy class.
   This avoids the need to check for Nil classes in the messenger.  */

static const struct objc_class freedObjectClass =
{
  Nil,				/* isa */
  Nil,				/* super_class */
  "FREED(id)",			/* name */
  0,				/* version */
  0,				/* info */
  0,				/* instance_size */
  NULL,				/* ivars */
  NULL,				/* methods */
  (Cache) &emptyCache,		/* cache */
  NULL				/* protocols */
};

#ifdef RUNTIME_DYLD

static const struct objc_class nonexistentObjectClass =
{
  Nil,				/* isa */
  Nil,				/* super_class */
  "NONEXISTENT(id)",   		/* name */
  0,				/* version */
  0,				/* info */
  0,				/* instance_size */
  NULL,				/* ivars */
  NULL,				/* methods */
  (Cache) &emptyCache,		/* cache */
  NULL				/* protocols */
};

#endif

const char *object_getClassName(id obj)
{
	if (obj == nil) 
		return "nil";
	else
		return ((Class)obj->isa)->name;
}

void *object_getIndexedIvars(id obj)
{
	return ((char *)obj) + obj->isa->instance_size;
}

// Allocate new instance of aClass with nBytes bytes of indexed vars

id _internal_class_createInstanceFromZone(Class aClass, unsigned nBytes, NXZone *zone) 
{
	id obj; 
	register unsigned siz;

	if (aClass == Nil)
		__objc_error((id)aClass, _errAllocNil, 0);

	siz = aClass->instance_size + nBytes;

	if ((obj = (id)NXZoneMalloc(zone, siz))) { 
		bzero((char *)obj, siz);
		obj->isa = aClass; 
		return obj;
	} else {
		__objc_error((id)aClass, _errNoMem, aClass->name, nBytes);
                return nil;
        }
} 

id class_createInstanceFromZone(Class aClass, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneAlloc)(aClass, nBytes, zone);
} 

id _internal_class_createInstance(Class aClass, unsigned nBytes) 
{
    return _internal_class_createInstanceFromZone(aClass, nBytes, NXDefaultMallocZone());
} 

id class_createInstance(Class aClass, unsigned nBytes) 
{
	return (*_alloc)(aClass, nBytes);
} 

void class_setVersion(Class aClass, int version)
{
	aClass->version = version;
}

int class_getVersion(Class aClass)
{
	return aClass->version;
}

static inline Method class_getMethod(Class cls, SEL sel)
{
	do {
		register int n;
		register Method smt;
		struct objc_method_list *mlist;

		for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		   smt = mlist->method_list;
		   n = mlist->method_count;

		   while (--n >= 0) {
		      if (sel == smt->method_name)
#ifdef RUNTIME_DYLD
			{
			  _objc_bindModuleContainingList (mlist);
			  return smt;
			}
#else
		      return smt;
#endif
		      smt++;
		   }
		}
	} while ((cls = cls->super_class));

	return 0;
}

Method class_getInstanceMethod(Class aClass, SEL aSelector)
{
	if (aClass && aSelector)
		return class_getMethod(aClass, aSelector);
       	else 
		return 0;
}

Method class_getClassMethod(Class aClass, SEL aSelector)
{
	if (aClass && aSelector)
		return class_getMethod(GETMETA(aClass), aSelector);
       	else 
		return 0;
}

static Ivar class_getVariable(Class cls, const char *name)
{
	do {
		if (cls->ivars) {
			int i;
			Ivar ivars = cls->ivars->ivar_list;

			for (i = 0; i < cls->ivars->ivar_count; i++)	
				if (strcmp(name,ivars[i].ivar_name) == 0)
					return &ivars[i];
		}
	} while ((cls = cls->super_class));

	return 0;
}

Ivar class_getInstanceVariable(Class aClass, const char *name)
{
	if (aClass && name)
		return class_getVariable(aClass, name);	
	else
		return 0;
}

/* Someday add class_getClassVariable(). */


/* Flush the instance and optionally class method caches of all subclasses. */

static void flush_caches(Class cls, BOOL flush_meta)
{
	NXHashTable *class_hash;
	NXHashState state;
	Class clsObject;

        if (cls->cache == 0)
            return;

	OBJC_LOCK (&classLock);

	class_hash = objc_getClasses();
	state = NXInitHashState(class_hash);

	while (NXNextHashState(class_hash, &state, (void **)&clsObject)) {

		Class clsIter = clsObject;

		while (clsIter) {

			if (clsIter == cls) {
				// it is `aKindOf' the class that has
				// been modified - flush!
				_cache_flush (clsObject);
				if (flush_meta)
					_cache_flush (clsObject->isa);
				clsIter = 0;
			} else if (clsIter->isa == cls) {
				_cache_flush (clsObject);
				clsIter = 0;
                        } else if (CLS_GETINFO(clsIter, CLS_INITIALIZED)) {
                                clsIter = clsIter->super_class;
                        } else {
                                clsIter = 0;
                        }
		}
	}

	OBJC_UNLOCK (&classLock);
}

/* Private extern.  Flush instance and class method caches. */

void _objc_flush_caches (Class cls)
{
  flush_caches (cls, YES);
}

/* Formerly class_addInstanceMethods(). */

void class_addMethods(Class cls, struct objc_method_list *meths)
{
	// Insert atomically.
	meths->method_next = cls->methods;
        cls->methods = meths;
	// must flush when dynamically adding methods.
	flush_caches (cls, NO);
}

/* Obsolete (for binary compatibility only). */

void class_addClassMethods(Class cls, struct objc_method_list *meths)
{
	class_addMethods (cls->isa, meths);
}

void class_removeMethods(Class cls, struct objc_method_list *meths)
{
	// Remove atomically.
	if (cls->methods == meths) {
		/* it is at the front of the list - take it out */
		cls->methods = meths->method_next;
	} else {
        	struct objc_method_list *mlist, *prev;

		/* it is not at the front of the list - look for it */
		prev = cls->methods;
		mlist = prev->method_next;

		while (mlist) {
			if (mlist == meths) {
				prev->method_next = mlist->method_next;
				mlist = 0;
			} else {
				prev = mlist;
				mlist = prev->method_next;
			}
		}
	}
	// must flush when dynamically removing methods.
	flush_caches (cls, NO); 
}

/* Private extern */
void _class_removeProtocols(Class cls, struct objc_protocol_list *protos)
{
	// Remove atomically.
	if (cls->protocols == protos) {
		/* it is at the front of the list - take it out */
		cls->protocols = protos->next;
	} else {
        	struct objc_protocol_list *plist, *prev;

		/* it is not at the front of the list - look for it */
		prev = cls->protocols;
		plist = prev->next;

		while (plist) {
			if (plist == protos) {
				prev->next = plist->next;
				plist = 0;
			} else {
				prev = plist;
				plist = prev->next;
			}
		}
	}
}


/*
 * This is a hash table of classes involved in a posing
 * situation.  We use this when we need to get to the "original" 
 * class for some particular name though the function
 * objc_getOrigClass.   For instance, the implementation of
 * [super ...] will use this to be sure that it gets hold of the 
 * correct super class, so that no infinite loops will occur
 * if the class it appears in is involved in posing.
 * We use the classLock to guard this hash table.
 * See tracker bug #51856.
 */

static NXHashTable *posed_class_hash = 0;



Class
objc_getOrigClass (const char *name)
{
    Class ret = 0;

    OBJC_LOCK (&classLock);
    if (posed_class_hash)
      {
        ret = (Class)NXMapGet(posed_class_hash, name);
      }
    OBJC_UNLOCK (&classLock);

    if (ret == nil)
      {
        ret = objc_getClass (name);
      }

    return ret;
}

/*
 * This function is only used from class_poseAs.  It's purpose is to
 * register the original class names, before they get obscured by 
 * posing, so that [super ..] will work correctly from categories 
 * in posing classes and in categories in classes being posed for.
 */

static void
_objc_addOrigClass (Class origClass)
{
    OBJC_LOCK (&classLock);

    if (posed_class_hash == 0)
      {
        posed_class_hash = NXCreateMapTableFromZone (NXStrValueMapPrototype,
                                                     8,
                                                     _objc_create_zone ());
      }

    if (NXMapGet(posed_class_hash, origClass->name) == 0)
      {
        NXMapInsert(posed_class_hash, origClass->name, origClass);
      }

    OBJC_UNLOCK (&classLock);
}

/* !!! class_poseAs() does not currently flush any caches. */

Class class_poseAs(Class imposter, Class original) 
{
	Class clsObject;
	char imposterName[256], *imposterNamePtr; 
	NXHashTable *class_hash;
	NXHashState state;
	Class copy;
	unsigned int hidx, header_count = _objc_headerCount ();
	struct header_info *header_vector = _objc_headerVector (NULL);

	if (imposter == original) 
		return imposter;

	if (imposter->super_class != original)
		return (Class)[(id)imposter error:_errNotSuper, 
					imposter->name, original->name];
	if (imposter->ivars)
		return (Class)[(id)imposter error:_errNewVars, imposter->name, 
					original->name, imposter->name];

	// Build a string to use to replace the name of the original class.
	strcpy (imposterName, "_%"); 
	strcat (imposterName, original->name);

	imposterNamePtr = objc_malloc (strlen(imposterName)+1); 
	strcpy (imposterNamePtr, imposterName);

	// We lock the class hashtable, so we are thread safe with respect to
	// calls to objc_getClass().  However, the class names are not
	// changed atomically, nor are all of the subclasses updated
	// atomically.  I have ordered the operations so that you will
	// never crash, but you may get inconsistent results....

        // register the original class so that [super ..] knows
        // exactly which classes are the "original" classes.
        _objc_addOrigClass (original);
        _objc_addOrigClass (imposter);

	OBJC_LOCK (&classLock);

	class_hash = objc_getClasses ();

	// Remove both the imposter and the original class.
	NXHashRemove(class_hash, imposter);
	NXHashRemove(class_hash, original);

	// Copy the imposter, so that the imposter can continue
	// its normal life in addition to changing the behaviour of
	// the original.  As a hack we don't bother to copy the metaclass.
	// For some reason we modify the original rather than the copy.
	copy = object_copy ((Object *) imposter, 0);
	NXHashInsert (class_hash, copy);

	// Mark the imposter as such (this doesn't appear to be used).
	CLS_SETINFO(imposter, CLS_POSING);
	CLS_SETINFO(imposter->isa, CLS_POSING);

	// Change the name of the imposter to that of the original class.
	imposter->name = original->name;
	imposter->isa->name = original->isa->name;

	// Also copy the version field to avoid archiving problems.
	imposter->version = original->version;

        state = NXInitHashState(class_hash);

	// Change all subclasses of the original to point to the imposter.
	while (NXNextHashState(class_hash, &state, (void **)&clsObject))
	  {
	    while (clsObject && clsObject != imposter && clsObject != copy)
	      {
		if (clsObject->super_class == original)
		  {
		    clsObject->super_class = imposter;
		    clsObject->isa->super_class = imposter->isa;
		    // We must flush caches here!
		    break;
		  }
		clsObject = clsObject->super_class;
	      }
	  }

#ifdef OBJC_CLASS_REFS
	
	for (hidx = 0; hidx < header_count; hidx++)
	  {
	    Class *cls_refs;
	    unsigned int size;
	    
#ifdef RUNTIME_DYLD
	    cls_refs = (Class *) getsectdatafromheaderinfo
					     (&header_vector[hidx],
					      SEG_OBJC, "__cls_refs", &size);
#else
	    cls_refs = (Class *) getsectdatafromheader
					     (header_vector[hidx].mhdr,
					      SEG_OBJC, "__cls_refs", &size);
#endif
	    if (cls_refs)
	      {
		unsigned int i;
		
		for (i = 0; i < size / sizeof (Class); i++)
		  if (cls_refs[i] == original)
		    cls_refs[i] = imposter;
	      }
	  }
#endif /* OBJC_CLASS_REFS */

	// Change the name of the original class.
	original->name = imposterNamePtr+1;
	original->isa->name = imposterNamePtr;

	// Restore the imposter and the original class with their new names.
	NXHashInsert(class_hash, imposter);
	NXHashInsert(class_hash, original);

	OBJC_UNLOCK (&classLock);

	return imposter;
}


static void _freedHandler(id self, SEL sel) 
{
	__objc_error(self, _errFreedObject, SELNAME(sel), self);
}

#ifdef RUNTIME_DYLD
static void _nonexistentHandler(id self, SEL sel)
{
        __objc_error(self, _errNonExistentObject, SELNAME(sel), self);
}
#endif

/*
 *	Purpose: Send the 'initialize' message on demand to any un-initialized 
 *		 class. Force initialization of superclasses first.
 */

/* Called only from _class_lookupMethodAndLoadCache (or itself).
#ifdef OBJC_COLLECTING_CACHE
   The messageLock can be in either state.
#else
   The messageLock is already assumed to be taken out.
   It is temporarily released while the initialize method is sent. 
#endif

*/

static id class_initialize(Class clsDesc)
{
	Class super = clsDesc->super_class;

	if (ISINITIALIZED(clsDesc))
		return clsDesc;

	// force initialization of superclasses first
	if (super != Nil && !ISINITIALIZED(super))
		class_initialize(super);

	// Initializing the super class might have initialized us,
	// or another thread might have intialized us during this time.
	if (ISINITIALIZED(clsDesc))
		return clsDesc;

	MARKINITIALIZED(clsDesc);

#ifndef OBJC_COLLECTING_CACHE
	OBJC_UNLOCK (&messageLock);
#endif

	[clsDesc initialize];

#ifndef OBJC_COLLECTING_CACHE
	OBJC_LOCK (&messageLock);
#endif

	return clsDesc;
}

/* make a private extern in shlib */

void _class_install_relationships(Class class, long version)
{
	Class meta, clstmp;
	int errflag = 0;

	meta = class->isa;
	
	meta->version = version;

	if (class->super_class) {
	  if ((clstmp = objc_getClass((const char *)class->super_class)))
	    class->super_class = clstmp;
	  else
	    errflag = 1;
	}

	if ((clstmp = objc_getClass((const char *)meta->isa)))
	  meta->isa = clstmp->isa;
	else 
	  errflag = 1;

	if (meta->super_class) {
           if ((clstmp = objc_getClass((const char *)meta->super_class)))
              meta->super_class = clstmp->isa;
	  else
	    errflag = 1;
	}
	else /* `tie' the meta class down to its class */
	  meta->super_class = class;

	/* point all classes and meta-classes at static empty cache.  */
	if (class->cache == NULL)
	  class->cache = (Cache) &emptyCache;
	if (meta->cache == NULL)
	  meta->cache = (Cache) &emptyCache;

	if (errflag)
		_objc_fatal("please link appropriate classes in your program");
}

static void *objc_malloc(int size)
{
	void *space = NXZoneMalloc(_objc_create_zone(), size);

	if (space == 0 && size != 0)
	  _objc_fatal("unable to allocate space");

	return space;
}


/* Called from -[Object respondsTo:] and +[Object instancesRespondTo:]. */

BOOL class_respondsToMethod(Class savCls, SEL sel)
{
	Class cls = savCls;
#ifdef OBJC_COPY_CACHE
        struct objc_cache_bucket *buckets;
#else
	Method *buckets;
#endif
	int index, mask;
	
	if (sel == 0) 
		return NO;

	OBJC_LOCK (&messageLock);

	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = (unsigned int) sel & mask;

	for (;;) {
	    if (! CACHE_BUCKET_VALID (buckets[index]))
	      goto cacheMiss;
	    if (CACHE_BUCKET_NAME (buckets[index]) == sel)
	      {
		if (CACHE_BUCKET_IMP (buckets[index]) == &_objc_msgForward)
		  {
		    OBJC_UNLOCK (&messageLock);
		    return NO;
		  }
	        else
		  {
		    OBJC_UNLOCK (&messageLock);
		    return YES;
		  }
	      }
	    index++;
	    index &= mask;
	}

cacheMiss:
	
	do {
		register int n;
		register Method smt;
		struct objc_method_list *mlist;

		for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		   smt = mlist->method_list;
		   n = mlist->method_count;

		   while (--n >= 0) {
			if (sel == smt->method_name)
			  {
#ifdef RUNTIME_DYLD
			    _objc_bindModuleContainingList (mlist);
#endif
#ifdef OBJC_COLLECTING_CACHE
                            OBJC_UNLOCK (&messageLock);
                            _cache_fill(savCls, smt);
#else
                            _cache_fill(savCls, smt);
                            OBJC_UNLOCK (&messageLock);
#endif
			    return YES;
			  }
			smt++;
		   }
		}
	} while ((cls = cls->super_class));
		
	{
	  Method smt = NXZoneMalloc (NXDefaultMallocZone(), sizeof (struct objc_method));
	  
	  smt->method_name = sel;
	  smt->method_types = "";
	  smt->method_imp = &_objc_msgForward;
#ifdef OBJC_COLLECTING_CACHE
          OBJC_UNLOCK (&messageLock);
          _cache_fill(savCls, smt);
#else
          _cache_fill(savCls, smt);
          OBJC_UNLOCK (&messageLock);
#endif
          return NO;
	}
	
}


/* Called from -[Object methodFor:] and +[Object instanceMethodFor:]. */

IMP class_lookupMethod(Class cls, SEL sel)
{
#ifdef OBJC_COPY_CACHE
    struct objc_cache_bucket *buckets;
#else
    Method *buckets;
#endif
	int index, mask;
	IMP result;
	
	if (sel == 0) 
		[(id)cls error:_errBadSel, sel];

	OBJC_LOCK (&messageLock);

	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = (unsigned int) sel & mask;

	for (;;) {
	    if (! CACHE_BUCKET_VALID (buckets[index]))
	      goto cacheMiss;
	    if (CACHE_BUCKET_NAME (buckets[index]) == sel)
	      {
	        result = CACHE_BUCKET_IMP (buckets[index]);
		
		OBJC_UNLOCK (&messageLock);
		return result;
	      }
	    index++;
	    index &= mask;
	}

cacheMiss:
	result = _class_lookupMethodAndLoadCache(cls, sel);
	OBJC_UNLOCK (&messageLock);
	return result;
}


/* Private extern.  Called from objc-load.m and _objc_callLoads(). */

IMP class_lookupMethodInMethodList(struct objc_method_list *mlist, SEL sel)
{
	register int n;
	register Method smt;

	smt = mlist->method_list;
	n = mlist->method_count;

	while (--n >= 0) {
		if (sel == smt->method_name)
#ifdef RUNTIME_DYLD
		  {
		    _objc_bindModuleContainingList (mlist);
		    return smt->method_imp;
		  }
#else
			return smt->method_imp;
#endif
		smt++;
	}
	return 0;
}

#ifdef OBJC_COPY_CACHE
#define BUCKETSIZE(size) 	((size-1) * sizeof(struct objc_cache_bucket))
#else
#define BUCKETSIZE(size) 	((size-1) * sizeof(Method))
#endif

/* Private extern.  Called from _cache_expand() and objc_addClass(). */

Cache _cache_create(Class class)
{
	Cache new_cache;
	int size, i;

	size = (ISMETA (class)) ? META_CACHE_SIZE : CACHE_SIZE;

	// allocate and initialize table...
	new_cache = NXZoneMalloc (NXDefaultMallocZone(),
			sizeof(struct objc_cache) + BUCKETSIZE(size)); 

	for (i = 0; i < size; i++)
	   CACHE_BUCKET_VALID (new_cache->buckets[i]) = (void*)0;
	new_cache->occupied = 0;
	new_cache->mask = size - 1;

	// install the cache
	class->cache = new_cache;

	// Reset the cache flush flag
	class->info &= ~(CLS_FLUSH_CACHE);

        if (_class_slow_grow)
          {
	    // reset the grow flag
            class->info &= ~(CLS_GROW_CACHE);
          }

	return new_cache;
}


/* Called from _cache_fill().
#ifdef OBJC_COLLECTING_CACHE
   The cacheUpdateLock is assumed to be taken at this point. 
#endif
*/

static Cache _cache_expand(Class class)
{
	Cache old_cache, new_cache;
	unsigned int size, i;

	old_cache = class->cache;

	if (old_cache == &emptyCache)
	  return _cache_create (class);

        if (_class_slow_grow)
          {
            if ((class->info & CLS_GROW_CACHE) == 0)
              {
                old_cache->occupied = 0;
                for (i = 0; i < old_cache->mask + 1; i++)
                  {
                    if (CACHE_BUCKET_VALID (old_cache->buckets[i]))
                      {
#ifndef OBJC_COPY_CACHE
                        if (CACHE_BUCKET_IMP (old_cache->buckets[i])
			    == &_objc_msgForward) {
#ifdef OBJC_COLLECTING_CACHE
                            _cache_collect_free(old_cache->buckets[i], 0);
#else
                            NXZoneFree(NXDefaultMallocZone(), old_cache->buckets[i]);
#endif
                        }
#endif
                        CACHE_BUCKET_VALID (old_cache->buckets[i]) = (void*)0;
                    }
                  }
                class->info |= CLS_GROW_CACHE;
                return old_cache;
              }
            else
              {
                class->info &= ~CLS_GROW_CACHE;
              }
          }

	/* make sure size is a power of 2 */
	size = (old_cache->mask + 1) << 1;

	/* allocate and initialize table... */
	new_cache = NXZoneMalloc (NXDefaultMallocZone(),
			sizeof(struct objc_cache) + BUCKETSIZE(size));

	// Zero out new cache
	new_cache->mask = size - 1;
	new_cache->occupied = 0;
	
	for (i = 0; i < size; i++)
	   CACHE_BUCKET_VALID (new_cache->buckets[i]) = (void*)0;

        if (_class_uncache == 0)
          {
	    // Insert old cache entries into new cache
            for (i = 0; i < old_cache->mask + 1; i++)
              {
                if (CACHE_BUCKET_VALID (old_cache->buckets[i]))
                  {
                    int mask = new_cache->mask;
                    int index = (unsigned int) CACHE_BUCKET_NAME (old_cache->buckets[i]) & mask;

                    for (;;)
                      {
                        if (! CACHE_BUCKET_VALID (new_cache->buckets[index]))
                          {
                            new_cache->buckets[index] = old_cache->buckets[i];
                            break;
                          }
                        index++;
                        index &= mask;
                      }
                    new_cache->occupied++;
                  }
              }
	
	    // Set the cache flush flag so that we will flush this cache
	    // before expanding it again.
            class->info |= CLS_FLUSH_CACHE;
          }
        else
          {
	    // free any malloced method descriptors...class_respondsToMethod()
	    // does this for negative caching.
#ifndef OBJC_COPY_CACHE
            for (i = 0; i < old_cache->mask + 1; i++) {
                if (CACHE_BUCKET_VALID (old_cache->buckets[i]) &&
                    CACHE_BUCKET_IMP (old_cache->buckets[i]) == &_objc_msgForward) {
#ifdef OBJC_COLLECTING_CACHE
                    _cache_collect_free(old_cache->buckets[i], 0);
#else
                    NXZoneFree(NXDefaultMallocZone(), old_cache->buckets[i]);
#endif
                }
	      }
#endif
          }

	class->cache = new_cache;

#ifdef OBJC_COLLECTING_CACHE
        _cache_collect_free(old_cache, 1);
#else
	NXZoneFree(NXDefaultMallocZone(), old_cache);
#endif
	return new_cache;
}

/* Called only from _class_lookupMethodAndLoadCache and class_respondsToMethod.
#ifdef OBJC_COLLECTING_CACHE
   It doesn't matter if someone has the messageLock when we enter this function.
   This function will fail to do the update if someone else is already updating
   the cache, i.e. they have the cacheUpdateLock.
#else
   The messageLock is already assumed to be taken out.
#endif
 */

static void _cache_fill(Class class, Method smt)
{
    Cache cache;
    SEL sel = smt->method_name;
#ifdef OBJC_COPY_CACHE
    IMP imp = smt->method_imp;
    struct objc_cache_bucket *buckets;
#else
    Method *buckets;
#endif
    int index, mask;
    unsigned int new_count;

#ifdef OBJC_COLLECTING_CACHE
    if (! OBJC_TRYLOCK (&cacheUpdateLock))
        return;

    cache = class->cache;
    mask = cache->mask;
    index = (unsigned int) sel & mask;
    buckets = cache->buckets;

    for (;;) {
        if (! CACHE_BUCKET_VALID (buckets[index]))
            break;
        if (CACHE_BUCKET_NAME (buckets[index]) == sel)
          {
            OBJC_UNLOCK (&cacheUpdateLock);
            return;
          }
        index++;
        index &= mask;
    }

#else
    cache = class->cache;
    mask = cache->mask;
#endif


    /* Expand or flush the cache if more than 3/4 full.  */
    new_count = cache->occupied + 1;
    if ((new_count * 4) > (mask + 1) * 3)
      {
        if (class->info & CLS_FLUSH_CACHE)
            _cache_flush (class);		// Clears CLS_FLUSH_CACHE bit
        else
          {
            cache = _cache_expand (class);	// Sets CLS_FLUSH_CACHE bit
            mask = cache->mask;
          }
        cache->occupied++;
      }
    else
        cache->occupied = new_count;

    buckets = cache->buckets;

    index = (unsigned int)sel & mask;
    
#ifdef OBJC_COPY_CACHE
    imp = smt->method_imp;
    sel = smt->method_name;
#endif
    for (;;)
      {
#ifdef OBJC_COPY_CACHE
#ifdef OBJC_COLLECTING_CACHE
        if (! CACHE_BUCKET_VALID (buckets[index]))
          {
            CACHE_BUCKET_IMP  (buckets[index]) = imp;
            CACHE_BUCKET_NAME (buckets[index]) = sel;

            /* we can free it right away, because it was just
               allocated from _class_lookupMethodAndLoadCache */
            if (smt->method_imp == &_objc_msgForward)
                NXZoneFree (NXDefaultMallocZone (), smt);
            break;
          }
#else
        struct objc_cache_bucket prev = buckets[index];

        CACHE_BUCKET_NAME (buckets[index]) = sel;
        CACHE_BUCKET_IMP  (buckets[index]) = imp;

        if (! CACHE_BUCKET_VALID (prev))
          {
            if (smt->method_imp == &_objc_msgForward)
                NXZoneFree (NXDefaultMallocZone (), smt);
            break;
          }

        sel = CACHE_BUCKET_NAME (prev);
        imp = CACHE_BUCKET_IMP  (prev);
#endif /* OBJC_COLLECTING_CACHE */
        index++;
        index &= mask;
#else
        Method prev = buckets[index];
        buckets[index] = smt;

        if (prev == NULL)
            break;

        smt = prev;
        index++;
        index &= mask;
#endif	
      }

#ifdef OBJC_COLLECTING_CACHE
    OBJC_UNLOCK (&cacheUpdateLock);
#endif
}


/* Called from flush_caches(). */

static void _cache_flush (Class class)
{
  Cache cache = class->cache;
  unsigned int i;
  
  if (cache == &emptyCache)
    return;
  
  // free any malloced method descriptors...class_respondsToMethod()
  // does this for negative caching.
  for (i = 0; i <= cache->mask; i++)
    {
#ifndef OBJC_COPY_CACHE
      if (cache->buckets[i] &&
	  cache->buckets[i]->method_imp == &_objc_msgForward)
#ifdef OBJC_COLLECTING_CACHE
                    _cache_collect_free(cache->buckets[i], 0);
#else
                    NXZoneFree(NXDefaultMallocZone(), cache->buckets[i]);
#endif
#endif
      CACHE_BUCKET_VALID (cache->buckets[i]) = (void*)0;
    }
  
  cache->occupied = 0;
  
  // Reset the cache flush flag
  class->info &= ~CLS_FLUSH_CACHE;
}


/* Called only from _class_lookupMethodAndLoadCache.
   The messageLock is already assumed to be taken out. */

static inline Method _class_lookupMethod(Class cls, SEL sel)
{
	register int n;
	register Method smt;
	struct objc_method_list *mlist;

	for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		smt = mlist->method_list;
		n = mlist->method_count;

		while (--n >= 0) {
			if (sel == smt->method_name) {
#ifdef RUNTIME_DYLD
			  {
			    _objc_bindModuleContainingList (mlist);
			    return smt;
			  }
#else
			  return smt;
#endif
			}
			smt++;
		}
	}
	return 0;
}


/* Returns a pointer to the dummy freed object class (private extern).  */

Class _objc_getFreedObjectClass (void) { return (Class) &freedObjectClass; }

#ifdef RUNTIME_DYLD
Class _objc_getNonexistentClass (void) { return (Class) &nonexistentObjectClass; }
#endif


/* Called only from objc_msgSend, objc_msgSendSuper and class_lookupMethod.
   (*NOT*) The messageLock is already assumed to be taken out. */

IMP _class_lookupMethodAndLoadCache(Class savCls, SEL sel)
{
	register Class cls = savCls;
	Method method;

	if (cls == &freedObjectClass)
	  return (IMP)_freedHandler;
#ifdef RUNTIME_DYLD
        if (cls == &nonexistentObjectClass)
          return (IMP)_nonexistentHandler;
#endif

	// lazy initialization...
	if (CLS_GETINFO(cls,CLS_META) && !ISINITIALIZED(cls))
		class_initialize(objc_getClass (cls->name));

	do {
		method = _class_lookupMethod(cls, sel);
		if (method) {
			_cache_fill(savCls, method);
			return method->method_imp;
		}
	} while ((cls = cls->super_class));

	// class does not respond -- try forwarding
	{
		Method smt = NXZoneMalloc(NXDefaultMallocZone(), sizeof(struct objc_method));
		smt->method_name = sel;
		smt->method_types = "";
		smt->method_imp = &_objc_msgForward;
		_cache_fill(savCls, smt);
	}
	return &_objc_msgForward;
}

/* delegation */

static int SubTypeUntil (const char *type, char end) 
{
    int		level = 0;
    const char	*head = type;
    while (*type) {
	if (!*type || (! level && (*type == end)))
	    return (int)(type - head);
	switch (*type) {
	    case ']': case '}': case ')': level--; break;
	    case '[': case '{': case '(': level++; break;
	}
	type ++;
    }
    _NXLogError("Object: SubTypeUntil: end of type encountered prematurely\n");
    return 0;
}

static const char *SkipFirstType (const char *type) 
{
  while (1)
    {
    switch (*type++) 
      {
	case 'O':	/* bycopy */
	case 'n':	/* in */
	case 'o':	/* out */
	case 'N':	/* inout */
	case 'r':	/* const */
	case 'V':	/* oneway */
	case '^':	/* pointers */
            	break;

	/* arrays */
    	case '[':
	    	while ('0' <= *type && '9' >= *type) type++;
	    	return type + SubTypeUntil(type, ']') + 1;

	/* structures */
	case '{':
	    	return type + SubTypeUntil(type, '}') + 1;

	/* unions */
	case '(':
	    	return type + SubTypeUntil(type, ')') + 1;
	    
	/* basic types */
	default: 
	    	return type;
      }
    }
}

unsigned method_getNumberOfArguments(Method method)
{
	const char *typedesc = method->method_types;
	unsigned nargs = 0;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	/* next, skip stack size */
	while ('0' <= *typedesc && '9' >= *typedesc) 
		typedesc++;

	/* now, we have the arguments - count how many */
	while (*typedesc) {

		/* skip argument type */
		typedesc = SkipFirstType(typedesc);
		
		/* next is the argument offset, blow it off */
		if (*typedesc == '-')  /* skip negative sign in offset */
			typedesc++;
		while ('0' <= *typedesc && '9' >= *typedesc) 
			typedesc++;

		nargs++;
	}
	return nargs;
}

unsigned method_getSizeOfArguments(Method method)
{
	const char *typedesc = method->method_types;
	unsigned stack_size = 0;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	while ('0' <= *typedesc && '9' >= *typedesc) 
		stack_size = stack_size * 10 + (*typedesc++ - '0');

	return stack_size;
}

unsigned method_getArgumentInfo(Method method, int arg, 
				const char **type, int *offset)
{
	const char *typedesc = method->method_types;
	unsigned nargs = 0;
	unsigned self_offset = 0;
	BOOL offset_is_negative = NO;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	/* next, skip stack size */
	while ('0' <= *typedesc && '9' >= *typedesc) 
		typedesc++;

	/* now, we have the arguments - position typedesc to the appropriate argument */
	while (*typedesc && nargs != arg) {

		/* skip argument type */
		typedesc = SkipFirstType(typedesc);
		
		if (nargs == 0) {
			if (*typedesc == '-') { /* skip negative sign in offset */
				offset_is_negative = YES;
				typedesc++;
			} else {
				offset_is_negative = NO;
			}
			while ('0' <= *typedesc && '9' >= *typedesc) 
				self_offset = self_offset * 10 + (*typedesc++ - '0');
			if (offset_is_negative) 
				self_offset = - self_offset;
		} else {
			/* next is the argument offset, blow it off */
			if (*typedesc == '-') 
				typedesc++;
			while ('0' <= *typedesc && '9' >= *typedesc) 
				typedesc++;
		}
		nargs++;
	}
	if (*typedesc) {
		unsigned arg_offset = 0;

		*type = typedesc;
		typedesc = SkipFirstType(typedesc);

		if (arg == 0) {
#if hppa
			*offset = -sizeof(id);
#else
			*offset = 0;
#endif hppa			
		} else {
			if (*typedesc == '-') { /* skip negative sign in offset */
				offset_is_negative = YES;
				typedesc++;
			} else {
				offset_is_negative = NO;
			}
			while ('0' <= *typedesc && '9' >= *typedesc) 
				arg_offset = arg_offset * 10 + (*typedesc++ - '0');
			if (offset_is_negative) 
				arg_offset = - arg_offset;
				
#if hppa
			/*
			 * For stacks which grow up, since margs points
			 * to the top of the stack or the END of the args, 
			 * the first offset is at -sizeof(id) rather than 0.
			 */
			self_offset += sizeof(id);
#endif
			*offset = arg_offset - self_offset;
		}
	} else {
		*type = 0; *offset = 0;
	}
	return nargs;
}


/* Private extern. */

NXZone *_objc_create_zone (void)
{
  static NXZone *_objc_zone = 0;

  if (!_objc_zone)
    {
      _objc_zone = NXCreateZone (vm_page_size, vm_page_size, YES);
      NXNameZone (_objc_zone, "ObjC");
    }
  
  return _objc_zone;
}



/* cache collection */
#ifdef OBJC_COLLECTING_CACHE

#ifdef hppa
static unsigned long
_get_pc_for_thread (thread_t thread)
{
    struct hp_pa_frame_thread_state state;
    unsigned int count = HPPA_FRAME_THREAD_STATE_COUNT;
    thread_get_state (thread, HPPA_FRAME_THREAD_STATE, (thread_state_t)&state, &count);
    return state.ts_pcoq_front;
}
#elif defined (i386)
static unsigned long
_get_pc_for_thread (thread_t thread)
{
    i386_thread_state_t state;
    unsigned int count = i386_THREAD_STATE_COUNT;
    thread_get_state (thread, i386_THREAD_STATE, (thread_state_t)&state, &count);
    return state.eip;
}

#elif defined (m68k)
static unsigned long
_get_pc_for_thread (thread_t thread)
{
    struct m68k_thread_state_regs state;
    unsigned int count = M68K_THREAD_STATE_REGS_COUNT;
    thread_get_state (thread, M68K_THREAD_STATE_REGS, (thread_state_t)&state, &count);
    return state.pc;
}

#else
/* if OBJC_COLLECTING_CACHE is defined, you need the function above... */
{ you loose; }
#endif

extern unsigned long objc_entryPoints[];
extern unsigned long objc_exitPoints[];

static int
_collecting_in_critical ()
{
    thread_array_t threads;
    unsigned number, count;
    kern_return_t ret;
    int result = 0;

    ret = task_threads (task_self (), &threads, &number);
    if (ret != KERN_SUCCESS)
      {
        _NXLogError ("objc: task_thread failed\n");
        exit (1);
      }

    for (count = 0; !result && count < number; count++)
      {
        int region;
        unsigned long pc;

        if (threads[count] == thread_self ())
            continue;

        pc = _get_pc_for_thread (threads[count]);

        for (region = 0; objc_entryPoints[region]; region++)
          {
            if (   pc >= objc_entryPoints[region]
                && pc <= objc_exitPoints[region])
              {
                result = 1;
              }
          }
      }

    vm_deallocate(task_self(), (vm_address_t)threads, sizeof(threads)*count);
    return result;
}


static int garbage_byte_size = 0;
static int garbage_threshold = 1024;

static void **garbage_refs = 0;
static int garbage_count = 0;
static int garbage_max = 0;

static void
_garbage_make_room ()
{
    static int first = 1;

    if (first)
      {
        first = 0;
        garbage_refs = NXZoneMalloc (NXDefaultMallocZone (), 128 * sizeof (void*));
        garbage_max = 128;
      }
    else if (garbage_count == garbage_max)
      {
        garbage_refs = NXZoneRealloc (NXDefaultMallocZone (), garbage_refs, garbage_max*2);
        garbage_max *= 2;
      }
}

static void _cache_collect_free(void *data, BOOL tryCollect)
{
    char * report_garbage = getenv ("OBJC_REPORT_GARBAGE");
    OBJC_LOCK (&cacheCollectionLock);

    /* insert new element in garbage list */
    _garbage_make_room ();    
    garbage_byte_size += malloc_size (data);
    garbage_refs[garbage_count++] = data;

    if (tryCollect && report_garbage)
      {
        _NXLogError ("objc: total of %d bytes of garbage ...", garbage_byte_size);
      }

    if (!tryCollect || garbage_byte_size < garbage_threshold)
      {
        OBJC_UNLOCK (&cacheCollectionLock);
        if (tryCollect && report_garbage)
          {
            _NXLogError ("below threshold\n");
          }

        return;
      }

     /* If someone has the explicit read lock, we don't 
        try to garbage collect */
    if (OBJC_TRYLOCK (&messageLock))
      {
        if (! _collecting_in_critical ())
          {
            if (tryCollect && report_garbage)
              {
                _NXLogError ("collecting!\n");
              }

            while (garbage_count)
                free (garbage_refs[--garbage_count]);

            garbage_byte_size = 0;
          }
        else if (tryCollect && report_garbage)
          {
            _NXLogError ("in critical region\n");
          }

        OBJC_UNLOCK (&messageLock);
      }
    else if (tryCollect && report_garbage)
      {
        _NXLogError ("messageLock taken\n");
      }

    OBJC_UNLOCK (&cacheCollectionLock);
    return;

}

#endif /* OBJC_COLLECTING_CACHE */



#ifndef KERNEL

static void _cache_print (Cache cache)
{
  unsigned int i, count = cache->mask + 1;
  
  for (i = 0; i < count; i++)
    if (CACHE_BUCKET_VALID (cache->buckets[i]))
      {
	if (CACHE_BUCKET_IMP (cache->buckets[i]) == &_objc_msgForward)
	  printf ("does not recognize \n");
	printf ("%s\n", (const char *) CACHE_BUCKET_NAME (cache->buckets[i]));
      }
}


void _class_printMethodCaches (Class class)
{
  if (class->cache == &emptyCache)
    printf ("no instance-method cache for class %s\n", class->name);
  else
    {
      printf ("instance-method cache for class %s:\n", class->name);
      _cache_print (class->cache);
    }
  
  if (class->isa->cache == &emptyCache)
    printf ("no class-method cache for class %s\n", class->name);
  else
    {
      printf ("class-method cache for class %s:\n", class->name);
      _cache_print (class->isa->cache);
    }
}


static unsigned int log2 (unsigned int x)
{
  unsigned int log = 0;
  
  while (x >>= 1)
    log++;
  
  return log;
}


#define MAX_LOG_SIZE 32
#define MAX_CHAIN_SIZE 100

void _class_printMethodCacheStatistics (void)
{
  NXHashTable *class_hash = objc_getClasses ();
  NXHashState state = NXInitHashState (class_hash);
  Class class;
  unsigned int classCount = 0;
  unsigned int cacheCountBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int totalEntriesBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int maxEntriesBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int totalNegativeEntries = 0;
  unsigned int totalChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int totalMissChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int totalMaxChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int totalMaxMissChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int maxChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int maxMissChainBySize[2][MAX_LOG_SIZE] = {0};
  unsigned int chainCount[MAX_CHAIN_SIZE] = {0};
  unsigned int missChainCount[MAX_CHAIN_SIZE] = {0};
  
  while (NXNextHashState (class_hash, &state, (void **) &class))
    {
      unsigned int isMeta;
      
      classCount++;
      
      for (isMeta = 0; isMeta <= 1; isMeta++)
        {
	  Cache cache = isMeta ? class->isa->cache : class->cache;
	  
	  if (cache != &emptyCache)
	    {
	      unsigned int i;
	      unsigned int mask = cache->mask;
	      unsigned int count = mask + 1;
	      unsigned int size = log2 (count);
	      unsigned int entries = 0;
	      unsigned int negativeEntries = 0;
	      unsigned int totalChain = 0;
	      unsigned int totalMissChain = 0;
	      unsigned int maxChain = 0;
	      unsigned int maxMissChain = 0;
	      
	      cacheCountBySize[isMeta][size]++;
	      
	      for (i = 0; i < count; i++)
	        {
#ifdef OBJC_COPY_CACHE
		  struct objc_cache_bucket *buckets = cache->buckets;
#else
		  Method *buckets = cache->buckets;
#endif
		  
		  if (CACHE_BUCKET_VALID (buckets[i]))
		    {
#ifdef OBJC_COPY_CACHE
		      struct objc_cache_bucket method = buckets[i];
#else
		      Method method = buckets[i];
#endif
		      unsigned int hash = (unsigned int) CACHE_BUCKET_NAME (method);
		      unsigned int chain = (i - hash) & mask;
		      unsigned int missChain;
		      unsigned int j = i;
		      
		      entries++;
		      
		      if (CACHE_BUCKET_IMP (method) == &_objc_msgForward)
			negativeEntries++;
		      
		      if (chain < MAX_CHAIN_SIZE)
			chainCount[chain]++;
		      totalChain += chain;
		      if (chain > maxChain)
		        maxChain = chain;
		      
		      while (CACHE_BUCKET_VALID (buckets[j]))
			{
			  j++;
			  j &= mask;
			}
		      
		      missChain = (j - i) & mask;
		      if (missChain < MAX_CHAIN_SIZE)
			missChainCount[missChain]++;
		      totalMissChain += missChain;
		      if (missChain > maxMissChain)
		        maxMissChain = missChain;
		    }
		  else
		    missChainCount[0]++;
		}
	      
	      totalEntriesBySize[isMeta][size] += entries;
	      if (entries > maxEntriesBySize[isMeta][size])
	        maxEntriesBySize[isMeta][size] = entries;
	      totalNegativeEntries += negativeEntries;
	      totalChainBySize[isMeta][size] += totalChain;
	      totalMissChainBySize[isMeta][size] += totalMissChain;
	      totalMaxChainBySize[isMeta][size] += maxChain;
	      totalMaxMissChainBySize[isMeta][size] += maxMissChain;
	      if (maxChain > maxChainBySize[isMeta][size])
	        maxChainBySize[isMeta][size] = maxChain;
	      if (maxMissChain > maxMissChainBySize[isMeta][size])
	        maxMissChainBySize[isMeta][size] = maxMissChain;
	    }
	}
    }
  
    {
      unsigned int isMeta, i;
      unsigned int cacheCount[2] = {0};
      unsigned int totalCacheCount = 0;
      unsigned int totalEntries = 0;
      unsigned int totalSize = 0;
      unsigned int totalChain = 0;
      unsigned int maxChain = 0;
      unsigned int totalMissChain = 0;
      unsigned int maxMissChain = 0;
      
      for (isMeta = 0; isMeta <= 1; isMeta++)
        {
	  for (i = 0; i < MAX_LOG_SIZE; i++)
	    {
	      cacheCount[isMeta] += cacheCountBySize[isMeta][i];
	      totalEntries += totalEntriesBySize[isMeta][i];
	      totalSize += cacheCountBySize[isMeta][i] * (1 << i);
	      totalChain += totalChainBySize[isMeta][i];
	      if (maxChainBySize[isMeta][i] > maxChain)
	        maxChain = maxChainBySize[isMeta][i];
	      totalMissChain += totalMissChainBySize[isMeta][i];
	      if (maxMissChainBySize[isMeta][i] > maxMissChain)
	        maxMissChain = maxMissChainBySize[isMeta][i];
	    }
	  
	  totalCacheCount += cacheCount[isMeta];
	}
      
      for (isMeta = 0; isMeta <= 1; isMeta++)
        {
	  printf ("%u %s-method caches (out of %u classes):\n",
		  cacheCount[isMeta],
		  isMeta ? "class" : "instance",
		  classCount);
	  
	  printf ("\n");
	  
	  printf ("%s-method cache size distribution:\n",
		  isMeta ? "class" : "instance");

	  for (i = 0; i < MAX_LOG_SIZE; i++)
	    if (cacheCountBySize[isMeta][i])
	      printf ("  %d slots: %u\n", 1 << i, cacheCountBySize[isMeta][i]);
	  
	  printf ("\n");
	  
	  for (i = 0; i < MAX_LOG_SIZE; i++)
	    if (cacheCountBySize[isMeta][i])
	      {
		printf ("  %u %s-method caches with %d slots (%u%% efficiency):\n",
			cacheCountBySize[isMeta][i],
			isMeta ? "class" : "instance",
			1 << i,
			(100 * totalEntriesBySize[isMeta][i] + 50) /
			(cacheCountBySize[isMeta][i] * (1 << i)));
		printf ("  %.2f average entries (%u maximum)\n",
			(float) totalEntriesBySize[isMeta][i] /
			(float) cacheCountBySize[isMeta][i],
			maxEntriesBySize[isMeta][i]);
		printf ("  %.2f average lookup chain (%u maximum)\n",
			(float) totalChainBySize[isMeta][i] /
			(float) totalEntriesBySize[isMeta][i],
			maxChainBySize[isMeta][i]);
		printf ("  %.2f average miss chain (%u maximum)\n",
			(float) totalMissChainBySize[isMeta][i] /
			(float) (cacheCountBySize[isMeta][i] * (1 << i)),
			maxMissChainBySize[isMeta][i]);
      
		printf ("\n");
	      }
	}
      
      printf ("cummulative method cache statistics:\n");
      printf ("  %u entries in %u slots (%u%% efficiency)\n",
	      totalEntries, totalSize,
	      (100 * totalEntries + 50) / totalSize);
      printf ("  %u negative entries (%u%%)\n",
	      totalNegativeEntries,
	      (100 * totalNegativeEntries + 50) / totalEntries);
      printf ("  %.2f average lookup chain (%u maximum)\n",
	      (float) totalChain / (float) totalEntries,
	      maxChain);
      printf ("  %.2f average miss chain (%u maximum)\n",
	      (float) totalMissChain / (float) totalSize,
	      maxMissChain);
#ifdef OBJC_COPY_CACHE
      printf ("  %lu bytes malloced data\n",
	      totalCacheCount * (sizeof (struct objc_cache) - sizeof (struct objc_cache_bucket))+
	      totalSize * sizeof (struct objc_cache_bucket) +
	      totalNegativeEntries * sizeof (struct objc_method));
#else
      printf ("  %lu bytes malloced data\n",
	      totalCacheCount * (sizeof (struct objc_cache) - sizeof (Method))+
	      totalSize * sizeof (Method) +
	      totalNegativeEntries * sizeof (struct objc_method));
#endif
      
      printf ("\n");
      
      printf ("lookup chain length distribution:\n");
      
      for (i = 0; i < MAX_CHAIN_SIZE; i++)
        if (chainCount[i])
	  printf ("  length %u: %u\n", i, chainCount[i]);
      
      printf ("\n");
      
      printf ("miss chain length distribution:\n");
      
      for (i = 0; i < MAX_CHAIN_SIZE; i++)
        if (missChainCount[i])
	  printf ("  length %u: %u\n", i, missChainCount[i]);
      
      printf ("\n");
    }
}

#endif /* KERNEL */
