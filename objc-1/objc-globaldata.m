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
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"
#import "objc-class.h"
#import <objc/zone.h>

/*
 * Declarations of non-const global data.
 */
extern id _internal_class_createInstance(Class, unsigned);
extern id _internal_class_createInstanceFromZone(Class, unsigned, NXZone *);
extern id _internal_object_dispose(id);
extern id _internal_object_realloc(id, unsigned);
extern id _internal_object_reallocFromZone(id, unsigned, NXZone *);
extern id _internal_object_copy(id, unsigned);
extern id _internal_object_copyFromZone(id, unsigned, NXZone *);

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

#ifdef SHLIB
char _objc_global_data_pad[468] = {0};
#endif