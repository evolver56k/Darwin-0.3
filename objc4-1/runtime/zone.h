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

#ifndef _OBJC_ZONE_H_
#define _OBJC_ZONE_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(STRICT_OPENSTEP)

#import <stddef.h>
#import <objc/objc-api.h>	// for OBJC_EXPORT

/*
 * Interface to zone based malloc.  On non-Mach platforms the system's
 * malloc()/free() interface is used, and NXZones are not implemented.
 */

typedef struct _NXZone {
    void *(*realloc)(struct _NXZone *zonep, void *ptr, size_t size);
    void *(*malloc)(struct _NXZone *zonep, size_t size);
    void (*free)(struct _NXZone *zonep, void *ptr);
    void (*destroy)(struct _NXZone *zonep);
      	/* Implementation specific entries */
      	/* Do not depend on the size of this structure */
} NXZone;

#define NX_NOZONE  ((NXZone *)0)

OBJC_EXPORT NXZone *NXDefaultMallocZone(void);

/*
 *  Creates a zone.  Always returns default zone on non-Mach platforms.
 */
OBJC_EXPORT NXZone *NXCreateZone(size_t startSize, size_t granularity, int canFree);

/*
 *  Creates a child zone.  Always returns default zone on non-Mach platforms.
 */
OBJC_EXPORT NXZone  *NXCreateChildZone(NXZone *parentZone, size_t startSize, size_t granularity, int canFree);

/*
 *  Merges child zone back into parent zone.  Does nothing on non-Mach platforms.
 */
OBJC_EXPORT void NXMergeZone(NXZone *zonep);

#if defined(__MACH__)
#define NXDestroyZone(zonep) ((*(zonep)->destroy)(zonep))
#define NXZoneMalloc(zonep, size) ((*(zonep)->malloc)(zonep, size))
#define NXZoneRealloc(zonep, ptr, size) ((*(zonep)->realloc)(zonep, ptr, size))
#define NXZoneFree(zonep, ptr) ((*(zonep)->free)(zonep, ptr))
#else
#define NXDestroyZone(zonep)
#define NXZoneMalloc(zonep, size) malloc(size)
#define NXZoneRealloc(zonep, ptr, size) realloc(ptr, size)
#define NXZoneFree(zonep, ptr) free(ptr)
#endif

/*
 *  Zone version of calloc().  Just calls calloc() on non-Mach platforms.
 */
OBJC_EXPORT void *NXZoneCalloc(NXZone *zonep, size_t numElems, size_t byteSize);

/*
 *  Returns zone for pointer.  Always returns default zone on non-Mach platforms.
 */
OBJC_EXPORT NXZone *NXZoneFromPtr(void *ptr);

/*
 *  Debug helpers.  They do nothing on non-Mach platforms.
 */
OBJC_EXPORT void NXZonePtrInfo(void *ptr);
OBJC_EXPORT int NXMallocCheck(void);
OBJC_EXPORT void NXNameZone(NXZone *z, const char *name);

#endif	/* !STRICT_OPENSTEP */

#ifdef __cplusplus
}
#endif

#endif /* _OBJC_ZONE_H_ */
