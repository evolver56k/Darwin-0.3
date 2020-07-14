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
	NXSimpleReadOnlyString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXStringPrivate.h"

@implementation NXSimpleReadOnlyString

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    [super initFromCharactersNoCopy:chars length:len];
    characters = (unichar *)chars;
    _length = len;
    return self;
}

- (unichar)characterAt:(unsigned)loc
{
    if (loc >= _length) BOUNDSERROR;
    return characters[loc];
}

- (unsigned)length
{
    return _length;
}

- (void)getCharacters:(unichar *)buffer range:(NXRange)range
{
    if (RNGLOC(range) + RNGLEN(range) > _length) BOUNDSERROR;
    NX_CHARCOPY(characters + RNGLOC(range), buffer, RNGLEN(range));
}

- (void)getCString:(char *)buffer maxLength:(unsigned)bytes range:(NXRange)range remainingRange:(NXRange *)leftover
{
#if CHARS_ARE_EIGHT_BIT
    NXRange desiredRange = range;
    unsigned cnt;
    if (RNGLOC(range) + RNGLEN(range) > _length) BOUNDSERROR;
    if (RNGLEN(range) > bytes) RNGLEN(range) = bytes;
    for (cnt = 0; cnt < RNGLEN(range); cnt++) {
        unichar ch = characters[RNGLOC(range) + cnt];
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

- (NXComparisonResult)compare:string mask:(unsigned int)options table:(void *)table
{
    unsigned ownLength = [self length], otherLength = [string length];
    static Class simpleReadOnlyStringClass = Nil;
    
    if (simpleReadOnlyStringClass == Nil) simpleReadOnlyStringClass = [NXSimpleReadOnlyString class];

    if ([string isKindOf:simpleReadOnlyStringClass]) {
	return NXCompareCharacters(characters, ((NXSimpleReadOnlyString *)string)->characters, ownLength, otherLength, options, NULL);
    } else {
	unichar otherBuffer[COMPARELENGTH];
	NXRange ownRange = {0, 0}, otherRange = {0, 0};
	NXComparisonResult res = NX_OrderedSame;
	while (1) {
	    RNGLEN(ownRange) = MIN(ownLength - RNGLOC(ownRange), COMPARELENGTH);	
	    RNGLEN(otherRange) = MIN(otherLength - RNGLOC(otherRange), COMPARELENGTH);	
	    if (RNGLEN(ownRange) == 0 && RNGLEN(otherRange) == 0) return NX_OrderedSame;
	    [string getCharacters:otherBuffer range:otherRange];
	    if ((res = NXCompareCharacters(characters + RNGLOC(ownRange), otherBuffer, RNGLEN(ownRange), RNGLEN(otherRange), options, NULL)) != NX_OrderedSame) return res;
	    RNGLOC(ownRange) += RNGLEN(ownRange);
	    RNGLOC(otherRange) += RNGLEN(otherRange);
	}
    }
}

- (NXRange)findString:(NXString *)findStr range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    unsigned findStrLen = [findStr length];	// fromLoc and toLoc are inclusive
    NXRange range = {NX_STRING_NOT_FOUND, 0};
    unichar tmpBuf[MAXTMPBUFFERLEN], *findBuf;

    if (RNGLOC(fRange) + RNGLEN(fRange) > _length) BOUNDSERROR;

    if (findStrLen > RNGLEN(fRange)) {	// ??? This can't be here for correct Unicode compares
	return range;
    }
    
    findBuf = (findStrLen > MAXTMPBUFFERLEN) ? NX_CHARALLOC(NXDefaultMallocZone(), findStrLen) : tmpBuf;
    [findStr getCharacters:findBuf];

    range = NXFindCharacters (findBuf, characters + RNGLOC(fRange), findStrLen, RNGLEN(fRange), options, table);

    if (RNGLOC(range) != NX_STRING_NOT_FOUND) {
	RNGLOC(range) += RNGLOC(fRange);
    }

    if (findBuf != tmpBuf) {
	free(findBuf);
    }

    return range;
}

- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    NXRange range;

    if (RNGLOC(fRange) + RNGLEN(fRange) > _length) BOUNDSERROR;

    range = NXFindCharacters (&ch, characters + RNGLOC(fRange), 1, RNGLEN(fRange), options, table);

    return RNGLOC(range) + ((RNGLOC(range) != NX_STRING_NOT_FOUND) ? RNGLOC(fRange) : 0);
}

- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    int step;
    unsigned fromLoc, toLoc, cnt;	// fromLoc and toLoc are inclusive
    BOOL found = NO, done = NO;

    if (RNGLOC(fRange) + RNGLEN(fRange) > _length) BOUNDSERROR;

    if (options & NX_BACKWARDS_SEARCH) {
        fromLoc = RNGLOC(fRange) + RNGLEN(fRange) - 1;
        toLoc = RNGLOC(fRange);
    } else {       
        fromLoc = RNGLOC(fRange);
        toLoc = RNGLOC(fRange) + RNGLEN(fRange) - 1;
    }

    step = (fromLoc <= toLoc) ? 1 : -1;
    cnt = fromLoc;
 
    do {
	if ([set characterIsMember:characters[cnt]]) {
	    done = found = YES;
        } else if (cnt == toLoc) {
	    done = YES;
	} else {
            cnt += step;
        }
    } while (!done);

    return found ? cnt : NX_STRING_NOT_FOUND;
}

- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone
{
    if (RNGLOC(range) + RNGLEN(range) > _length) BOUNDSERROR;
    if (RNGLOC(range) == 0 && RNGLEN(range) == _length) {
	return [[self class] copyFromZone:zone];
    } else {
	return [[[self class] allocFromZone:zone] initFromCharacters:characters + RNGLOC(range) length:RNGLEN(range)];
    }
}

- (unsigned)hash
{
    return NXHashCharacters(characters, [self length]);
}

- copyFromZone:(NXZone *)zone
{
    NXSimpleReadOnlyString *newInstance = [super copyFromZone:zone];
    newInstance->_length = _length;
    newInstance->characters = [newInstance allocateCharacterBuffer:_length];
    NX_CHARCOPY(characters, newInstance->characters, _length);
    return newInstance;
}

- free
{
    if (characters) {
	free (characters);
    }
    return [super free];
}

- write:(NXTypedStream *)s
{
    [super write:s];
    NXWriteType (s, "i", &_length);
    if (_length > 0) {
	NXWriteArray (s, DATATYPEFORCHAR, _length, characters);
    }
    return self;
}

- read:(NXTypedStream *)s
{
    [super read:s];
    NXReadType (s, "i", &_length);
    if (_length > 0) {
	characters = [self allocateCharacterBuffer:_length];
	NXReadArray (s, DATATYPEFORCHAR, _length, characters);
    } else {
        characters = NULL;
    }
    return self;
}

@end
#endif