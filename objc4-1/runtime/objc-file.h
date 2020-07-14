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

#import <objc/objc.h>
#import <objc/objc-runtime.h>
#import <objc/Protocol.h>

/* Opaque cookie used in _getObjc... routines.  File format independant.
 * This is used in place of the mach_header.  In fact, when compiling
 * for NEXTSTEP, this is really a (struct mach_header *).
 *
 * had been: typedef void *objc_header;
 */
#if defined(NeXT_PDO)
    typedef void headerType;
#else 
    #import <mach-o/loader.h>
    typedef struct mach_header headerType;
#endif 

typedef struct _ProtocolTemplate { @defs(Protocol) } ProtocolTemplate;
typedef struct _NXConstantStringTemplate {
    Class isa;
    void *characters;
    unsigned int _length;
} NXConstantStringTemplate;

#if defined(NeXT_PDO)
    #define OBJC_CONSTANT_STRING_PTR NXConstantStringTemplate**
    #define OBJC_CONSTANT_STRING_DEREF
    #define OBJC_PROTOCOL_PTR ProtocolTemplate**
    #define OBJC_PROTOCOL_DEREF ->
#elif defined(__MACH__)
    #define OBJC_CONSTANT_STRING_PTR NXConstantStringTemplate*
    #define OBJC_CONSTANT_STRING_DEREF &
    #define OBJC_PROTOCOL_PTR ProtocolTemplate*
    #define OBJC_PROTOCOL_DEREF .
#endif


// both
OBJC_EXPORT headerType **	_getObjcHeaders();
OBJC_EXPORT Module		_getObjcModules(headerType *head, int *nmodules);
OBJC_EXPORT Class *		_getObjcClassRefs(headerType *head, int *nclasses);
OBJC_EXPORT void *		_getObjcHeaderData(headerType *head, unsigned *size);
OBJC_EXPORT const char *	_getObjcHeaderName(headerType *head);

#if defined(NeXT_PDO) // GENERIC_OBJ_FILE
    OBJC_EXPORT ProtocolTemplate ** _getObjcProtocols(headerType *head, int *nprotos);
    OBJC_EXPORT NXConstantStringTemplate **_getObjcStringObjects(headerType *head, int *nstrs);
#elif defined(__MACH__)
    OBJC_EXPORT ProtocolTemplate * _getObjcProtocols(headerType *head, int *nprotos);
    OBJC_EXPORT NXConstantStringTemplate *_getObjcStringObjects(headerType *head, int *nstrs);
    OBJC_EXPORT const char *	_getObjcStrings(headerType *head, int *nbytes);
    OBJC_EXPORT SEL *		_getObjcBackRefs(headerType *head, int *nmess);
    OBJC_EXPORT SEL **		_getObjcConflicts(headerType *head, int *nbytes);
    OBJC_EXPORT SEL *		_getObjcMessageRefs(headerType *head, int *nmess);
    OBJC_EXPORT void *		_getObjcFrozenTable(headerType *head);
#endif 
