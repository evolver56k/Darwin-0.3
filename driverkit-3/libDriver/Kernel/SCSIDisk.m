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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * SCSIDisk.m - Implementation of exported methods of SCSI Disk device class. 
 *
 * HISTORY
 *
 * 30-Jul-98	Dan Markarian at Apple. See Radar 2260508, 2258860. If the
 *		SCSIDisk driver attempts to probe a non-zero LUN and a
 *		target exists on LUN 0 that is actively doing (tagged) I/O,
 *		the target may abort all commands with an overlapped command
 *		error, causing the original command to fail without completion.
 *		Suggested work-arounds (for MacOS X Server, release 1): probe
 *		only LUN 0. If a target is discovered, reserve all LUNs for
 *		that target. This code must be revisited for IOKit.
 *
 * 11-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#define MACH_USER_API	1
#undef	KERNEL_PRIVATE

#import <bsd/sys/types.h>
#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import <driverkit/SCSIDiskPrivate.h>
#import <driverkit/SCSIDiskTypes.h>
#import <driverkit/SCSIDisk.h>	
#import <driverkit/SCSIDiskKern.h>
#import <driverkit/SCSIStructInlines.h>
#import <bsd/dev/scsireg.h>	
#import <driverkit/xpr_mi.h>
#import <kernserv/prototypes.h>
#import <mach/mach_interface.h>
#import <driverkit/kernelDiskMethods.h>
#import <machkit/NXLock.h>

#ifdef	DEBUG
#define SCSI_SA_TEST	0
#else	DEBUG
#define SCSI_SA_TEST	0
#endif	DEBUG

/*
 * FIXME - ensure that diskUnit doesn't exceed NUM_SD_DEV.
 */
static int diskUnit = 0;

@implementation SCSIDisk

+ (IODeviceStyle)deviceStyle
{
	return IO_IndirectDevice;
}

/*
 * The protocol we need as an indirect device.
 */
static Protocol *protocols[] = {
	@protocol(IOSCSIControllerExported),
	nil
};

+ (Protocol **)requiredProtocols
{
	return protocols;
}


+ initialize
{
	if(self == [SCSIDisk class]) {
		sd_init_idmap();
	}
	return [super initialize];
}  

/*
 * probe is invoked at load time. It determines what devices are on the
 * bus and alloc's and init:'s an instance of this class for each one.
 */
+ (BOOL)probe : deviceDescription;
{
	char Target, Lun;
	sdInitReturn_t irtn = SDR_ERROR;
	SCSIDisk *diskId = nil;
	IODevAndIdInfo *idMap = sd_idmap();
	id controllerId = [deviceDescription directDevice];
	BOOL brtn = NO;
	SCSIDisk *diskIdArray[SCSI_NLUNS];
	int nLuns;
	/*
	 * Radar 2005639
	 */
	unsigned int paramValue[1];
	unsigned int count = 1;
	IOReturn ioReturn;
	
/* asm volatile("int3");  */

#if hppa
	/* search from the top down on hp since that was done before */
	for(Target=[controllerId numberOfTargets]-1; Target>=0; Target--)
#else
	for(Target=0; Target<[controllerId numberOfTargets]; Target++)
#endif
	{

#if 0  // Prior to Radar Fix #2260508
		for(Lun=nLuns=0; Lun<SCSI_NLUNS; Lun++) {
#else  // Radar Fix #2260508
                for(Lun=nLuns=0; Lun<1; Lun++) {
#endif // Radar Fix #2260508
			if(diskId == nil) {
				/*
				 * Create an instance, do some basic 
				 * initialization. Set up a default 
				 * device name for error reporting during
				 * initialization.
				 */
				diskId = [SCSIDisk alloc];
				[diskId setName:"SCSIDisk"];
				/*
				 * Radar 2005639. If the controller implements
				 * the getIntValues APPLE_MAX_THREADS selector,
				 * instantiate with the specified number of
				 * of threads, otherwise let the diskID
				 * instantiate itself with its default number.
				 */
				ioReturn = [controllerId getIntValues
							: &paramValue[0]
					forParameter	: APPLE_MAX_THREADS
					count		: &count];
				if (ioReturn == IO_R_SUCCESS && count == 1) {
#if 0 /* Radar 2291688: supress IOLog */
				    IOLog("%s: %s requests %d threads\n",
					[self name],
					[controllerId name],
					paramValue[0]
				    );
#endif
				    [diskId initResourcesWithThreadCount
						: paramValue[0]];
				}
				else {
#if 0 /* Radar 2291688: supress IOLog */
				    IOLog(
		"%s: %s does not support %s selector, using default\n",
					[self name],
					[controllerId name],
					APPLE_MAX_THREADS
				    );
#endif
				    [diskId initResources];
				}
				[diskId setDevAndIdInfo:&(idMap[diskUnit])];
			}
			if([controllerId reserveTarget:Target
			    lun:Lun
			    forOwner:diskId]) {
			 	/*
				 * Someone already has this one.
				 */
				continue;   
			}
			diskId->_isReserved = 1;
#if 1  // Radar Fix #2260508
	                /*
	                 * A valid target at LUN 0 has been reserved: reserve
                         * all non-zero LUNs as well.   Note that _controller
                         * must be set before calling this next method, so go
                         * ahead and set it ourselves since the diskId object
                         * might be newly allocated (and yet-uninitialized).
	                 */
                        diskId->_controller = controllerId;
			[diskId reserveNonZeroLunsOnTarget:Target];
#endif // Radar Fix #2260508
			irtn = [diskId SCSIDiskInit:(int)diskUnit
				targetId:Target
				lun:Lun
				controller:controllerId];
			[diskId setDeviceDescription:deviceDescription];
			switch(irtn) {
			    case SDR_GOOD:
				/*
				 * Postpone registering this device
				 * until we have looked at all other LUNs.
				 * This prevents I/O to multiple LUNs
				 * while we are probing, which is not handled
				 * well by some devices.
				 */
				diskIdArray[nLuns++] = diskId;
				diskUnit++;
				diskId = nil;
				brtn = YES;
				break;				
				
			    default:
#if 1  // Radar Fix #2260508
                                /*
                                 * LUN 0 will be unreserved: unreserve all
                                 * non-zero LUNs first.   Note _controller
                                 * is already set at this point.
                                 */
                                [diskId releaseNonZeroLunsOnTarget:Target];
#endif // Radar Fix #2260508
			        [controllerId releaseTarget:Target
			    		lun:Lun
			    		forOwner:diskId];
				diskId->_isReserved = 0;

				if(irtn == SDR_SELECTTO) {
					/*
					 * Skip the rest of the luns on 
					 * this target.
					 */
					goto nextTarget;
				}
				/* 
				 * else try next lun.
				 */
			}
		}	/* for lun */
nextTarget:
		/* Now we have looked at all luns. */
		for (Lun=0; Lun < nLuns; Lun++) {
			SCSIDisk *thisId = diskIdArray[Lun];
			/*
			 * All right! Have IODisk superclass take 
			 * care of the rest.
			 */
			[thisId setDeviceKind:"SCSIDisk"];
			[thisId setIsPhysical:YES];
			[thisId registerDevice];
			thisId->_isRegistered = 1;
		}
		continue;
	}		/* for target */
	
	/*
	 * Free up leftover owner and id. At this point, diskId does NOT have
	 * a target/lun reserved.
	 */
	if(diskId) {
		[diskId free];
	}
	
#if	SCSI_SA_TEST
	diskTest("sd0");
#endif	SCSI_SA_TEST
	return brtn;
}

/*
 * IODiskReadingAndWriting protocol methods.
 */ 
- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength 
				  client : (vm_task_t)client
{
	IOReturn rtn;
	
	xpr_sd("%s read: offset 0x%x length 0x%x\n",
		[self name], offset, length, 4,5);
	rtn = [self deviceRwCommon : SDOP_READ
		  block : offset
		  length : length 
		  buffer : buffer
		  client: client
		  pending : NULL
		  actualLength : actualLength];
	xpr_sd("%s read: RETURNING %s\n", [self name],
		[self stringFromReturn:rtn], 3,4,5);
	return(rtn);
}

- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
	IOReturn rtn;
	
	xpr_sd("%s readAsync: offset 0x%x length 0x%x\n",
		[self name], offset, length, 4,5);
	rtn = [self deviceRwCommon : SDOP_READ
		  block : offset
		  length : length 
		  buffer : buffer
		  client : client
		  pending : (void *)pending
		  actualLength : NULL];
	xpr_sd("%s readAsync: RETURNING %s\n", [self name],
		[self stringFromReturn:rtn], 3,4,5);
	return(rtn);
}	

- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength 
				  client : (vm_task_t)client
{
	IOReturn rtn;
	
	xpr_sd("%s write: offset 0x%x length 0x%x\n",
		[self name], offset, length, 4,5);
	rtn = [self deviceRwCommon : SDOP_WRITE
		  block : offset
		  length : length 
		  buffer : buffer
		  client: client
		  pending : NULL
		  actualLength : actualLength];
	xpr_sd("%s deviceWrite: RETURNING %s\n", [self name],
		[self stringFromReturn:rtn], 3,4,5);
	return(rtn);
}				  	

- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
	IOReturn rtn;
	
	xpr_sd("%s writeAsync: offset 0x%x length 0x%x\n",
		[self name], offset, length, 4,5);
	rtn = [self deviceRwCommon : SDOP_WRITE
		  block : offset
		  length : length 
		  buffer : buffer
		  client : client
		  pending : (void *)pending
		  actualLength : NULL];
	xpr_sd("%s writeAsync: RETURNING %s\n", [self name],
		[self stringFromReturn:rtn], 3,4,5);
	return(rtn);
}


- (void) synchronizeCache
{
	IOSCSIRequest scsiReq;
	cdb_10_t *cdbp = &scsiReq.cdb.cdb_c10;
	sdBuf_t *sdBuf;
	
	bzero(&scsiReq, sizeof(IOSCSIRequest));
	scsiReq.target = _target;
	scsiReq.lun = _lun;
	scsiReq.timeoutLength = SD_TIMEOUT_SIMPLE;
	scsiReq.disconnect = 1;
	
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_CDB_WRITE;
	sdBuf->scsiReq = &scsiReq;
	sdBuf->retryDisable = 1;
	sdBuf->needsDisk = 0;
	
	/* Zeroing the lba and count fields will cause the drive to
	 * flush everything. We should be able to zero these fields
	 * explicitly, but for some reason the 10-byte commands don't
	 * have the proper field definitions. 
	 */
	bzero(cdbp,10);
	
	cdbp->c10_opcode = 0x35;
	cdbp->c10_lun = _lun;

	[self enqueueSdBuf:sdBuf];

	[self freeSdBuf:sdBuf];
}

- (IODiskReadyState)updateReadyState
{
	IOSCSIRequest scsiReq;
	cdb_6_t *cdbp = &scsiReq.cdb.cdb_c6;
	sdBuf_t *sdBuf;
	IODiskReadyState readyState;
	esense_reply_t senseReply;
	
	xpr_sd("%s updateReadyState\n", [self name], 2,3,4,5);

	if( NO == _isReserved) {
            return([self lastReadyState]);
	}

	bzero(&scsiReq, sizeof(IOSCSIRequest));
	scsiReq.target = _target;
	scsiReq.lun = _lun;
	scsiReq.timeoutLength = SD_TIMEOUT_SIMPLE;
	scsiReq.disconnect = 1;
	
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_CDB_WRITE;
	sdBuf->scsiReq = &scsiReq;
	sdBuf->retryDisable = 1;
	sdBuf->needsDisk = 0;
	
	cdbp->c6_opcode = C6OP_TESTRDY;
	cdbp->c6_lun = _lun;
	
	[self enqueueSdBuf:sdBuf];

	/*
	 * FIXME - how to distinguish not ready from no disk???
	 */
	switch(scsiReq.driverStatus) {
	    case SR_IOST_GOOD:
	    	readyState = IO_Ready;
		break;
	    default:
	    	readyState = IO_NotReady;
		break;
		
	}
	xpr_sd("%s updateReadyState: DONE; state = %s\n", 
		[self name], 
		IOFindNameForValue(readyState, readyStateValues), 3,4,5);
	[self freeSdBuf:sdBuf];
	return(readyState);
}

/*
 * IOPhysicalDiskMethods protocol methods.
 */
- (IOReturn) ejectPhysical
{
	return [self scsiStartStop:SS_EJECT inhibitRetry:NO];
}


/*
 * Get physical parameters (dev_size, block_size, etc.) from new disk.
 */
- (IOReturn)updatePhysicalParameters
{
	capacity_reply_t capacityData;
	mode_sel_data_t	 modeData;
	sc_status_t rtn;
	int block;
	void *dataBuf;
	void *freePtr;
	unsigned freeCnt;
	u_int block_size;
	BOOL wp = NO;
	int i;
	
	rtn = [self updateReadyState];
	if(rtn) {
		return IO_R_NOT_READY;
	}
	rtn = [self sdReadCapacity:&capacityData];
	if(rtn) {
		return(IO_R_IO);
	}
	[self setDiskSize:scsi_lastlba(&capacityData) + 1];
	[self setBlockSize:scsi_blklen(&capacityData)];
	
	/*
	 * Try reading a block to see if disk is formatted.
	 */
	block_size = [self blockSize];
	dataBuf = [_controller allocateBufferOfLength:block_size
			actualStart:&freePtr
			actualLength:&freeCnt];
	block = 10;
	for(i=0; i<5; i++) {
		rtn = [self sdRawRead:block blockCnt:1 buffer:dataBuf];
		if(rtn == IO_R_SUCCESS) {
			break;
		}
		block += 10;
	}
	if(rtn == IO_R_SUCCESS) 
		[self setFormattedInternal:1];
	else
		[self setFormattedInternal:0];

	IOFree(freePtr, freeCnt);
	
	/*
	 * Mark CD-ROMs as write-protected; use a mode sense for the others.
	 */

	if(_inquiryDeviceType == DEVTYPE_CDROM)
		wp = YES;
	else {
		bzero(&modeData, sizeof(mode_sel_data_t));
		rtn = [self sdModeSense:&modeData];

		if (modeData.msd_header.msh_wp)
			wp = YES;
	}

	[self setWriteProtected: wp];

	return(IO_R_SUCCESS);
}

/*
 * Called by volCheck thread when WS has told us that a requested disk is
 * not present. Pending I/Os which require a disk to be present must be 
 * aborted.
 */
- (void)abortRequest
{
	sdBuf_t *sdBuf;
	
	xpr_sd("%s: abortRequest\n", [self name], 2,3,4,5);
	
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_ABORT;
	sdBuf->needsDisk = 0;
	[self enqueueSdBuf:sdBuf];
	xpr_sd("%s abortRequest: done %s\n", [self name], 2,3,4,5);
	[self freeSdBuf:sdBuf];
	return;
}

/*
 * Called by the volCheck thread when a transition to "ready" is detected.
 * Pending I/Os which require a disk may proceed. All we have to do is 
 * wakeup the I/O threads which are waiting for something to show up 
 * in an I/O queue.
 */
- (void)diskBecameReady
{
	xpr_sd("diskBecameReady: %s\n", [self name], 2,3,4,5);
	[_ioQLock lock];
	[_ioQLock unlockWith:WORK_AVAILABLE];
}

/*
 * Inquire if disk is present; if not, and 'prompt' is YES, ask for it. 
 * Returns IO_R_NO_DISK if:
 *    prompt YES, disk not present, and user cancels request for disk.
 *    prompt NO, disk not present.
 * Else returns IO_R_SUCCESS.
 */
- (IOReturn)isDiskReady	: (BOOL)prompt
{
	sdBuf_t *sdBuf;
	IOReturn rtn;
	
	xpr_sd("%s: diskBecameReady\n", [self name], 2,3,4,5);

	if( NO == _isReserved)
		return(IO_R_NO_DISK);

	if([self lastReadyState] == IO_Ready) {
		/*
		 * This one's easy...
		 */
		return(IO_R_SUCCESS);
	}
	if(!prompt) {
		return(IO_R_NO_DISK);
	}
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_PROBEDISK;
	sdBuf->needsDisk = 1;
	rtn = [self enqueueSdBuf:sdBuf];
	xpr_sd("%s diskBecameReady: returning %s\n", [self name], 
		[self stringFromReturn:rtn], 3,4,5);
	[self freeSdBuf:sdBuf];
	return(rtn);
}	

/*
 * We have to override IODisk's setLastReadyState: so we know when to 
 * clear our local ejectPending flag.
 */
 - (void)setLastReadyState : (IODiskReadyState)readyState
 {
 	xpr_sd("sd setLastReadyState\n", 1,2,3,4,5);
 	if(_ejectPending && (readyState != IO_Ejecting))
		_ejectPending = 0;
	[super setLastReadyState:readyState];
 }

/*
 * Exported methods unique to the SCSIDisk class.
 * Note caller must provide well-aligned DMA buffers.
 */
- (IOReturn) sdCdbRead 		: (IOSCSIRequest *)scsiReq	 /* SCSI parameters */
			  	   buffer:(void *)buffer /* data destination */
				   client:(vm_task_t)client;
{
	sdBuf_t *sdBuf;
	IOReturn rtn;
	
	xpr_sd("sd%d sdCdbRead: opcode = 0x%x\n", [self unit],
		scsiReq->cdb.cdb_opcode, 3,4,5);
		
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_CDB_READ;
	sdBuf->scsiReq = scsiReq;
	sdBuf->buf = buffer;
	sdBuf->client = client;
	sdBuf->pending = NULL;
	sdBuf->needsDisk = 0;
	rtn = [self enqueueSdBuf:sdBuf];
	xpr_sd("sd%d sdCdbRead: returning %s\n", [self unit],
		[self stringFromReturn:rtn], 3,4,5);
	[self freeSdBuf:sdBuf];
	return(rtn);
}

- (IOReturn) sdCdbWrite		: (IOSCSIRequest *)scsiReq	 /* SCSI parameters */
			  	   buffer:(void *)buffer /* data destination */
				   client:(vm_task_t)client
{
	sdBuf_t *sdBuf;
	IOReturn rtn;
	
	xpr_sd("sd%d sdCdbWrite: opcode = 0x%x\n", [self unit],
		scsiReq->cdb.cdb_opcode, 3,4,5);
		
	sdBuf = [self allocSdBuf:NULL];
	sdBuf->command = SDOP_CDB_WRITE;
	sdBuf->scsiReq = scsiReq;
	sdBuf->buf = buffer;
	sdBuf->client = client;
	sdBuf->pending = NULL;
	sdBuf->needsDisk = 0;
	rtn = [self enqueueSdBuf:sdBuf];
	xpr_sd("sd%d sdCdbWrite: returning %s\n", [self unit],
		[self stringFromReturn:rtn], 3,4,5);
	[self freeSdBuf:sdBuf];
	return(rtn);	
}

- (int)target
{
	return _target;
}

- (int)lun
{
	return _lun;
}

/*
 * This is mainly here for the convenience of 486 CDROM boot. It's cheap so
 * we'll leave it in for other platforms too.
 */
- (unsigned char)inquiryDeviceType
{	
	return _inquiryDeviceType;
}

- controller
{
	return _controller;
}

- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    if( [super getDevicePath:path maxLength:maxLen useAlias:doAlias]) {

	char	unitStr[ 12 ];
	int	len = maxLen - strlen( path);

	sprintf( unitStr, "/@%x", [self target]);
	len -= strlen( unitStr);
	if( len < 0)
	    return( nil);
        strcat( path, unitStr);
	if( [self lun]) {
            sprintf( unitStr, ",%x", [self lun]);
            len -= strlen( unitStr);
            if( len < 0)
                return( nil);
            strcat( path, unitStr);
	}
	return( self);
    }
    return( nil);
}

- (char *) matchDevicePath:(char *)matchPath
{
    BOOL	matches = NO;
    char    *	unitStr;
    extern long int strtol(const char *nptr, char **endptr, int base);

    unitStr = [super matchDevicePath:matchPath];
    if( unitStr && (*unitStr == '/')) {
        unitStr = (char *)strchr( unitStr, '@');
        if( unitStr) {
            matches = ([self target] == strtol( unitStr + 1, &unitStr, 16));
            if( matches && (*unitStr == ','))
                matches = ([self lun] == strtol( unitStr + 1, &unitStr, 16));
        }
    }
    if( matches)
        return( unitStr);
    else
	return( NULL);
}

- property_IOUnit:(char *)result length:(unsigned int *)maxLen
{
    if( [self lun])
        sprintf( result, "%d,%d", [self target], [self lun]);
    else
        sprintf( result, "%d", [self target]);

    return( self);
}

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    static const char * inquiryDevTypeStrings[ 9 ] = {
	0, IOTypeTape, IOTypePrinter, 0,
	IOTypeWORM, IOTypeCDROM, IOTypeScanner, IOTypeOptical,
	IOTypeChanger
    };

    [super property_IODeviceType:types length:maxLen];

    if( (_inquiryDeviceType <= 8) && inquiryDevTypeStrings[ _inquiryDeviceType ]) {
	strcat( types, " ");
	strcat( types, inquiryDevTypeStrings[ _inquiryDeviceType ]);
    }
    return( self);
}

@end
