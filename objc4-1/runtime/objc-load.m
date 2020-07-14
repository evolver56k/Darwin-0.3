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
/*
 *	objc-load.m
 *	Copyright 1988-1996, NeXT Software, Inc.
 *	Author:	s. naroff
 *
 */

#import "objc-private.h"
#import "objc-load.h"	
#import <objc/objc-runtime.h>
#import <objc/hashtable2.h>
#import <objc/Object.h>
#import <objc/Protocol.h>
#if defined(__MACH__) || defined(WIN32)	
#import <streams/streams.h>
#endif 

#if !defined(NeXT_PDO)
    // MACH
    #include <mach-o/rld.h>
    #include <mach-o/dyld.h>
#endif 

#if defined(WIN32)
    #import <winnt-pdo.h>
    #import <windows.h>
#endif

#if defined(__svr4__)
    #import <dlfcn.h>
#endif

extern char *	getsectdatafromheader	(const headerType * mhp, const char * segname, const char * sectname,  int * size);

@interface Protocol(Private)
+ _fixup: (OBJC_PROTOCOL_PTR)protos numElements: (int) nentries;
@end

/* Private extern */
OBJC_EXPORT void (*callbackFunction)( Class, Category );

#if !defined(NeXT_PDO)
    // MACH
    extern headerType *rld_get_current_header(void);
#endif

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

struct objc_method_list *get_base_method_list(Class cls) {
    struct objc_method_list **ptr = cls->methodLists;
    if (!*ptr) return NULL;
    while ( *ptr != 0 && *ptr != END_OF_METHODS_LIST ) { ptr++; }
    return(*(ptr-1));
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
		if (!cls)
		    _objc_inform("class `%s' not linked into application", cat->class_name);

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, 
				@selector(startUnloading));
	}
}

static void _objc_fixup_string_objects_from_header(headerType *header)
{
    OBJC_CONSTANT_STRING_PTR section;
    unsigned int size;
    
    section = _getObjcStringObjects(header,  &size);
    if (section != 0 && size != 0)
    {
	Class constantStringClass = objc_getClass ("NXConstantString");
	unsigned int i;

	for (i = 0; i < size; i++)
	    *((Class *) OBJC_CONSTANT_STRING_DEREF section[i]) = constantStringClass;
    }
}

#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
    void send_load_message_to_class(Class cls, void *header_addr)
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

    void send_load_message_to_category(Category cat, void *header_addr)
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
#endif // GENERIC_OBJ_FILE

#if !defined(KERNEL)
static
#endif
int objc_registerModule (headerType *header_addr, void (*class_callback) (Class, Category))
{
  Module mod, modhdr;
  int midx, size, mcount, i;
  OBJC_PROTOCOL_PTR protos;
  Class *cls_refs;
  int errflag = 0;

  modhdr = _getObjcModules(header_addr, &mcount);

  /* Check for duplicate classes.
      This should never happen since rld should detect multiple
      definitions of the .objc_class_name_XXX symbols. */
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++) {
	  Class cls = mod->symtab->defs[i];

	  if ( (cls = objc_lookUpClass (mod->symtab->defs[i])) ) {
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

#if defined(__MACH__)
  /* Map selectors. */
  {
    SEL *sels = _getObjcMessageRefs(header_addr, &size);

    if (sels) {
	map_selrefs (sels, size); 
    }
  }
#endif

  /* Fixup protocol objects. */
  protos = _getObjcProtocols((headerType*)header_addr, &size);
  if (protos) {
      unsigned int i;
      
      for (i = 0; i < size; i++) {

	  /* can't use map_methods, the sizes differ. */
	  if (protos[i] OBJC_PROTOCOL_DEREF instance_methods)
	    map_method_descs (protos[i] OBJC_PROTOCOL_DEREF instance_methods);
	  if (protos[i] OBJC_PROTOCOL_DEREF class_methods)
	    map_method_descs (protos[i] OBJC_PROTOCOL_DEREF class_methods);
	}
      [Protocol _fixup:(OBJC_PROTOCOL_PTR)protos numElements: size];
    }

  /* Insert classes into class hashtable */
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++) {
	  objc_addClass ((Class)mod->symtab->defs[i]);
	}
    }

  /* Process class definitions. */
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++) {
	  Class cls = mod->symtab->defs[i];

	  if (cls->methodLists) 
	    map_methods (cls->methodLists[0]);
	  if (cls->isa->methodLists)
	    map_methods (cls->isa->methodLists[0]);

	  _class_install_relationships (cls, mod->version);

	  /* Versions 3 and 4 will never ship; internal compatibility only.
	      Compensate for "next" field in struct objc_protocol_list. */
	  if (cls->isa->version >= 3 && cls->isa->version <= 4 && cls->protocols) {
	      (char *) cls->protocols -= 4;
	      (char *) cls->isa->protocols -= 4;
	    }

	  /* for internal compatibility...should remove! */
	  if ((cls->isa->version == 3) && cls->protocols) {
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
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt; i++) {
	  Category cat = mod->symtab->defs[i];

	  if (cat->instance_methods)
	    map_methods (cat->instance_methods);
	  if (cat->class_methods)
	    map_methods (cat->class_methods);

	  _objc_add_category (mod->symtab->defs[i], mod->version);
	}
    }

  /* Handle v1.0 mapping of selector references */
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      if (mod->version == 1)
	map_selrefs (mod->symtab->refs, mod->symtab->sel_ref_cnt);
    }

#ifdef OBJC_CLASS_REFS
  /* Fixup class references. */
  cls_refs = _getObjcClassRefs (header_addr,&size);
  if (cls_refs) {
      unsigned int i;
      
      for (i = 0; i < size; i++)
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
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++) {
	  if (class_callback)
	    (*class_callback) (mod->symtab->defs[i], 0);

#if defined(NeXT_PDO)
	  send_load_message_to_class (mod->symtab->defs[i], header_addr);
#endif
	}
    }

  /* Send load messages and activate call backs to categories. */
  for (mod = modhdr, midx = 0; mod && (midx < mcount); ++midx, ++mod) {
      for (i = mod->symtab->cls_def_cnt;
	    i < mod->symtab->cls_def_cnt + mod->symtab->cat_def_cnt; i++) {
	  if (class_callback)
	    (*class_callback) (objc_getClass(((Category)mod->symtab->defs[i])->class_name),
		    (Category)mod->symtab->defs[i]);

#if defined(NeXT_PDO)
	  send_load_message_to_category((Category)mod->symtab->defs[i], header_addr);
#endif
	}
    }

  return 0;
}

#if !defined(KERNEL)
static
#endif
void objc_unregisterModule (headerType *header_addr,
                            void (*unload_callback) (Class, Category))
{
  Module mod, modhdr;
  int midx, i, size;

  modhdr = _getObjcModules(header_addr, &size);

  /* Send unload messages and deactivate call backs to categories. */
  for (mod = modhdr, midx = 0;
	mod && (midx < size);
	++midx, ++mod)
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
  for (mod = modhdr, midx = 0;
	mod && (midx < size);
	++midx, ++mod)
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
  for (mod = modhdr, midx = 0;
	mod && (midx < size);
	++midx, ++mod)
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
  for (mod = modhdr, midx = 0;
	mod && (midx < size);
	++midx, ++mod)
    {
      for (i = 0; i < mod->symtab->cls_def_cnt; i++)
	{
	  Class cls = mod->symtab->defs[i];

	  _objc_removeClass (cls);
	}
    }

#if defined(__MACH__)
    {
    int strsize;
    const char *strs;
  /* Do we really want to do this?
      What if I load Foo.o which contains a new selector "foo",
      then I unload Foo.o and load Bar.o which also contains the
      selector "foo".  The value of the selector may change!
      I think that selector mappings need to be permanent. */

  /* Unload any selectors that might have been unique to this module. */
  strs = _getObjcStrings(header_addr, &size);

  if (strs)
    _sel_unloadSelectors (strs, strs + strsize);

  /* Unload old-style strings too. */
  strs = (const char *) getsectdatafromheader (header_addr,
			    SEG_OBJC, "__selector_strs", &strsize);

  if (strs)
    _sel_unloadSelectors (strs, strs + strsize);
    }
#endif 

  _objc_removeHeader (header_addr);

}

/**********************************************************************************
 * objc_loadModules.
 **********************************************************************************/
#if !defined(KERNEL)
    /* Lock for dynamic loading and unloading. */
	static OBJC_DECLARE_LOCK (loadLock);
#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
	void		(*load_class_callback) (Class, Category);
#endif 


long	objc_loadModules   (char *			modlist[], 
							void *			errStream,
							void			(*class_callback) (Class, Category),
							headerType **	hdr_addr,
							char *			debug_file)
{
	char **				modules;
	int					code;
	int					itWorked;

	if (modlist == 0)
		return 0;

	for (modules = &modlist[0]; *modules != 0; modules++)
	{
		itWorked = objc_loadModule (*modules, class_callback, &code);
		if (itWorked == 0)
		{
#if defined(__MACH__) || defined(WIN32)	
			if (errStream)
				NXPrintf ((NXStream *) errStream, "objc_loadModules(%s) code = %d\n", *modules, code);
#endif
			return 1;
		}

		if (hdr_addr)
			*(hdr_addr++) = 0;
	}

	return 0;
}

/**********************************************************************************
 * objc_loadModule.
 *
 * NOTE: Loading isn't really thread safe.  If a load message recursively calls
 * objc_loadModules() both sets will be loaded correctly, but if the original
 * caller calls objc_unloadModules() it will probably unload the wrong modules.
 * If a load message calls objc_unloadModules(), then it will unload
 * the modules currently being loaded, which will probably cause a crash.
 *
 * Error handling is still somewhat crude.  If we encounter errors while
 * linking up classes or categories, we will not recover correctly.
 * Hopefully rld_load () will detect these problems for us.
 *
 * I removed attempts to lock the class hashtable, since this introduced
 * deadlock which was hard to remove.  The only way you can get into trouble
 * is if one thread loads a module while another thread tries to access the
 * loaded classes (using objc_lookUpClass) before the load is complete.
 **********************************************************************************/
int		objc_loadModule	   (char *			moduleName, 
							void			(*class_callback) (Class, Category),
							int *			errorCode)
{
	int								successFlag = 1;
	int								locErrorCode;
#if defined(__MACH__)	
	NSObjectFileImage				objectFileImage;
	NSObjectFileImageReturnCode		code;
#endif
#if defined(WIN32) || defined(__svr4__)
	void *		handle;
	void		(*save_class_callback) (Class, Category) = load_class_callback;
#endif

	// So we don't have to check this everywhere
	if (errorCode == NULL)
		errorCode = &locErrorCode;

#if defined(__MACH__)
	if (moduleName == NULL)
	{
		*errorCode = NSObjectFileImageInappropriateFile;
		return 0;
	}

	if (_dyld_present () == 0)
	{
		*errorCode = NSObjectFileImageFailure;
		return 0;
	}

	callbackFunction = class_callback;
	code = NSCreateObjectFileImageFromFile (moduleName, &objectFileImage);
	if (code != NSObjectFileImageSuccess)
	{
		*errorCode = code;
 		return 0;
	}

	(void) NSLinkModule(objectFileImage, moduleName, 0);
	callbackFunction = NULL;

#else
	// The PDO cases
	if (moduleName == NULL)
	{
		*errorCode = 0;
		return 0;
	}

	OBJC_LOCK(&loadLock);

#if defined(WIN32) || defined(__svr4__)

	load_class_callback = class_callback;

#if defined(WIN32)
	if ((handle = LoadLibrary (moduleName)) == NULL)
	{
		FreeLibrary(moduleName);
		*errorCode = 0;
		successFlag = 0;
	}

#else
	handle = dlopen(moduleName, (RTLD_NOW | RTLD_GLOBAL));
	if (handle == 0)
	{
		*errorCode = 0;
		successFlag = 0;
	}
	else
	{
		objc_register_header(moduleName);
		objc_finish_header();
	}
#endif

	load_class_callback = save_class_callback;

#elif defined(NeXT_PDO) 
	// NOTHING YET...
	successFlag = 0;
#endif // WIN32

	OBJC_UNLOCK (&loadLock);

#endif // MACH

	return successFlag;
}

/**********************************************************************************
 * objc_unloadModules.
 *
 * NOTE:  Unloading isn't really thread safe.  If an unload message calls
 * objc_loadModules() or objc_unloadModules(), then the current call
 * to objc_unloadModules() will probably unload the wrong stuff.
 **********************************************************************************/

long	objc_unloadModules (void *			errStream,
							void			(*unload_callback) (Class, Category))
{
#if defined(NeXT_PDO)
	headerType *	header_addr = 0;
#else // MACH
	headerType *	header_addr = rld_get_current_header ();
#endif 
	int errflag = 0;

	if (header_addr)
	{
		objc_unregisterModule (header_addr, unload_callback);

		OBJC_LOCK (&loadLock);

#if !defined(NeXT_PDO)
		if (rld_unload (errStream) == 0)
			errflag = 1;
#endif 

		OBJC_UNLOCK (&loadLock);
	}
	else
	{
		errflag = 1;
	}

  return errflag;
}
#endif /* not KERNEL */
