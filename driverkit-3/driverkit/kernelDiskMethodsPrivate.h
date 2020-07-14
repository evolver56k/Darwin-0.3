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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * kernelDiskMethodsPrivate.h - private Kernel devsw glue for IODisk class.
 *
 * HISTORY
 * 20-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */
#import <driverkit/return.h>
#import <driverkit/IODisk.h>

@interface IODisk(kernelDiskMethodsPrivate)

/*
 * Register either a LogicalDisk or a DiskObject subclass with the owner's
 * IODevAndIdInfo array.
 */
- (void)registerUnixDisk 		:(int) partition;

- (void)unregisterUnixDisk		:(int) partition;


@end

