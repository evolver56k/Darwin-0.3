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
/*
 * Copyright (c) 1991, 1992, 1993 NeXT Computer, Inc.
 *
 * Abstract superclass for SCSI controllers.
 *
 * HISTORY
 *
 * 1998-3-11 gvdl@apple.com Moved SCSI user to drvSCSIServer project.
 * 12 December 1997 Martin Minow at Apple
 * 	Added IOSCISUser kernel support
 * 14 June 1995	Doug Mitchell at NeXT
 *	Added SCSI-3 support.
 * 25 June 1993 David E. Bohman at NeXT
 *	Cleaned up some (made machine independent).  Added this header.
 * ?? ??? ???? Doug Mitchell at NeXT
 *	Created.
 */

#import <driverkit/IOSCSIController.h>
#import <machkit/NXLock.h>
#import <driverkit/return.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/align.h>
#import <driverkit/xpr_mi.h>
#import <driverkit/kernelDriver.h>

static int scUnitNum = 0;

/*
 * An element of _reserveQ.
 */
typedef struct {
	unsigned long long 	target;
	unsigned long long 	lun;
	id			owner;
	id			lender;
	queue_chain_t		link;
} reserveElt;

@interface IOSCSIController(private)

- (reserveElt *) searchReserveQ: (unsigned long long) target 
	                    lun: (unsigned long long) lun;
			   
@end

@implementation IOSCSIController

- initFromDeviceDescription:deviceDescription
{
	unsigned int	target, lun;
	char		devName[20];
	IODMAAlignment 	dmaAlign;

	queue_init(&_reserveQ);
	if([[self class] deviceStyle] == IO_DirectDevice) {
		/*
		 * Allow a subclass to act like an indirect device by 
		 * skipping this stuff.
		 */
		if ([super initFromDeviceDescription:deviceDescription] == nil)
			return nil;
			
		if ([self startIOThread] != IO_R_SUCCESS) {
		    [self free]; return nil;
		}
	} else
	    [self setDeviceDescription:deviceDescription];
	
	/*
	 *  We keep track of all scsi controllers in the system by number.
	 *  Their device names look like 'scXX'.  Note that the rest of the
	 *  system actually LOOKS FOR sc0 instead of something more basic,
	 *  so we're forced into using this scheme.
	 */	
	[self setUnit:scUnitNum];
	sprintf(devName, "sc%d", scUnitNum++);
	[self setName:devName];
	[self setDeviceKind:"sc"];

	/*
	 * Determine worst case alignment requirement for this controller.
	 * The result will be used to do the alignment in 
	 * allocateBufferOfLength.
	 */
	[self getDMAAlignment:&dmaAlign];
	_worstCaseAlign = dmaAlign.readStart;
	if(dmaAlign.writeStart > _worstCaseAlign) {
		_worstCaseAlign = dmaAlign.writeStart;
	}
	if(dmaAlign.readLength > _worstCaseAlign) {
		_worstCaseAlign = dmaAlign.readLength;
	}
	if(dmaAlign.writeLength > _worstCaseAlign) {
		_worstCaseAlign = dmaAlign.writeLength;
	}
	if(_worstCaseAlign == 1) {
		_worstCaseAlign = 0;
	}
	return self;
}


- free
{
	int size;
	reserveElt *rsvElt;
	
	if(_reserveQ.next != NULL) {
		/*
		 * Avoid if queue hasn't been init'd....
		 */
		while(!queue_empty(&_reserveQ)) {
			rsvElt = (reserveElt *)queue_first(&_reserveQ);
			queue_remove(&_reserveQ, rsvElt, reserveElt *, link);
			IOFree(rsvElt, sizeof(*rsvElt));
		}
	}
	
	/*
	 * If this is the last IOSCSIController to have been initialized,
	 * update the global unit counter.
	 */
	if([self unit] == (scUnitNum - 1)) {
		scUnitNum--;
	}
	
	/* XXX what about the SCSIController thread?? */
	return [super free];
}

/*
 * Release a reserved element for the owner
 * _reserveLock held
 */

- (void) releaseReservationElement:(reserveElt *) rsvElt
{
    id	lender;

    lender = rsvElt->lender;
    if( lender
    && [lender respondsTo:@selector(reacquireTarget)]
    && (IO_R_SUCCESS == [lender reacquireTarget]) ) {

	rsvElt->lender = nil;
	rsvElt->owner = lender;

    } else {
        queue_remove(&_reserveQ, rsvElt, reserveElt *, link);
        IOFree(rsvElt, sizeof(*rsvElt));
        _reserveCount--;
    }
}

/*
 * Attempt to reserve specified target and lun for calling device. Returns
 * non-zero if device already reserved.
 *
 * This is called by a client (e.g., SCSIDisk) in an attempt to mark a 
 * particular target/lun as being "in use" by that client; it's normally
 * only used during SCSI bus configuration.
 */

- releaseAllUnitsForOwner: owner;
{
    reserveElt *rsvElt, *nxtElt;

    [_reserveLock lock];

    rsvElt = (reserveElt *) queue_first(&_reserveQ);
    while ( !queue_end(&_reserveQ, (queue_t) rsvElt) ) {
	nxtElt = (reserveElt *) rsvElt->link.next;
	if (rsvElt->owner == owner) {
	    [self releaseReservationElement:rsvElt];
	}
	rsvElt = nxtElt;
    }

    [_reserveLock unlock];
}

- (int) reserveTarget:(u_char)target lun:(u_char)lun forOwner:(id)owner
{
	return [self reserveSCSI3Target:(unsigned long long)target
		lun:(unsigned long long)lun
		forOwner:owner];
}


- (void)releaseTarget:(u_char)target lun:(u_char)lun forOwner:(id)owner
{
	[self releaseSCSI3Target:(unsigned long long)target
		lun:(unsigned long long)lun
		forOwner:owner];
}

- (int)reserveSCSI3Target	: (unsigned long long)target
			    lun : (unsigned long long)lun
		       forOwner : owner
{
	int rtn = 0;
	int targInt = target;
	reserveElt *rsvElt;
	id	currentOwner;
	
	xpr_sdev("reserveSCSI3Target: t %x:%x l %x:%x\n",
		(unsigned)(target >> 32),
		(unsigned)target,
		(unsigned)(lun >> 32),
		(unsigned)lun, 5);
	[_reserveLock lock];
	
	if(targInt >= [self numberOfTargets]) {
		rtn = 1;
		goto out;
	}
	rsvElt = [self searchReserveQ:target lun:lun];
	if(rsvElt) {
		xpr_sdev("...already reserved\n", 1,2,3,4,5);

		currentOwner = rsvElt->owner;
		if( (rsvElt->lender == nil)		// only 1 deep stack
		&& [currentOwner respondsTo:@selector(requestReleaseTarget)]
		&& (IO_R_SUCCESS == [currentOwner requestReleaseTarget]) ) {

		    rsvElt->lender = currentOwner;
		    rsvElt->owner = owner;

		} else
                    rtn = 1;

                goto out;
	}
	
	/*
	 * Cons up a new element for _reserveQ.
	 */
	rsvElt = IOMalloc(sizeof(*rsvElt));
	rsvElt->target = target;
	rsvElt->lun = lun;
	rsvElt->owner = owner;
	rsvElt->lender = nil;
	queue_enter(&_reserveQ, rsvElt, reserveElt *, link);
	_reserveCount++;
out:
	[_reserveLock unlock];
	return(rtn);
}

- (void)releaseSCSI3Target	: (unsigned long long)target
			    lun : (unsigned long long)lun
		       forOwner : owner
{
	int targInt = target;
	reserveElt *rsvElt;
	
	xpr_sdev("releaseSCSI3Target: t %x:%x l %x:%x\n",
		(unsigned)(target >> 32),
		(unsigned)target,
		(unsigned)(lun >> 32),
		(unsigned)lun, 5);
	[_reserveLock lock];
	if(targInt >= [self numberOfTargets]) {
		IOLog("IOSCSIController releaseTarget: INVALID TARGET\n");
		goto out;
	}
	rsvElt = [self searchReserveQ:target lun:lun];
	if(rsvElt == NULL) {
		IOLog("IOSCSIController releaseTarget: NOT RESERVED\n");
		goto out;
	}
	if(rsvElt->owner != owner) {
		IOLog("IOSCSIController releaseTarget: INVALID OWNER\n");
		goto out;
	}

	[self releaseReservationElement:rsvElt];

out:
	[_reserveLock unlock];
}


/*
 * Returns the number of SCSI targets supported. Default is 8, returned here.
 * Subclasses may override if they support more than 8 targets.
 */
- (int)numberOfTargets
{
	return SCSI_NTARGETS;
}

- (unsigned int) numReserved
{
	return _reserveCount;
}

- (sc_status_t) executeRequest : (IOSCSIRequest *)scsiReq	 
		        buffer : (void *)buffer /* data destination */
		        client : (vm_task_t)client
{
	/*
	 * Subclass must implement this; we should never get here.
	 */
	return SR_IOST_INVALID;
}

/*
 * executeRequest (with an IOMemoryDescriptor) requires buffers aligned to
 * IO_SCSI_DMA_ALIGNMENT. The client identification is contained in the
 * IOMemoryDescriptor object
 */
- (sc_status_t) executeRequest	: (IOSCSIRequest *) scsiReq
	ioMemoryDescriptor	: (IOMemoryDescriptor *) ioMemoryDescriptor
{
	/*
	 * Subclass must implement this; we should never get here.
	 */
	return SR_IOST_INVALID;
}
			   
- (sc_status_t) executeSCSI3Request : (IOSCSI3Request *)scsiReq	 
	buffer			: (void *)buffer /* data destination */
	client			: (vm_task_t)client
{
	/*
	 * Optional.
	 */
	return SR_IOST_CMDREJ;
}

/*
 * executeRequest (with an IOMemoryDescriptor) requires buffers aligned to
 * IO_SCSI_DMA_ALIGNMENT. The client identification is contained in the
 * IOMemoryDescriptor object
 */
- (sc_status_t) executeSCSI3Request : (IOSCSI3Request *) scsiReq
	ioMemoryDescriptor	: (IOMemoryDescriptor *) ioMemoryDescriptor
{
	/*
	 * Optional.
	 */
	return SR_IOST_CMDREJ;
}
			   
- (sc_status_t) resetSCSIBus
{
	return SR_IOST_INVALID;
}

/*
 * Convert an sc_status_t to an IOReturn.
 */
- (IOReturn)returnFromScStatus:(sc_status_t)sc_status
{
	switch(sc_status) {
	    case SR_IOST_GOOD:
	    	return(IO_R_SUCCESS);
	    case SR_IOST_ALIGN:
	    	return(IO_R_ALIGN);
	    case SR_IOST_CMDREJ:
	    	return(IO_R_INVALID_ARG);
	    case SR_IOST_IPCFAIL:
	    	return(IO_R_IPC_FAILURE);
	    case SR_IOST_INT:
	    	return(IO_R_INTERNAL);
	    case SR_IOST_MEMF:
	    	return(IO_R_VM_FAILURE);
	    case SR_IOST_MEMALL:
	    	return(IO_R_RESOURCE);
	    case SR_IOST_DMA:
	    	return(IO_R_DMA);
	    case SR_IOST_WP:
	    	return(IO_R_NOT_WRITABLE);
	    default:
	    	return(IO_R_IO);
	}
}


/*
 * Determine maximum DMA which can be peformed in a single call to 
 * executeRequest:buffer:client:.
 */
- (unsigned)maxTransfer
{
	return 16 * 1024 * 1024;		/* XXX */
}


/*
 * Return required DMA alignment for current architecture.
 * May be overridden by subclass.
 */
- (void)getDMAAlignment:(IODMAAlignment *)alignment
{
	alignment->readStart   = 1;	/* XXX */
	alignment->writeStart  = 1;	/* XXX */
	alignment->readLength  = 1;	/* XXX */
	alignment->writeLength = 1;	/* XXX */
}



- (unsigned int) numQueueSamples
{
	return 0;
}


- (unsigned int) sumQueueLengths
{
	return 0;
}


- (unsigned int) maxQueueLength
{
	return 0;
}


- (void)resetStats
{
}


- (IOReturn)getIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
{
	unsigned maxCount = *count;
	unsigned *returnedCount = count;
	
	unsigned int params[IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE];
	int i;
	if(maxCount == 0) {
		maxCount = IO_MAX_PARAMETER_ARRAY_LENGTH;
	}
	if(strcmp(parameterName, IO_SCSI_CONTROLLER_STATS) == 0) {
		params[IO_SCSI_CONTROLLER_MAX_QUEUE_LENGTH] = 
			[self maxQueueLength];
		params[IO_SCSI_CONTROLLER_QUEUE_SAMPLES] = 
			[self numQueueSamples];
		params[IO_SCSI_CONTROLLER_QUEUE_TOTAL] = 
			[self sumQueueLengths];
	
		*returnedCount = 0;
		for(i=0; i<IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE; i++) {
			if(*returnedCount == maxCount)
				break;
			parameterArray[i] = params[i];
			(*returnedCount)++;
		}
		return IO_R_SUCCESS;
	}
	else if(strcmp(parameterName, IO_IS_A_SCSI_CONTROLLER) == 0) {
		/*
		 * No data; just let caller know we're a disk.
		 */
		*returnedCount = 0;
		return IO_R_SUCCESS;
	}
	else {
		return [super getIntValues:parameterArray
			forParameter:parameterName
			count:count];

	}
}					


- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
{
	if(strcmp(parameterName, IO_SCSI_CONTROLLER_STATS) == 0) {
		[self resetStats];
		return IO_R_SUCCESS;
	}
	else
		return [super setIntValues:parameterArray
				   forParameter:parameterName
				   count : count];
}

/*
 * Allocate some well-aligned memory. *actualStart is what has to be 
 * IOFree()'d eventually; *actualLength is the size arg to IOFree. The 
 * return value is the well-aligned pointer. There is room at the end of 
 * the returned pointer's buffer to do a DMA_DO_ALIGN(...,size, 
 * _worstCaseAlign).
 */

- (void *)allocateBufferOfLength: (unsigned)length
	actualStart : (void **)actualStart
	actualLength : (unsigned *)actualLength
{
	char *cp;
	int cnt = length + 2 * _worstCaseAlign;
	
	cp = IOMalloc(cnt);
	*actualStart = cp;
	*actualLength = cnt;
	if(_worstCaseAlign > 1) {
		return(IOAlign(void *, cp, _worstCaseAlign));
	} 
	else {
		return cp;
	}
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassSCSIController);
    return( self);
}

- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen
{
    if( [[self class] deviceStyle] == IO_DirectDevice)
        strcat( classes, " "IOTypeSCSI);
    return( self);
}

@end

@implementation IOSCSIController(private)

/* 
 * Locate reserveElt for specified target and LUN. _reserveLock must be
 * held on entry.
 */
- (reserveElt *)searchReserveQ : (unsigned long long)target 
	                   lun : (unsigned long long)lun
{
	reserveElt	*rsvElt;
	
	rsvElt = (reserveElt *)queue_first(&_reserveQ);
	while(!queue_end(&_reserveQ, (queue_t)rsvElt)) {
		if((rsvElt->target == target) && (rsvElt->lun == lun)) {
		 	return rsvElt;
		}
		rsvElt = (reserveElt *)rsvElt->link.next;
	}
	return NULL;
}

@end
