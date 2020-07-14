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
 *	objc-kernel.m
 * 	Copyright 1988-1996, NeXT Software, Inc.
 *
 *	this module enables the Objective-C runtime to plug into the kernel.
 *
 *	it should only be included in "kobjc.a"
 */

#ifdef KERNEL

#import <objc/zone.h>

static void *kern_realloc(struct _NXZone *zonep, void *ptr, size_t size) 
{
	return realloc(ptr, size);
}
static void *kern_malloc(struct _NXZone *zonep, size_t size) 
{
	return malloc(size);
}
static void kern_free(struct _NXZone *zonep, void *ptr) 
{
	if (ptr)
		free(ptr);
}
static void kern_destroy(struct _NXZone *zonep) 
{
}

NXZone	KernelZone = {
	kern_realloc,
	kern_malloc,
	kern_free,
	kern_destroy,
};

NXZone *NXDefaultMallocZone()
{
	return &KernelZone; 
}

NXZone *NXZoneFromPtr(void *ptr)
{
	return(&KernelZone);
}


NXZone *NXCreateZone(size_t startsize, size_t granularity, int canfree)
{
	return(&KernelZone);
}

void NXNameZone(NXZone *z, const char *name)
{
}

void           *
NXZoneCalloc(NXZone *zonep, size_t numElems, size_t byteSize)
{
	return calloc(numElems, byteSize);
}

#endif KERNEL
