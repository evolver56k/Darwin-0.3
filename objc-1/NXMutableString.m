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
	NXMutableString.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/
#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXStringPrivate.h"

@implementation NXMutableString

- (void)replaceCharactersInRange:(NXRange)range withString:(NXString *)string
{
    [self subclassResponsibility:_cmd];
}

- (void)insertString:(NXString *)string at:(unsigned)loc
{
    NXRange range = {loc, 0};
    [self replaceCharactersInRange:range withString:string];
}    

- (void)appendString:(NXString *)string
{
    NXRange range = {[self length], 0};
    [self replaceCharactersInRange:range withString:string];
}

- (void)deleteCharactersInRange:(NXRange)range
{
    [self replaceCharactersInRange:range withString:@""];
}

- mutableCopyFromZone:(NXZone *)zone
{
    return [self copyFromZone:zone];
}

- immutableCopyFromZone:(NXZone *)zone
{
    return [[NXReadOnlyString allocFromZone:zone] initFromString:self];
}

#ifndef DONT_USE_OLD_NXSTRING_NAMES

/* Compatibility stuff. These are 2.x/3.0 methods which should be preserved for 3.x but removed in 4.0.
*/

- (void)replaceRange:(NXStringRange)range with:(NXString *)string
{
    [self replaceCharactersInRange:range withString:string];
}

- (void)replaceWith:(NXString *)string
{
    [self replaceCharactersInRange:(NXRange){0, [self length]} withString:string];
}

- (void)insert:(NXString *)string at:(unsigned)loc
{
    [self insertString:string at:loc];
}

- (void)append:(NXString *)string
{
    [self appendString:string];
}

- (void)deleteRange:(NXStringRange)range
{
    [self deleteCharactersInRange:range];
}

#endif DONT_USE_OLD_NXSTRING_NAMES

@end

#endif