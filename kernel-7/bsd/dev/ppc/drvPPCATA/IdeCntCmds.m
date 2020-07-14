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
 * IdeCntCmds.m - Implementation of Commands category for IDE Controller 
 * device. 
 *
 * It contains implementation of the ATA command set. 
 *
 *
 * HISTORY 
 *
 * 1-Aug-1995	 	Rakesh Dubey at NeXT 
 *	Reduced the timeout in IDE_IDENTIFY_DRIVE method. 
 *
 * 17-July-1994 	Rakesh Dubey at NeXT 
 *	Created. 
 */

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntDma.h"
#import <kern/assert.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "io_inline.h"
#import "IdeDDM.h"

/*
 * All methods in this file implement one or more of the IDE commands. These
 * commands get invoked by IdeDisk objects. 
 */

@implementation IdeController(Commands)

/*
 * Returns a struct containing IDE registers values for a given sector
 * address and size. Depending upon whether the drive supports LBA or CHS we
 * fill in different values. The calling routine must make sure that these
 * values are not overwritten. The only thing that they need to fill in is
 * the drive number. 
 */
- (ideRegsVal_t)logToPhys:(unsigned)block numOfBlocks:(unsigned)nblk
{
    ideRegsVal_t rv;
    unsigned int cyl, nheads;
    unsigned int track, spt;

    if (_addressMode[_driveNum] == ADDRESS_MODE_CHS)	
    {
	nheads = _ideInfo[_driveNum].heads;
	spt = _ideInfo[_driveNum].sectors_per_trk;
	
	track = (block / spt);
	cyl = track / nheads;
	rv.sectNum = block % spt + 1;
	
	rv.drHead = track % nheads;
	rv.cylLow = (cyl & 0xff);
	rv.cylHigh = ((cyl & 0xff00) >> 8);
    } 
    else 
    {
        rv.sectNum = block & 0x0ff;
	rv.cylLow = (block >> 8) & 0xff;
	rv.cylHigh = (block >> 16) & 0xff;
	rv.drHead = (block >> 24) & 0x0f;
    }
    
    rv.sectCnt = (unsigned char)nblk;

    rv.drHead |= _addressMode[_driveNum];
    rv.drHead |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
	    
    return rv;
}

/*
 * Commands to the IDE controller. 
 */


- (ide_return_t)ideDiagnose:(unsigned *)diagError
{
    ide_return_t rtn;
    unsigned char status;
    
    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return IDER_CMD_ERROR;
	
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_DIAGNOSE);
    
    rtn = [self ideWaitForInterrupt:IDE_DIAGNOSE ideStatus:&status];
    if (rtn != IDER_SUCCESS)	
    {
	[self getIdeRegisters:NULL Print:"Diagnose"];
	return IDER_CMD_ERROR;
    }

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return IDER_CMD_ERROR;
    
    *diagError = inb(_ideRegsAddrs.error);
    
    if (*diagError == 0x01)	
    {
        /* FIXME: do we need to soft reset ATAPI devices? */
	return IDER_SUCCESS;
    }

    /*
     * At least one of the drives is bad. 
     */
    if (*diagError & 0x080) 
    {
	IOLog("%s: Drive 1 has failed diagnostics.\n", [self name]);
	_ideInfo[1].type = 0;

	if ((*diagError & 0x07f) != 1) 
        {
	    IOLog("%s: Drive 0 has failed diagnostics, error %d\n",
		    [self name], (*diagError & 0x07f));
	    return IDER_CMD_ERROR;
	}
    } 
    else 
    {
	_ideInfo[0].type = 0;
	IOLog("%s: Drive 0 has failed diagnostics, error %d\n",
		[self name], (*diagError & 0x07f));
    }
    
    return IDER_CMD_ERROR;
}

- (ide_return_t)ideSetParams:(unsigned)sectCnt numHeads:(unsigned)nHeads
			ForDrive:(unsigned)drive
{
    unsigned char dh = _addressMode[drive];
    ide_return_t rtn;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return rtn;
	
    dh |= ((drive ? SEL_DRIVE1 : SEL_DRIVE0) | ((nHeads - 1) & 0x0f));
    outb(_ideRegsAddrs.drHead, dh);
    outb(_ideRegsAddrs.sectCnt, (sectCnt & 0xff));
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_SET_PARAMS);
    
    rtn = [self ideWaitForInterrupt:IDE_SET_PARAMS ideStatus:NULL];
    if (rtn != IDER_SUCCESS) 
    {
	[self getIdeRegisters:NULL Print:"SetParams"];
	return rtn;
    }
    
    return rtn;
}

- (ide_return_t)ideSetDriveFeature:(unsigned)feature value:(unsigned)val
{
    unsigned char dh = _addressMode[_driveNum];
    unsigned char status;
    ide_return_t rtn;
    
    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return rtn;
	
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    outb(_ideRegsAddrs.sectCnt, (val & 0xff));
    
    outb(_ideRegsAddrs.features, feature);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_SET_FEATURES);
    
    rtn = [self ideWaitForInterrupt:IDE_SET_FEATURES ideStatus:&status];
    if (rtn != IDER_SUCCESS) 
    {
	[self getIdeRegisters:NULL Print:"SetFeatures"];
	return rtn;
    }
    
    if (status & ERROR)
    	return IDER_ERROR;
    
    return rtn;
}

- (ide_return_t)ideRestore:(ideRegsVal_t *)ideRegs
{
    ide_return_t rtn;
    unsigned char status;
    unsigned char dh = _addressMode[_driveNum];

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return rtn;
    }
    
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_RESTORE);

    rtn = [self ideWaitForInterrupt:IDE_RESTORE ideStatus: &status];
    if (rtn != IDER_SUCCESS) 
    {
	[self getIdeRegisters:NULL Print:"Restore"];
	return rtn;
    }
    
    rtn = [self waitForDeviceReady];
    if (rtn == IDER_SUCCESS) 
    {
	if (status & ERROR) 
        {
	    [self getIdeRegisters:ideRegs Print:"Restore"];
	    rtn = IDER_CMD_ERROR;
	}
    }

    return (rtn);
}

- (ide_return_t)ideReadGetInfoCommon:(ideRegsVal_t *)ideRegs
			    client:(struct vm_map *)client
			    addr:(caddr_t)xferAddr
			    command:(unsigned int)cmd
{
    ide_return_t rtn;
    unsigned int sec_cnt = ideRegs->sectCnt;
    int i;
    unsigned char *taddr;
    unsigned char status;
    unsigned char dh = _addressMode[_driveNum];

    if (sec_cnt == 0)
	sec_cnt = MAX_BLOCKS_PER_XFER;
	
    taddr = xferAddr;

    /*
     * We have to define this for the IDE_IDENTIFY_DRIVE command. 
     */
    if (cmd == IDE_IDENTIFY_DRIVE)	
    {
	ideRegs->sectCnt = 1;
	sec_cnt = 1;
    }

    /*
     * Select the drive first. This routine is invoked by the initialization
     * code as well (-resetAndInit) so it is necessary to do this here. 
     */
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);

    
    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    if (cmd == IDE_READ) 
    {
	outb(_ideRegsAddrs.drHead, ideRegs->drHead);
	outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
	outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
	outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
	outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    } 
    else 
    {
        /* probably unnecessary */
	outb(_ideRegsAddrs.drHead, dh);
	outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    }
        
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, cmd);
        
    for (i = 0; i < sec_cnt; i++)	
    {
	rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
	if (rtn != IDER_SUCCESS) 
        {
	    return rtn;
	} 
        else 
        {
	    /*
	    if (status & ERROR_CORRECTED) {
		IOLog("%s: Corrected error during read.\n", [self name]);
	    }
	    */
	}
    
       /*
	* Same as waitForDataReady but with a quick timeout. 
	*/
	rtn = [self ataIdeReadGetInfoCommonWaitForDataReady];
	if (rtn != IDER_SUCCESS) 
        {
	    return (rtn);
	}
	
	[self xferData:taddr read:YES client:client length:IDE_SECTOR_SIZE];
	taddr += IDE_SECTOR_SIZE;
    }

    return rtn;
}

- (ide_return_t)ideWrite:(ideRegsVal_t *)ideRegs
		    client:(struct vm_map *)client
		    addr:(caddr_t)xferAddr
{
    ide_return_t rtn;
    unsigned int sec_cnt = ideRegs->sectCnt;
    int i;
    unsigned char *taddr;
    unsigned char status;

    if (sec_cnt == 0)
	sec_cnt = MAX_BLOCKS_PER_XFER;
    
    taddr = xferAddr;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_WRITE);

    for (i = 0; i < sec_cnt; i++)	
    {
	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) 
        {
	    return (rtn);
	}

	[self xferData:taddr read:NO client:client length:IDE_SECTOR_SIZE];
	taddr += IDE_SECTOR_SIZE;

	rtn = [self ideWaitForInterrupt:IDE_WRITE ideStatus:&status];
    
	if (rtn != IDER_SUCCESS) 
        {
	    [self getIdeRegisters:ideRegs Print:"Write"];
	    return (rtn);
	} 
        else 
        {
	    /*
	    if (status & ERROR_CORRECTED) {
		IOLog("%s: Corrected error during write.\n", [self name]);
	    }
	    */
	}
    }

    return (rtn);
}

- (ide_return_t)ideReadVerifySeekCommon:(ideRegsVal_t *)ideRegs
			    command:(unsigned int)cmd
{
    ide_return_t rtn;
    unsigned char status;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    if (cmd == IDE_READ_VERIFY)
	outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);

    [self enableInterrupts];
    outb(_ideRegsAddrs.command, cmd);
    
    rtn = [self ideWaitForInterrupt:cmd ideStatus: &status];
    if (rtn != IDER_SUCCESS) 
    {
	[self getIdeRegisters:NULL Print:"ReadVerify/Seek"];
	return (rtn);
    }
    
    if (status & (ERROR | WRITE_FAULT)) 
    {
	rtn = IDER_CMD_ERROR;
    } 
    else 
    {
	if ((cmd == IDE_SEEK) && (!(status & SEEK_COMPLETE)))
	    rtn = IDER_CMD_ERROR;
    }

    return (rtn);
}

- (ide_return_t)ideSetMultiSectorMode:(ideRegsVal_t *)ideRegs
			    numSectors:(unsigned char)nSectors
{
    ide_return_t rtn;
    unsigned char status;
    unsigned char dh = _addressMode[_driveNum];

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    outb(_ideRegsAddrs.sectCnt, nSectors);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_SET_MULTIPLE);

    rtn = [self ideWaitForInterrupt:IDE_SET_MULTIPLE ideStatus:&status];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    rtn = [self waitForDeviceReady];
    if (rtn == IDER_SUCCESS) 
    {
	if (status & ERROR) 
        {
	    rtn = IDER_CMD_ERROR;
	    [self getIdeRegisters:ideRegs Print:NULL];
	} 
        else
	    rtn = IDER_SUCCESS;
    } 

    return (rtn);
}

/*
 * Note: Never read the status register at the end of data transfer, you may
 * clobber the next interrupt from the drive. If it is necessary to get
 * status use the alternate status register. 
 */
- (ide_return_t)ideReadMultiple:(ideRegsVal_t *)ideRegs
	client:(struct vm_map *)client
	addr:(caddr_t)xferAddr
{
    ide_return_t rtn;
    unsigned char status;
    unsigned sec_cnt = ideRegs->sectCnt;
    unsigned int nSectors;
    unsigned char *taddr;
    unsigned int length;

    if (sec_cnt == 0)
	sec_cnt = MAX_BLOCKS_PER_XFER;
	
    taddr = xferAddr;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);

    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_READ_MULTIPLE);

    while (sec_cnt > 0) 
    {
	rtn = [self ideWaitForInterrupt:IDE_READ_MULTIPLE
			ideStatus: &status];

	if (rtn != IDER_SUCCESS) 
        {
	    [self getIdeRegisters:ideRegs Print:"Read Multiple"];
	    return (rtn);
	}

#if 1	
	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) 
        {
	    return (rtn);
	}
#endif 

	if (status & (ERROR | WRITE_FAULT)) 
        {
	    [self getIdeRegisters:ideRegs Print:"Read Multiple"];
	    return IDER_CMD_ERROR;
	}

	/*
	 * Any drive formatted with 63 sector/track (which most over 400 MB
	 * are) reporting this status (ERROR_CORRECTED) will cause an
	 * fallacious error (possibly uncorrectable) due to a long-standing
	 * bug in DOS. This status bit should be made vendor specific, like
	 * IDX. It has outlived its usefulness. 
	 * -- Gene_Milligan@notes.seagate.com 
	 */
	 
	/*
	if (status & ERROR_CORRECTED) 
        {
	    IOLog("%s: Corrected error during read.\n", [self name]);
	}
	*/

	/*
	 * All is well. Read in the data. 
	 */
	if (sec_cnt > _multiSector[_driveNum])
	    nSectors = _multiSector[_driveNum];
	else
	    nSectors = sec_cnt;
	    
	sec_cnt -= nSectors;

	while (nSectors) 
        {
	    if (nSectors > PAGE_SIZE / IDE_SECTOR_SIZE) 
            {
		length = PAGE_SIZE;
		nSectors -= (PAGE_SIZE / IDE_SECTOR_SIZE);
	    } 
            else 
            {
		length = nSectors * IDE_SECTOR_SIZE;
		nSectors = 0;
	    }

	    [self xferData:taddr read:YES client:client length:length];
	    taddr += length;
	}
    }

    return (rtn);
}

- (ide_return_t)ideWriteMultiple:(ideRegsVal_t *)ideRegs
	client:(struct vm_map *)client 
	addr:(caddr_t)xferAddr
{
    ide_return_t rtn;
    unsigned char status;
    unsigned sec_cnt = ideRegs->sectCnt;
    unsigned int nSectors;
    unsigned char *taddr;
    unsigned int length;

    if (sec_cnt == 0)
	sec_cnt = MAX_BLOCKS_PER_XFER;
	
    taddr = xferAddr;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_WRITE_MULTIPLE);

    while (sec_cnt > 0) 
    {
    
	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) 
        {
	    return (rtn);
	}
	
	if (sec_cnt > _multiSector[_driveNum])
	    nSectors = _multiSector[_driveNum];
	else
	    nSectors = sec_cnt;
	    
	sec_cnt -= nSectors;

	while (nSectors) 
        {
	    if (nSectors > PAGE_SIZE / IDE_SECTOR_SIZE) 
            {
		length = PAGE_SIZE;
		nSectors -= (PAGE_SIZE / IDE_SECTOR_SIZE);
	    } 
            else 
            {
		length = nSectors * IDE_SECTOR_SIZE;
		nSectors = 0;
	    }

	    [self xferData:taddr read:NO client:client length:length];
	    taddr += length;
	}
	rtn = [self ideWaitForInterrupt:IDE_WRITE_MULTIPLE 
				ideStatus:&status];

	if (rtn != IDER_SUCCESS) 
        {
	    [self getIdeRegisters:ideRegs Print:"Write Multiple"];
	    return (rtn);
	}
	
	if (status & (ERROR | WRITE_FAULT)) 
        {
	    [self getIdeRegisters:ideRegs Print:"Write Multiple"];
	    return IDER_CMD_ERROR;
	}
	
	/*
	if (status & ERROR_CORRECTED) 
        {
	    IOLog("%s: Corrected error during write.\n", [self name]);
	}
	*/
	
    }

    return (rtn);
}

/*
 * All I/O to to controller object is done through this method. We first
 * acquire a lock before executing any IDE commands since the IDE controller
 * can do only one thing at a time (both drivers can not be active
 * simultaneously except for reset and disgnostics). 
 *
 * If it necessary to call any of the IDE command methods (which are invoked by
 * the switch() below, like ideReadGetInfoCommon:client:addr:command) it is
 * necessary to acquire the lock. The IDE command methods should not be
 * invoked directly by the Disk object. 
 *
 * This method will in turn call one of several methods which deal with
 * hardware. 
 */


#define MAX_COMMAND_RETRY 	3

- (IOReturn) _ideExecuteCmd:(ideIoReq_t *)ideIoReq ToDrive:(unsigned char)drive
{
    int     		retry;
    ideRegsVal_t 	irv;
    unsigned 		block, cnt;
    unsigned 		error;
    unsigned int 	maxSectors;
    unsigned char 	dh;

    /*
     * If the controller wishes to put the drive to sleep, it sets this flag
     * to YES and tries to acquire the lock. This method should not try to
     * get the lock (and execute any more commands) if the flag is set. This
     * is needed in order to enter sleep mode as soon as the current command
     * is finished. If we do not do this there is contention for lock between
     * the controller (which wants to put the drive to sleep) and this method
     * (which wants to service more requests). Note that this does not do
     * away with the need for a lock, it only gives the controller a little
     * head-start. 
     */
    
    while (_driveSleepRequest) 
    {
	IOSleep(100);
    }
     
    [self ideCntrlrLock];

    _driveNum = drive;		/* used by IDE command methods. */
    
    /*
     * Select the drive first. We don't know the head number at this time so
     * this register will be rewritten by the specific routine later. 
     */
    dh = _addressMode[_driveNum];
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    
   /*
    * Check if we need to do a media access to get the drive respinning
    * after a suspend operation. 
    */
    if ([self drivePowerState] != IDE_PM_ACTIVE)	
    {
        (void)[self startUpAttachedDevices];
    }

    ideIoReq->status = IDER_CMD_ERROR;
    ideIoReq->blocks_xfered = 0;

    for  (retry = 0; retry < MAX_COMMAND_RETRY; retry++) 
    {    
	[self clearInterrupts];

	switch (ideIoReq->cmd) 
        {
	  case IDE_READ:
	  case IDE_READ_MULTIPLE:

	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
	    if ((cnt > MAX_BLOCKS_PER_XFER) || (cnt > (IDE_MAX_PHYS_IO/IDE_SECTOR_SIZE)))	
            {
		ideIoReq->status = IDER_REJECT;
		break;
	    }
            [self setTransferRate: _driveNum UseDMA:NO];
	    ideIoReq->regValues = [self logToPhys:block numOfBlocks:cnt];
	    if (ideIoReq->cmd == IDE_READ)
		ideIoReq->status = 
			[self ideReadGetInfoCommon:&(ideIoReq->regValues) 
			client:(ideIoReq->map) 
			addr:(ideIoReq->addr) command:IDE_READ];
	    else
		ideIoReq->status =
		    [self ideReadMultiple:&(ideIoReq->regValues)
		     client:(ideIoReq->map) addr:(ideIoReq->addr)];

	    if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = ideIoReq->blkcnt;
	    break;

	  case IDE_WRITE:
	  case IDE_WRITE_MULTIPLE:

	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
	    if ((cnt > MAX_BLOCKS_PER_XFER) || (cnt > (IDE_MAX_PHYS_IO/IDE_SECTOR_SIZE)))	
            {
		ideIoReq->status = IDER_REJECT;
		break;
	    }
            [self setTransferRate: _driveNum UseDMA:NO];
	    ideIoReq->regValues = [self logToPhys:block numOfBlocks:cnt];
	    if (ideIoReq->cmd == IDE_WRITE)
		ideIoReq->status = [self ideWrite:&(ideIoReq->regValues)
					client:(ideIoReq->map)
					addr:(ideIoReq->addr)];
	    else
		ideIoReq->status = 
			[self ideWriteMultiple:&(ideIoReq->regValues)
			client:(ideIoReq->map) addr:(ideIoReq->addr)];

	    if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = ideIoReq->blkcnt;
	    break;

	  case IDE_WRITE_DMA:
	  case IDE_READ_DMA:
	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
	    if ((cnt > MAX_BLOCKS_PER_XFER) || (cnt > (IDE_MAX_PHYS_IO / IDE_SECTOR_SIZE))) 
            {
		ideIoReq->status = IDER_REJECT;
		break;
	    }
            [self setTransferRate: _driveNum UseDMA:YES];
	    ideIoReq->status = [self ideDmaRwCommon:ideIoReq];
	    if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = ideIoReq->blkcnt;
	    break;

	  case IDE_SEEK:
	    block = ideIoReq->block;
	    cnt = 1;
	    ideIoReq->regValues = [self logToPhys:block numOfBlocks:cnt];
	    ideIoReq->status = 
	    		[self ideReadVerifySeekCommon:&(ideIoReq->regValues) 
				command:IDE_SEEK];
	    break;

	  case IDE_RESTORE:
	    ideIoReq->status = [self ideRestore:&(ideIoReq->regValues)];
	    break;

	  case IDE_READ_VERIFY:
	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
	    if (cnt > MAX_BLOCKS_PER_XFER) {
		ideIoReq->status = IDER_REJECT;
		break;
	    }
	    ideIoReq->regValues = [self logToPhys:block numOfBlocks:cnt];
	    ideIoReq->status = 
	    	[self ideReadVerifySeekCommon:&(ideIoReq->regValues)
				command:IDE_READ_VERIFY];
	    break;

	  case IDE_DIAGNOSE:
	    [self ideDiagnose:&error];
	    ideIoReq->diagResult = error;
	    ideIoReq->status = IDER_SUCCESS;
	    break;

	  case IDE_SET_PARAMS:
	    ideIoReq->status = 
	    		[self ideSetParams:_ideInfo[_driveNum].sectors_per_trk
			numHeads:_ideInfo[_driveNum].heads ForDrive:_driveNum];
	    break;

	  case IDE_IDENTIFY_DRIVE:
            [self setTransferRate: _driveNum UseDMA:NO];
	    ideIoReq->status = 
	    		[self ideReadGetInfoCommon:&(ideIoReq->regValues)
			client :(ideIoReq->map) addr :(ideIoReq->addr)
			command:IDE_IDENTIFY_DRIVE];

	    if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = 1;
	    break;

	  case IDE_SET_MULTIPLE:
            maxSectors = (_ideIdentifyInfo[_driveNum].multipleSectors) &
			IDE_MULTI_SECTOR_MASK;
	    if (ideIoReq->maxSectorsPerIntr > maxSectors) 
            {
		ideIoReq->status = IDER_REJECT;
	    } 
            else 
            {
		ideIoReq->status = [self ideSetMultiSectorMode:
				    &(ideIoReq->regValues)
				    numSectors:ideIoReq->maxSectorsPerIntr];
		if (ideIoReq->status == IDER_SUCCESS)
		    _multiSector[_driveNum] = ideIoReq->maxSectorsPerIntr;
		else
		    _multiSector[_driveNum] = 0;
	    }
	    break;

	  default:
	    ideIoReq->status = IDER_REJECT;
	    [self ideCntrlrUnLock];
	    return (IDER_REJECT);
	}

#if 0
        kprintf("Disk(ATA): Cmd = %02x LBA = %08x Length = %08x Addr=%08x:%08x Status = %d\n\r", 
               ideIoReq->cmd, (int)ideIoReq->block, (int)ideIoReq->blkcnt, (int)ideIoReq->addr, (int)ideIoReq->map, 
               ideIoReq->status);
#endif

	/*
	 * Return if command has been executed successfully or summarily
	 * rejected. 
	 */
	if (ideIoReq->status == IDER_SUCCESS)	
        {
	    [self ideCntrlrUnLock];
	    return IDER_SUCCESS;
	}
	
	if (ideIoReq->status == IDER_REJECT)	
        {
	    [self ideCntrlrUnLock];
	    return IDER_REJECT;
	}

	/*
	 * The command failed to exceute properly but was accepted by the
	 * drive. Reset the drives and try again. 
	 */
	IOLog("%s: ATA command %x failed. Retrying..\n", [self name], 
		ideIoReq->cmd);
	[self getIdeRegisters:NULL Print:"ATA Command"];
	[self resetAndInit];
	(void) [self ideRestore:&irv];
    }

    /*
     * If we get here then this is a catastrophic failure. 
     */
    [self ideCntrlrUnLock];
    return ideIoReq->status;
}

@end
