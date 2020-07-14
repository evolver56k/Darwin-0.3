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
 * PCPointerDefs.h - PC Mouse Defs for clients
 *
 * HISTORY
 * 09 Sep 1992    Joe Pasqua
 *      Created. 
 * 11 April 1997  Simon Douglas
 *      Munged into ADB version.
 */

// TO DO:
//
// NOTES:
// * This module defines types and protocols for use by PCPointer clients.
//

#ifdef	DRIVER_PRIVATE

//
// The following structure defines an event on the pointer.
// We pass events of this form to the object registered as
// our event target.
//
typedef struct _t_PointerEvent {
    ns_time_t timeStamp; // When did this event happen
    int buttonCount;
    boolean_t b0;// TRUE if left button down
    boolean_t b1;
    boolean_t b2;
    boolean_t b3;
    int dx;
    int dy;
} PointerEvent;


//
// The following protocol must be implemented by any class that wishes to
// receive pointer events. When such a class wishes to receive events it
// invokes the setEventTarget: method of the PCPointer instance specifying
// itself as the target. When the PCPointer instance has an event to dispatch,
// it will invoke the dispatchMouseEvent: method defined in this protocol.
//
@protocol ADBPointerTarget

- (void)dispatchPointerEvent:(PointerEvent *)event;

@end

#endif	/* DRIVER_PRIVATE */
