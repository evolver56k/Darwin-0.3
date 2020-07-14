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
/* JapaneseConverter.h
 * Copyright 1998, Apple Computer, Inc. All rights reserved.
*/

#ifndef __MACJAPANESE_H__
#define __MACJAPANESE_H__ 1

#if !defined(__CFSTRING__)
typedef char SInt8;
typedef short SInt16;
typedef long SInt32;
typedef long long SInt64;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned long UInt32;
typedef unsigned long long UInt64;
typedef unsigned char Boolean;
typedef UInt16 UniChar;

#ifndef TRUE
#define TRUE ((Boolean)1)
#define FALSE ((Boolean)0)
#endif

#ifndef NULL
#define NULL 0
#endif
#endif

/* Values for flags argument for the conversion functions below.  These can be combined, but the three NonSpacing behavior flags are exclusive.
*/
enum {
    kCFStringEncodingAllowLossyConversion = 1, // Uses fallback functions to substitutes non mappable chars
    kCFStringEncodingCheckDirection = (1 << 1), // Directionality is checked when mapping Unicode to encodings with directionality (i.e. MacArabic)
    kCFStringEncodingSubstituteCombinings = (1 << 2), // Uses fallback function to combining chars.
    kCFStringEncodingComposeCombinings = (1 << 3), // Checks mappable precomposed equivalents for decomposed sequences.  This is the default behavior.
    kCFStringEncodingIgnoreCombinings = (1 << 4), // Ignores combining chars.
    kCFStringEncodingUseCanonical = (1 << 5), // Always use canonical form
};

/* Convenience functions for converter development */
typedef struct _CFStringEncodingUnicodeTo8BitCharMap {
    UniChar _u;
    UInt8 _c;
} CFStringEncodingUnicodeTo8BitCharMap;

/* Binary searches CFStringEncodingUnicodeTo8BitCharMap */
static inline Boolean CFStringEncodingUnicodeTo8BitEncoding(const CFStringEncodingUnicodeTo8BitCharMap *theTable, UInt32 numElem, UniChar character, UInt8 *ch) {
    const CFStringEncodingUnicodeTo8BitCharMap *p, *q, *divider;

    if ((character < theTable[0]._u) || (character > theTable[numElem-1]._u)) {
        return 0;
    }
    p = theTable;
    q = p + (numElem-1);
    while (p <= q) {
        divider = p + ((q - p) >> 1);	/* divide by 2 */
        if (character < divider->_u) { q = divider - 1; }
        else if (character > divider->_u) { p = divider + 1; }
        else { *ch = divider->_c; return 1; }
    }
    return 0;
}

#define CFCharMappingDefinitionWithSize(theSize)		\
typedef struct _CFStringEncodingUnicodeTo16BitCharMapWithArray##theSize {\
    UniChar startChar;\
    UInt16 bytes[theSize];\
} CFStringEncodingUnicodeTo16BitCharMapWithArray##theSize;\
static inline UInt16 CFStringEncodingUnicodeTo16BitEncodingWithArray##theSize(const CFStringEncodingUnicodeTo16BitCharMapWithArray##theSize *theTable, UInt32 numElem, UniChar character) {\
    const CFStringEncodingUnicodeTo16BitCharMapWithArray##theSize *p, *q, *divider;\
    if ((character < theTable[0].startChar) || (character > theTable[numElem-1].startChar + theSize)) {\
        return 0;\
    }\
    p = theTable;\
    q = p + (numElem-1);\
    while (p <= q) {\
        divider = p + ((q - p) >> 1);	/* divide by 2 */\
        if (character < divider->startChar) { q = divider - 1; }\
        else if (character < (divider->startChar + theSize)) { return divider->bytes[character - divider->startChar]; }\
        else { p = divider + 1; }\
    }\
    return 0;\
}\

extern UInt32 __CFToMacJapanese(UInt32 flags, const UniChar *characters, UInt32 numChars, UInt8 *bytes, UInt32 maxByteLen, UInt32 *usedByteLen);
extern UInt32 __CFFromMacJapanese(UInt32 flags, const UInt8 *bytes, UInt32 numBytes, UniChar *characters, UInt32 maxCharLen, UInt32 *usedCharLen);

#endif __MACJAPANESE_H__
