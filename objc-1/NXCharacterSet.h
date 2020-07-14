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
/*	NXCharacterSet.h
	Copyright 1991, NeXT, Inc.
*/

#ifndef _OBJC_NXCHARACTERSET_H_
#define _OBJC_NXCHARACTERSET_H_

#import "Object.h"
#import <objc/zone.h>
#import "unichar.h"

// The NXCharacterSet class... This object stores a set of
// Unicode characters with an O(1) membership test.

@interface NXCharacterSet : Object {
    unsigned int *_bits;
    unsigned int _reserved;
}

- init;

- (BOOL)characterIsMember:(unichar)ch;

- addCharacters:(const unichar *)chars length:(unsigned)len;
- removeCharacters:(const unichar *)chars length:(unsigned)len;
- addRange:(unichar)from :(unichar)to;
- removeRange:(unichar)from :(unichar)to;

- unionWith:(const NXCharacterSet *)otherSet;
- intersectWith:(const NXCharacterSet *)otherSet;

- invert;

- copyFromZone:(NXZone *)zone;
- free;

- write:(NXTypedStream *)s;
- read:(NXTypedStream *)s;

@end

#endif /* _OBJC_NXCHARACTERSET_H_ */
