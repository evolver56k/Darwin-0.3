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
 * AtapiCntCmds.m - ATAPI command implementation for ATA interface. 
 *
 * HISTORY 
 *
 */

#import "AtapiCntCmds.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdeCntDma.h"

@implementation IdeController(ATAPI)

#define ATAPI_MAX_WAIT_FOR_NOTBUSY		(5 * 1000)

/*
 * We could use the ATA version instead of rolling our own here. 
 */
- (atapi_return_t)atapiWaitForNotBusy
{
    int i;
    unsigned char status;

    for (i = 0; i < ATAPI_MAX_WAIT_FOR_NOTBUSY; i++)  
    {
	status = inb(_ideRegsAddrs.status);
	
	if (!(status & BUSY)) 
        {
	    return IDER_SUCCESS;
	}
	IOSleep(1);
    }
    return IDER_TIMEOUT;
}

/*
 * Before sending a packet to ATAPI device, we test if BSY == 0 and DRQ == 0. 
 */
#define ATAPI_MAX_WAIT_READY_FOR_PACKET		(5 * 1000)

- (atapi_return_t)atapiWaitReadyForPacket
{
    int i;
    unsigned char status;

    for (i = 0; i < ATAPI_MAX_WAIT_READY_FOR_PACKET; i++)  
    {
	status = inb(_ideRegsAddrs.status);
	
	if (!(status & BUSY) && !(status & DREQUEST)) 
        {
	    return IDER_SUCCESS;
	}
	IOSleep(1);	
    }
    
    return IDER_DEV_NOT_READY;
}

-(void)printAtapiInfo:(ideIdentifyInfo_t *)ideIdentifyInfo 
			Device:(unsigned char)unit
{
    int 		i;
    char 		name[50];
    char 		firmware[9];
    atapiGenConfig_t 	*atapiGenConfig;
    char 		*protocolTypeStr, *deviceTypeStr, *cmdDrqTypeStr;
    char 		*removableStr, *cmdPacketSizeStr;

    /*
     * Print drive name with firmware revision number after doing byte swaps. 
     */
    for (i = 0; i < 20; i++)	
    {
	name[2*i] = ideIdentifyInfo->modelNumber[2*i+1];
	name[2*i+1] = ideIdentifyInfo->modelNumber[2*i];
    }
    name[40] = '\0';
    
    for (i = 0; i < 4; i++)	
    {
	firmware[2*i] = ideIdentifyInfo->firmwareRevision[2*i+1];
	firmware[2*i+1] = ideIdentifyInfo->firmwareRevision[2*i];
    }
    firmware[8] = '\0';
    
    for (i = 38; i >= 0; i--)	{
	if (name[i] != ' ')
	    break;
    }
    strcpy(name+i+2, firmware);
    
    /*
     * This information should be printed since it is not duplicated
     * elsewhere. 
     */
    atapiGenConfig = (atapiGenConfig_t *) &ideIdentifyInfo->genConfig;
    
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
}

/*
 * This is a private version for ATAPI Identify command below. It has a short
 * delay time. 
 */
- (ide_return_t)atapiIdentifyDeviceWaitForDataReady
{
    int     delay = (100 * 1000);
    unsigned char status;

    delay -= 2;
    while (delay > 0) 
    {
	status = inb(_ideRegsAddrs.altStatus);
	if ((!(status & BUSY)) && (status & DREQUEST))
	    return (IDER_SUCCESS);
	if (delay % 1000) 	
        {
	    IODelay(2);
            delay -= 2;
	} 
        else 
        {
	    IOSleep(1);
	    delay -= 1000;
	}
    }
    
    return IDER_DEV_NOT_READY;
}

/*
 * There are lots of ATAPI CD-ROMs that shadow the task file register and
 * hence show up as two devices. They also might issue a valid interrupt when
 * ATAPI Identify Device command is sent so we need to really make sure
 * whether the device has data for us. 
 */
-(atapi_return_t)_atapiIdentifyDevice:(struct vm_map *)client
			    	addr:(caddr_t)xferAddr
{
    unsigned int cmd = ATAPI_IDENTIFY_DRIVE;
    unsigned char dh = ADDRESS_MODE_LBA;	/* ALWAYS */
    unsigned char status;
    atapi_return_t rtn;
    
    unsigned int oldTimeout;

    rtn = [self atapiWaitForNotBusy];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    [self setTransferRate: _driveNum UseDMA:NO];

    dh |= _driveNum ? SEL_DRIVE1 : SEL_DRIVE0;
    outb(_ideRegsAddrs.driveSelect, dh);
    
    bzero(xferAddr, IDE_SECTOR_SIZE);
    
    [self enableInterrupts];
    
    outb(_ideRegsAddrs.command, ATAPI_IDENTIFY_DRIVE);

    oldTimeout = [self interruptTimeOut];
    [self setInterruptTimeOut:4000];		// four seconds
    
    rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
        
    if (rtn != IDER_SUCCESS) {
	[self getIdeRegisters:NULL Print:"Atapi Identify"];
	[self setInterruptTimeOut:oldTimeout];
	return rtn;
    }

    /*
     * Let's see if it really has data for us. 
     */
    rtn = [self atapiIdentifyDeviceWaitForDataReady];
    if (rtn != IDER_SUCCESS) 
    {
	[self setInterruptTimeOut:oldTimeout];
	return (rtn);
    }

    /*
     * FIXME: We need to do some sanity check on this data to be really sure
     * that there is an ATAPI device out here. 
     */
    [self xferData:xferAddr read:YES client:client length:IDE_SECTOR_SIZE];
    
    [self setInterruptTimeOut:oldTimeout];
    
    return IDER_SUCCESS;
}

/*
 * Some CD-ROMS (like SONY CDU-55D) fail the first request so we reset the
 * device and retry. 
 */
#define ATAPI_IDENTIFY_DEVICE_RETRIES		5

-(atapi_return_t)atapiIdentifyDevice:(struct vm_map *)client
			    	addr:(caddr_t)xferAddr
				unit:(unsigned char)unit
{
    int i;
    atapi_return_t ret;
    
    for (i = 0; i < ATAPI_IDENTIFY_DEVICE_RETRIES; i++)	
    {
	ret = [self _atapiIdentifyDevice:client addr:xferAddr];
	if (ret == IDER_SUCCESS)
	    break;
	    
	/* Reset hardware and try again */
	[self atapiSoftReset:unit];
	IOSleep(500);
    }
    
    if (i == ATAPI_IDENTIFY_DEVICE_RETRIES)	{
	IOLog("%s: FATAL: Device %d: ATAPI Identify Device.\n", 
		[self name], unit);
    }

    return ret;
}

-(void)atapiInitParameters:(ideIdentifyInfo_t *)infoPtr 
		Device:(unsigned char)unit
{
    atapiGenConfig_t *atapiGenConfig = 
    	(atapiGenConfig_t *) &infoPtr->genConfig;
    
    if (atapiGenConfig->cmdPacketSize == 0x01)
	_atapiCmdLen[unit] = 16;
    else
    	_atapiCmdLen[unit] = 12;

    /*
     * The command len is 12. However some devices think it is 16 which is
     * actually reserved for SAM compatibility. See page 58, SFF 8020, rev
     * 1.2. This is a workaround.
     */
    if (_atapiCmdLen[unit] != 12)	
    {
	IOLog("%s: ATAPI: command len changed to 12 from %d.\n", 
		[self name], _atapiCmdLen[unit]);
	_atapiCmdLen[unit] = 12;
    }

    _atapiCmdDrqType[unit] = atapiGenConfig->cmdDrqType;

    if (infoPtr->capabilities & IDE_CAP_DMA_SUPPORTED) 
    {
        if ( [self allocDmaMemory] == IDER_SUCCESS )
        {
            _dmaSupported[unit] = YES;
        }
    }

    [self getTransferModes:infoPtr Unit:unit];

    /*
     * Set the Disk/CDROM transfer mode (Set Features SC=3) 
     */
    [self setTransferMode: unit]; 
}

/*
 * Identify ATAPI device must have been executed first before callling this
 * method. 
 */
-(unsigned char)atapiCommandPacketSize:(unsigned char)unit
{
    if (unit < MAX_IDE_DRIVES)
	return _atapiCmdLen[unit];
    else
    	return 0;
}


/*
 * The amount of time in milliseconds it takes for an ATAPI device to post
 * its signature after a reset. Some CD-ROMs take a long time to respond. Do
 * not decrease it without testing with various makes of ATAPI devices. 
 */
#define ATAPI_RESET_DELAY		2500

-(atapi_return_t)atapiSoftReset:(unsigned char)unit
{
    int  delay;
    unsigned char status;
    atapi_return_t rtn;
    unsigned char dh = ADDRESS_MODE_LBA;	/* ALWAYS */

    _driveReset[unit] = YES;

    dh |= unit ? SEL_DRIVE1 : SEL_DRIVE0;

    outb(_ideRegsAddrs.driveSelect, dh);
    outb(_ideRegsAddrs.command, ATAPI_SOFT_RESET);

    delay = ATAPI_RESET_DELAY;
    
    IOSleep(50);			/* Enough time to assert busy */
    
    rtn = IDER_ERROR;
    
    while (delay > 0) 
    {
	status = inb(_ideRegsAddrs.status);
	if (!(status & BUSY)) 
        {
	    rtn = IDER_SUCCESS;
            break;
	}
	
	IOSleep(10);
	delay -= 10;
    }

    return rtn;
}


#define MAX_DRQ_WAIT 		(1000 * 2)		// milliseconds

-(atapi_return_t)atapiPacketCommand
{
    int i;
    atapi_return_t rtn;
    unsigned char interruptReason;
    unsigned char status;
	
    outb(_ideRegsAddrs.command, ATAPI_PACKET);

    /*
     * Some devices might have interrupted so we should take care of that
     * first.
     */
    if (_atapiCmdDrqType[_driveNum] == ATAPI_CMD_DRQ_INT) 
    {
	rtn = [self ideWaitForInterrupt:ATAPI_PACKET ideStatus:&status];
    }
    
    /*
     * Wait till we get okay from the device. 
     */
    for (i = 0; i < MAX_DRQ_WAIT; i++)	
    {
	interruptReason = inb(_ideRegsAddrs.interruptReason);
	if ((interruptReason & CMD_OR_DATA) && !(interruptReason & IO_DIRECTION))
	    break;
	IOSleep(1);
    }

    if (i == MAX_DRQ_WAIT)	
    {
	IOLog("%s: ATAPI Device %d:  Invalid Interrupt Reason: %x.\n", 
		[self name], _driveNum, interruptReason);
	return IDER_ERROR;
    }

    /*
     * Now DRQ should be set. 
     */
    status = inb(_ideRegsAddrs.status);
    if (status & DREQUEST)	
    {
	return IDER_SUCCESS;
    }
    
    IOLog("%s: ATAPI Device %d: No Data Request: %x.\n", 
    		[self name], _driveNum, status);
	
    return IDER_ERROR;
}

/*
 * Note that the command is sent out as words in little endian format. 
 */
- (void) sendAtapiCommand:(unsigned char *)atapiCmd
	cmdLen:(unsigned char)len
{
    int i;
    
    for (i = 0; i < len/2; i++)	
    {
    	outw(_ideRegsAddrs.data, (atapiCmd[2*i] << 8) | atapiCmd[2*i+1]);
    }
}

/*
 * Prints command and result in case of error. 
 */
- (void)dumpStatus:(atapiIoReq_t *)atapiIoReq
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


#define	C10OP_MODESELECT	0x55	/* OPT: set device parameters */
#define	C10OP_MODESENSE		0x5a	/* OPT: get device parameters */

#define MAX_SENDPACKET_RETRIES		3

#define MAX_BUSY_WAIT 			(1000*100)

- (sc_status_t) atapiExecuteCmd:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client
{
    int 		i;
    atapi_return_t 	rtn;
    unsigned int 	offset;
    unsigned char 	dh = ADDRESS_MODE_LBA;	/* ALWAYS */
    unsigned char       xferType;
    BOOL		dmaAllowed;
    
    /*
     * Reject command if there are no ATAPI devices detected. 
     */
     if ((atapiIoReq->drive >= MAX_IDE_DRIVES) || (atapiIoReq->lun != 0))  
     {
	return SR_IOST_SELTO;
     }
     
     if ([self isAtapiDevice:atapiIoReq->drive] == NO)	
     {
	return SR_IOST_SELTO;
     }
         
    [self clearInterrupts];
    
    _driveNum = atapiIoReq->drive;

    dmaAllowed = [self atapiDmaAllowed: buffer];

    if ( atapiIoReq->maxTransfer != 0 )
    {
        [self setTransferRate: _driveNum UseDMA:dmaAllowed];
    }

    if ( _driveReset[_driveNum] == YES )
    {
        [self setTransferMode: _driveNum];
        _driveReset[_driveNum] = NO;
    }

    /* Now execute the SCSI command. */

    for (i = 0; i < MAX_SENDPACKET_RETRIES; i++)	
    {
	rtn = [self atapiWaitReadyForPacket];
	if (rtn != IDER_SUCCESS) 
        {
	    IOLog("%s: ATAPI Device %d: Not Ready For Packet command.\n", 
	    	[self name], _driveNum);
	    [self atapiSoftReset:_driveNum];
	    continue;
	}
	
	/*
	 * Select the drive first. 
	 */
	dh |= _driveNum ? SEL_DRIVE1 : SEL_DRIVE0;
	outb(_ideRegsAddrs.driveSelect, dh);

	/*
	 * Send our preferred data transfer size (2048 bytes). 
	 */
	outb(_ideRegsAddrs.byteCountLow, 0x00);
	outb(_ideRegsAddrs.byteCountHigh, 0x08);

	/*
	 * Set data transfer mode PIO = 0x00 / DMA = 0x01 
	 */
        xferType = (dmaAllowed == YES) ? 0x01 : 0x00;
	outb(_ideRegsAddrs.features, xferType);
	
	/*
	 * First tell the ATAPI device that we are going to send it a packet
	 * command. If this command fails the ATAPI device needs to be reset. 
	 */
	if ([self atapiPacketCommand] == IDER_SUCCESS)	
        {
	    break;
	}
	
	IOLog("%s: ATAPI Device %d: Packet command failed. Retrying...\n",
		 [self name], _driveNum);
	[self atapiSoftReset:_driveNum];
    }

    /*
     * Are we hosed? 
     */
    if (i == MAX_SENDPACKET_RETRIES)	
    {
	atapiIoReq->scsiStatus = STAT_CHECK;
	IOLog("%s: ATAPI Device %d: FATAL: Packet command.\n", 
		[self name], _driveNum);
	return SR_IOST_CMDREJ;    
    }
    
    /*
     * Send the actual command to the device. 
     */
    [self sendAtapiCommand:atapiIoReq->atapiCmd cmdLen:atapiIoReq->cmdLen];

    atapiIoReq->bytesTransferred = 0;
    offset = 0;
    
    [self enableInterrupts];

    if ( dmaAllowed == YES )
    {
        rtn = [self atapiXferDMA:(atapiIoReq_t *)atapiIoReq  buffer: (void *)buffer client: (struct vm_map *)client];
    }
    else
    {
        rtn =[self atapiXferPIO:(atapiIoReq_t *)atapiIoReq  buffer: (void *)buffer client: (struct vm_map *)client];
    }

#if 0
        kprintf("Disk(ATAPI): Cmd = %02x Length = %08x Addr=%08x:%08x Status = %d\n\r", 
               atapiIoReq->atapiCmd[0], (int)atapiIoReq->maxTransfer, (int)buffer, (int)client, 
               rtn);
#endif   

    return rtn;
}

- (sc_status_t) atapiXferPIO:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client
{
    int 		i;
    atapi_return_t 	rtn;
    unsigned char 	status;
    unsigned int 	bytes;
    unsigned int 	offset = 0;
    unsigned char 	cmd = atapiIoReq->atapiCmd[0];

    for (;;)	
    {
	rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];

	if (rtn != IDER_SUCCESS) 
        {
	    IOLog("%s: FATAL: ATAPI Device: %d Command %x failed.\n", 
	    	[self name], _driveNum, atapiIoReq->atapiCmd[0]);
	    [self getIdeRegisters:NULL Print:"ATAPI Command"];
	    [self atapiSoftReset:_driveNum];
	    atapiIoReq->scsiStatus = STAT_CHECK;
	    return SR_IOST_CHKSNV;
	}

	for (i = 0; i < MAX_BUSY_WAIT; i++)	
        {
	    if (status & BUSY)	
            {
		IODelay(10);
	    } 
            else 
            {
	    	break;
	    }
	    status = inb(_ideRegsAddrs.status);	
	}
	
	/*
	 * If DRQ == 0 then host has terminated the command. 
	 */
	if (!(status & DREQUEST))	
        {
	    /*
	     * Check for command completion status. 
	     */
	    if (status & ERROR)	
            {		    
		atapiIoReq->scsiStatus = STAT_CHECK;
		return SR_IOST_CHKSNV;
	    } 
            else 
            {
		atapiIoReq->scsiStatus = STAT_GOOD;
		return SR_IOST_GOOD;
	    }
	}

	/*
	 * Command is not completed. We need to transfer data as requested by
	 * the device. 
	 */
	bytes =  inb(_ideRegsAddrs.byteCountHigh) << 8 | 
	    inb(_ideRegsAddrs.byteCountLow);

	/*
	 * If the device requests more data to be transferred than required
	 * by the command protocol, then we should transfer null data. 
	 */

	
	if (atapiIoReq->bytesTransferred + bytes > atapiIoReq->maxTransfer)
        {
	    IOLog("%s: ATAPI Device %d: "
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
	}
	
	/*
	 * Now transfer data from device to memory. 
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


- (sc_status_t) atapiXferDMA:(atapiIoReq_t *)atapiIoReq 
		    buffer : (void *)buffer 
		    client : (struct vm_map *)client
{
    int 		i;
    atapi_return_t 	rtn;
    unsigned char 	status;
    unsigned char 	cmd = atapiIoReq->atapiCmd[0];
    unsigned long	cfgReg;

    [self setupDMA:(vm_offset_t) buffer client:(vm_task_t) client length:(int)atapiIoReq->maxTransfer fRead:(BOOL)atapiIoReq->read]; 
    if ( _controllerType != kControllerTypeCmd646X )
    {
        IODBDMAContinue( _ideDMARegs );        
    }
    else
    {
        [[self deviceDescription] configReadLong:0x70 value: &cfgReg];
        cfgReg &= ~0x08;
        cfgReg |= 0x01 | ((atapiIoReq->read) ? 0x08 : 0x00);
        [[self deviceDescription] configWriteLong:0x70 value: cfgReg];
    }

    rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];

    if (rtn != IDER_SUCCESS) 
    {
	IOLog("%s: FATAL: ATAPI Device: %d Command %x failed.\n", [self name], _driveNum, atapiIoReq->atapiCmd[0]);
	[self getIdeRegisters:NULL Print:"ATAPI Command"];
	[self atapiSoftReset:_driveNum];
	atapiIoReq->scsiStatus = STAT_CHECK;
	return SR_IOST_CHKSNV;
    }

    for (i = 0; i < MAX_BUSY_WAIT; i++)	
    {
	if (status & BUSY)	
        {
            IODelay(10);
	} 
        else 
        {
	    break;
	}
        status    = inb(_ideRegsAddrs.status);
    }

    if ( _controllerType != kControllerTypeCmd646X )
    {
        atapiIoReq->bytesTransferred = [self stopDBDMA];
    }
    else
    {
        [[self deviceDescription] configReadLong: 0x70 value: &cfgReg];
        cfgReg &= ~0x01;
        [[self deviceDescription] configWriteLong:0x70 value: cfgReg];

        atapiIoReq->bytesTransferred = atapiIoReq->maxTransfer;
    }

    /*
     * The DMA has handled all the data movement. 
     * Must have final command status at this point. 
     */
     if ( status & (ERROR) )	
     {		    
         atapiIoReq->scsiStatus = STAT_CHECK;
         return SR_IOST_CHKSNV;
     } 
     else 
     {
	  atapiIoReq->scsiStatus = STAT_GOOD;
	  return SR_IOST_GOOD;
     }
}     
  
- (BOOL) atapiDmaAllowed: (void *)buffer
{
    if ( _dmaSupported[_driveNum] == NO ) 
    {
        return NO;
    }
    
    if ( (_controllerType == kControllerTypeCmd646X) && ((u_int)buffer & 1) )
    {
        return NO;
    }

    return YES;
}
        
      
@end
