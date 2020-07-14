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
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOVGAShared.h - Definitions of objects and types shared between kernel
 *		   level VGA driver and PostScript level driver.
 *
 * HISTORY
 * 28 Sep 92	Gary Crum
 *      Created. 
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

typedef struct {
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
} VGAShmem_t;


//
// Get/Set parameters for the VGA device.  These include support
// for getting the frame buffer parameters, registering the frame buffer
// with the event system, and returning the registration token.
//
/* Valid parameters for getParameterInt */
#define VGA_FB_DIMENSIONS	"IO_Framebuffer_Dimensions"
typedef enum {
	VGA_FB_WIDTH,
	VGA_FB_HEIGHT,
	VGA_FB_ROWBYTES,
} kmFbDimensionIndices;
#define VGA_FB_DIMENSIONS_SIZE	(VGA_FB_ROWBYTES + 1)

#define VGA_FB_REGISTER	"IO_Framebuffer_Register"
#define VGA_FB_REGISTER_SIZE	1

/* Valid parameters for setParameterInt */
#define VGA_FB_UNREGISTER	"IO_Framebuffer_Unregister"
#define VGA_FB_UNREGISTER_SIZE	1

#define VGA_FB_SETDIMENSIONS	"IO_Framebuffer_SetDimensions"
#define VGA_FB_SETDIMENSIONS_SIZE (VGA_FB_ROWBYTES + 1)

#define VGA_FB_MAP		"IO_Framebuffer_Map"
#define VGA_FB_MAP_SIZE	1

#define VGA_FB_UNMAP		"IO_Framebuffer_Unmap"
#define VGA_FB_UNMAP_SIZE	1
