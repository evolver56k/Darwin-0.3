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
 * AtapiCntCmds.h - ATAPI command interface for ATA. 
 *
 * HISTORY 
 * 1-Sep-1994	 Rakesh Dubey at NeXT
 *	Created.
 */

#import "atapi_extern.h"
#import "IdeCnt.h"
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "io_inline.h"
#import <driverkit/scsiTypes.h>
#import <string.h>

#define MAX_ATAPI_CMD_LEN	16

typedef struct {
    unsigned char drive;
    unsigned char lun;				/* dream on */
    unsigned char atapiCmd[MAX_ATAPI_CMD_LEN];
    unsigned char cmdLen;
    unsigned int maxTransfer;
    BOOL  read;
    esense_reply_t senseData;
    unsigned int bytesTransferred;
    unsigned char scsiStatus;
} atapiIoReq_t;


@interface IdeController(ATAPI)

-(unsigned char)atapiCommandPacketSize:(unsigned char)unit;

-(atapi_return_t)atapiWaitForNotBusy;

// only for ATAPI Identify Device
- (ide_return_t)atapiIdentifyDeviceWaitForDataReady;	

-(atapi_return_t)atapiSoftReset:(unsigned char)unit;

-(void) printAtapiInfo:(ideIdentifyInfo_t *)ideIdentifyInfo
		Device:(unsigned char)unit;

-(atapi_return_t)atapiIdentifyDevice:(struct vm_map *)client
			    	addr:(caddr_t)xferAddr
				unit:(unsigned char)unit;
				
-(void)atapiInitParameters:(ideIdentifyInfo_t *)infoPtr 
		Device:(unsigned char)unit;
		
-(atapi_return_t)atapiPacketCommand;

- (void) sendAtapiCommand:(unsigned char *)atapiCmd
	cmdLen:(unsigned char)len;
	
- (void)dumpStatus:(atapiIoReq_t *)atapiIoReq;

- (sc_status_t) atapiExecuteCmd:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client;

- (sc_status_t) atapiXferPIO:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client;

- (sc_status_t) atapiXferDMA:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client;

- (BOOL) atapiDmaAllowed: (void *)buffer;

@end

/*
 * So that we can use ATAPI naming convention and still use the ATA
 * registers. 
 */
#define interruptReason		sectCnt
#define byteCountLow		cylLow
#define byteCountHigh		cylHigh
#define driveSelect		drHead
