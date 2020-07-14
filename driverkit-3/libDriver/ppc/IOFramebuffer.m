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
/* 	Copyright (c) 1992-96 NeXT Software, Inc.  All rights reserved. 
 *
 * IOFramebuffer.m - Implements common methods for "standard" frame
 *				buffers.
 *
 *
 * HISTORY
 * 26 Oct 95	Dean Reece
 *	Moved strtol() out of this class into its own file (strtol.c)
 * 24 Oct 95	Rakesh Dubey
 *      Added mode change feature. 
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 */

#define KERNEL_PRIVATE 1
#define DRIVER_PRIVATE 1

/* Notes:
 *   The definition of a standard frame buffer is anything with direct
 *   32 bit access to a 2 & 8 bit gray, 8 & 16 & 32 bit color. For 16 bit
 *   displays we support both 4/4/4 and 5/5/5 samples per channel. 
 *
 *   This class implements the evScreen protocol for all StdFBDisplays. Well,
 *   almost. The setBrightness method doesn't do anything. Subclasses must
 *   override this and implement it as appropriate for the device.
 *
 *   To find things that need to be fixed, search for FIX, to find questions
 *   to be resolved, search for ASK, to find stuff that still needs to be
 *   done, search for TO DO.
 */

#import <string.h>
#import <stdlib.h>
#ifdef i386
#import <bsd/dev/i386/FBConsole.h>
#endif

#ifdef ppc
#import <bsd/dev/ppc/FBConsole.h>
#import <driverkit/ppc/IOSmartDisplay.h>
#import	<driverkit/ppc/IOFramebuffer.h>
#import	<driverkit/ppc/IOTreeDevice.h>		// for IOApertureInfo
#endif

#import	<driverkit/EventDriver.h>
#import	<bsd/dev/evio.h>

#ifdef i386
#import	<bsd/dev/i386/kmDevice.h>
#endif
#ifdef ppc
#import	<bsd/dev/ppc/kmDevice.h>
#endif

#import <driverkit/KernBus.h>
#import <driverkit/KernBusMemory.h>
#import <driverkit/IODisplayPrivate.h>
#import	<driverkit/IOFrameBufferShared.h>
#import <driverkit/IODirectDevicePrivate.h>
#import <driverkit/displayDefs.h>
#ifdef i386
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/driverTypes.h>
#endif
#ifdef ppc
#import <driverkit/ppc/directDevice.h>
#import <driverkit/ppc/driverTypes.h>
#endif

#define CLEARSEMA(shmem)	ev_unlock(&shmem->cursorSema)
#define SETSEMA(shmem)		\
	if (!ev_try_lock(&shmem->cursorSema)) return self;
#define TOUCHBOUNDS(one, two) \
	(((one.minx < two.maxx) && (two.minx < one.maxx)) && \
	((one.miny < two.maxy) && (two.miny < one.maxy)))

#define RBMASK	0xF0F0		/* Short, or 16 bit format */
#define GAMASK	0x0F0F		/* Short, or 16 bit format */
#define AMASK	0x000F		/* Short, or 16 bit format */

#define	GetShmem(instance)	((StdFBShmem_t *)(instance->priv))

extern long int strtol(const char *nptr, char **endptr, int base);

#ifndef IO_DISPLAY_MODE_SAFE
#define IO_DISPLAY_MODE_SAFE		0x00000001
#endif
#ifndef IO_DISPLAY_MODE_DEFAULT
#define IO_DISPLAY_MODE_DEFAULT		0x00000002
#endif

static BOOL	wsStartup;
static Class	gSmartDisplayClass;

@implementation IOFramebuffer


//
// BEGIN:	Generic utility routines and methods
//
- (IOReturn)_registerWithED
// Description:	Register this display device with the event driver.
{
    int token;
    int shmem_size;
    Bounds bounds;
    StdFBShmem_t *shmem;
    
    token = [[EventDriver instance] registerScreen:self
	bounds:&bounds
	shmem:&(self->priv)
	size:&shmem_size];
    shmem = GetShmem(self);
    if ( token == -1 )
	return IO_R_INVALID_ARG;
    // We allow the shmem_size to be less than sizeof(StdFBShmem_t)
    // so that we need not consume the extra space that some of the
    // larger cursor variants require if we are really a lower bitdepth.
    if ( shmem_size > sizeof(StdFBShmem_t) )
    {
	IOLog("%s: shmem_size > sizeof (StdFBShmem_t)(%d<>%d)\n",
	    [self name], shmem_size, sizeof (StdFBShmem_t));
	[[EventDriver instance]	unregisterScreen:token];
	return IO_R_INVALID_ARG;
    }
    // Init shared memory area
    memset((char *)shmem, 0, shmem_size);
    shmem->cursorShow = 1;
    shmem->screenBounds = bounds;
    [self setToken:token];
    cursorShmemBitsPerPixel = [self displayInfo]->bitsPerPixel;
    cursorBitsPerPixel = cursorShmemBitsPerPixel;
    return IO_R_SUCCESS;
}

#define short34to35WithGamma(x) \
	(  (_bm34To35SampleTable[((x) & 0x00F0) >> 4])		\
	 | (_bm34To35SampleTable[((x) & 0x0F00) >> 8] << 5)	\
	 | (_bm34To35SampleTable[(x) >> 12] << 10) )

#define short35to34WithGamma(x) \
	(  0x000F						\
	 | (_bm35To34SampleTable[x & 0x001F] << 4)		\
	 | (_bm35To34SampleTable[(x & 0x03E0) >> 5] << 8)	\
	 | (_bm35To34SampleTable[(x & 0x7C00) >> 10] << 12) )

static void StdFBDisplayCursor16(IOFramebuffer *inst)
// Description: Displays the cursor on the framebuffer by first saving
//		what's underneath the cursor, then drawing the cursor there.
//		NOTE:The topleft of the cursorRect passed in is not
//		necessarily the cursor location.The cursorRect is adjusted to
//		compensate for the cursor hotspot.If the frame buffer is
//		cacheable, flush at the end of the drawing operation. A
//		saveRect is stored which defines the actual area of the screen
//		that is saved.This is used by RemoveCursor in restoring the
//		screen data later.
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    vm_offset_t startPtr;		/* Starting screen data pointer */
    unsigned int vramRow;
    Bounds saveRect;
    short i, j, width, cursRow;
    unsigned short *vramPtr;	/* screen data pointer */
    volatile unsigned short *savePtr;	/* saved screen data pointer */
    unsigned short s, d, f;
    volatile unsigned short *cursPtr;
    unsigned char *_bm34To35SampleTable;
    unsigned char *_bm35To34SampleTable;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->cursorRect;
    /* Clip saveRect vertical within screen bounds */
    if (saveRect.miny < shmem->screenBounds.miny)
	saveRect.miny = shmem->screenBounds.miny;
    if (saveRect.maxy > shmem->screenBounds.maxy)
	saveRect.maxy = shmem->screenBounds.maxy;
    if (saveRect.minx < shmem->screenBounds.minx)
	saveRect.minx = shmem->screenBounds.minx;
    if (saveRect.maxx > shmem->screenBounds.maxx)
	saveRect.maxx = shmem->screenBounds.maxx;
    shmem->saveRect = saveRect; /* Remember save rect for RemoveCursor */

    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned short *)dpy->frameBuffer + 
        (vramRow * (saveRect.miny - shmem->screenBounds.miny)) +
	(saveRect.minx - shmem->screenBounds.minx);
    startPtr = (vm_offset_t) vramPtr;
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    
    cursRow = CURSORWIDTH - width;
    savePtr = shmem->cursor.rgb.save;
    cursPtr = shmem->cursor.rgb.image[shmem->frame];
    cursPtr += (saveRect.miny - shmem->cursorRect.miny) * CURSORWIDTH +
		(saveRect.minx - shmem->cursorRect.minx);

    if (dpy->bitsPerPixel == IO_12BitsPerPixel)
    {
	for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	    for (j = width; --j != -1; ) {
		d = *savePtr++ = *vramPtr;
		if ( (s = *cursPtr++) == 0 )
		{	/* Transparent black area.  Leave dst as is. */
		    ++vramPtr;
		    continue;
		}
		if ( (f = (~s) & (unsigned int)AMASK) == 0 )
		{	/* Opaque cursor pixel.  Mark it. */
		    *vramPtr++ = s;
		    continue;
		}
		/* Alpha is not 0 or 1.0.  Sover the cursor. */
		*vramPtr++ = s + (((((d & RBMASK)>>4)*f + GAMASK) & RBMASK)
			     | ((((d & GAMASK)*f+GAMASK)>>4) & GAMASK));
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    }
    else	// dpy->bitsPerPixel == IO_15BitsPerPixel
    {
    	// These tables should always be set by the Window Server before
	// it enables cursor drawing.  If they're not set, it's an error.
        if (   (_bm34To35SampleTable = inst->_bm34To35SampleTable) == NULL
            || (_bm35To34SampleTable = inst->_bm35To34SampleTable) == NULL )
		return;
	for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	    for (j = width; --j>=0; ) {
		d = *savePtr++ = *vramPtr;
		if ( (s = *cursPtr++) == 0 )
		{	/* Transparent black area.  Leave dst as is. */
		    ++vramPtr;
		    continue;
		}
		if ( (f = (~s) & (unsigned int)AMASK) == 0 )
		{	/* Opaque cursor pixel.  Mark it. */
		    *vramPtr++ = short34to35WithGamma(s);
		    continue;
		}
		/* Alpha is not 0 or 1.0.  Sover the cursor. */
		d = short35to34WithGamma(d);
		d = s + (((((d & RBMASK)>>4)*f + GAMASK) & RBMASK)
		    | ((((d & GAMASK)*f+GAMASK)>>4) & GAMASK));
		*vramPtr++ = short34to35WithGamma(d);
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    }
}

static inline unsigned int MUL32(unsigned int a, unsigned int b)
{
    unsigned int v, w;

    v  = ((a & 0xff00ff00) >> 8) * b;
    v += ((v & 0xff00ff00) >> 8) + 0x00010001;
    w  = (a & 0x00ff00ff) * b;
    w += ((w & 0xff00ff00) >> 8) + 0x00010001;

    return (v & 0xff00ff00) | ((w >> 8) & 0x00ff00ff);
}

static inline unsigned char map32to256( unsigned char *directToLogical, unsigned int s)
{
    unsigned char logicalValue;

    if ((s ^ (s>>8)) & 0x00ffff00) {
	logicalValue =  directToLogical[(s>>24)        + 0] +
			directToLogical[((s>>16)&0xff) + 256] +
			directToLogical[((s>>8)&0xff)  + 512];
    } else {
	logicalValue =  directToLogical[(s>>24)        + 768];
    }
    return( directToLogical[ logicalValue + 1024 ]);		// final conversion from NeXT palette
								// to actual palette
}

static void StdFBDisplayCursor8(IOFramebuffer *inst)
// Description: Displays the cursor on the framebuffer by first saving
//		what's underneath the cursor, then drawing the cursor there.
//		NOTE:The topleft of the cursorRect passed in is not
//		necessarily the cursor location.The cursorRect is adjusted to
//		compensate for the cursor hotspot.If the frame buffer is
//		cacheable, flush at the end of the drawing operation. A
//		saveRect is stored which defines the actual area of the screen
//		that is saved.This is used by RemoveCursor in restoring the
//		screen data later.
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    vm_offset_t startPtr;		/* Starting screen data pointer */
    unsigned int vramRow;
    Bounds saveRect;
    short i, j, width, cursRow;
    unsigned char *vramPtr;	/* screen data pointer */
    volatile unsigned char *savePtr;	/* saved screen data pointer */
    unsigned short s, d;
    unsigned char dst, alpha;
    unsigned int rgb32val;
    volatile unsigned char *cursPtr;
    volatile unsigned char *maskPtr;		/* cursor mask pointer */
    unsigned int *_bm256To38SampleTable = inst->_bm256To38SampleTable;
    unsigned char *_bm38To256SampleTable = inst->_bm38To256SampleTable;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->cursorRect;
    /* Clip saveRect vertical within screen bounds */
    if (saveRect.miny < shmem->screenBounds.miny)
	saveRect.miny = shmem->screenBounds.miny;
    if (saveRect.maxy > shmem->screenBounds.maxy)
	saveRect.maxy = shmem->screenBounds.maxy;
    if (saveRect.minx < shmem->screenBounds.minx)
	saveRect.minx = shmem->screenBounds.minx;
    if (saveRect.maxx > shmem->screenBounds.maxx)
	saveRect.maxx = shmem->screenBounds.maxx;
    shmem->saveRect = saveRect; /* Remember save rect for RemoveCursor */

    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned char *)dpy->frameBuffer + 
        (vramRow * (saveRect.miny - shmem->screenBounds.miny)) +
	(saveRect.minx - shmem->screenBounds.minx);
    startPtr = (vm_offset_t) vramPtr;
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    
    cursRow = CURSORWIDTH - width;
    savePtr = shmem->cursor.bw8.save;
    cursPtr = shmem->cursor.bw8.image[shmem->frame];
    maskPtr = shmem->cursor.bw8.mask[shmem->frame];
    i = (saveRect.miny - shmem->cursorRect.miny) * CURSORWIDTH +
	(saveRect.minx - shmem->cursorRect.minx);
    cursPtr += i;
    maskPtr += i;

    if (dpy->colorSpace == IO_OneIsWhiteColorSpace) {
	for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	    for (j = width; --j>=0; ) {
		int t;
		d = *savePtr++ = *vramPtr;
		s = *cursPtr++;
		t = d * (255 - *maskPtr++);
		d = s + ((t + (t >> 8) + 1) >> 8);
		*vramPtr++ = d;
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    maskPtr += cursRow;
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    } else { // dpy->colorSpace == IO_RGBColorSpace 
	for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	    for (j = width; --j>=0; savePtr++,maskPtr++,cursPtr++,vramPtr++) {
		*savePtr = *vramPtr;
		if (alpha = *maskPtr) {
		    dst = *cursPtr;
		    if (alpha = ~alpha) {
			rgb32val = _bm256To38SampleTable[*vramPtr];
			rgb32val = (_bm256To38SampleTable[dst] & ~0xff) +
				   MUL32(rgb32val, alpha);
			dst = map32to256(_bm38To256SampleTable, rgb32val);
		    }
		    *vramPtr = dst;
		}
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    maskPtr += cursRow;
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    }
}

static void StdFBRemoveCursor16(IOFramebuffer *inst)
// Description:	RemoveCursor erases the cursor by replacing the background
//		image that was saved by the previous call to DisplayCursor.
//		If the frame buffer is cacheable, flush at the end of the
//		drawing operation.
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    short i, j, width;
    vm_offset_t startPtr;
    unsigned int vramRow;
    Bounds saveRect;
    unsigned short *vramPtr;
    volatile unsigned short *savePtr;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->saveRect;
    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned short *)dpy->frameBuffer +
        (vramRow * (saveRect.miny - shmem->screenBounds.miny))
	+ (saveRect.minx - shmem->screenBounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    savePtr = shmem->cursor.rgb.save;
    startPtr = (vm_offset_t) vramPtr;
    for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	for (j = width; --j != -1; )
	    *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}

static void StdFBRemoveCursor8(IOFramebuffer *inst)
// Description:	RemoveCursor erases the cursor by replacing the background
//		image that was saved by the previous call to DisplayCursor.
//		If the frame buffer is cacheable, flush at the end of the
//		drawing operation.
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    short i, j, width;
    vm_offset_t startPtr;
    unsigned int vramRow;
    Bounds saveRect;
    unsigned char *vramPtr;
    volatile unsigned char *savePtr;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->saveRect;
    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned char *)dpy->frameBuffer +
        (vramRow * (saveRect.miny - shmem->screenBounds.miny))
	+ (saveRect.minx - shmem->screenBounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    savePtr = shmem->cursor.bw8.save;
    startPtr = (vm_offset_t) vramPtr;
    for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	for (j = width; --j != -1; )
	    *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}

static void StdFBDisplayCursor32(IOFramebuffer *inst)
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    unsigned int vramRow;
    Bounds saveRect;
    int i, j, width, cursRow;
    unsigned int *vramPtr;	/* screen data pointer */
    volatile unsigned int *savePtr;	/* saved screen data pointer */
    unsigned int s, d, f;
    volatile unsigned int *cursPtr;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->cursorRect;
    /* Clip saveRect vertical within screen bounds */
    if (saveRect.miny < shmem->screenBounds.miny)
	saveRect.miny = shmem->screenBounds.miny;
    if (saveRect.maxy > shmem->screenBounds.maxy)
	saveRect.maxy = shmem->screenBounds.maxy;
    if (saveRect.minx < shmem->screenBounds.minx)
	saveRect.minx = shmem->screenBounds.minx;
    if (saveRect.maxx > shmem->screenBounds.maxx)
	saveRect.maxx = shmem->screenBounds.maxx;
    shmem->saveRect = saveRect; /* Remember save rect for RemoveCursor */

    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned int *)dpy->frameBuffer + 
        (vramRow * (saveRect.miny - shmem->screenBounds.miny)) +
	(saveRect.minx - shmem->screenBounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    
    cursRow = CURSORWIDTH - width;
    savePtr = shmem->cursor.rgb24.save;
    cursPtr = shmem->cursor.rgb24.image[shmem->frame];
    cursPtr += (saveRect.miny - shmem->cursorRect.miny) * CURSORWIDTH +
		(saveRect.minx - shmem->cursorRect.minx);

    if (dpy->pixelEncoding[0] == IO_SampleTypeAlpha ||
	dpy->pixelEncoding[0] == IO_SampleTypeSkip) {
	/* Pixel format is Axxx */
	for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	    for (j = width; --j != -1; ) {
		d = *savePtr++ = *vramPtr;
		s = *cursPtr++;
		f = s >> 24;
		if (f) {
		    if (f == 0xff)		// Opaque pixel
			*vramPtr++ = s;
		    else {			// SOVER the cursor pixel
			s <<= 8;  d <<= 8;   /* Now pixels are xxxA */
			f ^= 0xFF;
			d = s+(((((d&0xFF00FF00)>>8)*f+0x00FF00FF)&0xFF00FF00)
			    | ((((d & 0x00FF00FF)*f+0x00FF00FF)>>8) &
				0x00FF00FF));
			*vramPtr++ = (d>>8) | 0xff000000;
		    }
		} else			// Transparent cursor pixel
		    vramPtr++;
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    } else {
	/* Pixel format is xxxA */
	for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	    for (j = width; --j != -1; ) {
		d = *savePtr++ = *vramPtr;
		s = *cursPtr++;
		f = s & (unsigned int)0xFF;
		if (f) {
		    if (f == 0xff)		// Opaque pixel
			*vramPtr++ = s;
		    else {			// SOVER the cursor pixel
			f ^= 0xFF;
			d = s+(((((d&0xFF00FF00)>>8)*f+0x00FF00FF)&0xFF00FF00)
			    | ((((d & 0x00FF00FF)*f+0x00FF00FF)>>8) &
				0x00FF00FF));
			*vramPtr++ = d;
		    }
		} else			// Transparent cursor pixel
		    vramPtr++;
	    }
	    cursPtr += cursRow; /* starting point of next cursor line */
	    vramPtr += vramRow; /* starting point of next screen line */
	}
    }
}

static void StdFBRemoveCursor32(IOFramebuffer *inst)
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    int i, j, width;
    unsigned int vramRow;
    Bounds saveRect;
    unsigned int *vramPtr;
    unsigned int *savePtr;

    dpy = [inst displayInfo];
    shmem = GetShmem(inst);
    saveRect = shmem->saveRect;
    vramRow = dpy->totalWidth;	/* Scanline width in pixels */
    vramPtr = (unsigned int *)dpy->frameBuffer +
        (vramRow * (saveRect.miny - shmem->screenBounds.miny))
	+ (saveRect.minx - shmem->screenBounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    savePtr = (unsigned int *)shmem->cursor.rgb24.save;
    for (i = saveRect.maxy - saveRect.miny; --i != -1; ) {
	for (j = width; --j != -1; )
	    *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}

static inline void StdFBDisplayCursor(IOFramebuffer *inst) 
{
    switch (inst->cursorBitsPerPixel) {
    	default:
	case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    StdFBDisplayCursor16(inst);
	    break;
	case IO_8BitsPerPixel:
	    StdFBDisplayCursor8(inst);
	    break;
	case IO_24BitsPerPixel:
	    StdFBDisplayCursor32(inst);
	case (-1):
	    break;
    }
}

static inline void StdFBRemoveCursor(IOFramebuffer *inst) 
{
    switch (inst->cursorBitsPerPixel) {
    	default:
	case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    StdFBRemoveCursor16(inst);
	    break;
	case IO_8BitsPerPixel:
	    StdFBRemoveCursor8(inst);
	    break;
	case IO_24BitsPerPixel:
	    StdFBRemoveCursor32(inst);
	case (-1):
	    break;
    }
}


#define RemoveCursor(inst)	StdFBRemoveCursor(inst)
static inline void DisplayCursor(IOFramebuffer *inst)
{
    Point hs;
    StdFBShmem_t *shmem;

    shmem = GetShmem(inst);
    hs = shmem->hotSpot[shmem->frame];
    shmem->cursorRect.maxx =
        (shmem->cursorRect.minx = (shmem->cursorLoc).x - hs.x) + 16;
    shmem->cursorRect.maxy =
        (shmem->cursorRect.miny = (shmem->cursorLoc).y - hs.y) + 16;
    StdFBDisplayCursor(inst);
    shmem->oldCursorRect = shmem->cursorRect;
}

static inline void SysHideCursor(IOFramebuffer *inst)
{
    if (!GetShmem(inst)->cursorShow++)
	RemoveCursor(inst);
}

static inline void SysShowCursor(IOFramebuffer *inst)
{
    if (GetShmem(inst)->cursorShow)
	if (!--(GetShmem(inst)->cursorShow))
	    DisplayCursor(inst);
}

static inline void CheckShield(IOFramebuffer *inst)
{
    Point hs;
    int intersect;
    Bounds tempRect;
    StdFBShmem_t *shmem;
    
    shmem = GetShmem(inst);
    /* Calculate temp cursorRect */
    hs = shmem->hotSpot[shmem->frame];
    tempRect.maxx = (tempRect.minx = (shmem->cursorLoc).x - hs.x) + 16;
    tempRect.maxy = (tempRect.miny = (shmem->cursorLoc).y - hs.y) + 16;

    intersect = TOUCHBOUNDS(tempRect, shmem->shieldRect);
    if (intersect != shmem->shielded)
	(shmem->shielded = intersect) ?
	    SysHideCursor(inst) : SysShowCursor(inst);
}
//
// END:		Generic utility routines
//


//
// BEGIN:	Implementation of the evScreen protocol
//
- hideCursor: (int)token
{
    SETSEMA(GetShmem(self));
    SysHideCursor(self);
    CLEARSEMA(GetShmem(self));
    return self;
}

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t
{
    StdFBShmem_t *shmem;
    
    shmem = GetShmem(self);
    SETSEMA(shmem);
    shmem->frame = frame;
    shmem->cursorLoc = *cursorLoc;
    if (!shmem->cursorShow++)
	RemoveCursor(self);
    if (shmem->cursorObscured) {
	shmem->cursorObscured = 0;
	if (shmem->cursorShow)
	    --shmem->cursorShow;
    }
    if (shmem->shieldFlag) CheckShield(self);
    if (shmem->cursorShow)
	if (!--shmem->cursorShow)
	    DisplayCursor(self);
    CLEARSEMA(shmem);
    return self;
}

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t
{
    StdFBShmem_t *shmem;
    
    shmem = GetShmem(self);
    SETSEMA(GetShmem(self));
    shmem->frame = frame;
    shmem->cursorLoc = *cursorLoc;
    if (shmem->shieldFlag) CheckShield(self);
    SysShowCursor(self);
    CLEARSEMA(shmem);
    return self;
}

- setBrightness:(int)level token:(int)t
{
    if ( level < EV_SCREEN_MIN_BRIGHTNESS
	|| level > EV_SCREEN_MAX_BRIGHTNESS )
    {
	IOLog("%s: Invalid arg to setBrightness:%d\n",
	    [self name], level );
    }
    return self;
}
//
// END:		Implementation of the evScreen protocol
//
//
// BEGIN:	EXPORTED Methods
//

- initFromDeviceDescription:deviceDescription
{
    extern int sprintf(char *s, const char *format, ...);
    static int nextUnit = 0;
    char nameBuf[20];

    if ([super initFromDeviceDescription:deviceDescription] == nil)
	return [super free];

    _pendingDisplayMode = -1;
    
    sprintf(nameBuf, "Display%d", nextUnit);
    [self setUnit: nextUnit++];
    [self setName:nameBuf];
    return self;
}

- (IOConsoleInfo *)allocateConsoleInfo;
// Description:	Allocates a console support info structure based on this
//		display. This structure, and the functions in it, are used
//		to display alert and console windows.
{
    return FBAllocateConsole([self displayInfo]);
}

static const char *
find_parameter(const char *parameter, const char *string)
{
    int c;
    size_t length;

    length = strlen(parameter);
    while (*string != 0) {
	if (strncmp(string, parameter, length) == 0) {
	    string += length;
	    while ((c = *string) != '\0' && (c == ' ' || c == '\t'))
		string++;
	    return (c != 0) ? string : 0;
	}
	string++;
    }
    return 0;
}


static UInt32
ActiveBytes( IODisplayInfo * displayInfo )
{
    UInt32	bytesPerPixel;

    switch( displayInfo->bitsPerPixel) {
	case IO_8BitsPerPixel:
	    bytesPerPixel  = 1;
	    break;
	case IO_15BitsPerPixel:
	    bytesPerPixel  = 2;
	    break;
	case IO_24BitsPerPixel:
	default:
	    bytesPerPixel  = 4;
	    break;
    }
    // The last line may not have the full rowBytes physically present,
    // only the active pixels.
    return( displayInfo->rowBytes * (displayInfo->height - 1)
	  + (displayInfo->width * bytesPerPixel));
}

- convertPixelInfoToDisplayInfo:(IOFBPixelInformation *)pixelInfo mono:(BOOL)mono
	displayInfo:(IODisplayInfo *)displayInfo
{
    IOBitsPerPixel	nextDepth;
    const char *	nextPixels;
    IOColorSpace	nextCSpace;
    static const char * apple8BitMonoEncoding = "WWWWWWWW";
    static const char * apple8BitEncoding = "PPPPPPPP";
    static const char * apple16BitEncoding = "-RRRRRGGGGGBBBBB";
    static const char *	apple32BitEncoding = "--------RRRRRRRRGGGGGGGGBBBBBBBB";

    nextCSpace = IO_RGBColorSpace;

    switch( pixelInfo->bitsPerPixel ) {
	default:
	case 8:
	    nextDepth   = IO_8BitsPerPixel;
	    if( mono) {
		nextCSpace = IO_OneIsWhiteColorSpace;
		nextPixels = apple8BitMonoEncoding;
	    } else {
		nextPixels  = apple8BitEncoding;
	    }
	    break;
	case 16:
	    nextDepth   = IO_15BitsPerPixel;
	    nextPixels  = apple16BitEncoding;
	    break;
	case 32:
	    nextDepth   = IO_24BitsPerPixel;
	    nextPixels  = apple32BitEncoding;
	    break;
    }

    bzero(displayInfo, sizeof(IODisplayInfo));

    strncpy(displayInfo->pixelEncoding, nextPixels, strlen(nextPixels));
    displayInfo->width = pixelInfo->width;
    displayInfo->height = pixelInfo->height;
    displayInfo->rowBytes = pixelInfo->rowBytes;
    displayInfo->totalWidth = (pixelInfo->rowBytes * 8) / pixelInfo->bitsPerPixel;
    displayInfo->refreshRate = (pixelInfo->refreshRate + 0x8000) >> 16;
    displayInfo->bitsPerPixel = nextDepth;
    displayInfo->colorSpace = nextCSpace;
    displayInfo->flags = IO_DISPLAY_HAS_TRANSFER_TABLE
			   | [self tempFlags];
    return( self);
}

- (IOReturn) getNextMode:(IOFBIndex *)index modeInfo:(IOFBDisplayModeInformation *)modeInfo needFlags:(UInt32)needFlags
{
    IOReturn			err;
    IOFBDisplayModeID		modeID;
    IOFBTimingInformation	timingInfo;
    UInt32			flags;

    do {
	err = [self getDisplayModeByIndex:(*index)++ displayMode:&modeID];
	if( err)
	    break;
	err = [self getDisplayModeInformation:modeID info:modeInfo];
	if( err)
	    break;

	err = [self getDisplayModeTiming:0 mode:modeID timingInfo:&timingInfo connectFlags:&flags];

    } while( (err == noErr) && ((flags & needFlags) != needFlags) );

    return( err);
}

// Apple standard 8-bit CLUT

const UInt8 appleClut8[ 256 * 3 ] = {
	0xFF,0xFF,0xFF, 0xFF,0xFF,0xCC,	0xFF,0xFF,0x99,	0xFF,0xFF,0x66,
	0xFF,0xFF,0x33, 0xFF,0xFF,0x00,	0xFF,0xCC,0xFF,	0xFF,0xCC,0xCC,
	0xFF,0xCC,0x99, 0xFF,0xCC,0x66,	0xFF,0xCC,0x33,	0xFF,0xCC,0x00,
	0xFF,0x99,0xFF, 0xFF,0x99,0xCC,	0xFF,0x99,0x99,	0xFF,0x99,0x66,
	0xFF,0x99,0x33, 0xFF,0x99,0x00,	0xFF,0x66,0xFF,	0xFF,0x66,0xCC,
	0xFF,0x66,0x99, 0xFF,0x66,0x66,	0xFF,0x66,0x33,	0xFF,0x66,0x00,
	0xFF,0x33,0xFF, 0xFF,0x33,0xCC,	0xFF,0x33,0x99,	0xFF,0x33,0x66,
	0xFF,0x33,0x33, 0xFF,0x33,0x00,	0xFF,0x00,0xFF,	0xFF,0x00,0xCC,
	0xFF,0x00,0x99, 0xFF,0x00,0x66,	0xFF,0x00,0x33,	0xFF,0x00,0x00,
	0xCC,0xFF,0xFF, 0xCC,0xFF,0xCC,	0xCC,0xFF,0x99,	0xCC,0xFF,0x66,
	0xCC,0xFF,0x33, 0xCC,0xFF,0x00,	0xCC,0xCC,0xFF,	0xCC,0xCC,0xCC,
	0xCC,0xCC,0x99, 0xCC,0xCC,0x66,	0xCC,0xCC,0x33,	0xCC,0xCC,0x00,
	0xCC,0x99,0xFF, 0xCC,0x99,0xCC,	0xCC,0x99,0x99,	0xCC,0x99,0x66,
	0xCC,0x99,0x33, 0xCC,0x99,0x00,	0xCC,0x66,0xFF,	0xCC,0x66,0xCC,
	0xCC,0x66,0x99, 0xCC,0x66,0x66,	0xCC,0x66,0x33,	0xCC,0x66,0x00,
	0xCC,0x33,0xFF, 0xCC,0x33,0xCC,	0xCC,0x33,0x99,	0xCC,0x33,0x66,
	0xCC,0x33,0x33, 0xCC,0x33,0x00,	0xCC,0x00,0xFF,	0xCC,0x00,0xCC,
	0xCC,0x00,0x99, 0xCC,0x00,0x66,	0xCC,0x00,0x33,	0xCC,0x00,0x00,
	0x99,0xFF,0xFF, 0x99,0xFF,0xCC,	0x99,0xFF,0x99,	0x99,0xFF,0x66,
	0x99,0xFF,0x33, 0x99,0xFF,0x00,	0x99,0xCC,0xFF,	0x99,0xCC,0xCC,
	0x99,0xCC,0x99, 0x99,0xCC,0x66,	0x99,0xCC,0x33,	0x99,0xCC,0x00,
	0x99,0x99,0xFF, 0x99,0x99,0xCC,	0x99,0x99,0x99,	0x99,0x99,0x66,
	0x99,0x99,0x33, 0x99,0x99,0x00,	0x99,0x66,0xFF,	0x99,0x66,0xCC,
	0x99,0x66,0x99, 0x99,0x66,0x66,	0x99,0x66,0x33,	0x99,0x66,0x00,
	0x99,0x33,0xFF, 0x99,0x33,0xCC,	0x99,0x33,0x99,	0x99,0x33,0x66,
	0x99,0x33,0x33, 0x99,0x33,0x00,	0x99,0x00,0xFF,	0x99,0x00,0xCC,
	0x99,0x00,0x99, 0x99,0x00,0x66,	0x99,0x00,0x33,	0x99,0x00,0x00,
	0x66,0xFF,0xFF, 0x66,0xFF,0xCC,	0x66,0xFF,0x99,	0x66,0xFF,0x66,
	0x66,0xFF,0x33, 0x66,0xFF,0x00,	0x66,0xCC,0xFF,	0x66,0xCC,0xCC,
	0x66,0xCC,0x99, 0x66,0xCC,0x66,	0x66,0xCC,0x33,	0x66,0xCC,0x00,
	0x66,0x99,0xFF, 0x66,0x99,0xCC,	0x66,0x99,0x99,	0x66,0x99,0x66,
	0x66,0x99,0x33, 0x66,0x99,0x00,	0x66,0x66,0xFF,	0x66,0x66,0xCC,
	0x66,0x66,0x99, 0x66,0x66,0x66,	0x66,0x66,0x33,	0x66,0x66,0x00,
	0x66,0x33,0xFF, 0x66,0x33,0xCC,	0x66,0x33,0x99,	0x66,0x33,0x66,
	0x66,0x33,0x33, 0x66,0x33,0x00,	0x66,0x00,0xFF,	0x66,0x00,0xCC,
	0x66,0x00,0x99, 0x66,0x00,0x66,	0x66,0x00,0x33,	0x66,0x00,0x00,
	0x33,0xFF,0xFF, 0x33,0xFF,0xCC,	0x33,0xFF,0x99,	0x33,0xFF,0x66,
	0x33,0xFF,0x33, 0x33,0xFF,0x00,	0x33,0xCC,0xFF,	0x33,0xCC,0xCC,
	0x33,0xCC,0x99, 0x33,0xCC,0x66,	0x33,0xCC,0x33,	0x33,0xCC,0x00,
	0x33,0x99,0xFF, 0x33,0x99,0xCC,	0x33,0x99,0x99,	0x33,0x99,0x66,
	0x33,0x99,0x33, 0x33,0x99,0x00,	0x33,0x66,0xFF,	0x33,0x66,0xCC,
	0x33,0x66,0x99, 0x33,0x66,0x66,	0x33,0x66,0x33,	0x33,0x66,0x00,
	0x33,0x33,0xFF, 0x33,0x33,0xCC,	0x33,0x33,0x99,	0x33,0x33,0x66,
	0x33,0x33,0x33, 0x33,0x33,0x00,	0x33,0x00,0xFF,	0x33,0x00,0xCC,
	0x33,0x00,0x99, 0x33,0x00,0x66,	0x33,0x00,0x33,	0x33,0x00,0x00,
	0x00,0xFF,0xFF, 0x00,0xFF,0xCC,	0x00,0xFF,0x99,	0x00,0xFF,0x66,
	0x00,0xFF,0x33, 0x00,0xFF,0x00,	0x00,0xCC,0xFF,	0x00,0xCC,0xCC,
	0x00,0xCC,0x99, 0x00,0xCC,0x66,	0x00,0xCC,0x33,	0x00,0xCC,0x00,
	0x00,0x99,0xFF, 0x00,0x99,0xCC,	0x00,0x99,0x99,	0x00,0x99,0x66,
	0x00,0x99,0x33, 0x00,0x99,0x00,	0x00,0x66,0xFF,	0x00,0x66,0xCC,
	0x00,0x66,0x99, 0x00,0x66,0x66,	0x00,0x66,0x33,	0x00,0x66,0x00,
	0x00,0x33,0xFF, 0x00,0x33,0xCC,	0x00,0x33,0x99,	0x00,0x33,0x66,
	0x00,0x33,0x33, 0x00,0x33,0x00,	0x00,0x00,0xFF,	0x00,0x00,0xCC,
	0x00,0x00,0x99, 0x00,0x00,0x66,	0x00,0x00,0x33,	0xEE,0x00,0x00,
	0xDD,0x00,0x00, 0xBB,0x00,0x00,	0xAA,0x00,0x00,	0x88,0x00,0x00,
	0x77,0x00,0x00, 0x55,0x00,0x00,	0x44,0x00,0x00,	0x22,0x00,0x00,
	0x11,0x00,0x00, 0x00,0xEE,0x00,	0x00,0xDD,0x00,	0x00,0xBB,0x00,
	0x00,0xAA,0x00, 0x00,0x88,0x00,	0x00,0x77,0x00,	0x00,0x55,0x00,
	0x00,0x44,0x00, 0x00,0x22,0x00,	0x00,0x11,0x00,	0x00,0x00,0xEE,
	0x00,0x00,0xDD, 0x00,0x00,0xBB,	0x00,0x00,0xAA,	0x00,0x00,0x88,
	0x00,0x00,0x77, 0x00,0x00,0x55,	0x00,0x00,0x44,	0x00,0x00,0x22,
	0x00,0x00,0x11, 0xEE,0xEE,0xEE,	0xDD,0xDD,0xDD,	0xBB,0xBB,0xBB,
	0xAA,0xAA,0xAA, 0x88,0x88,0x88,	0x77,0x77,0x77,	0x55,0x55,0x55,
	0x44,0x44,0x44, 0x22,0x22,0x22,	0x11,0x11,0x11,	0x00,0x00,0x00
};

///////////////////// Backwards compatibility goo /////////////////////

- (IOReturn) makeConfigList
{
    IOFBDisplayModeInformation	info;
    IOFBPixelInformation	pixelInfo;
    IOFBIndex			depth;
    struct ModeData	*	modeData;
    UInt32			modeIndex;
    UInt32			preflight;
    IOReturn			err;
    enum {			kNeedFlags = (kDisplayModeValidFlag /*| kDisplayModeSafeFlag */) };

    numModes = numConfigs = 0;
    preflight = 1;
    modeData = NULL;
    do {

	if( numModes) {
	    modeData = IOMalloc( numModes * sizeof( struct ModeData));
	    modeTable = modeData;
	}
	modeIndex = 0;
	while( (noErr == [self getNextMode:&modeIndex modeInfo:&info needFlags:kNeedFlags])) {

	    if( modeData) {
		if( (modeData - modeTable) >= numModes)
		    kprintf("!!MODE LIST BAD!!\n");
		else {

		    depth = -1;
		    do {
                        err = [self getPixelInformationForDisplayMode:info.displayModeID andDepthIndex:++depth
                            pixelInfo:&pixelInfo];

			if( (err == noErr) && (pixelInfo.bitsPerPixel >= 8)) {
                            modeData->modeID = info.displayModeID;
			    modeData->firstDepth = depth;
                            modeData->numDepths = 1 + info.maxDepthIndex - depth;
                            if( pixelInfo.bitsPerPixel == 8)
                                modeData->numDepths++;			// +MONO
                            numConfigs += modeData->numDepths;
                            modeData++;
			    break;
			}
		    } while( (err == noErr) && (depth < info.maxDepthIndex));
		}
	    } else
		numModes++;
	}
    } while( preflight--);

    return( noErr);
}

- (IOReturn) getDisplayModeAndDepthForIndex:(UInt32)configIndex
	mode:(IOFBDisplayModeID *)mode depth:(IOFBIndex *)depth mono:(BOOL *)mono
{
    struct ModeData	*	modeData;
    UInt32			modeIndex;

    for( modeIndex = 0, modeData = modeTable;
	modeIndex < numModes;
	modeIndex++, modeData++) {

	if( configIndex < modeData->numDepths) {
	    *mode = modeData->modeID;
	    *mono = (configIndex == 0);
	    if( *mono)
		*depth = modeData->firstDepth;
	    else
		*depth = modeData->firstDepth + configIndex - 1;
	    return( noErr);
	}
	configIndex -= modeData->numDepths;
    }

    return( IO_R_UNDEFINED_MODE);
}

- (IOReturn) getConfigIndexForDisplayModeAndDepth:
	(IOFBDisplayModeID)mode depth:(IOFBIndex)depth mono:(BOOL)mono
        configIndex:(UInt32 *)configIndex
{
    UInt32			index;
    struct ModeData	*	modeData;
    UInt32			modeIndex;

    for( modeIndex = 0, modeData = modeTable, index = 0;
	modeIndex < numModes;
	modeIndex++, modeData++) {

	if( mode == modeData->modeID) {
	    if( depth <= modeData->firstDepth)		// 8 bpp mode
		index += (mono ? 0 : 1);
	    else
		index += depth - modeData->firstDepth + 1;
	    *configIndex = index;
	    return( noErr);
	}
	index += modeData->numDepths;
    }

    return( IO_R_UNDEFINED_MODE);
}

- (IOReturn) getCurrentConfigIndex:(UInt32 *)configIndex
{
    return( [self getConfigIndexForDisplayModeAndDepth:currentDisplayModeID
		depth:currentDepthIndex mono:currentMono
		configIndex:configIndex]);
}

- (IOReturn) getDisplayInfoForConfigIndex:(UInt32)index info:(IODisplayInfo *)displayInfo
						connectFlags:(UInt32 *)flags
{
    IOFBPixelInformation	pixelInfo;
    IOReturn			err;
    IOFBDisplayModeID		mode;
    IOFBIndex			depth;
    IOFBTimingInformation	timingInfo;
    BOOL			mono;

    err = [self getDisplayModeAndDepthForIndex:index
		mode:&mode depth:&depth mono:&mono];

    if( err == noErr)
	err = [self getPixelInformationForDisplayMode:mode andDepthIndex:depth
	    pixelInfo:&pixelInfo];

    if( err == noErr)
	[self convertPixelInfoToDisplayInfo:&pixelInfo mono:mono displayInfo:displayInfo];

    if( flags &&
        [self getDisplayModeTiming:0 mode:mode timingInfo:&timingInfo connectFlags:flags])
            *flags = 0;

    return( err);
}

///////////////////////////////////////////////////////

- (IOReturn) open
{
    IOReturn			err;
    IOFBConfiguration		config;
    IOFBTimingInformation	timingInfo;
    IOFBDisplayModeID		startMode;
    IOFBIndex			startDepth;
    UInt32			startFlags = 0;
    UInt32			modeIndex;
    IOFBDisplayModeInformation	info;

    enum {			kNeedFlags = (kDisplayModeValidFlag | kDisplayModeSafeFlag) };

    if( opened == NO) {

	opened = YES;
	smartDisplay = [gSmartDisplayClass findForConnection:self refCon:0];
	[self makeConfigList];

	// Check the startup mode with the display

	[self getConfiguration:&config];
	err = [self getStartupMode:&startMode depth:&startDepth];
	if( err) {
	    startMode = config.displayMode;
	    startDepth = config.depth;
	}

	err = [self getDisplayModeInformation:startMode info:&info];

	if( err == noErr)
	    err = [self getDisplayModeTiming:0 mode:startMode
		    timingInfo:&timingInfo connectFlags:&startFlags];

	if( err || ((startFlags & kDisplayModeValidFlag) != kDisplayModeValidFlag) ) {

	    // look for default
	    modeIndex = 0;
	    err = [self getNextMode:&modeIndex modeInfo:&info needFlags:kDisplayModeDefaultFlag];
	    if( err) {
		// D'oh, look for a valid and safe
		modeIndex = 0;
		err = [self getNextMode:&modeIndex modeInfo:&info needFlags:kNeedFlags];
	    }
	    if( err == noErr)
		startMode = info.displayModeID;
	}

	if( startDepth > info.maxDepthIndex)
	    startDepth = info.maxDepthIndex;

        // make the preferred mode pending, but come up at 8bpp for faster console
        _pendingDisplayMode = 0;
        pendingDisplayModeID = startMode;
        pendingDepthIndex = startDepth;
	// look for 8bpp mode
        err = [self getConfigIndexForDisplayModeAndDepth:startMode depth:0
		mono:NO configIndex:&modeIndex];
	if( err == noErr)
            err = [self getDisplayModeAndDepthForIndex:modeIndex
                    mode:&startMode depth:&startDepth mono:&currentMono];

        currentMono = (0 != (config.flags & kFBLuminanceMapped));
	pendingMono = currentMono;

	if( (startMode != config.displayMode) || (startDepth != config.depth)) {

	    id deviceDesc = [self deviceDescription];

	    if( [deviceDesc respondsTo:@selector(denyNVRAM:)])
		[deviceDesc denyNVRAM:YES];

	    [self setDisplayMode:startMode depth:startDepth page:0];

            if( [deviceDesc respondsTo:@selector(denyNVRAM:)])
                [deviceDesc denyNVRAM:NO];
	}
	[self setupForCurrentConfig];
    }

    return( noErr);
}

+ (BOOL)probe:deviceDescription
{
    IOFramebuffer *inst;
    
    // Create an instance and initialize some basic instance variables.
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];
    if (inst == nil)
        return NO;

    [inst setDeviceKind:"Linear Framebuffer"];

    [inst registerDevice];

    return YES;
}

- (IOReturn) setUserRanges
{
    IOReturn			err;
    id				deviceDescrip;
    UInt32			i, start, size;

    deviceDescrip = [self deviceDescription];
    [deviceDescrip setMemoryRangeList:0 num:0];

    // needs to be much better !!
    for( i = 0; i < numRanges; i++) {

	start = userAccessRanges[ i ].start;
	size = start & 0xfff;
	start = start - size;
	size = (size + 0xfff + userAccessRanges[ i ].size) & 0xfffff000;
	userAccessRanges[ i ].start = start;
	userAccessRanges[ i ].size = size;
    }

    err = [deviceDescrip setMemoryRangeList:userAccessRanges num:numRanges];
 
    if(err) {
	IOLog("%s: setMemoryRangeList FAIL:%d for \n",[self name], err );
#if 1		// print ranges
	for( i = 0; i < numRanges; i++)
	    IOLog(" start:%x size:%x", userAccessRanges[ i ].start, userAccessRanges[ i ].size );
#endif
    }
#if 1		// print ranges
    {
    int			i, num;
    IORange	*	range;

    num = [deviceDescrip numMemoryRanges];
	kprintf("%s: setMemoryRangeList num:%d", [self name], num);
    for (i = 0; i < num; i++) {
	range = [deviceDescrip memoryRangeList] + i;
	    kprintf(" start:%x size:%x", range->start, range->size);
	}
	kprintf("\n");
    }
#endif
    return( err);
}

- (IOReturn) addUserRanges:(IORange *)ranges num:(UInt32)num
{
    UInt32	i;

    for( i = 0; i < num; i++) {
	userAccessRanges[ i + 1 ].start = ranges[ i ].start;
	userAccessRanges[ i + 1 ].size = ranges[ i ].size;
    }
    numRanges = num + 1;
    return( noErr);
}

- (IOReturn) setupForCurrentConfig
{
    IOReturn			err;
    IOFBConfiguration		config;
    IOFBPixelInformation	pixelInfo;
    IODisplayInfo	*	displayInfo;
    IOFBApertureInformation	apInfo;
    IOFBIndex			apIndex;

    displayInfo = [self displayInfo];		// current mode

    err = [self getConfiguration:&config];
    if( err)
	kprintf("setupForCurrentConfig:getConfiguration %d\n", err);
    currentDisplayModeID = config.displayMode;
    currentDepthIndex = config.depth;

    err = [self getPixelInformationForDisplayMode:currentDisplayModeID andDepthIndex:currentDepthIndex
	pixelInfo:&pixelInfo];
    if( err)
	kprintf("setupForCurrentConfig:getPixelInformationForDisplayMode %d\n", err);

    [self convertPixelInfoToDisplayInfo:&pixelInfo
	mono:currentMono
	displayInfo:displayInfo];
    displayInfo->frameBuffer = (void *) config.mappedFramebuffer;	// for cursor rendering

    if( (clutValid == NO) && (pixelInfo.pixelType == kIOFBRGBCLUTPixelType)) {

	IOFBColorEntry	*	tempTable;
	int			i;

	tempTable = IOMalloc( 256 * sizeof( IOFBColorEntry));
	if( tempTable) {

	    for( i = 0; i < 256; i++) {
                if( currentMono) {
                    UInt32	lum;

		    lum = 0x0101 * i;
                    tempTable[ i ].red   = lum;
                    tempTable[ i ].green = lum;
                    tempTable[ i ].blue  = lum;
                } else {
                    tempTable[ i ].red   = (appleClut8[ i * 3 + 0 ] << 8) | appleClut8[ i * 3 + 0 ];
                    tempTable[ i ].green = (appleClut8[ i * 3 + 1 ] << 8) | appleClut8[ i * 3 + 1 ];
                    tempTable[ i ].blue  = (appleClut8[ i * 3 + 2 ] << 8) | appleClut8[ i * 3 + 2 ];
		}
	    }
	    [self setCLUT:tempTable index:0 numEntries:256
		    brightness:EV_SCREEN_MAX_BRIGHTNESS options:0];
	    IOFree( tempTable, 256 * sizeof( IOFBColorEntry));
	}
    }
    clutValid = YES;

    [kmId registerDisplay:self];		// console now available

    kprintf( "%s: using (%dx%d@%dHz,%s)\n", [self name], displayInfo->width, displayInfo->height, 
			displayInfo->refreshRate,displayInfo->pixelEncoding );

    // our guess:
    if( numRanges < 1)
	numRanges = 1;
    userAccessRanges[0].start = config.physicalFramebuffer;
    userAccessRanges[0].size = ActiveBytes( displayInfo);
    // what the driver says:
    apIndex = 0;
    while( noErr == [self getApertureInformationByIndex:apIndex++ apertureInfo:&apInfo]) {
	if( apInfo.type == kIOFBGraphicsMemory) {
	    userAccessRanges[0].start = apInfo.physicalAddress;
	    userAccessRanges[0].size = apInfo.length;
	}
    }
    err = [self setUserRanges];

    framebufferOffset = config.physicalFramebuffer - userAccessRanges[0].start;

    if( cursorShmemBitsPerPixel == displayInfo->bitsPerPixel)
	cursorBitsPerPixel = displayInfo->bitsPerPixel;

    return( noErr);
}

- (IOReturn)
    getDisplayModeTiming:(IOFBIndex)connectIndex mode:(IOFBDisplayModeID)modeID
		timingInfo:(IOFBTimingInformation *)info connectFlags:(UInt32 *)flags

{
    OSStatus			err = noErr;

    // if smartDisplay is nil, the flags get passed through
    err = [smartDisplay getDisplayInfoForMode:info flags:flags];
    return( err );
}

//////////////////////////////////////////////////////////////////////
// IOCallDeviceMethods:


//////////////////////////////////////////////////////////////////////

// Description:	Get parameters for the display object. These include support
//		for getting the frame buffer parameters, registering it
//		with the event system, returning the registration token.

- (IOReturn)getIntValues:(unsigned *)parameterArray
		forParameter:(IOParameterName)parameterName
    		count:(unsigned int *)count
{
    unsigned int fb_dimensions[STDFB_FB_DIMENSIONS_SIZE];
    IOReturn r;
    int i;
    unsigned *returnedCount = count;
    unsigned maxCount = *count;
        
    if (strcmp(parameterName, STDFB_FB_MAP) == 0) {
        parameterArray[0] = 0;

//        [self enterLinearMode];
	*returnedCount = 1;
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_FB_DIMENSIONS) == 0) {
        IODisplayInfo *display = [self displayInfo];

	r = [self open];
	if( r)
	    return( r);				// this will halt WS on this display
	if( wsStartup)
            (void) [self _commitToPendingMode];
	[self setupForCurrentConfig];
	
	fb_dimensions[STDFB_FB_WIDTH] = display->width;
	fb_dimensions[STDFB_FB_HEIGHT] = display->height;
	fb_dimensions[STDFB_FB_ROWBYTES] = display->rowBytes;
	fb_dimensions[STDFB_FB_FLAGS] = display->flags;
	switch (display->bitsPerPixel) {
	    case IO_2BitsPerPixel:
	        fb_dimensions[STDFB_FB_BITS_PER_PIXEL] = 2; break;
	    case IO_8BitsPerPixel:
	        fb_dimensions[STDFB_FB_BITS_PER_PIXEL] = 8; break;
	    case IO_12BitsPerPixel:
	        fb_dimensions[STDFB_FB_BITS_PER_PIXEL] = 12; break;
	    case IO_15BitsPerPixel:
	        fb_dimensions[STDFB_FB_BITS_PER_PIXEL] = 15; break;
	    case IO_24BitsPerPixel:
	        fb_dimensions[STDFB_FB_BITS_PER_PIXEL] = 32; break;
	    default:
		/* Should return an error. */
		break;
	}

	*returnedCount = 0;
	for( i=0; i<STDFB_FB_DIMENSIONS_SIZE; i++) {
	    if (*returnedCount == maxCount)
		break;
	    parameterArray[i] = fb_dimensions[i];
	    (*returnedCount)++;
	}
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_FB_REGISTER) == 0) {
	r = [self _registerWithED];
	*returnedCount = 0;
	if (maxCount > 0) {
	    *returnedCount = 1;
	    parameterArray[0] = [self token];
	}
	return r;

    } else if (strcmp(parameterName, IO_GET_DISPLAY_INFO) == 0) {
	const IODisplayInfo *displayInfo;
	if ((*count != 5) && (*count != 7))
	    return IO_R_INVALID_ARG;
	displayInfo = [self displayInfo];
	parameterArray[0] = displayInfo->width;
	parameterArray[1] = displayInfo->height;
	parameterArray[2] = displayInfo->refreshRate;
	parameterArray[3] = (unsigned)displayInfo->bitsPerPixel;
	parameterArray[4] = (unsigned)displayInfo->colorSpace;
	if (*count == 7)	{
	    parameterArray[5] = displayInfo->totalWidth;
	    parameterArray[6] = displayInfo->rowBytes;
	}
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_GET_DISPLAY_MODE_NUM) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = numConfigs;
	return IO_R_SUCCESS;
 
   } else if (strncmp(parameterName, IO_GET_DISPLAY_MODE_INFO,
    		sizeof(IO_GET_DISPLAY_MODE_INFO)-1) == 0)	{
	IODisplayInfo theDisplayInfo;
	IODisplayInfo *displayInfo = &theDisplayInfo;
	const char *s;
	int mode;
	UInt32	flags;
	
	if (*count != 14)	{
	    return IO_R_INVALID_ARG;
	}
	s = find_parameter(IO_GET_DISPLAY_MODE_INFO, parameterName);
	if (s == 0)	{
	    return IO_R_INVALID_ARG;
	}
	mode = strtol(s, 0, 10);

        r = [self getDisplayInfoForConfigIndex:mode info:displayInfo
                        connectFlags:&flags];
	if( r)
	    return( r);

	parameterArray[0] = displayInfo->width;
	parameterArray[1] = displayInfo->height;
	parameterArray[2] = displayInfo->refreshRate;
	parameterArray[3] = (unsigned)displayInfo->bitsPerPixel;
	parameterArray[4] = (unsigned)displayInfo->colorSpace;
	parameterArray[5] = displayInfo->totalWidth;
	parameterArray[6] = displayInfo->rowBytes;
	
	parameterArray[7] = displayInfo->memorySize;
	parameterArray[8] = displayInfo->scanRate;
        parameterArray[9] = ((flags & kDisplayModeSafeFlag) ? IO_DISPLAY_MODE_SAFE : 0)
                        |  ((flags & kDisplayModeDefaultFlag) ? IO_DISPLAY_MODE_DEFAULT : 0);
	parameterArray[10] = displayInfo->dotClockRate;
	parameterArray[11] = displayInfo->screenWidth;
	parameterArray[12] = displayInfo->screenHeight;
	parameterArray[13] = displayInfo->modeUnavailableFlag;
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_GET_DISPLAY_MEMORY) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = 0;
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_GET_RAMDAC_SPEED) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = 0;
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_GET_CURRENT_DISPLAY_MODE) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	return( [self getCurrentConfigIndex:(UInt32 *)parameterArray]);

    } else if (strcmp(parameterName, IO_GET_PENDING_DISPLAY_MODE) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	if (_pendingDisplayMode >= 0)	{
	    parameterArray[0] = _pendingDisplayMode;
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_UNDEFINED_MODE;
	}

    } else if (strcmp(parameterName, "IOGetFramebufferOffset") == 0) {

	if( *count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = framebufferOffset;
	return IO_R_SUCCESS;

    } else if (strncmp(parameterName, IO_SET_PENDING_DISPLAY_MODE,
		sizeof(IO_SET_PENDING_DISPLAY_MODE) - 1 ) == 0) {
	const char *s;
	int mode;
	
	if (*count != 0) {
	    return IO_R_INVALID_ARG;
	}
	s = find_parameter(IO_SET_PENDING_DISPLAY_MODE, parameterName);
	if (s == 0)	{
	    return IO_R_INVALID_ARG;
	}
	mode = strtol(s, 0, 10);

	if ([self setPendingDisplayMode:mode] == YES) {
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_FAILED_TO_SET_MODE;
	}

    } else {
	return [super getIntValues:parameterArray
	    forParameter:parameterName
	    count:count];
    }
}


/* Set parameters for the display object. This can be used to unregister the
 * frame buffer as well as to set the transfer function.
 */
- (IOReturn)setIntValues:(unsigned *)parameterArray
		forParameter:(IOParameterName)parameterName
    		count:(unsigned int)count
{
    int i;

    if (strcmp(parameterName, STDFB_FB_UNMAP) == 0) {
	//[self unMapFrameBuffer];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_FB_UNREGISTER) == 0) {
	if (count != 1)
	    return IO_R_INVALID_ARG;
	[[EventDriver instance]	unregisterScreen:parameterArray[0]];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_SET_TRANSFER_TABLE) == 0) {
	switch ([self displayInfo]->bitsPerPixel) {
	case IO_2BitsPerPixel:
	    if (count != IO_2BPP_TRANSFER_TABLE_SIZE)
		return IO_R_INVALID_ARG;
	    break;
	case IO_8BitsPerPixel:
	    if (count != IO_8BPP_TRANSFER_TABLE_SIZE)
		return IO_R_INVALID_ARG;
	    break;
	case IO_12BitsPerPixel:
	    if (count != IO_12BPP_TRANSFER_TABLE_SIZE)
		return IO_R_INVALID_ARG;
	    break;
	case IO_15BitsPerPixel:
	    if (count != IO_15BPP_TRANSFER_TABLE_SIZE)
		return IO_R_INVALID_ARG;
	    break;
	case IO_24BitsPerPixel:
	    if (count != IO_24BPP_TRANSFER_TABLE_SIZE)
		return IO_R_INVALID_ARG;
	    break;
	default:
	    return IO_R_INVALID_ARG;
	}
	[self setTransferTable:parameterArray count:count];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_BM256_TO_BM38_MAP) == 0) {
	if (count != STDFB_BM256_TO_BM38_MAP_SIZE)
	    return IO_R_INVALID_ARG;
	if (_bm256To38SampleTable == NULL)
	    _bm256To38SampleTable = (unsigned int *)
			IOMalloc(STDFB_BM256_TO_BM38_MAP_SIZE * sizeof(int));
	for (i = 0; i < count; i++)
	    _bm256To38SampleTable[i] = parameterArray[i];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_BM38_TO_BM256_MAP) == 0) {

	if ((count != STDFB_BM38_TO_BM256_MAP_SIZE)
	&& (count != STDFB_BM38_TO_256_WITH_LOGICAL_SIZE))
	    return IO_R_INVALID_ARG;
	if (_bm38To256SampleTable == NULL) {
	    _bm38To256SampleTable = (unsigned char *)
			IOMalloc(STDFB_BM38_TO_256_WITH_LOGICAL_SIZE * sizeof(int));
        }
	if( count != STDFB_BM38_TO_256_WITH_LOGICAL_SIZE)
	    for (i = 0; i < 256; i++)
		_bm38To256SampleTable[ 1024 + i ] = i;
	for (i = 0; i < count; i++)
	    *(((unsigned int *)_bm38To256SampleTable) + i) = parameterArray[i];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, IO_SET_PENDING_DISPLAY_MODE) == 0) {
	if (count != 1)
	    return IO_R_INVALID_ARG;
	if ([self setPendingDisplayMode:parameterArray[0]] == YES) {
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_FAILED_TO_SET_MODE;
	}
    } else {
	return [super setIntValues:parameterArray forParameter:parameterName
	    count:count];
    }
}


/* Get parameters for the display object.
 */
- (IOReturn)getCharValues:(unsigned char *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned *)count
{    
    if (strcmp(parameterName, STDFB_FB_PIXEL_ENCODING) == 0) {
	if (*count != STDFB_FB_PIXEL_ENCODING_SIZE)
	    return IO_R_INVALID_ARG;

	/* Copy the format string out of the display structure */
	strncpy(parameterArray, [self displayInfo]->pixelEncoding,
		STDFB_FB_PIXEL_ENCODING_SIZE);
	return IO_R_SUCCESS;


    } else if (strcmp(parameterName, "PostScript Driver") == 0) {
	wsStartup = YES;
        return IO_R_UNSUPPORTED;

#if 0
    } else if (strcmp(parameterName, "PostScript Driver") == 0) {

	if (*count != sizeof(IOString))
	    return IO_R_INVALID_ARG;

	strcpy(parameterArray, "NDRV_psdrvr");
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, "Server Name") == 0) {

	strcpy(parameterArray, "IONDRVFramebuffer");
	return IO_R_SUCCESS;
#endif

    } else {
	return [super getCharValues:parameterArray
	    forParameter:parameterName
	    count:count];
    }
}

// Description:	Set parameters for the display object. This can be used
//		to set the mapping tables used for conversion between
//		4 BPS and 5 BPS data in 5-5-5 frame buffer support,
//		and bm256 <--> bm38 conversion tables as well.
//
- (IOReturn)setCharValues:(unsigned char *)parameterArray
		forParameter:(IOParameterName)parameterName
		count:(unsigned int)count
{
    int i;

    if (strcmp(parameterName, STDFB_4BPS_TO_5BPS_MAP) == 0) {
	if (count != STDFB_4BPS_TO_5BPS_MAP_SIZE)
	    return IO_R_INVALID_ARG;
	if (_bm34To35SampleTable == NULL)
	    _bm34To35SampleTable = IOMalloc(STDFB_4BPS_TO_5BPS_MAP_SIZE);
	for (i = 0; i < count; i++)
	    _bm34To35SampleTable[i] = parameterArray[i];
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_5BPS_TO_4BPS_MAP) == 0) {
	if (count != STDFB_5BPS_TO_4BPS_MAP_SIZE)
	    return IO_R_INVALID_ARG;
	if (_bm35To34SampleTable == NULL)
	    _bm35To34SampleTable = IOMalloc(STDFB_5BPS_TO_4BPS_MAP_SIZE);
	for (i = 0; i < count; i++)
	    _bm35To34SampleTable[i] = parameterArray[i];
	return IO_R_SUCCESS;
    } else if (strcmp(parameterName, IO_COMMIT_TO_PENDING_DISPLAY_MODE) == 0) {
	if ( [self _commitToPendingMode]) {
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_UNSUPPORTED;
	}
    } else {
	return [super setCharValues:parameterArray
	    forParameter:parameterName
	    count:count];
    }
}


- (BOOL)setPendingDisplayMode:(int)configIndex
{
    IOReturn		err;

    err = [self getDisplayModeAndDepthForIndex:configIndex
		mode:&pendingDisplayModeID depth:&pendingDepthIndex mono:&pendingMono];
    if( err == noErr) {
	wsStartup = NO;
	_pendingDisplayMode = configIndex;
	[self setStartupMode:pendingDisplayModeID depth:pendingDepthIndex];
    }
    return( err == noErr);
}

- (int)pendingDisplayMode
{
    return _pendingDisplayMode;
}

- setTransferTable:(const unsigned int *)table count:(int)count
{
    return self;
}

- (UInt32) tempFlags
{
    return( 0);
}

- (BOOL)_commitToPendingMode
{

    if( _pendingDisplayMode < 0)
	return( NO);

    [kmId unregisterDisplay:self];		// tell kmDevice this device is unavail
    cursorBitsPerPixel = (-1);
    currentMono = pendingMono;
    [self setDisplayMode:pendingDisplayModeID depth:pendingDepthIndex page:0];
    _pendingDisplayMode = -1;	/* mode switch is complete */
    [self setupForCurrentConfig];

    return( YES);
}

+ initialize
{
    gSmartDisplayClass = objc_lookUpClass("IOSmartDisplay");
    return( self);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// IOCallDeviceMethods:

// these should be somewhere else, IODevice or something:

- (IOReturn) IOGetApertures:(IOApertureInfo *)outputParams size:(unsigned *) outputCount
{
    IOReturn	err = noErr;
    UInt32	items;

    items = (*outputCount) / sizeof( IOApertureInfo);
    err = [[self deviceDescription]
	getApertures:outputParams items:&items];
    *outputCount = items * sizeof( IOApertureInfo);

    return( err);
}


- (IOReturn) IOGetDeviceName:(char *)outputParams size:(unsigned *) outputCount
{
    IOReturn	err = noErr;
    char * 	name;
    UInt32	len, plen;

    name = [[self deviceDescription] nodeName];
    plen = strlen( name);
    len = *outputCount - 1;
    *outputCount = plen + 1;
    if( plen < len)
	len = plen;
    strncpy( outputParams, name, len);
    outputParams[ len ] = 0;

    return( err);
}

// really IOFramebuffer:

- (IOReturn) IOFBSetUserRange:(IORange *)inputParams size:(unsigned) inputCount
{
    IOReturn	err = noErr;
    UInt32	items;

    items = inputCount / sizeof( IORange);
    err = [self addUserRanges:(IORange *)inputParams num:items];
    if( err == noErr)
	err = [self setUserRanges];

    return( err);

}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassFramebuffer);
    return( self);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

- (IOReturn)
    getDisplayModeByIndex:(IOFBIndex)modeIndex displayMode:(IOFBDisplayModeID *)displayModeID  {}

- (IOReturn)
    getDisplayModeInformation:(IOFBDisplayModeID)modeID info:(IOFBDisplayModeInformation *)info  {}

- (IOReturn)
    getPixelInformationForDisplayMode:(IOFBDisplayModeID)mode andDepthIndex:(IOFBIndex)depthIndex
    	 pixelInfo:(IOFBPixelInformation *)info  {}

- (IOReturn)
    setDisplayMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex page:(IOFBIndex)pageIndex  {}
- (IOReturn)
    setStartupMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex  {}
- (IOReturn)
    getStartupMode:(IOFBDisplayModeID *)modeID depth:(IOFBIndex *)depthIndex  {}

- (IOReturn)
    getConfiguration:(IOFBConfiguration *)config   {}

- (IOReturn)
    getAttributeForMode:(IOFBDisplayModeID) modeID andDepth:(IOFBIndex)depthIndex
    				attribute:(IOFBSelect)attr value:(UInt32 *)value  {}

- (IOReturn)
    getApertureInformationByIndex:(IOFBIndex)apertureIndex
    			 apertureInfo:(IOFBApertureInformation *)apertureInfo
{
    return( IO_R_UNSUPPORTED);
}

- (IOReturn)
    setCLUT:(IOFBColorEntry *) colors index:(UInt32)index numEntries:(UInt32)numEntries 
    	brightness:(IOFixed)brightness options:(IOOptionBits)options  {}

//// Gammas

- (IOReturn)
    setGammaTable:(UInt32) channelCount dataCount:(UInt32)dataCount dataWidth:(UInt32)dataWidth
    	data:(void *)data  {}


+ (IOReturn)
    convertCursorImage:(void *)cursorImage hwDescription:(HardwareCursorDescriptorRec *)hwDescrip
			cursorResult:(HardwareCursorInfoRec *)cursorResult  {}


- (IOReturn)
    setCursorImage:(void *)cursorImage  {}
- (IOReturn)
    setCursorState:(SInt32)x y:(SInt32)y visible:(Boolean)visible  {}

- (IOReturn)
    registerForInterruptType:(IOFBSelect)interruptType proc:(IOFBInterruptProc)proc refcon:(UInt32)refcon  {}

- (IOReturn)
    setInterruptState:(IOFBSelect)interruptType state:(UInt32)state  {}

- (IOReturn)
    setAttribute:(IOFBSelect)attribute value:(UInt32)value  {}

- (IOReturn)
    getAttribute:(IOFBSelect)attribute value:(UInt32 *)value  {}

- (IOReturn)
    privateCall:(IOFBSelect)callClass select:(IOFBSelect)select
    		paramSize:(UInt32)paramSize params:(void *)params
		resultSize:(UInt32 *)resultSize results:(void *)results  {}

- (IOReturn)
    getConnections:(IOFBIndex)connectIndex 
		 connectInfo:(IOFBConnectionInfo *)info  {}

- (IOReturn)
    setConnectionAttribute:(IOFBIndex)connectIndex attribute:(IOFBSelect)attribute value:(UInt32)value  {}

- (IOReturn)
    getConnectionAttribute:(IOFBIndex)connectIndex attribute:(IOFBSelect)attribute value:(UInt32 *)value  {}


@end

