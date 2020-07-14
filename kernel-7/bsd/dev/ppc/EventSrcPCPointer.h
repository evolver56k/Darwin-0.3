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
 * EventSrcPCPointer.h - PC Pointer EventSrc subclass definition
 *
 * HISTORY
 * 28 Aug 1992    Joe Pasqua
 *      Created. 
 */

#ifdef	DRIVER_PRIVATE

#import <driverkit/IOEventSource.h> 
#import <driverkit/eventProtocols.h> 
#import <bsd/dev/ev_types.h>
#import <bsd/dev/ppc/PCPointer.h>
#import <bsd/dev/ppc/PCPointerDefs.h>

@interface EventSrcPCPointer : IOEventSource <ADBPointerTarget>
{
@private
    id		deviceLock;		// Locks access to this device
    id		pointerDevice;		// Underlying PC pointer device
    unsigned	lastPointerEvent;	// Time of last pointer event (ticks)
    unsigned	resolution;		// Pointer resolution
    NXMouseButton	buttonMode;	// The "handedness" of the pointer
    NXMouseScaling	pointerScaling;	// Scaling table
}

+ probe;		// Our factory method
- init;
- free;
- (IOReturn)getIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count;
- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count;

@end

#endif	/* DRIVER_PRIVATE */

