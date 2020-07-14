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

#if defined(WIN32) || defined(NeXT_PDO)

#import <objc/objc.h>
#import <malloc.h>
#import <objc/zone.h>

#if defined(WIN32)

#if defined(free)
    #undef free
#endif

#if defined(calloc)
    #undef calloc
#endif

#if defined(malloc)
    #undef malloc
#endif

#if defined(realloc)
    #undef realloc
#endif

#endif WIN32

void set_malloc_singlethreaded(int singlethreaded) {
}

void malloc_singlethreaded() {
}

void malloc_error(void (*malloc_error_func)(int code)) {
}

size_t malloc_size(void *ptr) {
    return 0;
}

void NXAddRegion(int start, int size, NXZone *zonep) {
}

void *NXZoneCalloc(NXZone *zone, size_t num, size_t size) {
	return calloc(num, size);
}

OBJC_EXPORT void *pdo_malloc(size_t size) {
	return malloc(size);
}

OBJC_EXPORT void *pdo_realloc(void *ptr, size_t size) {
	return realloc(ptr, size);
}

OBJC_EXPORT void *pdo_calloc(size_t num, size_t size) {
	return calloc(num, size);
}

OBJC_EXPORT void pdo_free(void* ptr) {
	free(ptr);
}

void *mymalloc(NXZone *zone, size_t size) {
	return malloc(size);
}

void *myrealloc(NXZone *zone, void *ptr, size_t size) {
	return realloc(ptr, size);
}

void myfree(NXZone *zone, void *ptr) {
	free(ptr);
}

void mydestroy(NXZone *zone) {
}

static NXZone one = {
	myrealloc,
	mymalloc,
	myfree,
	mydestroy
};

NXZone *NXCreateZone(size_t startSize, size_t granularity, int canFree) {
	return &one;
}

NXZone *NXDefaultMallocZone(void) {
	return &one;
}

NXZone *NXCreateChildZone(NXZone *parent, size_t start, size_t granularity, int canFree) {
	return &one;
}

void NXMergeZone(NXZone *parent) {
}

NXZone *NXZoneFromPtr(void *ptr) {
	return &one;
}

void NXZonePtrInfo(void *ptr) {
}

int NXMallocCheck(void) {
	return 0;
}

void NXNameZone(NXZone *zone, const char *name) {
}

#endif
