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
	StreamTable.m
	Pickling data structures
  	Copyright 1989-1996, NeXT Software, Inc.
	Responsability: Bertrand Serlet
  
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

#import <objc/StreamTable.h>

typedef struct {
    id		object;
    char	*buffer;
    int		length;
    } Pickle;
    
@implementation  StreamTable

+ newKeyDesc: (const char *) aKeyDesc {
    return [super newKeyDesc: aKeyDesc valueDesc: "!"];
    };

+ new {
    return [self newKeyDesc: "@"];
    };

- initKeyDesc: (const char *) aKeyDesc {
    return [self initKeyDesc: aKeyDesc valueDesc: "!"];
    };

- init {
    return [self initKeyDesc: "@"];
    };

static void freePickle (void *value) {
    Pickle	*pick;
    if (! value) return;
    pick = (Pickle *) value;
    free (pick->buffer);
    free (pick);
    };

static void freePickleAndObject (void *value) {
    Pickle	*pick;
    if (! value) return;
    pick = (Pickle *) value;
    [pick->object free];
    free (pick->buffer);
    free (pick);
    };
    
static void freeObject (void *item) {
    [(id) item free];
    };
    
static void noFree (void *item) {
    };
    
- free {
    [self freeKeys: noFree values: freePickle];
    return [super free];
    };
        
- freeObjects {
    return [self freeKeys: (keyDesc[0] == '@') ? freeObject : noFree values: freePickleAndObject];
    };

- (id) valueForStreamKey: (const void *) aKey {
    return nil;
    };
     
- (id) insertStreamKey: (const void *) aKey value: (id) aValue {
    return nil;
    };

- (id) removeStreamKey: (const void *) aKey {
    freePickle ([self removeKey: aKey]);
    return nil;
    };

- (NXHashState) initStreamState {
    return [self initState];
    };
    
- (BOOL) nextStreamState: (NXHashState *) aState key: (const void **) aKey value: (id *) aValue {
    return NO;
    };

@end

#endif // not KERNEL
