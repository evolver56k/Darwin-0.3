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
 * FBConsPriv.h - Private constants and typedefs for FBConsole implementation.
 *
 * HISTORY
 * 08 Sep 92	Joe Pasqua
 *      Created. 
 */

#ifdef	DRIVER_PRIVATE

/*
 * Standard font.
 */
#define	CHAR_W	8
#define	CHAR_H	12
extern char ohlfs12[96][CHAR_H];

/*
 * Pixel masks, defining valid memory bits within a pixel.
 */
#define	PIXEL_MASK_16		0x0000ffff
#define	PIXEL_MASK_32		0x00ffffff

/*
 * Sizes (in pixels) for text and alert windows
 */
#define TEXT_WIN_WIDTH		640
#define TEXT_WIN_HEIGHT		480

#define ALERT_WIN_WIDTH		320
#define ALERT_WIN_HEIGHT	200

/*
 * Misc. constants.
 */
#define	TAB_SIZE		8		// in characters

#endif	/* DRIVER_PRIVATE */
