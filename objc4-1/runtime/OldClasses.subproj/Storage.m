/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	Storage.m
  	Copyright 1988-1996 NeXT Software, Inc.
	Responsibility: Bertrand Serlet

*/

#if !defined(KERNEL)

#if defined(WIN32)
    #include <winnt-pdo.h>
#endif

#import <stdlib.h>
#import <stdio.h>
#import <string.h>

#if defined(NeXT_PDO)
    #import <pdo.h>
#endif

#import <objc/Storage.h>

OBJC_EXPORT void _NXLogError(const char *format, ...);

#define MAX_SLOTS 0xD0000000

/* one invariant is that each block of memory allocated is filled with zeros */

#define ELMT(dataPtr, index, elementSize) \
	((char *) dataPtr + (index * elementSize))

@implementation  Storage

+ initialize
{
    [self setVersion: 1];
    return self;
}

+ new
{
    return [self newCount: 0 elementSize: sizeof(id) description: "@"];
}

- init
{
    return [self initCount: 0 elementSize: sizeof(id) description: "@"];
}

- initCount:(unsigned)count elementSize:(unsigned)sizeInBytes description:(const char *)descriptor
{
    unsigned	newSize;
    NXZone *zone = [self zone];

    if (MAX_SLOTS <= count) {
	_NXLogError("*** -[Storage initCount:elementSize:description:]: count cannot be greater than 0xD0000000\n");
	return self;
    }

    maxElements = count;
    newSize =  maxElements * sizeInBytes;
    numElements = count;
    elementSize = sizeInBytes;
    /* coercion to be fixed when Archiver goes */
    description = descriptor; 
    if (newSize) {
        dataPtr = NXZoneMalloc (zone, newSize);
	bzero(dataPtr, newSize);
    }
    
    return self;
}

+ newCount:(unsigned)count elementSize:(unsigned)sizeInBytes description:(const char *)descriptor
{
    return [[self allocFromZone:NXDefaultMallocZone()]
	initCount:count elementSize:sizeInBytes description:descriptor];
}


- free
{
    free(dataPtr);
    return[super free];
}

- empty
{
    numElements = 0;
    return self;
}

- copyFromZone:(NXZone *)zone
{
    Storage	*new = [[[self class] allocFromZone:zone]
	initCount: numElements elementSize: elementSize 
	description: description];
    bcopy (dataPtr, new->dataPtr, numElements * elementSize);
    return new;
}

- (BOOL) isEqual: anObject
{
    Storage	*other;
    if (! [anObject isKindOf: [self class]]) return NO;
    other = (Storage *) anObject;
    return (numElements == other->numElements) 
    	&& (bcmp (dataPtr, other->dataPtr, numElements * elementSize) == 0);
}

- setNumSlots:(unsigned)numSlots
{
    NXZone *zone = [self zone];

    if (MAX_SLOTS <= numSlots) {
	_NXLogError("*** -[Storage setNumSlots:]: numSlots cannot be greater than 0xD0000000\n");
	return self;
    }

    if (numSlots > maxElements)
      {
	volatile void *tempDataPtr;
	unsigned old = maxElements;
	
	maxElements = numSlots;
	tempDataPtr = (void*)NXZoneRealloc(zone, dataPtr, maxElements * elementSize);
	dataPtr = (void*)tempDataPtr;
	bzero(dataPtr + old * elementSize, (maxElements - old) * elementSize);
      }
    numElements = numSlots;
    return self;
}

- setAvailableCapacity:(unsigned)numSlots
{
    unsigned	old = maxElements;
    NXZone *zone = [self zone];
    volatile void *tempDataPtr;

    if (MAX_SLOTS <= numSlots) {
	_NXLogError("*** -[Storage setAvailableCapacity:]: numSlots cannot be greater than 0xD0000000\n");
	return self;
    }

    if (numSlots < numElements) return nil;
    maxElements = numSlots;
    tempDataPtr = (void*)NXZoneRealloc(zone, dataPtr, maxElements * elementSize);
    dataPtr = (void*)tempDataPtr;
    if (maxElements > old)
	bzero(dataPtr + old * elementSize, (maxElements - old) * elementSize);
    return self;
}

- (const char *) description
{
    return description;
}


- (unsigned) count
{
    return numElements;
}

- (void *)elementAt :(unsigned)index
{
    if (index < numElements)
	return ELMT(dataPtr, index, elementSize);
    else
	return NULL;
}

/* For 2.0 compatibility */
- replace:(void *)anElement at:(unsigned )index
{
  return [self replaceElementAt:index with:anElement];
}

- replaceElementAt:(unsigned)index with:(void *)anElement;
{
    if (index < numElements)
	bcopy (anElement, ELMT(dataPtr, index, elementSize), elementSize);
    return self;
}

- addElement:(void *)anElement 
{
    unsigned	index = numElements;
    
    if (MAX_SLOTS <= numElements + 1) {
	_NXLogError("*** -[Storage addElement:]: adding element would cause numSlots to be greater than 0xD0000000\n");
	return self;
    }

    [self setNumSlots:(numElements + 1)];
    bcopy (anElement, ELMT(dataPtr, index, elementSize), elementSize);
    return self;
}


- removeLastElement
{
    if (0 < numElements)
	[self setNumSlots:(numElements - 1)];
    return self;
}

/* For 2.0 compatibility */
- insert:(void *)anElement at:(unsigned)index
{
  return [self insertElement:anElement at:index];
}

- insertElement:(void *)anElement at:(unsigned)index; 
{
    if (MAX_SLOTS <= index) {
	_NXLogError("*** -[Storage insertElement:at:]: index cannot be greater than 0xD0000000\n");
	return self;
    }

    if (index < numElements) {
    	char	*start;
        [self setNumSlots:(numElements + 1)];
	start = ELMT(dataPtr, index, elementSize);
	bcopy (start, start + elementSize, (numElements - index - 1) * elementSize); 
    } else
        [self setNumSlots:(index + 1)];
    bcopy (anElement, ELMT(dataPtr, index, elementSize), elementSize);
    return self;
}


/* For 2.0 compatibility */
- removeAt:(unsigned)index
{
  return [self removeElementAt:index];
}

- removeElementAt:(unsigned)index; 
{
    if (index < numElements) {
	char	*start = ELMT(dataPtr, index, elementSize);
	
	bcopy(start + elementSize, start, (numElements - index - 1) * elementSize);
	[self setNumSlots:(numElements - 1)];
    }
    return self;
}

@end

#endif // not KERNEL
