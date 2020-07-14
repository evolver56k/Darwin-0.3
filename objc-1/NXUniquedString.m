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
	NXUniquedString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <mach/cthreads.h>
#import "NXStringPrivate.h"
#import "HashTable.h"

/* Mutex for mucking around with global stuff */
static struct mutex stringLock;

/* For storing unique strings... Maps objects to objects. */
static HashTable *uniquedStringHashTable = nil;

#if SEPARATE_UNIQUED_ZONE
static NXZone *uniquedStringZone = NULL;	/* Zone for uniqued strings */
#else
#define uniquedStringZone stringZone
#endif


@implementation NXUniquedString

+ initialize
{
    if ([self class] == [NXUniquedString class]) {
        mutex_init(&stringLock);
#if SEPARATE_UNIQUED_ZONE
	uniquedStringZone = NXCreateZone(vm_page_size, vm_page_size, YES);
#endif
        uniquedStringHashTable = [[HashTable allocFromZone:uniquedStringZone] init];
    }
    return self;
}

+ newFromString:(NXString *)string
{
    NXUniquedString *result;
    if ([string isKindOf:self]) {
	result = (NXUniquedString *)string;
    } else {
	mutex_lock(&stringLock);
	if (!(result = [uniquedStringHashTable valueForKey:string])) {
	    unsigned newLength = [string length];
	    result = [self allocFromZone:uniquedStringZone];
	    result->characters = [result allocateCharacterBuffer:newLength];
	    result->_length = newLength;
	    [string getCharacters:result->characters];
	    [uniquedStringHashTable insertKey:result value:result];
	}
	mutex_unlock(&stringLock);
    }
    return result;
}

+ newFromCharacters:(const unichar *)chars length:(unsigned)length
{
    NXReadOnlyString *string = [[NXReadOnlyString alloc] initFromCharactersNoCopy:(unichar *)chars length:length freeWhenDone:NO];
    NXUniquedString *result = [self newFromString:string];
    [string free];
    return result;
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len
{
    NXUniquedString *result;
    NXReadOnlyString *string = [[NXReadOnlyString alloc] initFromCharactersNoCopy:chars length:len freeWhenDone:NO];
    mutex_lock(&stringLock);
    if ((result = [uniquedStringHashTable valueForKey:string])) {
	// Already in the hash table
	[self free];
    } else {	
	// Not in the table; create a new string
	[super initFromCharactersNoCopy:chars length:len];
        [uniquedStringHashTable insertKey:self value:self];
	result = self;
    }
    mutex_unlock(&stringLock);
    [string free];
    return result;
}

+ allocFromZone:(NXZone *)zone
{
    return [super allocFromZone:uniquedStringZone];
}

+ alloc
{
    return [super allocFromZone:uniquedStringZone];
}

- free
{
    return nil;
}

- copyFromZone:(NXZone *)zone
{
    return self;
}

- (BOOL)isEqual:string
{
    static id uniquedStringClass = nil;		// Cache to avoid hash lookup...
    if (!uniquedStringClass) {			// No mutex necessary
	uniquedStringClass = [NXUniquedString class];
    }
    if ([string isKindOf:uniquedStringClass]) {
	return (self == string) ? YES : NO;
    } else {
        return [super isEqual:string];
    }
}

/*
finishUnarchiving not only has to look the string up in the hash table, it also has to make sure the object itself was allocated in the appropriate zone. 
*/

- finishUnarchiving
{
    id actual;
    mutex_lock(&stringLock);
    if (!(actual = [uniquedStringHashTable valueForKey:self])) {
	// If not in this zone, reallocate...
	actual = ([self zone] == uniquedStringZone) ? self : object_copyFromZone (self, 0, uniquedStringZone);
	[uniquedStringHashTable insertKey:actual value:actual];
    } else {
	free (characters);	// Already in the hash table, so we free this guy	
    }    
    mutex_unlock(&stringLock);
    if (actual != self) {
        object_dispose(self);
	return actual;
    } else {
	return nil;
    }
}

@end
#endif /* KERNEL */