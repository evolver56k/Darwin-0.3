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
 * kmDevice.h - private #defines and class definition for kernel
 *		keyboard/monitor module.
 *
 * HISTORY
 * 16-Jan-92    Doug Mitchell at NeXT 
 *      Created. 
 */
 
#ifdef	DRIVER_PRIVATE

#import <driverkit/IODevice.h>
#import "PCKeyboardDefs.h"
#import "ConsoleSupport.h"

#define NUM_KM_DEVS	1
#define FB_DEV_NORMAL	0		// frameBuffer for normal I/O
#define FB_DEV_ALERT	1		// frameBuffer for alert panels
#define NUM_FB_DEVS	2

#define INBUF_SIZE	16		// size of circular buffer for 
					//   blocking getc



/*
 * This class's interface is private to km.m and kmPrivate.m. The 
 * kmDevice is the glue between the bsd device interface (kmopen, kmputc, 
 * etc.) and the two classes which do the actual I/O - adbKeyboard and 
 * frameBuffer.
 */
@interface kmDevice : IODevice <PCKeyboardImported>
{
	/*
	 * kmOpenLock (an NXLock) protects all operations which create and
	 * destroy frameBuffer instances.
	 */
	id		kmOpenLock;

	/*
	 * frame buffer state.
	 */
	IOConsoleInfo	*fbp[NUM_FB_DEVS];	// output devices
	ScreenMode	fbMode;			// current screen mode
	ScreenMode	savedFbMode;		// screen mode before entering
						//   FBM_ALERT
	int 		alertRefCount;
	
	/*
	 * Keyboard info.
	 */
	id		kbId;			// input device (adbKeyboard)
	unsigned	blockIn:1;		// indicates "blocking getc()
						//   mode"
	
	
	/*
	 * Circular buffer for blocking getc(). A thread waiting for
	 * synchronous input locks inBufLock and sleeps on inBuf.
	 */
	int		inBuf[INBUF_SIZE];
	int		inDex;			// in pointer
	int		outDex;			// out pointer
	simple_lock_data_t inBufLock;
	
	BOOL		hasRegistered;		// registerDevice has been
						//   called
}

/*
 * Standard direct device probe.
 */
+ (BOOL)probe : deviceDescription;
+ (IODeviceStyle)deviceStyle;

- init 					: (BOOL)initKb
					  fb_mode : (ScreenMode)fb_mode;

/*
 * Keyboard-specific initialization.
 */
- initKb;

/*
 * Interface to bsd code.
 */
- (int)kmOpen				: (int)flag;
- (int)kmPutc 				: (int)c;	// paint one character
- (int)kmGetc;						// blocking read

/*
 * ioctl equivalents.
 */
- (int)restore;
- (int)drawRect				: (const struct km_drawrect *)kmRect;
- (int)eraseRect			: (const struct km_drawrect *)kmRect;
- (int)disableCons;
- (int)dumpMsgBuf;

- (int)animationCtl			: (km_anim_ctl_t)ctl;
- (int)getStatus			: (unsigned *)statusp;
- (int)getScreenSize			: (struct ConsoleSize *)size;

/*
 * Kernel internal alert panel support.
 */
- (void)doAlert				: (const char *)windowTitle
					  msg : (const char *)msg;

- (void)registerDisplay:newDisplay;
- (void)unregisterDisplay:oldDisplay;

- (void)returnToVGAMode;

@end

@interface kmDevice (KmGraphics)

- (void)graphicPanelString:(char *)string;
- (void)drawGraphicPanel:(GraphicPanelType)panelType;

@end

/*
 * 'Global' variables, shared only by km.m and kmDevice.m.
 */
extern kmDevice *kmId;				// the kmDevice
extern IOConsoleInfo *basicConsole;
extern ScreenMode basicConsoleMode;			
extern const char *mach_title;

#endif	/* DRIVER_PRIVATE */

/* end of kmDevice.h */


