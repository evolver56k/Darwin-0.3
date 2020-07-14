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
	NXString.m
	Copyright 1991 NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <mach/mach.h>

#import "NXStringPrivate.h"
#import <streams/streams.h>
#import <streams/streamsextra.h>
#import <stdio.h>
#import "error.h"

/* Separate zone for ref counted strings... */
static NXZone *theStringZone = NULL;

NXZone *_NXStringZone (void)
{
  return theStringZone;
}

static void objectPrintfProc(NXStream *stream, void *item, void *procData)
{
    if ([(id)item respondsTo:@selector(writeToStream:)]) {
	[(id)item writeToStream:stream];
    } else {
	NXPutc(stream, '%'); NXPutc(stream, '@');
    }
}

void _NXStringErrorRaise (int errorCode, const char *errorMsg)
{
    extern void _NXLogError(const char *format, ...);
    _NXLogError( "NXString error %d: %s", errorCode, errorMsg);
    NX_RAISE (errorCode, errorMsg, 0);
}


@implementation NXString

+ initialize
{
    if (theStringZone == NULL) {
#if 1
	/* Using a separate string zone seems to hurt more than it helps now.
	   ??? However, we need to solve the copyFromZone: problem... */
	theStringZone = NXDefaultMallocZone();
#else
	theStringZone = NXCreateZone(vm_page_size, vm_page_size, YES);
	NXNameZone (theStringZone, "String Zone");
#endif
	NXRegisterPrintfProc('@', objectPrintfProc, NULL);
    }
    return self;
}

- (unichar *)allocateCharacterBuffer:(unsigned)nChars
{
    return NX_CHARALLOC([self zone], nChars);
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    return [super init];
}

- init
{
    return [self initFromCharactersNoCopy:NULL length:0];
}

- initFromCharacters:(const unichar *)chars length:(unsigned)len;
{
    unichar *buffer = [self allocateCharacterBuffer:len];
    NX_CHARCOPY (chars, buffer, len);
    return [self initFromCharactersNoCopy:buffer length:len];
}

- initFromString:string
{
    NXRange range = {0, [string length]};
    return [self initFromString:string range:range];
}

- initFromString:string range:(NXRange)range
{
    unichar *buffer = [self allocateCharacterBuffer:RNGLEN(range)];
    [string getCharacters:buffer range:range];
    return [self initFromCharactersNoCopy:buffer length:RNGLEN(range)];
}

// The starting size for the buffer when reading from a stream, if no other clues about the desired size
// or the stream size are provided...

#define INITIALLENGTH 16

- initFromStream:(NXStream *)stream untilOneOf:(NXCharacterSet *)set maxLength:(unsigned int)maxLen
{
#if CHARS_ARE_EIGHT_BIT
    return [self initFromCStringStream:stream untilOneOf:set maxLength:maxLen];
#else
#warning initFromStream:untilOneOf:maxLength: not implemented for Unicode
    _NXStringErrorRaise (NXStringInternalError, "initFromStream:untilOneOf:maxLength: not implemented for Unicode");
    return nil;
#endif
}

- initFromCStringStream:(NXStream *)stream untilOneOf:(NXCharacterSet *)set maxLength:(unsigned int)maxLen
{
    volatile unsigned int readLen = 0;
    volatile unsigned int charsLen = MIN(maxLen, INITIALLENGTH);
    unichar *volatile chars = NULL;
    int chRead;
   
    if (stream->flags & NX_CANSEEK) {	// Try to fine tune the initial buffer size

	long curLoc = NXTell(stream);
	if (!set) {		// If no set, then length is determined by maxLen or stream size
	    NXSeek (stream, 0, NX_FROMEND);
	    charsLen = (NXTell(stream) - curLoc);
	    if (charsLen > maxLen) charsLen = maxLen;
	} else {		// Otherwise we have to look at every character to determine how many we'll read
	    charsLen = 0;
	    while (!NXAtEOS(stream) && (charsLen < maxLen) && ![set characterIsMember:(unichar)NXGetc(stream)]) charsLen++;
	}
	NXSeek (stream, curLoc, NX_FROMSTART);
	
    }

    if ((chars = [self allocateCharacterBuffer:charsLen])) {
    
	NX_DURING
    
	    // Seems like AtEOS is set after we attempt to read the last character.
	    // ??? Thus the !NXAtEOS is pretty useless down here...
	
	    while (!NXAtEOS(stream) && (readLen < maxLen) && ((chRead = NXGetc(stream)) != EOF)) {
		unichar ch = (unichar)chRead;
		if (set && [set characterIsMember:ch]) {
		    NXUngetc(stream);
		    break;
		}
		if (charsLen == readLen) {
		    unichar *newChars;
		    if ((charsLen *= 2) > maxLen) charsLen = maxLen;
		    newChars = [self allocateCharacterBuffer:charsLen];
		    NX_CHARCOPY ((unichar *)chars, newChars, readLen);
		    free ((unichar *)chars);
		    chars = newChars;
		}
		chars[readLen++] = ch;
	    }
    
	NX_HANDLER

	    free ((unichar *)chars);
	    NX_RERAISE ();
    
	NX_ENDHANDLER

    }

    return [self initFromCharactersNoCopy:(unichar *)chars length:readLen];
}

- initFromFormat:(NXString *)format, ...
{
    va_list argList;

    va_start (argList, format);
    self = [self initFromFormat:format withArgList:argList];
    va_end (argList);
    
    return self;
}

- initFromFormat:(NXString *)format withArgList:(va_list)argList
{
    int maxLen, len;
    unsigned formatLen = [format cStringLength] + 1;	// Including terminating zero
    char *bytes;
    NXStream *stream = NXOpenSmallMemory(NX_WRITEONLY);
    char tmpBuf[MAXTMPBUFFERLEN], *formatChars;

    formatChars = (formatLen > MAXTMPBUFFERLEN) ? NXZoneMalloc(NXDefaultMallocZone(), formatLen * sizeof(char)) : tmpBuf;
    [format getCString:formatChars];
    NXVPrintf(stream, formatChars, argList);
    NXFlush(stream);
    NXGetMemoryBuffer(stream, &bytes, &len, &maxLen);
    self = [self initFromCString:bytes length:len];
    NXCloseMemory(stream, NX_FREEBUFFER);
    if (formatChars != tmpBuf) free(formatChars);

    return self;
}


/* Byte oriented methods */

- initFromCString:(const char *)bytes
{
    return [self initFromCString:bytes length:strlen(bytes)];
}

- initFromCString:(const char *)bytes length:(unsigned)length
{
    unichar *buffer = NULL;
    unsigned cnt;

    if (length && bytes && *bytes) {
        buffer = [self allocateCharacterBuffer:length];
        for (cnt = 0; cnt < length; cnt++) {
            buffer[cnt] = bytes[cnt];
        }
    }
    return [self initFromCharactersNoCopy:buffer length:length];
}

- (void)getCString:(char *)buffer
{
    [self getCString:buffer maxLength:NX_MAX_STRING_LENGTH range:(NXRange){0, [self length]} remainingRange:NULL];
}

- (void)getCString:(char *)buffer maxLength:(unsigned)bytes
{
    [self getCString:buffer maxLength:bytes range:(NXRange){0, [self length]} remainingRange:NULL];
}    

- (void)getCString:(char *)buffer maxLength:(unsigned)bytes range:(NXRange)range remainingRange:(NXRange *)leftover
{
#if CHARS_ARE_EIGHT_BIT
    NXRange desiredRange = range;

    if (RNGLEN(range) > bytes) RNGLEN(range) = bytes;
    [self getCharacters:(unichar *)buffer range:range];
    buffer[RNGLEN(range)] = 0;

    if (leftover) {
	RNGLOC(*leftover) = RNGLOC(desiredRange) + RNGLEN(range);
	RNGLEN(*leftover) = RNGLEN(desiredRange) - RNGLEN(range);
    }
#else
#warning getCString:maxLength:range:remainingRange: not implemented for Unicode
    _NXStringErrorRaise (NXStringInternalError, "getCString:maxLength:range:remainingRange: not implemented for Unicode");
#if 0
    NXRange processedRange;
    unichar tmpBuf[MAXTMPBUFFERLEN];
    unsigned uCnt = 0, cCnt = 0;
    
    if (RNGLOC(range) + RNGLEN(range) > [self length]) BOUNDSERROR;

    processedRange = range;
    if (RNGLEN(processedRange) > MAXTMPBUFFERLEN) RNGLEN(processedRange) = MAXTMPBUFFERLEN;

    [self getCharacters:tmpBuf range:range];

    while (uCnt < RNGLEN(range)) {
	unichar ch = uStr[RNGLOC(range) + uCnt];
	if (cCnt < bytes) {	// ??? This check will have to be smarter
	    buffer[cCnt] = (ch > 0x0ff) ? NX_BADBYTE : ch;
	} else {
	    break;
	}
	uCnt++;
	cCnt++;
    }
    buffer[cCnt] = 0;
#endif
#endif
}

- (unsigned)length
{
    [self subclassResponsibility:_cmd];
    return 0;
}

- (unsigned)cStringLength
{
#if CHARS_ARE_EIGHT_BIT
    return [self length];
#else
#warning cStringLength not implemented for Unicode
    _NXStringErrorRaise (NXStringInternalError, "cStringLength not implemented for Unicode");
    return 0;
#endif
}

- (char *)cStringCopy
{
    char *str = NX_BYTEALLOC (NXDefaultMallocZone(), [self cStringLength] + 1);
    [self getCString:str];
    return str;
}

- (NXAtom)uniqueCStringCopy
{
    char tmpBuf[MAXTMPBUFFERLEN], *str;
    NXAtom unique;
    unsigned int len = [self cStringLength] + 1;
    
    str = (len > MAXTMPBUFFERLEN) ? NX_BYTEALLOC(NXDefaultMallocZone(), len) : tmpBuf;
    [self getCString:str];
    unique = NXUniqueString (str);
    if (str != tmpBuf) free (str);    

    return unique;
}

- (unichar)characterAt:(unsigned)pos
{
    [self subclassResponsibility:_cmd];
    return 0;
}

- (void)getCharacters:(unichar *)buffer range:(NXRange)range
{
    unsigned int cnt;
    
    for (cnt = 0; cnt < RNGLEN(range); cnt++) {
        buffer[cnt] = [self characterAt:RNGLOC(range) + cnt];
    }
}

- (void)getCharacters:(unichar *)buffer
{
    NXRange range = {0, [self length]};
    [self getCharacters:buffer range:range];
}

/* Comparision and find stuff */

// Compare the two character strings (whose lengths are given in firstLen & secondLen)
// according to the flags in flagMask.

// ??? Unicode string compares work differently: As characters are compared and found to be unequal,
// they are normalized. Thus if "o" and "O" are compared and found unequal, they are normalized
// (depending on flagMask), and compared again. This normalization might include case conversion,
// floating diacritics, etc.

NXComparisonResult NXCompareCharacters (const unichar *first, const unichar *second, unsigned firstLen, unsigned secondLen, unsigned flagMask, void *table)
{
    unsigned cnt = 0, compareLen = MIN(firstLen, secondLen);
    BOOL caseInsensitive = (flagMask & NX_CASE_INSENSITIVE) ? YES : NO;

    while (cnt < compareLen) {
	unichar ch1 = first[cnt], ch2 = second[cnt];
	if (caseInsensitive) {	// Don't worry about unrolling this into two loops...
	    if (ch1 >= 'a' && ch1 <= 'z') ch1 += 'A' - 'a';
	    if (ch2 >= 'a' && ch2 <= 'z') ch2 += 'A' - 'a';
	}
	if (ch1 < ch2) return NX_OrderedAscending;
	else if (ch1 > ch2) return NX_OrderedDescending;
	else cnt++;
    }
    return (firstLen < secondLen) ? NX_OrderedAscending : ((firstLen > secondLen) ? NX_OrderedDescending : NX_OrderedSame);
}

// Find findStr of len findStrLen in inStr of inStrLen. If flagMask contains NX_BACKWARDS_SEARCH,
// then look at inStr starting from the last valid character.
// See Unicode related warning under NXCompareCharacters().

NXRange NXFindCharacters (const unichar *findStr, const unichar *inStr, unsigned findStrLen, unsigned inStrLen, unsigned flagMask, void *table)
{
    int step;
    unsigned fromLoc, toLoc, cnt;	// fromLoc and toLoc are inclusive
    BOOL found = NO, done = NO;
    BOOL caseInsensitive = (flagMask & NX_CASE_INSENSITIVE) ? YES : NO;
    NXRange range = {NX_STRING_NOT_FOUND, 0};

    if (findStrLen > inStrLen) {	// ??? This can't be here for correct Unicode compares
	return range;
    }
    
    if (flagMask & NX_BACKWARDS_SEARCH) {
        fromLoc = inStrLen - findStrLen;	// Inclusive
        toLoc = 0;
    } else {       
        fromLoc = 0;
        toLoc = inStrLen - findStrLen;		// Inclusive
    }

    step = (fromLoc <= toLoc) ? 1 : -1;
    cnt = fromLoc;
    do {
        unsigned int chCnt;
        for (chCnt = 0; chCnt < findStrLen; chCnt++) {
	    unichar ch1 = findStr[chCnt], ch2 = inStr[chCnt + cnt];
	    if (caseInsensitive) {
		if (ch1 >= 'a' && ch1 <= 'z') ch1 += 'A' - 'a';
		if (ch2 >= 'a' && ch2 <= 'z') ch2 += 'A' - 'a';
	    }
            if (ch1 != ch2) {
                break;
            }
        }
        if (chCnt == findStrLen) {
            found = done = YES;
	    RNGLOC(range) = cnt;
	    RNGLEN(range) = findStrLen;
        } else if (cnt == toLoc) {
	    done = YES;
	} else {
            cnt += step;
        }
    } while (!done);

    return range;
}

unsigned NXHashCharacters(const unichar *characters, unsigned length)
{
    unsigned int h = length, cnt;

    if (length > MAXSTRINGLENFORHASHING) {
	length = MAXSTRINGLENFORHASHING;
    }
    
    for (cnt = 0; cnt < length; cnt++) {
	h <<= 4;
	h += (unsigned int)characters[cnt];
	h ^= (h >> 24);
    }
    
    return h;
}

- (BOOL)isEqual:string
{
    static Class stringClass = Nil;
    
    if (stringClass == Nil) stringClass = [NXString class];
    
    return (self == string) ||
	([string isKindOf: stringClass] &&
#if CHARS_ARE_EIGHT_BIT
	 ([self cStringLength] == [string cStringLength]) &&
#endif
	 ([self compare:string] == NX_OrderedSame));
}

- (NXComparisonResult)compare:string
{
    return [self compare:string mask:0 table:NULL];
}

- (NXComparisonResult)compare:string mask:(unsigned int)options table:(void *)table
{
    if (![string isKindOf:[NXString class]]) {
	return NX_OrderedAscending;	// ???
    } else {
	unsigned ownLength = [self length], otherLength = [string length];
	unichar ownBuffer[COMPARELENGTH], otherBuffer[COMPARELENGTH];
	NXRange ownRange = {0, 0}, otherRange = {0, 0};
	NXComparisonResult res = NX_OrderedSame;
    
	while (1) {
	    RNGLEN(ownRange) = MIN(ownLength - RNGLOC(ownRange), COMPARELENGTH);	
	    RNGLEN(otherRange) = MIN(otherLength - RNGLOC(otherRange), COMPARELENGTH);	
	    if (RNGLEN(ownRange) == 0 && RNGLEN(otherRange) == 0) return NX_OrderedSame;
	    [self getCharacters:ownBuffer range:ownRange];
	    [string getCharacters:otherBuffer range:otherRange];
	    if ((res = NXCompareCharacters(ownBuffer, otherBuffer, RNGLEN(ownRange), RNGLEN(otherRange), options, table)) != NX_OrderedSame) return res;
	    RNGLOC(ownRange) += RNGLEN(ownRange);
	    RNGLOC(otherRange) += RNGLEN(otherRange);
	}
    }
}

- (NXRange)findString:(NXString *)string
{
    NXRange range = {0, [self length]};
    return [self findString:string range:range mask:0 table:NULL];
}

- (NXRange)findString:(NXString *)findStr range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    int step;
    unsigned fromLoc, toLoc, cnt, findStrLen, len;	// fromLoc and toLoc are inclusive
    BOOL found = NO, done = NO;
    BOOL caseInsensitive = (options & NX_CASE_INSENSITIVE) ? YES : NO;
    NXRange range = {NX_STRING_NOT_FOUND, 0};
    unichar tmpBuf[MAXTMPBUFFERLEN], *findBuf;

    findStrLen = [findStr length];
    len = [self length];

    if (findStrLen > RNGLEN(fRange)) {	// ??? This can't be here for correct Unicode compares
	return range;
    }
    
    findBuf = (findStrLen > MAXTMPBUFFERLEN) ? NX_CHARALLOC(NXDefaultMallocZone(), findStrLen) : tmpBuf;
    [findStr getCharacters:findBuf];
      
    if (options & NX_BACKWARDS_SEARCH) {
        fromLoc = RNGLOC(fRange) + RNGLEN(fRange) - findStrLen;
        toLoc = RNGLOC(fRange);
    } else {       
        fromLoc = RNGLOC(fRange);
        toLoc = RNGLOC(fRange) + RNGLEN(fRange) - findStrLen;
    }

    step = (fromLoc <= toLoc) ? 1 : -1;
    cnt = fromLoc;
    do {
        unsigned int chCnt;
        for (chCnt = 0; chCnt < findStrLen; chCnt++) {
	    unichar ch1 = findBuf[chCnt], ch2 = [self characterAt:chCnt + cnt];
	    if (caseInsensitive) {
		if (ch1 >= 'a' && ch1 <= 'z') ch1 += 'A' - 'a';
		if (ch2 >= 'a' && ch2 <= 'z') ch2 += 'A' - 'a';
	    }
            if (ch1 != ch2) {
                break;
            }
        }
        if (chCnt == findStrLen) {
            found = done = YES;
	    RNGLOC(range) = cnt;
	    RNGLEN(range) = findStrLen;
        } else if (cnt == toLoc) {
	    done = YES;
	} else {
            cnt += step;
        }
    } while (!done);

    if (findBuf != tmpBuf) free(findBuf);

    return range;
}

- (unsigned)findCharacter:(unichar)ch
{
    NXRange range = {0, [self length]};
    return [self findCharacter:ch range:range mask:0 table:NULL];
}

- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    NXString *string = [[NXReadOnlyString alloc] initFromCharactersNoCopy:&ch length:1 freeWhenDone:NO];
    NXRange result = [self findString:string range:fRange mask:options table:NULL];
    [string free];
    return RNGLOC(result);
}

- (unsigned)findOneOf:(NXCharacterSet *)set
{
    NXRange range = {0, [self length]};
    return [self findOneOf:set range:range mask:0 table:NULL];
}

// ??? How should we deal with the CASEINSENSITIVE flag? Probably we shouldn't care...

- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)fRange mask:(unsigned int)options table:(void *)table
{
    int step;
    unsigned fromLoc, toLoc, cnt, len;	// fromLoc and toLoc are inclusive
    BOOL found = NO, done = NO;

    len = [self length];

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
        unichar ch = [self characterAt:cnt];
	if ([set characterIsMember:ch]) {
	    done = found = YES;
        } else if (cnt == toLoc) {
	    done = YES;
	} else {
            cnt += step;
        }
    } while (!done);

    return found ? cnt : NX_STRING_NOT_FOUND;
}

- (unsigned)hash
{
    unichar buffer[MAXSTRINGLENFORHASHING];
    unsigned len = MIN([self length], MAXSTRINGLENFORHASHING);
    NXRange range = {0, len};
    
    [self getCharacters:buffer range:range];
    return NXHashCharacters(buffer, [self length]);
}

- (NXString *)copySubstring:(NXRange)range
{
    return [self copySubstring:range fromZone:[self zone]];
}

- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone
{
    if (RNGLOC(range) + RNGLEN(range) > [self length]) BOUNDSERROR;
    if (RNGLOC(range) == 0 && RNGLEN(range) == [self length]) {
	return [self copyFromZone:zone];
    } else {
	id newObject = [[self class] allocFromZone:zone];
	unichar *chars = [newObject allocateCharacterBuffer:RNGLEN(range)];
	[self getCharacters:chars range:range];
	return [newObject initFromCharactersNoCopy:chars length:RNGLEN(range)];
    }
}

- immutableCopy
{
    return [self immutableCopyFromZone:[self zone]];
}

- immutableCopyFromZone:(NXZone *)zone
{
    return [self copyFromZone:zone];
}

- mutableCopy
{
    return [self mutableCopyFromZone:[self zone]];
}

- mutableCopyFromZone:(NXZone *)zone
{
    return [[NXReadWriteString allocFromZone:zone] initFromString:self];
}

- (void)writeToStream:(NXStream *)stream
{
#if CHARS_ARE_EIGHT_BIT
    [self writeCStringToStream:stream];
#else
#warning writeToStream: not implemented for Unicode
    _NXStringErrorRaise (NXStringInternalError, "writeToStream: not implemented for Unicode");
#endif
}

#define WRITEBUFFERLEN 1024

- (void)writeCStringToStream:(NXStream *)stream
{
    char buf[WRITEBUFFERLEN+1];
    NXRange range = {0, [self length]};

    while (RNGLEN(range) > 0) {
	NXRange remainingRange;
	[self getCString:buf maxLength:WRITEBUFFERLEN range:range remainingRange:&remainingRange];
	NXWrite (stream, buf, NX_LENGTH(range) - NX_LENGTH(remainingRange));
	range = remainingRange;
    }
}

- (void)printForDebugger:(NXStream *)stream
{
    unsigned int cnt, length = [self length];
    
    NXPrintf (stream, "%s, length %d: ", [[self class] name], length);
    
    for (cnt = 0; cnt < length; cnt++) {
        unichar ch = [self characterAt:cnt];
	NXPrintf (stream, (ch >= ' ' && ch < 127) ? "%c" : "<0x%x>", ch);
    }
    NXFlush (stream);
}

#define TOOLONGLIMIT 200
#define EACHSECTION 80

- (void)_print
{
    unsigned int cnt = 0, length = [self length], breakAt = (length > TOOLONGLIMIT) ? EACHSECTION : UINT_MAX;
    
    fprintf (stderr, "%s, length %d: ", [[self class] name], length);
    
    while (cnt < length) {
        unichar ch = [self characterAt:cnt];
	fprintf (stderr, (ch >= ' ' && ch < 127) ? "%c" : "<0x%x>", ch);
	if (++cnt == breakAt) {
	    cnt = length - EACHSECTION;
	    fprintf (stderr, "...");
	}
    }
    fprintf (stderr, "\n");
}


#ifndef DONT_USE_OLD_NXSTRING_NAMES

/* Compatibility stuff. These are 2.x/3.0 methods which should be preserved for 3.x but removed in 4.0.
*/

- initFromStream:(NXStream *)stream uptoLength:(unsigned)length orUntilOneOf:(NXCharacterSet *)set
{
    return [self initFromStream:stream untilOneOf:set maxLength:length];
}

- initFromByteStream:(NXStream *)stream uptoLength:(unsigned)length orUntilOneOf:(NXCharacterSet *)set
{
    return [self initFromCStringStream:stream untilOneOf:set maxLength:length];
}

- (void)getCString:(char *)buffer range:(NXStringRange)range
{
    [self getCString:buffer maxLength:NX_MAX_STRING_LENGTH range:range remainingRange:NULL];
}

- (void)getCString:(char *)buffer length:(unsigned)bytes
{
    [self getCString:buffer maxLength:bytes];
}

- (void)getCString:(char *)buffer length:(unsigned)bytes range:(NXStringRange)range
{
    [self getCString:buffer maxLength:bytes range:range remainingRange:NULL];
}

- (NXComparisionResult)compare:(NXString *)string mask:(unsigned int)options
{
    return [self compare:string mask:options table:NULL];
}

- (NXComparisionResult)compare:(NXString *)string mask:(unsigned int)options usingTable:(void *)table
{
    return [self compare:string mask:options table:table];    
}

- (NXStringRange)find:(NXString *)string
{
    return [self findString:string];
}

- (NXStringRange)find:(NXString *)string range:(NXStringRange)range
{
    return [self findString:string range:range mask:0 table:NULL];
}

- (NXStringRange)find:(NXString *)string mask:(unsigned int)options
{
    return [self findString:string range:(NXRange){0, [self length]} mask:options table:NULL];
} 

- (NXStringRange)find:(NXString *)string range:(NXStringRange)range mask:(unsigned int)options usingTable:(void *)table
{
    return [self findString:string range:range mask:options table:table];
}

- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXStringRange)range
{
    return [self findOneOf:set range:range mask:0 table:NULL];
}

- (unsigned)findOneOf:(NXCharacterSet *)set mask:(unsigned int)options
{
    return [self findOneOf:set range:(NXRange){0, [self length]} mask:options table:NULL];
}

- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXStringRange)range mask:(unsigned int)options usingTable:(void *)table
{
    return [self findOneOf:set range:range mask:options table:table];
}

- (unsigned)findCharacter:(unichar)ch range:(NXStringRange)range
{
    return [self findCharacter:ch range:range mask:0 table:NULL];
}

- (unsigned)findCharacter:(unichar)ch mask:(unsigned int)options
{
    return [self findCharacter:ch range:(NXRange){0, [self length]} mask:options table:NULL];
}

- (unsigned)findCharacter:(unichar)ch range:(NXStringRange)range mask:(unsigned int)options usingTable:(void *)table
{
    return [self findCharacter:ch range:range mask:options table:table];
}

- (unichar *)createCharacterBuffer:(unsigned)nChars
{
    return [self allocateCharacterBuffer:nChars];
}

- (void)writeBytesToStream:(NXStream *)stream
{
    return [self writeCStringToStream:stream];
}

#endif DONT_USE_OLD_NXSTRING_NAMES

@end
#endif /* KERNEL */