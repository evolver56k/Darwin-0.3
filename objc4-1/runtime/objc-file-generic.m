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
 *      Objective-C runtime information module.
 *      This module is generic for all object format files.
 */

#import <pdo.h>
#import <Protocol.h>
#import "objc-private.h"
#import "objc-file.h"
#import "objc-load.h"
#if defined(WIN32)
    #import <stdlib.h>
#endif

int		NXArgc = 0;
char	**	NXArgv = NULL;


char ***_NSGetArgv(void)
{
	return &NXArgv;
}

int *_NSGetArgc(void)
{
	return &NXArgc;

}

#if defined(WIN32)
    OBJC_EXPORT char ***_environ_dll;
#elif defined(NeXT_PDO)
    OBJC_EXPORT char ***environ;
#endif

char ***_NSGetEnviron(void)
{
#if defined(WIN32)
	return (char ***)_environ_dll;
#elif defined(NeXT_PDO)
	return (char ***)&environ;
#else
        #warning "_NSGetEnviron() is unimplemented for this architecture"
	return (char ***)NULL;
#endif
}


#if !defined(hpux) && !defined(__osf__) 
    const char OBJC_METH_VAR_NAME_FORWARD[10]="forward::";
#else
    OBJC_EXPORT char OBJC_METH_VAR_NAME_FORWARD[];
#endif

static objcSectionStruct objcHeaders = {0,0,sizeof(objcModHeader)};
objcModHeader *CMH = 0;  // Current Module Header

int _objcModuleCount() {
   return objcHeaders.count;
}

const char *_objcModuleNameAtIndex(int i) {
   if ( i < 0 || i >= objcHeaders.count)
      return NULL;
   return ((objcModHeader*)objcHeaders.data + i)->name;
}

static inline void allocElements (objcSectionStruct *ptr, int nelmts)
{
    if (ptr->data == 0) {
        ptr->data = (void*)malloc ((ptr->count+nelmts) * ptr->size);
    } else {
        volatile void *tempData = (void *)realloc(ptr->data, (ptr->count+nelmts) * ptr->size);
        ptr->data = (void **)tempData;
    }

    bzero((char*)ptr->data + ptr->count * ptr->size, ptr->size * nelmts);
}

OBJC_EXPORT void _objcInit(void);
void objc_finish_header (void)
{
     _objcInit ();
     CMH = (objcModHeader *)0;
     // leaking like a stuck pig.
}

void objc_register_header_name (const char * name) {
    if (name) {
        CMH->name = malloc(strlen(name)+1);
#if defined(WIN32) || defined(__svr4__)
		bzero(CMH->name, (strlen(name)+1));
#endif 
        strcpy(CMH->name, name);
    }
}

void objc_register_header (const char * name)
{
    if (CMH) {
      	// we've already registered a header (probably via __objc_execClass), 
	// so just update the name.
       if (CMH->name)
         free(CMH->name);
    } else {
        allocElements (&objcHeaders, 1);
        CMH = (objcModHeader *)objcHeaders.data + objcHeaders.count;
        objcHeaders.count++;
        bzero(CMH, sizeof(objcModHeader));

        CMH->Modules.size       = sizeof(struct objc_module);
        CMH->Classes.size       = sizeof(void *);
        CMH->Protocols.size     = sizeof(void *);
        CMH->StringObjects.size = sizeof(void *);
    }
    objc_register_header_name(name);
}

#if defined(DEBUG)
void printModule(Module mod)
{
    _NXLogError("name=\"%s\", symtab=%x\n", mod->name, mod->symtab);
}

void dumpModules(void)
{
    int i,j;
    Module mod;
    objcModHeader *cmh;

    _NXLogError("dumpModules(): found %d header(s)\n", objcHeaders.count);
    for (j=0; j<objcHeaders.count; ++j) {
	        cmh = (objcModHeader *)objcHeaders.data + j;

	_NXLogError("===%s, found %d modules\n", cmh->name, cmh->Modules.count);


	mod = (Module)cmh->Modules.data;
    
	for (i=0; i<cmh->Modules.count; i++) {
		    _NXLogError("\tname=\"%s\", symtab=%x, sel_ref_cnt=%d\n", mod->name, mod->symtab, (Symtab)(mod->symtab)->sel_ref_cnt);
	    mod++;
	}
}
}
#endif  // DEBUG

static inline void addObjcProtocols(struct objc_protocol_list * pl)
{
   if ( !pl )
      return;
   else {
      int count = 0;
      struct objc_protocol_list *list = pl;
      while ( list ) {
         count += list->count;
         list = list->next;
      }
      allocElements( &CMH->Protocols, count );

      list = pl;
      while ( list ) {
         int i = 0;
         while ( i < list->count )
            CMH->Protocols.data[ CMH->Protocols.count++ ] = (void*) list->list[i++];
         list = list->next;
      }

      list = pl;
      while ( list ) {
         int i = 0;
         while ( i < list->count )
            addObjcProtocols( ((ProtocolTemplate*)list->list[i++])->protocol_list );
         list = list->next;
      }
   }
}


static inline void addObjcMessages(struct objc_method_list *ml)
{
  int   mcount;
  int i;

  if (! ml)
	return;

  mcount = ml->method_count;

  // Walk through all of the methods in this class
  for ( i=0; i < mcount; i++ )
  {
      ml->method_list[i].method_name = 
      _sel_registerName ((const char *)ml->method_list[i].method_name);
  }
}

static void
_parseObjcModule(struct objc_symtab *symtab)
{
    int i=0, j=0, k;
    SEL *refs = symtab->refs;


    // Add the selector references

    if (refs)
    {
        symtab->sel_ref_cnt = 0;

        while (*refs)
        {
            symtab->sel_ref_cnt++;
            *refs = _sel_registerName ((const char *) *refs);
            refs++;
        }
    }

    // Walk through all of the ObjC Classes

    if ((k = symtab->cls_def_cnt))
      {
	allocElements (&CMH->Classes, k);

	for ( i=0, j = symtab->cls_def_cnt; i < j; i++ )
	  {
	    struct objc_class       *class;
 	    unsigned loop;
	    
	    class  = (struct objc_class *)symtab->defs[i];
	    objc_addClass(class);
	    CMH->Classes.data[ CMH->Classes.count++ ] = (void*) class->name;
	    addObjcProtocols (class->protocols);

	    loop = 0;
	    if (class->methodLists)
	        while (class->methodLists[loop])
	            addObjcMessages  (class->methodLists[loop++]);

 	    loop = 0;
	    if (class->isa->methodLists)
	        while (class->isa->methodLists[loop])
	            addObjcMessages  (class->isa->methodLists[loop++]);
	  }
      }

    // Walk through all of the ObjC Categories

    if ((k = symtab->cat_def_cnt))
      {
	allocElements (&CMH->Classes, k);

	for ( j += symtab->cat_def_cnt;
	     i < j;
	     i++ )
	  {
	    struct objc_category       *category;
	    
	    category  = (struct objc_category *)symtab->defs[i];
	    CMH->Classes.data[ CMH->Classes.count++ ] = 
		(void*) category->class_name;

	    addObjcProtocols (category->protocols);
	    addObjcMessages  (category->instance_methods);
	    addObjcMessages  (category->class_methods);
	  }
      }

    // Walk through all of the ObjC Static Strings

    if ((k = symtab->obj_defs))
      {
	allocElements (&CMH->StringObjects, k);

	for ( j += symtab->obj_defs;
	     i < j;
	     i++ )
	  {
	    NXConstantStringTemplate *string = ( NXConstantStringTemplate *)symtab->defs[i];
	    CMH->StringObjects.data[ CMH->StringObjects.count++ ] = 
		(void*) string;
	  }
      }

    // Walk through all of the ObjC Static Protocols

    if ((k = symtab->proto_defs))
      {
	allocElements (&CMH->Protocols, k);

	for ( j += symtab->proto_defs;
	     i < j;
	     i++ )
	  {
	    ProtocolTemplate *proto = ( ProtocolTemplate *)symtab->defs[i];
            allocElements (&CMH->Protocols, 1);
	    CMH->Protocols.data[ CMH->Protocols.count++ ] = 
		(void*) proto;

	    addObjcProtocols(proto->protocol_list);
	  }
      }
}

void __objc_execClass(Module mod)
{
    _sel_registerName ((const char *)OBJC_METH_VAR_NAME_FORWARD);

    if (CMH == 0) {
	    objc_register_header(NXArgv ? NXArgv[0] : "");
    }

    allocElements (&CMH->Modules, 1);

    memcpy( (Module)CMH->Modules.data 
                  + CMH->Modules.count,
	    mod,
	    sizeof(struct objc_module));
    CMH->Modules.count++;

    _parseObjcModule(mod->symtab);
}

const char * NSModulePathForClass(Class cls)
{
#if defined(WIN32)
    int i, j, k;

    for (i = 0; i < objcHeaders.count; i++) {
	volatile objcModHeader *aHeader = (objcModHeader *)objcHeaders.data + i;
	for (j = 0; j < aHeader->Modules.count; j++) {
	    Module mod = (void *)(aHeader->Modules.data) + j * aHeader->Modules.size;
	    struct objc_symtab *symtab = mod->symtab;
	    for (k = 0; k < symtab->cls_def_cnt; k++) {
		if (cls == (Class)symtab->defs[k])
		    return aHeader->name;
	    }
	}
    }
#else
    #warning "NSModulePathForClass is not fully implemented!"
#endif
    return NULL;
}

unsigned int _objc_goff_headerCount (void)
{
    return objcHeaders.count;
}

/* Build the header vector, of all headers seen so far. */

struct header_info *_objc_goff_headerVector ()
{
  unsigned int hidx;
  struct header_info *hdrVec;

  hdrVec = NXZoneMalloc (_objc_create_zone(),
                         objcHeaders.count * sizeof (struct header_info));
#if defined(WIN32) || defined(__svr4__)
  bzero(hdrVec, (objcHeaders.count * sizeof (struct header_info)));
#endif

  for (hidx = 0; hidx < objcHeaders.count; hidx++)
    {
      objcModHeader *aHeader = (objcModHeader *)objcHeaders.data + hidx;
 
      hdrVec[hidx].mhdr = (headerType**) aHeader;
      hdrVec[hidx].mod_ptr = (Module)(aHeader->Modules.data);
    }
  return hdrVec;
}


#if defined(sparc)
    int __NXArgc = 0;
    char ** __NXArgv = 0;  
#endif 

/* Returns an array of all the objc headers in the executable (and shlibs)
 * Caller is responsible for freeing.
 */
headerType **_getObjcHeaders()
{
								   
#if defined(hpux)
    OBJC_EXPORT int __argc_value;
    OBJC_EXPORT char ** __argv_value;
#endif

  /* Will need to fill in with any shlib info later as well.  Need more
   * info on this.
   */
  
  headerType **hdrs = (headerType**)malloc(2 * sizeof(headerType*));
#if defined(WIN32) || defined(__svr4__)
  bzero(hdrs, (2 * sizeof(headerType*)));
#endif
#if hpux
  NXArgv = __argv_value;
  NXArgc = __argc_value;
#else /* hpux */
#if defined(sparc) 
  NXArgv = __NXArgv;
  NXArgc = __NXArgc;
#endif /* sparc */
#endif /* hpux */

  hdrs[0] = (headerType*)CMH;
  hdrs[1] = 0;
  return hdrs;
}

static objcModHeader *_getObjcModHeader(headerType *head)
{
	return (objcModHeader *)head;
}
 
Module _getObjcModules(headerType *head, int *size)
{
    objcModHeader *modHdr = _getObjcModHeader(head);
    if (modHdr) {
	*size = modHdr->Modules.count;
	return (Module)(modHdr->Modules.data);
    }
    else {
	*size = 0;
	return (Module)0;
    }
}

ProtocolTemplate **_getObjcProtocols(headerType *head, int *nprotos)
{
    objcModHeader *modHdr = _getObjcModHeader(head);

    if (modHdr) {
	*nprotos = modHdr->Protocols.count;
	return (ProtocolTemplate **)modHdr->Protocols.data;
    }
    else {
	*nprotos = 0;
	return (ProtocolTemplate **)0;
    }
}


NXConstantStringTemplate **_getObjcStringObjects(headerType *head, int *nstrs)
{
    objcModHeader *modHdr = _getObjcModHeader(head);

    if (modHdr) {
	*nstrs = modHdr->StringObjects.count;
	return (NXConstantStringTemplate **)modHdr->StringObjects.data;
    }
    else {
	*nstrs = 0;
	return (NXConstantStringTemplate **)0;
    }
}

Class *_getObjcClassRefs(headerType *head, int *nclasses)
{
    objcModHeader *modHdr = _getObjcModHeader(head);

    if (modHdr) {
	*nclasses = modHdr->Classes.count;
	return (Class *)modHdr->Classes.data;
    }
    else {
	*nclasses = 0;
	return (Class *)0;
    }
}

/* returns start of all objective-c info and the size of the data */
void *_getObjcHeaderData(headerType *head, unsigned *size)
{
  *size = 0;
  return NULL;
}

SEL *_getObjcMessageRefs(headerType *head, int *nmess)
{
  *nmess = 0;
  return (SEL *)NULL;
}

SEL *_getObjcBackRefs(headerType *head, int *nmess)
{
  *nmess = 0;
  return (SEL *)NULL;
}

SEL **_getObjcConflicts(headerType *head, int *nbytes)
{
  *nbytes = 0;
  return (SEL **)NULL;
}

const char *_getObjcHeaderName(headerType *header)
{
  return "InvalidHeaderName";
}
