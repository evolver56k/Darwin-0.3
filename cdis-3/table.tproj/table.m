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
* Copyright 1994 NeXT Computer, Inc.
* All rights reserved.
*/

#import <libc.h>
#import <stdio.h>
#import <objc/NXStringTable.h>
#import <Foundation/Foundation.h>

void
usage(void)
{
    fprintf(stderr, "usage: table <key> <table file>\n");
}

int
main(int argc, char **argv)
{
    const char *key;
    const char *tablePath;
    const char *value;
    id table;

    if (argc != 3) {
        usage();
        exit(1);
    }
    key = argv[1];
    tablePath = argv[2];

    table = [[NXStringTable alloc] init];
    if ([table readFromFile:tablePath] == nil)
        exit(1);

    value = [table valueForStringKey:key];
    if (value)
        printf(value);
    else
        exit(1);
    
    exit(0);
}
