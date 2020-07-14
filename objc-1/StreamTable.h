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
    StreamTable.h
    Copyright 1989 NeXT, Inc.
    
    DEFINED AS:	A common class
    HEADER FILES:	objc/StreamTable.h

*/
 
#ifndef _OBJC_STREAMTABLE_H_
#define _OBJC_STREAMTABLE_H_

#import "HashTable.h"

@interface StreamTable: HashTable 
	
/* Creating, freeing, and initializing */

- free;	
- freeObjects;	
- init;
- initKeyDesc:(const char *)aKeyDesc;

/* Manipulating */

- valueForStreamKey:(const void *)aKey;
- insertStreamKey:(const void *)aKey value:aValue;
- removeStreamKey:(const void *)aKey;

/* Iterating */

- (NXHashState)initStreamState;
- (BOOL)nextStreamState:(NXHashState *)aState key:(const void **)aKey 
	value:(id *)aValue;

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new;
+ newKeyDesc:(const char *)aKeyDesc;

@end

#endif /* _OBJC_STREAMTABLE_H_ */
