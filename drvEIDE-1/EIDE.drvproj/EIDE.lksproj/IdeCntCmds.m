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
 * IdeCntCmds.m - Implementation of Commands category for IDE Controller 
 * device. 
 *
 * It contains implementation of the ATA command set. 
 *
 *
 * HISTORY 
 *
 * 23-Feb-1998		Joe Liu at Apple
 *  startUpAttachedDevices method is called immediately after acquiring
 *  the lock.
 *
 * 1-Feb-1998		Joe Liu at Apple
 *	ideSetDriveFeature method changed to support all EIDE transfer modes.
 *	Added support for Bus Master DMA.
 *  ideExecuteCmd now sets _driveNum within the retry loop.
 *
 * 1-Aug-1995	 	Rakesh Dubey at NeXT 
 *	Reduced the timeout in IDE_IDENTIFY_DRIVE method. 
 *
 * 17-July-1994 	Rakesh Dubey at NeXT 
 *	Created. 
 */

//#define DEBUG

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdePIIX.h"
#import <kern/assert.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import <machdep/i386/io_inline.h>
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

    if (_drives[_driveNum].addressMode == ADDRESS_MODE_CHS) {
		nheads = _drives[_driveNum].ideInfo.heads;
		spt = _drives[_driveNum].ideInfo.sectors_per_trk;
	
		track = (block / spt);
		cyl = track / nheads;
		rv.sectNum = block % spt + 1;
	
		rv.drHead = track % nheads;
		rv.cylLow = (cyl & 0xff);
		rv.cylHigh = ((cyl & 0xff00) >> 8);
    } else {
        rv.sectNum = block & 0x0ff;
		rv.cylLow = (block >> 8) & 0xff;
		rv.cylHigh = (block >> 16) & 0xff;
		rv.drHead = (block >> 24) & 0x0f;
    }
    
    rv.sectCnt = (nblk == MAX_BLOCKS_PER_XFER) ? 0 : nblk;

    rv.drHead |= _drives[_driveNum].addressMode;
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
    if (rtn != IDER_SUCCESS)	{
	[self getIdeRegisters:NULL Print:"Diagnose"];
	return IDER_CMD_ERROR;
    }

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return IDER_CMD_ERROR;
    
    *diagError = inb(_ideRegsAddrs.error);
    
    if (*diagError == 0x01)	{
        /* FIXME: do we need to soft reset ATAPI devices? */
	return IDER_SUCCESS;
    }

    /*
     * At least one of the drives is bad. 
     */
    if (*diagError & 0x080) {
	IOLog("%s: Drive 1 has failed diagnostics.\n", [self name]);
	_drives[1].ideInfo.type = 0;

	if ((*diagError & 0x07f) != 1) {
	    IOLog("%s: Drive 0 has failed diagnostics, error %d\n",
		    [self name], (*diagError & 0x07f));
	    return IDER_CMD_ERROR;
	}
    } else {
	_drives[0].ideInfo.type = 0;
	IOLog("%s: Drive 0 has failed diagnostics, error %d\n",
		[self name], (*diagError & 0x07f));
    }
    
    return IDER_CMD_ERROR;
}

- (ide_return_t)ideSetParams:(unsigned)sectCnt numHeads:(unsigned)nHeads
			ForDrive:(unsigned)drive
{
    unsigned char dh = _drives[drive].addressMode;
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
    if (rtn != IDER_SUCCESS) {
	[self getIdeRegisters:NULL Print:"SetParams"];
	return rtn;
    }
    
    return rtn;
}

- (ide_return_t)ideSetDriveFeature:(unsigned char)feature
	value:(unsigned char)val
	transferType:(ideTransferType_t)type
{
    unsigned char dh = _drives[_driveNum].addressMode;
    unsigned char status;
    ide_return_t rtn;
//	unsigned char tmp;

#ifdef DEBUG
    IOLog("Setting drive feature %x to %x\n", feature, val);
#endif DEBUG

    /*
     * We are not going to do anything if EIDE support is disabled.
	 *
	 * Currently, this is useful only for setting the transfer mode.
     */
    if ((_EIDESupport == NO) || (feature != FEATURE_SET_TRANSFER_MODE) ||
		(val == ATA_MODE_NONE)) 
    	return IDER_SUCCESS;
	
	val = ata_mode_to_num(val);
//	IOLog("Mode num: %d\n", val);
	
    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS)
    	return rtn;
	
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
	
	val &= 0x07;
	switch (type) {
		case IDE_TRANSFER_PIO:
			val |= (1 << 3);
			break;
		case IDE_TRANSFER_SW_DMA:
			val |= (1 << 4);
			break;
		case IDE_TRANSFER_MW_DMA:
			val |= (1 << 5);
			break;
		case IDE_TRANSFER_ULTRA_DMA:
			val |= (1 << 6);
			break;
		default:
			val = 0;
	}
	outb(_ideRegsAddrs.sectCnt, val);
    outb(_ideRegsAddrs.features, feature);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_SET_FEATURES);
    
    rtn = [self ideWaitForInterrupt:IDE_SET_FEATURES ideStatus:&status];
    if (rtn != IDER_SUCCESS) {
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
    unsigned char dh = _drives[_driveNum].addressMode;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) {
	return rtn;
    }
    
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_RESTORE);

    rtn = [self ideWaitForInterrupt:IDE_RESTORE ideStatus: &status];
    if (rtn != IDER_SUCCESS) {
	[self getIdeRegisters:NULL Print:"Restore"];
	return rtn;
    }
    
    rtn = [self waitForDeviceReady];
    if (rtn == IDER_SUCCESS) {
	if (status & ERROR) {
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
    unsigned char dh = _drives[_driveNum].addressMode;

    if (sec_cnt == 0)
	sec_cnt = MAX_BLOCKS_PER_XFER;
	
    taddr = xferAddr;

    /*
     * We have to define this for the IDE_IDENTIFY_DRIVE command. 
     */
    if (cmd == IDE_IDENTIFY_DRIVE)	{
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
    if (rtn != IDER_SUCCESS) {
        if (_ide_debug)	{
	    IOLog("ideReadGetInfoCommon: waitForDeviceReady\n");
	    [self getIdeRegisters:ideRegs Print:NULL];
	}
	return (rtn);
    }
    
    if (cmd == IDE_READ) {
	outb(_ideRegsAddrs.drHead, ideRegs->drHead);
	outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
	outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
	outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
	outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    } else {
        /* probably unnecessary */
	outb(_ideRegsAddrs.drHead, dh);
	outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    }
        
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, cmd);
        
    for (i = 0; i < sec_cnt; i++)	{

	rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
	if (rtn != IDER_SUCCESS) {
	    if (_ide_debug)	{
		IOLog("ideReadGetInfoCommon: ideWaitForInterrupt\n");
		[self getIdeRegisters:ideRegs Print:NULL];
	    }
	    return rtn;
	} else {
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
	if (rtn != IDER_SUCCESS) {
	    if (_ide_debug)	{
		IOLog("ideReadGetInfoCommon: "
			"ataIdeReadGetInfoCommonWaitForDataReady\n");
		[self getIdeRegisters:ideRegs Print:NULL];
	    }
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
    if (rtn != IDER_SUCCESS) {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_WRITE);

    for (i = 0; i < sec_cnt; i++)	{

	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) {
	    return (rtn);
	}

	[self xferData:taddr read:NO client:client length:IDE_SECTOR_SIZE];
	taddr += IDE_SECTOR_SIZE;

	rtn = [self ideWaitForInterrupt:IDE_WRITE ideStatus:&status];
    
	if (rtn != IDER_SUCCESS) {
	    [self getIdeRegisters:ideRegs Print:"Write"];
	    return (rtn);
	} else {
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
    if (rtn != IDER_SUCCESS) {
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
    if (rtn != IDER_SUCCESS) {
	[self getIdeRegisters:NULL Print:"ReadVerify/Seek"];
	return (rtn);
    }
    
    if (status & (ERROR | WRITE_FAULT)) {
	rtn = IDER_CMD_ERROR;
    } else {
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
    unsigned char dh = _drives[_driveNum].addressMode;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) {
	return (rtn);
    }
    
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    outb(_ideRegsAddrs.sectCnt, nSectors);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_SET_MULTIPLE);

    rtn = [self ideWaitForInterrupt:IDE_SET_MULTIPLE ideStatus:&status];
    if (rtn != IDER_SUCCESS) {
	return (rtn);
    }
    
    rtn = [self waitForDeviceReady];
    if (rtn == IDER_SUCCESS) {
	if (status & ERROR) {
	    rtn = IDER_CMD_ERROR;
	    [self getIdeRegisters:ideRegs Print:NULL];
	} else
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
    if (rtn != IDER_SUCCESS) {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);

    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_READ_MULTIPLE);

    ddm_ide_cmd("ideReadMultiple: sec_cnt %d sectors\n", sec_cnt, 2,3,4,5);
    
    while (sec_cnt > 0) {

	ddm_ide_cmd("ideReadMultiple: waiting for interrupt\n",1,2,3,4,5);
	rtn = [self ideWaitForInterrupt:IDE_READ_MULTIPLE
			ideStatus: &status];
	ddm_ide_cmd("ideReadMultiple: received interrupt\n",1,2,3,4,5);

	if (rtn != IDER_SUCCESS) {
	    [self getIdeRegisters:ideRegs Print:"Read Multiple"];
	    return (rtn);
	}

#if 1	
	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) {
	    return (rtn);
	}
#endif 1

	if (status & (ERROR | WRITE_FAULT)) {
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
	if (status & ERROR_CORRECTED) {
	    IOLog("%s: Corrected error during read.\n", [self name]);
	}
	*/

	/*
	 * All is well. Read in the data. 
	 */
	if (sec_cnt > _drives[_driveNum].multiSector)
	    nSectors = _drives[_driveNum].multiSector;
	else
	    nSectors = sec_cnt;
	    
	sec_cnt -= nSectors;

	ddm_ide_cmd("ideReadMultiple: starting data transfer\n",1,2,3,4,5);

	while (nSectors) {
	    if (nSectors > PAGE_SIZE / IDE_SECTOR_SIZE) {
		length = PAGE_SIZE;
		nSectors -= (PAGE_SIZE / IDE_SECTOR_SIZE);
	    } else {
		length = nSectors * IDE_SECTOR_SIZE;
		nSectors = 0;
	    }

	    [self xferData:taddr read:YES client:client length:length];
	    taddr += length;
	}

	ddm_ide_cmd("ideReadMultiple: data transfer done\n",1,2,3,4,5);
    }

    ddm_ide_cmd("ideReadMultiple: completed\n",1,2,3,4,5);

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
    if (rtn != IDER_SUCCESS) {
	return (rtn);
    }
    
    outb(_ideRegsAddrs.drHead, ideRegs->drHead);
    outb(_ideRegsAddrs.sectNum, ideRegs->sectNum);
    outb(_ideRegsAddrs.sectCnt, ideRegs->sectCnt);
    outb(_ideRegsAddrs.cylLow, ideRegs->cylLow);
    outb(_ideRegsAddrs.cylHigh, ideRegs->cylHigh);
    
    [self enableInterrupts];
    outb(_ideRegsAddrs.command, IDE_WRITE_MULTIPLE);

    ddm_ide_cmd("ideWriteMultiple: sec_cnt %d sectors\n", sec_cnt, 2,3,4,5);

    while (sec_cnt > 0) {
    
	rtn = [self waitForDataReady];
	if (rtn != IDER_SUCCESS) {
	    return (rtn);
	}
	
	if (sec_cnt > _drives[_driveNum].multiSector)
	    nSectors = _drives[_driveNum].multiSector;
	else
	    nSectors = sec_cnt;
	    
	sec_cnt -= nSectors;

	ddm_ide_cmd("ideWriteMultiple: starting data transfer\n",1,2,3,4,5);

	while (nSectors) {
	    if (nSectors > PAGE_SIZE / IDE_SECTOR_SIZE) {
		length = PAGE_SIZE;
		nSectors -= (PAGE_SIZE / IDE_SECTOR_SIZE);
	    } else {
		length = nSectors * IDE_SECTOR_SIZE;
		nSectors = 0;
	    }

	    [self xferData:taddr read:NO client:client length:length];
	    taddr += length;
	}
	ddm_ide_cmd("ideWriteMultiple: data transfer done\n",1,2,3,4,5);

	ddm_ide_cmd("ideWriteMultiple: waiting for interrupt\n",1,2,3,4,5);
	rtn = [self ideWaitForInterrupt:IDE_WRITE_MULTIPLE 
				ideStatus:&status];
	ddm_ide_cmd("ideWriteMultiple: received interrupt\n",1,2,3,4,5);

	if (rtn != IDER_SUCCESS) {
	    [self getIdeRegisters:ideRegs Print:"Write Multiple"];
	    return (rtn);
	}
	
	if (status & (ERROR | WRITE_FAULT)) {
	    [self getIdeRegisters:ideRegs Print:"Write Multiple"];
	    return IDER_CMD_ERROR;
	}
	
	/*
	if (status & ERROR_CORRECTED) {
	    IOLog("%s: Corrected error during write.\n", [self name]);
	}
	*/
	
    }

    ddm_ide_cmd("ideWriteMultiple: completed\n",1,2,3,4,5);

    return (rtn);
}

/*
 * Method: performDMATestOnDrive:(unsigned char)drive
 *
 * Purpose:
 * Read a few sectors and make sure DMA really "seems" to works.
 *
 * NOTE: _driveNum must be set prior to calling this method.
 */
- (ide_return_t)performDMATest
{
    ideIoReq_t	ideIoReq;
    ide_return_t status;
    vm_offset_t tempDmaBuf;
	vm_offset_t alignBuf;
	unsigned int currentTimeout;
    unsigned char 	dh;
	
    /*
     * The hardware claims to supports DMA. Verify by reading a
     * few sectors. Allocate memory for dummy buffer.
     */
	tempDmaBuf = (vm_offset_t)IOMalloc(PAGE_SIZE);
    if (tempDmaBuf == NULL)	{
		IOLog("%s: memory allocation failed\n", [self name]);
		return IDER_REJECT;
    }

	/*
	 * Advance the pointer and make the buffer 4 byte aligned.
	 */
	alignBuf = (tempDmaBuf + 3) & ~3;

	bzero((unsigned char *)&ideIoReq, sizeof(ideIoReq_t));
    ideIoReq.cmd = IDE_READ_DMA;
    ideIoReq.block = 0;
    ideIoReq.blkcnt = (PAGE_SIZE - 4)/IDE_SECTOR_SIZE;
    ideIoReq.addr = (caddr_t)alignBuf;
    ideIoReq.timeout = 5000;
    ideIoReq.map = (struct vm_map *)IOVmTaskSelf();
	
	/*
	 * Select the drive first.
	 */
	if ([self waitForDeviceIdle] != IDER_SUCCESS) {
		IOLog("%s: Drive %d DMA test FAILED\n", [self name], _driveNum);
		return IDER_TIMEOUT;
	}
    dh = _drives[_driveNum].addressMode;
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
	
	/*
	 * Wait 400ns before reading the status register and make sure the
	 * currently selected drive is ready to accept a command.
	 */
	IODelay(1);
	if ([self waitForDeviceIdle] != IDER_SUCCESS) {
		IOLog("%s: Drive %d DMA test FAILED\n", [self name], _driveNum);
		return IDER_TIMEOUT;
	}	
	
	/*
	 * Clear any unwanted interrupts which may have accumulated.
	 */
	[self enableInterrupts];
	IOSleep(100);
	
	/*
	 * Set a smaller (3 sec) timeout for this test.
	 */
	currentTimeout = [self interruptTimeOut];
	[self setInterruptTimeOut:3000];
	[self clearInterrupts];
	
	/*
	 * Perform test.
	 */
	if (([self performDMA:(ideIoReq_t *)&ideIoReq]) == IDER_SUCCESS) {
		status = IDER_SUCCESS;
//		IOLog("%s: Drive %d: DMA test PASSED\n", [self name], _driveNum);
	}
	else {
		status = IDER_REJECT;
    	IOLog("%s: Drive %d: DMA test FAILED\n", [self name], _driveNum);
	}
	
	/*
	 * Revert the original timeout value and free allocated memory.
	 */
	[self setInterruptTimeOut:currentTimeout];
    IOFree((void *)tempDmaBuf, PAGE_SIZE);
	return status;
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

static unsigned char unaligned_warnings;
#define UNALIGNED_WARNINGS_MAX	20

#define MAX_COMMAND_RETRY 	3

- (IOReturn) ideExecuteCmd:(ideIoReq_t *)ideIoReq ToDrive:(unsigned char)drive
{
    int     		retry;
    ideRegsVal_t 	irv;
    unsigned 		block, cnt;
    unsigned 		error;
    unsigned int 	maxSectors;
    unsigned char 	dh;

    ddm_ide_cmd("ideExecuteCmd: executing %x\n", ideIoReq->cmd,2,3,4,5);

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
    
    while (_driveSleepRequest) {
		IOSleep(100);
    }
     
    ddm_ide_lock("ideExecuteCmd: acquiring lock\n",1,2,3,4,5);
    [self ideCntrlrLock];
    ddm_ide_lock("ideExecuteCmd: acquired lock\n",1,2,3,4,5);

   /*
    * Check if we need to do a media access to get the drive respinning
    * after a suspend operation. 
    */
    if ([self drivePowerState] != IDE_PM_ACTIVE) {
		[self startUpAttachedDevices];
    }

    _driveNum = drive;		/* used by IDE command methods. */
	
    for  (retry = 0; retry < MAX_COMMAND_RETRY; retry++) {
    
    /*
     * Select the drive first. We don't know the head number at this time so
     * this register will be rewritten by the specific routine later.
	 *
	 * The Device Selection protocol is defined as follows:
	 * HOST: Read Status or AltStatus register
	 * HOST: Continue reading until BSY = 0, and DRQ = 0
	 * HOST: Write Device/Head register with appropriate DEV bit value
	 * HOST: Wait 400ns
	 * HOST: Read Status or AltStatus register
	 * HOST: Continue reading until BSY = 0, and DRQ = 0
	 *
     */
//	[self waitForDeviceIdle];	// Devices should be already in an idle state
    dh = _drives[_driveNum].addressMode;
    dh |= (_driveNum ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
	IODelay(1);
//	[self waitForDeviceIdle];	// Each method below will do their own wait

    ideIoReq->status = IDER_CMD_ERROR;
    ideIoReq->blocks_xfered = 0;
	
	[self clearInterrupts];

	switch (ideIoReq->cmd) {

	  case IDE_READ_DMA:
		if (((vm_offset_t)ideIoReq->addr & 0x03) == 0) {
	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
		ddm_ide_log("IDE_READ_DMA: %d\n", cnt, 2, 3, 4, 5);
	    if (cnt > MAX_BLOCKS_PER_XFER) {
			ideIoReq->status = IDER_REJECT;
			break;
	    }

		ideIoReq->status = [self performDMA:ideIoReq];
		if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = ideIoReq->blkcnt;
	    break;
		}
		
		/*
		 * If we reached here, it means that the buffer is not 4-byte
		 * aligned. This should not happen.
		 */
		if (unaligned_warnings < UNALIGNED_WARNINGS_MAX) {
			IOLog("%s: READ DMA: buffer not 4-byte aligned\n", [self name]);
			unaligned_warnings++;
		}

	  case IDE_READ:
	  case IDE_READ_MULTIPLE:

	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
		ddm_ide_log("IDE_READ: %d\n", cnt, 2, 3, 4, 5);
	    if (cnt > MAX_BLOCKS_PER_XFER)	{
			ideIoReq->status = IDER_REJECT;
			break;
	    }
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

	  case IDE_WRITE_DMA:
		if (((vm_offset_t)ideIoReq->addr & 0x03) == 0) {
	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
		ddm_ide_log("IDE_WRITE_DMA: %d\n", cnt, 2, 3, 4, 5);
	    if (cnt > MAX_BLOCKS_PER_XFER) {
			ideIoReq->status = IDER_REJECT;
			break;
	    }

		ideIoReq->status = [self performDMA:ideIoReq];
		if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = ideIoReq->blkcnt;
	    break;
		}

		/*
		 * If we reached here, it means that the buffer is not 4-byte
		 * aligned. This should not happen.
		 */
		if (unaligned_warnings < UNALIGNED_WARNINGS_MAX) {
			IOLog("%s: WRITE DMA: buffer not 4-byte aligned\n", [self name]);
			unaligned_warnings++;
		}

	  case IDE_WRITE:
	  case IDE_WRITE_MULTIPLE:

	    block = ideIoReq->block;
	    cnt = ideIoReq->blkcnt;
		ddm_ide_log("IDE_WRITE: %d\n", cnt, 2, 3, 4, 5);
	    if (cnt > MAX_BLOCKS_PER_XFER) {
			ideIoReq->status = IDER_REJECT;
			break;
	    }
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
		ddm_ide_log("IDE_READ_VERIFY: %d\n", cnt, 2, 3, 4, 5);
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
	    		[self ideSetParams:_drives[_driveNum].ideInfo.sectors_per_trk
			numHeads:_drives[_driveNum].ideInfo.heads ForDrive:_driveNum];
	    break;

	  case IDE_IDENTIFY_DRIVE:
	    ideIoReq->status = 
	    		[self ideReadGetInfoCommon:&(ideIoReq->regValues)
			client :(ideIoReq->map) addr :(ideIoReq->addr)
			command:IDE_IDENTIFY_DRIVE];

	    if (ideIoReq->status == IDER_SUCCESS)
		ideIoReq->blocks_xfered = 1;
	    break;

	  case IDE_SET_MULTIPLE:
            maxSectors = (_drives[_driveNum].ideIdentifyInfo->multipleSectors) 
				& IDE_MULTI_SECTOR_MASK;
	    if (ideIoReq->maxSectorsPerIntr > maxSectors) {
		ideIoReq->status = IDER_REJECT;
	    } else {
		ideIoReq->status = [self ideSetMultiSectorMode:
				    &(ideIoReq->regValues)
				    numSectors:ideIoReq->maxSectorsPerIntr];
		if (ideIoReq->status == IDER_SUCCESS)
		    _drives[_driveNum].multiSector = ideIoReq->maxSectorsPerIntr;
		else
		    _drives[_driveNum].multiSector = 0;
	    }
	    break;

	  default:
	    ideIoReq->status = IDER_REJECT;
	    ddm_ide_lock("ideExecuteCmd: releasing lock, bad cmd\n",1,2,3,4,5);
	    [self ideCntrlrUnLock];
	    return (IDER_REJECT);
	}

	/*
	 * Return if command has been executed successfully or summarily
	 * rejected. 
	 */
	if (ideIoReq->status == IDER_SUCCESS)	{
	    [self ideCntrlrUnLock];
	    ddm_ide_lock("ideExecuteCmd: releasing lock, success\n",1,2,3,4,5);
	    return IDER_SUCCESS;
	}
	
	if (ideIoReq->status == IDER_REJECT)	{
	    [self ideCntrlrUnLock];
	    ddm_ide_lock("ideExecuteCmd: releasing lock, reject\n",1,2,3,4,5);
	    return IDER_REJECT;
	}

	/*
	 * The command failed to exceute properly but was accepted by the
	 * drive. Reset the drives and try again. 
	 */
	IOLog("%s: ATA command %x failed. Retrying...\n", [self name], 
		ideIoReq->cmd);
	[self getIdeRegisters:NULL Print:"ATA Command"];
	[self resetAndInit];
	
	/*
	 * resetAndInit will change the value of _driveNum.
	 * Revert _driveNum to the original value before retrying the
	 * command.
	 */
	_driveNum = drive;
	(void) [self ideRestore:&irv];
    }

    /*
     * If we get here then this is a catastrophic failure. 
     */
    ddm_ide_lock("ideExecuteCmd: releasing lock, failed\n",1,2,3,4,5);
    [self ideCntrlrUnLock];
    return ideIoReq->status;
}

@end
