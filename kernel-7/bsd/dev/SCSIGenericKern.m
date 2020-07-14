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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * SCSIGenericKern.m - UNIX front end for SCSIGeneric device. 
 *
 * HISTORY
 * 19-Aug-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
/*
 * Note that this file builds with KERNEL_PRIVATE and !MACH_USER_API.
 */
#import <sys/param.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <bsd/dev/scsireg.h>

#import <vm/vm_kern.h>

#import <driverkit/SCSIGeneric.h>
#import <driverkit/SCSIGenericPrivate.h>
#import <driverkit/scsiTypes.h>
#import <driverkit/xpr_mi.h>
#import <driverkit/align.h>
#import <kernserv/ns_timer.h>

/*
 * Map minor number to an id of a SCSIGeneric instance.
 */
id sgIdMap[NUM_SG_DEV];

/*
 * Index of our entry in cdevsw.
 */
static int sg_major;

/*
 * prototypes of static functions.
 */
static id sgDevToId(dev_t dev);
static int sg_doiocreq(id devId, scsi_req_t *srp, scsi3_req_t *s3rp);

/*
 * cdevsw callouts.
 */
 
int sgopen(dev_t dev, 
	int oflags, 
	int devtype, 
	struct proc *pp)
{
	id devId = sgDevToId(dev);
	
	xpr_sdev("sgopen: unit %d\n", minor(dev), 2,3,4,5);
	if(devId == nil) {
		return ENXIO;
	}
	
	/*
	 * Note we use devId as "caller" since we're not an object; this 
	 * works since "caller" isn't used for any authentication.
	 */
	if([devId acquire:devId]) {
		return EBUSY;
	}
	else {
		return 0;
	}
}

int sgclose(dev_t dev,
	int fflag,
	int devtype,
	struct proc *pp)
{
	id devId = sgDevToId(dev);
	
	xpr_sdev("sgclose: unit %d\n", minor(dev), 2,3,4,5);
	if(devId == nil) {
		return ENXIO;
	}
	[devId release:devId];
}

#define SCSI3_DEBUG	0

int
sgioctl(dev_t dev, 
	u_long cmd, 		/* SGIOCREQ, etc */
	caddr_t data, 		/* actually a ptr to scsi_req, if used */
	int flag,		/* for historical reasons. Not used. */
	struct proc *p)
{
	id devId = sgDevToId(dev);
	
	scsi_req_t *srp = (scsi_req_t *)data;
	scsi_adr_t *sap = (scsi_adr_t *)data;
	scsi3_req_t *s3rp = (scsi3_req_t *)data;
	scsi3_adr_t *s3ap = (scsi3_adr_t *)data;
	int rtn = 0;
	
	xpr_sdev("sgioctl: unit %d\n", minor(dev), 2,3,4,5);
	if(devId == nil) {
		return ENXIO;
	}
	switch (cmd) {
	    case SGIOCSTL:			/* set target/lun */
	    	if([devId setTarget:sap->sa_target
				lun:sap->sa_lun
				isRoot:suser(p->p_ucred, &p->p_acflag) ? 0 : 1]) {
			rtn = EACCES;
		}
		break;

	    case SGIOCCNTR:
		if([devId setController:*(int *)data]) {
			rtn = ENODEV;
		}
		break;

	    case SGIOCSTL3:			/* set SCSI3 target/lun */
	    	#if	SCSI3_DEBUG
		IOLog("SGIOCSTL3: s3a_target = 0x%x:%x s3a_lun = 0x%x:%x\n",
			(unsigned)(s3ap->s3a_target >> 32),
			(unsigned)(s3ap->s3a_target),
			(unsigned)(s3ap->s3a_lun >> 32),
			(unsigned)(s3ap->s3a_lun));
		#endif	SCSI3_DEBUG
	    	if([devId setSCSI3Target:s3ap->s3a_target
				lun:s3ap->s3a_lun
				isRoot:suser(p->p_ucred, &p->p_acflag) ? 0 : 1]) {
			rtn = EACCES;
		}
		break;

	    case SGIOCGTL3:
	    	s3ap->s3a_target = [devId SCSI3_target];
		s3ap->s3a_lun    = [devId SCSI3_lun];
	    	#if	SCSI3_DEBUG
		IOLog("SGIOCGTL3: s3a_target = 0x%x:%x s3a_lun = 0x%x:%x\n",
			(unsigned)(s3ap->s3a_target >> 32),
			(unsigned)(s3ap->s3a_target),
			(unsigned)(s3ap->s3a_lun >> 32),
			(unsigned)(s3ap->s3a_lun));
		#endif	SCSI3_DEBUG
		break;
		
	    case SGIOCREQ:
		rtn = sg_doiocreq(devId, srp, NULL);
		break;
		
	    case SGIOCREQ3:
		rtn = sg_doiocreq(devId, NULL, s3rp);
		break;
		
	    case SGIOCENAS:			/* enable auto sense */
		if([devId enableAutoSense]) {
			rtn = EINVAL;
		}
		break;
		
	    case SGIOCDAS: 			/* disable auto sense */
		if([devId disableAutoSense]) {
			rtn = EINVAL;
		}
		break;
		
	    case SGIOCGAS:
	    	*(int *)data = ([devId autoSense] ? 1 : 0);
		break;
		
	    case SGIOCMAXDMA:
	    	*(int *)data = [[devId controller] maxTransfer];
		break;
		
	    case SGIOCNUMTARGS:
	    	*(int *)data = [[devId controller] numberOfTargets];
		break;

	    case SGIOCRST:
#ifndef	DEBUG
		if (suser(p->p_ucred, &p->p_acflag)) {			
				/* this is too dangerous for
				 * the hoi polloi */
			rtn = EPERM;
			break;
		}
#endif	DEBUG
		if([devId resetSCSIBus]) {	/* what could go wrong? */
			rtn = EIO;
		}
		break;

	    default:
		rtn = EINVAL;
	}
	xpr_sdev("sgioctl: returning %d\n", rtn, 2,3,4,5);
	return rtn;
} /* sgioctl() */

/*
 * Static functions.
 */

/* 
 * Obtain id for SCSIGeneric instance associated with specified dev_t.
 * A return of nil indicates ENXIO.
 */
static id sgDevToId(dev_t dev)
{
	int minor = minor(dev);
	
	if((minor < 0) || (minor >= NUM_SG_DEV)) {
		return nil;
	}
	return sgIdMap[minor];
}

/* 
 * Execute one scsi_req. Called from client's task context. Returns an errno.
 * Exactly one of srp and s3rp must be non-NULL.
 */

/*
 * FIXME - DMA to non-page-aligned user memory doesn't work. There
 * is data corruption on read operations;l the corruption occurs on page
 * boundaries. 
 */
#define FORCE_PAGE_ALIGN	1
#if	FORCE_PAGE_ALIGN
int forcePageAlign = 1;
#endif	FORCE_PAGE_ALIGN

static int sg_doiocreq(id devId, scsi_req_t *srp, scsi3_req_t *s3rp)
{
	void 		*alignedPtr;
	unsigned 	alignedLen = 0;
	void 		*freePtr;
	unsigned 	freeLen;
	BOOL 		didAlign = NO;
	vm_task_t	client = NULL;
	int		rtn = 0;
	sc_status_t	srtn;
	int		dma_max;
	sc_dma_dir_t	dma_dir;
	caddr_t		dma_addr;
	unsigned	bytesTransferred;
	
	if(srp) {
		dma_max  = srp->sr_dma_max;
		dma_dir  = srp->sr_dma_dir;
		dma_addr = srp->sr_addr;
	}
	else if(s3rp) {
		dma_max  = s3rp->s3r_dma_max;
		dma_dir  = s3rp->s3r_dma_dir;
		dma_addr = s3rp->s3r_addr;
	}
	else {
		IOPanic("sg_doiocreq: no scsi_req ptr");
	}
	if(dma_max > [[devId controller] maxTransfer]) {
		return EINVAL;
	}
	
	/* Get some well-aligned memory if necessary. By using 
	 * allocateBufferOfLength we guarantee that there is enough space 
	 * in the buffer we pass to the controller to handle 
	 * end-of-buffer alignment, although we won't copy more 
	 * than sr_dma_max to or from the  caller.
	 */
	if(dma_max != 0) {

		IODMAAlignment dmaAlign;
		id controller = [devId controller];
		unsigned alignLength;
		unsigned alignStart;

		/*
		 * Get appropriate alignment from controller.
		 */
		[controller getDMAAlignment:&dmaAlign];
		if(dma_dir == SR_DMA_WR) {
			alignLength = dmaAlign.writeLength;
			alignStart  = dmaAlign.writeStart;
		}
		else {
			alignLength = dmaAlign.readLength;
			alignStart  = dmaAlign.readStart;
		}
#if		FORCE_PAGE_ALIGN
		if(forcePageAlign) {
			alignStart = PAGE_SIZE;
		}
#endif		FORCE_PAGE_ALIGN
		if (1) {	/* XXX fix the well aligned case for 3.3 */

			/* 
			 * DMA from kernel memory, we allocate and copy.
			 */
			
			didAlign = YES;
			client = kernel_map;
			
			if(alignLength > 1) {
				alignedLen = IOAlign(unsigned,
						dma_max,
						alignLength);
			}
			else {
				alignedLen = dma_max;
			}	
			alignedPtr = [controller allocateBufferOfLength:
						dma_max
					actualStart:&freePtr
					actualLength:&freeLen];
			if(dma_dir == SR_DMA_WR) {
				rtn = copyin(dma_addr, alignedPtr, dma_max);
				if(rtn) {
				    xpr_sd(" ...copyin() returned %d\n",
					    rtn, 2,3,4,5);
				    rtn = EFAULT;
				    goto err_exit;
				}
			}
		}
		else {
			/*
			 * Well-aligned buffer, DMA directly to/from user 
			 * space.
			 */
			alignedLen = dma_max;
			alignedPtr = dma_addr;
			client = current_task()->map;
			didAlign = NO;
		}
		
	} else {
		alignedPtr = dma_addr;
	}
	

	/*
	 * Generate a contemporary version of scsi_req.
	 */
	if(srp) {
	    IOSCSIRequest	scsiReq;
	    unsigned long long 	addr;
	    
	    bzero(&scsiReq, sizeof(scsiReq));
	    addr = [devId SCSI3_target];
	    
	    /*
	     * Can only deal with SCSI-2 style addressing
	     * (32 targets, 8 LUNs).
	     */
	    if(addr >= SCSI3_NTARGETS) {
	    	rtn = EINVAL;
		goto err_exit;
	    }
	    scsiReq.target = (unsigned char)addr;
	    addr = [devId SCSI3_lun];
	    if(addr >= SCSI_NLUNS) {
	    	rtn = EINVAL;
		goto err_exit;
	    }
	    scsiReq.lun = (unsigned char)addr;
	    
	    /*
	     * Careful. this assumes that the old and new cdb structs are
	     * equivalent...
	     */
	    scsiReq.cdb 		= srp->sr_cdb;
	    scsiReq.read 		= (srp->sr_dma_dir == SR_DMA_RD) 
	    					? YES : NO;
	    scsiReq.maxTransfer 	= alignedLen;
	    scsiReq.timeoutLength 	= srp->sr_ioto;
	    scsiReq.disconnect	        = srp->sr_discon_disable ? 0 : 1;
	    scsiReq.cmdQueueDisable	= srp->sr_cmd_queue_disable;
	    scsiReq.syncDisable 	= srp->sr_sync_disable;
	    scsiReq.cdbLength 	        = srp->sr_cdb_length;
	    
	    
	    /*
	     * Go for it.
	     */
	    srtn = [devId executeRequest : &scsiReq
			          buffer : alignedPtr
			          client : client
			        senseBuf : &srp->sr_esense];
	    
	    /*
	     * Copy status back to user. Note that if we got this far, we
	     * return good status from the function; errors are in 
	     * srp->sr_io_status.
	     */
	    srp->sr_io_status = srtn;
	    srp->sr_scsi_status = scsiReq.scsiStatus;
	    bytesTransferred = srp->sr_dma_xfr = scsiReq.bytesTransferred;
	    if(srp->sr_dma_xfr > srp->sr_dma_max) {
		    srp->sr_dma_xfr = srp->sr_dma_max;
	    }
	    ns_time_to_timeval(scsiReq.totalTime, &srp->sr_exec_time);
	}
	else {
	    IOSCSI3Request	scsi3Req;

	    bzero(&scsi3Req, sizeof(scsi3Req));
	    scsi3Req.target = [devId SCSI3_target];
	    scsi3Req.lun    = [devId SCSI3_lun];
	    
	    /*
	     * Careful. this assumes that the old and new cdb structs are
	     * equivalent...
	     */
	    scsi3Req.cdb 		= s3rp->s3r_cdb;
	    scsi3Req.read 		= (s3rp->s3r_dma_dir == SR_DMA_RD) 
	    					? YES : NO;
	    scsi3Req.maxTransfer 	= alignedLen;
	    scsi3Req.timeoutLength 	= s3rp->s3r_ioto;
	    scsi3Req.disconnect	        = s3rp->s3r_discon_disable ? 0 : 1;
	    scsi3Req.cmdQueueDisable	= s3rp->s3r_cmd_queue_disable;
	    scsi3Req.syncDisable 	= s3rp->s3r_sync_disable;
	    scsi3Req.cdbLength 	        = s3rp->s3r_cdb_length;
	    
	    
	    /*
	     * Go for it.
	     */
	    srtn = [devId executeSCSI3Request : &scsi3Req
			               buffer : alignedPtr
			               client : client
			             senseBuf : &srp->sr_esense];
	    
	    /*
	     * Copy status back to user. Note that if we got this far, we
	     * return good status from the function; errors are in 
	     * s3rp->sr_io_status.
	     */
	    s3rp->s3r_io_status = srtn;
	    s3rp->s3r_scsi_status = scsi3Req.scsiStatus;
	    bytesTransferred = s3rp->s3r_dma_xfr = scsi3Req.bytesTransferred;
	    if(s3rp->s3r_dma_xfr > s3rp->s3r_dma_max) {
		    s3rp->s3r_dma_xfr = s3rp->s3r_dma_max;
	    }
	    ns_time_to_timeval(scsi3Req.totalTime, &s3rp->s3r_exec_time);
	}
	
	/*
	 * Copy read data back to user if appropriate.
	 */
	if((dma_dir == SR_DMA_RD) && 
	   (bytesTransferred != 0) &&
	    didAlign) {
		rtn = copyout(alignedPtr, dma_addr, bytesTransferred);
	}
err_exit:
	if(didAlign) {
		IOFree(freePtr, freeLen);
	}
	return rtn;
}



