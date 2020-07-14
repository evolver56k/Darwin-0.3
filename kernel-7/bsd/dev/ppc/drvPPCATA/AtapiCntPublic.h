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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/*
 * Copyright (c) 1994-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * AtapiCntPublic.h - public interface for ATAPI controller class. 
 *
 * HISTORY 
 * 31-Aug-1994	 Rakesh Dubey at NeXT
 *	Created.
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_I386_ATAPICNTPUBLIC_H_
#define _BSD_DEV_I386_ATAPICNTPUBLIC_H_

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/generalFuncs.h>
#import "ata_extern.h"

@protocol AtapiControllerPublic
- (unsigned int)numDevices;			/* devices on the bus */
- (BOOL)isAtapiDevice:(unsigned char)unit;
@end

#endif	_BSD_DEV_I386_ATAPICNTPUBLIC_H_

#endif	DRIVER_PRIVATE
