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
 * EIDEInspector.h
 *
 * Driver inspector.
 */

#import <appkit/appkit.h>
#import <driverkit/IODeviceMaster.h>
#import <driverkit/IODeviceInspector.h>

@interface EIDEInspector:IODeviceInspector
{
    id	boundingBox;
    id	optionsBox;

	/* Set for Dual EIDE channel.
	 */
	BOOL isDualChannel;
	
	/* Set for PIIX PCI controllers
	 */
	BOOL isDMACapable;

	/*
	 * DMA Selection checkboxes.
	 */
	id	enableDMAMaster;
	id	enableDMASlave;
	id	enableDMAMasterPrimary;
	id	enableDMASlavePrimary;
	id	enableDMAMasterSecondary;
	id	enableDMASlaveSecondary;	
	
    id	multipleSectors;
	id  overrideButton;		// Advanced settings button

	/* Controls for the Primary channel in case of Dual EIDE,
	 * and for the single channel case.
	 */
	id	popUpMasterDual;
	id	popUpSlaveDual;
	id	titleMasterDual;
	id	titleSlaveDual;
	
	id	popUpMasterSingle;
	id  popUpSlaveSingle;	
	id	titleMasterSingle;
	id	titleSlaveSingle;
	
	/* Secondary channel controls for Dual EIDE.
	 */
	id	popUpMasterSec;
	id	popUpSlaveSec;
	id	titleMasterSec;
	id	titleSlaveSec;
	
	/*
	 * Override panels.
	 */
	id	panelDualChannel;
	id	panelSingleChannel;
	
	/*
	 * Bounding boxes within override panels.
	 */
	id	boxPriDual;
	id	boxSecDual;
	id	boxSingle;
	
	NXBundle *myBundle;
}

- init;
- (void)_initButton : button   key : (const char *)key;
- setTable:(NXStringTable *)instance;
- setDMAMode:sender;
- multipleSectors:sender;
- selectOverrides:sender;
- ok:sender;

@end
