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
	NXReadOnlyString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXStringPrivate.h"

@implementation NXReadOnlyString

+ alloc
{
    return [super allocFromZone:stringZone];
}
 
+ allocFromZone:(NXZone *)zone
{
    return [super allocFromZone:stringZone];
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    return [self initFromCharactersNoCopy:chars length:len freeWhenDone:YES];
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len freeWhenDone:(BOOL)flag
{
    [super initFromCharactersNoCopy:chars length:len];
    _flags.notCopied = !flag;
    _flags.refs = 1;
    return self;
}

- (unichar *)allocateCharacterBuffer:(unsigned)nChars
{
    return NX_CHARALLOC(stringZone, nChars);
}

- copyFromZone:(NXZone *)zone
{
    if (_flags.refs < NX_STRING_MAXREFS) _flags.refs += 1;
    return self;
}

- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone
{
    if (RNGLOC(range) + RNGLEN(range) > _length) BOUNDSERROR;
    if (RNGLOC(range) == 0 && RNGLEN(range) == _length) {
	return [self copyFromZone:zone];
    } else {
	id newInstance = [[NXReadOnlySubstring allocFromZone:zone] initFromCharactersNoCopy:characters + RNGLOC(range) length:RNGLEN(range) freeWhenDone:NO];
	((struct {@defs(NXReadOnlySubstring);} *)newInstance)->_referenceString = [self copy];
	return newInstance;
    }
}

- mutableCopyFromZone:(NXZone *)zone
{
    id newInstance = [NXReadWriteString allocFromZone:zone];
    ((struct {@defs(NXReadWriteString);} *)newInstance)->actualString = [self copyFromZone:zone];
    return newInstance;
}

- free
{
    if ((_flags.refs < NX_STRING_MAXREFS) && (--_flags.refs == 0)) {
        if (_flags.notCopied) {
            characters = NULL;	// So the super won't attempt to free the buffer...
        }
        return [super free];
    } else {
	return nil;
    }
}

// We want to make sure the object itself comes out of the string zone
// and not some random area which might be deallocated later...

- finishUnarchiving
{
    id actual = nil; 
    if ([self zone] != stringZone) {
        actual = object_copyFromZone (self, 0, stringZone);
        object_dispose(self);
    }
    return actual;
}

- write:(NXTypedStream *)s
{
    unsigned int refs = _flags.refs;
    [super write:s];
    NXWriteTypes (s, "i", &refs);
    return self;
}

- read:(NXTypedStream *)s
{
    unsigned int refs;
    [super read:s];
    NXReadTypes (s, "i", &refs);
    _flags.refs = refs;
    return self;
}

@end
#endif