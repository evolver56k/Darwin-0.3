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
    Storage.h
    Copyright 1988, 1989 NeXT, Inc.

    DEFINED AS:	A common class
    HEADER FILES:	objc/Storage.h
*/

#ifndef _OBJC_STORAGE_H_
#define _OBJC_STORAGE_H_

#import "Object.h"
#import "typedstream.h"

@interface Storage : Object
{
@public
    void	*dataPtr;	/* Data of the Storage object */
    const char	*description;	/* Encoded data type of the stored elements */
    unsigned	numElements;	/* Number of elements actually in the array */
    unsigned	maxElements;	/* Total allocated elements */
    unsigned	elementSize;	/* Size of each element in the array */
}

/* Creating, freeing, initializing, and emptying */

- init;
- initCount:(unsigned)count elementSize:(unsigned)sizeInBytes 
	description:(const char *)descriptor;
- free; 
- empty;
- copyFromZone:(NXZone *)zone;
  
/* Manipulating the elements */

- (BOOL)isEqual: anObject;
- (const char *)description; 
- (unsigned)count; 
- (void *)elementAt:(unsigned)index; 
- replaceElementAt:(unsigned)index with:(void *)anElement;
- setNumSlots:(unsigned)numSlots; 
- setAvailableCapacity:(unsigned)numSlots;
- addElement:(void *)anElement; 
- removeLastElement; 
- insertElement:(void *)anElement at:(unsigned)index; 
- removeElementAt:(unsigned)index; 

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new; 
+ newCount:(unsigned)count elementSize:(unsigned)sizeInBytes 
	description:(const char *)descriptor; 

@end

typedef struct {
    @defs(Storage)
} NXStorageId;

#endif /* _OBJC_STORAGE_H_ */
