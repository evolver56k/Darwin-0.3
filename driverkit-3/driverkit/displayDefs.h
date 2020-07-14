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
/* 	Copyright (c) 1992-95 NeXT Computer, Inc.  All rights reserved. 
 *
 * displayDefs.h - Defs of various structs/data used by the display system.
 *
 *
 * HISTORY
 * 24 Oct 95	Rakesh Dubey
 *      Added/changed some data structs. 
 * 01 Sep 92	Joe Pasqua
 *      Created.
 */

#ifndef __DISPLAYDEFS_H__
#define __DISPLAYDEFS_H__

/* Bits per pixel values. */

typedef enum _IOBitsPerPixel {
    IO_2BitsPerPixel,	/*  2 bpp grayscale */
    IO_8BitsPerPixel,	/*  8 bpp grayscale */
    IO_12BitsPerPixel,	/* 16 bpp, 12 used (4 bits/component) */
    IO_15BitsPerPixel,	/* 16 bpp, 15 used (5 bits/component) */
    IO_24BitsPerPixel,	/* 32 bpp, 24 used (8 bits/component) */
    IO_VGA		/* VGA framebuffer (VGA is special, and may not be
			 * linearly mapped or packed pixel format.) */
} IOBitsPerPixel;

/* Definitions of colorspace type and values */

typedef enum _IOColorSpace {
    IO_OneIsBlackColorSpace = 0,	/* Monochrome, 1 is black. */
    IO_OneIsWhiteColorSpace = 1,	/* Monochrome, 1 is white. */
    IO_RGBColorSpace = 2,
    IO_CMYKColorSpace = 5,
} IOColorSpace;

/* Enumeration to encode the use of bits within a pixel. */

typedef enum _IOSampleType {
    IO_SampleTypeEnd = '\0',
    IO_SampleTypeRed = 'R',
    IO_SampleTypeGreen = 'G',
    IO_SampleTypeBlue = 'B',
    IO_SampleTypeAlpha = 'A',
    IO_SampleTypeGray = 'W',	/* 1 is white colorspace */

    IO_SampleTypeCyan = 'C',
    IO_SampleTypeMagenta = 'M',
    IO_SampleTypeYellow = 'Y',
    IO_SampleTypeBlack = 'K',	/* 1 is black colorspace */

    IO_SampleTypeLuminance = 'l',
    IO_SampleTypeChromaU = 'u',
    IO_SampleTypeChromaV = 'v',
    IO_SampleTypeChromaA = 'a',
    IO_SampleTypeChromaB = 'b',

    IO_SampleTypePseudoColor = 'P',
    IO_SampleTypeMustSet = '1',
    IO_SampleTypeMustClear = '0',
    IO_SampleTypeSkip = '-'	/* Unused bits in the pixel */
} IOSampleType;

/* The bits composing a pixel are identified by an array of `IOSampleType's
 * cast as chars.  The first element of the array describes the most
 * significant bit of the pixel.  The encoding char is repeated as many
 * times as is needed to represent the number of bits in the encoded
 * channel of the pixel.  The array is terminated by SampleType_End,
 * or a '\0'.  Calling `strlen' with the pixel encoding as an argument
 * returns the bit-depth of the pixels.
 *
 * Common pixel formats:
 *   RGBA (32 bits; 8 bits/component)	"RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA"
 *   RGB- (32 bits; 8 bits/component)	"RRRRRRRRGGGGGGGGBBBBBBBB--------"
 *   ARGB (32 bits; 8 bits/component)	"AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB"
 *   -RGB (32 bits; 8 bits/component)	"--------RRRRRRRRGGGGGGGGBBBBBBBB"
 *   RGBA (16 bits; 4 bits/component)	"RRRRGGGGBBBBAAAA"
 *   RGB- (16 bits; 4 bits/component)	"RRRRGGGGBBBB----"
 *   -RGB (16 bits; 5 bits/component)	"-RRRRRGGGGGBBBBB"
 *   W    (8 bit gray; one-is-white)	"WWWWWWWW"
 *   K	  (2 bit gray; one-is-black)	"KK"
 *
 */

/*
 * Reasons why a particular mode is not available. If the corresponding bit
 * is set then this mode is not available. 
 */
#define	IO_DISPLAY_MODE_VALID			0
#define IO_DISPLAY_MODE_NEEDS_MORE_MEMORY	2
#define IO_DISPLAY_MODE_SLOW_RAMDAC		4
#define IO_DISPLAY_MODE_WRONG_VERSION		8
#define IO_DISPLAY_MODE_OTHER_INVALID		16

#define IO_MAX_PIXEL_BITS	64	 /* Max length to keep MiG happy */

typedef char IOPixelEncoding[IO_MAX_PIXEL_BITS];

/* Structure describing the layout of the display. */

typedef struct _IODisplayInfo {
    int width;			/* Width in pixels. (can be virtual) */
    int height;			/* Height in pixels. (can be virtual) */
    int totalWidth;		/* Width in pixels including undisplayed
				 * pixels. */
    int rowBytes;		/* # bytes to get from one scanline to next. */
    int refreshRate;		/* Monitor refresh setting. */

    /* Pointer to origin of screen. This pointer is deliberately untyped to
     * force actual screen writes to be dependent on `bitsPerPixel'. */
    void *frameBuffer;	
    
    /* VRAM configuration, indicated by memory space occupied by one pixel. */
    IOBitsPerPixel bitsPerPixel;
    IOColorSpace colorSpace;
    IOPixelEncoding pixelEncoding;
    
    /* Flags used to indicate special requirements or conditions to DPS. */
    unsigned int flags;
    
    /* Driver specific parameters. */
    void *parameters;

    /* Resource requirements for this mode */
    int memorySize;		/* bytes */
    int scanRate;		/* Hz */
    int _reserved1;
    int dotClockRate;		/* Hz */

    /*
     * Real screen co-ordinates (as opposed to the ones known by
     * windowserver). The display driver can use these and override
     * moveCursor:frame:token: to implement panning. 
     */
    int screenWidth;
    int screenHeight;

    /*
     * This is set to zero only if this mode is available for the hardware
     * else the subclass should set specific bit(s) indicating why this mode
     * is not available. 
     */
    unsigned int modeUnavailableFlag;	
    
    unsigned int _reserved[1];

} IODisplayInfo;

/* Definition of values for the `IODisplayInfo.flags' field. */

/* Bit 1 determines whether the display requires gamma corrected 444->555
 * conversion in software. */

#define IO_DISPLAY_NEEDS_SOFTWARE_GAMMA_CORRECTION	0x00000002

/* Bits 2 and 3 specify cache behavior. */

#define IO_DISPLAY_CACHE_WRITETHROUGH		0x00000000 	/* default */
#define IO_DISPLAY_CACHE_COPYBACK		0x00000004
#define IO_DISPLAY_CACHE_OFF			0x00000008
#define IO_DISPLAY_CACHE_MASK			0x0000000C

/* Bit 4 indicates if the a hardware gamma correction transfer table
 * (CLUT)exists and can be changed by an IODISPLAY_SET_TRANSFER_TABLE call. */

#define IO_DISPLAY_HAS_TRANSFER_TABLE		0x00000010

/* Parameter to be supported by Display subclasses in their implementation
 * of setIntValues:forParameter:count: method, if the driver supports
 * setting a hardware gamma correction transfer table.
 *
 * The transfer table has a maximum size of 256 ints, and may be smaller.
 * 32 or 24 bit color, and 8 bit monochrome displays use the full 256 entries.
 * 15 bit color displays use 32 entries. 12 bit color displays use 16 entries.
 * 2 bit monochrome displays use 4 entries.  Each integer in the table holds
 * a packed RGBM value.  Monochrome displays use the low byte.  Color displays
 * should use the high 3 bytes, with Red in the most significant byte.
 */
#define IO_SET_TRANSFER_TABLE			"IOSetTransferTable"
#define IO_2BPP_TRANSFER_TABLE_SIZE		4
#define IO_8BPP_TRANSFER_TABLE_SIZE		256
#define IO_12BPP_TRANSFER_TABLE_SIZE		16
#define IO_15BPP_TRANSFER_TABLE_SIZE		32
#define IO_24BPP_TRANSFER_TABLE_SIZE		256
#define IO_MAX_TRANSFER_TABLE_SIZE		256


/* Bit 5 indicates if the device can perform a blit operation which
 * moves a rectangle from a source position to a destination position
 * on the screen.  Currently (12/94) the device must allow access to 
 * the framebuffer while the blit is occuring or prevent access to the
 * framebuffer from interfering with the blit operation. 
 *
 */
#define IO_DISPLAY_CAN_BLIT		0x00000020
#define IO_DISPLAY_BLIT_MASK		0x00000020

/*
 * Don't use IO_DISPLAY_DO_BLIT unless IO_DISPLAY_CAN_BLIT is set.
 *
 * Drivers can return IO_R_RESOURCE if the blit is not available or fails.
 * Users should be prepared to do the functional equivalent of the blit in
 * software.
 *
 * Parameters for the setIntValues interface are:
 * [0] = src_x; [1] = src_y;
 * [2] = width; [3] = height;
 * [4] = dst_x; [5] = dst_y;
 */
/* defines for setIntValues interface */
#define IO_DISPLAY_DO_BLIT           		"IODisplayDoBlit"
#define IO_DISPLAY_BLIT_SIZE			6

#define IO_GET_DISPLAY_PORT			"IOGetDisplayPort"
#define IO_GET_DISPLAY_PORT_SIZE		1

#define IO_GET_DISPLAY_INFO			"IOGetDisplayInfo"

#define IO_GET_DISPLAY_MEMORY			"IOGetDisplayMemory"
#define IO_GET_RAMDAC_SPEED			"IOGetRAMDACSpeed"

#define IO_SET_PENDING_DISPLAY_MODE		"IOSelectPendingDisplayMode"
#define IO_GET_PENDING_DISPLAY_MODE		"IOGetPendingDisplayMode"
#define IO_GET_CURRENT_DISPLAY_MODE		"IOGetCurrentDisplayMode"
#define IO_COMMIT_TO_PENDING_DISPLAY_MODE	"IOCommitToPendingDisplayMode"
#define IO_GET_DISPLAY_MODE_NUM			"IOGetDisplayModeNum"
#define IO_GET_DISPLAY_MODE_INFO		"IOGetDisplayModeInfo:"

/* IO_GET_DISPLAY_MODE_INFO parameterArray indices */

#define IO_DISPLAY_MODE_INFO_WIDTH		0
#define IO_DISPLAY_MODE_INFO_HEIGHT		1
#define IO_DISPLAY_MODE_INFO_REFRESH_RATE	2
#define IO_DISPLAY_MODE_INFO_DEPTH		3
#define IO_DISPLAY_MODE_INFO_CSPACE		4
#define IO_DISPLAY_MODE_INFO_TOTAL_WIDTH	5
#define IO_DISPLAY_MODE_INFO_ROW_BYTES		6
#define IO_DISPLAY_MODE_INFO_MEMORY_SIZE	7
#define IO_DISPLAY_MODE_INFO_SCAN_RATE		8
#define IO_DISPLAY_MODE_INFO_CONNECT_FLAGS	9
#define IO_DISPLAY_MODE_INFO_DOT_CLOCK		10
#define IO_DISPLAY_MODE_INFO_SCREEN_WIDTH	11
#define IO_DISPLAY_MODE_INFO_SCREEN_HEIGHT	12
#define IO_DISPLAY_MODE_INFO_UNAVAIL_FLAG	13
#define IO_DISPLAY_MODE_INFO_SIZE		14

/* IO_DISPLAY_MODE_INFO_CONNECT_FLAGS definitions */

#define IO_DISPLAY_MODE_SAFE		0x00000001
#define IO_DISPLAY_MODE_DEFAULT		0x00000002

#endif	/* __DISPLAYDEFS_H__ */
