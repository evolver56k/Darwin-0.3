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
/* -*- mode:C++; tab-width: 4 -*- */

#include <Types.h>
#include <BootBlocksPriv.h>
// #include "/usr/include/setjmp.h"
#include <stdio.h>
#include <memory.h>
#include <KernelParams.h>

#include "SecondaryLoaderOptions.h"
#include <SecondaryLoader.h>
#include "Display.h"

static CICell displayIHandle;
static int displayWidth;
static int displayHeight;
static int displayRowBytes;
static int displayDepth;
static UInt32 displayBase;

// Indexed by dibit values as brokenSystemFolderIcon is being
// expanded to yield the byte value to store in the output icon
// pixel map
static const UInt8 twoBitPixelValues[] = {kWhitePixel, kLtGrayPixel,
										  kDkGrayPixel, kBlackPixel};

void fExpandTo8WithMask (UInt8 *expandedP, int expandedRowBytes,
						 const UInt32 *sourceP, const UInt32 *maskP,
						 const int srcDepth, const int width, const int height,
						 const UInt8 *pixelValuesP, const UInt8 backgroundPixel);
CICell fFindAndOpenDisplay (int *widthP, int *heightP);

void fFillRectangle (UInt8 pixel, int x, int y, int width, int height);
void fDrawRectangle (UInt8 *pixels, int x, int y, int width, int height);
void fReadRectangle (UInt8 *pixels, int x, int y, int width, int height);
void fDeathScreen (char *msg);
void fShowWelcomeIcon ();
//void fSpinActivity ();
//extern const UInt32 NeXTIcon[];


// Expand a srcDepth bit/pixel packed source (sourceP) into an 8
// bit/pixel packed destination (expandedP) such that ONE bits in a
// corresponding 1 bit/pixel mask imply a translated copy via pixel
// values from pixelValuesP[sourcPixelValue] and ZERO bits in the mask
// imply backgroundPixel.  The width and height of the pixel maps are
// as specified by parameters of those names.  A NIL maskP means don't
// mask.
void fExpandTo8WithMask (UInt8 *expandedP, int expandedRowBytes,
						 const UInt32 *sourceP, const UInt32 *maskP,
						 const int srcDepth, const int width, const int height,
						 const UInt8 *pixelValuesP, const UInt8 backgroundPixel)
{
	int n = 0;
	int srcShift = 0;
	int maskShift = 0;
	UInt32 srcLong = 0;		// Dumb compilers: "used before initialized"
	UInt32 maskLong = 0;	// Dumb compilers: "used before initialized"
	int srcMod = 32 / srcDepth - 1;
	UInt32 srcMask = ((1 << srcDepth) - 1);
	long totalPixels = width * height;
	int h;
	UInt8 *rowStartP = expandedP;

	// For each pixel in the source, create a destination pixel value by masked expansion
	for (n = 0; n < totalPixels; ++n) {
		UInt32 srcPixel;
		int maskBit;

		if ((n & srcMod) == 0) srcLong = *sourceP++;

		// This wiggy shift value is actually pretty simple if you
		// break it down.  We are using the shift values UNSIGNED
		// modulo 32 and ignoring the fact that they are going
		// negative by our repeated subtraction.

		// For example, say srcDepth is 2.  The sequence begins with
		// srcShift of zero.  Since we presubtract 2, we end up
		// shifting srcLong right by 30 bits and ANDing with 3 which
		// yields the leftmost 2 bits in srcLong.  Each successive
		// iteration shifts two bits fewer down until we underflow --
		// in which case we end up with 30 as the shift count again
		// due to the modulo action of ANDing the shift count with 31.
		srcPixel = (srcLong >> ((srcShift -= srcDepth) & 31)) & srcMask;
		
		if (maskP != nil) {
			int maskBit;

			if ((n & 31) == 0) maskLong = *maskP++;
			maskBit = (maskLong >> ((maskShift -= 1) & 31)) & 1;

			// Store expanded version of srcPixel or background,
			// depending on corresponding mask bit.
			*expandedP++ = maskBit ? pixelValuesP[srcPixel] : backgroundPixel;
		} else {
			*expandedP++ = pixelValuesP[srcPixel];
		}

		if (++h >= width) {		// Time to start a new row of destination pixels
			rowStartP += expandedRowBytes;
			expandedP = rowStartP;
			h = 0;
		}
	}
}


void fFillRectangle (UInt8 pixel, int x, int y, int width, int height)
{
	CIArgs ciArgs;
	CICell result;
	CICell catchResult;

    if( displayIHandle) {
	ciArgs.service = "call-method";
	ciArgs.nArgs = 7;
	ciArgs.nReturns = 1;
	ciArgs.args.callMethod_5_0.method = "fill-rectangle";		// ( index x y w h -- )
	ciArgs.args.callMethod_5_0.iHandle = displayIHandle;
	ciArgs.args.callMethod_5_0.arg1 = height;
	ciArgs.args.callMethod_5_0.arg2 = width;
	ciArgs.args.callMethod_5_0.arg3 = y;
	ciArgs.args.callMethod_5_0.arg4 = x;
	ciArgs.args.callMethod_5_0.arg5 = pixel;
	result = CallCI (&ciArgs);
	catchResult = ciArgs.args.callMethod_5_0.catchResult;
#ifdef DEBUG
	if (result != 0 && (slDebugFlag & kDebugLots)) VCALL(ShowMessage) ("?F-r CallCI");
	if (catchResult != 0 && (slDebugFlag & kDebugLots)) VCALL(ShowMessage) ("?F-r catch");
#endif
    }
}


const UInt8 Clut[] = {
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


static CICell tryThisDisplay (CICell pHandle)
{
	int len;
	char path[1024];			// Longer than this and we don't cope
	CIArgs ciArgs;

	ciArgs.service = "package-to-path";
	ciArgs.nArgs = 3;
	ciArgs.nReturns = 1;
	ciArgs.args.packageToPath.phandle = pHandle;
	ciArgs.args.packageToPath.buf = path;
	ciArgs.args.packageToPath.buflen = sizeof (path) - 1;
	ciArgs.args.packageToPath.length = -1;
	CallCI (&ciArgs);

	len = ciArgs.args.packageToPath.length;

	if (len > 0 && len < sizeof (path)) {
		CICell foo;

		path[len] = 0;
#ifdef DEBUG
		if (slDebugFlag & kDebugLots) printf ("Trying display \"%s\"\n", path);
#endif
		ciArgs.service = "open";
		ciArgs.nArgs = 1;
		ciArgs.nReturns = 1;
		ciArgs.args.open.deviceSpecifier = path;

		if (CallCI (&ciArgs) != 0)
			foo = 0;
		else
			foo = ciArgs.args.open.ihandle;

#ifdef DEBUG
		if (slDebugFlag & kDebugLots) printf ("   open returned 0x%08X\n", foo);
#endif
		return foo;
	}
	
	if (kShowProgress) printf ("Found a display whose pkgtopath call failed: %d\n", len);
	return (CICell) 0;
}


static CICell findBestMainDisplayRecursively (CICell root)
{
	CICell node;
	CICell openOne = 0;
	char *devType;

	if (root == 0) return 0;

	devType = VCALL(GetPackagePropertyString) (root, "device_type");

	if (devType != 0 && strcmp (devType, "display") == 0) {
		openOne = tryThisDisplay (root);
	}
	
	// Now traverse the first child of this node and its peers, recursing on each one.
	for (node = VCALL(GetChildPHandle) (root);
		 node != 0 && openOne == 0;
		 node = VCALL(GetPeerPHandle) (node))
	{
		openOne = findBestMainDisplayRecursively (node);
	}

	return openOne;
}


static CICell findBestMainDisplay ()
{
	CICell node;
	CIArgs ciArgs;

	// Give preference to the System 7.x boot display if we can open it
#ifdef DEBUG
	if ( slDebugFlag & kDebugLots) printf ("Trying to use \"/AAPL,boot-display\" alias for display\n");
#endif

	ciArgs.service = "open";
	ciArgs.nArgs = 1;
	ciArgs.nReturns = 1;
	ciArgs.args.open.deviceSpecifier = "/AAPL,boot-display";

	if (CallCI (&ciArgs) != 0)
		node = 0;
	else
		node = ciArgs.args.open.ihandle;

	if (node != 0) return node;

	// Give preference to the devalias "screen" if we can open it (for PowerBooks)
#ifdef DEBUG
	if (slDebugFlag & kDebugLots) printf ("Trying to use \"screen\" alias for display\n");
#endif

	ciArgs.args.open.deviceSpecifier = "screen";

	if (CallCI (&ciArgs) != 0)
		node = 0;
	else
		node = ciArgs.args.open.ihandle;

	if (node != 0) return node;

#ifdef DEBUG
       	if (slDebugFlag & kDebugLots) printf ("Trying to find a good display by treewalk\n");
#endif
	return findBestMainDisplayRecursively (VCALL(GetPeerPHandle) (0));
}


CICell fFindAndOpenDisplay (int *widthP, int *heightP)
{
	CIArgs ciArgs;
	CICell result;
	CICell catchResult;

#ifdef DEBUG
	if (slDebugFlag & kDebugLots) VCALL(ShowMessage) ("F&Odisp");
#endif

	Interpret (
		"0 value DIH 0 value Dheight 0 value Dwidth "

		// Short form to draw-rectangle on display ihandle
		": ^drect DIH ?dup if \" draw-rectangle\" rot $CM else 3drop 2drop then ; "

		// Get property from iHandle as an assumed integer-formatted
		// property, or zero if none ( propname propnamelen -- propvalue )
		": GMP get-my-property 0= if D2NIP else 0 then ; "
   );

	if( 0 == (slDebugFlag & kDisableDisplays))
            displayIHandle = findBestMainDisplay ();
	else
            displayIHandle = 0;

	if( displayIHandle) {

	    ciArgs.service = "interpret";
	    ciArgs.nArgs = 3;
	    ciArgs.nReturns = 5;
	    ciArgs.args.interpret_2_4.arg1 = displayIHandle;
	    ciArgs.args.interpret_2_4.arg2 = (CICell) Clut;
	    ciArgs.args.interpret_2_4.forth = 		// ( clut -- width height display-ihandle )
		    "to DIH "							// Remember display iHandle in FORTH world
		    "value &CL "						// Remember address of clut array
		    "DIH to my-self "
		    "1 encode-int \" AAPL,boot-display\" property "		// We picked this device
    		    // Install colormap
		    "&CL 0 100 \" set-colors\" DIH $CM "
    
		    "\" width\" GMP ?dup 0= if 280 then "		// ( width )
		    "\" height\" GMP ?dup 0= if 1E0 then "		// ( width rowbytes height )
		    "2dup to Dheight to Dwidth "
		    "\" linebytes\" GMP ?dup 0= if Dwidth then "	// ( width height rowbytes )
		    "\" depth\" GMP ?dup 0= if 8 then "			// ( width height rowbytes depth )
    
		// Replace all drivers' rectangle words. Some ATI cards, some Apple drivers are wrong.
    
		"over value rowbytes "
		"active-package DIH ihandle>phandle to active-package "
		": rect-setup "                                // ( adr|index x y w h -- w adr|index xy-adr h )
		">r >r rowbytes * + frame-buffer-adr + r> -rot r> "
		"; "
    
	    ": DRAW-RECTANGLE "                            // ( adr x y w h -- )
	    "rect-setup "                                  // ( w adr xy-adr h )
		"0 ?do "                                  // ( w adr xy-adr )
		"2dup 4 pick move "
		"2 pick rowbytes d+ "
		"loop "
		"3drop "
	    "; "
    
		": FILL-RECTANGLE "                           //( index x y w h -- )
	    "rect-setup 0 ?do "
		"dup 3 pick 3 pick fill "
		"rowbytes + "
		"loop "
		"3drop "
	    "; "
    
		": READ-RECTANGLE "                            // ( adr x y w h -- )
		"rect-setup  >r swap r> 0 ?do "
			    "2dup 4 pick move "
			    "rowbytes 3 pick d+ "
			"loop "
			"3drop "
			"; "
		"to active-package "
	    ;
    
	    result = CallCI (&ciArgs);
	    if (result != 0) {
#ifdef DEBUG
		if (slDebugFlag & kDebugLots) VCALL(ShowMessage) ("?F&OD CallCI");
#endif
                return 0;
	    }
    
	    catchResult = ciArgs.args.interpret_2_4.catchResult;
	    if (catchResult != 0) {
#ifdef DEBUG
                    if (slDebugFlag & kDebugLots) VCALL(ShowMessage) ("?F&OD catch");
#endif
		    return 0;
	    }
	    
	    displayDepth = ciArgs.args.interpret_2_4.return1;
	    displayRowBytes = ciArgs.args.interpret_2_4.return2;
	    displayHeight = ciArgs.args.interpret_2_4.return3;
	    displayWidth  = ciArgs.args.interpret_2_4.return4;
    	    displayBase = InterpretReturn1 ("frame-buffer-adr");

	} else
	    displayBase = nil;

	if (heightP != nil) *heightP = displayHeight;
	if (widthP != nil) *widthP = displayWidth;
	
	// Fill the screen with our 50% gray background
	VCALL(FillRectangle) (128/*kBackgroundPixel*/, 0, 0, displayWidth, displayHeight);

	return displayIHandle;
}


void fDrawRectangle (UInt8 *pixels, int x, int y, int width, int height)
{
    CIArgs ciArgs;

    if( displayIHandle) {
	ciArgs.service = "call-method";
	ciArgs.nArgs = 7;
	ciArgs.nReturns = 1;
	ciArgs.args.callMethod_5_0.method = "draw-rectangle";		// ( adr x y w h -- )
	ciArgs.args.callMethod_5_0.iHandle = displayIHandle;
	ciArgs.args.callMethod_5_0.arg1 = height;
	ciArgs.args.callMethod_5_0.arg2 = width;
	ciArgs.args.callMethod_5_0.arg3 = y;
	ciArgs.args.callMethod_5_0.arg4 = x;
	ciArgs.args.callMethod_5_0.arg5 = (CICell) pixels;
	(void) CallCI (&ciArgs);
    }
}


void fReadRectangle (UInt8 *pixels, int x, int y, int width, int height)
{
    CIArgs ciArgs;

    if( displayIHandle) {
	ciArgs.service = "call-method";
	ciArgs.nArgs = 7;
	ciArgs.nReturns = 1;
	ciArgs.args.callMethod_5_0.method = "read-rectangle";		// ( adr x y w h -- )
	ciArgs.args.callMethod_5_0.iHandle = displayIHandle;
	ciArgs.args.callMethod_5_0.arg1 = height;
	ciArgs.args.callMethod_5_0.arg2 = width;
	ciArgs.args.callMethod_5_0.arg3 = y;
	ciArgs.args.callMethod_5_0.arg4 = x;
	ciArgs.args.callMethod_5_0.arg5 = (CICell) pixels;
	(void) CallCI (&ciArgs);
    }
}


void fDeathScreen (char *message)
{
	int i;

	// This is in two bit/pixel format, which is expanded at runtime
	// (to save space in image).  The mask is stored separately.  The
	// colors are: 00->white, 01->lite gray, 10->dark gray, 11->black.
	static const UInt32 brokenSystemFolderIcon[] = {
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x003FFF00,0x00000000,
		0x00D555C0,0x00000000,0x03555570,0x00000000,
		0x0D55555C,0x00000000,0x35555570,0x03FFFFFC,
		0xD555555C,0x00D55557,0xC0000030,0x03000003,
		0xC4444470,0x03111117,0xD1110FC0,0x0FFC4447,
		0xC44475C0,0x0D971117,0xD11136C0,0x0F774447,
		0xC4447630,0x03171117,0xD11136C0,0x0C874447,
		0xC4447630,0x03171117,0xD11136C0,0x0C874447,
		0xC4447630,0x03171117,0xD11135C0,0x0C474447,
		0xC4447570,0x03571117,0xD1113770,0x03574447,
		0xC4447670,0x03F71117,0xD11135C0,0x0D574447,
		0xC4445F00,0x3FFC1117,0xD1110F00,0x3FFC4447,
		0xC4444C00,0xD1111117,0xFFFFFF00,0x3FFFFFFF
	};

	// In this mask, zero bit=>transparent, one bit=>expanded pixel
	// from brokenSystemFolderIcon
	static const UInt32 brokenSystemFolderMask[] = {
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x07F00000,
		0x0FF80000,0x1FFC0000,0x3FFE0000,0x7FFC1FFE,
		0xFFFE0FFF,0xFFFC1FFF,0xFFFC1FFF,0xFFF83FFF,
		0xFFF83FFF,0xFFF83FFF,0xFFFC1FFF,0xFFF83FFF,
		0xFFFC1FFF,0xFFF83FFF,0xFFFC1FFF,0xFFF83FFF,
		0xFFFC1FFF,0xFFFC1FFF,0xFFFC1FFF,0xFFF83FFF,
		0xFFF07FFF,0xFFF07FFF,0xFFE0FFFF,0xFFF07FFF
	};

	// Icon as expanded to 8bits/pixel
	UInt8 expandedIcon[256 + kIconHeight * kIconWidth];

	// Expand 2 bit/pixel icon to 8 bits/pixel
	VCALL(ExpandTo8WithMask) (expandedIcon, kIconWidth,
							  brokenSystemFolderIcon, brokenSystemFolderMask,
							  2, kIconWidth, kIconHeight,
							  twoBitPixelValues, 0x80);
	// hack: clear the garbage in the first line of broken folder icon
	for (i = 0 ; i < kIconWidth ; i++) expandedIcon[i] = 0x80;

	VCALL(ShowMessage) (message);

	// Fill the screen with our 50% gray background
	VCALL(FillRectangle) (0x80, 0, 0, displayWidth, displayHeight);

	// Display broken system folder icon
	VCALL(DrawRectangle) (expandedIcon,
						  (displayWidth - kIconWidth) / 2,
						  (displayHeight - kIconHeight) / 2,
						  kIconWidth, kIconHeight);
	
	// TEMPORARY	-- eventually we need to present a RESTART button onscreen
	VCALL(BailToOpenFirmware) ();
}

void fShowWelcomeIcon ()
{
#if 0
	// Fill the screen with our 50% gray background
	VCALL(FillRectangle) (0x80, 0, 0, displayWidth, displayHeight);

	VCALL(DrawRectangle) ((UInt8 *) NeXTIcon,
						  (displayWidth - kNeXTIconWidth) / 2,
						  (displayHeight - kNeXTIconHeight) / 2,
						  kNeXTIconWidth, kNeXTIconHeight);
#endif
}


void SetupKernelVideoParams (Boot_Video *p)
{
	p->v_baseAddr = displayBase;
	p->v_display = 0;			/* What IS this? */
	p->v_rowBytes = displayRowBytes;
	p->v_width = displayWidth;
	p->v_height = displayHeight;
	p->v_depth = displayDepth;
}

