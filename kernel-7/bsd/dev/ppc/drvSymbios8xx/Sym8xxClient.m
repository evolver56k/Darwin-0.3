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

/* Sym8xxClient.m created by russb2 on Sat 30-May-1998 */

#import "Sym8xxController.h"

static u_int8_t		xferMsgSync[]   = {0x01, 0x03, 0x01, 0x0c, 0x10};
static u_int8_t		xferMsgAsync[]  = {0x01, 0x03, 0x01, 0x00, 0x00};
static u_int8_t		xferMsgWide16[] = {0x01, 0x02, 0x03, 0x01};
static u_int8_t 	cdbLength[8] 	= { 6, 10, 10, 0, 0, 12, 0, 0 };

@implementation Sym8xxController(Client)

/*-----------------------------------------------------------------------------*
 *  Client thread routines.
 *
 *  This module processes I/O requests from driverKit. It does most of the resource 
 *  allocation and command preparation. Once a command is prepared it is queued
 *  to the driver's I/O Thread for execution.
 *
 *-----------------------------------------------------------------------------*/
- (sc_status_t) executeRequest:(IOSCSIRequest *)scsiReq  buffer:(void *)buffer  client:(vm_task_t)client
{
    SRB			*srb;
    Nexus		*nexus, *nexusPhys;
    u_int32_t		len;
    ns_time_t		startTime, endTime;

    IOGetTimestamp( &startTime );

    /*
     * If a SCSI Bus reset is detected, we hold-off command processing until the targets have
     * had a chance to recover.
     */
    while ( resetQuiesceTimer )
    {
        [resetQuiesceSem lock];
    }
    [resetQuiesceSem unlock];    

    /*
     * Allocate and initialize a SRB structure.
     * Note: This routine clears the SRB and initializes srb->srbPhys
     *       which contains the physical address of the srb. 
     */
    srb = [self Sym8xxAllocSRB];
    if ( srb == NULL )
    {
        return -1;
    }

    nexus            = &srb->nexus;
    nexusPhys        = &srb->srbPhys->nexus;

    /*
     * Set client data buffer pointers in the SRB
     */
    srb->xferClient = client;
    srb->xferBuffer = (vm_offset_t) buffer;
    srb->xferCount  = scsiReq->maxTransfer;

    /*
     * Set request sense buffer pointers in the SRB
     */
    if ( !scsiReq->ignoreChkcond )
    {
        srb->senseData       = (vm_offset_t) &scsiReq->senseData;
        srb->senseDataLength = sizeof(esense_reply_t);
    }

    srb->srbCmd   = ksrbCmdExecuteReq;
    srb->srbState = ksrbStateCDBDone;

    srb->target = scsiReq->target;
    srb->lun    = scsiReq->lun;

    /*
     * Setup timeout. (250ms ticks)
     */
    if ( scsiReq->timeoutLength )
    {
        srb->srbTimeout  = (scsiReq->timeoutLength * 1000) / kSCSITimerIntervalMS + 1;
    }

    srb->directionMask = (scsiReq->read) ? 0x01000000 : 0x00000000;

    /*
     * Setup the Nexus struct. This part of the SRB is read/written both by the
     * script and the driver.
     */
    nexus->targetParms.target    = srb->target;

    nexus->cdb.ppData = EndianSwap32((u_int32_t)&nexusPhys->cdbData);

    len = cdbLength[scsiReq->cdb.cdb_opcode >> 5];
    if ( len == 0 ) len = scsiReq->cdbLength;

    nexus->cdb.length = EndianSwap32( len );
    nexus->cdbData    = scsiReq->cdb;

    /*
     * Setup SCSI Messages to send on inital selection of the target.
     * Note: A SCSI tag for command-queuing requests is allocated 
     *       when messages are generated.
     */
    srb->srbRequestFlags |= (scsiReq->disconnect)       ? ksrbRFDisconnectAllowed : 0;
    srb->srbRequestFlags |= (!scsiReq->syncDisable)     ? ksrbRFXferSyncAllowed   : 0;
    srb->srbRequestFlags |= (!scsiReq->cmdQueueDisable) ? ksrbRFCmdQueueAllowed   : 0;

    [self Sym8xxCalcMsgs:srb];

    /*
     * Setup initial data transfer list (SGList) 
     */
    nexus->ppSGList   = (SGEntry *)EndianSwap32((u_int32_t)&nexusPhys->sgListData[2]);
    [self Sym8xxUpdateSGList: srb ];

    /*
     * Queue command to I/O Thread and wait for completion.
     */
    [self Sym8xxSendCommand: srb];

    /*
     * If the command timed-out then issue a Maibox abort to clear
     * the request from the target.
     * 
     * Note: We lock the abortBdrSem to insure there is only one abort
     *       active at a time.
     */
    if ( srb->srbCmd == ksrbCmdProcessTimeout )
    {
        [abortBdrSem lock];
        srb->srbCmd = ksrbCmdAbortReq;
        [self Sym8xxSendCommand: srb];
        [abortBdrSem unlock];
    }
    
    /* 
     * Release the tag for the request.
     */
    [self Sym8xxFreeTag: srb];

    /* 
     * Transfer final request status from the SRB to the original request
     */
    IOGetTimestamp( &endTime );
    scsiReq->totalTime        = endTime - startTime;
    scsiReq->driverStatus     = srb->srbSCSIResult;
    scsiReq->scsiStatus       = srb->srbSCSIStatus;
    scsiReq->bytesTransferred = srb->xferDone;

    [self Sym8xxFreeSRB: srb];

    return scsiReq->driverStatus;
}

/*-----------------------------------------------------------------------------*
 * Requests from Blue Box.
 *
 * Note: Hopefully this kludge of having multiple variants of executeRequest
 *       will go away soon!
 *-----------------------------------------------------------------------------*/
- (sc_status_t) executeRequest	: (IOSCSIRequest *) scsiReq
	ioMemoryDescriptor	: (IOMemoryDescriptor *) ioMemoryDescriptor
{
    return [self executeRequest:scsiReq buffer:(void *)ioMemoryDescriptor client:(vm_task_t) -1];
}


/*-----------------------------------------------------------------------------*
 * This routine queues an SRB to reset the SCSI Bus
 *
 *-----------------------------------------------------------------------------*/
- (sc_status_t) resetSCSIBus
{
    SRB			*srb;
    sc_status_t		scsiResult;

    srb = [self Sym8xxAllocSRB];
    if ( srb == NULL )
    {
        return -1;
    }

    srb->srbCmd = ksrbCmdResetSCSIBus;
    [self Sym8xxSendCommand: srb];

    scsiResult = srb->srbSCSIResult;
    [self Sym8xxFreeSRB: srb];

    return scsiResult;
}


/*-----------------------------------------------------------------------------*
 * This routine queues a command on the driver's I/O Thread, wakes up
 * the I/O Thread and then waits for the command to complete. 
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxSendCommand: (SRB *) srb
{
    kern_return_t	kr;

    msg_header_t 	msg = 
    {
        0,				// msg_unused
        1,				// msg_simple
        sizeof(msg_header_t),		// msg_size
        MSG_TYPE_NORMAL,		// msg_type
        PORT_NULL,			// msg_local_port
        PORT_NULL,			// msg_remote_port - TO BE FILLED IN
        IO_COMMAND_MSG			// msg_id
    };

    srb->srbCmdLock = [[NXConditionLock alloc] initWith: ksrbCmdPending];

    [srbPendingQLock lock];
    queue_enter( &srbPendingQ, srb, SRB *, srbQ );
    [srbPendingQLock unlock];

    msg.msg_remote_port = interruptPortKern;
    kr = msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
    if( kr != KERN_SUCCESS )
    {
        goto executeCmd_error;
    }

    [srb->srbCmdLock lockWhen: ksrbCmdComplete];
    [srb->srbCmdLock free];
 
executeCmd_error:
    ;
    return;
}
      
         
/*-----------------------------------------------------------------------------*
 * This routine provides our data alignment/length restrictions to the
 * super class. 
 *
 *-----------------------------------------------------------------------------*/
- (void)getDMAAlignment:(IODMAAlignment *)alignment
{
    alignment->readStart   = 1;
    alignment->writeStart  = 1;
    alignment->readLength  = 1;
    alignment->writeLength = 2;
}

/*-----------------------------------------------------------------------------*
 * This routine returns the number of targets we support.
 *
 *-----------------------------------------------------------------------------*/
- (int) numberOfTargets		
{
    return MAX_SCSI_TARGETS;
}

/*-----------------------------------------------------------------------------*
 * This routine creates SCSI messages to send during the initial connection
 * to the target. It is called during client request processing and also by
 * the I/O thread when a request sense operation is required.
 *
 * Outbound messages are setup in the MsgOut buffer in the Nexus structure of
 * the SRB.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxCalcMsgs: (SRB *)srb 
{
    Nexus		*nexus;
    Nexus		*nexusPhys;
    u_int32_t		msgIndex;
    BOOL		fCmdQueue;
    BOOL		fNegotiateSync;
    BOOL		fNegotiateWide;
    u_int32_t		targetFlags;
    u_int32_t		reqFlags;
    u_int8_t		*xferMsg = NULL;

    nexus     = &srb->nexus;
    nexusPhys = &srb->srbPhys->nexus;

    reqFlags  = srb->srbRequestFlags;

    /*
     * Setup Identify message 
     */
    msgIndex = 0;
    nexus->msg.ppData = EndianSwap32((u_int32_t)&nexusPhys->msgData);
    nexus->msgData[msgIndex++] = srb->lun | (( reqFlags & ksrbRFDisconnectAllowed ) ? 0xC0 : 0x80);

    targetFlags = targets[srb->target].flags;

    /* 
     * Setup Tag message if cmdQueueing is supported.
     *
     * Note: On target flags:
     *             kTFxxxxSupported - Inquiry data indicates the function is supported.
     *             kTFxxxxAllowed   - The function is not explicity disabled for this target.
     *             kRFxxxxAllowed   - The function is not explicitly disabled by the command
     */
    fCmdQueue = ( (targetFlags & kTFCmdQueueSupported) 
                      && (targetFlags & kTFCmdQueueAllowed) 
                            && (reqFlags & ksrbRFCmdQueueAllowed) );

    /*
     * Allocate tag for request.
     *
     * For non-tagged requests a pseudo-tag is created consisting of target*16+lun. For tagged
     * requests a tag in the range 128-255 is allocated.
     *
     * If a pseudo-tag is inuse for a non-tagged command or there are no tags available for
     * a tagged request, then the command is blocked until a tag becomes available.
     *
     * Note: If we are being called during request sense processing (srbState != ksrbStateCDBDone)
     *       then a tag has already been allocated to the request.
     */
    if ( srb->srbState == ksrbStateCDBDone )
    {
        srb->tag = srb->nexus.tag = [self Sym8xxAllocTag:(SRB *)srb CmdQueue:(BOOL)fCmdQueue];
    }

    if ( fCmdQueue )
    {
        nexus->msgData[msgIndex++] = 0x20;
        nexus->msgData[msgIndex++] = srb->nexus.tag;
    }

    /*
     * Setup to negotiate for Wide (16-bit) data transfers
     *
     * Note: There is no provision to negotiate back to narrow transfers although
     *       SCSI does support this.
     */
    fNegotiateWide = (targetFlags & kTFXferWide16Supported) 
                          && (targetFlags & kTFXferWide16Allowed) 
                                && !(targetFlags & kTFXferWide16);
    
    if ( fNegotiateWide )
    {
        srb->srbRequestFlags |= ksrbRFNegotiateWide;
        bcopy( xferMsgWide16, &nexus->msgData[msgIndex], sizeof(xferMsgWide16) );
        msgIndex += sizeof(xferMsgWide16);
    }

    /*
     * Setup to negotiate for Synchronous data transfers.
     *
     * Note: We can negotiate back to async based on the flags in the command. 
     */

    fNegotiateSync = (targetFlags & kTFXferSyncSupported) 
                        && (targetFlags & kTFXferSyncAllowed)
                               && ( ((reqFlags & ksrbRFXferSyncAllowed) != 0) ^ ((targetFlags & kTFXferSync) != 0) ) ;

    if ( fNegotiateSync )
    {
        srb->srbRequestFlags |= ksrbRFNegotiateSync;
        xferMsg = (reqFlags & ksrbRFXferSyncAllowed) ? xferMsgSync : xferMsgAsync;
        bcopy( xferMsg, &nexus->msgData[msgIndex], sizeof(xferMsgSync) );
        msgIndex      += sizeof(xferMsgSync);
    }

    /*
     * If we are negotiating for both Sync and Wide data transfers, we setup both messages
     * in the Nexus msgOut buffer. However, after each message the script needs to wait for
     * a reply message from the target. In this case, we set the msgOut length to include
     * bytes upto the end of the Wide message. When we get the reply from the target, the
     * routine handling the WDTR will setup the Nexus pointers/counts to send the remaining
     * message bytes. See Sym8xxExecute.m(Sym8xxNegotiateWDTR).
     */
    srb->srbMsgLength = msgIndex;

    if ( fNegotiateSync && fNegotiateWide ) msgIndex -= sizeof(xferMsgSync);

    nexus->msg.length = EndianSwap32( msgIndex );
}

/*-----------------------------------------------------------------------------*
 * This routine sets up the data transfer SG list for the client's buffer in the
 * Nexus structure.
 *
 * The SGList actually consists of script instructions. The script will branch
 * to the SGList when the target enters data transfer phase. When the SGList completes
 * it will either execute a script INT instruction if there are more segments of the
 * user buffer that need to be transferred or will execute a script RETURN instruction
 * to return to the script.
 *
 * The first two slots in the SGList are reserved for partial data transfers. See
 * Sym8xxExecute.m(Sym8xxAdjustDataPtrs).
 * 
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxUpdateSGList: (SRB *) srb
{
    BOOL		rc;

    if ( srb->xferClient != (vm_task_t)-1 )
    {
        rc = [self Sym8xxUpdateSGListVirt: srb];
    }
    else
    {
        rc = [self Sym8xxUpdateSGListDesc: srb];
    }
    return rc;
}

/*-----------------------------------------------------------------------------*
 * Build SG list based on a single virtual address range/length
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxUpdateSGListVirt: (SRB *) srb
{
    u_int32_t			offset;
    u_int32_t			physAddr;
    u_int32_t			bytesLeft;
    u_int32_t			bytesOnPage;
    u_int32_t			i;
    u_int32_t			len = 0;
    IOReturn			rc = IO_R_SUCCESS;

    offset    = srb->xferOffset;
    bytesLeft = srb->xferCount - srb->xferOffset;
    i         = 2;

    while ( (bytesLeft > 0) && (i < MAX_SGLIST_ENTRIES-1))
    {

        rc = IOPhysicalFromVirtual(	(vm_task_t) 	srb->xferClient, 	
	    				(vm_address_t)	(srb->xferBuffer+offset), 
	    			  	(u_int32_t *)	&physAddr );

        if ( rc != IO_R_SUCCESS )
        {
            break;
        }

        /*
         * Note: The script instruction(s) to transfer data to/from the scsi bus
         *       have the same format as a typical SGList with the transfer length 
         *       as the first word and the physical transfer address as the second. 
         *       The data transfer direction is specified by a bit or'd into the
         *       high byte of the SG entry's length field.
         */
        srb->nexus.sgListData[i].physAddr = EndianSwap32( physAddr );

        bytesOnPage = page_size - ((srb->xferBuffer + offset) & (page_size - 1));
        len = ( bytesLeft < bytesOnPage ) ? bytesLeft : bytesOnPage;

        srb->nexus.sgListData[i].length = EndianSwap32( len | srb->directionMask );

        bytesLeft -= len;
        offset    += len;
        i++;
    }

    if ( !bytesLeft )
    {
        srb->nexus.sgListData[i].length   = EndianSwap32( 0x90080000 );
        srb->nexus.sgListData[i].physAddr = EndianSwap32( 0x00000000 );
    }
    else
    {
        srb->nexus.sgListData[i].length   = EndianSwap32( 0x98080000 );
        srb->nexus.sgListData[i].physAddr = EndianSwap32( A_sglist_complete );
    }

    srb->xferOffsetPrev = srb->xferOffset;
    srb->xferOffset     = offset;

    return ((rc != IO_R_SUCCESS) ? NO : YES) ;
}

/*-----------------------------------------------------------------------------*
 * Build SG list based on an IOMemoryDescriptor object.
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxUpdateSGListDesc: (SRB *) srb
{

    PhysicalRange 		range;
    u_int32_t			actRanges;
    u_int32_t			offset;
    u_int32_t			bytesLeft;
    u_int32_t			i;
    IOReturn			rc = YES;

    offset    = srb->xferOffset;
    bytesLeft = srb->xferCount - srb->xferOffset;
    i         = 2;

    [(id)srb->xferBuffer setPosition: offset];

    while ( (bytesLeft > 0) && (i < MAX_SGLIST_ENTRIES-1))
    {
        [(id)srb->xferBuffer getPhysicalRanges: 1
                             maxByteCount:      0x00FFFFFF
                    	     newPosition:       &offset
			     actualRanges:      &actRanges
			     physicalRanges:    &range];

        if ( actRanges != 1 )
        {
            rc = NO;
            break;
        }

        /*
         * Note: The script instruction(s) to transfer data to/from the scsi bus
         *       have the same format as a typical SGList with the transfer length 
         *       as the first word and the physical transfer address as the second. 
         *       The data transfer direction is specified by a bit or'd into the
         *       high byte of the SG entry's length field.
         */
        srb->nexus.sgListData[i].physAddr = EndianSwap32( (u_int32_t)range.address );
        srb->nexus.sgListData[i].length   = EndianSwap32( range.length | srb->directionMask );

        bytesLeft -= range.length;
        i++;
    }

    if ( !bytesLeft )
    {
        srb->nexus.sgListData[i].length   = EndianSwap32( 0x90080000 );
        srb->nexus.sgListData[i].physAddr = EndianSwap32( 0x00000000 );
    }
    else
    {
        srb->nexus.sgListData[i].length   = EndianSwap32( 0x98080000 );
        srb->nexus.sgListData[i].physAddr = EndianSwap32( A_sglist_complete );
    }

    srb->xferOffsetPrev = srb->xferOffset;
    srb->xferOffset     = offset;

    return rc;
}


/*-----------------------------------------------------------------------------*
 * This routine allocates a SCSI Tag value for a request. For non-tagged requests
 * a pseudo-tag is generated with the value target*16+lun.
 *
 * If all tags are in-use or a pseudo tag is in-use, the request is blocked until
 * the tag becomes available.
 *
 *-----------------------------------------------------------------------------*/
- (u_int32_t) Sym8xxAllocTag:(SRB *) srb CmdQueue:(BOOL)fCmdQueue
{
    u_int32_t		i;
    u_int32_t		tagIndex;
    u_int32_t		tagMask;

    while ( 1 )
    {
        if ( fCmdQueue )
        {
            for ( i = MIN_SCSI_TAG; i < MAX_SCSI_TAG; i ++ )
            {
                tagIndex = i / 32; 
                tagMask  = 1 << (i % 32);
                if ( !(tags[tagIndex] & tagMask) )
                {
                    tags[tagIndex] |= tagMask;
                    return i;
                }
            }
            /*
             * This semaphore gets unlocked whenever a tag gets returned to the pool. Any
             * requests waiting for a tag will wake-up and try to allocate a tag. If they
             * fail they will return here and will be put back to sleep.
             */
            [cmdQTagSem lock];
        }
        else
        {
            i = ((u_int32_t)srb->target << 3) | srb->lun;
            tagIndex = i / 32;
            tagMask  = 1 << (i % 32); 
            if ( !(tags[tagIndex] & tagMask) )
            {
                tags[tagIndex] |= tagMask;
                return i;
            }
            /*
             * This per-target semaphore gets unlocked whenever a request completes on a target. Any
             * requests pending for this target will wake-up and try to allocate this pseudo-tag. If they
             * fail they will return here and will be put back to sleep.
             */
            [targets[srb->target].targetTagSem lock];    
        }
    }
    return -1;
}

/*-----------------------------------------------------------------------------*
 * This routine frees a previously allocates SCSI tag. It unlocks the appropriate
 * semaphore based on the type of tag returned.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxFreeTag:(SRB *) srb
{
    u_int32_t		i;

    i = srb->tag;
    tags[i/32] &= ~(1 << (i % 32));

    if ( i < MIN_SCSI_TAG )
    {
        [targets[srb->target].targetTagSem unlock];
    }
    else
    {
        [cmdQTagSem unlock];
    }
}  


/*-----------------------------------------------------------------------------*
 * This routine maintains a list of pages which are divided up into SRB sized
 * allocations. The list of pages is grown or shrunk as needed.
 * 
 * The reason we dont use the driverKit IOMalloc function is that it does not
 * guarantee that allocations will not cross page boundaries. The driver does 
 * require this since the script accesses memory based on physical rather than
 * virtual addresses. 
 *
 *-----------------------------------------------------------------------------*/
- (SRB *) Sym8xxAllocSRB
{
    SRBPool		*pSRBPool;
    SRB			*pSRB = NULL;

    do
    {
        /* 
         * We hold the srbPoolLock when we are searching or changing the SRB pool 
         * data structures
         */
        [srbPoolLock lock];

        /*
         * Search the list of pages currently in the SRB pool until we find a page
         * with at least one free SRB to allocate.
         */
        pSRBPool = (SRBPool *) queue_first( &srbPool );
        while (!queue_end( &srbPool, &pSRBPool->nextPage ) )
        {
            if ( !queue_empty( &pSRBPool->freeSRBList ) )
            {
                pSRBPool->srbInUseCount++;
                queue_remove_first( &pSRBPool->freeSRBList, pSRB, SRB *, srbQ );
                break;
            }
            pSRBPool = (SRBPool *)queue_next( &pSRBPool->nextPage );
        }
    
        [srbPoolLock unlock];

        if ( pSRB )
        {
            bzero( pSRB, sizeof(SRB) );
            pSRB->srbPhys = (SRB *)(pSRBPool->pagePhysAddr + (uint)pSRB - (uint)pSRBPool);
            pSRB->srbSeqNum = ++srbSeqNum;            
            break;
        }

        /*
         * If we can find no available SRBs, we unlock a thread to grow the SRB pool and
         * block this request until the pool grow operation completes. When our thread runs
         * again it will retry the SRB allocation.
         */
        if ( srbPoolGrow == NO )
        {
            srbPoolGrow = YES;
            [srbPoolGrowLock unlockWith: kSRBGrowPoolRunning];
        }

        [srbPoolGrowLock lockWhen:   kSRBGrowPoolIdle];
        [srbPoolGrowLock unlockWith: kSRBGrowPoolIdle];        
    }
    while ( 1 );
    
    return pSRB;
}         

/*-----------------------------------------------------------------------------*
 * This routine returns SRBs to the SRB pool.
 *
 * The page in the pool containing the SRB is located and the
 * SRB is added to that page's SRB free list.
 *
 * The pool is then scanned for pages with no SRBs allocated.
 * If more than two pages are found with zero SRBs allocate, the 
 * additional idle pages are returned to the kernel.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxFreeSRB: (SRB *) pSRB
{
    SRB				*srbMin, *srbMax;
    SRBPool			*pSRBPool, *pSRBPoolNext;
    u_int32_t			numSRBs;
    kern_return_t		kr;
    u_int32_t			idlePageCount = 0;

    [srbPoolLock lock];

    numSRBs = (page_size - sizeof(SRBPool)) / sizeof(SRB);

    /* 
     * Scan the pool for a page containing the returned SRB
     */
    pSRBPool = (SRBPool *) queue_first( &srbPool );
    while (!queue_end( &srbPool, &pSRBPool->nextPage ) )
    {
        srbMin = (SRB *) (pSRBPool+1);
        srbMax = &srbMin[numSRBs-1];

        if ( pSRB >= srbMin && pSRB <= srbMax )
        {
            pSRBPool->srbInUseCount--;
            queue_enter( &pSRBPool->freeSRBList, pSRB, SRB *, srbQ );
            break;
        }    
        pSRBPool = (SRBPool *)queue_next( &pSRBPool->nextPage );
    }

    /*
     * If we fell off the end of the SRB Pool page list without finding
     * the owning page, we have a bug.
     */
    if ( queue_end( &srbPool, &pSRBPool->nextPage ) )
    {
        kprintf("Sym8xxFreeSRB: Bad SRB returned = %08x\n\r", (u_int32_t)pSRB );
    }

    /*
     * We scan the SRBPool page list again looking for pages with no SRBs inuse.
     * If more than idle pool pages are found, we release the remaining pages to
     * the kernel.
     */
    pSRBPool = (SRBPool *) queue_first( &srbPool );
    while (!queue_end( &srbPool, &pSRBPool->nextPage ) )
    {
        pSRBPoolNext = (SRBPool *)queue_next( &pSRBPool->nextPage );

        if ( !pSRBPool->srbInUseCount )
        {
            if ( ++idlePageCount > kSRBPoolMaxFreePages )
            {
                queue_remove( &srbPool, pSRBPool, SRBPool *, nextPage );

//              kprintf("SCSI(Symbios8xx): Sym8xxShrinkSRBPool\n\r");

                kr = kmem_free(IOVmTaskSelf(), (vm_offset_t) pSRBPool, page_size );
                if ( kr != KERN_SUCCESS )
                {
                    IOPanic("SCSI(Symbios8xx): kmem_free failed - Help me\n\r");
                }
            }    
        }           
        pSRBPool = pSRBPoolNext;
    }

    [srbPoolLock unlock];

}

/*-----------------------------------------------------------------------------*
 * This routines grows the SRBPool. It runs on its own thread to avoid pager deadlocks.
 * 
 * We need this entry thunk since the thread creation routines dont support objC 
 * interfaces directly.
 *
 *-----------------------------------------------------------------------------*/
IOThreadFunc Sym8xxGrowSRBPool( Sym8xxController *controller )
{
    [controller Sym8xxGrowSRBPool];
    return NULL;
}

- (void) Sym8xxGrowSRBPool
{
    SRBPool			*pSRBPool;
    SRB				*pSRB;
    kern_return_t		kr;
    u_int32_t			numSRBs;
    u_int32_t			i;

    while ( 1 )
    {
        [srbPoolGrowLock lockWhen: kSRBGrowPoolRunning];
 
//      kprintf("SCSI(Symbios8xx): Sym8xxGrowSRBPool\n\r");

        kr = kmem_alloc_wired(IOVmTaskSelf(), (vm_offset_t *) &pSRBPool, page_size );
        if ( kr != KERN_SUCCESS )
        {
            IOPanic("kmem_alloc_wired failed - Help me\n\r");
        }

        IOPhysicalFromVirtual((vm_task_t)IOVmTaskSelf(), (vm_offset_t)pSRBPool, (vm_offset_t *)&pSRBPool->pagePhysAddr );
    
        pSRBPool->srbInUseCount = 0;

        numSRBs = (page_size - sizeof(SRBPool)) / sizeof(SRB);
        pSRB    = (SRB *) (pSRBPool+1);
        
        queue_init( &pSRBPool->freeSRBList );
        for ( i=0; i < numSRBs; i++ )
        {
            queue_enter( &pSRBPool->freeSRBList, (pSRB+i), SRB *, srbQ );
        }

        [srbPoolLock lock];
        queue_enter( &srbPool, pSRBPool, SRBPool *, nextPage );
        [srbPoolLock unlock];

        srbPoolGrow = NO;
        [srbPoolGrowLock unlockWith: kSRBGrowPoolIdle];
    }
}


/*-----------------------------------------------------------------------------*
 * This routine interfaces between the system timer and our I/O Thread. It
 * sends a message to the IOThread to run the -timeoutOccurred routine which
 * does various timing functions for the driver. See Sym8xxExecuteRequest(timeoutOccurred).
 *
 *-----------------------------------------------------------------------------*/
IOThreadFunc Sym8xxTimerReq( Sym8xxController *device )
{
    msg_header_t	msg = { 0 };

    msg.msg_size = sizeof (msg);
    msg.msg_remote_port = device->interruptPortKern;
    msg.msg_id =  IO_TIMEOUT_MSG;
	
    msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);

    return NULL;
}

@end
