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
 * SCSIGeneric.h - ObjC Interface to Generic SCSI Driver.
 *
 * HISTORY
 * 14-Jun-95	Doug Mitchell at NeXT
 *	Added SCSI-3 support.
 * 19-Aug-92    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/IODevice.h>
#import <driverkit/return.h>
#import <driverkit/scsiTypes.h>

@interface SCSIGeneric:IODevice
{
	unsigned long long _target;
	unsigned long long _lun;
	
	unsigned        _controllerNum;        // SCSIController unit number
	unsigned	_autoSense:1,
			_isReserved,
			_targLunValid:1;
	id		_controller;		// SCSIController to which 
						//    we're attached
	id		_openLock;		// NXLock; serializes opens
						//    and closes
	id		_owner;			// nil means no owner
}

+ (BOOL)probe : deviceDescription;
+ (IODeviceStyle)deviceStyle;
+ (Protocol **)requiredProtocols;

- (int)sgInit			: (unsigned)unitNum 
				  controller : controller;

/*
 * Two flavors of this - we now keep the addresses around as 64-bit
 * data, but we provide a way of obtaining them as unsigned chars 
 * for pre-4.0 code.
 */
- (unsigned char)target;
- (unsigned char)lun;
- (unsigned long long)SCSI3_target;
- (unsigned long long)SCSI3_lun;

/*
 * Acquire and release, to allow exclusive access to this device.
 * -acquire returns non-zero if device in use, else marks device as in use 
 * and returns zero. -release returns non-zero if device not currently
 * in use by 'caller'.
 */
- (int)acquire			: caller;
- (int)release			: caller;

/* 
 * ioctl equivalents.
 */
 
/*
 * Set target and lun. If isRoot is true, this will succeed even if specified
 * target/lun are currently reserved.
 */
- (IOReturn)setTarget		: (unsigned char)target
				  lun:(unsigned char)lun
				  isRoot:(BOOL)isRoot;

- (IOReturn)setSCSI3Target	: (unsigned long long)target
				  lun:(unsigned long long)lun
				  isRoot:(BOOL)isRoot;

/*
 * Set controller number.
 */
- (IOReturn)setController       : (unsigned)controllerNum;

/*
 * Enable/disable 'autosense' mechanism.
 */
- (IOReturn)enableAutoSense;
- (IOReturn)disableAutoSense;
- (int)autoSense;		/* returns _autoSense */

/*
 * Execute CDB. *senseBuf will contain sense data if autosense is enabled
 * and command resulted in Check status.
 */
- (sc_status_t)executeRequest	: (IOSCSIRequest *)scsiReq
			  	   buffer:(void *)buffer /* data destination */
				   client:(vm_task_t)client
				   senseBuf:(esense_reply_t *)senseBuf;
	
- (sc_status_t) executeSCSI3Request : (IOSCSI3Request *)scsiReq
			     buffer : (void *)buffer 	/* data destination */
			     client : (vm_task_t)client
			   senseBuf : (esense_reply_t *)senseBuf;
		  
/*
 * Reset SCSI bus. We can't authorize this; caller should ensure that 
 * client is root!
 */
- (sc_status_t) resetSCSIBus;	

- controller;
@end
