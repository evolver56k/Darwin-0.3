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
 * volCheck.m - common volume check logic for all DiskObjects.
 *
 * This only does useful work in the kernel, since it relies heavily
 * on the vol driver functionality. Someday we'll have to come up 
 * with a new interface bewteen WS and a user-level version of this.
 *
 * HISTORY
 * 07-May-91    Doug Mitchell at NeXT
 *      Created.
 */
#import <driverkit/IODisk.h>
#import <driverkit/IODiskPartition.h>
#import <machkit/NXLock.h>
#import <kernserv/queue.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/IODeviceDescription.h>
#import <bsd/dev/disk.h>

#undef	DRIVER_PRIVATE
#define DRIVER_PRIVATE

#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <bsd/dev/voldev.h>
#else	KERNEL
#import <bsd/libc.h>
#import "volDriver.h"
#endif	KERNEL

#import <driverkit/volCheck.h>
#import "volCheckPrivate.h"

/*
 * Static data.
 */
/*
 * Queue of volCheckEntry_t's. One per removable disk drive. It's not
 * protected with a lock; all accesses to this queue are by the 
 * volCheckThread.
 */
static queue_head_t volCheckEntryQ;

/*
 * Input queue. Commands are queued here by public routines to ensure 
 * single-threaded response to change in device state.
 */
static queue_head_t volCheckCmdQ;
static id volCheckCmdLock;
 
/*
 * Prototypes of static functions.
 */
static void volCheckThread(void *foo);
static void volCheckResponse(void *param, 
	int tag, 
	int response_value);
static volCheckEntry_t *getVCEntry(id diskObj);
static void vcEnqueueCmd(volCheckOp_t op,
	id diskObj,
	int diskType,
	dev_t blockDev,
	dev_t rawDev);
static void volCheckCmdHandler();
static void volDriverNotify(id diskObj,
	dev_t blockDev,
	dev_t rawDev);

/*
 * Public functions.
 */
 
/*
 * One-time only initialization.
 */
void volCheckInit()
{
	volCheckCmdLock = [NXSpinLock new];
	queue_init(&volCheckEntryQ);
	queue_init(&volCheckCmdQ);
	
	/*
	 * FIXME - only start up a thread when we get the first 
	 * volCheckRegister().
	 */
	IOForkThread((IOThreadFunc)volCheckThread, NULL);
#ifndef	KERNEL
	volDriverInit();
#endif	KERNEL
}

/*
 * These public routines queue up a command on volCheckCmdQ to ensure
 * single-threaded handling of lastReadyState transitions.
 */
/*
 * Register a removable media drive. This should be called once for the
 * physical disk object, not for any LogicalDisks which sit on top of it.
 */
void volCheckRegister(id diskObj,
	dev_t blockDev,
	dev_t rawDev)
{	
	xpr_vc("volCheckRegister: disk %s\n", [diskObj name], 2,3,4,5);
	if(![diskObj isPhysical]) {
		IOLog("volCheckRegister: %s is not a physical device\n",
			[diskObj name]);
		return;
	}
	vcEnqueueCmd(VC_REGISTER,
		diskObj,
		0,
		blockDev,
		rawDev);
}

/*
 * Unregister a removable media drive. This probably should never happen,
 * at least not in the kernel...
 */
void volCheckUnregister(id diskObj)
{
	xpr_vc("volCheckUnregister: disk %s\n", 
		[diskObj name], 2,3,4,5);
	vcEnqueueCmd(VC_UNREGISTER,
		diskObj,
		0,
		0,
		0);
}

/*
 * Request a "please insert disk" panel. Called by physDevice DiskObject.
 *
 * For now, we don't have a connection with WS unless we're in the 
 * kernel.
 */
void volCheckRequest(id diskObj,		// requestor
	int diskType)				// PR_DRIVE_FLOPPY, etc.
{
	xpr_vc("volCheckRequest: disk %s\n", 
		[diskObj name], 2,3,4,5);
	vcEnqueueCmd(VC_REQUEST,
		diskObj,
		diskType,
		0,
		0);
}

/*
 * Notify volCheck that specified device is in a "disk ejecting" state.
 * Called by physDevice DiskObject in ejectPhysical:. Only necessary 
 * for drives which may not respond to autoeject. Thus floppy drivers shouldn't
 * have to call this.
 *
 * NOTE: we force the low-level driver to pass in diskType in this call
 * and in volCheckRequest() to avoid having DiskObject know this 
 * detail at registerDisk: time...
 */
void volCheckEjecting(id diskObj,
	int diskType)				// PR_DRIVE_FLOPPY, etc.
{
	xpr_vc("volCheckEjecting: disk %s\n", [diskObj name], 2,3,4,5);
	
	vcEnqueueCmd(VC_EJECTING,
		diskObj,
		diskType,
		0,
		0);
}

/*
 * Notify volCheck that disk has gone not ready. Typically called on
 * gross error detection.
 */
void volCheckNotReady(id diskObj)
{
	xpr_vc("volCheckNotReady: disk %s\n", [diskObj name], 2,3,4,5);
	vcEnqueueCmd(VC_NOTREADY,
		diskObj,
		0,
		0,
		0);
}

/*
 * Private functions.
 */
/*
 * Get the volCheckEntry_t for specified DiskObject id.
 */
static volCheckEntry_t *getVCEntry(id diskObj)
{
	volCheckEntry_t *vcEntry;
	
	vcEntry = (volCheckEntry_t *)queue_first(&volCheckEntryQ);
	while(!queue_end(&volCheckEntryQ, (queue_t)vcEntry)) {
		if(vcEntry->diskObj == diskObj)
			return(vcEntry);
		vcEntry = (volCheckEntry_t *)vcEntry->link.next;
	}
	xpr_err("getVCEntry: ENTRY NOT FOUND\n", 1,2,3,4,5);
	return(NULL);
}

/*
 * Enqueue a command on volCheckCmdQ. Called by public routines.
 */
static void vcEnqueueCmd(volCheckOp_t op,
		id diskObj,
		int diskType,
		dev_t blockDev,
		dev_t rawDev)
{
	volCheckCmd_t *vcCmd;
	
	vcCmd           = IOMalloc(sizeof(*vcCmd));
	vcCmd->op       = op;
	vcCmd->diskObj  = diskObj;
	vcCmd->diskType = diskType;
	vcCmd->blockDev = blockDev;
	vcCmd->rawDev   = rawDev;
	[volCheckCmdLock lock];
	queue_enter(&volCheckCmdQ,
		vcCmd,
		volCheckCmd_t *,
		link);
	[volCheckCmdLock unlock];
}

/*
 * Called by vol driver upon reception of panel response. The only 
 * response we know about is an abort...do we need others? All we do
 * is forward this notification to the appropriate DiskObject.
 */
static void volCheckResponse(void *param, 	
	int tag, 
	int response_value)
{
	id diskObj = (id)param;
	
	xpr_vc("volCheckResponse disk %s\n", [diskObj name], 2,3,4,5);
	vcEnqueueCmd(VC_RESPONSE,
		diskObj,
		0,
		0,
		0);
}

/*
 * volCheck thread function. Max of two per system (one in the kernel,
 * one in user space).
 */
static void volCheckThread(void *foo)
{

	volCheckEntry_t *vcEntry;
	IODiskReadyState lastReadyState;
	IODiskReadyState newReadyState = IO_NotReady;	// damned compiler!
	const char *devname;
	id diskObj;
	int manualPoll;
	
	while(1) {
	
	    /*
	     * First handle any queued commands.
	     */
	    volCheckCmdHandler();
	    
	    /*
	     * Process each registered disk.
	     */
	    vcEntry = (volCheckEntry_t *)queue_first(&volCheckEntryQ);
	    manualPoll = vol_check_manual_poll();
	    while(!queue_end(&volCheckEntryQ, (queue_t)vcEntry)) {
	    
		devname = [vcEntry->diskObj name];
		diskObj = vcEntry->diskObj;
		lastReadyState = [diskObj lastReadyState];
		if(lastReadyState != IO_Ready) {
			/*
			 * Poll for transition if:
			 * -- this is a normal once-per-second polling 
			 *    drive, or
			 * -- vol driver tells us it's time to poll, or
			 * -- ready state is Ejecting
			 */
			if(![vcEntry->diskObj needsManualPolling] ||
					manualPoll ||
					(lastReadyState == IO_Ejecting)) {
				newReadyState = [diskObj updateReadyState];
			}
			else {
				/*
				 * Don't poll; assume state unchanged.
				 */
				newReadyState = lastReadyState;
			}   
		}
	        switch(lastReadyState) {
		    case IO_Ready:
		    	break;		// nothing to do here
			
		    case IO_Ejecting:
			/*
			 * We hope to see a transition to not ready or
			 * no disk...when we see "ready" enough times,
			 * we'll ask the WS to put up a "please eject disk"
			 * panel.
			 */
			if(newReadyState == IO_Ready) {
			    if(--vcEntry->ejectCounter == 0) {
				xpr_vc("volCheck: requesting disk eject"
				    " panel for disk %s\n", devname,
				    2,3,4,5);
				vol_panel_request(NULL,     // no callback
				    PR_RT_EJECT_REQ,
				    PR_RT_CANCEL,	    // leave up 'til
							    // we take it
							    // down
				    0,		            // p1, not used
				    vcEntry->diskType,      // p2
				    [diskObj unit],	    // p3
				    0,		     	    // p4, not used
				    "",		     	    // string1
				    "",		     	    // string2
				    NULL,		    // no callback
				    &vcEntry->tag);
				vcEntry->ejectRequestPending = YES;
				
			    }   	/* counter 0 */
			}		/* still ready */
			else {
			
			    /*
			     * Allright! It went not ready. Remove 
			     * eject request panel if present.
			     */
			    xpr_vc("volCheck: %s: Eject Complete\n",
				    devname, 2,3,4,5);
			    [diskObj setLastReadyState:IO_NoDisk];
			    if(vcEntry->ejectRequestPending) {
				vcEntry->ejectRequestPending  = NO;
				vol_panel_remove(vcEntry->tag);
			    }
			}		/* not ready/not present */
			break;		/* from case IO_Ejecting */
						
		    case IO_NotReady:
		    case IO_NoDisk:
		    	/*
			 * The drive wasn't ready the last time we checked.
			 * We're interested in transitions to a ready state,
			 * indicating disk insertion. 
			 */
			if(newReadyState == IO_Ready) {
			    
			    id descr;
			    
			    [diskObj setLastReadyState:IO_Ready];
			    if(vcEntry->diskRequestPending) {
			    	/*
				 * Cancel the panel.
				 */
				xpr_vc("volCheck: cancelling request panel"
				    " for %s\n", devname, 2,3,4,5);
				vol_panel_remove(vcEntry->tag);
			    }
			
			   /* 
			    * Have the driver get other physical parameters.
			    */
			   [diskObj updatePhysicalParameters];

			    /*
			     * Notify driver whether or not we think it
			     * is waiting for this disk to cover race 
			     * conditions between panels and insertions. 
			     * Then take care of layering some 
			     * IODiskPartitions on top of this new disk.
			     * We have to do this before a possible 
			     * vol_notify_dev().
			     */
			    [diskObj diskBecameReady];
			    descr = [IODeviceDescription new];
			    [descr setDirectDevice:diskObj];
			    if(![IODiskPartition probe:descr]) {
				    [descr free];
			    }

			    if(vcEntry->diskRequestPending) {
			    	vcEntry->diskRequestPending = NO;
			    }
			    else {
			    	/*
				 * Unexpected disk insertion. Notify WS.
				 * We are the one to determine whether it has
				 * a valid label. We also have to gather
				 * other cruft like write protect for use by
				 * WS.
				 */
				volDriverNotify(diskObj, vcEntry->blockDev,
					vcEntry->rawDev);
					
			    }	/* new disk insertion */
			} 	/* new state == ready */
				/* else no action (wasn't ready, still 
				 * isn't) */
			break;	/* from case not ready/no disk */
		}		/* switch lastReadyState */

	    	/*
		 * Next disk.
		 */
	        vcEntry = (volCheckEntry_t *)vcEntry->link.next;
	    }
	    
	    /*
	     * OK, sleep a while.
	     */
	    IOSleep(1000);
	    
	} /* while 1 */
	
	/* NOT REACHED */
	 
}

/*
 * Handle queued commands.
 */
static void volCheckCmdHandler()
{
	volCheckCmd_t *vcCmd;
	volCheckEntry_t *vcEntry = NULL;
	id diskObj;
	IODiskReadyState readyState;
	
	[volCheckCmdLock lock];
	while(!queue_empty(&volCheckCmdQ)) {
		vcCmd = (volCheckCmd_t *)queue_first(&volCheckCmdQ);
		queue_remove(&volCheckCmdQ,
			vcCmd,
			volCheckCmd_t *,
			link);
		[volCheckCmdLock unlock];
		diskObj = vcCmd->diskObj;
		if(vcCmd->op != VC_REGISTER) {
			vcEntry = getVCEntry(diskObj);
			if(vcEntry == NULL) {
				IOLog("volCheck: disk %s not "
					"registered, cmd = %d\n",
					[diskObj name], vcCmd->op);
				goto freeCmd;
			}

		}
		switch(vcCmd->op) {
		    case VC_REGISTER:
			xpr_vc("volCheck: registering disk %s\n", 
				[diskObj name], 2,3,4,5);
#if 1
			/*
			 * We should use the current state since that
			 * indicates whether or the disk has been probed
			 * at registration. It may have come ready after
			 * probe, but before we get here.
			 */
			readyState = [diskObj lastReadyState];
#else
			readyState = [diskObj updateReadyState];
			/* 
			 * IODiskPartition does this too, but we have to do
			 * it here as well in case we run before 
			 * IODiskPartition does this.
			 */
			[diskObj setLastReadyState:readyState];
#endif
			
			/* 
			 * If ready now, go ahead and notify vol driver. 
			 */
			if(readyState == IO_Ready) {
				/*
				 * If this disk is not removable and 
				 * it's currently ready, we have no 
				 * reason to do anything other than 
				 * the initial notify.
				 */
				volDriverNotify(diskObj, vcCmd->blockDev, 
						vcCmd->rawDev);
					

				if(![diskObj isRemovable]) {
					/* 
			 		 * If ready now, go ahead and 
					 * notify vol driver. 
			 		 */
					break;
				};
			}
			
			vcEntry = IOMalloc(sizeof(*vcEntry));
			vcEntry->diskObj             = diskObj;
			vcEntry->blockDev            = vcCmd->blockDev;
			vcEntry->rawDev              = vcCmd->rawDev;
			vcEntry->ejectCounter        = 0;
			vcEntry->ejectRequestPending = NO;
			vcEntry->diskRequestPending  = NO;
			vcEntry->diskType	     = vcCmd->diskType;
			queue_enter(&volCheckEntryQ, 
				vcEntry, 
				volCheckEntry_t *, 
				link);
			break;

		    case VC_UNREGISTER:
			xpr_vc("volCheck: unregistering disk %s\n", 
				[vcCmd->diskObj name], 2,3,4,5);
			queue_remove(&volCheckEntryQ, 
				vcEntry, 
				volCheckEntry_t *, 
				link);
			IOFree(vcEntry, sizeof(*vcEntry));
		        break;
			
		    case VC_REQUEST:
		    	/*
			 * Panel request.
			 */
		    {
			kern_return_t krtn;
			int unit = [diskObj unit];
			
			xpr_vc("volCheck VC_REQUEST: disk %s\n", 
				[diskObj name], 2,3,4,5);
			
			/*
			 * If there is already a panel up for this drive, 
			 * this is a nop.
			 */
			if(vcEntry->diskRequestPending) {
				xpr_vc("volCheckRequest: Panel already "
				    "pending\n", 1,2,3,4,5);
				break;
			}
			if([diskObj lastReadyState] == IO_Ready) {
				/*
				 * This can be the result of a call to
				 * volCheckRequest() follwed by immediate
				 * insertion of the desired disk.
				 * No error. No panel.
				 */
				xpr_vc("volCheckRequest: Panel Not "
					"Necessary\n", 1,2,3,4,5);
				break;
			}
			krtn = vol_panel_disk_num(volCheckResponse,
				0,			// volume number
				vcCmd->diskType,
				unit,			// drive number
				diskObj,		// param for callback
				NO,			// wrong disk
				&vcEntry->tag);
			/* 
			 * Record this request in *vcEntry.
			 */
			vcEntry->diskRequestPending = YES;
			break;
		    }		    
		    
		    case VC_EJECTING:
			xpr_vc("volCheck VC_EJECTING: disk %s\n", 
				[diskObj name], 2,3,4,5);
			[diskObj setLastReadyState:IO_Ejecting];
#if i386
			/*
			 * FIXME - this should only be done for 
			 * i386 *floppy* drives!
			 */
			vcEntry->ejectCounter        = 1;
#else
			vcEntry->ejectCounter        = VC_EJECT_DELAY;
#endif
			vcEntry->ejectRequestPending = NO;
			vcEntry->diskType = vcCmd->diskType;
			break;
					    
		    case VC_NOTREADY:
			xpr_vc("volCheck VC_NOTREADY: disk %s\n", 
				[diskObj name], 2,3,4,5);
			[diskObj setLastReadyState:IO_NotReady];
			break;
			
		    case VC_RESPONSE:
		    	/* 
			 * This comes from the vol driver - it's a panel 
			 * response, which can only mean "disk not available".
			 */
			xpr_vc("volCheck VC_RESPONSE: disk %s\n", 
				[diskObj name], 2,3,4,5);
			if(!vcEntry->diskRequestPending) {
				/*
				 * This can happen if the disk is inserted 
				 * at around the same time as the user hit 
				 * "cancel".
				 */
				break;
			}
			vcEntry->diskRequestPending = NO;
			[diskObj abortRequest];
			break;

		} /* switch op */
		
freeCmd:
		IOFree(vcCmd, sizeof(*vcCmd));
		
		/*
		 * We need to hold this lock at the top of the loop, and
		 * we'll hold it when we drop out.
		 */
		[volCheckCmdLock lock];
	}	  /* for each cmd */
	
	[volCheckCmdLock unlock];
	xpr_vc("volCheckCmdHandler: done\n", 1,2,3,4,5);
}

/*
 * Common means by which vol driver is notified of presence of disk, either
 * at disk insertion time or at volCheckRegister() time.
 */
static void volDriverNotify(id diskObj,
	dev_t blockDev,
	dev_t rawDev)
{
	struct disk_label *label = IOMalloc(sizeof(struct disk_label));
	int vol_state;
	int flags;
	id logicalDisk;
	IOReturn rtn;
		
	xpr_vc("volDriverNotify: dev=%s\n", [diskObj name], 2,3,4,5);
	if([diskObj isRemovable]) {
		flags = IND_FLAGS_REMOVABLE;
	}
	else {
		flags = IND_FLAGS_FIXED;
	}
	logicalDisk = [diskObj nextLogicalDisk];
	if(logicalDisk != nil) {
	    	rtn = [logicalDisk readLabel:label];
	}
	else {
	   	/*
		 * This is pretty weird. IODiskPartitionProbe:
		 * should have created an IODiskPartition object as partition
		 * 0's raw device...
		 */
	    	IOLog("volCheck: physDev with no logicalDisk!!\n");
	    	rtn = IO_R_NO_LABEL;
	}
	if(rtn == IO_R_SUCCESS) {
	    	vol_state = IND_VS_LABEL;
	}
	else {
	    	if([diskObj isFormatted]) {
			vol_state = IND_VS_FORMATTED;
		}
		else {
		 	vol_state = IND_VS_UNFORMATTED;
		}
	}
	if([diskObj isWriteProtected]) {
		flags |= IND_FLAGS_WP;
	}
	vol_notify_dev(blockDev,
		rawDev,
		"",		 	// form_type - what was this for
					//    again???
		vol_state,
		[diskObj name],		// "sd0", etc.
		flags);		 	// WP, removable
	
	IOFree(label, sizeof(struct disk_label));
}
