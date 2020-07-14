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
 *	objc-load.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"
#import "objc-load.h"	
#import "objc-runtime.h"
#import "hashtable.h"
#import "Object.h"
#import "Protocol.h"
#include <NXString.h>
#include <mach-o/rld.h>

#ifdef RUNTIME_DYLD
#include <mach-o/dyld.h>
#endif

#ifdef KERNEL
extern char *getsectdatafromheader(
        const struct mach_header *mhp,
        const char *segname,
        const char *sectname,
        int *size);
#endif

@interface Protocol(Private)
+ _fixup: (void *)protos numElements: (int) nentries;
@end

/* Private extern */
extern struct mach_header *rld_get_current_header(void);
extern void (*callbackFunction)( Class, Category );

static inline void map_selrefs (SEL *sels, unsigned int cnt)
{ 
  unsigned int i;
  
  /* Overwrite the string with a unique identifier. */
  for (i = 0; i < cnt; i++)
    {
      SEL sel = sel_registerName ((const char *) sels[i]);
      
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
      SEL sel = sel_registerName ((const char *) method->method_name);
      
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
      SEL sel = sel_registerName ((const char *) method->name);
      
      if (method->name != sel)
	method->name = sel;
    }		  
}


static struct objc_method_list *get_base_method_list(Class cls)
{
	struct objc_method_list *mlist, *prev = 0;

	for (mlist = cls->methods; mlist; mlist = mlist->method_next)
		prev = mlist;

	return prev;
}

static void send_load_message_to_class(Class cls, void *header_addr)
{
	struct objc_method_list *mlist = get_base_method_list(cls->isa);
	IMP load_method;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(finishLoading:));

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, @selector(finishLoading:), 
				header_addr);
	}
}

static void send_load_message_to_category(Category cat, void *header_addr)
{
	struct objc_method_list *mlist = cat->class_methods;
	IMP load_method;
	Class cls;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(finishLoading:));

		cls = objc_getClass (cat->class_name);

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)(cls, @selector(finishLoading:), 
				header_addr);
	}
}

static void send_unload_message_to_class(Class cls)
{
	struct objc_method_list *mlist = get_base_method_list(cls->isa);
	IMP load_method;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(startUnloading));

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, 
				@selector(startUnloading));
	}
}

static void send_unload_message_to_category(Category cat)
{
	struct objc_method_list *mlist = cat->class_methods;
	IMP load_method;
	Class cls;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(startUnloading));

		cls = objc_getClass (cat->class_name);

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, 
				@selector(startUnloading));
	}
}

static void _objc_fixup_string_objects_from_header(struct mach_header *header)
{
    NXConstantString *section;
    unsigned int size;
    
    section = (NXConstantString *) getsectdatafromheader (header,
		SEG_OBJC, "__string_object", &size);
    if (section != 0 && size != 0)
    {
	Class constantStringClass = objc_getClass ("NXConstantString");
	unsigned int i;
	
	for (i = 0; i < size / sizeof (NXConstantString); i++)
	*((Class *) &section[i]) = constantStringClass;
    }
}


int objc_registerModule (struct mach_header *header_addr,
		    void (*class_callback) (Class, Category))
{
  Module mod, modhdr;
  int size, saveSize, i;
  SEL *sels;
  // this isn't very object oriented!...bypass ivar protection.
  struct proto_template { @defs(Protocol) } *protos;
  Class *cls_refs;
  int errflag = 0;

#ifdef RUNTIME_DYLD
  if (_dyld_present ())
    {
      fprintf (stderr, "objc_registerModule not supported in dynamic programs\n");
      return 1;
    }
#endif

  modhdr = (Module) getsectdatafromheader (header_addr,
		    SEG_OBJC, "__module_info", &saveSize);

  /* Check for duplicate classes.
      This should never happen since rld should detect multiple
      definitions of the .objc_class_name_XXX symbols. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  Class cls = mod->symtab->defs[i];

	  if ((cls = objc_lookUpClass (mod->symtab->defs[i])))
	    {
#if 0
	      NXPrintf (errStream, "objc_loadModules(): "
			"Duplicate definition for class `%s'\n",
			[cls name]);
#endif
	      errflag = 1;
	    }
	}
    }

  if (errflag)
    return 1;

  _objc_addHeader (header_addr, 0);

  /* Fixup string objects. */
  _objc_fixup_string_objects_from_header (header_addr);

  /* Map selectors. */
  sels = (SEL *) getsectdatafromheader (header_addr,
		    SEG_OBJC, "__message_refs", &size);
  if (sels)
    map_selrefs (sels, size / sizeof (SEL));

  /* Fixup protocol objects. */
  protos = (struct proto_template *) getsectdatafromheader (
		header_addr, SEG_OBJC, "__protocol", &size);
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
      [Protocol _fixup: protos numElements: size / sizeof (Protocol)];
    }

  /* Insert classes into class hashtable */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  objc_addClass (mod->symtab->defs[i]);
	}
    }

  /* Process class definitions. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  Class cls = mod->symtab->defs[i];

	  if (cls->methods)
	    map_methods (cls->methods);
	  if (cls->isa->methods)
	    map_methods (cls->isa->methods);

	  _class_install_relationships (cls, mod->version);

	  /* Versions 3 and 4 will never ship; internal compatibility only.
	      Compensate for "next" field in struct objc_protocol_list. */
	  if (cls->isa->version >= 3 && cls->isa->version <= 4
	      && cls->protocols)
	    {
	      (char *) cls->protocols -= 4;
	      (char *) cls->isa->protocols -= 4;
	    }

	  /* for internal compatibility...should remove! */
	  if ((cls->isa->version == 3) && cls->protocols)
	    {
	      _objc_inform ("Unable to install protocols by name...\n");
	      _objc_inform ("Class %s must be recompiled.\n",
			    cls->name);
	      cls->protocols = 0;
	      cls->isa->protocols = 0;
	    }
	}
    }

  /* We must process all classes before any categories, or else
      we might map the selectors for a category method twice. */

  /* Process category definitions. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt;
	    i++)
	{
	  Category cat = mod->symtab->defs[i];

	  if (cat->instance_methods)
	    map_methods (cat->instance_methods);
	  if (cat->class_methods)
	    map_methods (cat->class_methods);

	  _objc_add_category (mod->symtab->defs[i], mod->version);
	}
    }

  /* Handle v1.0 mapping of selector references */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      if (mod->version == 1)
	map_selrefs (mod->symtab->refs, mod->symtab->sel_ref_cnt);
    }

#ifdef OBJC_CLASS_REFS
  /* Fixup class references. */
  cls_refs = (Class *) getsectdatafromheader (
		header_addr, SEG_OBJC, "__cls_refs", &size);
  if (cls_refs)
    {
      unsigned int i;
      
      for (i = 0; i < size / sizeof (Class); i++)
	cls_refs[i] = objc_getClass ((const char *) cls_refs[i]);
    }
#endif /* OBJC_CLASS_REFS */

  /* Do we guarantee any ordering of load messages?
      We might send a load message to a subclass before its superclass.

      Also, a load method might send a message to a class which hasn't
      received its load message yet by using objc_getClass ().

      To solve these problems properly, we probably need to add a
      "loaded" bit to the class structure in addition to the
      "initialized" bit. */

  /* Send load messages and activate call backs to classes. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  if (class_callback)
	    (*class_callback) (mod->symtab->defs[i], 0);

	  send_load_message_to_class (mod->symtab->defs[i],
				      header_addr);
	}
    }

  /* Send load messages and activate call backs to categories. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt;
	    i++)
	{
	  if (class_callback)
	    (*class_callback) (objc_getClass(((Category)mod->symtab->defs[i])->class_name),
		    (Category)mod->symtab->defs[i]);

	  send_load_message_to_category((Category)mod->symtab->defs[i],
		    header_addr);
	}
    }

  return 0;
}


#ifndef KERNEL
static
#endif
void objc_unregisterModule (struct mach_header *header_addr,
			void (*unload_callback) (Class, Category))
{
  Module mod, modhdr;
  int size, i, saveSize, strsize;
  const char *strs;

#ifdef RUNTIME_DYLD
  if (_dyld_present ())
    {
      fprintf (stderr, "objc_unregisterModule not supported in dynamic programs\n");
      return;
    }
#endif


  modhdr = (Module) getsectdatafromheader (header_addr,
		    SEG_OBJC, "__module_info", &saveSize);

  /* Send unload messages and deactivate call backs to categories. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt;
	    i++)
	{
	  Category cat = mod->symtab->defs[i];

	  if (unload_callback)
	    (*unload_callback) (objc_getClass (cat->class_name), cat);

	  send_unload_message_to_category (cat);
	}
    }

  /* Send unload messages and deactivate call backs to classes. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  Class cls = mod->symtab->defs[i];

	  if (unload_callback)
	    (*unload_callback) (cls, 0);

	  send_unload_message_to_class (cls);
	}
    }

  /* Remove categories. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt;
	    i++)
	{
	  Category cat = mod->symtab->defs[i];

	  _objc_remove_category (cat, mod->version);
	}
    }

  /* Remove classes. */
  for (mod = modhdr, size = saveSize;
	mod && size;
	size -= mod->size, (char *) mod += mod->size)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  Class cls = mod->symtab->defs[i];

	  _objc_removeClass (cls);
	}
    }

  /* Do we really want to do this?
      What if I load Foo.o which contains a new selector "foo",
      then I unload Foo.o and load Bar.o which also contains the
      selector "foo".  The value of the selector may change!
      I think that selector mappings need to be permanent. */

  /* Unload any selectors that might have been unique to this module. */
  strs = (const char *) getsectdatafromheader (header_addr,
			    SEG_OBJC, "__meth_var_names", &strsize);

  if (strs)
    _sel_unloadSelectors (strs, strs + strsize);

  /* Unload old-style strings too. */
  strs = (const char *) getsectdatafromheader (header_addr,
			    SEG_OBJC, "__selector_strs", &strsize);

  if (strs)
    _sel_unloadSelectors (strs, strs + strsize);

  _objc_removeHeader (header_addr);
}


#ifndef KERNEL

/* Lock for dynamic loading and unloading. */

static OBJC_DECLARE_LOCK (loadLock);

/* Loading isn't really thread safe.  If a load message recursively calls
   objc_loadModules() both sets will be loaded correctly, but if the original
   caller calls objc_unloadModules() it will probably unload the wrong modules.
   If a load message calls objc_unloadModules(), then it will unload
   the modules currently being loaded, which will probably cause a crash. */

/* Error handling is still somewhat crude.  If we encounter errors while
   linking up classes or categories, we will not recover correctly.
   Hopefully rld_load () will detect these problems for us. */

/* I removed atempts to lock the class hashtable, since this introduced
   deadlock which was hard to remove.  The only way you can get into trouble
   is if one thread loads a module while another thread tries to access the
   loaded classes (using objc_lookUpClass) before the load is complete. */

long objc_loadModules (
	char *modlist[], 
	NXStream *errStream,
	void (*class_callback) (Class, Category),
	struct mach_header **hdr_addr,
	char *debug_file)
{
  struct mach_header *header_addr;
  int errflag = 0;

#ifdef RUNTIME_DYLD
  if (_dyld_present ())
    {
      NSObjectFileImage objectFileImage;
      NSModule module;

      char **modules;

      callbackFunction = class_callback;
      for (modules = &modlist[0]; *modules != 0; modules++)
        {
          NSObjectFileImageReturnCode code
              = NSCreateObjectFileImageFromFile (*modules, &objectFileImage);

          if (code != NSObjectFileImageSuccess)
            {
              if (errStream)
                  NXPrintf (errStream,
                            "objc_loadModules(%s) NSObjectFileImageReturnCode = %d\n", *modules, code);

              errflag = 1;
              continue;
            }

          module = NSLinkModule(objectFileImage, *modules, 0);
       }
      callbackFunction = 0;
    }
  else
#endif

    {

        OBJC_LOCK (&loadLock);

        if (rld_load (errStream, &header_addr, modlist, debug_file))
          {
            errflag = objc_registerModule (header_addr, class_callback);

            /* Unload everything on errors. */
            if (errflag)
                rld_unload (errStream);
            else
              {
                  /* Return optional output parameter */
                  if (hdr_addr)
                      *hdr_addr = header_addr;
              }
          }
        else
            errflag = 1;

        OBJC_UNLOCK (&loadLock);
    }

  return errflag;
}


/* Unloading isn't really thread safe.  If an unload message calls
   objc_loadModules() or objc_unloadModules(), then the current call
   to objc_unloadModules() will probably unload the wrong stuff. */

long objc_unloadModules (
	NXStream *errStream,
	void (*unload_callback) (Class, Category))
{
  struct mach_header *header_addr = rld_get_current_header ();
  int errflag = 0;

  if (header_addr)
    {
      objc_unregisterModule (header_addr, unload_callback);

      OBJC_LOCK (&loadLock);

      if (rld_unload (errStream) == 0)
        errflag = 1;

      OBJC_UNLOCK (&loadLock);
    }
  else
    {
      NXPrintf (errStream, "objc_unloadModules(): No modules to unload\n");
      errflag = 1;
    }

  return errflag;
}

#endif /* not KERNEL */
