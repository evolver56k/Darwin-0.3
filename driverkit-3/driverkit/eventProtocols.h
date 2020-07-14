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
 * eventProtocols.h - ObjC protocols used by the Event Driver and it's clients.
 *
 * HISTORY
 * 31 Mar 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 * 	API cleanup
 */
 
#import <driverkit/generalFuncs.h>
#import <driverkit/driverTypes.h>
#import <bsd/dev/ev_types.h>	/* Basic types for event system */
#import <bsd/dev/event.h>	/* Event flags */
#import <objc/objc.h>

/*
 * Methods exported by the EventDriver to event source objects.
 *
 *	The IOEventSourceClient protocol is used by event source objects to
 *	post state changes into the event driver.
 * 	The event driver implements this protocol.
 */
@protocol IOEventSourceClient

/*
 * Called when another client wishes to assume ownership of calling 
 * IOEventSource. This happens when a client attempts a becomeOwner: on a
 * source which is owned by the callee of this method. 
 *
 * Returns:
 *    IO_R_SUCCESS ==> It's OK to give ownership of the source to other client.
 *		       In this case, callee no longer owns the source.
 *    IO_R_BUSY    ==> Don't transfer ownership.
 *
 * Note that this call occurs during (i.e., before the return of) a 
 * becomeOwner: call to the caller of this method.
 */
- (IOReturn)relinquishOwnershipRequest	: source;

/*
 * Method by which a client, who has registered "intent to own" via 
 * desireOwnership:client, is notified that the calling device is available.
 * Client will typically call becomeOwner: during this call.
 */
- (void)canBecomeOwner			: source;

/* Mouse event reporting */
- relativePointerEvent:(int)buttons deltaX:(int)dx deltaY:(int)dy;
- relativePointerEvent:(int)buttons
		deltaX:(int)dx
		deltaY:(int)dy
		atTime:(ns_time_t)ts;

/* Tablet event reporting */
- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity;
- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity
		withPressure:(int)pressure;
- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity
		withPressure:(int)pressure
		withAngle:(int)stylusAngle
		atTime:(ns_time_t)ts;

/* Keyboard Event reporting */
- keyboardEvent:(unsigned)eventType
		flags:(unsigned)flags
		keyCode:(unsigned)key
		charCode:(unsigned)charCode
		charSet:(unsigned)charSet
		originalCharCode:(unsigned)origCharCode
		originalCharSet:(unsigned)origCharSet
		repeat:(BOOL)repeat
		atTime:(ns_time_t)ts;
- keyboardSpecialEvent:(unsigned)eventType
		flags:(unsigned)flags
		keyCode:(unsigned)key
		specialty:(unsigned)flavor
		atTime:(ns_time_t)ts;
- updateEventFlags:(unsigned)flags;	/* Does not generate events */

/* Return current event flag values */
- (int)eventFlags;

@end /* IOEventSourceClient */

/*
 * The EventClient API lists generally useful methods exported by the Event
 * driver.  These can be used by Event sources, or other users of the event
 * system within the same address space.
 */
@protocol IOEventThread

/* 
 * Event clients may need to use an I/O thread from time to time.
 * Rather than have each instance running it's own thread, we provide
 * a callback mechanism to let all the instances share a common Event I/O
 * thread running in the IOTask space, and managed by the Event Driver.
 */
- (IOReturn)sendIOThreadMsg:	(SEL)selector	// Selector to call back on
	to		:	(id)instance	// Instance to call back
	with		:	(id)data;	// Data to pass back

/* Returns self, or nil on error. */
- sendIOThreadAsyncMsg:		(SEL)selector	// Selector to call back on
	to		:	(id)instance	// Instance to call back
	with		:	(id)data;	// Data to pass back

@end /* IOEventThread */

/*
 * Methods exported by IOEventSource objects.
 */
@protocol IOEventSourceExported

/*
 * Register for event dispatch via IOEventSourceClient protocol. Returns
 * IO_R_SUCCESS if successful, else IO_R_BUSY. The relinquishOwnershipRequest: 
 * method may be called on another client during the execution of this method.
 */
- (IOReturn)becomeOwner		: client;

/*
 * Relinquish ownership. Returns IO_R_BUSY if caller is not current owner.
 */
- (IOReturn)relinquishOwnership	: client;

/*
 * Request notification (via canBecomeOwner:) when relinquishOwnership: is
 * called. This allows one potential client to place itself "next in line"
 * for ownership. The queue is only one deep.
 */
- (IOReturn)desireOwnership	: client;

@end /* IOEventSourceExported */

/*
 * Methods exported by the EventDriver for display systems.
 *
 *	The screenRegister protocol is used by frame buffer drivers to register
 *	themselves with the Event Driver.  These methods are called in response
 *	to an _IOGetParameterInIntArray() call with "IO_Framebuffer_Register" or
 *	"IO_Framebuffer_Unregister".
 */
@protocol IOScreenRegistration

- (int) registerScreen:	(id)instance
		bounds:(Bounds *)bp
		shmem:(void **)addr
		size:(int *)size;
- (void) unregisterScreen:(int)token;

@end /* IOScreenRegistration */


/*
 * Methods exported by compliant frame buffer drivers to the Event Driver.
 *
 *	Any frame buffer driver which will be used by the Window Server should
 *	implement this protocol.  The methods in this protocol are invoked by
 *	the Event Driver on command from the Window Server and from the pointer
 *	management software.
 * 
 *	The frame field is an integer index into the driver's private array of
 *	cursor bitmaps.  It is used to select the cursor to be displayed.
 *	The token field is a small integer as returned by registerScreen, which
 *	indicates which of the driver's screens should be used for drawing.
 */
@protocol IOScreenEvents

- (port_t) devicePort;
- hideCursor: (int)token;
- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;
- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;
- setBrightness:(int)level token:(int)t;

@end	/* IOScreenEvents */

@protocol IOWorkspaceBounds
/*
 * Absolute position input devices and some specialized output devices
 * may need to know the bounding rectangle for all attached displays.
 * The following method returns a Bounds* for the workspace.  Please note
 * that the bounds are kept as signed values, and that on a multi-display
 * system the minx and miny values may very well be negative.
 */
- (Bounds *)workspaceBounds;
@end

