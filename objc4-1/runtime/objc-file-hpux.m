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
// Copyright 1988-1996 NeXT Software, Inc.

/* 
 *      Objective-C runtime information module.
 *      This module is specific to hp-ux a.out format files.
 */

#import <pdo.h>	// place where padding_bug would be
#include <a.out.h>
#include "objc-file.h"

OBJC_EXPORT int __argc_value;
OBJC_EXPORT char **__argv_value;
int NXArgc = 0;
char **NXArgv = NULL;

OBJC_EXPORT unsigned SEG_OBJC_CLASS_START;
OBJC_EXPORT unsigned SEG_OBJC_METACLASS_START;
OBJC_EXPORT unsigned SEG_OBJC_CAT_CLS_METH_START;
OBJC_EXPORT unsigned SEG_OBJC_CAT_INST_METH_START;
OBJC_EXPORT unsigned SEG_OBJC_CLS_METH_START;
OBJC_EXPORT unsigned SEG_OBJC_INST_METHODS_START;
OBJC_EXPORT unsigned SEG_OBJC_MESSAGE_REFS_START;
OBJC_EXPORT unsigned SEG_OBJC_SYMBOLS_START;
OBJC_EXPORT unsigned SEG_OBJC_CATEGORY_START;
OBJC_EXPORT unsigned SEG_OBJC_PROTOCOL_START;
OBJC_EXPORT unsigned SEG_OBJC_CLASS_VARS_START;
OBJC_EXPORT unsigned SEG_OBJC_INSTANCE_VARS_START;
OBJC_EXPORT unsigned SEG_OBJC_MODULES_START;
OBJC_EXPORT unsigned SEG_OBJC_STRING_OBJECT_START;
OBJC_EXPORT unsigned SEG_OBJC_CLASS_NAMES_START;
OBJC_EXPORT unsigned SEG_OBJC_METH_VAR_NAMES_START;
OBJC_EXPORT unsigned SEG_OBJC_METH_VAR_TYPES_START;
OBJC_EXPORT unsigned SEG_OBJC_CLS_REFS_START;

OBJC_EXPORT unsigned SEG_OBJC_CLASS_END;
OBJC_EXPORT unsigned SEG_OBJC_METACLASS_END;
OBJC_EXPORT unsigned SEG_OBJC_CAT_CLS_METH_END;
OBJC_EXPORT unsigned SEG_OBJC_CAT_INST_METH_END;
OBJC_EXPORT unsigned SEG_OBJC_CLS_METH_END;
OBJC_EXPORT unsigned SEG_OBJC_INST_METHODS_END;
OBJC_EXPORT unsigned SEG_OBJC_MESSAGE_REFS_END;
OBJC_EXPORT unsigned SEG_OBJC_SYMBOLS_END;
OBJC_EXPORT unsigned SEG_OBJC_CATEGORY_END;
OBJC_EXPORT unsigned SEG_OBJC_PROTOCOL_END;
OBJC_EXPORT unsigned SEG_OBJC_CLASS_VARS_END;
OBJC_EXPORT unsigned SEG_OBJC_INSTANCE_VARS_END;
OBJC_EXPORT unsigned SEG_OBJC_MODULES_END;
OBJC_EXPORT unsigned SEG_OBJC_STRING_OBJECT_END;
OBJC_EXPORT unsigned SEG_OBJC_CLASS_NAMES_END;
OBJC_EXPORT unsigned SEG_OBJC_METH_VAR_NAMES_END;
OBJC_EXPORT unsigned SEG_OBJC_METH_VAR_TYPES_END;
OBJC_EXPORT unsigned SEG_OBJC_CLS_REFS_END;

typedef struct	_simple_header_struct {
	char * 	subspace_name	;
	void *	start_address	;
	void *	end_address	;
	} simple_header_struct ;

static simple_header_struct our_objc_header[] = {
	{ "$$OBJC_CLASS$$", 		&SEG_OBJC_CLASS_START, 		&SEG_OBJC_CLASS_END },
	{ "$$OBJC_METACLASS$$", 	&SEG_OBJC_METACLASS_START, 	&SEG_OBJC_METACLASS_END },
	{ "$$OBJC_CAT_CLS_METH$$",	&SEG_OBJC_CAT_CLS_METH_START, 	&SEG_OBJC_CAT_CLS_METH_END },
	{ "$$OBJC_CAT_INST_METH$$", 	&SEG_OBJC_CAT_INST_METH_START, 	&SEG_OBJC_CAT_INST_METH_END },
	{ "$$OBJC_CLS_METH$$", 		&SEG_OBJC_CLS_METH_START, 	&SEG_OBJC_CLS_METH_END },
	{ "$$OBJC_INST_METHODS$$",	&SEG_OBJC_INST_METHODS_START, 	&SEG_OBJC_INST_METHODS_END },
	{ "$$OBJC_MESSAGE_REFS$$",	&SEG_OBJC_MESSAGE_REFS_START, 	&SEG_OBJC_MESSAGE_REFS_END },
	{ "$$OBJC_SYMBOLS$$", 		&SEG_OBJC_SYMBOLS_START, 	&SEG_OBJC_SYMBOLS_END },
	{ "$$OBJC_CATEGORY$$", 		&SEG_OBJC_CATEGORY_START, 	&SEG_OBJC_CATEGORY_END },
	{ "$$OBJC_PROTOCOL$$", 		&SEG_OBJC_PROTOCOL_START, 	&SEG_OBJC_PROTOCOL_END },
	{ "$$OBJC_CLASS_VARS$$", 	&SEG_OBJC_CLASS_VARS_START, 	&SEG_OBJC_CLASS_VARS_END },
	{ "$$OBJC_INSTANCE_VARS$$", 	&SEG_OBJC_INSTANCE_VARS_START, 	&SEG_OBJC_INSTANCE_VARS_END },
	{ "$$OBJC_MODULES$$", 		&SEG_OBJC_MODULES_START, 	&SEG_OBJC_MODULES_END },
	{ "$$OBJC_STRING_OBJECT$$", 	&SEG_OBJC_STRING_OBJECT_START, 	&SEG_OBJC_STRING_OBJECT_END },
	{ "$$OBJC_CLASS_NAMES$$", 	&SEG_OBJC_CLASS_NAMES_START, 	&SEG_OBJC_CLASS_NAMES_END },
	{ "$$OBJC_METH_VAR_NAMES$$", 	&SEG_OBJC_METH_VAR_TYPES_START, &SEG_OBJC_METH_VAR_NAMES_END },
	{ "$$OBJC_METH_VAR_TYPES$$",	&SEG_OBJC_METH_VAR_TYPES_START, &SEG_OBJC_METH_VAR_TYPES_END },
	{ "$$OBJC_CLS_REFS$$", 		&SEG_OBJC_CLS_REFS_START, 	&SEG_OBJC_CLS_REFS_END },
	{ NULL, NULL, NULL }
	};

/* Returns an array of all the objc headers in the executable (and shlibs)
 * Caller is responsible for freeing.
 */
headerType **_getObjcHeaders()
{

  /* Will need to fill in with any shlib info later as well.  Need more
   * info on this.
   */
  
  /*
   *	this is truly ugly, hpux does not map in the header so we have to
   * 	try and find it and map it in.  their crt0 has some global vars
   *    that hold argv[0] which we will use to find the executable file
   */

  headerType **hdrs = (headerType**)malloc(2 * sizeof(headerType*));
  NXArgv = __argv_value;
  NXArgc = __argc_value;
  hdrs[0] = &our_objc_header;
  hdrs[1] = 0;
  return hdrs;
}

// I think we are getting the address of the table (ie the table itself) 
//	isn't that expensive ?
static void *getsubspace(headerType *objchead, char *sname, unsigned *size)
{
	simple_header_struct *table = (simple_header_struct *)objchead;
	int i = 0;

	while (  table[i].subspace_name){
		if (!strcmp(table[i].subspace_name, sname)){
			*size = table[i].end_address - table[i].start_address;
			return table[i].start_address;
		}
		i++;
	}
	*size = 0;
	return nil;
}

Module _getObjcModules(headerType *head, int *nmodules)
{
  unsigned size;
  void *mods = getsubspace(head,"$$OBJC_MODULES$$",&size);
  *nmodules = size / sizeof(struct objc_module);
  return (Module)mods;
}

void *_getObjcFrozenTable(headerType *head)
{
  unsigned size;
  return getsubspace(head,"$$OBJC_RUNTIME_SETUP$$",&size);
}

const char *_getObjcStrings(headerType *head, int *nbytes)
{
  return (const char *)getsubspace(head,"$$OBJC_METH_VAR_NAMES$$",nbytes);
}

SEL *_getObjcMessageRefs(headerType *head, int *nmess)
{
  unsigned size;
  void *refs = getsubspace (head,"$$OBJC_MESSAGE_REFS$$", &size);
  *nmess = size / sizeof(SEL);
  return (SEL *)refs;
}

struct proto_template *_getObjcProtocols(headerType *head, int *nprotos)
{
  unsigned size;
  char *p;
  char *end;
  char *start;

  start = getsubspace (head,"$$OBJC_PROTOCOL$$", &size);

#ifdef PADDING_BUG
  /*
   * XXX: Look for padding of 4 zero bytes and remove it.
   * XXX: Depends upon first four bytes of a proto_template never being 0.
   * XXX: Somebody should check to see if this is really the case.
   */
  end = start + size;
  for (p = start; p < end; p += sizeof(struct proto_template)) {
      if (!p[0] && !p[1] && !p[2] && !p[3]) {
          memcpy(p, p + sizeof(long), (end - p) - sizeof(long));
          end -= sizeof(long);
      }
  }
  size = end - start;
#endif
  *nprotos = size / sizeof(struct proto_template);
  return ((struct proto_template *)start);
}

NXConstantStringTemplate *_getObjcStringObjects(headerType *head, int *nstrs)
{
  unsigned size;
  void *str = getsubspace (head,"$$OBJC_STRING_OBJECT$$", &size);
  *nstrs = size / sizeof(NXConstantStringTemplate);
  return (NXConstantStringTemplate *)str;
}

Class *_getObjcClassRefs(headerType *head, int *nclasses)
{
  unsigned size;
  void *classes = getsubspace (head,"$$OBJC_CLS_REFS$$", &size);
  *nclasses = size / sizeof(Class);
  return (Class *)classes;
}

/* returns start of all objective-c info and the size of the data */
void *_getObjcHeaderData(headerType *head, unsigned *size)
{
#warning _getObjcHeaderData not implemented yet
  *size = 0;
  return nil;
}


const char *_getObjcHeaderName(headerType *header)
{
  return "oh poo";
}
