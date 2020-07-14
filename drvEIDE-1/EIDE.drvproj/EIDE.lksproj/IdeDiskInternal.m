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
 * IdeDiskInternal.m - internal IDE disk methods. 
 *
 * HISTORY 
 *
 * 04-Mar-1996	 Rakesh Dubey at NeXT
 *	Modified so that no memory is allocated at run-time.
 *
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */
 
#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdeDiskInternal.h"
#import "IdeKernel.h"
#import <driverkit/kernelDiskMethods.h>
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/align.h>
#import <bsd/stdio.h>
#import <bsd/string.h>

IOReturn iderToIo(ide_return_t);

//#define DEBUG

@implementation IdeDisk(Internal)

/*
 * Print information about the disk and fill up the ideDriveName field. 
 */
-(void)printInfo:(ideIdentifyInfo_t *)ideIdentifyInfo unit:(unsigned int)unit
{
    int i;
    char name[50];
    char firmware[9];

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


    IOLog("%s: %s\n", [self name], name);

    /*
     * The drive name is limited to 32 characters and we don't want firmware
     * version in there. 
     */
    name[i+1] = '\0'; name[31] = '\0';
    strcpy(_ideDriveName, name);

    /*
     * Print drive geometry and how did we get it. 
     */
    IOLog("%s: CHS = %d/%d/%d ",
	    [self name], _ideInfo.cylinders, 
	    _ideInfo.heads, _ideInfo.sectors_per_trk);

    if ([_cntrlr isDiskGeometry:unit] == YES)
    	IOLog("(disk geometry)\n");
    else
    	IOLog("(bios geometry)\n");
	
    /*
     * Other diagnostic information. All of this can also be obtained via
     * /usr/etc/idemodes 
     */
#ifdef DEBUG
    IOLog("%s: PIO timing cycle: %d ns (mode %d).\n", [self name],
	    ideIdentifyInfo->pioDataTransferCyleTimingMode &
	    IDE_PIO_TIMING_MODE_MASK, 
	    [_cntrlr getTransferModeFromCycleTime:ideIdentifyInfo
	    	transferType:IDE_TRANSFER_PIO]);

    if (ideIdentifyInfo->capabilities & IDE_CAP_DMA_SUPPORTED)	{
        IOLog("%s: DMA timing cycle: %d ns (mode %d).\n", [self name],
		ideIdentifyInfo->dmaDataTransferCyleTimingMode &
		IDE_DMA_TIMING_MODE_MASK,
		[_cntrlr getTransferModeFromCycleTime:ideIdentifyInfo
		    transferType:IDE_TRANSFER_SW_DMA]);
    }
    if (ideIdentifyInfo->capabilities & IDE_CAP_LBA_SUPPORTED)	{
	IOLog("%s: LBA supported.\n", [self name]); 
    }
    if (ideIdentifyInfo->capabilities & IDE_CAP_IORDY_SUPPORTED)	{
	IOLog("%s: IORDY supported.\n", [self name]); 
    }

    if (ideIdentifyInfo->bufferType != 0)	{
	IOLog("%s: buffer type %d of %d sectors.\n", [self name],
		ideIdentifyInfo->bufferType, ideIdentifyInfo->bufferSize);
    }
#endif DEBUG
}

/*
 * Device-specific initialization. We just do enough here to do some I/O and
 * to find out if the requested drive is present. This function is "reusable"
 * for a given instance of IdeDisk; initResources must have been called
 * exactly once prior to any use of this method. Returns YES if drive is
 * present, else NO. 
 */
- (BOOL)ideDiskInit:(unsigned int)diskUnit target:(unsigned int)unit
{
    char dev_name[10];
    unsigned total_sectors;
    ideIdentifyInfo_t *ideIdentifyInfo;
#ifdef NO_ATA_RUNTIME_MEMORY_ALLOCATION
    int i;
    ideBuf_t *ideBuf;
#endif NO_ATA_RUNTIME_MEMORY_ALLOCATION


    queue_init(&_ioQueueDisk);
    queue_init(&_ioQueueNodisk);


#ifdef NO_ATA_RUNTIME_MEMORY_ALLOCATION

    /* Set up a queue of ideBufs */
    queue_init(&_ideBufQueue);
    _ideBufLock = [NXLock new];
    [_ideBufLock lock];

   
    for (i = 0; i < MAX_NUM_ATABUF; i++)	{
	ideBuf = &_ideBufPool[i];
	ideBuf->waitLock = [NXConditionLock alloc];
	[ideBuf->waitLock initWith:NO];
	queue_enter(&_ideBufQueue, ideBuf, ideBuf_t *, bufLink);
    }
    [_ideBufLock unlock];

    
    if (_ide_debug)	{
	IOLog("NO_ATA_RUNTIME_MEMORY_ALLOCATION\n");
    }

#endif NO_ATA_RUNTIME_MEMORY_ALLOCATION

    _driveNum = unit;

#ifdef DEBUG    
    IOLog("IDEDisk: ideDiskInit for disk %d target %d\n", diskUnit, unit);
#endif DEBUG
    
    /* Skip ATAPI devices. */
    if ([_cntrlr isAtapiDevice:unit] == YES) {
//		IOLog("IDEDisk: disk %d is ATAPI\n", unit);
		return NO;
    }

    _ideInfo = [_cntrlr getIdeDriveInfo:unit];
    if (_ideInfo.type == 0)	{
    //IOLog("IDEDisk: Bogus info for disk %d target %d.\n", diskUnit, unit);
	return NO;
    }

    sprintf(dev_name, "hd%d", diskUnit);
    [self setUnit:diskUnit];
    [self setName:dev_name];

    ideIdentifyInfo = [_cntrlr getIdeIdentifyInfo:unit];
    if (ideIdentifyInfo == NULL) {
		IOLog("%s: CHS = %d/%d/%d\n",
			[self name], _ideInfo.cylinders, 
			_ideInfo.heads, _ideInfo.sectors_per_trk);
		_ideDriveName[0] = '\0';
    } else {
		[self printInfo:ideIdentifyInfo unit:unit];
    }

    /* 
     * Cache optimal data transfer command.
     */

    if ([_cntrlr isDmaSupported:unit]) {
		IOLog("%s: using DMA transfers.\n", [self name]);
		_ideReadCommand = IDE_READ_DMA;
		_ideWriteCommand = IDE_WRITE_DMA;
    } else if ([_cntrlr isMultiSectorAllowed:_driveNum]) {
		IOLog("%s: using multisector (%d) transfers.\n", 
			[self name], [_cntrlr getMultiSectorValue:unit]);
        _ideReadCommand = IDE_READ_MULTIPLE;
        _ideWriteCommand = IDE_WRITE_MULTIPLE;
    } else {
		IOLog("%s: using single sector transfers.\n", [self name]);
		_ideReadCommand = IDE_READ;
		_ideWriteCommand = IDE_WRITE;
    }

    if ([self initIdeDrive] != IO_R_SUCCESS) {
		return NO;
    }

    total_sectors = _ideInfo.total_sectors;

    [self setRemovable:NO];
    [self setBlockSize:_ideInfo.bytes_per_sector];
    [self setDiskSize:total_sectors];
    [self setFormattedInternal:YES];
    [self setLastReadyState: IO_Ready];

    if (_ideDriveName[0] == '\0')	{
		sprintf(_ideDriveName, "IDE Drive Type %d", _ideInfo.type);
    }
    [self setDriveName:_ideDriveName];

    [super init];

    return YES;
}

/*
 * One-time only initialization. We have only one thread per disk since ATA
 * disks do not support command queueing. 
 */
#ifdef	DEBUG
void *ideThreadPtr;
#endif	DEBUG

- initResources	: controller
{
    _cntrlr = controller;
    _ioQLock = [NXConditionLock alloc];
    [_ioQLock initWith:NO_WORK_AVAILABLE];
#ifdef	DEBUG
    ideThreadPtr = IOForkThread((IOThreadFunc)ideThread, self);
#else	DEBUG
    IOForkThread((IOThreadFunc)ideThread, self);
#endif	DEBUG
    return self;
}

/*
 * Free up local resources. 
 */
- free
{
    /*
     * First kill the I/O thread, then free alloc'd instance variables. 
     */
    ideBuf_t *ideBuf;
    int i;

    ideBuf = [self allocIdeBuf:NULL];
    ideBuf->command = IDEC_THREAD_ABORT;
    ideBuf->buf = NULL;
    ideBuf->needsDisk = 0;
    ideBuf->oneWay = 0;
    [self enqueueIdeBuf:ideBuf];
    
    [self freeIdeBuf:ideBuf];
    [_ioQLock free];
    
#ifdef NO_ATA_RUNTIME_MEMORY_ALLOCATION
    if (_ideBufLock)
	[_ideBufLock free];
    
    for (i = 0; i < MAX_NUM_ATABUF; i++) {
	if (_ideBufPool[i].waitLock)
	    [_ideBufPool[i].waitLock free];
    }
#endif NO_ATA_RUNTIME_MEMORY_ALLOCATION

    return ([super free]);
}

/*
 * Allocate and free IdeBuf_t's.
 */

#ifdef NO_ATA_RUNTIME_MEMORY_ALLOCATION
- (ideBuf_t *) allocIdeBuf:(void *)pending
{
    ideBuf_t *ideBuf;
    id waitLock;

    while (1)	{
    	[_ideBufLock lock];
	if (!queue_empty(&_ideBufQueue))
	    break;
	[_ideBufLock unlock];
	IOSleep(10);		// the system is overloaded
    }
    
    ASSERT(queue_empty(&_ideBufQueue) != 0);
    ideBuf = (ideBuf_t *) queue_first(&_ideBufQueue);
    ASSERT(ideBuf != 0);
    queue_remove(&_ideBufQueue, ideBuf, ideBuf_t *, bufLink);

    waitLock = ideBuf->waitLock;
    bzero(ideBuf, sizeof(ideBuf_t));
    ideBuf->waitLock = waitLock;
    [ideBuf->waitLock initWith:NO];
    
    if (pending != NULL)
	ideBuf->pending = pending;
	
    [_ideBufLock unlock];
    return (ideBuf);
}

- (void)freeIdeBuf:(ideBuf_t *) ideBuf
{
    [_ideBufLock lock];
    queue_enter(&_ideBufQueue, ideBuf, ideBuf_t *, bufLink);
    ASSERT(queue_empty(&_ideBufQueue) != 0);
    [_ideBufLock unlock];
}

#else NO_ATA_RUNTIME_MEMORY_ALLOCATION

- (ideBuf_t *) allocIdeBuf:(void *)pending
{
    ideBuf_t *ideBuf = IOMalloc(sizeof(ideBuf_t));

    bzero(ideBuf, sizeof(ideBuf_t));
    if (pending == NULL) {
	ideBuf->waitLock = [NXConditionLock alloc];
	[ideBuf->waitLock initWith:NO];
    } else
	ideBuf->pending = pending;
    return (ideBuf);
}

- (void)freeIdeBuf:(ideBuf_t *) ideBuf
{
    if (ideBuf->waitLock) {
	[ideBuf->waitLock free];
    }
    IOFree(ideBuf, sizeof(ideBuf_t));
}
#endif NO_ATA_RUNTIME_MEMORY_ALLOCATION

- (IOReturn) ideXfrIoReq:(ideIoReq_t *)ideIoReq
{
    ideBuf_t *ideBuf;
    IOReturn rtn;

    ideBuf = [self allocIdeBuf:NULL];
    ideBuf->command = IDEC_IOREQ;
    ideBuf->ideIoReq = ideIoReq;
    ideBuf->block = 0;
    ideBuf->blockCnt = 0;
    ideBuf->buf = 0;
    ideBuf->client = 0;	/* it picked up from ideIoReq	*/
    ideBuf->needsDisk = 1;
    ideBuf->bytesXfr = 0;
    ideBuf->oneWay = 0;
    rtn = [self enqueueIdeBuf:ideBuf];
    [self freeIdeBuf:ideBuf];

    return (rtn);
}

- (IOReturn) initIdeDrive
{
    IOReturn rtn;
    ideBuf_t *ideBuf;

    ideBuf = [self allocIdeBuf:NULL];
    ideBuf->command = IDEC_INIT;
    ideBuf->block = 0;
    ideBuf->blockCnt = 0;
    ideBuf->buf = 0;
    ideBuf->client = 0;
    ideBuf->needsDisk = 1;
    ideBuf->bytesXfr = 0;
    ideBuf->oneWay = 0;
    rtn  = [self enqueueIdeBuf:ideBuf];
    [self freeIdeBuf:ideBuf];

    return(rtn);
}

/*
 * Common read/write routine.
 */
- (IOReturn) deviceRwCommon:(IdeCmd_t) command
		    block:(u_int) deviceBlock
		    length:(u_int) length
		    buffer:(void *)buffer
		    client:(vm_task_t) client
		    pending:(void *)pending
		    actualLength:(u_int *) actualLength
{
    ideBuf_t *ideBuf;
    IOReturn rtn;
    u_int   blocksReq;
    u_int   block_size;
    u_int   dev_size;

    rtn = [self isDiskReady:NO];

    switch (rtn) {
		case IO_R_SUCCESS:
			break;
		case IO_R_NO_DISK:
			return (rtn);
		default:
			IOLog("%s deviceRwCommon: bogus return from isDiskReady (%s)\n",
				[self name], [self stringFromReturn:rtn]);
			return (rtn);
    }

    block_size = [self blockSize];
    dev_size = [self diskSize];

    if (length % block_size) {
		return (IO_R_INVALID);
    }
    blocksReq = length / block_size;
    if ((deviceBlock + blocksReq) > dev_size) {
		if (deviceBlock >= dev_size) {
			return (IO_R_INVALID_ARG);
		}
		blocksReq = dev_size - deviceBlock;
    }
    ideBuf = [self allocIdeBuf:pending];
    ideBuf->command = command;
    ideBuf->block = deviceBlock;
    ideBuf->blockCnt = blocksReq;
    ideBuf->buf = buffer;
    ideBuf->client = client;
    ideBuf->needsDisk = 1;
    ideBuf->bytesXfr = 0;
    ideBuf->oneWay = 0;

    rtn = [self enqueueIdeBuf:ideBuf];

    if (pending == NULL) {
		/*
		 * Sync I/O. 
		 */
		*actualLength = ideBuf->bytesXfr;
		[self freeIdeBuf:ideBuf];
    }
    return (rtn);
}

/*
 * -- Enqueue an IdeBuf_t on ioQueue<Disk,Nodisk>
 * -- wake up the I/O thread
 * -- wait for I/O complete (if ideBuf->pending == NULL)
 *
 * All I/O goes thru here; this is the last method called by exported methods
 * before the I/O thread takes over. 
 */
- (IOReturn) enqueueIdeBuf:(ideBuf_t *) ideBuf
{
    queue_head_t *q;

    ideBuf->status = IO_R_INVALID;
    [_ioQLock lock];
    if (ideBuf->needsDisk)
	q = &_ioQueueDisk;
    else
	q = &_ioQueueNodisk;
    queue_enter(q, ideBuf, ideBuf_t *, link);
    [_ioQLock unlockWith:WORK_AVAILABLE];

    if (ideBuf->pending != NULL)
	return (IO_R_SUCCESS);

    /*
     * Wait for I/O complete if not an async command. 
     */

    if (ideBuf->oneWay) {
	return IO_R_SUCCESS;
    }
    [ideBuf->waitLock lockWhen:YES];
    [ideBuf->waitLock unlock];

    return (ideBuf->status);
}


/*
 * Either wake up the thread which is waiting on the ideCmdBuf, or send an 
 * ioComplete back to client. ideCmdBuf->status must be valid.
 */
- (void)ideIoComplete:(ideBuf_t *) ideBuf
{
    if (ideBuf->pending) {
	[self completeTransfer:ideBuf->pending
		    withStatus:ideBuf->status
		    actualLength:ideBuf->bytesXfr];
	[self freeIdeBuf:ideBuf];
    } else {
	/*
	 * Sync I/O. Just wake up the waiting thread. 
	 */
	[ideBuf->waitLock lock];
	[ideBuf->waitLock unlockWith:YES];
    }

}

/*
 * Main command dispatch method. 
 */
- (void)ideCmdDispatch:(ideBuf_t *)ideBuf
{
    IOReturn rtn = IO_R_SUCCESS;
    ideBuf_t *abortBuf;

    switch (ideBuf->command) {
    
      case IDEC_IOREQ:
	[_cntrlr ideExecuteCmd:ideBuf->ideIoReq ToDrive:_driveNum];

	break;

      case IDEC_READ:
      case IDEC_WRITE:
	rtn = [self ideRwCommon:ideBuf];

	break;

      case IDEC_ABORT:

	/*
	 * Each I/O pending in the needsDisk queue must be aborted. 
	 */
	[_ioQLock lock];
	while (!queue_empty(&_ioQueueDisk)) {
	    abortBuf = (ideBuf_t *) queue_first(&_ioQueueDisk);
	    queue_remove(&_ioQueueDisk,
			 abortBuf,
			 ideBuf_t *,
			 link);
	    [_ioQLock unlock];
	    abortBuf->status = IO_R_NO_DISK;
	    if (abortBuf->ideIoReq)
		abortBuf->ideIoReq->status = IDER_VOLUNAVAIL;
	    [self ideIoComplete:abortBuf];
	    [_ioQLock lock];
	}
	[_ioQLock unlock];

	break;

      case IDEC_THREAD_ABORT:

	/*
	 * First give I/O complete before we die. 
	 */
	ideBuf->status = IO_R_SUCCESS;
	[self ideIoComplete:ideBuf];
	IOExitThread();

      case IDEC_INIT:
#if 0
	/* Called once during initialization. 	*/
	if ([_cntrlr initIdeDriveLowLevel] == IDER_SUCCESS)
	    rtn = IO_R_SUCCESS;
	else
	    rtn = IO_R_IO;
#endif 0
        rtn = IO_R_SUCCESS;
	break;

      default:
	IOLog("%s: Bogus ideBuf->command 0x%0x in ideCmdDispatch\n", 
		[self name], ideBuf->command);
	IOPanic("ideThread");
    }

    /*
     * I/O complete the command if appropriate. 
     */
    if (ideBuf->oneWay) {

	[self freeIdeBuf:ideBuf];

	return;
    }
    ideBuf->status = rtn;

    [self ideIoComplete:ideBuf];
    return;
}

/*
 * Common r/w routine. The following ideBuf fields are required:
 *	block
 *	blockCnt
 *	buf
 *	client
 *	pending
 *
 * block and blockCnt are assumed to be already adjusted for overflow.
 */
- (IOReturn) ideRwCommon:(ideBuf_t *)ideBuf
{
    int     currentBlock = ideBuf->block;	/* start block, current
						 * segment */
    int     	currentBlockCnt;	/* block count, current segment */
    int     	blocksToGo = ideBuf->blockCnt;
    char   	*currentBuf = ideBuf->buf;
    ideIoReq_t ideIoReq;
    IOReturn 	rtn;
    unsigned 	int block_size = _ideInfo.bytes_per_sector;
    BOOL    	readFlag = (ideBuf->command == IDEC_READ) ? YES : NO;
    int     	blocksMoved;
    ns_time_t 	start_time;

    IOGetTimestamp(&start_time);

    while (blocksToGo) {

	/*
	 * Set up controller command block for current segment. 
	 */
	currentBlockCnt = ((blocksToGo > MAX_BLOCKS_PER_XFER) ?
			   MAX_BLOCKS_PER_XFER : blocksToGo);

	ideIoReq.cmd = readFlag ? _ideReadCommand : _ideWriteCommand;

	ideIoReq.addr = currentBuf;
	ideIoReq.blkcnt = currentBlockCnt;
	ideIoReq.block = currentBlock;

	/*
	 * Note we're compiling MACH_USER_API, hence the cast... 
	 */
	ideIoReq.map = (struct vm_map *) ideBuf->client;

	rtn = [_cntrlr ideExecuteCmd:&ideIoReq ToDrive:_driveNum];

	blocksMoved = ideIoReq.blocks_xfered;
	blocksToGo -= blocksMoved;
	currentBlock += blocksMoved;
	currentBuf += ideIoReq.blocks_xfered * block_size;

	/*
	 * Handle errors. 
	 */

	if (rtn) {
	    /*
	     * This is a colossal error, which should never happen... 
	     */
	    ideIoReq.status = IDER_REJECT;
	}
	switch (ideIoReq.status) {
	  case IDER_SUCCESS:
	    break;
	  case IDER_TIMEOUT:
	  case IDER_MEMALLOC:
	  case IDER_MEMFAIL:
	  case IDER_REJECT:
	  case IDER_BADDRV:
	  case IDER_CMD_ERROR:
	  case IDER_VOLUNAVAIL:
	  case IDER_SPURIOUS:
	    goto done;
	    break;
	  default:

	    /*
	     * Non-retriable errors. 
	     */
	    [self logRwErr:"FATAL" block:currentBlock status:ideIoReq.status
		    readFlag:readFlag];
	    goto done;
	    break;

	} /* switch status */
    } /* while blocksToGo */
done:

    /*
     * Finished. Update client's status and log statistics. 
     */
    ideBuf->bytesXfr = (ideBuf->blockCnt - blocksToGo) * block_size;
    rtn = ideBuf->status = iderToIo(ideIoReq.status);
    if (ideIoReq.status != IO_R_SUCCESS) {

	if (readFlag) {
	    [self incrementReadErrors];
	} else {
	    [self incrementWriteErrors];
	}

    } else {
	ns_time_t end_time;

	/* no "latency" measurement...	*/
	ns_time_t delta;

	IOGetTimestamp(&end_time);
	delta = end_time - start_time;
	if (readFlag) {

	    [self addToBytesRead:ideBuf->bytesXfr
		    totalTime:delta
		    latentTime:0];
	} else {
	    [self addToBytesWritten:ideBuf->bytesXfr
		    totalTime:delta
		    latentTime:0];
	}
    }

    return (rtn);
}

- (void)logRwErr : (const char *)errType	// e.g., "RECALIBRATING"
		   block : (int)block
		   status : (ide_return_t)status
		   readFlag : (BOOL)readFlag
{

    IOLog("%s: Sector %d cmd = %s; %s: %s\n",
	    [self name], block, readFlag ? "Read" : "Write",
	    IOFindNameForValue(status, iderValues), errType);

}

/*
 * Unlock ioQLock, updating condition variable as appropriate.
 */
- (void)unlockIoQLock
{
    int     queue_state;
    IODiskReadyState lastReady = [self lastReadyState];

    /*
     * There's still work to do when: 
     * -- ioQueueNodisk non-empty, or 
     * -- ioQueueDisk non-empty and we have a disk. 
     */

    if ((!queue_empty(&_ioQueueNodisk)) ||
	((!queue_empty(&_ioQueueDisk)) && (lastReady != IO_NoDisk))) {
	queue_state = WORK_AVAILABLE;
    } else
	queue_state = NO_WORK_AVAILABLE;
    [_ioQLock unlockWith:queue_state];
}


/*
 * I/O thread. Each one of these sits around waiting for work to do on 
 * ioQueue; when something appears, the thread grabs it and disptahes it.
 */
 
volatile void ideThread(IdeDisk *idisk)
{
    ideBuf_t *ideBuf;
    queue_head_t *q;

    while (1) {

	/*
	 * Wait for some work to do. 
	 */
	[idisk->_ioQLock lockWhen:WORK_AVAILABLE];

	/*
	 * Service all requests which do not require a disk. 
	 */
	q = &idisk->_ioQueueNodisk;
	while (!queue_empty(q)) {
	    ideBuf = (ideBuf_t *) queue_first(q);
	    queue_remove(q, ideBuf, ideBuf_t *, link);
	    [idisk->_ioQLock unlock];
	    ASSERT(ideBuf->needsDisk == 0);
	    [idisk ideCmdDispatch:ideBuf];
	    [idisk->_ioQLock lock];
	}

	/*
	 * Now service all requests which require a disk, as long as we have
	 * one. 
	 */

	q = &idisk->_ioQueueDisk;
	ASSERT([idisk lastReadyState] == IO_Ready);
	while ((!queue_empty(q)) &&
	       ([idisk lastReadyState] == IO_Ready)) {
	    ideBuf = (ideBuf_t *) queue_first(q);
	    queue_remove(q, ideBuf, ideBuf_t *, link);
	    [idisk->_ioQLock unlock];
	    ASSERT(ideBuf->needsDisk == 1);
	    [idisk ideCmdDispatch:ideBuf];
	    [idisk->_ioQLock lock];
	}

	[idisk unlockIoQLock];
    }

    /* NOT REACHED */
}

IOReturn iderToIo(ide_return_t ider)
{
    switch  (ider) {
		case IDER_SUCCESS:
			return (IO_R_SUCCESS);
		case IDER_TIMEOUT:
		case IDER_CNTRL_REJECT:
		case IDER_CMD_ERROR:
		case IDER_SPURIOUS:
			return (IO_R_IO);
		case IDER_MEMALLOC:
			return (IO_R_NO_MEMORY);
		case IDER_MEMFAIL:
			return (IO_R_VM_FAILURE);
		case IDER_REJECT:
			return (IO_R_UNSUPPORTED);
		case IDER_BADDRV:
			return (IO_R_NO_DEVICE);
		case IDER_VOLUNAVAIL:
			return (IO_R_NO_DISK);
		default:
			return (IO_R_INTERNAL);
    }
}

@end

