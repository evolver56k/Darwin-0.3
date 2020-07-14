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
 * IdeCntCmds.m - IDE commands interface. 
 *
 * HISTORY 
 * 17-July-1994 	Rakesh Dubey at NeXT 
 *	Created. 
 */

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/generalFuncs.h>
#import "IdeCntPublic.h"


/*
 * IDE commands to the controller. These methods are used by the Disk objects
 * via the IdeSendCmd:ToDrive: interface. They also might be invoked by one
 * of the internal methods. 
 */

@interface IdeController(Commands)

/*
 * The following commands use the revelant register values from
 * ideRegsAddrs as inputs. In case of a command failure (i.e., when the
 * returned value is IDER_CMD_ERROR, all registers are dumped into 
 * ideRegsAddrs structure.
 */

- (ide_return_t) ideReadGetInfoCommon:(ideRegsVal_t *)ideRegs 
			client:(struct vm_map *)client 
			addr:(caddr_t)xferAddr 
			command:(unsigned)cmd;

- (ide_return_t) ideReadMultiple:(ideRegsVal_t *)ideRegs 
			client:(struct vm_map *)client 
			addr:(caddr_t)xferAddr;

- (ide_return_t) ideWrite:(ideRegsVal_t *)ideRegs 
			client:(struct vm_map *)client	
			addr:(caddr_t)addr;

- (ide_return_t) ideWriteMultiple:(ideRegsVal_t *)ideRegs 
			client: (struct vm_map *)client 
			addr:(caddr_t)addr;

- (ide_return_t) ideReadVerifySeekCommon:(ideRegsVal_t *)ideRegs 
			command:(unsigned)cmd;

/*
 * ideDiagnose and ideSetParams:numHeads:ForDrive are also used by the
 * controller class. 
 */

- (ide_return_t)ideDiagnose:(unsigned *)error;

- (ide_return_t)ideSetParams:(unsigned)sectCnt numHeads:(unsigned)nHeads
			ForDrive:(unsigned)drive;
			
- (ide_return_t)ideSetDriveFeature:(unsigned)feature value:(unsigned)val;
			
- (ide_return_t) ideRestore:(ideRegsVal_t *)ideRegs;

- (ide_return_t) ideSetMultiSectorMode:(ideRegsVal_t *)ideRegs 
			numSectors:(unsigned char)nSectors;


- (ideRegsVal_t)logToPhys:(unsigned)block numOfBlocks:(unsigned)nblk;

/*
 * These methods are used by the IdeDisk class. 
 */
- (IOReturn) _ideExecuteCmd : (ideIoReq_t *)ideIoReq  
		 ToDrive: (unsigned char) drive;

@end
