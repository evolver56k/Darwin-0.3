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
 * AtapiCnt.m - Implementation of ATAPI controller class.
 *
 * HISTORY
 *
 * 4-Jan-1998	Joe Liu at Apple
 *	Modified the ATAPI to SCSI translation routines.
 *
 * 31-Aug-1994 	Rakesh Dubey at NeXT 
 *	Created. 
 */

//#define DEBUG
//#define COMMAND_HISTORY
//#define COMMAND_PRINT   

#import "IdeCnt.h"
#import "AtapiCnt.h"
#import "AtapiCntInternal.h"
#import <kern/assert.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import <machdep/i386/io_inline.h>
#import <bsd/dev/scsireg.h>

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

#ifdef undef
static void testDebug(id driver);
#endif undef

/*
 * List of SCSI commands that do not have a counterpart in ATAPI
 * implementation, or commands which have different command structure.
 * These commands will need to be specially handled. 
 */
#define C10OP_MODESELECT 0x55

static unsigned char mapToAtapi[] = {
    C6OP_MODESELECT,
	C10OP_MODESELECT, 
    C6OP_MODESENSE
};

/*
 * List of controllers that have been already probed. We need this since each
 * Instance table lists IdeDisk as well as IdeController classes. And we need
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


#ifdef undef
    IOLog("AtapiController probed with direct device %x\n", direct);
#endif undef
    
    for (i = 0; i < probedControllerCount; i++)	{
    	if (probedControllers[i] == direct)	{
		{
#ifdef undef
			IOLog("AtapiController already probed for controller %x\n", 
			direct);
#endif undef
	    	return YES;
		}
	}
    }
    
    probedControllers[probedControllerCount++] = direct;

    for (unit = 0; unit < [direct numDevices]; unit++) {
    
	if ([direct isAtapiDevice:unit]) {
	    atapiCnt = [[self alloc] 
	    	initFromDeviceDescription:deviceDescription];
	    
	    if (atapiCnt == nil) {
			IOLog("ATAPI: failed to probe device %d.\n", unit);
			continue;
	    }

#ifdef undef
		IOLog("ATAPI: found ATAPI device %d.\n", unit);
#endif undef

	    if ([atapiCnt initResources:direct] == nil)	{
			IOLog("ATAPI: failed to initialize device %d.\n", unit);
	    	[atapiCnt free];
			continue;
		}

	    /*
	     * To clear pending Unit attention. Not needed.
	     */
	    //[direct atapiRequestSense:NULL];

	    /*
	     * Use this to test before we call registerDevice. 
	     */
	    //testDebug(direct);
	    	    
	    if ([atapiCnt registerDevice] == nil) {
			IOLog("ATAPI: failed to register device %d.\n", unit);
			[atapiCnt free];
			continue;
	    } else {
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
#define C10OP_READCAPACITY	0x25    /* read capacity */

/*
 * This will enable us to print last 32 commands in case of an error. The
 * command that caused the error is printed last. 
 */
#ifdef COMMAND_HISTORY
#define MAXCMDS 32
static atapiIoReq_t cmdBuf[MAXCMDS];
static unsigned int cmdPos = 0;
static unsigned int cmdsInBuf = 0;
#endif COMMAND_HISTORY

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
	BOOL cmdMapped = NO;
   
	[_ataController atapiCntrlrLock];
    
    my_cdb = scsiReq->cdb;

#ifdef DEBUG
     if ((scsiReq->lun == 0) && (scsiReq->target < 2)) 	{
	IOLog("%s: executeRequest %x target %x lun %x Read %d\n", 
	    [self name], my_cdb.cdb_opcode, scsiReq->target, scsiReq->lun,
	    scsiReq->read);
    }
#endif DEBUG
    
    bzero(&atapiIoReq, sizeof(atapiIoReq_t));
    
    atapiIoReq.cmdLen = [_ataController 
		atapiCommandPacketSize:scsiReq->target];
    	
    atapiIoReq.read        = scsiReq->read;
    atapiIoReq.maxTransfer = scsiReq->maxTransfer;
    atapiIoReq.drive       = scsiReq->target;
    atapiIoReq.lun         = scsiReq->lun;
	atapiIoReq.timeout     = scsiReq->timeoutLength * 1000;	// sec to ms
    
	// IOLog("max: %d\n", atapiIoReq.maxTransfer);
	
    scsiCmd = (unsigned char *) &(my_cdb.cdb_opcode);
	atapiIoReq.scsiCmd = *scsiCmd;

    for (i = 0; i < [self scsiCmdLen:scsiReq]; i++)	{
    	atapiIoReq.atapiCmd[i] = *scsiCmd;
		scsiCmd++;
    }

    /*
     * Use this to print SCSI commands being sent to ATA object. 
     */

#ifdef COMMAND_PRINT
    if ((atapiIoReq.atapiCmd[0] == 0x28) && (atapiIoReq.lun == 0))	{
        IOLog("%s: Command 0x28.\n", [self name]);
	for (i = 0; i < [self scsiCmdLen:scsiReq]; i+=2)	{
	    IOLog("%s: %02x %02x\n", [self name], 
		atapiIoReq.atapiCmd[i],  atapiIoReq.atapiCmd[i+1]);
	}
	
    }
    if ((atapiIoReq.atapiCmd[0] == 0x15) && (atapiIoReq.lun == 0))	{
        unsigned char *tmp = (unsigned char *)buffer;
        IOLog("%s: Command 0x15 data.\n", [self name]);
	for (i = 0; i < 0x1c; i+=2)	{
	    IOLog("%s: %02x %02x\n", [self name], 
		tmp[i], tmp[i+1]);
	}
    }
#endif COMMAND_PRINT
	
#ifdef COMMAND_HISTORY
    /* Keep history */
    cmdBuf[cmdPos++] = atapiIoReq;
    if (cmdsInBuf < MAXCMDS)
	cmdsInBuf += 1;
    if (cmdPos == MAXCMDS)
	cmdPos = 0;
#endif COMMAND_HISTORY

    /*
     * Map to available ATAPI command if necessary.
     */
    for (i = 0; i < sizeof(mapToAtapi)/sizeof(mapToAtapi[0]); i++) {
    	if (atapiIoReq.atapiCmd[0] == mapToAtapi[i]) {
			//IOLog("%s: Mapping to SCSI command\n", [self name]);
			bzero((void *)&modeData, sizeof(modeData));
			if ([self maptoAtapiCmd: &atapiIoReq buffer:buffer
				newBuffer:&modeData] == NO)	{
				[_ataController atapiCntrlrUnLock];
				return SR_IOST_CMDREJ;
			}
			cmdMapped = YES;
			break;
		}
    }
	
    /*
     * If emulation is successful then return quickly. 
     */
    if ([self emulateSCSICmd: &atapiIoReq buffer:buffer] == YES) {
		[_ataController atapiCntrlrUnLock];
		return SR_IOST_GOOD;
    }

    /*
     * Send the command to the ioThread. 
     */
    atapiBuf = [self allocAtapiBuf];
    atapiBuf->atapiIoReq = &atapiIoReq;
	if (cmdMapped) {
		atapiBuf->buffer = (void *)&modeData;
		atapiBuf->client = IOVmTaskSelf();
	}
	else {
		atapiBuf->buffer = buffer;
		atapiBuf->client = client;
	}
    atapiBuf->command = ATAPI_CNT_IOREQ;
    driverStatus = [self enqueueAtapiBuf:atapiBuf];
    ret = atapiBuf->status;		// SCSI status

    /*
     * Re-map to SCSI command if necessary.
     */
	if (driverStatus == SR_IOST_GOOD) {
    for (i = 0; i < sizeof(mapToAtapi)/sizeof(mapToAtapi[0]); i++) {
    	if (atapiIoReq.scsiCmd == mapToAtapi[i]) {
			if ([self maptoSCSICmd: &atapiIoReq buffer:buffer
				newBuffer:&modeData] == NO)	{
				[_ataController atapiCntrlrUnLock];
				return SR_IOST_CMDREJ;
			}
			break;
		}
    }
	}

    [self freeAtapiBuf:atapiBuf];

    /*
     * Use this to filter and print data returned from the device. 
     */
#ifdef DEBUG
    {
    	unsigned char *buf = (unsigned char *)buffer;
	if ((atapiIoReq.atapiCmd[0] == 0x25) && (atapiIoReq.lun == 0))	{
	    IOLog("%s: Returned data.\n", [self name]);
	    for (i = 0; i < atapiIoReq.bytesTransferred; i+=4)	{
		IOLog("%s: %02x %02x %02x %02x\n", [self name], buf[i], 
		    buf[i+1], buf[i+2], buf[i+3]);
	    }
	    IOLog("%s: total bytes transferred = %d\n", [self name], 
			atapiIoReq.bytesTransferred);
	    IOLog("%s: scsi status: %x driver status: %x\n", [self name],
	    	atapiIoReq.scsiStatus, ret);
	}

    }
#endif DEBUG

    /*
     * This is a workaround for the Mitsumi Read CD-ROM capacity bug. It
     * reports the block size as 2352 bytes instead of 2048 bytes. It is
     * including preamble and other info. I have seen this on MITSUMI CD-ROM
     * !B B02. The workaround is truncate the returned value. Remove this
     * when it is no longer needed. 
     */
    {
	unsigned int blockSize, value;
    	char *buf = (char *)buffer;
	if ((atapiIoReq.atapiCmd[0] == C10OP_READCAPACITY) && 
		(atapiIoReq.lun == 0))	{
	    blockSize = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | 
	    		buf[7];
	    if (blockSize == 0)			/* self defense */
	    	blockSize = 2048;
	    for (value = 16; value < blockSize; value *= 2)
	    	;
	    if (value > blockSize)	{
		//IOLog("%s: block size set to %d from %d\n", [self name], value/2, blockSize);
	    	blockSize = value / 2;
		buf[4] = (blockSize >> 24) & 0xff;
		buf[5] = (blockSize >> 16) & 0xff;
		buf[6] = (blockSize >> 8) & 0xff;
		buf[7] = blockSize & 0xff;
	    }
	}
   }
    
#ifdef COMMAND_HISTORY
	{
	    int j, k;
	    	
	    /* Print history in case of error. */
	    if ((ret != SR_IOST_GOOD) && (atapiIoReq.atapiCmd[0] != 0x00)
	    		&& (atapiIoReq.atapiCmd[0] != 0x12))	{
	        IOLog("%s: Command %02x failed. Backtrace..\n", [self name], 
			atapiIoReq.atapiCmd[0]);
		i = (cmdPos - cmdsInBuf) % MAXCMDS;
		k = (int) cmdsInBuf;
		while (--k >= 0)	{
		    IOLog("%s: ", [self name]);
		    for (j = 0; j < 12; j++)	{
			IOLog("%02x ", cmdBuf[i].atapiCmd[j]);
		    }
		    IOLog("\n");
		    if (i == MAXCMDS)
			i = 0;
		    else
			i += 1;
		}
		cmdsInBuf = 0;
	    } 
	}
#endif COMMAND_HISTORY

    scsiReq->bytesTransferred = atapiIoReq.bytesTransferred;
    scsiReq->scsiStatus = atapiIoReq.scsiStatus;
    scsiReq->driverStatus = driverStatus;

	[_ataController atapiCntrlrUnLock];
    
    return ret;    
}

#define MPH_SCSI_6_SIZE		sizeof(mode_sel_hdr_t)
#define MPH_SCSI_10_SIZE	8
#define MPH_ATAPI_SIZE		sizeof(atapiMPH_t)
#define MPH_DELTA			(MPH_ATAPI_SIZE - MPH_SCSI_6_SIZE)

/*
 * Map SCSI commands into ATAPI commands.
 * We use this to modify the SCSI Mode Sense/Select commands to fit
 * the ATAPI protocol.
 *
 * FIXME - We do not handle multiple mode pages. Only the first page is
 * translated.
 */
- (BOOL) maptoAtapiCmd:(atapiIoReq_t *)atapiIoReq 
                buffer:(void *)buffer
             newBuffer:(atapiMPL_t *)mode
{
    int i, pageLength, bd_len, page_start, hdr_size, maxTransfer;
    unsigned char *data = (unsigned char *)buffer;

	/*
	 * Modify the SCSI mode sense (6) command.
	 *
	 * SCSI mode sense (10) should be fine for ATAPI since it matches
	 * the ATAPI spec very closely.
	 */	
    if (atapiIoReq->scsiCmd == C6OP_MODESENSE) {

		/*
		 * Our ATAPI buffer must be MPH_DELTA bytes larger than the size
		 * of the SCSI buffer passed in.
		 */
		if ((atapiIoReq->atapiCmd[4] + MPH_DELTA) > sizeof(atapiMPL_t))
			return NO;

		/*
		 * Transform the SCSI mode sense (6) command into a SCSI
		 * mode sense (10) command.
		 */
		atapiIoReq->atapiCmd[0] = C10OP_MODESENSE;
		atapiIoReq->atapiCmd[8] = atapiIoReq->atapiCmd[4] + MPH_DELTA;
		atapiIoReq->atapiCmd[4] = 0;
		atapiIoReq->atapiCmd[5] = 0;
		
		/* Increase the maxTransfer by 4 bytes, since in SCSI Mode
		 * Sense(6), the Mode Parameter Header is only 4 bytes long,
		 * while in ATAPI, it is always 8-bytes. Therefore, we may
		 * need to transfer 4 additional bytes over the set SCSI limit.
		 * We also know that we have enough storage since we did the
		 * check earlier.
		 */
		atapiIoReq->maxTransfer += MPH_DELTA;
		
		return YES;
    } 

	/*
	 * Modify the SCSI mode select commands.
	 */		 
    if ((atapiIoReq->scsiCmd == C6OP_MODESELECT) || 
		(atapiIoReq->scsiCmd == C10OP_MODESELECT)) {
		
		if (atapiIoReq->scsiCmd == C6OP_MODESELECT) {
			atapiIoReq->atapiCmd[0] = C10OP_MODESELECT;
			atapiIoReq->atapiCmd[4] = 0;
			atapiIoReq->atapiCmd[5] = 0;		/* reset control field */
			
			bd_len = data[3];					/* block descriptor length */
			hdr_size = MPH_SCSI_6_SIZE;			/* mode parameter hdr size */
		}
		else {	// C10OP_MODESELECT
			atapiIoReq->atapiCmd[9] = 0;		/* reset control field */
			bd_len = (data[6] << 8) | data[7];	/* block descriptor length */
			hdr_size = MPH_SCSI_10_SIZE;		/* mode parameter hdr size */
		}
		
		page_start = hdr_size + bd_len;			/* start of mode page */
		pageLength = data[page_start + 1] + 2;	/* total size of mode page */
		if (pageLength > MODSEL_DATA_LEN)		/* page too large */
			return NO;
		
		/* copy the mode page to our own buffer space */
		for (i = 0; i < pageLength; i++) {
			mode->pageData[i] = data[page_start + i];
		}
		
		/* Update allocation length in the mode select command.
		 * length = page size + ATAPI header size
		 */
		maxTransfer = pageLength + MPH_ATAPI_SIZE;
		atapiIoReq->atapiCmd[8] = maxTransfer & 0xff;			/* LSB */
		atapiIoReq->atapiCmd[7] = (maxTransfer >> 8) & 0xff;	/* MSB */
	    
		/*
		 * Update the maxTransfer count.
		 */
		atapiIoReq->maxTransfer = maxTransfer;
	    return YES;
	}
    
    return YES;
}

/*
 * Map the result of ATAPI commands into their SCSI counterparts.
 *
 * FIXME - We do not handle multiple mode pages. Only the first page is
 * translated.
 */
- (BOOL) maptoSCSICmd:(atapiIoReq_t *)atapiIoReq
               buffer:(void *)buffer
            newBuffer:(atapiMPL_t *)mode
{
    int i, pageLength;
    unsigned char *data = (unsigned char *)buffer;

	/*
	 * If the executed command was originally a SCSI Mode Sense(6)
	 * command, we need to modify the result since the returned
	 * Page Parameter Header size is now 8-bytes instead of the
	 * expected 4-bytes.
	 */ 
    if ((atapiIoReq->scsiCmd == C6OP_MODESENSE) &&
		(atapiIoReq->bytesTransferred >= 10)) {
		
		// bytesTransferred must be greater than 10 or otherwise the
		// PageLength field in the page (byte 1) would not be valid.

		pageLength = mode->pageData[1] + 2;
		
		/* Mode Page + Mode Header must fit in the SCSI buffer.
		 * Remember that the value in atapiCmd[8] was
		 * artificially increased by 4 bytes, so we need to
		 * take that into account.
		 */
		if ((pageLength + MPH_SCSI_6_SIZE) >
			(atapiIoReq->atapiCmd[8] - MPH_DELTA)) {
			/* SCSI buffer is too small */
#ifdef DEBUG
			IOLog("maptoSCSICmd:  Mode Select buffer too small\n");
#endif DEBUG
			return NO;
		}
		
		data[0] = mode->mph.mdl0;		// mode data length
		data[1] = mode->mph.mt;			// medium type
		data[2] = 0;					// device-specific parameter
		data[3] = 0;					// block descriptor length
		
		for (i = 0; i < pageLength; i++) {
			data[4 + i] = mode->pageData[i];
		}
		
		/*
		 * Modify bytesTransferred count to make it appear that
		 * we transferred 4 less bytes.
		 */
		atapiIoReq->bytesTransferred -= MPH_DELTA;
		return YES;
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
    if (atapiIoReq->atapiCmd[0] == C10OP_MODESENSE) {
	if ((atapiIoReq->atapiCmd[2] & 0x1f) == 0x02) {
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
    
	if ([_ataController isAtapiDevice:unit])	{
	
	    [_ataController atapiCntrlrLock];
	    ret = [_ataController atapiSoftReset:unit];
	    [_ataController atapiCntrlrUnLock];
	    
	    if (ret != IDER_SUCCESS)	{
	    	IOLog("%s: ATAPI reset failed.\n", [self name]);
		status = SR_IOST_HW;
	    }
	}
    }
    
    return status;
}

#ifdef undef
/* For testing individual commands. */
static void testDebug(id driver)
{
    void *buffer;
    atapiIoReq_t atapiIoReq;
    
    buffer = IOMalloc(2048);
    bzero(&atapiIoReq, sizeof(atapiIoReq_t));
    
    atapiIoReq.cmdLen = 12;		
    atapiIoReq.read = 1;
    atapiIoReq.drive = 0;
    atapiIoReq.lun = 0;
    
    atapiIoReq.atapiCmd[0] = 0x28;
    atapiIoReq.atapiCmd[2] = 0;
    atapiIoReq.atapiCmd[3] = 0;
    atapiIoReq.atapiCmd[4] = 0;
    atapiIoReq.atapiCmd[5] = 5;
    atapiIoReq.atapiCmd[7] = 0;
    atapiIoReq.atapiCmd[8] = 1;
    
   [driver atapiExecuteCmd:&atapiIoReq 
    		buffer:buffer client:IOVmTaskSelf()];
		
}
#endif undef

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

