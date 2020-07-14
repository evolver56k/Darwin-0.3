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
/* 	Copyright (c) 1994 NeXT Computer, Inc.  All rights reserved. 
**
** ppcfb.h -- defines and prototypes for ppc graphics interface
**
*/

#import <driverkit/return.h>
#ifndef _PPC_FB_H_ /*[*/
#define _PPC_FB_H_


typedef struct {
    unsigned short x,y;			// source upperleft pixel coordinates
    unsigned short width, height;	// dimensions in pixels
} PPCFBRect;

typedef unsigned int PPCFBColor; // 24-bit rgb value: 4 unsigned chars: '-rgb'


#if 0

IOReturn ppcfbConfigDisplay(unsigned int displayNum);
IOReturn ppcfbClearDisplay(unsigned int displayNum, PPCFBColor rgb);
IOReturn ppcfbFillRect(unsigned int displayNum, PPCFBRect *Rect,
			  PPCFBColor rgb);
IOReturn ppcfbDrawRect(unsigned int displayNum, PPCFBRect *Rect,
			  unsigned int depth, unsigned char *bitmap);
IOReturn ppcfbMoveRect(unsigned int displayNum, PPCFBRect *Rect,
			 unsigned int dx, unsigned int dy);  // on screen only
IOReturn ppcfbLoadCmap(unsigned int displayNum);
#endif 0

#endif _PPC_FB_H_ /*] */
