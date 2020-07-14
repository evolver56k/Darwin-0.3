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
/* Copyright (c) 1992, 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOFrameBufferShared.h - Definitions of objects and types shared between
 *   kernel level IOFrameBufferDisplay driver and PostScript level driver.
 *
 * HISTORY
 * 03 Sep 92	Joe Pasqua
 *      Created. 
 * 24 Jun 93	Derek B Clegg
 * 	Moved to driverkit.
 */

#import <mach/boolean.h>
#import <bsd/dev/ev_types.h>

#if !defined(__Bounds__) && !defined(BINTREE_H)
#define __Bounds__
typedef struct { short minx, maxx, miny, maxy; } Bounds;
#endif

#if !defined(__Point__) && !defined(BINTREE_H)
#define __Point__
typedef struct { short x, y; } Point;
#endif

//
// Cursor and Window Server state data, occupying a slice of shared memory
// between the kernel and WindowServer.
//

#define CURSORWIDTH  16         /* width in pixels */
#define CURSORHEIGHT 16         /* height in pixels */

struct bm12Cursor {
    unsigned int image[4][16];
    unsigned int mask[4][16];
    unsigned int save[16];
};

struct bm18Cursor {
    unsigned char image[4][256];
    unsigned char mask[4][256];
    unsigned char save[256];
};

struct bm34Cursor {
    unsigned short image[4][256];
    unsigned short save[256];
};

struct bm38Cursor {
    unsigned int image[4][256];
    unsigned int save[256];
};

typedef volatile struct {
    int frame;
    ev_lock_data_t cursorSema;	
    char cursorShow;
    char cursorObscured;
    char shieldFlag;
    char shielded;
    Bounds saveRect;
    Bounds shieldRect;
    Point cursorLoc;
    Bounds cursorRect;
    Bounds oldCursorRect;
    Bounds screenBounds;
    Point hotSpot[4];
    union {
	struct bm12Cursor bw;
	struct bm18Cursor bw8;
	struct bm34Cursor rgb;
	struct bm38Cursor rgb24;
    } cursor;
} StdFBShmem_t;

//
// Get/Set parameters for the StdFB device.  These include support
// for getting the frame buffer parameters, registering the frame buffer
// with the event system, and returning the registration token.
//

/* Valid parameters for getParameterInt */

#define STDFB_FB_DIMENSIONS	"IO_Framebuffer_Dimensions"
typedef enum {
	STDFB_FB_WIDTH,
	STDFB_FB_HEIGHT,
	STDFB_FB_ROWBYTES,
	STDFB_FB_BITS_PER_PIXEL,
	STDFB_FB_FLAGS
} kmFbDimensionIndices;
#define STDFB_FB_DIMENSIONS_SIZE	(STDFB_FB_FLAGS + 1)

#define STDFB_FB_REGISTER	"IO_Framebuffer_Register"
#define STDFB_FB_REGISTER_SIZE	1

/* Valid parameters for setParameterInt */
#define STDFB_FB_UNREGISTER	"IO_Framebuffer_Unregister"
#define STDFB_FB_UNREGISTER_SIZE	1

#define STDFB_FB_MAP		"IO_Framebuffer_Map"
#define STDFB_FB_MAP_SIZE	1

#define STDFB_FB_UNMAP		"IO_Framebuffer_Unmap"
#define STDFB_FB_UNMAP_SIZE	1

#define STDFB_BM256_TO_BM38_MAP		"IO_BM256_to_BM38_map"
#define STDFB_BM256_TO_BM38_MAP_SIZE	256

#define STDFB_BM38_TO_BM256_MAP		"IO_BM38_to_BM256_map"
#define STDFB_BM38_TO_BM256_MAP_SIZE	256

/* Valid parameters for setParameterChar */
#define STDFB_4BPS_TO_5BPS_MAP		"IO_4BPS_to_5BPS_map"
#define STDFB_4BPS_TO_5BPS_MAP_SIZE	16

#define STDFB_5BPS_TO_4BPS_MAP		"IO_5BPS_to_4BPS_map"
#define STDFB_5BPS_TO_4BPS_MAP_SIZE	32

#define STDFB_FB_PIXEL_ENCODING		"IO_Framebuffer_Pixel_Encoding"
#define STDFB_FB_PIXEL_ENCODING_SIZE	64
