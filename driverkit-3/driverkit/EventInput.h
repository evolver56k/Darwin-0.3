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
 * EventInput.h - Exported Interface Event Driver object input services.
 *
 *
 * HISTORY
 * 19 Mar 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	API cleanup
 */

#ifndef	_EVENT_INPUT_
#define _EVENT_INPUT_

#import <driverkit/EventDriver.h>

@interface EventDriver(Input) <IOEventSourceClient>

- runPeriodicEvent:(ns_time_t)nst;	// When to run periodic tasks next.
					// '0' implies NOW.
- scheduleNextPeriodicEvent;		// Schedule next periodic run
					// based on current event system state.
- (void)periodicEvents;			// Message invoked to run periodic
					// events.  This method may run in
					// IOTask or the kernel, on a softint
					// thread.
- (void)startCursor;			// Start the cursor running
- (int)pointToScreen:(Point*)p;		// Return screen number a point lies on

- attachDefaultEventSources;		// Connect to IOEventSource objects
- attachEventSource:(const char *)classname;	// Connect one IOEventSource object
- detachEventSources;			// Give up ownership of event sources

// Wait Cursor machinery
- (void)showWaitCursor;
- (void)hideWaitCursor;
- (void)animateWaitCursor;
- (void)changeCursor:(int)frame;

- doAutoDim;				// dim all displays
- undoAutoDim;				// Return display brightness to normal
- forceAutoDimState:(BOOL)dim;		// Force dim/undim.
- setBrightness:(int)b;			// Set the undimmed brightness
- (int)brightness;			// return undimmed brightness
- setAutoDimBrightness:(int)b;		// Set the dimmed brightness
- (int)autoDimBrightness;		// return dimmed brightness
- (int)currentBrightness;		// Return the current brightness
- setBrightness;			// Propagate state out to screens
- setAudioVolume:(int)v;		// Audio volume control
- setUserAudioVolume:(int)v;		// Audio volume control, from ext user
- (int)audioVolume;
- showCursor;
- hideCursor;
- moveCursor;

- setCursorPosition:(Point *)newLoc;	// Set abs cursor position
- _setCursorPosition:(Point *)newLoc atTime:(unsigned)t; // internal ONLY
- _setButtonState:(int)buttons atTime:(unsigned)t;	// internal ONLY

- registerEventSource:source;

@end

#endif	_EVENT_INPUT_

