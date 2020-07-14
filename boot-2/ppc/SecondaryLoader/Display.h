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
#ifndef DISPLAY_H
#define DISPLAY_H 1

#if 0
enum {
	kNeXTIconWidth = 96,
	kNeXTIconHeight = 96,
	kNeXTIconRowBytes8 = kNeXTIconWidth
};
#endif
void ShowWelcomeIcon ();


void ExpandTo8WithMask (UInt8 *expandedP, int expandedRowBytes,
							   const UInt32 *sourceP, const UInt32 *maskP,
							   const int srcDepth, const int width, const int height,
							   const UInt8 *pixelValuesP, const UInt8 backgroundPixel);

void FillRectangle (UInt8 pixel, int x, int y, int width, int height);
void DrawRectangle (UInt8 *pixels, int x, int y, int width, int height);
void ReadRectangle (UInt8 *pixels, int x, int y, int width, int height);
void DeathScreen (char *msg);
extern const UInt32 NeXTIcon[];
extern CICell FindAndOpenDisplay (int *widthP, int *heightP);
extern void SetupKernelVideoParams (struct Boot_Video *p);

#endif

