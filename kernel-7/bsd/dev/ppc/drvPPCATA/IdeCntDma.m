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
/* 	Copyright (c) 1991-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCntDma.m - Implementation for Dma category of Ide Controller device.
 *
 */

#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/ppc/IODBDMA.h>
#import <driverkit/align.h>
#import <machdep/ppc/powermac.h>
#import <sys/systm.h>

#import "IdeCntDma.h"
#import "io_inline.h"

extern 	IOReturn		kmem_alloc_wired();

static 	void 			*bitBucketAddr;
static 	uint			bitBucketPhysAddr; 		

@implementation IdeController(Dma)

/*
 * Called once during initialization to figure out if the hardware 
 * supports dma
 */

- (ide_return_t) initIdeDma:(unsigned int)unit 
{
    ideIoReq_t			ideIoReq;
    vm_offset_t 		tempDmaBuf;
    ide_return_t 		status;

    /*
     * The hardware claims to supports DMA. Verify that by trying to read a
     * few sectors. 
     */
    tempDmaBuf = (vm_offset_t)IOMalloc(PAGE_SIZE);
    if (tempDmaBuf == NULL)	
    {
	_dmaSupported[unit] = NO;
	return IDER_REJECT;
    }
    
    bzero((unsigned char *)&ideIoReq, sizeof(ideIoReq_t));
    ideIoReq.cmd = IDE_READ_DMA;
    ideIoReq.block = 0;
    ideIoReq.blkcnt = (PAGE_SIZE/IDE_SECTOR_SIZE);
    ideIoReq.addr = (caddr_t)tempDmaBuf;
    ideIoReq.timeout = 5000;
    ideIoReq.map = (struct vm_map *)IOVmTaskSelf();
    
    [self setTransferRate: unit UseDMA: YES];

    if ([self ideDmaRwCommon:(ideIoReq_t *)&ideIoReq] == IDER_SUCCESS)
    {
	status = IDER_SUCCESS;
    } 
    else 
    {
	_dmaSupported[unit] = NO;
	status = IDER_REJECT;
    }
    
    IOFree((void *)tempDmaBuf, PAGE_SIZE);
    
    return status;
}

- (ide_return_t) allocDmaMemory
{
    IOReturn	rc;

    if ( !_ideDMACommands )
    {
        rc = kmem_alloc_wired(IOVmTaskSelf(), (vm_offset_t *) &_ideDMACommands, PAGE_SIZE );

        if ( rc != KERN_SUCCESS  )
	{
          return IDER_REJECT;
        }
  	rc = IOPhysicalFromVirtual( IOVmTaskSelf(), (vm_offset_t)_ideDMACommands, (vm_offset_t *)&_ideDMACommandsPhys );
        if ( rc != IO_R_SUCCESS )
	{
            return IDER_REJECT;
        }
    }   
    
    if ( !bitBucketAddr )
    {
        bitBucketAddr = IOMalloc(32);
        bitBucketAddr = IOAlign(void *, bitBucketAddr, 16);
        rc = IOPhysicalFromVirtual( IOVmTaskSelf(), (vm_offset_t) bitBucketAddr, (vm_offset_t *)&bitBucketPhysAddr );
        if ( rc != IO_R_SUCCESS )
	{
            return IDER_REJECT;
        }
    }

    return IDER_SUCCESS;
}


- (ide_return_t) ideDmaRwCommon: (ideIoReq_t *)ideIoReq
{
    vm_offset_t 		startAddr = (vm_offset_t)ideIoReq->addr;
    unsigned 			blocksToGo = ideIoReq->blkcnt;
    unsigned 			startBlock = ideIoReq->block;
    ide_return_t 		rc = IDER_SUCCESS;
    BOOL			fRead = (ideIoReq->cmd == IDE_READ_DMA);

    /*
     * Do read or write DMA. 
     */
    ideIoReq->regValues = [self logToPhys:startBlock numOfBlocks:blocksToGo];

    rc = [self setupDMA: startAddr client:(vm_task_t)ideIoReq->map length:blocksToGo * IDE_SECTOR_SIZE fRead:(BOOL)fRead];
    if ( rc != IDER_SUCCESS )
    {
        return ( rc );
    }  

    rc = [self ideReadWriteDma: &ideIoReq->regValues fRead:fRead]; 

    return(rc);	
}

- (ide_return_t) ideReadWriteDma:(ideRegsVal_t *)ideRegs fRead:(BOOL)read
{
    ide_return_t 		rtn;
    ideRegsAddrs_t 		*rp = &_ideRegsAddrs;
    unsigned char 		status;
    unsigned long		cfgReg;

    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) 
    {
	return (rtn);
    }

    outb(rp->drHead,  ideRegs->drHead);
    outb(rp->sectNum, ideRegs->sectNum);
    outb(rp->sectCnt, ideRegs->sectCnt);
    outb(rp->cylLow,  ideRegs->cylLow);
    outb(rp->cylHigh, ideRegs->cylHigh);
    
    [self enableInterrupts];
    outb(rp->command, (read ? IDE_READ_DMA : IDE_WRITE_DMA));

    if ( _controllerType != kControllerTypeCmd646X )
    {
        IODBDMAContinue( _ideDMARegs );        
    }
    else
    {
        [[self deviceDescription] configReadLong:0x70 value: &cfgReg];
        cfgReg &= ~0x08;
        cfgReg |= 0x01 | ((read) ? 0x08 : 0x00);
        [[self deviceDescription] configWriteLong:0x70 value: cfgReg];
    }

    rtn = [self ideWaitForInterrupt:(read ? IDE_READ_DMA : IDE_WRITE_DMA)
    			ideStatus: &status];

#if 0
  {
    uint	dmaStatus;

    dmaStatus = IOGetDBDMAChannelStatus( _ideDMARegs );
    kprintf( "DMA Status = %04x IDE Status = %02x\n\r", dmaStatus, status );
  }
#endif

    if ( _controllerType != kControllerTypeCmd646X )
    {
        IODBDMAStop( _ideDMARegs );
    }
    else
    {
        [[self deviceDescription] configReadLong: 0x70 value: &cfgReg];
        cfgReg &= ~0x01;
        [[self deviceDescription] configWriteLong:0x70 value: cfgReg];
    }
	 
    if ( (rtn == IDER_SUCCESS) ) 
    {
	if ((rtn = [self waitForDeviceReady]) == IDER_SUCCESS) 
        {
	    if (status & (ERROR | WRITE_FAULT)) 
            {
		[self getIdeRegisters:ideRegs Print:"Read Dma error"];
		return IDER_CMD_ERROR;
	    }
	    
	    /*
	    if (status & ERROR_CORRECTED) {
		IOLog("%s: Error during data transfer (corrected).\n", 
			    [self name]);
	    }
	    */
	} 
        else 
        {
	    return IDER_CMD_ERROR;
	}
	
    } 
    else 
    {
	[self getIdeRegisters:ideRegs Print:NULL];
	IODBDMAReset( _ideDMARegs );
	return IDER_CMD_ERROR;
    }

    return (rtn);
}

/*
 *
 *
 */
-(ide_return_t) setupDMA:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead 
{
    ide_return_t		rc;

    if ( _controllerType == kControllerTypeCmd646X )
    {
        rc = [self setupDMAList: startAddr client:client length:length fRead:fRead];
    }
    else
    {
        rc = [self setupDBDMA: startAddr client:client length:length fRead:fRead];
    }
    return rc;
}

-(ide_return_t) setupDBDMA:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead 
{
    uint		count;
    uint		addr;
    uint		physAddr;
    uint		len;
    uint		bytesLeft;
    uint		i;
    IOReturn		rc;
    uint                maxDescriptors;

    /*
     * This is for the benefit of - getTransferCount in case of a zero length transfer.
     */
    bzero( (unsigned char *)_ideDMACommands, sizeof(IODBDMADescriptor) );

    if ( length == 0 )
    {
       return IDER_SUCCESS;
    }  

    bytesLeft = length;
    count     = PAGE_SIZE - (startAddr & (PAGE_SIZE-1));
    addr      = startAddr;
 
    maxDescriptors = MAX_IDE_DESCRIPTORS - 1 - ((length & 1) ? 1 : 0);
   
    for ( i=0; i < maxDescriptors && bytesLeft; i++ )
    {
    	len = ( count > bytesLeft ) ? bytesLeft : count;

     	rc = IOPhysicalFromVirtual( client, trunc_page(addr), &physAddr );
        if ( rc != IO_R_SUCCESS )
        {
            IOLog("IDE Disk - IOPhysicalFromVirtual rc = %d\n\r", rc);
            return IDER_MEMFAIL;
        }
        physAddr = (physAddr & ~(PAGE_SIZE-1)) | (addr & (PAGE_SIZE-1));

        IOMakeDBDMADescriptor( &_ideDMACommands[i],
	  		       ((fRead) ? kdbdmaInputMore : kdbdmaOutputMore),
	  		       kdbdmaKeyStream0,
			       kdbdmaIntNever,
			       kdbdmaBranchNever,
			       kdbdmaWaitNever,
			       len,
			       physAddr );
        bytesLeft -= len;
        addr      += len;
        count      = PAGE_SIZE;
    }

    /*
     * Note: ATAPI always transfers even byte-counts. Send the extra byte to/from the bit-bucket
     *       if the requested transfer length is odd.
     */
    if ( length & 1 )
    {
        IOMakeDBDMADescriptor( &_ideDMACommands[i++],
	    		       ((fRead) ? kdbdmaInputMore : kdbdmaOutputMore),
	  		       kdbdmaKeyStream0,
			       kdbdmaIntNever,
			       kdbdmaBranchNever,
			       kdbdmaWaitNever,
			       1,
			       bitBucketPhysAddr );
    }


    if ( bytesLeft )
    {
        IOLog("IDE I/O request too large - Length = %d\n\r", length);
        return IDER_REJECT;
    }
 
    IOMakeDBDMADescriptor( &_ideDMACommands[i],
	  		   kdbdmaStop,
	  		   kdbdmaKeyStream0,
			   kdbdmaIntNever,
			   kdbdmaBranchNever,
			   kdbdmaWaitNever,
			   0,
			   0  );
 
   IOSetDBDMACommandPtr( _ideDMARegs, _ideDMACommandsPhys );
       
   return IDER_SUCCESS;
}           
 
-(ide_return_t) setupDMAList:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead 
{
    uint		count;
    uint		addr;
    uint		physAddr;
    uint		len;
    uint		bytesLeft;
    uint		i;
    IOReturn		rc;
    uint                maxDescriptors;
    ideDMAList_t	*ideDMAList;

    if ( length == 0 )
    {
       return IDER_SUCCESS;
    }  

    bytesLeft = length;
    count     = PAGE_SIZE - (startAddr & (PAGE_SIZE-1));
    addr      = startAddr;
 
    maxDescriptors = MAX_IDE_DESCRIPTORS * 2;
   
    ideDMAList = (ideDMAList_t *) _ideDMACommands;
    for ( i=0; i < maxDescriptors && bytesLeft; i++ )
    {
    	len = ( count > bytesLeft ) ? bytesLeft : count;

     	rc = IOPhysicalFromVirtual( client, trunc_page(addr), &physAddr );
        if ( rc != IO_R_SUCCESS )
        {
            IOLog("IDE Disk - IOPhysicalFromVirtual rc = %d\n\r", rc);
            return IDER_MEMFAIL;
        }
        physAddr = (physAddr & ~(PAGE_SIZE-1)) | (addr & (PAGE_SIZE-1));

        ideDMAList[i].start  = EndianSwap32( physAddr );
        ideDMAList[i].length = EndianSwap32( len );

        bytesLeft -= len;
        addr      += len;
        count      = PAGE_SIZE; 
    }

    if ( bytesLeft )
    {
        IOLog("IDE I/O request too large - Length = %d\n\r", length);
        return IDER_REJECT;
    }

    ideDMAList[i-1].length |= EndianSwap32( 0x80000000 );

    [[self deviceDescription] configWriteLong:0x74 value: _ideDMACommandsPhys];
       
    return IDER_SUCCESS;
}           


/*
 * This routine makes sure the DMA channel has shutdown at the end of an 
 * I/O request and obtains tha actual byte count transferred.
 */
- (uint) stopDBDMA
{
    uint	dmaStatus;
    uint        xferCount;
 
    dmaStatus = IOGetDBDMAChannelStatus( _ideDMARegs );
    
    if ( dmaStatus & (kdbdmaDead | kdbdmaActive) )
    {
        if ( IsPowerStar() )
        {
            [self fixOHareDMACorruption_1];
        }
        else
        {
            IODBDMAReset( _ideDMARegs );
        }
    }

    xferCount = [self getDBDMATransferCount];
    
    if ( dmaStatus & (kdbdmaDead | kdbdmaActive) )
    {
        if ( IsPowerStar() )
        {
            [self fixOHareDMACorruption_2];
        }
    }
    return xferCount;
}

/*
 * The OHare DMA controller and its predecessor Grand Central have a nasty habit of 
 * clobbering bytes prior to the data buffer if the buffer is not 16-byte aligned.
 *
 * To fix this we check for a non-aligned DMA read operation and save the bytes
 * prior to the start of the data buffer, and them restore them after the DMA channel
 * is shutdown.
 */

- (void) fixOHareDMACorruption_1
{
    IOReturn		rc;
    uint		i;
    uint		physAddr;
    uint		virtAddr;
    unsigned char	*p;
    BOOL		resetDone = NO;
    uint		dmaOp;
    unsigned char       saveUs[16];
    uint                len;

    do 
    {
        dmaOp = IOGetCCOperation(&_ideDMACommands[0])  >> 28; 
        if ( dmaOp != kdbdmaInputMore )
        {
            continue;
        }

        physAddr = IOGetCCAddress(&_ideDMACommands[0]);
        if ( !(physAddr & 0x0000000F) )
        {
            continue;
        }

        rc = IOMapPhysicalIntoIOTask( (vm_offset_t)   trunc_page(physAddr),
                                                      PAGE_SIZE,
                                      (vm_offset_t *) &virtAddr );
        if ( rc != IO_R_SUCCESS )
        {
            continue;
        }

        p =   (unsigned char *) ( (virtAddr & ~(PAGE_SIZE-1)) | (physAddr & 0xFF0) );
        len = (physAddr & 0x00F);

        for ( i=0; i < len; i++ )
        {
            saveUs[i] = p[i];
        }

        IODBDMAReset( _ideDMARegs );
        resetDone = YES;

        for ( i=0; i < len; i++ )
        {
            p[i] = saveUs[i];
        }

        IOUnmapPhysicalFromIOTask( (vm_offset_t)   trunc_page(virtAddr),
                                                   PAGE_SIZE );
    }
    while( 0 );     
 
    if ( resetDone == NO )
    {
        IODBDMAReset( _ideDMARegs );
    }
}    


/*
 * The OHare II controller has a bug where if there is a pending DMA write operation
 * that was not accepted by the device, it 'remembers' is and inserts an spurious 
 * DMA write to the device on the next write operation. This results in the data
 * for the next write operation to be shifted foward by two bytes.
 *
 */

- (void) fixOHareDMACorruption_2
{
    uint		dmaOp;

    dmaOp = IOGetCCOperation(&_ideDMACommands[0])  >> 28; 
    if ( dmaOp == kdbdmaOutputMore )
    {
        IOPanic("Disk(ata): IDE Hardware error - unable to recover\n\r");
    }
}
     
/*
 * Calculate the actual transfer count by reading back the DMA descriptors.
 */
- (uint) getDBDMATransferCount
{
    uint	i;
    uint        ccResult;
    uint        xferCount = 0;

    for ( i=0; i < MAX_IDE_DESCRIPTORS; i++ )
    {
        ccResult = IOGetCCResult( &_ideDMACommands[i] );
    
        if ( !(ccResult & kdbdmaStatusRun) )
        {
            break;
        } 
        xferCount += (IOGetCCOperation( &_ideDMACommands[i] ) & kdbdmaReqCountMask) - (ccResult & kdbdmaResCountMask); 
    }
    return xferCount;
}

@end

