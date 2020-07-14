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
/*
 * font.h -- definitions used in generating bitmap fonts for newblit
 */

typedef struct {
	short width;
	short height;
	short xoff;
	short yoff;
} bbox_t;

typedef struct {
	bbox_t bbx;
	short dwidth;
	int bitx;
} bitmap_t;

#define	FONTNAMELEN		128
#define	ENCODEBASE		0x20
#define	ENCODELAST		0xFC

typedef struct {
	char font[FONTNAMELEN+1];
	unsigned short size;
	bbox_t bbx;
	bitmap_t bitmaps[ENCODELAST - ENCODEBASE + 1];
	unsigned char bits[0];
} font_t;

/*
 * For 'c' output.
 */
typedef struct {
	char *font;
	unsigned short size;
	bbox_t bbx;
	bitmap_t bitmaps[ENCODELAST - ENCODEBASE + 1];
	unsigned char *bits;
} font_c_t;
