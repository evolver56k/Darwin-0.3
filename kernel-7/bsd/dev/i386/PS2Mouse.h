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
// PS2Mouse.h - Interface to PS/2 mouse object
// 
//
// HISTORY
// 11-Aug-92    Joe Pasqua at NeXT
//      Created. 
//  5 May 93	T. Kevin Dang
//	Added resolution setting.
//

#ifdef	DRIVER_PRIVATE

#import <driverkit/IODevice.h>
#import <driverkit/IODirectDevice.h>
#import <bsd/dev/i386/PCPointer.h>
#import <bsd/dev/i386/PS2Keyboard.h>

#undef	private	// Kernel builds define this somewhere...

#define RESOLUTION	"Resolution"
#define	DEFAULTRES	100
#define INVERTED	"Inverted"

@interface PS2Mouse : PCPointer
{
@private
    PS2Keyboard *kbdDevice;	// The keyboard device we are based on
}

// Public methods
- free;

- (void)interruptHandler;

@end

#endif	/* DRIVER_PRIVATE */
