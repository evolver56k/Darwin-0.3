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

// 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
//
// PPCKeyboard.h - Interface to PC keyboard object
// 
//
// HISTORY
// 11-Aug-92    Joe Pasqua at NeXT
//      Created. 
//

#ifdef	DRIVER_PRIVATE

#import "kernobjc.h"
#import <driverkit/IODevice.h>
#import <driverkit/IODirectDevice.h>
#import <bsd/dev/ppc/PCKeyboardDefs.h>

#if	KERNOBJC

@interface PPCKeyboard : IODirectDevice <PCKeyboardExported>
{
@private
    enum {NOT_WAITING, SET_LEDS, DATA_ACK} pendingAck;
    unsigned char	lastSent;
    int			pendingLEDVal;
    unsigned		interfaceId;    // see NXEventSystemDevice struct
    unsigned		handlerId;      // in ev_types.h

    // Info about the mouse object we are associated with
//    id			mouseObject;
//   port_t		mouseIntPort;
    
    // Device ownership stuff
    id			_owner;
    id			_desiredOwner;
    id			_ownerLock;
    			// NXLock; protects _owner and	desiredOwner
}

/* Had to make it public as interrupt handler uses it */
extern unsigned char	extendCount;

// Public methods
- (int)interfaceId;
- (int)handlerId;
- (void)interruptHandler;

// Description:	Return the device port of the object.

- (void)setAlphaLockFeedback:(BOOL)locked;

- (IOReturn)becomeOwner		: client;
- (IOReturn)relinquishOwnership	: client;
- (IOReturn)desireOwnership	: client;

@end

#endif	/* KERNOBJC */

extern PCKeyboardEvent *StealKeyEvent();
// Description:	Call this routine to steal a keyboard event directly
//		from the hardware. Must be called from interrupt level.

#endif	/* DRIVER_PRIVATE */

