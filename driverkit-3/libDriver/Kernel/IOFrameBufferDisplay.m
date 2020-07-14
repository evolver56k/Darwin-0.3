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
 * IOFrameBufferDisplay.m - Implements common methods for "standard" frame
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
#import	<driverkit/IOFrameBufferDisplay.h>
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

@interface IOFrameBufferDisplay(Private)
- (BOOL)_commitToPendingMode;
@end

@implementation IOFrameBufferDisplay

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

static void StdFBDisplayCursor16(IOFrameBufferDisplay *inst)
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
    unsigned short *savePtr;	/* saved screen data pointer */
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

static void StdFBDisplayCursor8(IOFrameBufferDisplay *inst)
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
    unsigned char *savePtr;	/* saved screen data pointer */
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

static void StdFBRemoveCursor16(IOFrameBufferDisplay *inst)
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
    unsigned short *savePtr;

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

static void StdFBRemoveCursor8(IOFrameBufferDisplay *inst)
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
    unsigned char *savePtr;

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

static void StdFBDisplayCursor32(IOFrameBufferDisplay *inst)
{
    IODisplayInfo *dpy;
    StdFBShmem_t *shmem;
    unsigned int vramRow;
    Bounds saveRect;
    int i, j, width, cursRow;
    unsigned int *vramPtr;	/* screen data pointer */
    unsigned int *savePtr;	/* saved screen data pointer */
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

static void StdFBRemoveCursor32(IOFrameBufferDisplay *inst)
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

static inline void StdFBDisplayCursor(IOFrameBufferDisplay *inst) 
{
    switch ([inst displayInfo]->bitsPerPixel) {
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
	    break;
    }
}

static inline void StdFBRemoveCursor(IOFrameBufferDisplay *inst) 
{
    switch ([inst displayInfo]->bitsPerPixel) {
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
	    break;
    }
}


#define RemoveCursor(inst)	StdFBRemoveCursor(inst)
static inline void DisplayCursor(IOFrameBufferDisplay *inst)
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

static inline void SysHideCursor(IOFrameBufferDisplay *inst)
{
    if (!GetShmem(inst)->cursorShow++)
	RemoveCursor(inst);
}

static inline void SysShowCursor(IOFrameBufferDisplay *inst)
{
    if (GetShmem(inst)->cursorShow)
	if (!--(GetShmem(inst)->cursorShow))
	    DisplayCursor(inst);
}

static inline void CheckShield(IOFrameBufferDisplay *inst)
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
+ (BOOL)probe:deviceDescription
{
    IOFrameBufferDisplay *inst;
    
    // Create an instance and initialize some basic instance variables.
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];
    if (inst == nil)
        return NO;

    [inst setDeviceKind:"Linear Framebuffer"];

    [inst registerDevice];

    return YES;
}

- initFromDeviceDescription:deviceDescription
{
    extern int sprintf(char *s, const char *format, ...);
    static int nextUnit = 0;
    char nameBuf[20];

    if ([super initFromDeviceDescription:deviceDescription] == nil)
	return [super free];

    _currentDisplayMode = _pendingDisplayMode = -1;
    _displayModeCount = -1; _displayModes = NULL;
    
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
	[self revertToVGAMode];		// start from a well-defined state
	[self enterLinearMode];
	[kmId registerDisplay:self];
	*returnedCount = 1;
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, STDFB_FB_DIMENSIONS) == 0) {
        IODisplayInfo *display = [self displayInfo];
	
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
	parameterArray[0] = [self displayModeCount];
	return IO_R_SUCCESS;
    } else if (strncmp(parameterName, IO_GET_DISPLAY_MODE_INFO,
    		sizeof(IO_GET_DISPLAY_MODE_INFO)-1) == 0)	{
	const IODisplayInfo *displayInfo, *displayModes;
	const char *s;
	int mode;
	
	if (*count != 14)	{
	    return IO_R_INVALID_ARG;
	}
	s = find_parameter(IO_GET_DISPLAY_MODE_INFO, parameterName);
	if (s == 0)	{
	    return IO_R_INVALID_ARG;
	}
	mode = strtol(s, 0, 10);
	if (mode < 0 || mode >= [self displayModeCount])
	    return IO_R_INVALID_ARG;
	    
	displayModes = [self displayModes];
	displayInfo = &displayModes[mode];
	
	parameterArray[0] = displayInfo->width;
	parameterArray[1] = displayInfo->height;
	parameterArray[2] = displayInfo->refreshRate;
	parameterArray[3] = (unsigned)displayInfo->bitsPerPixel;
	parameterArray[4] = (unsigned)displayInfo->colorSpace;
	parameterArray[5] = displayInfo->totalWidth;
	parameterArray[6] = displayInfo->rowBytes;
	
	parameterArray[7] = displayInfo->memorySize;
	parameterArray[8] = displayInfo->scanRate;
	parameterArray[9] = 0;
	parameterArray[10] = displayInfo->dotClockRate;
	parameterArray[11] = displayInfo->screenWidth;
	parameterArray[12] = displayInfo->screenHeight;
	parameterArray[13] = displayInfo->modeUnavailableFlag;
	
	return IO_R_SUCCESS;
    } else if (strcmp(parameterName, IO_GET_DISPLAY_MEMORY) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = [self displayMemorySize];
	return IO_R_SUCCESS;
    } else if (strcmp(parameterName, IO_GET_RAMDAC_SPEED) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	parameterArray[0] = [self ramdacSpeed];
	return IO_R_SUCCESS;
    } else if (strcmp(parameterName, IO_GET_CURRENT_DISPLAY_MODE) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	if (_currentDisplayMode >= 0)	{
	    parameterArray[0] = _currentDisplayMode;
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_UNDEFINED_MODE;
	}
    } else if (strcmp(parameterName, IO_GET_PENDING_DISPLAY_MODE) == 0)	{
	if (*count != 1)
	    return IO_R_INVALID_ARG;
	if (_pendingDisplayMode >= 0)	{
	    parameterArray[0] = _pendingDisplayMode;
	    return IO_R_SUCCESS;
	} else {
	    return IO_R_UNDEFINED_MODE;
	}

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
        [self revertToVGAMode];
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

// For NeXT logical palette to real palette conversion, an additional
//   256 bytes can be added to the table
#define STDFB_BM38_TO_256_WITH_LOGICAL_SIZE	(STDFB_BM38_TO_BM256_MAP_SIZE + (256/sizeof(int)))

	if ((count != STDFB_BM38_TO_BM256_MAP_SIZE) && (count != STDFB_BM38_TO_256_WITH_LOGICAL_SIZE))
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

/*
 * This is the first message sent by the windowserver. If there is a pending
 * display mode then we commit to it (i.e. update our IODisplayInfo
 * structure). 
 */
#define IO_POSTSCRIPT_DRIVER		"PostScript Driver"

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
    } else if (strcmp(parameterName, IO_POSTSCRIPT_DRIVER) == 0) {
	/*
	 * FIXME: This is a temporary hack till WS gets fixed. At that time
	 * WS will use IO_COMMIT_TO_PENDING_DISPLAY_MODE and this code can be
	 * deleted. -- rkd.
	 */
	if (_pendingDisplayMode >= 0) {
	    (void) [self _commitToPendingMode];
	    _pendingDisplayMode = -1;	/* mode switch is complete */
	}
	return [super getCharValues:parameterArray
	    forParameter:parameterName
	    count:count];
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
    int i, mode;

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
	if (_pendingDisplayMode >= 0) {
	    (void) [self _commitToPendingMode];
	    _pendingDisplayMode = -1;	/* mode switch is complete */
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

- (void)enterLinearMode;
// Description:	Put the display into linear framebuffer mode. This typically
//		happens when the window server starts running. This method
//		is implemented by subclasses in a device specific way. The
//		value returned is a pointer to the framebuffer in user space.
{
}

- (void)revertToVGAMode;
// Description:	Get the device out of whatever advanced linear mode it was
//		using and back into a state where it can be used as a standard
//		VGA device. This method is implemented by subclasses in a
//		device specific way.
{
}

- (BOOL)setPendingDisplayMode:(int)displayMode
{
    IODisplayInfo *displayInfo; 
	
    displayInfo = [self displayInfo];		// current mode
    
    if (displayMode < 0 || displayMode >= [self displayModeCount])	{
        IOLog("%s: Invalid display mode: %d\n", [self name], displayMode);
    	return NO;
    }
    if (displayInfo->modeUnavailableFlag != 0)	{
        IOLog("%s: Display mode %d not available (error 0x%0x)\n", 
		[self name], displayMode, displayInfo->modeUnavailableFlag);
    	return NO;
    }
    _pendingDisplayMode = displayMode;
    return YES;
}

- (int)pendingDisplayMode
{
    return _pendingDisplayMode;
}

/*
 * The subclass must implement these two methods if it want display mode
 * changes. 
 */
- (unsigned int)displayModeCount
{
    return _displayModeCount;
}

- (IODisplayInfo *)displayModes
{
    return _displayModes;
}

/*
 * Subclass should override and supply appropriate values. 
 */
- (unsigned int)displayMemorySize
{
    return 0;
}

- (unsigned int)ramdacSpeed
{
    return 0;
}

- (vm_address_t)mapFrameBufferAtPhysicalAddress:(unsigned int)addr
	length:(int)length;
// Description:	Look up the physical memory location for this device instance
//		and map it into VM for use by the device driver. If problems
//		occur, the method returns (vm_address_t)0. If addr is not 0,
//		then it is used as the physical memory address and
//		length is used as the length.
{
    vm_address_t	vmLocation;
    IOReturn		result = IO_R_SUCCESS;
    IOCache		cacheType;
    
    switch ([self displayInfo]->flags & IO_DISPLAY_CACHE_MASK) {
        default:
        case IO_DISPLAY_CACHE_WRITETHROUGH:
	    cacheType = IO_WriteThrough;
	    break;
	case IO_DISPLAY_CACHE_COPYBACK:
	    cacheType = IO_CopyBack;
	    break;
	case IO_DISPLAY_CACHE_OFF:
	    cacheType = IO_CacheOff;
	    break;
	}

    if (addr != 0)	// Override configuration XXX BOGUS!!!
    {
    	// This is totally bogus.  I cannot believe that it is
	// public API.  NB: This mapping cannot be freed using
	// unmapMemoryRange:from: in IOEISADirectDevice.
	if (_KernBusMemoryCreateMapping(addr,
				length,
				&vmLocation,
				current_task_EXTERNAL(),
				YES,
				cacheType) != KERN_SUCCESS)
		result = IO_R_NO_MEMORY;
    }
    else
    {
	result = [self	mapMemoryRange:0
			to:&vmLocation
			findSpace:YES
			cache:cacheType];
    }

    if (result != IO_R_SUCCESS)
    {
	IOLog(
	    "IOFrameBufferDisplay/mapFrameBuffer: Can't map memory (%s)\n",
	    [self stringFromReturn:result]);
	return (vm_address_t)0;
    }
    
    return vmLocation;
}

#if 0
- (vm_address_t)mapFrameBufferAtPhysicalAddress:(unsigned int)addr
	length:(int)length;
// Description:	Look up the physical memory location for this device instance
//		and map it into VM for use by the device driver. If problems
//		occur, the method returns (vm_address_t)0. If addr is not 0,
//		then it is used as the physical memory address and
//		length is used as the length.
{
    vm_address_t	vmLocation;
    IOReturn		result = IO_R_SUCCESS;
    IOCache		cacheType;
    IORange		*range;
    
    /* The default is no cache */
    switch ([self displayInfo]->flags & IO_DISPLAY_CACHE_MASK) {
        default:
        case IO_DISPLAY_CACHE_WRITETHROUGH:
	    cacheType = IO_WriteThrough;
	    break;
	case IO_DISPLAY_CACHE_COPYBACK:
	    cacheType = IO_CopyBack;
	    break;
	case IO_DISPLAY_CACHE_OFF:
	    cacheType = IO_CacheOff;
	    break;
	}

    range = [[self deviceDescription] memoryRangeList];
    
    if (addr == 0)
    	addr = range[0].start;
    if (length == 0)
    	length = range[0].size;

    /*
     * The framebuffer must be contained in memory range 0. 
     */
    if ((addr < range[0].start) || 
        (addr+length > range[0].start + range[0].size))	{
	IOLog("%s: framebuffer is not contained in first memory range\n",
		[self name]);
	return IO_R_NO_MEMORY;
    }
    
    result = [self  mapMemoryRange:0
		    to:&vmLocation
		    findSpace:YES
		    cache:cacheType];
    IOLog("%s: Mapped framed buffer at 0x%x\n", [self name],
		vmLocation);
    
    if (result != IO_R_SUCCESS) {
	IOLog(
	    "IOFrameBufferDisplay/mapFrameBuffer: Can't map memory (%s)\n",
	    [self stringFromReturn:result]);
	return (vm_address_t)0;
    }
    
    return vmLocation + (addr - range[0].start);
}

- (void)unMapFrameBuffer:(unsigned int)addr length:(int)length
{
    void 	*vmLocation;
    IORange	*range;
    
    range = [[self deviceDescription] memoryRangeList];
    
    if (addr == 0)
    	addr = range[0].start;
    if (length == 0)
    	length = range[0].size;

    /*
     * The framebuffer must be contained in memory range 0. 
     */
    if ((addr < range[0].start) || 
        (addr+length > range[0].start + range[0].size))	{
	IOLog("%s: framebuffer is not contained in first memory range\n",
		[self name]);
	return IO_R_NO_MEMORY;
    }
    
    vmLocation = [self displayInfo]->frameBuffer - (addr - range[0].start);
    
    if (vmLocation)	{
	[self unmapMemoryRange:0 from:(vm_address_t)vmLocation];
    }
}
#endif 0


- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count
	valid:(const BOOL *)isValid modeString:(const char *)modeString
{
    const char *displayMode;
    IOConfigTable *configTable;
    int k, width, height;
    int screenWidth, screenHeight, memorySize, ramdacSpeed;
    int scanRate;
    unsigned int modeUnavailableFlag;
    IOBitsPerPixel bitsPerPixel;
    IOColorSpace colorSpace;
    int refreshRate;
    const char *s;

    _displayModeCount = count;
    _displayModes = modeList;
    
    /* Get the string describing the display mode. */

    if (modeString == NULL)	{
    
	configTable = [[self deviceDescription] configTable];
	if (configTable == nil)
	    return -1;

	displayMode = [configTable valueForStringKey:"Display Mode"];
	if (displayMode == 0) {
	    /* Historical: for 3.1 drivers only. */
	    displayMode = [configTable valueForStringKey:"DisplayMode"];
	    if (displayMode == 0)
		return -1;
	}
    } else {
    	displayMode = modeString;
    }

    /* Parse the string.  It should be of the form
     *   Width:# Height:# ColorSpace:(BW:#|RGB:###/#) Refresh:# Hz
     * where `Width' and `Height' specify the width and height of the
     * framebuffer, `ColorSpace' specifies the color space for the
     * framebuffer, and `Refresh' specifies the refresh rate in Hz.
     * The color space parameter should be either BW followed by the
     * bits/pixel, or RGB followed by the bits/component for each
     * component followed by the bits/pixel.
     *
     * For example, here is the display mode specification for the
     * RGB mode of the S3:
     *   Width: 800 Height: 600 ColorSpace: RGB:555/16 Refresh: 60 Hz
     */

    height = width = 0;
    s = find_parameter("Width:", displayMode);
    if (s != 0)	{
	width = strtol(s, 0, 10);
    }
    
    s = find_parameter("Height:", displayMode);
    if (s != 0)	{
	height = strtol(s, 0, 10);
    }

    s = find_parameter("Refresh:", displayMode);
    if (s == 0)
	return -1;
    refreshRate = strtol(s, 0, 10);

    s = find_parameter("ColorSpace:", displayMode);
    if (s == 0)
	return -1;
    if (strncmp(s, "BW:2", 4) == 0) {
        bitsPerPixel = IO_2BitsPerPixel;
	colorSpace = IO_OneIsBlackColorSpace;
    } else if (strncmp(s, "BW:8", 4) == 0) {
        bitsPerPixel = IO_8BitsPerPixel;
	colorSpace = IO_OneIsWhiteColorSpace;
    } else if (strncmp(s, "RGB:256/8", 9) == 0) {
        bitsPerPixel = IO_8BitsPerPixel;
	colorSpace = IO_RGBColorSpace;
    } else if (strncmp(s, "RGB:444/16", 10) == 0) {
        bitsPerPixel = IO_12BitsPerPixel;
	colorSpace = IO_RGBColorSpace;
    } else if (strncmp(s, "RGB:555/16", 10) == 0) {
        bitsPerPixel = IO_15BitsPerPixel;
	colorSpace = IO_RGBColorSpace;
    } else if (strncmp(s, "RGB:888/32", 10) == 0) {
        bitsPerPixel = IO_24BitsPerPixel;
	colorSpace = IO_RGBColorSpace;
    } else {
	return -1;
    }

    /*
     * Now look for 4.0 style optional parameters. The display mode
     * specification looks like "Resolution:1280x1024 Screen:1600x1200
     * Refresh:60Hz ColorSpace:RGB:444/16 Memory:4MB RAMDAC:175Hz Sync:100Hz"; 
     */
     
    s = find_parameter("Resolution:", displayMode);
    if (s != 0)	{
        char *end;
	width = strtol(s, &end, 10);
	height = strtol(end+1, 0, 10);
    }
    
    if ((height == 0) || (width == 0))
    	return -1;
	
    s = find_parameter("Screen:", displayMode);
    if (s != 0)	{
        char *end;
	screenWidth = strtol(s, &end, 10);
	screenHeight = strtol(end+1, 0, 10);
    } else {
	screenWidth = width;
	screenHeight = height;
    }

    /* These parameters are not used by the superclass. */
    s = find_parameter("Memory:", displayMode);
    if (s != 0)	{
	memorySize = strtol(s, 0, 10);
    }

    s = find_parameter("RAMDAC:", displayMode);
    if (s != 0)	{
	ramdacSpeed = strtol(s, 0, 10);
    }

    s = find_parameter("Sync:", displayMode);
    if (s != 0)	{
	scanRate = strtol(s, 0, 10);
    }

    modeUnavailableFlag = 0;
    s = find_parameter("Available:", displayMode);
    if (s != 0)	{
	modeUnavailableFlag = strtol(s, 0, 10);
    }
    
    /* Now try to match these parameters with the list of modes. */
    for (k = 0; k < count; k++) {
        
	if (isValid != 0 && !isValid[k])
	    continue;
        if (modeUnavailableFlag != 0)
            continue;
	if (modeList[k].width == width
	    && modeList[k].height == height
	    && modeList[k].colorSpace == colorSpace
	    && modeList[k].bitsPerPixel == bitsPerPixel
	    && modeList[k].refreshRate == refreshRate) {
	    switch (bitsPerPixel) {
	    case IO_2BitsPerPixel:  s = "BW:2"; break;
	    case IO_8BitsPerPixel:
		if (colorSpace == IO_RGBColorSpace) s = "RGB:256/8";
		else				    s = "BW:8";
		break;
	    case IO_12BitsPerPixel: s = "RGB:444/16"; break;
	    case IO_15BitsPerPixel: s = "RGB:555/16"; break;
	    case IO_24BitsPerPixel: s = "RGB:888/32"; break;
	    default:     s = "Unknown color space"; break;
	    }
	    IOLog("Display: Mode selected: %d x %d @ %d Hz (%s)\n",
	    	width, height, refreshRate, s);
	    _currentDisplayMode = k;
	    return k;
	}
    }
    IOLog("Display: Requested mode is not available.\n");
    return -1;
}

- (int)selectMode: (const IODisplayInfo *)modeList count:(int)count
{
    return [self selectMode:modeList count:count valid:0 modeString:NULL];
}

- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count
	valid:(const BOOL *)isValid
{
    return [self selectMode:modeList count:count valid:isValid modeString:NULL];
}

- setTransferTable:(const unsigned int *)table count:(int)count
{
    return self;
}

@end


//
// BEGIN:	PRIVATE Methods
//
@implementation IOFrameBufferDisplay(Private)

- (BOOL)_commitToPendingMode
{
    IODisplayInfo *displayInfo, *displayModes;
    int mode = _pendingDisplayMode; 
	
    displayInfo = [self displayInfo];		// current mode
    
    displayModes = [self displayModes];		// all possible modes
    if (displayModes == NULL)
    	return NO;
	
    bzero(displayInfo->pixelEncoding, IO_MAX_PIXEL_BITS);
    strncpy(displayInfo->pixelEncoding, displayModes[mode].pixelEncoding,
    	strlen(displayModes[mode].pixelEncoding));
    displayInfo->width = displayModes[mode].width;
    displayInfo->height = displayModes[mode].height;
    displayInfo->totalWidth = displayModes[mode].totalWidth;
    displayInfo->rowBytes = displayModes[mode].rowBytes;
    displayInfo->refreshRate = displayModes[mode].refreshRate;
    displayInfo->bitsPerPixel = displayModes[mode].bitsPerPixel;
    displayInfo->colorSpace = displayModes[mode].colorSpace;
    displayInfo->parameters = displayModes[mode].parameters;

    displayInfo->screenWidth = displayModes[mode].screenWidth;
    displayInfo->screenHeight = displayModes[mode].screenHeight;
    displayInfo->scanRate = displayModes[mode].scanRate;
    displayInfo->memorySize = displayModes[mode].memorySize;
    displayInfo->dotClockRate = displayModes[mode].dotClockRate;
    displayInfo->modeUnavailableFlag = displayModes[mode].modeUnavailableFlag;

    /*
     * If the driver defined the flag use it else chhose a default behavior. 
     */
    if (displayModes[mode].flags != 0) {
    	displayInfo->flags = displayModes[mode].flags;
    } else {
	if (displayInfo->bitsPerPixel == IO_8BitsPerPixel) {
	    displayInfo->flags = IO_DISPLAY_HAS_TRANSFER_TABLE;
	} else {
	    displayInfo->flags = IO_DISPLAY_NEEDS_SOFTWARE_GAMMA_CORRECTION;
	}
    }
    _currentDisplayMode = mode;
    
    return YES;
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassFramebuffer);
    return( self);
}

@end

