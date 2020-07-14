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
 *	objc-class.h
 *	Copyright 1988, NeXT, Inc.
 */

#ifndef _OBJC_CLASS_H_
#define _OBJC_CLASS_H_

#import "objc.h"
#import <objc/zone.h>
/* 
 *	Class Template
 */
struct objc_class {			
	struct objc_class *isa;	
	struct objc_class *super_class;	
	const char *name;		
	long version;
	long info;
	long instance_size;
	struct objc_ivar_list *ivars;
	struct objc_method_list *methods;
	struct objc_cache *cache;
 	struct objc_protocol_list *protocols;
};
#define CLS_GETINFO(cls,infomask)	((cls)->info & infomask)
#define CLS_SETINFO(cls,infomask)	((cls)->info |= infomask)

#define CLS_CLASS		0x1L
#define CLS_META		0x2L
#define CLS_INITIALIZED		0x4L
#define CLS_POSING		0x8L
#define CLS_MAPPED		0x10L
#define CLS_FLUSH_CACHE		0x20L
#define CLS_GROW_CACHE		0x40L
/* 
 *	Category Template
 */
typedef struct objc_category *Category;

struct objc_category {
	char *category_name;
	char *class_name;
	struct objc_method_list *instance_methods;
	struct objc_method_list *class_methods;
 	struct objc_protocol_list *protocols;
};
/* 
 *	Instance Variable Template
 */
typedef struct objc_ivar *Ivar;

struct objc_ivar_list {
	int ivar_count;
	struct objc_ivar {
		char *ivar_name;
		char *ivar_type;
		int ivar_offset;
	} ivar_list[1];			/* variable length structure */
};
/* 
 *	Method Template
 */
typedef struct objc_method *Method;

struct objc_method_list {
	struct objc_method_list *method_next;
	int method_count;
	struct objc_method {
		SEL method_name;
		char *method_types;
                IMP method_imp;
	} method_list[1];		/* variable length structure */
};

/* Protocol support */

@class Protocol;

struct objc_protocol_list {
	struct objc_protocol_list *next;
	int count;
	Protocol *list[1];
};

/* Definitions of filer types */

#define _C_ID		'@'
#define _C_CLASS	'#'
#define _C_SEL		':'
#define _C_CHR		'c'
#define _C_UCHR		'C'
#define _C_SHT		's'
#define _C_USHT		'S'
#define _C_INT		'i'
#define _C_UINT		'I'
#define _C_LNG		'l'
#define _C_ULNG		'L'
#define _C_FLT		'f'
#define _C_DBL		'd'
#define _C_BFLD		'b'
#define _C_VOID		'v'
#define _C_UNDEF	'?'
#define _C_PTR		'^'
#define _C_CHARPTR	'*'
#define _C_ARY_B	'['
#define _C_ARY_E	']'
#define _C_UNION_B	'('
#define _C_UNION_E	')'
#define _C_STRUCT_B	'{'
#define _C_STRUCT_E	'}'

/* Structure for method cache - allocated/sized at runtime */

typedef struct objc_cache *Cache;

#ifdef OBJC_COPY_CACHE

#define CACHE_BUCKET_NAME(B)  ((B).method_name)
#define CACHE_BUCKET_IMP(B)   ((B).method_imp)
#define CACHE_BUCKET_VALID(B) ((B).method_name)
struct objc_cache_bucket {
  SEL method_name;
  IMP method_imp;
};

struct objc_cache {
	unsigned int mask;            /* total = mask + 1 */
	unsigned int occupied;        
	struct objc_cache_bucket buckets[1];
};

#else

#define CACHE_BUCKET_NAME(B)  ((B)->method_name)
#define CACHE_BUCKET_IMP(B)   ((B)->method_imp)
#define CACHE_BUCKET_VALID(B) (B)
struct objc_cache {
	unsigned int mask;            /* total = mask + 1 */
	unsigned int occupied;        
	Method buckets[1];
};
#endif

/* operations */

extern id class_createInstance(Class, unsigned idxIvars);
extern id class_createInstanceFromZone(Class, unsigned idxIvars, NXZone *zone);

extern void class_setVersion(Class, int);
extern int class_getVersion(Class);

extern Ivar class_getInstanceVariable(Class, const char *);
extern Method class_getInstanceMethod(Class, SEL);
extern Method class_getClassMethod(Class, SEL);

extern void class_addMethods(Class, struct objc_method_list *);
extern void class_removeMethods(Class, struct objc_method_list *);

extern Class class_poseAs(Class imposter, Class original);

extern unsigned method_getNumberOfArguments(Method);
extern unsigned method_getSizeOfArguments(Method);
extern unsigned method_getArgumentInfo(Method m, int arg, const char **type, int *offset);

typedef void *marg_list;

#define marg_getRef(margs, offset, type) \
	( (type *)((char *)margs + offset) )

#define marg_getValue(margs, offset, type) \
	( *marg_getRef(margs, offset, type) )

#define marg_setValue(margs, offset, type, value) \
	( marg_getValue(margs, offset, type) = (value) )

#endif /* _OBJC_CLASS_H_ */
