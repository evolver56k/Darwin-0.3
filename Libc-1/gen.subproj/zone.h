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
#import <stddef.h>

/*
 * Interface to zone based malloc.
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

/*
 * Returns the default zone used by the malloc(3) calls.
 */
extern NXZone *NXDefaultMallocZone(void);

/* 
 * Create a new zone with its own memory pool.
 * If canfree is 0 the allocator will never free memory and mallocing will be fast
 */
extern NXZone *NXCreateZone(size_t startSize, size_t granularity, int canFree);

/*
 * Create a new zone who obtains memory from another zone.
 * Returns NX_NOZONE if the passed zone is already a child.
 */
extern NXZone  *NXCreateChildZone(NXZone *parentZone, size_t startSize, size_t granularity, int canFree);

/*
 * The zone is destroyed and all memory reclaimed.
 */
#define NXDestroyZone(zonep) \
	((*(zonep)->destroy)(zonep))
	
/*
 * Will merge zone with the parent zone. Malloced areas are still valid.
 * Must be an child zone.
 */
extern void NXMergeZone(NXZone *zonep);

#define NXZoneMalloc(zonep, size) \
	((*(zonep)->malloc)(zonep, size))

#define NXZoneRealloc(zonep, ptr, size) \
	((*(zonep)->realloc)(zonep, ptr, size))
	
#define NXZoneFree(zonep, ptr) \
	((*(zonep)->free)(zonep, ptr))

/*
 * Calls NXZoneMalloc and then bzero.
 */
extern void *NXZoneCalloc(NXZone *zonep, size_t numElems, size_t byteSize);

/*
 * Returns the zone for a pointer.
 * NX_NOZONE if not in any zone.
 * The ptr must have been returned from a malloc or realloc call.
 */
extern NXZone *NXZoneFromPtr(void *ptr);

/*
 * Debugging Helpers.
 */
 
 /*  
  * Will print to stdout if this pointer is in the malloc heap, free status, and size.
  */
extern void NXZonePtrInfo(void *ptr);

/*
 * Will verify all internal malloc information.
 * This is what malloc_debug calls.
 */
extern int NXMallocCheck(void);

/*
 * Give a zone a name.
 *
 * The string will be copied.
 */
extern void NXNameZone(NXZone *z, const char *name);
