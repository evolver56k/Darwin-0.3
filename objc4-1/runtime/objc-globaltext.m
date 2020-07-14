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

/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#if !defined(__DYNAMIC__)
#pragma CC_NO_MACH_TEXT_SECTIONS
#endif

#ifdef WINNT
#include <winnt-pdo.h>
#endif 

#import <objc/hashtable2.h>
#import "maptable.h"
#import "objc-private.h"

OBJC_EXPORT unsigned _mapPtrHash(NXMapTable *table, const void *key);
OBJC_EXPORT unsigned _mapStrHash(NXMapTable *table, const void *key);
OBJC_EXPORT unsigned _mapObjectHash(NXMapTable *table, const void *key);
OBJC_EXPORT int _mapPtrIsEqual(NXMapTable *table, const void *key1, const void *key2);
OBJC_EXPORT int _mapStrIsEqual(NXMapTable *table, const void *key1, const void *key2);
OBJC_EXPORT int _mapObjectIsEqual(NXMapTable *table, const void *key1, const void *key2);
OBJC_EXPORT void _mapNoFree(NXMapTable *table, void *key, void *value);
OBJC_EXPORT void _mapObjectFree(NXMapTable *table, void *key, void *value);

#if defined(BUG67896)
static unsigned mapObjectHash(NXMapTable *table, const void *key) {
    return _mapObjectHash(table, key); }

static int mapObjectIsEqual(NXMapTable *table, const void *key1, const void *key2) { 
    return _mapObjectIsEqual(table, key1, key2); }

static void mapObjectFree(NXMapTable *table, void *key, void *value) {
    return _mapObjectFree(table, key, value); }

static unsigned mapPtrHash(NXMapTable *table, const void *key) {
    return _mapPtrHash(table, key); }

static unsigned mapStrHash(NXMapTable *table, const void *key) {
    return _mapStrHash(table, key); }

static int mapPtrIsEqual(NXMapTable *table, const void *key1, const void *key2) { 
    return _mapPtrIsEqual(table, key1, key2); }

static int mapStrIsEqual(NXMapTable *table, const void *key1, const void *key2) {
    return _mapStrIsEqual(table, key1, key2); }

static void mapNoFree(NXMapTable *table, void *key, void *value) {}

static unsigned _hashPtrStructKey (const void *info, const void *data) {
    return hashPtrStructKey (info, data); }

static int _isEqualPtrStructKey (const void *info, const void *data1, const void *data2) {
    return isEqualPtrStructKey (info, data1, data2); }

static unsigned _hashStrStructKey (const void *info, const void *data) {
    return hashStrStructKey (info, data); }

static int _isEqualStrStructKey (const void *info, const void *data1, const void *data2) {
    return isEqualStrStructKey (info, data1, data2); }

static void _NXNoEffectFree (const void *info, void *data) {
    NXNoEffectFree (info, data); }

static void _NXReallyFree (const void *info, void *data) { 
    NXReallyFree (info, data); }

static unsigned _NXPtrHash(const void *info, const void *data) {
    return NXPtrHash(info, data); }

static unsigned _NXStrHash(const void *info, const void *data) {
    return NXStrHash(info, data); }

static int _NXPtrIsEqual(const void *info, const void *data1, const void *data2) {
    return NXPtrIsEqual(info, data1, data2); }

static int _NXStrIsEqual(const void *info, const void *data1, const void *data2) {
    return NXStrIsEqual(info, data1, data2); }
#endif // BUG67896

/*
 * Global const data would go here and would look like:
 * 	const int foo = 1;
 */	

/*
 * hashtable globals
 */
const NXHashTablePrototype NXPtrPrototype = {
#if defined(BUG67896)
    _NXPtrHash, _NXPtrIsEqual, _NXNoEffectFree, 0
#else
    NXPtrHash, NXPtrIsEqual, NXNoEffectFree, 0
#endif
    };

const NXHashTablePrototype NXStrPrototype = {
#if defined(BUG67896)
    _NXStrHash, _NXStrIsEqual, _NXNoEffectFree, 0
#else
    NXStrHash, NXStrIsEqual, NXNoEffectFree, 0
#endif
    };

const NXHashTablePrototype NXPtrStructKeyPrototype = {
#if defined(BUG67896)
    _hashPtrStructKey, _isEqualPtrStructKey, _NXReallyFree, 0
#else
    hashPtrStructKey, isEqualPtrStructKey, NXReallyFree, 0
#endif
    };

const NXHashTablePrototype NXStrStructKeyPrototype = {
#if defined(BUG67896)
    _hashStrStructKey, _isEqualStrStructKey, _NXReallyFree, 0
#else
    hashStrStructKey, isEqualStrStructKey, NXReallyFree, 0
#endif
    };

const NXMapTablePrototype NXPtrValueMapPrototype = {
#if defined(BUG67896)
    mapPtrHash, mapPtrIsEqual, mapNoFree, 0
#else
    _mapPtrHash, _mapPtrIsEqual, _mapNoFree, 0
#endif
};

const NXMapTablePrototype NXStrValueMapPrototype = {
#if defined(BUG67896)
    mapStrHash, mapStrIsEqual, mapNoFree, 0
#else
    _mapStrHash, _mapStrIsEqual, _mapNoFree, 0
#endif
};

const NXMapTablePrototype NXObjectMapPrototype = {
#if defined(BUG67896)
    mapObjectHash, mapObjectIsEqual, mapObjectFree, 0
#else
    _mapObjectHash, _mapObjectIsEqual, _mapObjectFree, 0
#endif
};
