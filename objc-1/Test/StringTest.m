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
   StringTest.m by Ali Ozer
   Run with -v for verbose mode (where all results, successful or not, are shown).
*/

#ifdef TESTING
#define NXString NxString
#define NXMutableString NxMutableString
#define NXReadWriteString NxReadWriteString
#define NXReadOnlyString NxReadOnlyString
#define NXReadOnlySubstring NxReadOnlySubstring
#define NXSimpleReadOnlyString NxSimpleReadOnlyString
#define NXUniquedString NxUniquedString
#define NXGapString NxGapString
#define NXBigString NxBigString
#endif

#import "NXString.h"

#define TESTTESTEDSTUFF
// #define TESTGAPSTRING
// #define TESTBIGSTRING

#ifdef TESTGAPSTRING
#import "NXGapString.h"
#endif
#ifdef TESTBIGSTRING
#import "NXBigString.h"
#endif

#define TESTCONSTANTSTRING

#import <stdio.h>
#import <streams/streams.h>
#import <appkit/nextstd.h>
#import <libc.h>

#define CHARALLOC(zone, var, num)		var = ((num) ?  NXZoneMalloc((zone), sizeof(unichar) * (num)) : NULL)

static int numErrors;
static BOOL verbose;

@interface NXString (DebugStuff)

- (void)verifyInt:(int)value :(int)desired;
- (void)verifyCStrings:(const char *)value :(const char *)desired;
- (void)verifyChars:(const char *)theRealThing;
- (void)verifyId:theRealThing;
- (void)verifyString:theRealThing;

@end

@implementation NXString (DebugStuff)

static Err()
{
    // Place to break on...
    numErrors++;
}

- (void)verifyChars:(const char *)theRealThing
{
    unichar buffer[1000];
    BOOL equal;

    [self getCharacters:buffer];

    if (equal = ([self length] == strlen(theRealThing))) {
	int cnt, length = [self length];
	for (cnt = 0; cnt < length; cnt++) {
	    if (buffer[cnt] != (unichar)theRealThing[cnt]) {
		equal = NO;
		break;
	    }
	}
    }

    if (verbose || !equal) {
	fprintf (stderr, "%s:", equal ? "OK" : "*** NOT OK");
	fprintf (stderr, " %p ", self);
	[self printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	fprintf (stderr, "\n");
    
	if (!equal) {
	    fprintf (stderr, "*** Should be: %s\n", theRealThing);
	    Err();
	}
    }
}

- (void)verifyInt:(int)value :(int)desired
{
    BOOL equal = (value == desired);

    if (verbose || !equal) {
	fprintf (stderr, "%s:", equal ? "OK" : "*** NOT OK");
	fprintf (stderr, " %p ", self);
	[self printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	fprintf (stderr, "\n");
    
	if (!equal) {
	    fprintf (stderr, "*** %d should be: %d\n", value, desired);
	    Err();
	}
    }
}

- (void)verifyCStrings:(const char *)value :(const char *)desired
{
    BOOL equal = !strcmp(value, desired);

    if (verbose || !equal) {
	fprintf (stderr, "%s:", equal ? "OK" : "*** NOT OK");
	fprintf (stderr, " %p ", self);
	[self printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	fprintf (stderr, "\n");
    
	if (!equal) {
	    fprintf (stderr, "*** %s should be: %s\n", value, desired);
	    Err();
	}
    }
}

- (void)verifyId:theRealThing
{
    BOOL equal = (theRealThing == self);

    if (verbose || !equal) {
	fprintf (stderr, "%s:", equal ? "OK" : "*** NOT OK");
	fprintf (stderr, " %p ", self);
	[self printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	fprintf (stderr, "\n");
    
	if (!equal) {
	    fprintf (stderr, "*** Should be: %p\n", theRealThing);
	    Err();
	}
    }
}

- (void)verifyString:theRealThing
{
    BOOL equal = ([self isEqual:theRealThing]);

    if (verbose || !equal) {
	fprintf (stderr, "%s:", equal ? "OK" : "*** NOT OK");
	fprintf (stderr, " %p ", self);
	[self printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	fprintf (stderr, "\n");
    
	if (!equal) {
	    fprintf (stderr, "*** Should be: ");
	    [theRealThing printForDebugger: NXOpenFile (fileno (stderr), NX_WRITEONLY)];
	    fprintf (stderr, "\n");
	    Err();
	}
    }
}

@end

#define TITLE(title) 		\
    printf ("%s%s %s%s\n",	\
	verbose ? "\n" : "",	\
	[testClass name],	\
	(title),		\
	verbose ? ":" : "...")

#define TITLE1(title)		\
    printf ("%s%s%s\n",		\
	verbose ? "\n" : "",	\
	(title),		\
	verbose ? ":" : "...")

/* This function returns a string containing 200 occurences of "ab..yz"
*/
static NXString *hugeString (id testClass)
{
#define HUGESTRINGLEN 5200
    static char buf[HUGESTRINGLEN] = {'\0'};
    if (buf[0] == '\0') {
	int cnt;
	for (cnt = 0; cnt < HUGESTRINGLEN; cnt++) buf[cnt] = ('a' + (cnt % 26));
    }
    return [[testClass alloc] initFromCString:buf];           
}

void testBasic /* create/read/copy/free */ (id testClass)
{
    id str1, str2, str3;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    unichar chars2[] = {'F', 'G', 'H', 'I', 'J'};
    unichar chars3[] = {'K', 'L', 'M', 'N', 'O'};
    unichar *charPtr;
    NXRange range = {1, 2};
    NXRange tmpRange = {0, 0};

    TITLE ("basic create/read/copy/free test");
    
    str1 = [[testClass alloc] initFromCharacters:chars1 length:5];
    str2 = [[testClass alloc] initFromString:str1];
    str3 = [[testClass alloc] initFromString:str2 range:range];
    [str3 verifyChars:"BC"];   
    [str3 free];
    str3 = [str2 copy];
    [str3 verifyChars:"ABCDE"];   
    [str2 free];
    [str3 verifyChars:"ABCDE"];
    [str3 free];

    str3 = [[testClass alloc] initFromCharacters:chars2 length:5];
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(range));
    [str3 getCharacters:charPtr range:range];
    str2 = [[testClass alloc] initFromCharacters:charPtr length:NX_LENGTH(range)];
    [str3 verifyChars:"FGHIJ"];
    [str2 verifyChars:"GH"];
    [str2 free];
    NX_FREE (charPtr);

    NX_LOCATION(tmpRange) = 1;
    NX_LENGTH(tmpRange) = [str3 length] - 1;
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(tmpRange));
    [str3 getCharacters:charPtr range:tmpRange];
    str2 = [[testClass alloc] initFromCharacters:charPtr length:NX_LENGTH(tmpRange)];
    [str2 verifyChars:"GHIJ"];
    [str2 free];
    [str3 free];
    [str1 free]; 
    NX_FREE (charPtr);

    str3 = [[testClass alloc] initFromCharacters:chars3 length:5];
    str2 = [str3 copy];
    NX_LOCATION(tmpRange) = 0;
    NX_LENGTH(tmpRange) = [str2 length];
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(tmpRange));
    [str2 getCharacters:charPtr range:tmpRange];
    str1 = [[testClass alloc] initFromCharactersNoCopy:charPtr length:NX_LENGTH(tmpRange)];
    [str3 free];
    [str2 verifyChars:"KLMNO"];
    [str2 free];
    [str1 verifyChars:"KLMNO"];
    [str1 free];

    str1 = [[testClass alloc] init];
    str2 = [[testClass alloc] init];
    [str1 free];
    str1 = [str2 copy];
    str3 = [[testClass alloc] initFromCharacters:NULL length:0];
    [str2 free];
    [str3 verifyChars:""];
    [str3 free];
    [str1 verifyChars:""];
    [str1 free];

}    

void testCopy /* copy */ (id testClass)
{
    id str1, str2, str3, str4;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    NXRange range = {1,3};

    TITLE ("mutable/immutable copy test");
    
    str1 = [[testClass alloc] initFromCharacters:chars1 length:5];
    str2 = [str1 mutableCopy];
    [str2 appendString:str1];
    [str2 verifyChars:"ABCDEABCDE"];
    str3 = [str1 copySubstring:range];
    str4 = [str2 copySubstring:range];
    [str4 appendString:str3];
    [str4 verifyChars:"BCDBCD"];
    [str1 free];
    [str2 free];
    str1 = [str3 mutableCopy];
    [str1 insertString:str4 at:1];
    [str1 verifyChars:"BBCDBCDCD"];
    [str3 verifyChars:"BCD"];
    [str4 verifyChars:"BCDBCD"];
    [str3 free];
    [str4 free];
    [str1 verifyChars:"BBCDBCDCD"];
    [str1 free];

}

void testRefCountedAndUniqued ()
{
    id str1, str2, str3, str4;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    NXRange range = {1,3};
    typedef struct {@defs(NXReadOnlyString);} ROString;
    
    TITLE1 ("NXReadOnlyString, NXReadWriteString, NXUniquedString tests");
    
    str1 = [[NXReadOnlyString alloc] initFromCharacters:chars1 length:5];
    str2 = [str1 mutableCopy];
    str3 = [str2 immutableCopy];
    [str1 verifyInt:(int)str3 :(int)str1];
    [str2 free];
    str2 = [[NXReadWriteString alloc] initFromCharacters:chars1 length:3];
    [str1 verifyInt:(int)str3 :(int)str1];
    [str1 free];
    [str3 verifyChars:"ABCDE"];
    [str2 appendString:str3];
    [str3 free];
    [str2 verifyChars:"ABCABCDE"];
    
    str1 = [[NXReadOnlyString alloc] initFromCharactersNoCopy:chars1 length:5 freeWhenDone:NO];
    str2 = [str1 mutableCopy];
    str3 = [str2 immutableCopy];
    [str2 free];
    str2 = [str3 copySubstring:range];
    [str1 verifyInt:(int)(((ROString *)str1)->characters)+1 :(int)(((ROString *)str2)->characters)];

    [str1 free];
    [str3 verifyChars:"ABCDE"];
    [str3 free];
    str3 = [str2 mutableCopy];
    [str3 insertString:str2 at:1];
    [str3 verifyChars:"BBCDCD"];
    [str3 deleteCharactersInRange:range];
    [str3 verifyChars:"BCD"];
    [str2 verifyChars:"BCD"];
       
    str1 = [NXUniquedString newFromString:str3];
    str4 = [NXUniquedString newFromString:str2];
    [str1 verifyInt:(int)str4 :(int)str1];
    [str1 verifyInt:(int)[NXUniquedString newFromString:[str4 mutableCopy]] :(int)str1];
      
}


void testEdit /* edit */ (id testClass)
{
    id str1, str2, str3;
    typedef struct _NXReadWriteStringKludge {
	@defs(NXReadWriteString);
    } NXReadWriteStringKludge;
    typedef struct _NXReadOnlyStringKludge {
	@defs(NXReadOnlyString);
    } NXReadOnlyStringKludge;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    unichar chars2[] = {'F', 'G', 'H', 'I', 'J'};
    unichar chars3[] = {'K', 'L', 'M', 'N', 'O'};
    int addr = 0;
    NXRange range = {1, 2};

    TITLE ("appendString/delete/insertString test");
    
    str1 = [[testClass alloc] initFromCharacters:chars1 length:5];
    str2 = [[testClass alloc] initFromString:str1 range:range];
    [str1 appendString:str2];
    [str1 verifyChars:"ABCDEBC"];
    [str2 verifyChars:"BC"];   
    [str1 insertString:str2 at:0];
    [str1 insertString:str2 at:3];
    [str1 deleteCharactersInRange:range];
    [str1 verifyChars:"BBCBCDEBC"];
    [str1 free];

    str1 = [[NXReadOnlyString alloc] initFromCharacters:chars3 length:5];
    [str2 appendString:str1];
    [str1 free];
    [str2 verifyChars:"BCKLMNO"];
    if (testClass == [NXReadWriteString class]) {
	addr = (int)((NXReadOnlyStringKludge *)(((NXReadWriteStringKludge *)str2)->actualString))->characters;
    }
    str1 = [str2 copy];
    str3 = [str1 copy];
    [str2 free];
    [str1 free];
    str2 = [str3 copy];
    [str3 verifyChars:"BCKLMNO"];
    [str2 verifyChars:"BCKLMNO"];
    if (testClass == [NXReadWriteString class]) {
	[str2 verifyInt:(int)((NXReadOnlyStringKludge *)(((NXReadWriteStringKludge *)str2)->actualString))->characters :addr];
    }
    str1 = [[NXReadOnlyString alloc] initFromCharacters:chars2 length:1];
    [str2 insertString:str1 at:2];
    [str1 free];
    [str3 verifyChars:"BCKLMNO"];
    [str2 verifyChars:"BCFKLMNO"];
    [str2 free];
    [str3 free];
    
    str1 = [[testClass alloc] init];
    str2 = [[testClass alloc] init];
    [str1 appendString:@"Foo Bar"];
    [str1 verifyChars:"Foo Bar"];
    [str2 verifyChars:""];
    [str1 free];
    str1 = [[testClass alloc] init];
    [str2 verifyChars:""];
    [str1 appendString:str2];
    [str2 free];
    [str1 verifyChars:""];    
    
}

void testLargeEdit /* large edit */ (id testClass)
{
    int cnt;
    id str1, str2, str3;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    NXRange range = {999, 25};
    NXRange findRange;

    TITLE ("large appendString/delete/insertString test");
    
    str1 = [[testClass alloc] initFromCharacters:chars1 length:5];
    str2 = [str1 copy];
    for (cnt = 1; cnt < 64; cnt++) {
	[str1 appendString:str2];
    } // Resulting length is 320
    [str2 free];
    str2 = [str1 copy];
    [str1 appendString:str2]; // 640
    [str1 appendString:str2]; // 960
    [str1 appendString:str2]; // 1280
    [str2 free];
    [str1 verifyInt:[str1 length] :1280];

    str2 = [[testClass alloc] initFromString:str1 range:range];
    [str2 verifyChars:"EABCDEABCDEABCDEABCDEABCD"];
    [str2 insertString:str1 at:2];
    [str2 insertString:str1 at:[str2 length]];
    [str1 free];
    str3 = [str2 copySubstring:(NXRange){0, 10} fromZone:[str2 zone]];
    [str3 verifyChars:"EAABCDEABC"];
    str1 = [str2 copy];
    [str2 replaceCharactersInRange:range withString:str1];
    [str2 appendString:str3];
    [str2 appendString:@"Howdy"];
    [str3 free];
    [str2 verifyInt:[str2 length] :5160];
    findRange = [str2 findString:@"EAABCDEABCH"];
    [str2 verifyInt:NX_LOCATION(findRange) :5145];
    [str1 free];
    NX_LOCATION(range) = 2;
    NX_LENGTH(range) = [str2 length] - 4;
    [str2 deleteCharactersInRange:range];
    [str2 verifyChars:"EAdy"];
    [str2 free];
}

void testCompare /* compare */ (id testClass)
{
    id str1, str2, str3, str4, str5;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E'};
    unichar chars2[] = {'F', 'G', 'h', 'f', 'g'};
    unichar chars3[] = {'F', 'g', 'H', 'f', 'G'};

    TITLE ("compare test");

    str1 = [[testClass alloc] initFromCharacters:chars1 length:5];    
    str2 = [[testClass alloc] initFromCharacters:chars1 length:4];    
    str3 = [[testClass alloc] initFromCharacters:chars2 length:4];
    [str1 verifyInt:[str1 compare:str1] :0];
    [str1 verifyInt:[str1 compare:str2] :1];
    [str1 verifyInt:[str1 compare:str3] :-1];
    [str2 verifyInt:[str2 compare:str1] :-1];
    [str2 verifyInt:[str2 compare:str3] :-1];
    [str3 verifyInt:[str3 compare:str1] :1];
    [str3 verifyInt:[str3 compare:str2] :1];
    str4 = [[NXReadWriteString alloc] initFromString:str1];
    str5 = [str4 copy];
    [str1 verifyInt:[str1 compare:str4] :0];
    [str5 verifyInt:[str5 compare:str1] :0];
    [str4 appendString:str4];
    [str1 verifyInt:[str1 compare:str4] :-1];
    [str5 verifyInt:[str5 compare:str1] :0];
    [str4 free];
    [str5 free];
    [str1 free];
    [str2 free];
    str4 = [[testClass alloc] initFromCharacters:chars3 length:4];
    [str4 verifyInt:[str4 compare:str3] :1];
    [str3 verifyInt:[str3 compare:str4] :-1];
    [str4 verifyInt:[str4 compare:str3 mask:NX_CASE_INSENSITIVE table:NULL] :0];
    [str3 verifyInt:[str3 compare:str4 mask:NX_CASE_INSENSITIVE table:NULL] :0];
    str2 = [[NXReadOnlyString alloc] initFromString:str3];
    [str4 verifyInt:[str4 compare:str2] :1];
    [str2 verifyInt:[str2 compare:str4] :-1];
    [str2 free];
    [str3 free];
    [str4 free];
}

#define wholeRange(s) ((NXRange){0, [(s) length]})

void testFind /* find */ (id testClass)
{
    id str1, str2, str3, str4;
    unichar chars1[] = {'A', 'B', 'C', 'D', 'E', 'F', 'g', 'h', 'a', 'b', 'B', 'C'};
    unichar chars2[] = {'F', 'G', 'h', 'f', 'g'};
    unichar chars3[] = {'F', 'g', 'H', 'f', 'G'};
    NXRange findRange;

    TITLE ("find test");

    str1 = [[testClass alloc] initFromCharacters:chars1 length:12];   // ABCDEFghabBC
    str2 = [[testClass alloc] initFromCharacters:chars1 length:2];    // AB
    str3 = [[testClass alloc] initFromCharacters:chars2 length:2];    // FG
    str4 = [[testClass alloc] initFromCharacters:chars3 length:2];    // Fg

    findRange = [str1 findString:str2];
    [str1 verifyInt:NX_LOCATION(findRange) :0];
    [str1 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str1 findString:str3];
    [str1 verifyInt:NX_LOCATION(findRange) :NX_STRING_NOT_FOUND];
    [str1 verifyInt:NX_LENGTH(findRange) :0];
    findRange = [str1 findString:str4];
    [str1 verifyInt:NX_LOCATION(findRange) :5];
    [str1 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str1 findString:str3 range:wholeRange(str1) mask:NX_CASE_INSENSITIVE table:NULL];
    [str1 verifyInt:NX_LOCATION(findRange) :5];
    [str1 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str1 findString:str2 range:wholeRange(str1) mask:NX_CASE_INSENSITIVE|NX_BACKWARDS_SEARCH table:NULL];
    [str1 verifyInt:NX_LOCATION(findRange) :8];
    [str1 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str1 findString:str2 range:wholeRange(str1) mask:NX_BACKWARDS_SEARCH table:NULL];
    [str1 verifyInt:NX_LOCATION(findRange) :0];
    [str1 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str3 findString:str4 range:wholeRange(str3) mask:NX_BACKWARDS_SEARCH table:NULL];
    [str3 verifyInt:NX_LOCATION(findRange) :NX_STRING_NOT_FOUND];
    [str3 verifyInt:NX_LENGTH(findRange) :0];
    findRange = [str3 findString:str4 range:wholeRange(str3) mask:NX_CASE_INSENSITIVE|NX_BACKWARDS_SEARCH table:NULL];
    [str3 verifyInt:NX_LOCATION(findRange) :0];
    [str3 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str3 findString:str4 range:wholeRange(str3) mask:NX_CASE_INSENSITIVE table:NULL];
    [str3 verifyInt:NX_LOCATION(findRange) :0];
    [str3 verifyInt:NX_LENGTH(findRange) :2];
    findRange = [str2 findString:str1];
    [str2 verifyInt:NX_LOCATION(findRange) :NX_STRING_NOT_FOUND];
    [str2 verifyInt:NX_LENGTH(findRange) :0];
    
    NX_LOCATION(findRange) = 1;
    NX_LENGTH(findRange) = [str1 length] - 1;
    findRange = [str1 findString:str2 range:findRange mask:NX_CASE_INSENSITIVE table:NULL];
    [str3 verifyInt:NX_LOCATION(findRange) :8];
    [str3 verifyInt:NX_LENGTH(findRange) :2];
    
}

void testStream (id testClass)
{
    const char *bytes = "Hello There. Hi There.";
    unichar chars1[] = {'A', 'B', 'C', 0, 1, 2, '0', '1', '2'};
    NXStream *stream;
    id str1, str2, str3, str4;
    NXCharacterSet *numberSet;

    TITLE ("stream test");

    numberSet = [[NXCharacterSet alloc] init];
    [numberSet addRange:'0' :'9'];

    stream = NXOpenMemory(bytes, strlen(bytes), NX_READONLY);
    str1 = [[testClass alloc] initFromCStringStream:stream untilOneOf:nil maxLength:1];
    str2 = [[testClass alloc] initFromCStringStream:stream untilOneOf:nil maxLength:17];
    str3 = [[testClass alloc] initFromCStringStream:stream untilOneOf:nil maxLength:120];
    
    [str1 verifyChars:"H"];
    [str2 verifyChars:"ello There. Hi Th"];
    [str3 verifyChars:"ere."];
    [str1 free];
    [str3 free];

    NXClose (stream);

    stream = NXOpenMemory(NULL, 0, NX_READWRITE);
    [str2 writeCStringToStream:stream];
    str1 = [[testClass alloc] initFromCharacters:chars1 length:9];
    [str1 writeCStringToStream:stream];

    NXSeek(stream, 1, NX_FROMSTART);
    str3 = [[testClass alloc] initFromCStringStream:stream untilOneOf:nil maxLength:NX_MAX_STRING_LENGTH];

    str4 = [[NXReadWriteString alloc] init];
    [str4 append:str2];
    [str4 deleteCharactersInRange:(NXRange){0,1}];	// Because we seeked to 1 above...
    [str4 append:str1];
    
    [str3 verifyString:str4];
    [str3 verifyInt:[str3 length] :[str2 length] - 1 + 9];

    [str3 free];

    NXSeek(stream, 1, NX_FROMSTART);
    str3 = [[testClass alloc] initFromCStringStream:stream untilOneOf:numberSet maxLength:NX_MAX_STRING_LENGTH];

    [str4 deleteCharactersInRange:(NXRange){[str4 length]-3, 3}];
    [str3 verifyString:str4];
    [str3 verifyInt:[str3 length] :[str2 length] - 1 + 6];
    [str3 free];

    NXSeek(stream, 1, NX_FROMSTART);
    str3 = [[testClass alloc] initFromCStringStream:stream untilOneOf:numberSet maxLength:5];

    [str4 deleteCharactersInRange:(NXRange){5, [str4 length] - 5}];
    [str3 verifyString:str4];
    [str3 verifyInt:[str3 length] :5];
    [str3 free];

    [str1 free];
    [str2 free];
    [str4 free];

    NXCloseMemory(stream, NX_FREEBUFFER);

    str1 = hugeString(testClass);

    stream = NXOpenMemory(NULL, 0, NX_READWRITE);
    [str1 writeCStringToStream:stream];
    NXSeek(stream, 0, NX_FROMSTART);
    str2 = [[testClass alloc] initFromCStringStream:stream untilOneOf:nil maxLength:NX_MAX_STRING_LENGTH];
    [str1 verifyString:str2];
    [str2 free];
    NXCloseMemory(stream, NX_FREEBUFFER);
    
    stream = NXOpenMemory(NULL, 0, NX_READWRITE);
    [str1 writeToStream:stream];
    NXSeek(stream, 0, NX_FROMSTART);
    str2 = [[testClass alloc] initFromStream:stream untilOneOf:nil maxLength:NX_MAX_STRING_LENGTH];
    [str1 verifyString:str2];
    [str2 free];
    NXCloseMemory(stream, NX_FREEBUFFER);
    
    [str1 free];
}

void testHash (id testClass)
{
    id str1;
    unichar chars1[] = {'F', 'o', 'o'};
    
    TITLE ("hash test");

    str1 = [[testClass alloc] initFromCharacters:chars1 length:3];
    [str1 verifyInt:[str1 hash] :32095];
    [str1 verifyInt:[str1 hash] :[@"Foo" hash]];
    [str1 free];
    
    str1 = [[testClass alloc] initFromString:@"Hello World"];
    [str1 verifyInt:[str1 hash] :850486630];
    [str1 free]; 
}

void testFormat (id testClass)
{
    id str1, str2, str3;
    int cnt;
    unichar chars1[] = {'F', 'o', 'o'};
    
    TITLE ("formatted create test");

    str1 = [[NXReadOnlyString alloc] initFromCString:"===%2.1f %d %s %@==="];
    str2 = [[NXReadOnlyString alloc] initFromCharacters:chars1 length:3];
    str3 = [[testClass alloc] initFromFormat:str1, 3.14, -42, "Hello World", str2];
    
    [str3 verifyChars:"===3.1 -42 Hello World Foo==="];

    [str1 free];
    [str2 free];
    [str3 free];

    str1 = [[NXReadOnlyString alloc] initFromCString:"Hello"];
    str3 = [[NXReadWriteString alloc] initFromString:str1];
    for (cnt = 0; cnt < 12; cnt++) {
	str2 = [[testClass alloc] initFromFormat:@"%@*%@", str1, str1];
	[str3 appendString:@"*"];	// Test string; we assume append works.
	[str3 appendString:str1];
	[str2 verifyString:str3];
	[str1 free];
	str1 = str2;
    }
    [str3 free];
    str3 = [[NXReadOnlyString alloc] initFromString:str1 range:(NXRange){[str1 length] - 6, 6}]; 
    [str3 verifyChars:"*Hello"];
    [str1 free];
    [str3 free];
}

void testCStrings (id testClass)
{
    id str1, str2;
    char *cString;
    NXAtom uString1, uString2;
#define CSTRBUFLEN 10
#define CSTRBIGBUFLEN 1000
    char cStringBuffer[CSTRBIGBUFLEN+1];
    int cnt;
    NXRange range;

    TITLE ("CString test");
    
    str1 = [[NXReadOnlyString alloc] initFromCString:"Hello World"];
    str2 = [str1 mutableCopy];
    cString = [str1 cStringCopy];
    [str1 verifyChars:cString];
    free (cString);
    uString1 = [str1 uniqueCStringCopy];
    [str1 verifyChars:uString1];
    uString2 = [str1 uniqueCStringCopy];
    [str1 verifyInt:(int)uString1 :(int)uString2];
    uString2 = [str2 uniqueCStringCopy];
    [str1 verifyInt:(int)uString1 :(int)uString2];
    [str2 appendString:@""];
    uString2 = [str2 uniqueCStringCopy];
    [str1 verifyInt:(int)uString1 :(int)uString2];
    [str2 appendString:@" "];
    uString1 = [str2 uniqueCStringCopy];
    uString2 = [@"Hello World " uniqueCStringCopy];
    [str2 verifyInt:(int)uString1 :(int)uString2];
    [str1 free];
    [str2 free];

    str1 = [[testClass alloc] initFromCString:"Bonjour Everyone"];
    [str1 getCString:cStringBuffer maxLength:3];
    [str1 verifyCStrings:cStringBuffer :"Bon"];
    [str1 getCString:cStringBuffer maxLength:0];
    [str1 verifyCStrings:cStringBuffer :""];
    NX_LOCATION(range) = 8; NX_LENGTH(range) = 5;
    [str1 getCString:cStringBuffer maxLength:NX_MAX_STRING_LENGTH range:range remainingRange:NULL];
    [str1 verifyCStrings:cStringBuffer :"Every"];
    [str1 getCString:cStringBuffer maxLength:2 range:range remainingRange:NULL];
    [str1 verifyCStrings:cStringBuffer :"Ev"];
    [str1 getCString:cStringBuffer maxLength:CSTRBUFLEN range:range remainingRange:NULL];
    [str1 verifyCStrings:cStringBuffer :"Every"];
    [str1 getCString:cStringBuffer maxLength:CSTRBUFLEN];
    [str1 verifyCStrings:cStringBuffer :"Bonjour Ev"];
    NX_LOCATION(range) = 0;
    NX_LENGTH(range) = 11;
    [str1 getCString:cStringBuffer maxLength:0 range:range remainingRange:&range];
    [str1 verifyCStrings:cStringBuffer :""];
    [str1 verifyInt:NX_LOCATION(range) :0];
    [str1 verifyInt:NX_LENGTH(range) :11];
    [str1 getCString:cStringBuffer maxLength:5 range:range remainingRange:&range];
    [str1 verifyCStrings:cStringBuffer :"Bonjo"];
    [str1 verifyInt:NX_LOCATION(range) :5];
    [str1 verifyInt:NX_LENGTH(range) :6];
    [str1 getCString:cStringBuffer maxLength:10 range:range remainingRange:&range];
    [str1 verifyCStrings:cStringBuffer :"ur Eve"];
    [str1 verifyInt:NX_LOCATION(range) :11];
    [str1 verifyInt:NX_LENGTH(range) :0];
    [str1 free];

    str1 = hugeString(testClass);
    [str1 getCString:cStringBuffer maxLength:CSTRBIGBUFLEN range:(NXRange){0,[str1 length]} remainingRange:&range];
    cnt = 26 * ((int)(CSTRBIGBUFLEN / 26) - 1);
    cStringBuffer[cnt + 26] = '\0';
    [str1 verifyCStrings:&cStringBuffer[cnt] :"abcdefghijklmnopqrstuvwxyz"];
    [str1 verifyInt:NX_LOCATION(range) :CSTRBIGBUFLEN];
    [str1 verifyInt:NX_LENGTH(range) :[str1 length] - CSTRBIGBUFLEN];
    [str1 free];        
}

void testFindAndSets (id testClass)
{
    id str1;
    unichar chars1[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '1', 'h', '3'};
    unichar chars2[] = {' ', '\t'};
    NXCharacterSet *numberSet, *alphaSet, *spaceSet, *notAlphaSet, *numAndAlphaSet;
    
    TITLE ("find and NXCharacterSet test");

    numberSet = [[NXCharacterSet alloc] init];
    [numberSet addRange:'0' :'9'];

    alphaSet = [[NXCharacterSet alloc] init];
    [alphaSet addRange:'a' :'z'];
    [alphaSet addRange:'A' :'Z'];

    notAlphaSet = [[NXCharacterSet alloc] init];
    [notAlphaSet addRange:'a' :'z'];
    [notAlphaSet addRange:'A' :'Z'];
    [notAlphaSet invert];

    spaceSet = [[NXCharacterSet alloc] init];
    [spaceSet addCharacters:chars2 length:2];
    [spaceSet addCharacters:chars1 length:14];
    [spaceSet removeCharacters:chars1 length:14];
    [spaceSet addCharacters:chars2 length:2];

    numAndAlphaSet = [[NXCharacterSet alloc] init];
    [numAndAlphaSet unionWith:alphaSet];
    [numAndAlphaSet unionWith:numberSet];

    str1 = [[testClass alloc] initFromCharacters:chars1 length:14];
    [str1 verifyInt:[str1 findCharacter:'l'] :2];
    [str1 verifyInt:[str1 findCharacter:'l' range:wholeRange(str1) mask:NX_BACKWARDS_SEARCH table:NULL] :9];
    [str1 verifyInt:[str1 findCharacter:'h' range:wholeRange(str1) mask:NX_CASE_INSENSITIVE table:NULL] :0];
    [str1 verifyInt:[str1 findCharacter:'h' range:wholeRange(str1) mask:NX_BACKWARDS_SEARCH table:NULL] :12];

    [str1 verifyInt:[str1 findOneOf:alphaSet] :0];
    [str1 verifyInt:[str1 findOneOf:notAlphaSet] :5];
    [str1 verifyInt:[str1 findOneOf:spaceSet] :5];
    [str1 verifyInt:[str1 findOneOf:numberSet] :11];
    [str1 verifyInt:[str1 findOneOf:numberSet range:wholeRange(str1) mask:NX_BACKWARDS_SEARCH table:NULL] :13];
}

#ifdef TESTCONSTANTSTRING
void testConstant /* basic constant string functionality */ ()
{
    id str1, str2, str3;
    unichar *charPtr;
    NXRange range = {1, 2};
    NXRange tmpRange = {0, 0};

    TITLE1 ("NXConstantString test");
    
    str1 = [[NXReadOnlyString alloc] initFromString:@"ABCDE"];
    str2 = [[NXReadOnlyString alloc] initFromString:str1];
    str3 = [[NXReadOnlyString alloc] initFromString:str2 range:range];
    [str3 verifyChars:"BC"];   
    [str3 free];
    str3 = [str2 copy];
    [str3 verifyChars:"ABCDE"];   
    [str2 free];
    [str3 verifyChars:"ABCDE"];
    [str3 free];

    str3 = [[NXReadOnlyString alloc] initFromString:@"FGHIJ"];
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(range));
    [str3 getCharacters:charPtr range:range];
    str2 = [[NXReadOnlyString alloc] initFromCharacters:charPtr length:NX_LENGTH(range)];
    [str3 verifyChars:"FGHIJ"];
    [str2 verifyChars:"GH"];
    [str2 free];
    NX_FREE (charPtr);

    NX_LOCATION(tmpRange) = 1;
    NX_LENGTH(tmpRange) = [str3 length] - 1;
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(tmpRange));
    [str3 getCharacters:charPtr range:tmpRange];
    str2 = [[NXReadOnlyString alloc] initFromCharacters:charPtr length:NX_LENGTH(tmpRange)];
    [str2 verifyChars:"GHIJ"];
    [str2 free];
    [str3 free];
    [str1 free]; 
    NX_FREE (charPtr);

    str3 = [[NXReadOnlyString alloc] initFromString:@"KLM" @"NO"];
    str2 = [str3 copy];
    NX_LOCATION(tmpRange) = 0;
    NX_LENGTH(tmpRange) = [str2 length];
    CHARALLOC(NXDefaultMallocZone(), charPtr, NX_LENGTH(tmpRange));
    [str2 getCharacters:charPtr range:tmpRange];
    str1 = [[NXReadOnlyString alloc] initFromCharactersNoCopy:charPtr length:NX_LENGTH(tmpRange)];
    [str3 free];
    [str2 verifyChars:"KLMNO"];
    [str2 free];
    [str1 verifyChars:"KLMNO"];
    [str1 free];
    
    str1 = [[NXReadWriteString alloc] init];
    str2 = [str1 copy];
    [str1 appendString:@"Foo"];
    [str1 verifyInt:[str1 isEqual:@""] :NO];
    [str2 verifyInt:[str2 isEqual:@""] :YES];    
    [str1 free];
    [str2 verifyInt:[str2 isEqual:@""] :YES];    
    [str2 free];
}


#endif

void main (int argc, char **argv)
{
    numErrors = 0;
    verbose = (argc > 1) && (strcmp(argv[1], "-v") == 0);

#ifdef TESTTESTEDSTUFF
    testBasic ([NXReadOnlyString class]);
    testBasic ([NXReadWriteString class]);
    testBasic ([NXUniquedString class]);
    testBasic ([NXSimpleReadOnlyString class]);

    testCopy ([NXReadOnlyString class]);
    testCopy ([NXReadWriteString class]);
    testCopy ([NXUniquedString class]);
    testCopy ([NXSimpleReadOnlyString class]);

    testRefCountedAndUniqued ();

    testEdit ([NXReadWriteString class]);
    testLargeEdit ([NXReadWriteString class]);

    testCompare ([NXReadOnlyString class]);
    testCompare ([NXReadWriteString class]);
    testCompare ([NXUniquedString class]);
    testCompare ([NXSimpleReadOnlyString class]);

    testFind ([NXReadOnlyString class]);
    testFind ([NXReadWriteString class]);
    testFind ([NXUniquedString class]);
    testFind ([NXSimpleReadOnlyString class]);

    testStream ([NXReadOnlyString class]);
    testStream ([NXReadWriteString class]);
    testStream ([NXUniquedString class]);
    testStream ([NXSimpleReadOnlyString class]);

    testFormat ([NXReadOnlyString class]);
    testFormat ([NXReadWriteString class]);
    testFormat ([NXUniquedString class]);
    testFormat ([NXSimpleReadOnlyString class]);

    testFindAndSets ([NXReadOnlyString class]);
    testFindAndSets ([NXReadWriteString class]);
    testFindAndSets ([NXUniquedString class]);
    testFindAndSets ([NXSimpleReadOnlyString class]);

    testCStrings ([NXReadOnlyString class]);
    testCStrings ([NXReadWriteString class]);
    testCStrings ([NXUniquedString class]);
    testCStrings ([NXSimpleReadOnlyString class]);

    testHash ([NXReadOnlyString class]);
    testHash ([NXReadWriteString class]);
    testHash ([NXUniquedString class]);
    testHash ([NXSimpleReadOnlyString class]);
#endif

#ifdef TESTGAPSTRING
    testBasic ([NXGapString class]);
    testCopy ([NXGapString class]);
    testEdit ([NXGapString class]);
    testCompare ([NXGapString class]);
    testFind ([NXGapString class]);
    testLargeEdit ([NXGapString class]);
    testStream ([NXGapString class]);
    testFormat ([NXGapString class]);
    testFindAndSets ([NXGapString class]);
    testCStrings ([NXGapString class]);
    testHash ([NXGapString class]);
#endif

#ifdef TESTBIGSTRING
    testBasic ([NXBigString class]);
    testCopy ([NXBigString class]);
    testEdit ([NXBigString class]);
    testCompare ([NXBigString class]);
    testFind ([NXBigString class]);
    testLargeEdit ([NXBigString class]);
    testStream ([NXBigString class]);
    testFormat ([NXBigString class]);
    testFindAndSets ([NXBigString class]);
    testCStrings ([NXBigString class]);
    testHash ([NXBigString class]);
#endif

#ifdef TESTCONSTANTSTRING
    testConstant();
#endif

    fprintf (stderr, "\n%s%d error%s.\n", numErrors ? "*** " : "", numErrors, numErrors == 1 ? "" : "s");
}
