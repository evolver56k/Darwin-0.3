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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOSVGADisplay.m - Implements common methods for SVGA display driver class.
 *
 *
 * HISTORY
 * 07 July 93	Scott Forstall
 *      Created from earlier work by Joe Pasqua and Gary Crum.
 */

#define KERNEL_PRIVATE 1
#define DRIVER_PRIVATE 1

/* Notes:
 *   This class implements the evScreen protocol for all IOSVGADisplays. Well,
 *   almost. The setBrightness method doesn't do anything. Subclasses must
 *   override this and implement it as appropriate for the device.
 *
 *   To find things that need to be fixed, search for FIX, to find questions
 *   to be resolved, search for QUESTION, to find stuff that still needs to be
 *   done, search for TO DO.
 */

#import <string.h>
#import <stdlib.h>
#import <bsd/dev/i386/VGAConsole.h>
#import <stdio.h>
#import	<driverkit/EventDriver.h>
#import	<bsd/dev/evio.h>
#import	<bsd/dev/i386/kmDevice.h>
#import <driverkit/KernBus.h>
#import <driverkit/KernBusMemory.h>
#import <driverkit/IODisplayPrivate.h>
#import <driverkit/IOSVGADisplay.h>
#import <driverkit/IOVGAShared.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/driverTypes.h>


//
// BEGIN:	Defines used in this file
//

#define CURSOR_WIDTH_IN_PIXELS 16
#define PPXMASK ((unsigned int)0x0000000f)

#if (SWAPBITS == DEVICE_CONSISTENT)
#define LSHIFT >>
#define RSHIFT <<
#define LSHIFTEQ >>=
#define RSHIFTEQ <<=
#else (SWAPBITS == DEVICE_CONSISTENT)
#define LSHIFT <<
#define RSHIFT >>
#define LSHIFTEQ <<=
#define RSHIFTEQ >>=
#endif (SWAPBITS == DEVICE_CONSISTENT)

#define CLEAR_SEMAPHORE(shmem) \
	ev_unlock(&shmem->cursorSema)
#define SET_SEMAPHORE(shmem) \
	if (!ev_try_lock(&shmem->cursorSema)) return self
#define RECTS_INTERSECT(one, two) \
	(((one.minx < two.maxx) && (two.minx < one.maxx)) && \
	((one.miny < two.maxy) && (two.miny < one.maxy)))

//
// END:		Defines used in this file
//


@implementation IOSVGADisplay

//
// BEGIN:	Implementation of private routines for SVGA
//


- (VGAShmem_t *)_shmem
// Description:	Return IOSVGADisplay's shared memory which is
//		stored in a private instance variable.
{
    return (VGAShmem_t *)_priv;
}


- (IOReturn)_registerWithED
// Description:	Register this display device with the event driver.
//		Set up and initialize the shared memory.
{
    int shmem_size;
    Bounds bounds;
    int token;
    VGAShmem_t *shmem;
    
    token = [[EventDriver instance] registerScreen:self
	bounds:&bounds
	shmem:&(self->_priv)
	size:&shmem_size];
    shmem = [self _shmem];
    
    if ( token == -1 ) {
	return IO_R_INVALID_ARG;
    }
    
    // We allow the shmem_size to be less than sizeof(VGAShmem_t)
    // so that we need not consume the extra space that some of the
    // larger cursor variants require if we are really a lower bitdepth.
    if ( shmem_size > sizeof(VGAShmem_t) ) {
	IOLog("%s: shmem_size > sizeof (VGAShmem_t)(%d<>%d)\n",
	    [self name], shmem_size, sizeof (VGAShmem_t));
	[[EventDriver instance]	unregisterScreen:token];
	return IO_R_INVALID_ARG;
    }
    
    // Init shared memory area
    bzero( (char *)shmem, shmem_size );
    shmem->cursorShow = 1;
    shmem->screenBounds = bounds;
    [self setToken:token];
    
    return IO_R_SUCCESS;
}


- (void)_readBpp4planar: (unsigned short *)fb toBpp2packed32: (unsigned int *)dst
// Description:	reads in a 4-planar representation, converts it into a
//		packed-32 representation, and writes the result into
//		dst.
{
    unsigned int v, i;
    
    [self setReadPlane: 1];
    i = 0xffff ^ *fb;
    v =   (i & 0x8000) <<  2 | (i & 0x4000) <<  5 | (i & 0x2000) <<  8
	| (i & 0x1000) << 11 | (i & 0x0800) << 14 | (i & 0x0400) << 17
	| (i & 0x0200) << 20 | (i & 0x0100) << 23 | (i & 0x0080) >>  6
	| (i & 0x0040) >>  3 | (i & 0x0020)       | (i & 0x0010) <<  3
	| (i & 0x0008) <<  6 | (i & 0x0004) <<  9 | (i & 0x0002) << 12
	| (i & 0x0001) << 15;

    [self setReadPlane: 0];
    i = 0xffff ^ *fb;
    v |=  (i & 0x8000) <<  1 | (i & 0x4000) <<  4 | (i & 0x2000) <<  7
	| (i & 0x1000) << 10 | (i & 0x0800) << 13 | (i & 0x0400) << 16
	| (i & 0x0200) << 19 | (i & 0x0100) << 22 | (i & 0x0080) >>  7
	| (i & 0x0040) >>  4 | (i & 0x0020) >>  1 | (i & 0x0010) <<  2
	| (i & 0x0008) <<  5 | (i & 0x0004) <<  8 | (i & 0x0002) << 11
	| (i & 0x0001) << 14;

    *dst = v;
}

- (void)_writeBpp2packed32: (unsigned int *)dst toBpp4planar: (unsigned short *)fb
// Description:	reads in a packed-32 representation, converts it into a
//		4-planar representation, and writes the result into
//		fb.
{
    unsigned int i, dstvalue;
    unsigned short fbvalue;
    unsigned char fblo, fbhi;

    [self setWritePlane: 1];
    dstvalue = *dst;
    i = dstvalue & 0xAAAA;
    fblo = 0xff ^ (char) (((i & 0x8000) >> 15) | ((i & 0x2000) >> 12)
			| ((i & 0x0800) >>  9) | ((i & 0x0200) >> 6)
			| ((i & 0x0080) >>  3) |  (i & 0x0020)
			| ((i & 0x0008) <<  3) | ((i & 0x0002) << 6));
    i = (dstvalue >> 16) & 0xAAAA;
    fbhi = 0xff ^ (char) (((i & 0x8000) >> 15) | ((i & 0x2000) >> 12)
			| ((i & 0x0800) >>  9) | ((i & 0x0200) >> 6)
			| ((i & 0x0080) >>  3) |  (i & 0x0020)
			| ((i & 0x0008) <<  3) | ((i & 0x0002) << 6));
    fbvalue = (((unsigned short)fbhi) << 8) | fblo;
    *fb = fbvalue;

    [self setWritePlane: 0];
    dstvalue = *dst;
    i = dstvalue & 0x5555;
    fblo = 0xff ^ (char) (((i & 0x4000) >> 14) | ((i & 0x1000) >> 11)
			| ((i & 0x0400) >>  8) | ((i & 0x0100) >> 5)
			| ((i & 0x0040) >>  2) | ((i & 0x0010) << 1)
			| ((i & 0x0004) <<  4) | ((i & 0x0001) << 7));
    i = (dstvalue >> 16) & 0x5555;
    fbhi = 0xff ^ (char) (((i & 0x4000) >> 14) | ((i & 0x1000) >> 11)
			| ((i & 0x0400) >>  8) | ((i & 0x0100) >> 5)
			| ((i & 0x0040) >>  2) | ((i & 0x0010) << 1)
			| ((i & 0x0004) <<  4) | ((i & 0x0001) << 7));
    fbvalue = (((unsigned short)fbhi) << 8) | fblo;
    *fb = fbvalue;
}


- (void)_VGADisplayCursor: (IODisplayInfo *)dpy shmem: (VGAShmem_t *)shmem
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
    Bounds screenBounds = shmem->screenBounds;
    Bounds saveRect;		/* cursor save rectangle (local) */
    unsigned int *cursPtr;	/* cursor data pointer */
    unsigned int *vramPtr;	/* screen data pointer */
    unsigned int *savePtr;	/* saved screen data pointer */
    unsigned int *maskPtr;	/* cursor mask pointer */
    int i, doLeft, doRight, skew, rSkew;
    static unsigned int vramBuf[2];
    unsigned short *this_segment, *fb;
    unsigned int fb_x_offset_shorts;
    unsigned int fb_shorts_per_line;
    unsigned int fb_lines_per_segment;
    unsigned int fb_segment;
    unsigned int fb_next_segment_y;
    int y;
    int miny, maxy;		/* absolute min and max y--not relative address */
    				/* e.g. 0 <= miny <= SCREEN_HEIGHT */

    [self savePlaneAndSegmentSettings];

    saveRect = shmem->cursorRect;
    vramPtr = (unsigned int *)vramBuf;

    //
    // Clip saveRect vertical within screen bounds
    //
    if (saveRect.miny < screenBounds.miny) {
	saveRect.miny = screenBounds.miny;
    }
    if (saveRect.maxy > screenBounds.maxy) {
	saveRect.maxy = screenBounds.maxy;
    }
    i = shmem->cursorRect.minx - screenBounds.minx;
    saveRect.minx = i - (i & PPXMASK) + screenBounds.minx;
    saveRect.maxx = saveRect.minx + CURSORWIDTH*2;
    shmem->saveRect = saveRect;

    //
    // skew is in bits
    //
    skew = (shmem->cursorRect.minx & PPXMASK)<<1;
    rSkew = 32-skew;

    //
    // Set up pointers for saving and drawing
    //
    cursPtr = shmem->cursor.bw.image[shmem->frame];
    maskPtr = shmem->cursor.bw.mask[shmem->frame];
    savePtr = shmem->cursor.bw.save;
    i = saveRect.miny - shmem->cursorRect.miny;
    cursPtr += i;
    maskPtr += i;

    //
    // Since we are drawing an int at a time, and it may
    // cross an int boundary, we draw it in two pieces (left
    // and right) which are offset by skew.
    //
    doLeft =  (saveRect.minx >= screenBounds.minx);
    doRight = (saveRect.maxx <= screenBounds.maxx);

    //
    // VGA related assignments
    //
    // QUESTION--isn't this a little fishy??  Should this be the address we mapped?
    this_segment = (unsigned short *)(0xa0000);
    fb_x_offset_shorts = (saveRect.minx - screenBounds.minx) >> 4;
    fb_shorts_per_line = dpy->width >> 4;
    fb_lines_per_segment = 0x10000 / (dpy->width >> 3);
    
    miny = saveRect.miny - screenBounds.miny;
    maxy = saveRect.maxy - screenBounds.miny;
    
    fb =    this_segment +
	    ((miny % fb_lines_per_segment) * fb_shorts_per_line) +
	    fb_x_offset_shorts;
    fb_segment = miny / fb_lines_per_segment;
    fb_next_segment_y = (fb_lines_per_segment * (fb_segment + 1));
    
    [self setWriteSegment: fb_segment];
    [self setReadSegment: fb_segment];

    for (   y = miny;
	    y < maxy;
	    y++) {
	register unsigned int workreg;

	//
	// Change VGA segment if necessary.
	//
	if (y == fb_next_segment_y) {
	    fb =    this_segment + 
		    ((y % fb_lines_per_segment) * fb_shorts_per_line) +
		    fb_x_offset_shorts;
	    fb_segment++;
	    fb_next_segment_y += fb_lines_per_segment;
	    
	    [self setWriteSegment: fb_segment];
	    [self setReadSegment: fb_segment];
	}

	if (doLeft) {
	    [self _readBpp4planar:fb toBpp2packed32:vramPtr];
	    *savePtr++ = workreg = *vramPtr;
	    *vramPtr = 	(workreg&(~((*maskPtr) RSHIFT skew))) |
			((*cursPtr) RSHIFT skew);
	    [self _writeBpp2packed32:vramPtr toBpp4planar:fb];
	}
	if (doRight) {
	    if (!skew) {
		savePtr++;
	    } else {
		[self _readBpp4planar:(fb+1) toBpp2packed32:(vramPtr+1)];
		*savePtr++ = workreg = *(vramPtr+1);
		*(vramPtr+1)=	(workreg&(~((*maskPtr) LSHIFT rSkew))) |
				((*cursPtr) LSHIFT rSkew);
		[self _writeBpp2packed32:(vramPtr+1) toBpp4planar:(fb+1)];
	    }
	}

	//
	// Advance fb (pointer into VGA framebuffer) one line.
	//
	fb += fb_shorts_per_line;

	cursPtr++;
	maskPtr++;
    }

    [self restorePlaneAndSegmentSettings];
}


- (void)_VGARemoveCursor: (IODisplayInfo *)dpy shmem: (VGAShmem_t *)shmem
// Description:	RemoveCursor erases the cursor by replacing the background
//		image that was saved by the previous call to DisplayCursor.
//		If the frame buffer is cacheable, flush at the end of the
//		drawing operation.
{
    int doLeft, doRight;
    unsigned int lmask = 0, rmask = 0;
    unsigned int *vramPtr;
    unsigned int *savePtr;
    Bounds screenBounds = shmem->screenBounds;
    Bounds saveRect = shmem->saveRect;
#if __BIG_ENDIAN__
    static const unsigned int mask_array[16] = {
    	0xFFFFFFFF,0x3FFFFFFF,0x0FFFFFFF,0x03FFFFFF,
	0x00FFFFFF,0x003FFFFF,0x000FFFFF,0x0003FFFF,
	0x0000FFFF,0x00003FFF,0x00000FFF,0x000003FF,
	0x000000FF,0x0000003f,0x0000000F,0x00000003
    };
#else
    static unsigned int mask_array[17] = {
	0xFFFFFFFF,0xFFFFFFFC,0xFFFFFFF0,0xFFFFFFC0,
	0xFFFFFF00,0xFFFFFC00,0xFFFFF000,0xFFFFC000,
	0xFFFF0000,0xFFFC0000,0xFFF00000,0xFFC00000,
        0xFF000000,0xFC000000,0xF0000000,0xC0000000,0x00000000};
#endif
    static unsigned int vramBuf[2];
    unsigned short *this_segment, *fb;
    unsigned int fb_x_offset_shorts;
    unsigned int fb_shorts_per_line;
    unsigned int fb_lines_per_segment;
    unsigned int fb_segment;
    unsigned int fb_next_segment_y;
    int y;
    int skew;
    int miny, maxy;		/* absolute min and max y--not relative address */
    				/* e.g. 0 <= miny <= SCREEN_HEIGHT */

    [self savePlaneAndSegmentSettings];

    vramPtr = (unsigned int *)vramBuf;

    //
    // VGA related assignments
    //
    // QUESTION--isn't this a little fishy??  Should this be the address we mapped?
    this_segment = (unsigned short *)(0xa0000);
    fb_x_offset_shorts = (saveRect.minx - screenBounds.minx) >> 4;
    fb_shorts_per_line = dpy->width >> 4;
    
    miny = saveRect.miny - screenBounds.miny;
    maxy = saveRect.maxy - screenBounds.miny;
    
    //
    // VGA bank size is 64K
    //
    fb_lines_per_segment = 0x10000 / (dpy->width >> 3);
    fb =    this_segment +
	    ((miny % fb_lines_per_segment) * fb_shorts_per_line) +
	    fb_x_offset_shorts;
    fb_segment = miny / fb_lines_per_segment;
    fb_next_segment_y = (fb_lines_per_segment * (fb_segment + 1));
    
    [self setWriteSegment: fb_segment];
    [self setReadSegment: fb_segment];

    //
    // skew is in bits
    //
    skew = (shmem->cursorRect.minx & PPXMASK)<<1;

    savePtr = shmem->cursor.bw.save;
    if (doLeft = (saveRect.minx >= screenBounds.minx)) {
    	lmask = mask_array[shmem->oldCursorRect.minx - saveRect.minx];
    }
    if (doRight = (saveRect.maxx <= screenBounds.maxx)) {
    	rmask = ~mask_array[16-(saveRect.maxx - shmem->oldCursorRect.maxx)];
    }
	
    for (   y = miny;
	    y < maxy;
	    y++) {

	//
	// Change VGA segment if necessary.
	//
	if (y == fb_next_segment_y) {
	    fb =    this_segment + 
		    ((y % fb_lines_per_segment) * fb_shorts_per_line) +
		    fb_x_offset_shorts;
	    fb_segment++;
	    fb_next_segment_y += fb_lines_per_segment;
	    
	    [self setWriteSegment: fb_segment];
	    [self setReadSegment: fb_segment];
	}

	if (doLeft) {
	    [self _readBpp4planar:fb toBpp2packed32:vramPtr];
	    *vramPtr = (*vramPtr&(~lmask))|(*savePtr++&lmask);
	    [self _writeBpp2packed32:vramPtr toBpp4planar:fb];
	}

	if (doRight) {
	    if (!skew) {
		savePtr++;
	    } else {
		[self _readBpp4planar:(fb+1) toBpp2packed32:(vramPtr+1)];
		*(vramPtr+1)=(*(vramPtr+1)&(~rmask))|(*savePtr++&rmask);
		[self _writeBpp2packed32:(vramPtr+1) toBpp4planar:(fb+1)];
	    }
	}

	//
	// Advance fb (pointer into VGA framebuffer) one line.
	//
	fb += fb_shorts_per_line;

    }

    [self restorePlaneAndSegmentSettings];
}

- (void)_displayCursor: (IODisplayInfo *)d shmem: (VGAShmem_t *)shmem
// Description:	Private routine to display the cursor.  Sets up the
//		cursor rect.
{
    Point hs;
    hs = shmem->hotSpot[shmem->frame];
    shmem->cursorRect.maxx =
        (shmem->cursorRect.minx = (shmem->cursorLoc).x - hs.x) + 16;
    shmem->cursorRect.maxy =
        (shmem->cursorRect.miny = (shmem->cursorLoc).y - hs.y) + 16;
    [self _VGADisplayCursor:d shmem:shmem];
    shmem->oldCursorRect = shmem->cursorRect;
}

- (void)_sysHideCursor: (IODisplayInfo *)d shmem: (VGAShmem_t *)shmem
// Description:	Private routing to hide the cursor.
{
    if (!shmem->cursorShow++) {
	[self _VGARemoveCursor:d shmem:shmem];
    }
}

- (void)_sysShowCursor: (IODisplayInfo *)d shmem: (VGAShmem_t *)shmem
// Description:	Private routine to show the cursor
{
    if (shmem->cursorShow)
	if (!--shmem->cursorShow)
	    [self _displayCursor:d shmem:shmem];
}

- (void)_checkShield: (IODisplayInfo *)d shmem: (VGAShmem_t *)shmem
// Description:	QUESTION
{
    Point hs;
    int intersect;
    Bounds tempRect;
    
    //
    // Calculate temp cursorRect
    //
    hs = shmem->hotSpot[shmem->frame];
    tempRect.maxx = (tempRect.minx = (shmem->cursorLoc).x - hs.x) + 16;
    tempRect.maxy = (tempRect.miny = (shmem->cursorLoc).y - hs.y) + 16;

    intersect = RECTS_INTERSECT(tempRect, shmem->shieldRect);
    if (intersect != shmem->shielded) {
	(shmem->shielded = intersect) ? [self _sysHideCursor:d shmem:shmem] :
				        [self _sysShowCursor:d shmem:shmem];
    }
}


static const char *
find_parameter(const char *parameter, const char *string)
// Description:	Find parameter in string and return what follows
//		it.  E.g. for string = "Width: 10" and
//		parameter = "Width:" return "10".
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


- (void)_generateName: (char *)name andUnit:(IOObjectNumber *)unit
// Description:	Used to generate a name for a new Display object.
//              "name" is a char array of size IO_STRING_LENGTH
{
    static int nextSVGAUnit = 0;

    *unit = nextSVGAUnit++;
    sprintf(name, "SVGADisplay%d", *unit);
}


//
// END:		Implementation of private routines for SVGA
//



//
// BEGIN:	Implementation of the evScreen protocol
//


- hideCursor: (int)token
{
    SET_SEMAPHORE([self _shmem]);
    [self _sysHideCursor:[self displayInfo] shmem:[self _shmem]];
    CLEAR_SEMAPHORE([self _shmem]);
    return self;
}

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t
{
    VGAShmem_t *shmem;
    
    shmem = [self _shmem];
    SET_SEMAPHORE(shmem);
    shmem->frame = frame;
    shmem->cursorLoc = *cursorLoc;
    if (!shmem->cursorShow++) {
	[self _VGARemoveCursor:[self displayInfo] shmem:shmem];
    }
    if (shmem->cursorObscured) {
	shmem->cursorObscured = 0;
	if (shmem->cursorShow)
	    --shmem->cursorShow;
    }
    if (shmem->shieldFlag) {
	[self _checkShield:[self displayInfo] shmem:shmem];
    }
    if (shmem->cursorShow) {
	if (!--shmem->cursorShow) {
	    [self _displayCursor:[self displayInfo] shmem:shmem];
	}
    }
    CLEAR_SEMAPHORE(shmem);
    return self;
}

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t
{
    VGAShmem_t *shmem;
    
    shmem = [self _shmem];
    SET_SEMAPHORE(shmem);
    shmem->frame = frame;
    shmem->cursorLoc = *cursorLoc;
    if (shmem->shieldFlag) {
	[self _checkShield:[self displayInfo] shmem:shmem];
    }
    [self _sysShowCursor:[self displayInfo] shmem:shmem];
    CLEAR_SEMAPHORE(shmem);
    return self;
}

//
// END:		Implementation of the evScreen protocol
//


//
// BEGIN:	EXPORTED methods
//

- (void)setReadSegment: (unsigned char)segmentNum
// Description:	Select which 64K segment we intend to read from -
//		this must be overridden by a subclasser.
{
}


- (void)setWriteSegment: (unsigned char)segmentNum
// Description:	Select which 64K segment we intend to write from -
//		this must be overridden by a subclasser.
{
}


- (void)setReadPlane: (unsigned char)planeNum
// Description:	Select which of 4 bit planes to read from in planar
//		modes - only one plane can be active at a time.
{
}


- (void)setWritePlane: (unsigned char)planeNum
// Description:	Select one of 4 bit planes to write to in planar modes,
//		although more than one plane can be active at a time,
//		this routine only allows access to 1 plane at a time.
{
}


- (void)savePlaneAndSegmentSettings
// Description:	Save plane and segment settings.  These methods must
//		be implemented by subclasses in a device specific way.
{
}


- (void)restorePlaneAndSegmentSettings
// Description:	Restore plane and segment settings.  These methods must
//		be implemented by subclasses in a device specific way.
{
}


- (void)enterSVGAMode
// Description:	Put the display into SVGA mode. This typically happens
//		when the window server starts running. This method is
//		implemented by subclasses in a device specific way.
{
}


- (void)revertToVGAMode
// Description:	Get the device out of whatever advanced mode it was using
//		and back into a state where it can be used as a standard
//		VGA device. This method is implemented by subclasses in a
//		device specific way.
{
}


task_t	(task_self)(void);	// from /NextDeveloper/Headers/mach/mach_traps.h

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
				    IO_WriteThrough) != KERN_SUCCESS)
		result = IO_R_NO_MEMORY;
    }
    else
    {
	result = [self	mapMemoryRange:0	//QUESTION--what is this???
			to:&vmLocation
			findSpace:YES
			cache:IO_WriteThrough];
    }

    if (result != IO_R_SUCCESS)
    {
	IOLog("IOSVGADisplay/mapFrameBuffer: Can't map memory (%s)\n",
	    [self stringFromReturn:result]);
	return (vm_address_t)0;
    }

    return vmLocation;
}



- (int)selectMode:(const IODisplayInfo *)modeList
	count:(int)count
	valid:(const BOOL *)isValid
// Description:	Choose a mode from the list `modeList' (containing `count'
//		modes) based on the value of the `DisplayMode' key in the
//		device's config table.  If `isValid' is nonzero, each element
//		specifies whether or not the corresponding mode is valid.
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

    /* Get the string describing the display mode. */

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
     * Refresh:60Hz ColorSpace:RGB:444/16 Memory:4MB RAMDAC:175Hz Sync:1000"; 
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
	height = strtol(end+1, 0, 10);
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
	    return k;
	}
    }
    IOLog("Display: Requested mode is not available.\n");
    return -1;
}


- (int)selectMode: (const IODisplayInfo *)modeList count:(int)count
// Description:	Equivalent to the above with `isValid' set to zero.
{
    return [self selectMode:modeList count:count valid:0];
}


+ (BOOL)probe:deviceDescription
// Description:	Create an instance of subclass to be associated with
//		specified deviceDescription. Returns whether or not
//		successful.
{
    IOSVGADisplay *inst;

    // Create an instance and initialize some basic instance variables.
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];
    if (inst == nil) {
	return NO;
    }
    
    [inst setDeviceKind:"frame buffer"];

    [inst registerDevice];

    return YES;
}




- initFromDeviceDescription:deviceDescription
// Description:	Initialize per specified deviceDescription. Returns
//		nil on error
{
    char name[IO_STRING_LENGTH];
    IOObjectNumber unit;
    
    if ([super initFromDeviceDescription:deviceDescription] == nil) {
	return [super free];
    }

    [self _generateName:name andUnit:&unit];
    [self setUnit:unit];
    [self setName:name];
    return self;
}



- setBrightness:(int)level token:(int)t
// Description:	Setting the brighness is a device-specific operation
//		which must be implemented by a subclass.  This just
//		checks if a legal level was sent.
{
    if ( level < EV_SCREEN_MIN_BRIGHTNESS
	|| level > EV_SCREEN_MAX_BRIGHTNESS )
    {
	IOLog("%s: Invalid arg to setBrightness:%d\n",
	    [self name], level );
    }
    return self;
}

- (IOReturn)getIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
// Description:	Get and set parameters. These include support
//		for getting the frame buffer parameters, registering it
//		with the event system, returning the registration token.
{
    unsigned int fb_dimensions[VGA_FB_DIMENSIONS_SIZE];
    IOReturn r;
    int i;
    unsigned *returnedCount = count;
    unsigned maxCount = *count;

    if ( strcmp(parameterName, VGA_FB_MAP) == 0 )
    {
        parameterArray[0] = 0;
	[self enterSVGAMode];
	[kmId registerDisplay:self];
	*returnedCount = 1;
	return IO_R_SUCCESS;
    }
    else if ( strcmp(parameterName, VGA_FB_DIMENSIONS) == 0 )
    {
        IODisplayInfo *display = [self displayInfo];
	
	fb_dimensions[VGA_FB_WIDTH] = display->width;
	fb_dimensions[VGA_FB_HEIGHT] = display->height;
	fb_dimensions[VGA_FB_ROWBYTES] = display->rowBytes;

	*returnedCount = 0;
	for( i=0; i<VGA_FB_DIMENSIONS_SIZE; i++) {
	    if (*returnedCount == maxCount)
		break;
	    parameterArray[i] = fb_dimensions[i];
	    (*returnedCount)++;
	}
	return IO_R_SUCCESS;

    }
    else if ( strcmp(parameterName, VGA_FB_REGISTER) == 0 )
    {
	r = [self _registerWithED];
	*returnedCount = 0;
	if ( maxCount > 0 )
	{
	    *returnedCount = 1;
	    parameterArray[0] = [self token];
	}
	return r;
    }
    else
    {
	r = [super getIntValues:parameterArray
			forParameter:parameterName
			count:returnedCount];
	return r;
    }
}

		
- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
// Description:	Set parameters. This can be used to unregister the frame
//		buffer as well as set the transfer function.
{
    if ( strcmp(parameterName, VGA_FB_UNMAP) == 0 ) {
        // [self _unmap];
	[self revertToVGAMode];
	return IO_R_SUCCESS;	
    } else if ( strcmp(parameterName, VGA_FB_UNREGISTER) == 0 ) {
	if ( count != 1 )
	    return IO_R_INVALID_ARG;
	[[EventDriver instance]	unregisterScreen:parameterArray[0]];
	return IO_R_SUCCESS;
	
    } else {
	return [super setIntValues:parameterArray
		      forParameter:parameterName
		      count:count];
    }
}

- (IOConsoleInfo *)allocateConsoleInfo;
// Description:	Allocates a console support info structure based on this
//		display. This structure, and the functions in it, are used
//		to display alert and console windows.
{
    return SVGAAllocateConsole([self displayInfo]);
}

//
// END:		EXPORTED methods
//

@end
