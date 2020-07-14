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
 * PCKeyboardDefs.h - PC Keyboard Defs for clients
 *
 * HISTORY
 * 09 Sep 1992    Joe Pasqua
 *      Created. 
 */

// TO DO:
//
// NOTES:
// * This module defines types and protocols for use by PCKeyboard clients.

#ifdef	DRIVER_PRIVATE

#import <kernserv/clock_timer.h>

typedef struct _t_PCKeyboardEvent {
    ns_time_t timeStamp;	// When did this event happen
    unsigned keyCode;		// Which key was involved
    BOOL goingDown;		// YES -> it was pressed, NO -> it was released
} PCKeyboardEvent;


//
// The following protocol is implemented by PCKeyboard objects. Clients
// of the PCKeyboard can invoke the methods below to indicate their
// ownership of the PCKeyboard object.
//
@protocol PCKeyboardExported

- (IOReturn)becomeOwner		: client;
// Description:	Register for event dispatch via device-specific protocol.
//		(See the dispatchKeyboardEvent method)
//		Returns IO_R_SUCCESS if successful, else IO_R_BUSY. The
//		relinquishOwnershipRequest: method may be called on another
//		client during the execution of this method.

- (IOReturn)relinquishOwnership	: client;
// Description:	Relinquish ownership. Returns IO_R_BUSY if caller is not
//		current owner.


- (IOReturn)desireOwnership	: client;
// Description:	Request notification (via canBecomeOwner:) when
//		relinquishOwnership: is called. This allows one potential
//		client to place itself "next in line" for ownership. The
//		queue is only one deep.

@end	// PCKeyboardExported


//
// The following protocol is implemented by clients of the PCKeyboard
// object. A PCKeyboard instance will invoke these methods to indicate
// that an event has come in or that a change in ownership can happen.
//
@protocol PCKeyboardImported

- (void)dispatchKeyboardEvent:(PCKeyboardEvent *)event;
// Description:	Called when the keyboard object receives a new keyboard
//		Event. The event structure should not be freed by the callee.


- (IOReturn)relinquishOwnershipRequest	: device;
// Description:	Called when another client wishes to assume ownership of
//		calling kbd Device. This happens when a client attempts a
//		becomeOwner: on a device which is  owned by the callee of
//		this method. 
// Returns:
//		IO_R_SUCCESS ==> It's OK to give ownership of the device to
//				 other client. In this case, callee no longer
//				 owns the device.
//		IO_R_BUSY    ==> Don't transfer ownership.


- (IOReturn)canBecomeOwner			: device;
// Description:	Method by which a client, who has registered "intent to own"
//		via desireOwnership:client, is notified that the calling
//		device is available. Client will typically call becomeOwner:
//		during this call.

@end	// PCKeyboardImported

#endif	/* DRIVER_PRIVATE */
