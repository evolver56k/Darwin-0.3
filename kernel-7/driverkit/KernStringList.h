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
 * A List object for C-strings.
 *
 * Its sole purpose is to make whitespace-separated lists of words
 * easier to deal with, since we use them a lot in driverkit string tables.
 *
 *
 */
 
#import <objc/Object.h>

@interface KernStringList : Object
{
@private
    char	**strings;
    unsigned	count;
}

/*
 * Takes a whitespace-separated list of words,
 * and separates it into a list of null-terminated strings.
 */
- initWithWhitespaceDelimitedString:(const char *)string;

- (unsigned)count;
- (const char *)stringAt:(unsigned)index;
- (const char *)lastString;

@end
