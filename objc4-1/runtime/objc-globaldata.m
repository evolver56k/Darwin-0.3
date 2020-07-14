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

#ifdef WINNT 
#include <winnt-pdo.h>
#endif

#import "objc-private.h"
#import <objc/objc-class.h>
#import <objc/zone.h>

/*
 * Declarations of non-const global data.
 */
OBJC_EXPORT id _internal_class_createInstance(Class, unsigned);
OBJC_EXPORT id _internal_class_createInstanceFromZone(Class, unsigned, NXZone *);
OBJC_EXPORT id _internal_object_dispose(id);
OBJC_EXPORT id _internal_object_realloc(id, unsigned);
OBJC_EXPORT id _internal_object_reallocFromZone(id, unsigned, NXZone *);
OBJC_EXPORT id _internal_object_copy(id, unsigned);
OBJC_EXPORT id _internal_object_copyFromZone(id, unsigned, NXZone *);

#if defined(BUG67896)
OBJC_EXPORT id ___internal_object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *zone) {
    return _internal_object_copyFromZone(anObject, nBytes, zone); }
id (*_zoneCopy)(id, unsigned, NXZone *) = ___internal_object_copyFromZone;

OBJC_EXPORT id ___internal_object_copy(Object *anObject, unsigned nBytes) {
    return _internal_object_copy(anObject, nBytes); }
id (*_copy)(id, unsigned) = ___internal_object_copy;

OBJC_EXPORT id ___internal_object_dispose(Object *anObject) {
    return _internal_object_dispose(anObject); }
id (*_dealloc)(id)  = ___internal_object_dispose;

OBJC_EXPORT id ___internal_object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *zone) {
    return _internal_object_reallocFromZone(anObject, nBytes, zone); } 
id (*_zoneRealloc)(id, unsigned, NXZone *) = ___internal_object_reallocFromZone;

OBJC_EXPORT id ___internal_object_realloc(Object *anObject, unsigned nBytes) {
    return _internal_object_realloc(anObject, nBytes); }
id (*_realloc)(id, unsigned) = ___internal_object_realloc;

OBJC_EXPORT id ___internal_objc_lookUpClass (const char *aClassName) {
    return objc_lookUpClass (aClassName); }
id (*_cvtToId)(const char *) = ___internal_objc_lookUpClass;

OBJC_EXPORT id ___internal_class_createInstanceFromZone(Class aClass, unsigned nBytes, NXZone *zone) {
    return _internal_class_createInstanceFromZone(aClass, nBytes, zone); } 
id (*_zoneAlloc)(Class, unsigned, NXZone *) = ___internal_class_createInstanceFromZone;

OBJC_EXPORT id ___internal_class_createInstance(Class aClass, unsigned nBytes) {
    return _internal_class_createInstance(aClass, nBytes); } 
id (*_alloc)(Class, unsigned) = ___internal_class_createInstance;

OBJC_EXPORT SEL ___internal_sel_getUid (const char *key) { return sel_getUid (key); }
SEL (*_cvtToSel)(const char *) = ___internal_sel_getUid;

OBJC_EXPORT Class ___internal_class_poseAs(Class imposter, Class original) {
    return class_poseAs(imposter, original); } 
id (*_poseAs)() = (id (*)())___internal_class_poseAs;

OBJC_EXPORT void ___objc_error(id self, const char *fmt, va_list ap) {
    return _objc_error(self, fmt, ap); } 
void (*_error)() = (void(*)())___objc_error;

#else // not BUG67896
    id (*_poseAs)() = (id (*)())class_poseAs;
    id (*_alloc)(Class, unsigned) = _internal_class_createInstance;
    id (*_copy)(id, unsigned) = _internal_object_copy;
    id (*_realloc)(id, unsigned) = _internal_object_realloc;
    id (*_dealloc)(id)  = _internal_object_dispose;

    id (*_cvtToId)(const char *)= objc_lookUpClass;
    SEL (*_cvtToSel)(const char *)= sel_getUid;
    void (*_error)() = (void(*)())_objc_error;

    id (*_zoneAlloc)(Class, unsigned, NXZone *) = _internal_class_createInstanceFromZone;
    id (*_zoneCopy)(id, unsigned, NXZone *) = _internal_object_copyFromZone;
    id (*_zoneRealloc)(id, unsigned, NXZone *) = _internal_object_reallocFromZone;
#endif
