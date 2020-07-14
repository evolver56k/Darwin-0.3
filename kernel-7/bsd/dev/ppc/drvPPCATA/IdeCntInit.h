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
/* 	Copyright (c) 1994-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCntInit.h - Initialization methods for Ide controller device class. 
 *
 * HISTORY
 *
 * 11-Aug-1994 	Rakesh Dubey at NeXT
 *      Created. 
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_I386_IDECNTINIT_H_
#define _BSD_DEV_I386_IDECNTINIT_H_

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/generalFuncs.h>
#import "IdeCnt.h"
#import "IdeCntPublic.h"

@interface IdeController(Initialize)

- (BOOL) checkMediaBayOption: (IOTreeDevice *) deviceDescription;

-(BOOL)controllerPresent;

- (BOOL)ideControllerInit:(IOTreeDevice *)devDesc;

- (BOOL)ideDetectDrive:(unsigned int)unit;

- (BOOL)ideDetectATAPIDevice:(unsigned int)unit;

/*
 * This is essential info on the IDE drive. It is either from the BIOS or
 * from IDE_IDENTIFY_DRIVE command. 
 */
- (ideDriveInfo_t)getIdeDriveInfo:(unsigned int)unit;

- (void)ideReset; 

/*
 * Initializes the IDE interface by sending commands to controller. 
 */
- (void)resetAndInit;

- (ide_return_t)getTransferModes:(ideIdentifyInfo_t *)infoPtr Unit: (int)unit;
		
-(void) calcIdeConfig:(int)unit;
-(void) calcIdeTimingsCmd646X:(int) unit;
-(void) calcIdeTimingsDBDMA:(int) unit;

- (ide_return_t)setATADriveCapabilities:(unsigned int)unit;	

/*
 * Returns NULL if the drive does not support the optional IDE_IDENTIFY_DRIVE
 * command. 
 */
- (ideIdentifyInfo_t *)getIdeIdentifyInfo:(unsigned int)unit;

-(void)endianSwapIdentifyData:(ideIdentifyInfo_t *)pId;

-(void) assignRegisterAddresses: (IOTreeDevice *) deviceDescription;

@end

#endif	_BSD_DEV_I386_IDECNTINIT_H_

#endif	DRIVER_PRIVATE

