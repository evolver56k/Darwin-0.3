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
 * KeyMap.m - Generic keymap string parser and keycode translator.
 *
 * HISTORY
 * 19 June 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	minor API cleanup
 */

#import <objc/Object.h>
#import <bsd/dev/ev_keymap.h>

/*
 * Key ip/down state is tracked in a bit list.  Bits are set
 * for key-down, and cleared for key-up.  The bit vector and macros
 * for it's manipulation are defined here.
 */
#define EVK_BITS_PER_UNIT	32
#define EVK_BITS_MASK		31
#define EVK_BITS_SHIFT		5	// 1<<5 == 32, for cheap divide
#define EVK_NUNITS ((NX_NUMKEYCODES + (EVK_BITS_PER_UNIT-1))/EVK_BITS_PER_UNIT)

typedef	unsigned long	kbdBitVector[EVK_NUNITS];

#define EVK_KEYDOWN(n, bits) \
	(bits)[((n)>>EVK_BITS_SHIFT)] |= (1 << ((n) & EVK_BITS_MASK))

#define EVK_KEYUP(n, bits) \
	(bits)[((n)>>EVK_BITS_SHIFT)] &= ~(1 << ((n) & EVK_BITS_MASK))

#define EVK_IS_KEYDOWN(n, bits) \
	(((bits)[((n)>>EVK_BITS_SHIFT)] & (1 << ((n) & EVK_BITS_MASK))) != 0)

/*
 * The following protocol must be implemented by any object which is a
 * KeyMap delegate.
 */
@protocol KeyMapDelegate

- keyboardEvent	:(int)eventType
	flags	:(unsigned)flags
	keyCode	:(unsigned)keyCode
	charCode:(unsigned)charCode
	charSet	:(unsigned)charSet
	originalCharCode:(unsigned)origCharCode
	originalCharSet:(unsigned)origCharSet;

- keyboardSpecialEvent:(unsigned)eventType
	flags	 :(unsigned)flags
	keyCode	:(unsigned)keyCode
	specialty:(unsigned)flavor;

- updateEventFlags:(unsigned)flags;	/* Does not generate events */

- (unsigned)eventFlags;			// Global event flags
- (unsigned)deviceFlags;		// per-device event flags
- setDeviceFlags:(unsigned)flags;	// Set device event flags
- (BOOL)alphaLock;			// current alpha-lock state
- setAlphaLock:(BOOL)val;		// Set current alpha-lock state
- (BOOL)charKeyActive;			// Is a character gen. key down?
- setCharKeyActive:(BOOL)val;		// Note that a char gen key is down.

@end /* KeyMapDelegate */

@interface KeyMap: Object
{
@private
	NXParsedKeyMapping	curMapping;	// current system-wide keymap
	id			keyMappingLock;	// Lock guarding keyMapping.
	id			delegate;	// KeyMap delegate
	BOOL			canFreeMapping;	// YES if map can be IOFreed.
}
- initFromKeyMapping	:(const unsigned char *)mapping
		length	:(int)len
		canFree	:(BOOL)canFree;
- setKeyMapping	:(const unsigned char *)mapping
		length	:(int)len
		canFree	:(BOOL)canFree;
- (const unsigned char *)keyMapping:(int *)len;
- (int)keyMappingLength;

- setDelegate:(id)delegate;
- delegate;

- free;

-doKeyboardEvent:(unsigned)key
	direction:(BOOL)down
	keyBits	:(kbdBitVector)keyBits;

/* Private API */
- _parseKeyMapping:(const unsigned char *)mapping
	length:(int)mappingLen
	into:(NXParsedKeyMapping *)newMapping;

- (void)_calcModBit	:(int)bit
		keyBits	:(kbdBitVector)keyBits;
- (void)_doModCalc:(int)key keyBits:(kbdBitVector)keyBits;
- (void)_doCharGen:(int)keyCode
		direction:(BOOL)down;

@end

