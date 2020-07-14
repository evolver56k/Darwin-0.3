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
	NXReadWriteString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXStringPrivate.h"

@implementation NXReadWriteString

#define actStr ((struct {@defs(NXReadOnlyString);} *)actualString)

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    return [self initFromCharactersNoCopy:chars length:len freeWhenDone:YES];
}

// To allow for cheap creation of empty read/write strings (with init, say),
// we keep around an empty readonly string and hand out copies of it whenever
// necessary.  A mutex could be used in the code below to prevent the one-time leak
// of one or more NXReadOnlyStrings. However, the chances of the leak are so small 
// it's not clear we should bother.

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len freeWhenDone:(BOOL)flag
{
    static NXReadOnlyString *emptyReadOnlyString = nil;
    [super initFromCharactersNoCopy:chars length:len];
    if (len == 0 && emptyReadOnlyString) {
	actualString = [emptyReadOnlyString copy];
    } else {
	actualString = [[NXReadOnlyString allocFromZone:[self zone]] initFromCharactersNoCopy:len ? chars : NULL length:len freeWhenDone:flag];
	if (len == 0 && !emptyReadOnlyString) {		// ??? mutex could be used to prevent possible leak
	    emptyReadOnlyString = [actualString copy];
	}
    }
    return self;
}

- (unsigned)length
{
    return actStr->_length;
}

- (unichar)characterAt:(unsigned)loc
{
    if (loc >= actStr->_length) BOUNDSERROR;
    return actStr->characters[loc];
}

- (unichar *)allocateCharacterBuffer:(unsigned)nChars
{
    return NX_CHARALLOC(stringZone, nChars);
}

- (void)getCharacters:(unichar *)buffer range:(NXRange)range
{
    if (RNGLOC(range) + RNGLEN(range) > actStr->_length) BOUNDSERROR;
    NX_CHARCOPY(actStr->characters + RNGLOC(range), buffer, RNGLEN(range));
}

- (void)getCString:(char *)buffer maxLength:(unsigned)bytes range:(NXRange)range remainingRange:(NXRange *)leftover
{
#if CHARS_ARE_EIGHT_BIT
    NXRange desiredRange = range;
    unsigned cnt;
    if (RNGLOC(range) + RNGLEN(range) > actStr->_length) BOUNDSERROR;
    if (RNGLEN(range) > bytes) RNGLEN(range) = bytes;
    for (cnt = 0; cnt < RNGLEN(range); cnt++) {
        unichar ch = actStr->characters[RNGLOC(range) + cnt];
	buffer[cnt] = (ch > 0x0ff) ? NX_UNREPRESENTABLE_CHARACTER : ch;
    }
    buffer[RNGLEN(range)] = 0;
    if (leftover) {
	RNGLOC(*leftover) = RNGLOC(desiredRange) + RNGLEN(range);
	RNGLEN(*leftover) = RNGLEN(desiredRange) - RNGLEN(range);
    }
#else
#warning getCString:maxLength:range:remainingRange: not implemented for Unicode
    _NXStringErrorRaise (NXStringInternalError, "getCString:maxLength:range:remainingRange: not implemented for Unicode");
#endif
}

// ??? Most of these methods simply forward the message onto the actualString, so we
// might just want to use forwarding here...

- (NXComparisonResult)compare:string mask:(unsigned int)options table:(void *)table
{
    return [actualString compare:string mask:options table:table];
}

- (NXRange)findString:(NXString *)findStr range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    return [actualString findString:findStr range:fRange mask:options table:table];
}

- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    return [actualString findCharacter:ch range:fRange mask:options table:NULL];
}

- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    return [actualString findOneOf:set range:fRange mask:options table:table];
}

- (unsigned)hash
{
    return [actualString hash];
}

- (void)replaceCharactersInRange:(NXRange)range withString:(NXString *)string
{
    unsigned int newLength, len = [self length], otherLength = [string length];
    unichar *newBuffer;
    NXRange strRange = {0, otherLength};

    if (RNGLOC(range) + RNGLEN(range) > len) BOUNDSERROR;   

    newLength = len + otherLength - RNGLEN(range);
    newBuffer = [self allocateCharacterBuffer:newLength];

    // Copy the three chunks into the new buffer

    NX_CHARCOPY (actStr->characters, newBuffer, RNGLOC(range));
    [string getCharacters:(newBuffer + RNGLOC(range)) range:strRange];
    NX_CHARCOPY (actStr->characters + (RNGLOC(range) + RNGLEN(range)), newBuffer + RNGLOC(range) + otherLength, (len - (RNGLOC(range) + RNGLEN(range))));

    // Now the new buffer is created. See what we do with the old one...

    if (actStr->_flags.refs > 1) {
        actStr->_flags.refs -= 1;
        actualString = [[NXReadOnlyString allocFromZone:[self zone]] initFromCharactersNoCopy:newBuffer length:newLength freeWhenDone:YES];
    } else {
        if (!actStr->_flags.notCopied) {
            free (actStr->characters);
        }
        actStr->_length = newLength;
        actStr->characters = newBuffer;
    }
}

- copyFromZone:(NXZone *)zone
{
    NXReadWriteString *newInstance = [super copyFromZone:zone];
    newInstance->actualString = [actualString copy];
    return newInstance;
}

- immutableCopyFromZone:(NXZone *)zone
{
    return [actualString copyFromZone:zone];
}

- free
{
    [actualString free];
    return [super free];
}

- write:(NXTypedStream *)s
{
    [super write:s];
    NXWriteObject (s, actualString);
    return self;
}

- read:(NXTypedStream *)s
{
    [super read:s];
    actualString = NXReadObject (s);
    return self;
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


@end
#endif