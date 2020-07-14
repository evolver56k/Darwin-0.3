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
 * volDriver.m - user-level "emulation" of vol driver. 
 *
 * a signal 14 delivered to this executable results in a panel response
 * action, which is interpreted as a "disk not available" by volCheck.
 *
 * a signal 1 delivered causes the same action as the death of the 
 * WorkSpace - all pending panel requests are ack'd.
 *
 * HISTORY
 * 09-May-91    Doug Mitchell at NeXT
 *      Created.
 */

#undef	DRIVER_PRIVATE
#define DRIVER_PRIVATE

#import <objc/objc.h>
#import <bsd/sys/types.h>
#import <mach/cthreads.h>	/* FIXME - for ASSERT for MD includes...*/
#import <kernserv/queue.h>
#import <driverkit/generalFuncs.h>
#import <bsd/sys/signal.h>
#import <driverkit/Device_ddm.h>
#import <machkit/NXLock.h>
#import <driverkit/volCheck.h>
#import "volCheckPrivate.h"
#import "volDriver.h"

/*
 * Struct for maintaining state of panel requests.
 */
typedef struct {
        queue_chain_t           link;
        int                     tag;
        vpt_func                fnc;            /* to be called upon receipt of
                                                 * vol_panel_resp message */    
        void                    *param;
} vol_panel_entry_t;

/*
 * static data.
 */
static int volDriverTag;
static queue_head_t volDriverPanelQ;
static id volDriverLock;		// NXLock - protects above

/*
 * Static function prototypes.
 */
static char *diskTypeToString(int diskType);
static char *volStateToString(int volState);
static vol_panel_entry_t *vol_panel_get_entry(int tag);
static void volDriverSig(int foo);
static void volDriverWSDeath(int foo);

/*
 * One-time only init.
 */
void volDriverInit()
{
	queue_init(&volDriverPanelQ);
	volDriverLock = [NXLock new];
	signal(SIGALRM, volDriverSig);
	signal(SIGHUP, volDriverWSDeath);
}

kern_return_t vol_notify_dev(dev_t block_dev, 
	dev_t raw_dev,
	const char *form_type,
   	int vol_state,				/* IND_VS_LABEL, etc. */
	const char *dev_str,
	int flags)
{
	
	IOLog("Disk Insertion: block_dev 0x%x vol_state %s\n",
		block_dev, volStateToString(vol_state));
	return(KERN_SUCCESS);
}

/*
 * Cancel notification message. Called on 'mount' of device.
 */
void vol_notify_cancel(dev_t device)
{
	IOLog("Cancelling Alert for dev 0x%x\n", device);
}

kern_return_t vol_panel_request(vpt_func fnc,
	int panel_type,				/* PR_PT_DISK_NUM, etc. */
	int response_type,			/* PR_RT_ACK, atc. */
	int p1,
	int p2,
	int p3,
	int p4,
	char *string1,
	char *string2,
	void *param,
	int *tag)				/* RETURNED */
{
	char *disk_string = diskTypeToString(p2);
	vol_panel_entry_t *vpe;
	
	[volDriverLock lock];
	
	/*
	 * Record this panel in case of ack or cancel.
	 */
	vpe = IOMalloc(sizeof(*vpe));
	vpe->tag = volDriverTag;
	vpe->fnc = fnc;
	vpe->param = param;
	queue_enter(&volDriverPanelQ,
		vpe,
		vol_panel_entry_t *,
		link);
		
	switch(panel_type) {
	    case PR_PT_DISK_NUM:
		IOLog("Please Insert %s Disk %d in Drive %d (tag %d)\n", 
			disk_string, p1, p3, volDriverTag);
		break;
	    case PR_PT_DISK_LABEL:
		IOLog("Please Insert %s Disk \'%s\' in Drive %d (tag %d)\n", 
			disk_string, string1, p3);
		break;
	    case PR_PT_DISK_NUM_W:
		IOLog("Wrong Disk: Please Insert %s Disk %d in Drive "
			"%d (tag %d)\n", disk_string, p1, p3);
		break;
	    case PR_PT_DISK_LABEL_W:
		IOLog("Wrong Disk: Please Insert %s Disk \'%s\' in "
			"Drive %d (tag %d)\n", disk_string, string1, p3);
		break;
	    case PR_PT_SWAPDEV_FULL:
		IOLog("***Swap Device Full***\n");
		break;
	    case PR_PT_FILESYS_FULL:
		IOLog("***File System %s Full***\n", string1);
		break;
	    case PR_RT_EJECT_REQ:
		IOLog("Please Eject %s Disk %d (tag %d)\n", disk_string, p3);
		break;
	    default:
		/* FIXME: what do we do here? */
		IOLog("vol_panel_request: bogus panel_type (%d)\n",
			panel_type);
		break;
	}
	*tag = volDriverTag++;
	[volDriverLock unlock];
	
	return(KERN_SUCCESS);
}

kern_return_t vol_panel_disk_num(vpt_func fnc,
	int volume_num,
	int drive_type,				/* PR_DRIVE_FLOPPY, etc. */
	int drive_num,
	void *param,
	boolean_t wrong_disk,
	int *tag)				/* RETURNED */
{
        return(vol_panel_request(fnc,
                wrong_disk ? PR_PT_DISK_NUM_W : PR_PT_DISK_NUM,
                PR_RT_ACK,
                volume_num,
                drive_type,
                drive_num,
                0,
                "",
                "",
                param,
                tag));  

}
kern_return_t vol_panel_disk_label(vpt_func fnc,
	char *label,
	int drive_type,				/* PR_DRIVE_FLOPPY, etc. */
	int drive_num,
	void *param,
	boolean_t wrong_disk,
	int *tag)				/* RETURNED */
{
	return(vol_panel_request(fnc,
                wrong_disk ? PR_PT_DISK_LABEL_W : PR_PT_DISK_LABEL,
                PR_RT_ACK,
                0,
                drive_type,
                drive_num,
                0,
                label,
                "",
                param,
                tag));
}

/*
 * Remove an alert panel. Called upon detection of desired disk after putting
 * up an alert panel.
 */
kern_return_t vol_panel_remove(int tag)
{
	vol_panel_entry_t *vpe;

	[volDriverLock lock];
	vpe = vol_panel_get_entry(tag);
	if(vpe == NULL) {
		IOLog("vol_panel_remove: Nonexistent panel (tag %d)\n",
			tag);
	}
	else {
		IOLog("volDriver: removing panel for tag %d\n", tag);
		IOFree(vpe, sizeof(*vpe));
	}
	[volDriverLock unlock];
	return(KERN_SUCCESS);
}

/*
 * Private functions.
 */
 
static char *diskTypeToString(int diskType)
{
	switch(diskType) {
	    case PR_DRIVE_FLOPPY:
		return("Floppy");
	    case PR_DRIVE_OPTICAL:
		return("Optical");
	    case PR_DRIVE_SCSI:
		return("SCSI");
	    default: 
		return("");
	}
}

static char *volStateToString(int volState)
{
	switch(volState) {
	    case IND_VS_LABEL:
	    	return("IND_VS_LABEL");
	    case IND_VS_FORMATTED:
	    	return("IND_VS_FORMATTED");
	    case IND_VS_UNFORMATTED:
	    	return("IND_VS_UNFORMATTED");
	    default:
	    	return("Unknown volState");
	}
}

static vol_panel_entry_t *vol_panel_get_entry(int tag) {

        /*
         * get and remove vol_panel_entry associated with 'tag' from 
	 * volDriverPanelQ. Returns NULL if entry not found.
	 * volDriverLock should be held on entry.
         */
	 
        vol_panel_entry_t *vpe;

        vpe = (vol_panel_entry_t *)queue_first(&volDriverPanelQ);
        while(!queue_end(&volDriverPanelQ, (queue_entry_t)vpe)) {
                if(vpe->tag == tag) {
                        /*
                         * Found it. 
                         */
                        queue_remove(&volDriverPanelQ,
                                vpe,
                                vol_panel_entry_t *,
                                link);
                        return(vpe);
                }
                vpe = (vol_panel_entry_t *)vpe->link.next;
        }
        /*
         * Not found. 
         */
        return(NULL);
}

/*
 * Signal catcher to emulate WS's panel response message. All we can do
 * is cancel the last request which went out.
 */
static void volDriverSig(int foo)
{
	vol_panel_entry_t *vpe;
	
	[volDriverLock lock];
	vpe = vol_panel_get_entry(volDriverTag - 1);
	if(vpe == NULL) {
		IOLog("volDriverSig: Couldn't find entry for tag %d\n",
			volDriverTag - 1);
		goto done;
	}
	/*
	 * Perform callout if necessary, then dispose of entry.
	 */
	xpr_vc("volDriverSig: doing VOL_PANEL_RESP callout\n", 1,2,3,4,5);
	if(vpe->fnc) {
		(*vpe->fnc)(vpe->param, 
			vpe->tag, 
			0);		// 'value'
	}
	IOFree(vpe, sizeof(*vpe));
done:
	[volDriverLock unlock];
	return;
}

/*
 * Signal catcher to abort all pending panels. Emulates death of workspace.
 */
static void volDriverWSDeath(int foo)
{
	vol_panel_entry_t *vpe;

	/*
	 * Ack and dispose of every vol_panel_entry in volDriverPanelQ.
	 */
	xpr_vc("volDriverWSDeath\n", 1,2,3,4,5);
	while(!queue_empty(&volDriverPanelQ)) {
		vpe = (vol_panel_entry_t *)queue_first(&volDriverPanelQ);
		queue_remove(&volDriverPanelQ,
			vpe,
			vol_panel_entry_t *,
			link);
		xpr_vc("volDriverWSDeath: doing VOL_PANEL_RESP "
			"callout\n", 1,2,3,4,5);
		if(vpe->fnc) {
			(*vpe->fnc)(vpe->param, 
				vpe->tag, 
				0);        /* as in "disk not 
					    * available" */
		}
		IOFree(vpe, sizeof(*vpe));
	}
}

static int manual_poll;

int vol_check_manual_poll()
{
	if(manual_poll) {
		manual_poll = 0;
		return(1);
	}
	else {
		return(0);
	}
}

/*
 * Internal version of ioctl(DKIOCCHECKINSERT).
 */
void vol_check_set_poll()
{
	manual_poll = 1;
}
