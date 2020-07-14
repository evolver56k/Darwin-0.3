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
 * AtapiCntCmds.m - ATAPI command implementation for ATA interface. 
 *
 * HISTORY 
 *
 * 4-Jan-1998	Joe Liu at Apple
 *		Merged the various wait routines into a single routine.
 *		Wait routine now sleeps longer to generate a more accurate wait
 *		interval.
 *		Check the BSY bit before sending the packet command.
 *		Detect shadow/phantom drives and timeout more quickly.
 *		Sprinkled some IODelays before checking the BSY bit. Since according
 *		to the spec, a drive may take up to 400ns to set the BSY bit, and
 *		we don't want to read it before it has enough time to assert itself.
 *
 * 3-Sep-1996	Becky Divinski at NeXT
 *	 	Moved LBA, IORDY, and buffer capabilities out
 *		from under DEBUG statements, so they will
 *		always print out during startup.
 *
 * 13-Feb-1996 	Rakesh Dubey at NeXT
 *      More resistant to broken firmwares. 
 *
 * 13-Jul-1995 	Rakesh Dubey at NeXT
 *      Improved device detection and log messages. 
 *
 * 1-Sep-1994	 Rakesh Dubey at NeXT
 *	Created.
 */

#import "AtapiCntCmds.h"
#import "IdePIIX.h"

//#define DEBUG

/*
 * Max timeout while waiting for BSY == 0.
 */
#define ATAPI_MAX_WAIT_FOR_NOTBUSY		(5 * 1000)		// 5 secs

@implementation IdeController(ATAPI)

/*
 * Method: atapiWaitStatusBitsFor:on:off:alt:
 *
 * General purpose wait routine. Wait for BSY bit in the Status/AltStatus
 * register to clear, then make sure that the "on" bits are set, and the
 * "off" bits are cleared in the status register.
 *
 * The routine busy waits for an initial count, then it uses IOSleeps to
 * do non-blocking delays. IOSleep() has a minimum delay of around 15ms.
 * Anything smaller will yield a longer than expected delay.
 *
 * Arguments:
 *  timeout	- How long to wait for BSY bit to clear in milliseconds.
 *  on		- Set bit mask.
 *  off		- Cleared bit mask (BSY bit is implicitly checked).
 *  alt		- YES to poll using AltStatus instead of Status register.
 *
 * Returns:
 *  IDER_SUCCESS - successful completion.
 *  IDER_TIMEOUT - timeout or bits check failed.
 *
 */

#define ATAPI_WAIT_DELAYS		5		// how many initial 1ms busy waits
#define ATAPI_SLEEP_DURATION	20		// how long to sleep using IOSleep

- (atapi_return_t) atapiWaitStatusBitsFor:(unsigned int)timeout
	on:(unsigned char)on
	off:(unsigned char)off
	alt:(BOOL)alt
{
    unsigned char status;
    int  delays = ATAPI_WAIT_DELAYS;
	
	if (timeout == 0)
		return IDER_TIMEOUT;
	
	IODelay(1);	/* give the drive 400ns to assert BSY bit */
	do {
		status = alt ? inb(_ideRegsAddrs.altStatus) :
					   inb(_ideRegsAddrs.status);
		if (!(status & BUSY))
			break;
		if (delays-- > 0) {	// use IODelay for initial wait
			IODelay(1000);	// busy-wait for 1ms
			timeout--;
		}
		else {				// too long, use IOSleep instead
			int sleep_interval = ATAPI_SLEEP_DURATION;
			if (timeout < sleep_interval)
				sleep_interval = timeout;
			IOSleep(sleep_interval);
			timeout -= sleep_interval;
		}
	} while (timeout > 0);
	if (timeout == 0)
		return IDER_TIMEOUT;

	IODelay(1);	/* give the drive another 400ns to update Status Register */
	if (((status & on) == on) && ((~status & off) == off))
		return IDER_SUCCESS;

	return IDER_TIMEOUT;
}

- (void) printAtapiInfo:(ideIdentifyInfo_t *)ideIdentifyInfo 
			Device:(unsigned char)unit
{
    int i;
    char name[50];
    char firmware[9];
    atapiGenConfig_t *atapiGenConfig;
    char *protocolTypeStr, *deviceTypeStr, *cmdDrqTypeStr;
    char *removableStr, *cmdPacketSizeStr;

	// IOLog("Capability: %04x\n", ideIdentifyInfo->capabilities);

    /*
     * Print drive name with firmware revision number after doing byte swaps. 
     */
    for (i = 0; i < 20; i++)	{
	name[2*i] = ideIdentifyInfo->modelNumber[2*i+1];
	name[2*i+1] = ideIdentifyInfo->modelNumber[2*i];
    }
    name[40] = '\0';
    
    for (i = 0; i < 4; i++)	{
	firmware[2*i] = ideIdentifyInfo->firmwareRevision[2*i+1];
	firmware[2*i+1] = ideIdentifyInfo->firmwareRevision[2*i];
    }
    firmware[8] = '\0';
    
    for (i = 38; i >= 0; i--)	{
	if (name[i] != ' ')
	    break;
    }
    strcpy(name+i+2, firmware);
    
#ifdef DEBUG
    IOLog("%s: %s\n", [self name], name);
#endif DEBUG

    /*
     * This information should be printed since it is not duplicated
     * elsewhere. 
     */
    atapiGenConfig = (atapiGenConfig_t *) &ideIdentifyInfo->genConfig;
    
#ifdef undef
    IOLog("%s: gen config: %x\n", [self name], ideIdentifyInfo->genConfig);
#endif undef
    
    if (atapiGenConfig->protocolType == 0 || atapiGenConfig->protocolType == 1)
    	protocolTypeStr = "ATA";
    else if (atapiGenConfig->protocolType == 2)
    	protocolTypeStr = "ATAPI";
    else 
    	protocolTypeStr = "UNKNOWN PROTOCOL";
	
    if (atapiGenConfig->deviceType == 0)
    	deviceTypeStr = "DIRECT ACCESS";
    else if (atapiGenConfig->deviceType == 5)
    	deviceTypeStr = "CD-ROM";
    else if (atapiGenConfig->deviceType == 7)
    	deviceTypeStr = "OPTICAL";
    else if (atapiGenConfig->deviceType == 1)
    	deviceTypeStr = "TAPE";
    else
    	deviceTypeStr = "UNKNOWN DEVICE TYPE";
	
    if (atapiGenConfig->cmdDrqType == 0)
    	cmdDrqTypeStr = "SLOW DRQ";
    else if (atapiGenConfig->cmdDrqType == 1)
    	cmdDrqTypeStr = "INTR DRQ";
    else if (atapiGenConfig->cmdDrqType == 2)
    	cmdDrqTypeStr = "FAST DRQ";
    else 
    	cmdDrqTypeStr = "UNKNOWN DRQ TYPE";
	
    if (atapiGenConfig->removable == 0)
    	removableStr = "NOT REMOVABLE";
    else if (atapiGenConfig->removable == 1)
    	removableStr = "REMOVABLE";
    
    if (atapiGenConfig->cmdPacketSize == 0)
    	cmdPacketSizeStr = "CMD PKT LEN=12";
    else if (atapiGenConfig->cmdPacketSize == 1)
    	cmdPacketSizeStr = "CMD PKT LEN=16";
    else 
    	cmdPacketSizeStr = "CMD PKT LEN=UNKNOWN";
	

    IOLog("%s: Drive %d: %s %s (%s, %s, %s)\n", [self name], unit,
    		protocolTypeStr, 
    		deviceTypeStr, cmdDrqTypeStr,
    		removableStr, cmdPacketSizeStr);

#ifdef DEBUG  
    if (ideIdentifyInfo->capabilities & IDE_CAP_LBA_SUPPORTED)	{
	IOLog("%s: LBA supported.\n", [self name]); 
    }
    if (ideIdentifyInfo->capabilities & IDE_CAP_IORDY_SUPPORTED)	{
	IOLog("%s: IORDY supported.\n", [self name]); 
    }

    if (ideIdentifyInfo->bufferType != 0)	{
	IOLog("%s: buffer type %d, %d sectors.\n", [self name],
		ideIdentifyInfo->bufferType, ideIdentifyInfo->bufferSize);
    }
 
    IOLog("%s: PIO timing cycle: %d ns.\n", [self name],
	    ideIdentifyInfo->pioDataTransferCyleTimingMode &
	    IDE_PIO_TIMING_MODE_MASK);

    if (ideIdentifyInfo->capabilities & IDE_CAP_DMA_SUPPORTED)	{
        IOLog("%s: DMA timing cycle: %d ns.\n", [self name],
		ideIdentifyInfo->dmaDataTransferCyleTimingMode &
		IDE_DMA_TIMING_MODE_MASK);
    }

#endif DEBUG
}

/*
 * There are lots of ATAPI CD-ROMs that shadow the task file register and
 * hence show up as two devices. They also might issue a valid interrupt when
 * ATAPI Identify Device command is sent so we need to really make sure
 * whether the device has data for us. 
 */
- (atapi_return_t) _atapiIdentifyDevice:(struct vm_map *)client
			    	addr:(caddr_t)xferAddr
{
    unsigned int cmd = ATAPI_IDENTIFY_DRIVE;
    unsigned char dh = ADDRESS_MODE_LBA;	/* ALWAYS */
    unsigned char status;
    atapi_return_t rtn;
    
    unsigned int oldTimeout;

#ifdef DEBUG    
    IOLog("ATAPI Identify Device\n");
#endif DEBUG

	rtn = [self atapiWaitStatusBitsFor:ATAPI_MAX_WAIT_FOR_NOTBUSY
			on:0 off:0 alt:NO];
    if (rtn != IDER_SUCCESS)
		return (rtn);
    
    dh |= _driveNum ? SEL_DRIVE1 : SEL_DRIVE0;
    outb(_ideRegsAddrs.driveSelect, dh);
    
    bzero(xferAddr, IDE_SECTOR_SIZE);
    
    [self enableInterrupts];
	[self clearInterrupts];
	
    outb(_ideRegsAddrs.command, ATAPI_IDENTIFY_DRIVE);

	/* Wait for BSY bit to clear before reading from AltStatus.
	 * We do this to avoid a long interrupt timeout when probing
	 * "phantom" CD-ROM drives shadowed by another device on the same
	 * IDE connector.
	/* Read from AltStatus instead of Status register to avoid
	 * ack'ing any pending interrupts.
	 */
	rtn = [self atapiWaitStatusBitsFor:ATAPI_MAX_WAIT_FOR_NOTBUSY
			on:0 off:0 alt:YES];
    if (rtn != IDER_SUCCESS) {
		IOLog("%s: Polling BSY bit timed-out\n", [self name]);
		return (rtn);
    }

	/* Shadow (false) devices should set the ABORT bit in the ATA
	 * Error register. We check for ABORT bit and return an error
	 * if that bit is set.
	 */
	if ((inb(_ideRegsAddrs.altStatus) & ERROR) &&
		(inb(_ideRegsAddrs.error) & CMD_ABORTED)) {
		// Don't bother waiting for an interrupt, abort immediately.
#ifdef DEBUG
		IOLog("%s: Phantom ATAPI drive detected. Status:0x%02x\n",
			[self name], inb(_ideRegsAddrs.altStatus));
#endif DEBUG
		return IDER_ERROR;
	}

    oldTimeout = [self interruptTimeOut];
    [self setInterruptTimeOut:4000];		// four seconds
    rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
	[self setInterruptTimeOut:oldTimeout];

    if (rtn != IDER_SUCCESS) {
		[self getIdeRegisters:NULL Print:"Atapi Identify"];
		return (rtn);
    }
		
	// Make sure DRQ is set
	rtn = [self atapiWaitStatusBitsFor:ATAPI_MAX_WAIT_FOR_NOTBUSY
			on:DREQUEST off:0 alt:NO];
    if (rtn != IDER_SUCCESS)
		return (rtn);
	
    /*
     * FIXME: We need to do some sanity check on this data to be really sure
     * that there is an ATAPI device out here. 
     */
    [self xferData:xferAddr read:YES client:client length:IDE_SECTOR_SIZE];
    return IDER_SUCCESS;
}

/*
 * Some CD-ROMS (like SONY CDU-55D) fail the first request so we reset the
 * device and retry. 
 */
#define ATAPI_IDENTIFY_DEVICE_RETRIES		3

- (atapi_return_t) atapiIdentifyDevice:(struct vm_map *)client
			    	addr:(caddr_t)xferAddr
				unit:(unsigned char)unit
{
    int i;
    atapi_return_t ret;
    
    for (i = 0; i < ATAPI_IDENTIFY_DEVICE_RETRIES; i++)	{
    
		ret = [self _atapiIdentifyDevice:client addr:xferAddr];
		if (ret == IDER_SUCCESS)
	    	break;
		
		/* Reset hardware and try again */
		[self atapiSoftReset:unit];
		IOSleep(500);
		if (_ide_debug)	{
	    	IOLog("%s: Drive %d: ATAPI Identify Device failed "
			"(error %d). Retrying..\n", [self name], unit, ret);
		}
    }
#ifdef DEBUG
    if (i == ATAPI_IDENTIFY_DEVICE_RETRIES)	{
		IOLog("%s: FATAL: Drive %d: ATAPI Identify Device.\n", 
		[self name], unit);
    }
#endif DEBUG
    return ret;
}

- (void) atapiInitParameters:(ideIdentifyInfo_t *)infoPtr 
		Device:(unsigned char)unit
{
    atapiGenConfig_t *atapiGenConfig = 
    	(atapiGenConfig_t *) &infoPtr->genConfig;
    
    if (atapiGenConfig->cmdPacketSize == 0x01)
		_drives[unit].atapiCmdLen = 16;
    else
    	_drives[unit].atapiCmdLen = 12;

    /*
     * The command len is 12. However some devices think it is 16 which is
     * actually reserved for SAM compatibility. See page 58, SFF 8020, rev
     * 1.2. This is a workaround.
     */
    if (_drives[unit].atapiCmdLen != 12)	{
	IOLog("%s: ATAPI: command len changed to 12 from %d.\n", 
		[self name], _drives[unit].atapiCmdLen);
	_drives[unit].atapiCmdLen = 12;
    }

    _drives[unit].atapiCmdDrqType = atapiGenConfig->cmdDrqType;
}

/*
 * Identify ATAPI device must have been executed first before callling this
 * method. 
 */
- (unsigned char) atapiCommandPacketSize:(unsigned char)unit
{
    if (unit < MAX_IDE_DRIVES)
	return _drives[unit].atapiCmdLen;
    else
    	return 0;
}


/*
 * The amount of time in milliseconds it takes for an ATAPI device to post
 * its signature after a reset. Some CD-ROMs take a long time to respond. Do
 * not decrease it without testing with various makes of ATAPI devices. 
 */
#define ATAPI_RESET_DELAY		2500

- (atapi_return_t) atapiSoftReset:(unsigned char)unit
{
    unsigned char dh = ADDRESS_MODE_LBA;	/* ALWAYS */

#ifdef DEBUG
    IOLog("%s: atapiSoftReset:device %d\n", [self name], unit);
#endif DEBUG

    dh |= unit ? SEL_DRIVE1 : SEL_DRIVE0;

    outb(_ideRegsAddrs.driveSelect, dh);
    outb(_ideRegsAddrs.command, ATAPI_SOFT_RESET);
    
    IOSleep(50);	/* Enough time to assert busy */

	if ([self atapiWaitStatusBitsFor:ATAPI_RESET_DELAY on:0 off:0 alt:NO] ==
		IDER_SUCCESS)
		return IDER_SUCCESS;

#ifdef DEBUG
    IOLog("%s: atapiSoftReset: FAILED. Status = %x\n", [self name], status);
#endif DEBUG

    return IDER_ERROR;
}

/*
 * Method: issuePacketCommand
 *
 * This method will issue the Packet Command code (0xA0) to the command
 * register. Then wait for an interrupt (INT DRQ devices only) and poll
 * the Ireason/Status for:
 *
 *  BSY = 0
 *  CoD = 1
 *  IO  = 0
 *
 * We assume that the task file has been initialized prior to calling
 * this method.
 *
 * Returns:
 *  IDER_SUCCESS - DRQ is set and ready to send the 12-byte command.
 *  IDER_ERROR   - Abort and send a ATAPI soft reset.
 */
#define MAX_DRQ_WAIT 		(100 * 1000 * 5)	// 5 second wait

- (atapi_return_t) issuePacketCommand
{
    int i;
    unsigned char interruptReason;
    unsigned char status;
	
#ifdef DEBUG    
    IOLog("%s: issuePacketCommand\n", [self name]);
#endif DEBUG

	/*
	 * Send the packet command.
	 */
    outb(_ideRegsAddrs.command, ATAPI_PACKET);

    /*
     * Some devices might have interrupted so we should take care of that
     * first.
     */
    if (_drives[_driveNum].atapiCmdDrqType == ATAPI_CMD_DRQ_INT) {
	    atapi_return_t rtn;
		rtn = [self ideWaitForInterrupt:ATAPI_PACKET ideStatus:&status];
		if (rtn != IDER_SUCCESS)
			return rtn;
    }
    
    /*
     * Wait till we get okay from the device. 
     */
	IODelay(1);
    for (i = 0; i < MAX_DRQ_WAIT; i++) {
		interruptReason = inb(_ideRegsAddrs.interruptReason);
		status = inb(_ideRegsAddrs.status);
		
		if (!(status & BUSY) &&
			(interruptReason & CMD_OR_DATA) &&
			!(interruptReason & IO_DIRECTION))
			break;
		IODelay(10);
    }

    if (i == MAX_DRQ_WAIT)	{
		IOLog("%s: ATAPI Drive %d:  Invalid Interrupt Reason: %x.\n", 
			[self name], _driveNum, interruptReason);
		return IDER_ERROR;
    }

    /*
     * Now DRQ should be set. 
     */
    status = inb(_ideRegsAddrs.status);
    if (status & DREQUEST) {
		return IDER_SUCCESS;
    }
    
    IOLog("%s: ATAPI Drive %d: DRQ not set: %x\n", 
    		[self name], _driveNum, status);
    return IDER_ERROR;
}

/*
 * Method: sendAtapiCommand:cmdLen:
 *
 * Send the ATAPI command packet bytes.
 * Note that the command is sent out as words in little endian format. 
 */
- (void) sendAtapiCommand:(unsigned char *)atapiCmd
	cmdLen:(unsigned char)len
{
    int i;
    
    for (i = 0; i < len/2; i++)	{
    	outw(_ideRegsAddrs.data, atapiCmd[2*i+1] << 8 | atapiCmd[2*i]);
    }
}

#define MAX_SENDPACKET_RETRIES		3
#define MAX_BUSY_WAIT 				(1000*100)

/*
 * Transfer data between host and the ATAPI device through PIO.
 *
 * Before calling this method, the packet command and command data bytes
 * have been issued to the drive. This method will transfer data until
 * DRQ = 0, or if the maximum transfer count is reached, whichever
 * occurs first. This method will require an interrupt for every iteration
 * of the loop.
 */
- (sc_status_t) atapiPIODataTransfer:(atapiIoReq_t *)atapiIoReq 
			buffer:(void *)buffer 
			client:(struct vm_map *)client
{
    int i;
    atapi_return_t rtn;
    unsigned char status;
    unsigned int offset;
    unsigned char cmd = atapiIoReq->atapiCmd[0];
    unsigned int bytes;
	
	/*
	 * Zero the counters.
	 */
    atapiIoReq->bytesTransferred = 0;
    offset = 0;
	
    for (;;) {
    
		/* At this point, the drive is executing the packet command.
			* For commands such as FORMAT_UNIT (0x04), the may take a very
			* long time. Probably much longer than IDE_INTR_TIMEOUT.
			*
			* To guard against those cases, we take the maximum of
			* IDE_INTR_TIMEOUT vs. scsiReq.timeout.
			*/
		if (atapiIoReq->timeout > IDE_INTR_TIMEOUT) {
			u_int current_timeout = [self interruptTimeOut];
			
			//IOLog("using SCSI timeout:%d\n", atapiIoReq->timeout);
			[self setInterruptTimeOut:atapiIoReq->timeout];
			rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
			[self setInterruptTimeOut:current_timeout];
		}
		else {
			rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
		}
	
		if (rtn != IDER_SUCCESS) {
			IOLog("%s: FATAL: ATAPI Drive: %d Command %x failed.\n", 
				[self name], _driveNum, atapiIoReq->atapiCmd[0]);
			[self getIdeRegisters:NULL Print:"ATAPI Command"];
			[self atapiSoftReset:_driveNum];
			atapiIoReq->scsiStatus = STAT_CHECK;
			return SR_IOST_CHKSNV;
		}

		/*
		 * This is stupid but the Chinon drive fires off an interrupt first
		 * and then updates the status register. It appears that any drive
		 * based on Western Digital chipset will do this. At any rate, this
		 * code is harmless and should be left here. 
		 */
		for (i = 0; i < MAX_BUSY_WAIT; i++)	{
			if (status & BUSY)
				IODelay(10);
			else
				break;
			status = inb(_ideRegsAddrs.status);	
		}
	
		/*
		 * If DRQ == 0 then host has terminated the command. 
		 */
		if (!(status & DREQUEST)) {
	    	/*
	     	 * Check for command completion status. 
	     	 */
			if (status & ERROR)	{
#ifdef DEBUG
				// Don't complain for TEST UNIT READY command
				if (cmd != 0x00) {
					[self dumpStatus: atapiIoReq];
				}
				IOLog("%s: ATAPI command %x failed. "
					"Error: %x Status: %x\n", [self name], 
					atapiIoReq->atapiCmd[0], 
					inb(_ideRegsAddrs.error), status);
#endif DEBUG
		    
				atapiIoReq->scsiStatus = STAT_CHECK;
				return SR_IOST_CHKSNV;
	    	} 
			else {
#ifdef DEBUG
	    		IOLog("%s: Comamnd %x completed. status %x\n", 
					[self name], atapiIoReq->atapiCmd[0], status);
#endif DEBUG	
				atapiIoReq->scsiStatus = STAT_GOOD;
				return SR_IOST_GOOD;
	    	}
		}

		/*
		 * Command is not completed. We need to transfer data as requested by
		 * the device. 
		 */
     	bytes = inb(_ideRegsAddrs.byteCountHigh) << 8 |
				inb(_ideRegsAddrs.byteCountLow);

#ifdef DEBUG
		IOLog("%s: ATAPI Drive %d: completed: %x, request: %x max: %x\n", 
	    	[self name], _driveNum, atapiIoReq->bytesTransferred, 
	    	bytes, atapiIoReq->maxTransfer);
#endif DEBUG	

		/*
		 * If the device requests more data to be transferred than required
		 * by the command protocol, then we should transfer null data. 
		 */

		if (atapiIoReq->bytesTransferred + bytes > atapiIoReq->maxTransfer) {

	    	IOLog("%s: ATAPI Drive %d: "
				"ERROR: Transfer limit (%x bytes) exceeded\n", 
	    		[self name], _driveNum, atapiIoReq->maxTransfer);
		
	    	IOLog("%s: Bytes already transfered: %x, next request: %x\n", 
	    		[self name], atapiIoReq->bytesTransferred, bytes);
	    
	    	[self dumpStatus: atapiIoReq];

		    /*
		     * This thing is probably hosed but let's do the best we can. 
	    	 */
			{
	    	int diff = atapiIoReq->maxTransfer-atapiIoReq->bytesTransferred;
	    	IOLog("%s: Transferring %x bytes instead of %x\n", 
	    		[self name], diff, bytes);
	    	[self xferData:buffer+offset read:atapiIoReq->read 
		    	client:client length:diff];
	    	atapiIoReq->bytesTransferred += diff;
	    	offset += diff;
	    	[self atapiSoftReset:_driveNum];	// reset hardware as well
	    	}

	    	atapiIoReq->scsiStatus = STAT_GOOD;
	    	return SR_IOST_GOOD;
#if 0
			/* this was old behavior */
			atapiIoReq->scsiStatus = STAT_CHECK;
			return SR_IOST_CMDREJ;
#endif 0
		}
	
		/*
		 * Now transfer data between device and memory.
		 */
		[self xferData:buffer+offset read:atapiIoReq->read client:client 
			length:bytes];
	
		atapiIoReq->bytesTransferred += bytes;
		offset += bytes;
	}

    /*
     * Will never get here. 
     */
    atapiIoReq->scsiStatus = STAT_GOOD;
    return SR_IOST_GOOD;
}

/*
 * Prints command and result in case of error. 
 */
- (void) dumpStatus:(atapiIoReq_t *)atapiIoReq
{
    int i;
    
    IOLog("%s: Failed command: drive: %d lun: %d len: %d read: %d\n", 
	[self name], 
	atapiIoReq->drive, atapiIoReq->lun, atapiIoReq->cmdLen, 
	atapiIoReq->read);
    IOLog("%s: Failed command: ", [self name]); 
    for (i = 0; i < atapiIoReq->cmdLen; i++)
	IOLog("%x ", atapiIoReq->atapiCmd[i]);
    IOLog("\n");
    
    [self getIdeRegisters:NULL Print:"dumpStatus"];
}

/*
 * Execute the ATAPI command specified in 'atapiIoReq'.
 */
- (sc_status_t) atapiExecuteCmd:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client
{
    int i;
    atapi_return_t rtn;
    unsigned char dh = ADDRESS_MODE_LBA;	/* ALWAYS */
	sc_status_t sc_ret;
	unsigned char cmd = atapiIoReq->atapiCmd[0];
	BOOL useDMA = NO;
    
    /*
     * Reject command if there are no ATAPI devices detected. 
     */
	if ((atapiIoReq->drive >= MAX_IDE_DRIVES) ||
		(atapiIoReq->lun != 0) ||
		([self isAtapiDevice:atapiIoReq->drive] == NO)) {
		return SR_IOST_SELTO;
	}

    /* Now execute the SCSI command. */

#ifdef DEBUG
    /* Print out the command received. */
    IOLog("%s: atapiExecuteCmd: drive: %d lun: %d len: %d read %d\n", 
    	[self name], atapiIoReq->drive, atapiIoReq->lun, atapiIoReq->cmdLen, 
	atapiIoReq->read);
    IOLog("%s: Command: ", [self name]); 
    if (atapiIoReq->atapiCmd[4] == 0x41)
	atapiIoReq->atapiCmd[4] = 0x24;
    for (i = 0; i < atapiIoReq->cmdLen; i++)
	IOLog("%x ", atapiIoReq->atapiCmd[i]);
    IOLog("\n");
    //IOBreakToDebugger();
#endif DEBUG

	_driveNum = atapiIoReq->drive;

	/*
	 * We are very particular on the type of commands that we allow
	 * DMA to be used.
	 *
	 * 1. It must be a Read/Write data command.
	 * 2. Buffer must be 4-byte aligned.
	 # 3. The drive must be DMA capable.
	 */
	useDMA = ((_drives[_driveNum].transferType != IDE_TRANSFER_PIO) &&
			 (((vm_offset_t)buffer & (PIIX_BUF_ALIGN - 1)) == 0) &&
			 ((cmd == 0x28) || (cmd == 0xa8) ||		// read 10 and read 12
              (cmd == 0x2a) || (cmd == 0xaa) ||		// write 10 and write 12
			  (cmd == 0x2f) || (cmd == 0x2e)));		// write and verify

    for (i = 0; i < MAX_SENDPACKET_RETRIES; i++) {

		/*
		 * Select the drive. Make sure the BSY bit is off before
		 * changing the drv bit.
		 */
		[self atapiWaitStatusBitsFor:ATAPI_MAX_WAIT_FOR_NOTBUSY
			on:0 off:0 alt:NO];
		dh |= _driveNum ? SEL_DRIVE1 : SEL_DRIVE0;
		outb(_ideRegsAddrs.driveSelect, dh);
		
		/*
		 * Wait for BSY = 0, DRQ = 0.
		 */
		rtn = [self atapiWaitStatusBitsFor:ATAPI_MAX_WAIT_FOR_NOTBUSY
				on:0 off:DREQUEST alt:NO];
		if (rtn != IDER_SUCCESS) {
			IOLog("%s: ATAPI Drive %d: Not Ready For Packet command.\n", 
				[self name], _driveNum);
			[self atapiSoftReset:_driveNum];
			continue;
		}
	
		/*
		 * Send our preferred data transfer size (2048 bytes). 
		 */
		outb(_ideRegsAddrs.byteCountLow, 0x00);
		outb(_ideRegsAddrs.byteCountHigh, 0x08);
	
		/*
		 * We will use PIO for data transfers. 
		 */
		if (useDMA == YES)
			outb(_ideRegsAddrs.features, 1);
		else
			outb(_ideRegsAddrs.features, 0);
		
		[self enableInterrupts];
		[self clearInterrupts];
	
		/*
		 * First tell the ATAPI device that we are going to send it a packet
		 * command. If this command fails the ATAPI device needs to be reset. 
		 */
		if ([self issuePacketCommand] == IDER_SUCCESS) {
			break;
		}
		
		IOLog("%s: ATAPI Drive %d: Packet command failed. Retrying...\n",
				[self name], _driveNum);
		[self atapiSoftReset:_driveNum];
    }

    /*
     * Are we hosed? 
     */
    if (i == MAX_SENDPACKET_RETRIES) {
		atapiIoReq->scsiStatus = STAT_CHECK;
		IOLog("%s: ATAPI Drive %d: FATAL: Packet command.\n",
			[self name], _driveNum);
		return SR_IOST_CMDREJ;
    }

//	[self clearInterrupts];

    /*
     * Send the ATAPI command packet bytes (usually 12-bytes) to the device.
     */
    [self sendAtapiCommand:atapiIoReq->atapiCmd cmdLen:atapiIoReq->cmdLen];

	/*
	 * Peform data transfer (if any) and return the result code.
	 */
	if (useDMA == YES)
		sc_ret = [self performATAPIDMA:atapiIoReq buffer:buffer client:client];
	else
		sc_ret = [self atapiPIODataTransfer:atapiIoReq buffer:buffer 
				 client:client];
	
	return sc_ret;
}

#if 0
#define ATAPI_SET_CDROM_SPEED	0xbb

/*
 * Doesn't seem to make any difference. 
 */
- (void)setMaxSpeedForATAPICDROM:(unsigned char)unit
{
    atapiIoReq_t atapiIoReq;
    
    bzero(&atapiIoReq, sizeof(atapiIoReq_t));
    
    atapiIoReq.cmdLen = [self atapiCommandPacketSize:unit];
    atapiIoReq.drive = unit;
    
    atapiIoReq.atapiCmd[0] = ATAPI_SET_CDROM_SPEED;
    atapiIoReq.atapiCmd[2] = 0xff;
    atapiIoReq.atapiCmd[3] = 0xff;
    
    (void) [self atapiExecuteCmd:&atapiIoReq 
    		buffer:NULL client:(struct vm_map *)IOVmTaskSelf()];
}
#endif 0

@end
