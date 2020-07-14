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
 * SCSIDiskThread.m - Implementation of SCSIDisk I/O thread and its associated
 *		  methods. All communication with the SCSIController object
 *		  is done here. 
 *
 * HISTORY
 * 04-Mar-91    Doug Mitchell at NeXT
 *      Created.
 */

/*
 * Enforce clean driverkit implementation.
 */
#define MACH_USER_API	1	
#undef	KERNEL_PRIVATE

#import <sys/types.h>
#import <kernserv/prototypes.h>
#import <driverkit/SCSIDiskPrivate.h>
#import <driverkit/SCSIDiskTypes.h>
#import <driverkit/SCSIDiskThread.h>
#import <driverkit/SCSIStructInlines.h>
#import <bsd/dev/scsireg.h>
#import <driverkit/xpr_mi.h>	
#import <driverkit/volCheck.h>
#import <driverkit/kernelDiskMethods.h>
#import <kern/assert.h>
#import <machkit/NXLock.h>
#import <driverkit/align.h>

static void sdThreadDequeue(SCSIDisk *sdp, 
	BOOL needs_disk);

@implementation SCSIDisk(Thread)

/*
 * Execute one I/O, as specified in sdBuf. Called out from an I/O thread.
 *
 * This is certainly the most complicated part of the SCSIDisk object; all 
 * retry logic is done here. It's kind of big. Maybe we can break this up 
 * a little bit.
 */
- (void)doSdBuf	: (sdBuf_t *)sdBuf
{
	IOSCSIRequest scsiReq;
	sc_status_t scrtn = SR_IOST_INVALID;
	IOReturn iortn;
	esense_reply_t senseReply;
	const char *name = [self name];
	
	xpr_sd("sd%d doSdBuf: sdBuf 0x%x\n", [self unit], sdBuf, 3,4,5);
	
	/*
	 * Initialize retry counters.
	 */
	sdBuf->busy_retry   = SD_RETRY_BUSY;
	sdBuf->norm_retry   = SD_RETRY_NORM;
	sdBuf->notRdy_retry = SD_RETRY_NOTRDY;

	while(sdBuf->busy_retry && sdBuf->norm_retry && sdBuf->notRdy_retry) {
	
		/*
		 * Set up a scsiReq to pass to the controller.
		 */
		scrtn = [self setupScsiReq:sdBuf scsiReq:&scsiReq];
		if(scrtn)
			return;
			
		
		/*
		 * Go for it.
		 */
		senseReply.er_ibvalid = 0;	// for error logging
		scrtn = [_controller executeRequest:&scsiReq
			buffer:sdBuf->buf
			client:sdBuf->client];
		xpr_sd("sd%d: CDB Complete: sdBuf 0x%x driverStatus "
			"%s scsiStatus"
			" 0x%x\n", [self unit], sdBuf, 
			IOFindNameForValue(scsiReq.driverStatus, 
				IOScStatusStrings),
			scsiReq.scsiStatus, 5);
		if(scrtn == SR_IOST_GOOD) {
			if((sdBuf->command == SDOP_READ) ||
			   (sdBuf->command == SDOP_WRITE)) {
				/* 
				 * Make sure the we moved the number of 
				 * bytes we wanted to. Note we skip this
				 * test for SDOP_CDB_xxx - that's the 
				 * user's problem.
			 	 */
				u_int block_size = [self blockSize];
			   	if((sdBuf->blockCnt * block_size) !=
				    scsiReq.bytesTransferred) {
				    if(--sdBuf->norm_retry <= 0) {
					IOLog("%s: TRANSFER COUNT ERROR; "
					    " FATAL.\n",  name);
					[self logOpInfo:sdBuf 
						sense:&senseReply];
					scrtn = SR_IOST_BCOUNT;
					goto abort;
				    }
				    IOLog("%s: TRANSFER COUNT ERROR. "
				    	" Expected = %d Received %d; "
					"Retrying.\n", name,
					sdBuf->blockCnt * block_size,
					sdBuf->bytesXfr);
					[self logOpInfo:sdBuf sense:NULL];
					goto do_retry;
				}
			}	
			
			/*
			 * Success. Log statistics.
			 */
			if(_isRegistered) {
			    switch(sdBuf->command) {
				case SDOP_READ:
				    [self addToBytesRead:
				    	scsiReq.bytesTransferred
					    totalTime:scsiReq.totalTime
					    latentTime:scsiReq.latentTime];
				    break;
				case SDOP_WRITE:
				    [self addToBytesWritten:
				    	scsiReq.bytesTransferred
					    totalTime:scsiReq.totalTime
					    latentTime:scsiReq.latentTime];
				    break;
			    }
			}
			
			/*
			 * All right, we're done.
			 */
			break;
		}
		
		if(sdBuf->retryDisable)
			goto abort;
			
		/*
		 * What seems to be the problem?
		 */
		switch(scrtn) {
		    case SR_IOST_SELTO:		/* selection timeout */
		    case SR_IOST_ALIGN:		/* DMA alignment */
		    case SR_IOST_BCOUNT:	/* DMA overrun */
		    case SR_IOST_CMDREJ:	/* command reject */
		    case SR_IOST_MEMALL:	/* malloc error */
		    case SR_IOST_MEMF:		/* memory failure */
		    case SR_IOST_IPCFAIL:	/* IPC failure */
		    case SR_IOST_INT:		/* internal error */
		    case SR_IOST_VOLNA:		/* volume not available */
		    case SR_IOST_INVALID:	/* our mistake */
		    case SR_IOST_WP:		/* write protect */
		    
		    	/*
			 * These are not retriable.
			 */
			IOLog("%s: %s : FATAL ERROR\n", name,
				IOFindNameForValue(scrtn, IOScStatusStrings));
			[self logOpInfo:sdBuf sense:&senseReply];

			goto abort;
			
		    case SR_IOST_BADST:
		    case SR_IOST_CHKSV:
		    case SR_IOST_CHKSNV:
		    	/*
			 * The only weird SCSI status values we know how to
			 * deal with are "Busy" and "Check Status".
			 */
			switch(scsiReq.scsiStatus) {
			    case STAT_BUSY:
			    	if(--sdBuf->busy_retry <= 0) {
					IOLog("%s: BUSY STATUS; FATAL.\n",
						name);
					[self logOpInfo:sdBuf 
						sense:&senseReply];
					goto abort;
				}
				IOLog("%s: BUSY STATUS; Retrying.\n", name);
				IOSleep(SD_BUSY_SLEEP * 1000);
				break;
				
			    case STAT_CHECK:
			    	/*
				 * Do a request sense to find out why.
				 */
				if(scrtn == SR_IOST_CHKSV) {
				    /*
				     * We already have sense data.
				     */
				    senseReply = scsiReq.senseData;
				}
				else {
				    scrtn = [self reqSense:&senseReply];
				    if(scrtn) {
					    IOLog("%s: REQUEST SENSE ERROR; "
						" FATAL.\n", name);
					    goto abort;
				    }
				}

				
				/*
				 * OK, we have valid sense data. 
				 */
				switch(senseReply.er_sensekey) {
				    case SENSE_NOTREADY:
					if(--sdBuf->notRdy_retry <= 0) {
					    IOLog("%s: NOT READY; "
					        "FATAL.\n", name);
					    /*
					     * FIXME - do we tell volCheck 
					     * about this??
					     */
					    [self logOpInfo:sdBuf 
						sense:&senseReply];
					    goto abort;
					}
					IOLog("%s: NOT READY; Retrying.\n",
						name);
					[self logOpInfo:sdBuf 
					    sense:&senseReply];
					IOSleep(SD_NOTRDY_SLEEP * 1000);
					break;
					
				    case SENSE_NOSENSE:		// I don't 
				    				//  trust this!
				    case SENSE_RECOVERED:	// or this
				    case SENSE_MEDIA:
				    case SENSE_HARDWARE:
					if(--sdBuf->norm_retry <= 0) {
					    IOLog("%s: %s; FATAL.\n", name,
					        IOFindNameForValue(senseReply.
						   er_sensekey,
						   IOSCSISenseStrings));
					    [self logOpInfo:sdBuf 
						sense:&senseReply];
					    goto abort;
					}
				    	IOLog("%s: %s; Retrying.\n", name,
					        IOFindNameForValue(senseReply.
						   er_sensekey,
						   IOSCSISenseStrings));
					[self logOpInfo:sdBuf 
					    sense:&senseReply];
				    	break;
					
				    case SENSE_UNITATTENTION:
				    	/*
					 * TBD - when we figure out how 
					 * to handle removable media...
					 */
					if(--sdBuf->norm_retry <= 0) {
					    IOLog("%s: UNIT ATTENTION;"
					    	" FATAL.\n", name);
					    [self logOpInfo:sdBuf 
						sense:&senseReply];
					    goto abort;
					}
				    	IOLog("%s: UNIT ATTENTION; "
						"Retrying.\n", name);
					[self logOpInfo:sdBuf 
					    sense:&senseReply];
				    	break;
					
				    case SENSE_DATAPROTECT:
				    	/*
					 * Not retryable, but at least we have 
					 * a unique error code.
					 */
					IOLog("%s: WRITE PROTECTED. FATAL"
						".\n", name);
					[self logOpInfo:sdBuf 
					    sense:&senseReply];
					scrtn = SR_IOST_WP;
					goto abort;
					
				    default:
				    	/*
					 * All others fatal.
					 */
					IOLog("%s: %s; FATAL.\n", name,
					        IOFindNameForValue(senseReply.
						   er_sensekey,
						   IOSCSISenseStrings));
					[self logOpInfo:sdBuf 
					    sense:&senseReply];
					scrtn = SR_IOST_CHKSV;
					goto abort;					
				}  		/* switch sense key */			
				break;		/* from case STAT_CHECK */
				
			    default:
			    	/*
				 * Sorry, no can do.
				 */
				IOLog("%s: BOGUS SCSI STATUS (0x%x): "
					"FATAL.\n", name, scsiReq.scsiStatus);
				[self logOpInfo:sdBuf 
				    sense:&senseReply];
				scrtn = SR_IOST_BADST;
				goto abort;
				
			} 		/* switch scsiStatus */
			break;		/* from case SR_IOST_BADST */
			
		    default:
		    	/*
			 * Well, what the hey. retry.
			 */
			if(--sdBuf->norm_retry <= 0) {
			    IOLog("%s: %s; FATAL.\n", name,
			    	IOFindNameForValue(scrtn, IOScStatusStrings));
			    [self logOpInfo:sdBuf 
				sense:&senseReply];
			    goto abort;
			}
			IOLog("%s: %s; Retrying.\n", name,
			    	IOFindNameForValue(scrtn, IOScStatusStrings));
			[self logOpInfo:sdBuf 
			    sense:&senseReply];
			break;
		} /* switch scrtn */
do_retry:
		/*
		 * Retrying. log this with IODisk.
		 */
		if(_isRegistered) {
		    switch(sdBuf->command) {
			case SDOP_READ:
			    [self incrementReadRetries];
			    break;
			case SDOP_WRITE:
			    [self incrementWriteRetries];
			    break;
			default:
			    [self incrementOtherRetries];
			    break;
		    }
		}
	} /* main retry loop */
	
abort:

	/*
	 * We've either successfuly executed the command or have given up.
	 * Transfer appropriate return values to sdBuf and sdIoComplete the
	 * result.
	 */
	if((sdBuf->busy_retry == 0) ||
	   (sdBuf->norm_retry == 0) ||
	   (sdBuf->notRdy_retry == 0)) {
	   	iortn = IO_R_IO;
	}
	else if(scrtn) {
		iortn = [_controller returnFromScStatus:scrtn];
	}
	else {
		iortn = IO_R_SUCCESS;
	}
	if(iortn && _isRegistered) {
		/*
		 * All fatal errors must be logged with IODisk.
		 */
		switch(sdBuf->command) {
		    case SDOP_READ:
			[self incrementReadErrors];
			break;
		    case SDOP_WRITE:
			[self incrementWriteErrors];
			break;
		    default:
			if(!sdBuf->retryDisable) {
				[self incrementOtherErrors];
			}
			break;
		}
	}
	if(sdBuf->scsiReq) {
		/*
		 * This implies SDOP_CDB_{READ,WRITE}.
		 */
		sdBuf->scsiReq->driverStatus = scsiReq.driverStatus;
		sdBuf->scsiReq->scsiStatus = scsiReq.scsiStatus;
		sdBuf->scsiReq->bytesTransferred = scsiReq.bytesTransferred;

		/*
		 * Don't forget to copy over the sense data if the controller
		 * has provided it to us.   We do not copy sense data that we
		 * manually requested for our own purposes in SCSIDisk.
		 */
		if (scsiReq.driverStatus == SR_IOST_CHKSV)
		  sdBuf->scsiReq->senseData = scsiReq.senseData;
	}
#if	0
	else if(iortn == IO_R_SUCCESS) {
		/*
		 * We know we can read this thing.
		 */
		[self setFormattedInternal:1];
	}
#endif	0
	sdBuf->bytesXfr = scsiReq.bytesTransferred;
	sdBuf->status = iortn;
	[self sdIoComplete:sdBuf];
	return;
}

/*
 * Log info about current operation for retry/error reporting.
 */
- (void)logOpInfo : (sdBuf_t *)sdBuf
		    sense : (esense_reply_t *)senseReply
{
	char opString[40];
	char blockString[80];

	blockString[0] = '\0';
	switch(sdBuf->command) {
	    case SDOP_READ:
	    	strcpy(opString, "Read");
		goto logBlock;
	    case SDOP_WRITE:
	    	strcpy(opString, "Write");
logBlock:	
		if(senseReply && senseReply->er_ibvalid) {
			/*
			 * Get block # from sense data
			 */
			unsigned blockInErr;
			
			blockInErr = scsi_error_info(senseReply);
			sprintf(blockString, "block:%d", blockInErr);
		}
		else {
			/*
			 * Just report starting block # and length.
			 */
			sprintf(blockString, "block:%d blockCount:%d",
				sdBuf->block, sdBuf->blockCnt);
		}
		break;
		
	    case SDOP_CDB_READ:
	    case SDOP_CDB_WRITE:
	    	strcpy(opString, 
			IOFindNameForValue(sdBuf->scsiReq->cdb.cdb_opcode,
			IOSCSIOpcodeStrings));
		break;
		
	    case SDOP_EJECT:
	    	strcpy(opString, "Eject");
		break;
		
	    default:
	    	/*
		 * The remainder should not result in disk errors...
		 */
	    	panic("Bogus op in logOpInfo");
	}
	IOLog("   target:%d lun:%d op:%s %s\n", _target, _lun, 
		opString, blockString);
	return;
}

/*
 * generate a CDB for a read or write operation. Assumes that *cdbp has
 * already been zero'd.
 */
- (void)genRwCdb : (cdb_t *)cdbp
	readFlag : (BOOL)readFlag
	block : (u_int)block
	blockCnt : (u_int)blockCnt
{
	cdb_10_t *cdb = &cdbp->cdb_c10; 
	unsigned short blockCntS = blockCnt;
		 
	if (readFlag)
		scsi_readextended_setup(cdb, _lun, block, blockCntS);
	else
		scsi_writeextended_setup(cdb, _lun, block, blockCntS);
}

/*
 * Set up a scsiReq for a given sdBuf_t to pass to the controller. Only used
 * inside of I/O thread.
 */
- (sc_status_t)setupScsiReq:(sdBuf_t *)sdBuf
	scsiReq:(IOSCSIRequest *)scsiReq
{ 
	u_int block_size;
	
	block_size = [self blockSize];
	bzero(scsiReq, sizeof(IOSCSIRequest));
	scsiReq->target = _target;
	scsiReq->lun = _lun;
	switch(sdBuf->command) {
	    case SDOP_READ:
		scsiReq->read = YES;
		[self genRwCdb:&scsiReq->cdb
			readFlag : YES
			block : sdBuf->block
			blockCnt : sdBuf->blockCnt];
		goto rw_common;
		
	    case SDOP_WRITE:
		scsiReq->read = NO;
		[self genRwCdb:&scsiReq->cdb
			readFlag : NO
			block : sdBuf->block
			blockCnt : sdBuf->blockCnt];
rw_common:
		sdBuf->scsiReq = NULL;
		scsiReq->maxTransfer = sdBuf->blockCnt * block_size;
		scsiReq->timeoutLength = SD_TIMEOUT_RW;
		scsiReq->disconnect = 1;
		break;
		
	    case SDOP_EJECT:
	    case SDOP_CDB_READ:
	    case SDOP_CDB_WRITE:
		if((sdBuf->scsiReq->target != _target) ||
		    (sdBuf->scsiReq->lun != _lun)) {
			/*
			 * Ooops, can't allow this!
			 */
			sdBuf->scsiReq->driverStatus = SR_IOST_CMDREJ;
			sdBuf->status = IO_R_INVALID_ARG;
			[self sdIoComplete:sdBuf];
			return(SR_IOST_CMDREJ);
		}
		/*
		 * Otherwise, we'll allow just about anything.
		 */
		*scsiReq = *sdBuf->scsiReq;
		break;
	    default:
	    	ASSERT(0);
		break;	
	}
	return(SR_IOST_GOOD);
}

/*
 * Execute a request sense command within the I/O thread. esenseReply
 * does not have to be aligned.
 */
- (sc_status_t)reqSense : (esense_reply_t *)esenseReply
{
	IOSCSIRequest scsiReq;
	sc_status_t rtn;
	cdb_6_t *cdbp = &scsiReq.cdb.cdb_c6;
	esense_reply_t *alignedReply;
	void *freep;
	int freeCnt;
	IODMAAlignment dmaAlign;
	
	xpr_sd("sd%d reqSense\n", [self unit], 2,3,4,5);

	/*
	 * Get some well-aligned memory.
	 */
	alignedReply = [_controller allocateBufferOfLength:
					sizeof(esense_reply_t)
			actualStart:&freep
			actualLength:&freeCnt];
	bzero(&scsiReq, sizeof(IOSCSIRequest));
	
	cdbp->c6_opcode = C6OP_REQSENSE;
	cdbp->c6_lun = _lun;
	cdbp->c6_len = sizeof(esense_reply_t);
	scsiReq.target = _target;
	scsiReq.lun = _lun;
	scsiReq.read = YES;

	/*
	 * Get appropriate alignment from controller. 
	 */
	[_controller getDMAAlignment:&dmaAlign];
	if(dmaAlign.readLength > 1) {
		scsiReq.maxTransfer = IOAlign(int, sizeof(esense_reply_t), 
			dmaAlign.readLength);
	}
	else {
		scsiReq.maxTransfer = sizeof(esense_reply_t);
	}
	scsiReq.timeoutLength = SD_TIMEOUT_SIMPLE;
	scsiReq.disconnect = 1;
	rtn = [_controller executeRequest:&scsiReq
		buffer:alignedReply
		client:IOVmTaskSelf()];
	/*
	 * Copy data back to caller's struct.
	 */
	 
	*esenseReply = *alignedReply;
	IOFree(freep, freeCnt);
	xpr_sd("sd%d reqSense: returning %d\n", [self unit], rtn, 3,4,5);
	
	return(rtn);
}

/*
 * Either wake up the thread which is waiting on the sdBuf, or send an 
 * ioComplete back to client. sdBuf->status must be valid.
 */
- (void)sdIoComplete : (sdBuf_t *)sdBuf
{
	xpr_sd("%s sdIoComplete: sdBuf 0x%x status %s\n", 
		[self name], sdBuf,
		[self stringFromReturn:sdBuf->status], 4,5);
	if(sdBuf->pending) {
	
		/*
		 * Async I/O complete. This will change with RO support.
		 */
		int dirRead = 0;
		
		switch(sdBuf->command) {
		    case SDOP_READ:
		    case SDOP_CDB_READ:
		    	dirRead = 1;
			break;
		    default:
		    	break;
		}
		[self completeTransfer : sdBuf->pending
			withStatus : sdBuf->status
			actualLength : sdBuf->bytesXfr];
		[self freeSdBuf:sdBuf];
	}
	else {
		/*
		 * Sync I/O. Just wake up the waiting thread.
		 */
		[sdBuf->waitLock unlockWith:YES];
	}
}

/*
 * Unlock ioQLock, updating condition variable as appropriate.
 */
- (void)unlockIoQLock
{
	int queue_state;
	IODiskReadyState lastReady = [self lastReadyState];
	
	/*
	 * There's still work to do when:
	 *    -- ioQueueNodisk non-empty, or
	 *    -- ioQueueDisk non-empty and we "really" have a disk.
	 */

	if((!queue_empty(&_ioQueueNodisk)) ||
	   	((!queue_empty(&_ioQueueDisk)) &&
	    		(lastReady != IO_NoDisk) &&
	    		(lastReady != IO_Ejecting) &&
      	    		(!_ejectPending)
		)
	   ) {
		queue_state = WORK_AVAILABLE;
	}
	else	
		queue_state = NO_WORK_AVAILABLE;
	[_ioQLock unlockWith:queue_state];

	if (queue_state == WORK_AVAILABLE)
		(void) thread_block();
}


/*
 * I/O thread. Each one of these sits around waiting for work to do on 
 * ioQueue; when something appears, the thread grabs it, creates an
 * appropriate scsiReq, invokes the proper controller method, and
 * handles a possible error condition (including retries).
 */
 
volatile void sdIoThread(id me)
{
	SCSIDisk *sdp = (SCSIDisk *)me;
	IODiskReadyState lastReady;
	
	xpr_sd("sdIoThread unit %d: STARTING\n", [sdp unit], 2,3,4,5);
		
	while(1) {
		/*
	 	 * Wait for some work to do. Keep the ioQLock until we 
		 * dequeue something.
		 */
		xpr_sd("sdIoThread: waiting for work\n", 1,2,3,4,5);
		[sdp->_ioQLock lockWhen:WORK_AVAILABLE];
		
		/*
		 * Service all requests which do not need a disk.
		 */
		xpr_sd("sdIoThread: servicing q_nodisk\n", 1,2,3,4,5);
		while(!queue_empty(&sdp->_ioQueueNodisk))
			sdThreadDequeue(sdp, NO);
			
		/*
		 * Now service all requests which need a disk, if our disk
		 * is present. We still hold ioQLock.
		 */
		xpr_sd("sdIoThread: servicing q_disk\n", 1,2,3,4,5);
		while((!queue_empty(&sdp->_ioQueueDisk)) &&
		      ([sdp lastReadyState] != IO_NoDisk) &&
		      ([sdp lastReadyState] != IO_Ejecting) &&
		      (!sdp->_ejectPending)) { 
			sdThreadDequeue(sdp, YES);
		}
		
		/*
		 * If we have work to do in ioQueueDisk but we don't have 
		 * a disk, ask volCheck to put up a panel. In either case, 
		 * when we unlock ioQueueLock for the last time, update its
		 * condition variable as appropriate so we and the other 
		 * I/O threads know whether or not to sleep.
		 */

		lastReady = [sdp lastReadyState];
		if((!queue_empty(&sdp->_ioQueueDisk)) && 
		   ((lastReady == IO_NotReady && [sdp isRemovable]) ||
		    (lastReady == IO_NoDisk) || sdp->_ejectPending)) {
		   	[sdp unlockIoQLock];
			xpr_sd("sdIoThread: volCheckRequest()\n", 1,2,3,4,5);
			volCheckRequest(sdp, PR_DRIVE_SCSI);
		}
		else {
			[sdp unlockIoQLock];
		}
	}	/* while 1 */
	
	/* NOT REACHED */
}

/*
 * Process a request at the head of one of the IOQueues.
 * ioQLock must be held on entry; it will still be held on exit.
 */
static void sdThreadDequeue(SCSIDisk *sdp, 
	BOOL needs_disk)
{
	queue_head_t *q;
	sdBuf_t *sdBuf;
	
	if(needs_disk)
		q = &sdp->_ioQueueDisk;
	else
		q = &sdp->_ioQueueNodisk;
	if(queue_empty(q)) {
		IOLog("sdThreadDequeue: Empty queue!\n");
		return;
	}
	sdBuf = (sdBuf_t *)queue_first(q);
	queue_remove(q, 
		sdBuf,
		sdBuf_t *,
		link);
	
	/*
	 * In case of an eject command, we have to ensure right now - while 
	 * the queue is locked - that no other threads attempt any 
	 * "needs disk" type of I/Os. That's what the ejectPending flag is 
	 * for; we can't rely on the volCheck thread setting our 
	 * lastReadyState to RS_EJECTING for another second or so...
	 *
	 * The numDiskIos counter allows the thread doing an eject command
	 * to wait for all other pending Disk I/Os to complete before doing
	 * the eject.
	 */
	if(needs_disk) {
		[sdp->_ejectLock lock];
		sdp->_numDiskIos++;
		if(sdBuf->command == SDOP_EJECT) {
			sdp->_ejectPending = YES;
			volCheckEjecting(sdp, PR_DRIVE_SCSI);
		}
		[sdp->_ejectLock unlockWith:sdp->_numDiskIos];
	}
	
	/*
	 * Execute command specified in sdBuf. We hold _ioQLock.
	 */
	
	xpr_sd("sdThreadDequeue: sdBuf 0x%x received\n", sdBuf, 2,3,4,5); 
	switch(sdBuf->command) {
	    case SDOP_ABORT:
	    	/*
		 * Ah hah, we have to ioComplete all of the sdBufs in
		 * ioQueueDisk.
		 */
		{
		    sdBuf_t *abortBuf;
		    queue_head_t *q = &sdp->_ioQueueDisk;
		    
		    while(!queue_empty(q)) {
			    abortBuf = (sdBuf_t *)queue_first(q);
			    queue_remove(q, abortBuf, sdBuf_t *, link);
			    [sdp->_ioQLock unlock];
			    abortBuf->status = IO_R_NO_DISK;
			    if(abortBuf->scsiReq)
				abortBuf->scsiReq->driverStatus = SR_IOST_VOLNA;
			    [sdp sdIoComplete:abortBuf];
			    [sdp->_ioQLock lock];
		    }
		    
		    /*
		     * and ioComplete the abort command itself.
		     */
		    sdBuf->status = IO_R_SUCCESS;
		    [sdp sdIoComplete:sdBuf];
		}
		break;
		
	    case SDOP_PROBEDISK:
	    	/*
		 * If we got this far, the disk is present. ioComplete
		 * immediately. (If disk was not present, we would have
		 * disposed of this in case SDOP_ABORT.)
		 */
		sdBuf->status = IO_R_SUCCESS;
		[sdp sdIoComplete:sdBuf];
		break;
	
	    case SDOP_EJECT:
		/*
		 * Wait until we're the only thread executing.
		 */
		[sdp->_ioQLock unlock];
		[sdp->_ejectLock lockWhen:1];
		[sdp->_ejectLock unlock];	 
	    	[sdp doSdBuf:sdBuf];	
		[sdp->_ioQLock lock];
		break;
		
	    case SDOP_READ:
	    case SDOP_WRITE:
	    case SDOP_CDB_READ:
	    case SDOP_CDB_WRITE:
	    	/*
		 * We don't want to hold this lock while executing this
		 * command...
		 */
	        [sdp->_ioQLock unlock];
	    	[sdp doSdBuf:sdBuf];
		[sdp->_ioQLock lock];
		break;
		
	    case SDOP_ABORT_THREAD:
	    	/*
		 * I/O complete this before we die.
		 */
		[sdp->_ioQLock unlock];
		sdBuf->status = IO_R_SUCCESS;
		[sdp sdIoComplete:sdBuf];
		xpr_sd("%s: calling IOExitThread()\n", [sdp name],
			2,3,4,5);
		IOExitThread();
		break;
	}
	
	if(needs_disk) {
		/*
		 * Enable possible waiting eject command.
		 */
		[sdp->_ejectLock lock];
		sdp->_numDiskIos--;
		[sdp->_ejectLock unlockWith:sdp->_numDiskIos];
	}
}	

@end


