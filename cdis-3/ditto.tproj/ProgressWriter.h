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
#import <Foundation/Foundation.h>
#import <IndexingKit/unixtree.h>

@interface ProgressWriter:NSObject <UnixCopierDelegate>
{
    NSString *installString;
    unsigned int installStrLen;
    unsigned int row;
    unsigned int ratio;
    unsigned int incr;
    unsigned int numListed;
    unsigned int tickCol;
    unsigned long objectsCopied;
    unsigned long numObjects;
    unsigned long objectsSkipped;
    unsigned int realCols;
    unsigned int maxLen;
    BOOL thermo;
    BOOL fbshow;
}

- initWithBom:(UnixBTree *)bom useThermometer:(BOOL)useThermo usefbshow:(BOOL)usefbshow;
@end

