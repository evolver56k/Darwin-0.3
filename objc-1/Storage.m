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
	Storage.m
  	Copyright 1988, 1989 NeXT, Inc.
	Responsibility: Bertrand Serlet

*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "Storage.h"

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

    if (numSlots > maxElements)
      {
	unsigned old = maxElements;
	
	maxElements = numSlots;
	dataPtr = NXZoneRealloc(zone, dataPtr, maxElements * elementSize);
	bzero(dataPtr + old * elementSize, (maxElements - old) * elementSize);
      }
    numElements = numSlots;
    return self;
}

- setAvailableCapacity:(unsigned)numSlots
{
    unsigned	old = maxElements;
    NXZone *zone = [self zone];

    if (numSlots < numElements) return nil;
    maxElements = numSlots;
    dataPtr = NXZoneRealloc(zone, dataPtr, maxElements * elementSize);
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
    
    [self setNumSlots:(numElements + 1)];
    bcopy (anElement, ELMT(dataPtr, index, elementSize), elementSize);
    return self;
}


- removeLastElement
{
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


- write:(NXTypedStream *) stream
{
    [super write: stream];
    NXWriteTypes (stream, "%ii", &description, &elementSize, &numElements);
    if (numElements)
	NXWriteArray (stream, description, numElements, dataPtr);
    return self;
}

- read:(NXTypedStream *) stream
{
    NXZone *zone = [self zone];

    [super read: stream];
    if (NXTypedStreamClassVersion (stream, "Storage") == 0) {
	unsigned	newSize;
	int		_growAmount;
	NXReadTypes (stream, "*iii", &description, &elementSize, &_growAmount, &numElements);
	maxElements = numElements;
	newSize = maxElements * elementSize;
	dataPtr = (char *) NXZoneMalloc(zone, newSize);
	bzero(dataPtr, newSize);
	NXReadArray (stream, description, numElements, dataPtr);
    } else {
	NXReadTypes (stream, "%ii", &description, &elementSize, &numElements);
	maxElements = numElements;
	if (maxElements) {
	    unsigned	newSize = maxElements * elementSize;
	    dataPtr = (char *) NXZoneMalloc(zone, newSize);
	    bzero(dataPtr, newSize);
	    NXReadArray (stream, description, numElements, dataPtr);
	}
    }
    return self;
}

@end
#endif /* KERNEL */