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
 * EventSrcPCKeyboard.h - PC Keyboard EventSrc subclass definition
 *
 * HISTORY
 * 28 Aug 1992    Joe Pasqua
 *      Created. 
 * 5  Aug 1993	  Erik Kay at NeXT
 *	minor changes for Event driver api changes
 */

#ifdef	DRIVER_PRIVATE

#import	<bsd/dev/evsio.h>
#import <driverkit/IOEventSource.h>
#import <driverkit/KeyMap.h>
#import	<bsd/dev/i386/PCKeyboardDefs.h>
#import	<bsd/dev/i386/kbd_entries.h>


/* Default key repeat parameters */
#define EV_DEFAULTINITIALREPEAT 500000000ULL    // 1/2 sec in nanoseconds
#define EV_DEFAULTKEYREPEAT     125000000ULL    // 1/8 sec in nanoseconds
#define EV_MINKEYREPEAT         16700000ULL     // 1/60 sec

#define	PC_NULL_KEYCODE		0x7f	// Keycode for no key event


@interface EventSrcPCKeyboard : IOEventSource <KeyMapDelegate, PCKeyboardImported>
{
@private
    id		deviceLock;		// Lock for all device access
    id		keyMap;			// KeyMap instance
    id		kbdDevice;		// The underlying keyboard object
    BOOL	ownDevice;		// YES if we own kbdDevice

    // The following fields decribe the state of the keyboard
    kbdBitVector keyState;		// bit vector of key state
    unsigned	eventFlags;		// Current eventFlags
    unsigned	deviceDependentFlags;	// The device dependent flag bits
    BOOL	alphaLock;		// YES means alpha lock is on
    BOOL	charKeyActive;		// YES means char gen. key active

    // The following fields are used in performing key repeats
    BOOL 	isRepeat;		// YES means we're generating repeat
    int		codeToRepeat;		// What we are repeating
    BOOL	calloutPending;		// YES means we've sched. a callout
    ns_time_t	lastEventTime;		// Time last event was dispatched
    ns_time_t	downRepeatTime;		// Time when we should next repeat
    ns_time_t	keyRepeat;		// Delay between key repeats
    ns_time_t	initialKeyRepeat;	// Delay before initial key repeat
}

// Methods we are overriding from superclass chain
+ probe;		// Our factory method
- init;
- free;
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out
			
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

@end

#endif	/* DRIVER_PRIVATE */
