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
/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * devswAndVfssw.m - Kernel-level functions to allow adding entries
 *		     to cdevsw, bdevsw, and vfssw.
 *
 * HISTORY
 * 20-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/devsw.h>
#import <bsd/sys/conf.h>
#import <bsd/sys/systm.h>

#ifndef	NULL
#define NULL	((void *)0)
#endif	NULL

/*
 * Empty devsw slots are explicitly set to actual stub functions in the 
 * kernel.
 */

static struct cdevsw no_cdev = NO_CDEVICE;
static struct bdevsw no_bdev = NO_BDEVICE;

/*
 * Add an entry to bdevsw. Returns the index into bdevsw at which the entry
 * was created, or -1 of no space can be found.
 */
 
int IOAddToBdevswAt(
	int index,
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc strategyFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc dumpFunc,
	IOSwitchFunc psizeFunc,
	BOOL isTape		// TRUE if device is a tape device
)
{
	struct bdevsw *devsw;

	if (index == -1) {
	    devsw = bdevsw;
	    for(index=0; index<nblkdev; index++, devsw++) {
		if(memcmp((char *)devsw, 
			    (char *)&no_bdev, 
			    sizeof(struct bdevsw)) == 0)
		    break;
	    }
	}
	devsw = &bdevsw[index];
	if ((index < 0) || (index >= nblkdev) ||
	    (memcmp((char *)devsw, 
		          (char *)&no_bdev, 
			  sizeof(struct bdevsw)) != 0)) {
		return -1;
	}
	devsw->d_open     = openFunc;
	devsw->d_close    = closeFunc;
	devsw->d_strategy = (strategy_fcn_t *)strategyFunc;
	devsw->d_ioctl 	  = ioctlFunc;
	devsw->d_dump     = dumpFunc;
	devsw->d_psize    = psizeFunc;
	devsw->d_type    = 0;
	if(isTape) {
		/*
		    * This is pure bullshit. This flag is 
		    * defined in sys/buf.h, and is only used 
		    * in ufs_vfsops.c, in the bdevsw flags!
		    */
		devsw->d_type |= D_TAPE;
	}
	return index;
}

int IOAddToBdevsw(
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc strategyFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc dumpFunc,
	IOSwitchFunc psizeFunc,
	BOOL isTape)		// TRUE if device is a tape device
{
	return IOAddToBdevswAt( -1,
	    openFunc,
	    closeFunc,
	    strategyFunc,
	    ioctlFunc,
	    dumpFunc,
	    psizeFunc,
	    isTape);
}
	
/*
 * Remove an entry from bdevsw, replace it with a null entry.
 */
void IORemoveFromBdevsw(int bdevswNumber)
{
	bdevsw[bdevswNumber] = no_bdev;
}

/*
 * Add an entry to cdevsw. Returns the index into cdevsw at which the entry
 * was created, or -1 of no space can be found.
 */
int IOAddToCdevswAt(
	int index,
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc readFunc,
	IOSwitchFunc writeFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc stopFunc,
	IOSwitchFunc resetFunc,
	IOSwitchFunc selectFunc,
	IOSwitchFunc mmapFunc,
	IOSwitchFunc getcFunc,
	IOSwitchFunc putcFunc
)
{
	struct cdevsw *devsw;
	
	if (index == -1) {
	    devsw = cdevsw;
	    for(index=0; index<nchrdev; index++, devsw++) {
		if (memcmp((char *)devsw, 
		          (char *)&no_cdev, 
			  sizeof(struct cdevsw)) == 0)
		    break;
	    }
	}
	devsw = &cdevsw[index];
	if ((index < 0) || (index >= nchrdev) ||
	    (memcmp((char *)devsw, 
		    (char *)&no_cdev, 
		    sizeof(struct cdevsw)) != 0)) {
		return -1;
	}
	devsw->d_open   = openFunc;
	devsw->d_close  = closeFunc;
	devsw->d_read   = readFunc;
	devsw->d_write  = writeFunc;
	devsw->d_ioctl  = ioctlFunc;
	devsw->d_stop   = stopFunc;
	devsw->d_reset  = resetFunc;
	devsw->d_select = selectFunc;
	devsw->d_mmap   = mmapFunc;
	devsw->d_getc   = getcFunc;
	devsw->d_putc   = (putc_fcn_t *)putcFunc;
	return index;
}

int IOAddToCdevsw(
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc readFunc,
	IOSwitchFunc writeFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc stopFunc,
	IOSwitchFunc resetFunc,
	IOSwitchFunc selectFunc,
	IOSwitchFunc mmapFunc,
	IOSwitchFunc getcFunc,
	IOSwitchFunc putcFunc)
{
	return IOAddToCdevswAt( -1,
	    openFunc,
	    closeFunc,
	    readFunc,
	    writeFunc,
	    ioctlFunc,
	    stopFunc,
	    resetFunc,
	    selectFunc,
	    mmapFunc,
	    getcFunc,
	    putcFunc);
}
	
/*
 * Remove an entry from cdevsw, replace it with a null entry.
 */
void IORemoveFromCdevsw(int cdevswNumber)
{
	cdevsw[cdevswNumber] = no_cdev;
}

#warning removed vfssw support
#if 0

/*
 * Add an entry to vfssw. Returns the index into vfssw at which the entry
 * was created, or -1 of no space can be found.
 */
int IOAddToVfsswAt(
    int index,
    const char *vfsswName,
    const struct vfsops *vfsswOps
)
{
	struct vfssw *sw = &vfssw[index];
	if ((index < 0) || (index >= (vfsNVFS - vfssw))
	     || (sw->vsw_name != NULL) || (sw->vsw_ops != NULL))
	    return -1;
	sw->vsw_name = (char *)vfsswName;
	sw->vsw_ops = (struct vfsops *)vfsswOps;
	return index;
}
int IOAddToVfssw(
	const char *vfsswName,
	const struct vfsops *vfsswOps)
{
	int index;
	struct vfssw *sw = vfssw;
	
	for(index=0; sw<vfsNVFS; index++, sw++) {
		if((sw->vsw_name == NULL) && (sw->vsw_ops == NULL)) {
			return IOAddToVfsswAt(index, vfsswName, vfsswOps);
		}
	}
	return -1;
}
	
/*
 * Remove an entry from vfssw, replace it with a null entry.
 */
void IORemoveFromVfssw(int vfsswNumber)
{
	vfssw[vfsswNumber].vsw_name = NULL;
	vfssw[vfsswNumber].vsw_ops  = NULL;
}

#endif
	
