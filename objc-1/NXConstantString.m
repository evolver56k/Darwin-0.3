/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	NXConstantString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXStringPrivate.h"

@implementation NXConstantString

// We allow allocFromZone:, since this what typedstreams will use when
// unarchiving.  However, we make the designated initializer raise.

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    return [self doesNotRecognize:_cmd];
}

// Simply returns self, since NXConstantString's cannot be freed.
// Beware that NXConstantString's will disappear when dynamically loaded
// code is unloaded!

- copyFromZone:(NXZone *)zone
{
    return self;
}

// Ignore free messages

- free
{
    return nil;
}

- _reallyFree
{
    return [super free];
}

// When unarchiving, replace ourselves with an NXReadOnlyString.

// Ugh.  We would rather write out an NXReadOnlyString in the first place,
// but because of how typedstreams work this won't work at all.
// By the time we recieve the write method, our class name has already
// been written out.  That code should be moved to [Object write]
// so that it only happens when [super write] is called (mself - 10/8/91).

- finishUnarchiving
{
    NXString *newString = [NXReadOnlyString alloc] ;
    
    [newString initFromCharacters:characters length:_length];
    [self _reallyFree];
    return newString;
}

@end
#endif