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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdePIIX.h - PIIX specific initialization methods for Ide controller
 * device class.
 *
 * HISTORY
 *
 * 1-May-1998 	Joe Liu at Apple
 *      Created. 
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_IDEPIIX_H
#define _BSD_DEV_IDEPIIX_H 1

#import <driverkit/i386/PCI.h>
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import "PIIX.h"
#import "AtapiCntCmds.h"

@interface IdeController(PIIX)

/*
 * Generic PCI controller methods.
 * Those methods are "exported" to other categories in IdeController class.
 *
 * In the future, if we support additional PCI controllers, we could make
 * this category into a separate object. Each controller object would then
 * export a protocol similar to what is declared below.
 */
- (BOOL) probePCIController:(IOPCIDeviceDescription *)devDesc;

- (void) getPCIControllerCapabilities:(txferModes_t *)modes;

- (BOOL) setPCIControllerCapabilitiesForDrives:(driveInfo_t *)drives;

- (ideTransferWidth_t) getPIOTransferWidth;

- (void) resetPCIController;

- (ide_return_t) performDMA:(ideIoReq_t *)ideIoReq;

- (sc_status_t) performATAPIDMA:(atapiIoReq_t *)atapiIoReq
	buffer:(void *)buffer 
	client:(struct vm_map *)client;

/*
 * PIIX specific (private) methods. They all start with the 'PIIX' prefix.
 * Those methods should not be called directly outside of the PIIX category.
 */
- (BOOL) PIIXInitController:(IOPCIDeviceDescription *)devDesc;

- (void) PIIXResetTimings:(IOPCIDeviceDescription *)devDesc;

- (void) PIIXComputePCIConfigSpace:(IOPCIConfigSpace *)configSpace
         forDrives:(driveInfo_t *)drv;

- (BOOL) PIIXRegisterBMRange:(IOPCIDeviceDescription *)devDesc;

- (BOOL) PIIXInitPRDTable;

- (void) PIIXReportTimings:(piix_idetim_u)tim
              slaveTiming:(piix_sidetim_u)stim
                isPrimary:(BOOL)primary;

- (void) PIIXInit;

@end

#endif	/* _BSD_DEV_IDEPIIX_H */

#endif	/* DRIVER_PRIVATE */

