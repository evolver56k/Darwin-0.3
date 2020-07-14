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
	NXStringPrivate.h
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef _OBJC_NXSTRINGPRIVATE_H_
#define _OBJC_NXSTRINGPRIVATE_H_

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
#define _NXStringZone _NxStringZone
#define _NXStringErrorRaise _NxStringErrorRaise
#define NXCompareCharacters NxCompareCharacters
#define NXFindCharacters NxFindCharacters
#define NXHashCharacters NxHashCharacters
#endif

#import "NXString.h"
#import <string.h>
#import <stdlib.h>
#import <limits.h>

#define CHARS_ARE_EIGHT_BIT 1
#define SEPARATE_UNIQUED_ZONE 0

/* Separate zone for ref counted strings... */
extern NXZone *_NXStringZone (void);
#define stringZone _NXStringZone ()

/* Where we allocate very temporary char buffers. */
#define scratchZone NXDefaultMallocZone ()

#if CHARS_ARE_EIGHT_BIT
#define DATATYPEFORCHAR "c"
#define MAXCHARACTERCODE 0x0ff
#define NUMCHARACTERS 256
#else
#define DATATYPEFORCHAR "s"
#define MAXCHARACTERCODE 0x0ffff
#define NUMCHARACTERS 65536
#endif

#define NX_STRING_MAXREFS USHRT_MAX

/* Length of a temporary buffer allocated on the stack.
   Used in several routines to avoid a malloc (). */
#define MAXTMPBUFFERLEN 100

/* Number of characters copied from the strings for comparison. */
#define COMPARELENGTH 32

#define MAXSTRINGLENFORHASHING NX_HASH_STRING_LENGTH

#define NX_CHARALLOC(zone, num) \
  ((num) ? NXZoneMalloc ((zone), sizeof (unichar) * (num)) : NULL)

#define NX_CHARCOPY(from, to, len) \
  bcopy ((from), (to), (len) * sizeof (unichar))

#define NX_BYTEALLOC(zone, num) \
  ((num) ? NXZoneMalloc ((zone), sizeof (char) * (num)) : NULL)

#define RNGLEN(r) NX_LENGTH(r)
#define RNGLOC(r) NX_LOCATION(r)

#ifndef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

extern void _initStrings (void);

extern void _NXStringErrorRaise (int errorCode, const char *errorMsg);

#define BOUNDSERROR _NXStringErrorRaise (NXStringBoundsError, "Out of bounds")

#endif /* _OBJC_NXSTRINGPRIVATE_H_ */
