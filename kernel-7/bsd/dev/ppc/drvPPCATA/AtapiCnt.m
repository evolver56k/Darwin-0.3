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
 * Copyright (c) 1995-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * AtapiCnt.m - Implementation of ATAPI controller class.
 *
 *
 * HISTORY 
 * 31-Aug-1994 	Rakesh Dubey at NeXT 
 *	Created. 
 */

#undef DEBUG

#import <kern/assert.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "io_inline.h"

#import "IdeCnt.h"
#import "AtapiCnt.h"
#import "AtapiCntInternal.h"

/*
 * opcode groups
 */
#define	SCSI_OPGROUP(opcode)	((opcode) & 0xe0)

#define	OPGROUP_0		0x00	/* six byte commands */
#define	OPGROUP_1		0x20	/* ten byte commands */
#define	OPGROUP_2		0x40	/* ten byte commands */
#define	OPGROUP_5		0xa0	/* twelve byte commands */
#define	OPGROUP_6		0xc0	/* six byte, vendor unique commands */
#define	OPGROUP_7		0xe0	/* ten byte, vendor unique commands */

/*
 * List of SCSI commands that do not have a counterpart in ATAPI
 * implementation. These commands will need to be mapped individually. 
 */
static unsigned char mapToAtapi[] = {
    C6OP_MODESELECT, 
    C6OP_MODESENSE
};

/*
 * List of controllers that have been already probed. We need this since each
 * Instance table lists IdeDisk as well as IdeCOntroller classes. And we need
 * to create instances of disks attached to each controller only once. 
 */
static int probedControllerCount = 0;
static id probedControllers[MAX_IDE_CONTROLLERS];

@implementation AtapiController

+ (BOOL)probe : deviceDescription
{

    int unit, i;
    id direct;
    id atapiCnt;

    direct = [deviceDescription directDevice];

    for (i = 0; i < probedControllerCount; i++)	
    {
    	if (probedControllers[i] == direct)	
	{
	    return YES;
	}
    }
    
    probedControllers[probedControllerCount++] = direct;

    for (unit = 0; unit < [direct numDevices]; unit++)	
    {
	if ([direct isAtapiDevice:unit])	
        {
	    atapiCnt = [[self alloc]  initFromDeviceDescription:deviceDescription];
	    
	    if (atapiCnt == nil)	
            {
		IOLog("ATAPI: failed to probe device %d.\n", unit);
		continue;
	    }

	    if ([atapiCnt initResources:direct] == nil)	
            {
		IOLog("ATAPI: failed to initialize device %d.\n", unit);
	    	[atapiCnt free];
		continue;
	     }
	    	    
	    if ([atapiCnt registerDevice] == nil)	
            {
		IOLog("ATAPI: failed to register device %d.\n", unit);
		[atapiCnt free];
		continue;
	    } 
            else 
            {
		return YES;
            }
	}
    }	
    return NO;
}

/*
 * We override IOSCSIController to make ourself look like an indirect device
 * in order to get connected to IdeController.
 */
+ (IODeviceStyle)deviceStyle
{
    return IO_IndirectDevice;
}

/*
 * The protocol we need as an indirect device.
 */
static Protocol *protocols[] = {
    @protocol(AtapiControllerPublic),
    nil
};

+ (Protocol **)requiredProtocols
{
    return protocols;
}


- (unsigned short)scsiCmdLen:(IOSCSIRequest *) scsiReq
{
    unsigned char cmdlen;
    union cdb *cdbp = &scsiReq->cdb;

    switch (SCSI_OPGROUP(cdbp->cdb_opcode)) {

      case OPGROUP_0:
	cmdlen = sizeof(struct cdb_6);
	break;

      case OPGROUP_1:
      case OPGROUP_2:
	cmdlen = sizeof(struct cdb_10);
	break;

      case OPGROUP_5:
	cmdlen = sizeof(struct cdb_12);
	break;

      case OPGROUP_6:
	if (scsiReq->cdbLength)
	    cmdlen = scsiReq->cdbLength;
	else
	    cmdlen = sizeof(struct cdb_6);
	break;

      case OPGROUP_7:
	if (scsiReq->cdbLength)
	    cmdlen = scsiReq->cdbLength;
	else
	    cmdlen = sizeof(struct cdb_10);
	break;

      default:
	scsiReq->driverStatus = SR_IOST_CMDREJ;
	return 0;
    }
    
    return cmdlen;
}

#define	C10OP_MODESELECT	0x55	/* OPT: set device parameters */
#define	C10OP_MODESENSE		0x5a	/* OPT: get device parameters */
#define C10OP_READCAPACITY      0x25    /* read capacity */

/*
 * Do a SCSI command, as specified by an IOSCSIRequest.
 */
- (sc_status_t) executeRequest : (IOSCSIRequest *)scsiReq 
		    buffer : (void *)buffer 
		    client : (vm_task_t)client
{
    int i;
    atapiIoReq_t atapiIoReq;
    atapiBuf_t	*atapiBuf;
    unsigned char *scsiCmd;
    cdb_t my_cdb;
    sc_status_t ret;
    IOReturn driverStatus;
   
    [_ataController atapiCntrlrLock];
    
    my_cdb = scsiReq->cdb;
    
    bzero(&atapiIoReq, sizeof(atapiIoReq_t));
    
    atapiIoReq.cmdLen = [_ataController atapiCommandPacketSize:scsiReq->target];
    	
    atapiIoReq.read = scsiReq->read;
    atapiIoReq.maxTransfer = scsiReq->maxTransfer;
    atapiIoReq.drive = scsiReq->target;
    atapiIoReq.lun = scsiReq->lun;
    
    scsiCmd = (unsigned char *) &(my_cdb.cdb_opcode);

    for (i = 0; i < [self scsiCmdLen:scsiReq]; i++)	{
    	atapiIoReq.atapiCmd[i] = *scsiCmd;
	scsiCmd++;
    }

    /*
     * Map to available ATAPI command if necessary.
     */
    for (i = 0; i < sizeof(mapToAtapi)/sizeof(mapToAtapi[0]); i++)	
    {
    	if (atapiIoReq.atapiCmd[0] == mapToAtapi[i])	
        {
	    if ([self maptoAtapiCmd: &atapiIoReq buffer:buffer] == NO)	
            {
		[_ataController atapiCntrlrUnLock];
	    	return SR_IOST_CMDREJ;
	    }
	    break;
	}
    }

    /*
     * If emulation is successful then return quickly. 
     */
    if ([self emulateSCSICmd: &atapiIoReq buffer:buffer] == YES)	
    {
	[_ataController atapiCntrlrUnLock];
	return SR_IOST_GOOD;
    }

    /*
     * Send the command to the ioThread. 
     */
    atapiBuf = [self allocAtapiBuf];
    atapiBuf->atapiIoReq = &atapiIoReq;
    atapiBuf->buffer = buffer;
    atapiBuf->client = client;
    atapiBuf->command = ATAPI_CNT_IOREQ;
    driverStatus = [self enqueueAtapiBuf:atapiBuf];
    ret = atapiBuf->status;		// SCSI status
    [self freeAtapiBuf:atapiBuf];
         
    /*
     * This is a workaround for the Mitsumi Read CD-ROM capacity bug. It
     * reports the block size as 2352 bytes instead of 2048 bytes. It is
     * including preamble and other info. I have seen this on MITSUMI CD-ROM
     * !B B02. The workaround is truncate the returned value. Remove this
     * when it is no longer needed. 
     */
    {
	unsigned int blockSize;
    	char *buf = (char *)buffer;

	if ((atapiIoReq.atapiCmd[0] == C10OP_READCAPACITY) && (atapiIoReq.lun == 0))	
        {
	    blockSize = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];

	    if ( blockSize == 2352 )	
            {
	    	blockSize = 2048;
		buf[4] = (blockSize >> 24) & 0xff;
		buf[5] = (blockSize >> 16) & 0xff;
		buf[6] = (blockSize >> 8) & 0xff;
		buf[7] = blockSize & 0xff;
	    }
	}
   }
       
    scsiReq->bytesTransferred = atapiIoReq.bytesTransferred;
    scsiReq->scsiStatus = atapiIoReq.scsiStatus;
    scsiReq->driverStatus = driverStatus;

    [_ataController atapiCntrlrUnLock];
    
    return ret;    
}

- (BOOL) maptoAtapiCmd:(atapiIoReq_t *)atapiIoReq buffer:(void *)buffer
{
    int i, pageLength;
    unsigned char *data = (unsigned char *)buffer;

    if (atapiIoReq->atapiCmd[0] == C6OP_MODESENSE) 
    {
	atapiIoReq->atapiCmd[0] = C10OP_MODESENSE;
	atapiIoReq->atapiCmd[8] = atapiIoReq->atapiCmd[4];
	atapiIoReq->atapiCmd[4] = 0;
	return YES;
    } 
    
    if (atapiIoReq->atapiCmd[0] == C6OP_MODESELECT) 
    {
	atapiIoReq->atapiCmd[0] = C10OP_MODESELECT;
	atapiIoReq->atapiCmd[8] = atapiIoReq->atapiCmd[4];
	atapiIoReq->atapiCmd[4] = 0;
    
	/*
	 * This is complicated since SCSI and ATAPI have very different
	 * data layouts for mode select. 
	 */
	if ((atapiIoReq->atapiCmd[0] == C10OP_MODESELECT) &&
		(data[3] == 0x08))	{
	    data[0] = data[1] = 0;	/* mode data length is reserved */
	    data[2] = 0;		/* always for CD-ROM. Tape? FIXME */
	    data[3] = 0;		/* reserved for ATAPI */
    
	    /*
	     * Move the data up by four bytes since there only four bytes of
	     * block descriptor in ATAPI while SCSI has eight. 
	     */
	    pageLength = data[13] & 0x01f;
	    for (i = 8; i < pageLength+8; i++)	
            {
		data[i] = data[i+4];
		data[i+4] = 0;
	    }
	    
	    atapiIoReq->atapiCmd[8] = 0x14;	/* 8 + 12 */
	    data[9] = 0;
	    
	    return YES;
	}
    
	/*
	 * Now handle the case where block descriptor length is zero. This is
	 * more messy since we are short of memory. FIXME later.. 
	 */
	 
	 return YES;		/* this will fail now */
    }
    
    return YES;
}

/*
 * If this SCSI command is not supported by ATAPI then try to fake its
 * execution if possible. 
 */
- (BOOL) emulateSCSICmd:(atapiIoReq_t *)atapiIoReq buffer:(void *)buffer
{
    unsigned char *data = (unsigned char *)buffer;
    
    /*
     * We need to fake mode sense page 2. Workspace sends that and this page
     * is reserved in ATAPI. In the SCSI world this page is for
     * disconnect-reconnect and ATAPI doesn't support that now. 
     */
    if (atapiIoReq->atapiCmd[0] == C10OP_MODESENSE) 
    {
	if ((atapiIoReq->atapiCmd[2] & 0x1f) == 0x02) 
        {
	    bzero(data, atapiIoReq->atapiCmd[4]);
	    data[0] = 0x02;
	    data[1] = atapiIoReq->atapiCmd[4];
	    data[2] = 1;		/* our preferred size: 2048 bytes */
	    atapiIoReq->bytesTransferred = atapiIoReq->atapiCmd[4];
	    atapiIoReq->scsiStatus = STAT_GOOD;
	    
	    return YES;
	}
    }
     
    return NO;		/* default */
}


/*
 * Reset all ATAPI devices connected to this controller. Note that we have
 * obtain a lock before resetting the controller. 
 */
- (sc_status_t)resetSCSIBus
{
    int unit;
    atapi_return_t ret;
    sc_status_t status = SR_IOST_GOOD;
    
    for (unit = 0; unit < [_ataController numDevices]; unit++)	{
    
	if ([_ataController isAtapiDevice:unit])	
        {
	    [_ataController atapiCntrlrLock];
	    ret = [_ataController atapiSoftReset:unit];
	    [_ataController atapiCntrlrUnLock];
	    
	    if (ret != IDER_SUCCESS)	
            {
	    	IOLog("%s: ATAPI reset failed.\n", [self name]);
		status = SR_IOST_HW;
	    }
	}
    }
    
    return status;
}

/*
 * Note: ATAPI requires transfer byte counts be even. 
 *       The alignment fields readLength and writeLength are set 2.
 */

- (void)getDMAAlignment:(IODMAAlignment *)alignment
{
	alignment->readStart   = 1;	/* XXX */
	alignment->writeStart  = 1;	/* XXX */
	alignment->readLength  = 2;	/* XXX */
	alignment->writeLength = 2;	/* XXX */
}

/*
 * Set maximum I/O length for SCSI framework.
 *
 * There are 256 DBDMA descriptors available. One is used for a DBDMA STOP command. Another is 
 * potentially used to handle odd-byte transfers. An additional one may be potentially used if
 * the transfer address is not page-aligned, for ex: a 64KB unaligned transfer requires 17 descriptors.
 */
- (unsigned) maxTransfer
{
    return  (MAX_IDE_DESCRIPTORS - 3) * PAGE_SIZE;
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassATAPIController);
    return( self);
}

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    strcat( types, " "IOTypeATAPI);
    return( self);
}


@end

