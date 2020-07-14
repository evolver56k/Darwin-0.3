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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * SCSIGenericPrivate.h - Private typedefs for Generic SCSI Driver.
 *
 * HISTORY
 * 19-Aug-92    Doug Mitchell at NeXT
 *      Created. 
 */

#ifndef	_DRIVERKIT_SCSIGENERICPRIVATE_H_
#define _DRIVERKIT_SCSIGENERICPRIVATE_H_

#import <driverkit/IODevice.h>
#import <driverkit/return.h>
#import <driverkit/scsiTypes.h>

#define NUM_SG_DEV	4

/*
 * Globals in SCSIGenericKern.m.
 */
extern id sgIdMap[NUM_SG_DEV];

#endif	_DRIVERKIT_SCSIGENERICPRIVATE_H_
