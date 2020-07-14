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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * VGAConsPriv.h - Private constants and typedefs for
 *                  VGAConsole implementation.
 *
 * HISTORY
 * 02 Feb 93	Peter Graffagnino
 *      Created from FBConsPriv.h
 */

#ifdef	DRIVER_PRIVATE

/*
 * Standard font.
 */
extern char ohlfs12[][];

#import "ohlfs12.h"

/*
 * Framebuffer characteristics.
 */
#define FRAMEBUFFER_ADDRESS 0xa0000

/*
 * For VGA.
 */
#define VGA_TOTALWIDTH 640
#define VGA_HEIGHT 480
#define VGA_ROWBYTES 80
#define VGA_BANKBYTES 0x10000
#define VGA_LINESPERBANK (VGA_BANKBYTES / VGA_ROWBYTES)

/*
 * For SVGA.
 */
#define SVGA_TOTALWIDTH 1024
#define SVGA_HEIGHT 768
#define SVGA_ROWBYTES 128
#define SVGA_BANKBYTES 0x10000
#define SVGA_LINESPERBANK (SVGA_BANKBYTES / SVGA_ROWBYTES)

/*
 * Sizes (in pixels) for text and alert windows
 */
#define TEXT_WIN_WIDTH		600
#define TEXT_WIN_HEIGHT		450

#define ALERT_WIN_WIDTH		320
#define ALERT_WIN_HEIGHT	200

/*
 * Misc. constants.
 */
#define	TAB_SIZE		8		// in characters


/*
 * VGA Graphics Controller Port
 */

#define VGA_GC_ADDR		0x3CE
#define VGA_NUM_GC_REGS		9     	// number of graphics controller
					// registers to preserve
#define VGA_SEQ_ADDR		0x3C4
#define VGA_NUM_SEQ_REGS	5     	// number of sequencer
					// registers to preserve

/*
 * default graphics controller registers for mode 0x12
 */
static unsigned char defaultGCRegisters[] = {
	0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff
};

/*
 * Standard NeXT colors (set up by DPS driver)
 */

#define VGA_WHITE	3		
#define VGA_LTGRAY	2		
#define VGA_DKGRAY	1		
#define VGA_BLACK	0		

#endif	/* DRIVER_PRIVATE */
