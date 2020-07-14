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
    HashTable.h
    Copyright 1988, 1989 NeXT, Inc.
	
    DEFINED AS: A common class
    HEADER FILES: objc/HashTable.h

*/

#ifndef _OBJC_HASHTABLE_H_
#define _OBJC_HASHTABLE_H_

#import "Object.h"
#import "hashtable.h"
#import "typedstream.h"

@interface HashTable: Object
{
    unsigned	count;		/* Current number of associations */
    const char	*keyDesc;	/* Description of keys */
    const char	*valueDesc;	/* Description of values */
    unsigned	_nbBuckets;	/* Current size of the array */
    void	*_buckets;	/* Data array */
}

/* Initializing */

- init;
- initKeyDesc: (const char *)aKeyDesc;
- initKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc;
- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc 
	capacity: (unsigned) aCapacity;

/* Freeing */

- free;	
- freeObjects;
- freeKeys:(void (*) (void *))keyFunc values:(void (*) (void *))valueFunc;
- empty;

/* Copying */

- copyFromZone:(NXZone *)zone;
  
/* Manipulating */

- (unsigned)count;
- (BOOL)isKey:(const void *)aKey;
- (void *)valueForKey:(const void *)aKey;
- (void *)insertKey:(const void *)aKey value:(void *)aValue;
- (void *)removeKey:(const void *)aKey;

/* Iterating */

- (NXHashState)initState;
- (BOOL)nextState:(NXHashState *)aState key:(const void **)aKey 
	value:(void **)aValue;

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new;
+ newKeyDesc: (const char *)aKeyDesc;
+ newKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc;
+ newKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc 
	capacity:(unsigned)aCapacity;

@end

#endif /* _OBJC_HASHTABLE_H_ */
