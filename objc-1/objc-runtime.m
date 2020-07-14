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
 *	objc-runtime.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"
#include <mach-o/ldsyms.h>
#import "objc-runtime.h"
#import "hashtable.h"
#import "maptable.h"
#import "Object.h"
#import "Protocol.h"

void *getsectdatafromheader(const struct mach_header *, char *, char *, int *);
static const struct segment_command *
getobjcsegmentfromheader(const struct mach_header *mhp);

void (*callbackFunction)( Class, Category ) = 0;

void *getsectdatafromheaderinfo(const struct header_info * info, 
				char * segname, 
				char * sectname, 
				int * size)
{
  char *addr = getsectdatafromheader (info->mhdr, segname, sectname, size);
  if (addr)
    addr += info->image_slide;
  return addr;
}

/* Turn on support for literal string objects. */
#define LITERAL_STRING_OBJECTS

/* system vectors...created at runtime by reading the `__OBJC' segments 
 * that are a part of the application.
 */

/* These two variables are now obsolete.
   They have been replaced by header_vector and header_count. */

static unsigned int module_count;
static Module *module_vector;	

/* We do not lock these variables, since they are only set during startup. */

static struct header_info *header_vector = 0;
static unsigned int header_count = 0;

/*
 * 	Hash table of classes...
 */
static NXHashTable *class_hash = 0;

/* Mask which specifies whether we are multi-threaded or not.
   A value of -1 means single-threaded, 0 means multi-threaded. */
int _objc_multithread_mask = -1;

/* Lock for class hashtable. (Private extern) */

OBJC_DECLARE_LOCK (classLock);

static unsigned classHash (void *info, Class data) 
{
	return (data) ? _objc_strhash ((unsigned char *)((Class) data)->name) : 0;
}
    
static int classIsEqual (void *info, Class name, Class cls) 
{
	return ((name->name[0] == cls->name[0]) &&
	 	(strcmp(name->name, cls->name) == 0));
}

static NXHashTablePrototype classHashPrototype = 
{
	(unsigned (*)(const void *, const void *))classHash, 
	(int (*)(const void *, const void *, const void *))classIsEqual, 
	NXNoEffectFree, 0
};

/* This function is very dangerous, since you cannot safely use the hashtable
   without locking it, and the lock is private! */

NXHashTable *objc_getClasses (void)
{
	return class_hash;
}

static int _objc_defaultClassHandler(const char *clsName)
{
	_objc_inform("class `%s' not linked into application", clsName);
	return 0;
}

static int (*objc_classHandler)(const char *) = _objc_defaultClassHandler;

void objc_setClassHandler(int (*userSuppliedHandler)(const char *))
{
	objc_classHandler = userSuppliedHandler;
}

id objc_getClass(const char *aClassName) 
{ 
	struct objc_class cls;
	id ret;

	OBJC_LOCK (&classLock);
	cls.name = aClassName;

	if (!(ret = (id)NXHashGet(class_hash, &cls)))
	  {
	    int result;
	    
	    OBJC_UNLOCK (&classLock);
	    result = (*objc_classHandler)(aClassName);
	    OBJC_LOCK (&classLock);
	    
	    if (result)
	      ret = (id)NXHashGet(class_hash, &cls);
	  }

	OBJC_UNLOCK (&classLock);
	return ret;
}

/* Formerly objc_getClassWithoutWarning(). */

id objc_lookUpClass (const char *aClassName) 
{ 
	struct objc_class cls;
	id ret;

	OBJC_LOCK (&classLock);
	cls.name = aClassName;
	
	ret = (id)NXHashGet(class_hash, &cls);

	OBJC_UNLOCK (&classLock);
	return ret;
}

id objc_getMetaClass(const char *aClassName) 
{ 
  Class class = objc_getClass (aClassName);
  
  if (class)
    return class->isa;
  else
    return Nil;
}

void objc_addClass (Class class) 
{
  OBJC_LOCK (&classLock);

  // make sure both the class and the metaclass have caches!
  // Clear all bits of the info fields except CLS_CLASS and CLS_META.
  // Normally these bits are already clear but if someone tries to cons
  // up their own class on the fly they may need to be cleared.

  if (class->cache == NULL) {
    class->cache = (Cache) &emptyCache;
    class->info = CLS_CLASS;
  }
  if (class->isa->cache == NULL) {
    class->isa->cache = (Cache) &emptyCache;
    class->isa->info = CLS_META;
  }
  
  NXHashInsert (class_hash, class);
  OBJC_UNLOCK (&classLock);

}

/* Private extern used by objc_unloadModules(). */
void _objc_removeClass (Class class)
{
  OBJC_LOCK (&classLock);
  NXHashRemove (class_hash, class);
  OBJC_UNLOCK (&classLock);

}

/* This only returns the statically linked modules.
   I do not believe this is used now.
   It has been replaced by _objc_headerVector(). */

Module *objc_getModules (void)
{
    if (!module_vector) {
        Module mods;
        unsigned int hidx, midx;
        Module *mvp;

        for (hidx = 0; hidx < header_count; hidx++)
            module_count += header_vector[hidx].mod_count;

        if (!(module_vector = NXZoneMalloc(_objc_create_zone(), sizeof(Module) * (module_count+1))))
            _objc_fatal("unable to allocate module vector");

        for (hidx = 0, mvp = module_vector;
             hidx < header_count; hidx++) {
            for (mods = header_vector[hidx].mod_ptr, midx = 0;
                 midx < header_vector[hidx].mod_count;
                 midx++, mvp++) {

                *mvp = mods+midx;
            }
        }
        *mvp = 0;
    }
    return module_vector;
}

/* What could this be used for?  It is not currently used. */

Module *objc_addModule(Module myModule) 
{
	module_count++;
	if (module_vector) {
		module_vector = (Module *)NXZoneRealloc(_objc_create_zone(),
					module_vector,
					sizeof(Module) * (module_count + 1));
	} else {
		module_vector = (Module *)NXZoneMalloc(_objc_create_zone(),
					sizeof(Module) * (module_count + 1));
	}
	if (!module_vector)
	   _objc_fatal("unable to reallocate module vector");

	module_vector[module_count-1] = myModule;
	module_vector[module_count] = 0;
	return module_vector;
}

/* Private extern used by objc_unloadModules(). */

void _objc_remove_category(struct objc_category *category, int version)
{
	Class cls;

	if ((cls = objc_getClass(category->class_name))) {
		if (category->instance_methods)
	  	      class_removeMethods(cls, category->instance_methods);

		if (category->class_methods)
		      class_removeMethods(cls->isa, category->class_methods);
		if (version >= 5 && category->protocols)
		      _class_removeProtocols (cls, category->protocols);
	} else {
		_objc_inform("unable to remove category %s...\n",
				category->category_name);
		_objc_inform("class `%s' not linked into application\n",
				category->class_name);
	}
}

static inline void
__objc_add_category(struct objc_category *category, int version)
{
	Class cls;

	if ((cls = (Class)objc_getClass(category->class_name))) {
		if (category->instance_methods) {
	  	  category->instance_methods->method_next = cls->methods;
		  cls->methods = category->instance_methods;
		}
		if (category->class_methods) {
		  category->class_methods->method_next = cls->isa->methods;
                  cls->isa->methods = category->class_methods;
		}
		if (version >= 5 && category->protocols)
		  {
		    if (cls->isa->version >= 5)
		      {
		        category->protocols->next = cls->protocols;
			cls->protocols = category->protocols;
			cls->isa->protocols = category->protocols;
		      }
		    else
		      {
			_objc_inform ("unable to add protocols from category %s...\n",
				      category->category_name);
			_objc_inform ("class `%s' must be recompiled\n",
				      category->class_name);
		      }
		  }
	} else {
		_objc_inform("unable to add category %s...\n",
				category->category_name);
		_objc_inform("class `%s' not linked into application\n",
				category->class_name);
	}
}

/* Private extern used by objc_loadModules(). */

void _objc_add_category(struct objc_category *category, int version)
{
  __objc_add_category (category, version);
  
  _objc_flush_caches (objc_getClass (category->class_name));
}

struct _objc_unresolved_category {
    struct _objc_unresolved_category* next;
    struct objc_category* cat;
    long version;
};

static NXMapTable* category_hash = 0;

static void _objc_resolve_categories_for_class (Class cls)
{
    struct _objc_unresolved_category *cat, *next;

    if (!category_hash)
        return;

    cat = NXMapRemove (category_hash, cls->name);

    while (cat)
      {
        _objc_add_category (cat->cat, cat->version);

        next = cat->next;
        free (cat);
        cat = next;
      }
}


static void _objc_register_category (struct objc_category *cat, long version)
{
    struct _objc_unresolved_category *new_cat, *old;

    if (objc_lookUpClass (cat->class_name))
      {
        _objc_add_category (cat, version);
        return;
      }

    if (!category_hash)
        category_hash = NXCreateMapTableFromZone (NXStrValueMapPrototype,
                                                  128,
                                                  _objc_create_zone ());

    old = NXMapGet (category_hash, cat->class_name);
    new_cat = NXZoneMalloc (_objc_create_zone(), sizeof (struct _objc_unresolved_category));
    new_cat->next = old;
    new_cat->cat = cat;
    new_cat->version = version;
    NXMapInsert (category_hash, cat->class_name , new_cat);
}


static void _objc_add_categories_from_image (struct header_info *hi)
{
    Module mods;
    unsigned int midx;

    for (mods = hi->mod_ptr, midx = 0;
         midx < hi->mod_count;
         midx++) {
        unsigned int i, total; 

        if (mods[midx].symtab == NULL)
            continue;

        total = mods[midx].symtab->cls_def_cnt +
            mods[midx].symtab->cat_def_cnt;

        /* add categories */
        for (i = mods[midx].symtab->cls_def_cnt;
             i < total; i++)
            _objc_register_category(mods[midx].symtab->defs[i],
                                     mods[midx].version);
    }
}

static const struct header_info *_headerForClass(Class class)
{
	const struct mach_header *foundHeader;
	const struct segment_command *objcSeg;
	unsigned int hidx;
	
	for (foundHeader = NULL, hidx = 0;
	     !foundHeader && (hidx < header_count);
	     hidx++)
          {
            unsigned long slide = header_vector[hidx].image_slide;
            objcSeg = getobjcsegmentfromheader(header_vector[hidx].mhdr);
            if ((objcSeg->vmaddr + slide <= (unsigned long)class)
                && ((unsigned long)class < (objcSeg->vmaddr + slide + objcSeg->vmsize)))
              {
                return &(header_vector[hidx]);
              }
          }

        return 0;
}

static const struct mach_header *_getExecHeader ()
{
	unsigned int hidx;
    	for (hidx = 0; hidx < header_count; hidx++)
    		if (header_vector[hidx].mhdr->filetype != MH_FVMLIB)
                  {
                    return header_vector[hidx].mhdr;
                  }
	return NULL;
}

/* private extern. */

const char *_nameForHeader(const struct mach_header *header)
{
    	const struct mach_header *execHeader;
	const struct fvmlib_command *libCmd, *endOfCmds;
	char **argv;
#if defined(__DYNAMIC__)
        extern char ***_NSGetArgv();
        argv = *_NSGetArgv();
#else
		extern char **NXArgv;
        argv = NXArgv;
#endif        
	if (header && header->filetype == MH_FVMLIB) {
		execHeader = _getExecHeader();
		for (libCmd = (const struct fvmlib_command *)(execHeader + 1),
		     endOfCmds = ((void *)libCmd) + execHeader->sizeofcmds;
		     libCmd < endOfCmds;
		     ((void *)libCmd) += libCmd->cmdsize) {
		     	if ((libCmd->cmd == LC_LOADFVMLIB)
			    && (libCmd->fvmlib.header_addr
			        == (unsigned long)header)) {
				return (char *)libCmd
				        + libCmd->fvmlib.name.offset;
			}
		}
		return NULL;
	} else
		return argv[0];
}

static void _resolveClassConflict(NXHashTable *clsHash,
                                  Class oldClass,
				  Class newClass)
{
	const struct header_info *oldHeader = _headerForClass(oldClass);
	const struct header_info *newHeader = _headerForClass(newClass);
	const char *oldName = _nameForHeader(oldHeader->mhdr);
	const char *newName = _nameForHeader(newHeader->mhdr);

	_objc_inform("Both %s and %s have implementations of class %s.",
	             oldName, newName, oldClass->name);				   
	if (newHeader->mhdr->filetype == MH_FVMLIB) {
		NXHashInsert(clsHash, oldClass);
		_objc_inform("Using implementation from %s.", oldName);
	} else
		_objc_inform("Using implementation from %s.", newName);
}

static NXHashTable *_objc_init_class_hash ()
{
    return NXCreateHashTableFromZone(classHashPrototype, 8, nil, _objc_create_zone());
}


static NXHashTable *_objc_get_classes_from_image (NXHashTable *clsHash,
                                                  struct header_info* hi)
{
    unsigned int i;
    Module mods;
    unsigned int midx;
    Class oldCls, newCls;

    for (mods = hi->mod_ptr, midx = 0;
         midx < hi->mod_count;
         midx++)
      {
        if (mods[midx].symtab == NULL)
            continue;

        for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++) {
            oldCls = NXHashInsert(clsHash,
                                  mods[midx].symtab->defs[i]);
            if (oldCls)
              {
                _resolveClassConflict(clsHash,
                                      oldCls,
                                      mods[midx].symtab->defs[i]);
		newCls = objc_lookUpClass (oldCls->name);
              }
            else
	      {
		newCls = mods[midx].symtab->defs[i];
	      }

	    if (newCls != oldCls)
	      {
		newCls->isa->version = mods[midx].version;
	      }

	    _objc_resolve_categories_for_class (newCls);
        }
      }

    return clsHash;
}


#ifdef LITERAL_STRING_OBJECTS

/* Initialize the isa pointers of all NXConstantString objects. */

#include <NXString.h>

static void _objc_fixup_string_objects_for_image (struct header_info* hi)
{
  NXConstantString *section;
  unsigned int size;

  section = getsectdatafromheaderinfo (hi,
                                       SEG_OBJC, "__string_object", &size);
  if (section != 0 && size != 0)
    {
      Class constantStringClass = objc_getClass ("NXConstantString");
      unsigned int i;
	
      for (i = 0; i < size / sizeof (NXConstantString); i++)
          *((Class *) &section[i]) = constantStringClass;
    }
}
#endif /* LITERAL_STRING_OBJECTS */

#ifdef OBJC_CLASS_REFS
static void _objc_map_class_refs_for_image (struct header_info* hi)
{
  Class *cls_refs;
  unsigned int size;

  cls_refs = (Class *) getsectdatafromheaderinfo (hi,
                                                  SEG_OBJC,
                                                  "__cls_refs",
                                                  &size);

  if (cls_refs)
    {
      unsigned int i;

      for (i = 0; i < size / sizeof (Class); i++) {
          const char *ref = (const char *) cls_refs[i];
          Class cls = objc_lookUpClass( ref );
             cls_refs[i] = cls;
      }
    }
}
#endif /* OBJC_CLASS_REFS */


static inline void map_selrefs (SEL *sels, unsigned int cnt)
{ 
  unsigned int i;
  
  /* Overwrite the string with a unique identifier. */
  for (i = 0; i < cnt; i++)
    {
      SEL sel = _sel_registerName ((const char *) sels[i]);
      
      if (sels[i] != sel)
	sels[i] = sel;
    }
}


static inline void map_methods (struct objc_method_list *methods)
{
  unsigned int i;
  
  for (i = 0; i < methods->method_count; i++)
    {
      Method method = &methods->method_list[i];
      SEL sel = _sel_registerName ((const char *) method->method_name);
      
      if (method->method_name != sel)
	method->method_name = sel;
    }		  
}


static inline void map_method_descs (struct objc_method_description_list *methods)
{
  unsigned int i;
  
  for (i = 0; i < methods->count; i++)
    {
      struct objc_method_description *method = &methods->list[i];
      SEL sel = _sel_registerName ((const char *) method->name);
      
      if (method->name != sel)
	method->name = sel;
    }		  
}

#import "Protocol.h"

struct objc_protocol
{
    @defs(Protocol)
};

@interface Protocol(RuntimePrivate)
+ _fixup: (Protocol *)protos numElements: (int) nentries;
@end

static void
_objc_fixup_protocol_objects_for_image (struct header_info* hi)
{
    unsigned int size;
    /* Fixup protocol objects. */
    struct objc_protocol * protos = (struct objc_protocol *)
        getsectdatafromheaderinfo (hi, SEG_OBJC, "__protocol", &size);

    if (protos)
      {
        unsigned int i;

        for (i = 0; i < size / sizeof (Protocol); i++) {

            /* can't use map_methods, the sizes differ. */
            if (protos[i].instance_methods)
                map_method_descs (protos[i].instance_methods);
            if (protos[i].class_methods)
                map_method_descs (protos[i].class_methods);
        }
        [Protocol _fixup: (Protocol *)protos numElements: size / sizeof (Protocol)];
      }
}



static const struct segment_command *
getobjcsegmentfromheader(const struct mach_header *mhp)
{
	struct segment_command *sgp;
	unsigned long i;

	sgp = (struct segment_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(sgp->cmd == LC_SEGMENT)
		if(strncmp(sgp->segname, "__OBJC", sizeof(sgp->segname)) == 0)
			return sgp;
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	return((struct segment_command *)0);
}


/* Comparison function for sorting headers.  Place all headers which have
   freeze-dried hashtables before any which don't.  Otherwise sort by
   decreasing size. */

static int compareHeaders (const void *x1, const void *x2)
{
  const struct header_info *header1 = x1, *header2 = x2;
  
  if (header1->frozenTable && !header2->frozenTable)
    return -1;
  else if (header2->frozenTable && !header1->frozenTable)
    return 1;
  
  return (header2->objcSize - header1->objcSize);
}


/* Build the header vector, sorting it as _objc_map_selectors() expects. */

struct header_info *_objc_headerVector (const struct mach_header * const *machhdrs)
{
  unsigned int hidx;
  struct header_info *hdrVec;
  
  if (header_vector)
    return header_vector;
  
  for (hidx = 0; machhdrs[hidx]; hidx++)
    header_count++;
  
  hdrVec = NXZoneMalloc (_objc_create_zone(),
			 header_count * sizeof (struct header_info));
  if (!hdrVec)
    _objc_fatal ("unable to allocate module vector");
  
  for (hidx = 0; hidx < header_count; hidx++)
    {
      int size;
      const struct segment_command *objcSeg;
      
      hdrVec[hidx].mhdr = machhdrs[hidx];
      hdrVec[hidx].image_slide = 0;
      hdrVec[hidx].mod_ptr = (Module) getsectdatafromheader (machhdrs[hidx],
					SEG_OBJC, SECT_OBJC_MODULES, &size);
      hdrVec[hidx].mod_count = size / sizeof (struct objc_module);
      hdrVec[hidx].frozenTable = getsectdatafromheader (machhdrs[hidx],
					SEG_OBJC, "__runtime_setup", &size);
      objcSeg = getobjcsegmentfromheader (machhdrs[hidx]);
      if (objcSeg)
	hdrVec[hidx].objcSize = objcSeg->filesize;
      else
	hdrVec[hidx].objcSize = 0;
    }
  
  /* Sort the headers for _objc_map_selectors(). */
  qsort (hdrVec, header_count, sizeof (struct header_info), &compareHeaders);
  
  return hdrVec;
}


unsigned int _objc_headerCount (void)
{
  return header_count;
}


void _objc_addHeader (const struct mach_header *header, long vmaddr_slide)
{
  header_count++;
  if (header_vector == 0)
      header_vector = NXZoneMalloc (_objc_create_zone(), 
                                    header_count * sizeof (struct header_info));
  else
    {
        void *old = (void*)header_vector;
        header_vector = NXZoneMalloc (_objc_create_zone(),
                                      header_count * sizeof (struct header_info));

        memcpy ((void*)header_vector, old, (header_count-1) * sizeof (struct header_info));
        NXZoneFree (_objc_create_zone(), old);
      //  header_vector = NXZoneRealloc (_objc_create_zone(), header_vector,
      //                                 header_count * sizeof (struct header_info));
    }

  header_vector[header_count - 1].mhdr = header;
  header_vector[header_count - 1].mod_ptr = NULL;
  header_vector[header_count - 1].mod_count = 0;
  header_vector[header_count - 1].frozenTable = NULL;
  header_vector[header_count - 1].image_slide = 0;
  header_vector[header_count - 1].objcSize = 0;
}


void _objc_removeHeader (const struct mach_header *header)
{
  unsigned int i, j;
  
  for (i = 0; i < header_count; i++)
    if (header_vector[i].mhdr == header)
      for (j = i; j < header_count - 1; j++)
        header_vector[j] = header_vector[j + 1];
  
  header_count--;
  header_vector = NXZoneRealloc (_objc_create_zone(), header_vector,
			 header_count * sizeof (struct header_info));
}


/* Register all of the selectors in each image, and fix them all up.
   We must register all images which have freeze-dried hashtables before any
   which don't, since the freeze-dried hashtables are searched before the
   dynamic hashtable.  We also want to register the freeze-dried hashtables
   in decreasing order of size to minimize selector lookup time.  We don't
   need to fixup the selectors in the first image we register, since those
   strings will always be used.  If the first image has a freeze-dried
   hashtable, then we don't need to run over its selectors at all, since
   we can register them all at once. */

static void _objc_map_install_selectors (struct header_info* hi)
{
   Module mods = hi->mod_ptr;
   unsigned int midx;
   unsigned int size;

   /* Register selectors from freeze-dried hashtables. */
   if (hi->frozenTable)
     {
       const char *strings = (const char *)
           getsectdatafromheaderinfo (hi,
                                      SEG_OBJC, "__meth_var_names", &size);

       if (!strings)
         {
           /* Try old-style strings. */
           strings = (const char *)
               getsectdatafromheaderinfo (hi,
                                          SEG_OBJC,
                                          "__selector_strs", &size);
         }

       _sel_init (hi->mhdr,
                  strings,
                  size,
                  hi->frozenTable);
     }
}

static void _objc_map_selectors_from_image (struct header_info* hi)
{
    Module mods = hi->mod_ptr;
    unsigned int midx;
    unsigned int size;
    SEL *messages_refs;
    struct proto_template { @defs (Protocol) } *protos;

      /* Map message refs. */
      messages_refs = (SEL *) getsectdatafromheaderinfo (hi,
                                                         SEG_OBJC,
                                                         "__message_refs",
                                                         &size);
      if (messages_refs)
	map_selrefs (messages_refs, size / sizeof (SEL));
      
      for (midx = 0; midx < hi->mod_count; midx++)
	{
	  unsigned int i;
          struct objc_method_list* mlist;

	  if (mods[midx].symtab == NULL)
	    continue;
	  
	  /* Map class defs. */
	  for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++)
	    {
	      Class cls = mods[midx].symtab->defs[i];
	      
	      if (cls->methods)
                {
                  mlist = cls->methods;
                  while (mlist->method_next) mlist = mlist->method_next;
                  map_methods (mlist);
                }

	      if (cls->isa->methods)
                {
                  mlist = cls->isa->methods;
                  while (mlist->method_next) mlist = mlist->method_next;
                  map_methods (mlist);
                }
	    }
	  
	  /* Map category defs. */
	  for (i = mods[midx].symtab->cls_def_cnt;  
	       i < (mods[midx].symtab->cls_def_cnt + 
		    mods[midx].symtab->cat_def_cnt);
	       i++)
	    {
	      Category cat = mods[midx].symtab->defs[i];
	      
	      if (cat->instance_methods)
		map_methods (cat->instance_methods);
	      if (cat->class_methods)
		map_methods (cat->class_methods);
	    }
      }
 
      /* Fixup protocols. */
      protos = (struct proto_template *) getsectdatafromheaderinfo ( hi,
                                                                     SEG_OBJC,
                                                                     "__protocol",
                                                                     &size);
      if (protos)
	{
	  unsigned int i;
	  
	  for (i = 0; i < size / sizeof (Protocol); i++)
	    {
	      if (protos[i].instance_methods)
	        map_method_descs (protos[i].instance_methods);
	      if (protos[i].class_methods)
	        map_method_descs (protos[i].class_methods);
	    }
        }
}

#if 0
/*
 *	Purpose: The idea here is to call objc_msgSend() with the 
 *		 arguments in args. The args must already be in the 
 *		 appropriate format for the stack (perhaps copied 
 *		 form some other stack frame).
 *
 *  		 The final call to objc_msgSend () is thus equivalent to
 *		 objc_msgSend (self, op, arg[0], arg[1], ..., arg[size - 1]).
 *
 *	Assumptions: Do not compile this with -fomit-frame-pointer!
 */
id objc_msgSendv (id self, SEL sel, unsigned size, marg_list args) 
{
	unsigned int i;

	size -= (sizeof(id) + sizeof(SEL));
	args += (sizeof(id) + sizeof(SEL));

#ifdef m68k
	/* Push the args onto the stack. */
	for (i = (size + 1) / sizeof (short); i > 0; i--)
		asm ("movew %0,sp@-" : : "a" (((short *) args)[i-1]));
#else
	panic ("objc_msgSendv");
#endif
	return objc_msgSend (self, sel);
}
#endif

static void _objc_call_loads_for_image (struct header_info* header)
{
  extern IMP class_lookupMethodInMethodList(struct objc_method_list *mlist,
                                            SEL sel);
  Class class, *pClass;
  Category *pCategory;
  IMP load_method;
  struct objc_method_list *methods;
  unsigned int nModules, nClasses, nCategories;
  struct objc_module *module;
  struct objc_symtab *symtab;

  for (nModules = header->mod_count, module = header->mod_ptr;
       nModules;
       nModules--, module++) {

      symtab = module->symtab;
      if (symtab == NULL)
          continue;
      for (nClasses = symtab->cls_def_cnt, pClass = (Class *)symtab->defs;
           nClasses;
           nClasses--, pClass++)
        {
          class = *pClass;

          if (methods = class->isa->methods)
            {
              while (methods->method_next)
                  methods = methods->method_next;

              load_method = class_lookupMethodInMethodList(methods,
                                                           @selector(load));
              if (load_method)
                  (*load_method)((id)class, @selector(load));
            }
        }

      for (nCategories = symtab->cat_def_cnt,
           pCategory = (Category *)&symtab->defs[symtab->cls_def_cnt];
           nCategories;
           nCategories--, pCategory++)
        {
          class = objc_getClass ((*pCategory)->class_name);

          if (methods = (*pCategory)->class_methods)
            {
              load_method = class_lookupMethodInMethodList(methods,
                                                           @selector(load));
              if (load_method)
                  (*load_method)((id)class, @selector(load));
            }
        }
      }
}


void objc_setMultithreaded (BOOL flag)
{
  if (flag == YES)
    _objc_multithread_mask = 0;
  else
    _objc_multithread_mask = -1;
}


static void
_objc_map_image_callback (struct header_info *h);
 
void _objcInit (void)
{
    int hidx;
    class_hash = _objc_init_class_hash();

      {
          extern struct mach_header **getmachheaders (void);
          struct mach_header **machheaders;

          if ((machheaders = getmachheaders())) {
              int i = -1;
              header_vector = _objc_headerVector( machheaders );
              while ( ++i < header_count )
                 _objc_map_install_selectors (&header_vector[i]);

              i = -1;
              while ( ++i < header_count )
                 _objc_map_image_callback (&header_vector[i]);

              free (machheaders);
           }

          for (hidx = 0; hidx < header_count; hidx++)
            {
              int nModules, i;
              struct objc_module* module;

              for (nModules = header_vector[hidx].mod_count, module = header_vector[hidx]. mod_ptr;
                   nModules;
                   nModules--, module++)
                {
                  for (i = 0; i < module->symtab->cls_def_cnt; i++)
                    {
                      Class cls = (Class)module->symtab->defs[i];
                      _class_install_relationships (cls, module->version);
                    }
                }

#ifdef LITERAL_STRING_OBJECTS
              _objc_fixup_string_objects_for_image (&header_vector[hidx]);
#endif /* LITERAL_STRING_OBJECTS */
#ifdef OBJC_CLASS_REFS
              _objc_map_class_refs_for_image (&header_vector[hidx]);
#endif /* OBJC_CLASS_REFS */
            }

          for (hidx = 0; hidx < header_count; hidx++)
              _objc_fixup_protocol_objects_for_image (&header_vector[hidx]);

          for (hidx = 0; hidx < header_count; hidx++)
              _objc_call_loads_for_image (&header_vector[hidx]);
      }
}

static void
_objc_map_image_callback (struct header_info *h )
{
    class_hash = _objc_get_classes_from_image (class_hash, h);

    _objc_map_selectors_from_image (h);
    _objc_add_categories_from_image (h);
}


