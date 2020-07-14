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
/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#if !defined(__DYNAMIC__)
#pragma CC_NO_MACH_TEXT_SECTIONS
#endif

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "hashtable.h"
#import "maptable.h"

/*
 * Global const data would go here and would look like:
 * 	const int foo = 1;
 */	
/*
 * hashtable globals
 */

extern unsigned hashPtrStructKey (const void *info, const void *data);
extern int isEqualPtrStructKey (const void *info, const void *data1, const void *data2);
extern unsigned hashStrStructKey (const void *info, const void *data);
extern int isEqualStrStructKey (const void *info, const void *data1, const void *data2);

const NXHashTablePrototype NXPtrPrototype = {
    NXPtrHash, NXPtrIsEqual, NXNoEffectFree, 0
    };
const NXHashTablePrototype NXStrPrototype = {
    NXStrHash, NXStrIsEqual, NXNoEffectFree, 0
    };


const NXHashTablePrototype NXPtrStructKeyPrototype = {
    hashPtrStructKey, isEqualPtrStructKey, NXReallyFree, 0
    };

const NXHashTablePrototype NXStrStructKeyPrototype = {
    hashStrStructKey, isEqualStrStructKey, NXReallyFree, 0
    };

extern unsigned _mapPtrHash(NXMapTable *table, const void *key);
extern unsigned _mapStrHash(NXMapTable *table, const void *key);
extern unsigned _mapObjectHash(NXMapTable *table, const void *key);
extern int _mapPtrIsEqual(NXMapTable *table, const void *key1, const void *key2);
extern int _mapStrIsEqual(NXMapTable *table, const void *key1, const void *key2);
extern int _mapObjectIsEqual(NXMapTable *table, const void *key1, const void *key2);
extern void _mapNoFree(NXMapTable *table, void *key, void *value);
extern void _mapObjectFree(NXMapTable *table, void *key, void *value);

const NXMapTablePrototype NXPtrValueMapPrototype = {
    _mapPtrHash, _mapPtrIsEqual, _mapNoFree, 0
};

const NXMapTablePrototype NXStrValueMapPrototype = {
    _mapStrHash, _mapStrIsEqual, _mapNoFree, 0
};

const NXMapTablePrototype NXObjectMapPrototype = {
    _mapObjectHash, _mapObjectIsEqual, _mapObjectFree, 0
};

#ifdef SHLIB
static const char _objc_global_text_pad[144] = {0};

/*
 * Declarations of static (literal) const data.
 */
static const char _objc_literal_text_pad[256] = {0};

#endif
