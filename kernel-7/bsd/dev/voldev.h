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

/*	@(#)voldev.h	2.0	03/19/90	(c) 1990 NeXT	*/

/* 
 * HISTORY
 * 19-Sep-90	Doug Mitchell
 *	Added prototype for vol_notify_cancel.
 * 03-20-90	Doug Mitchell at NeXT
 *	Created.
 */ 

#ifdef	DRIVER_PRIVATE

#ifndef	_VOLDEV_
#define _VOLDEV_

#import <sys/types.h>
#import <mach/kern_return.h>
#import <kernserv/insertmsg.h>
#if	KERNEL_PRIVATE
#import <kern/kern_port.h>
#endif	/* KERNEL_PRIVATE */

typedef	void (*vpt_func)(void *param, int tag, int response_value);

/*
 * Public Functions
 */
kern_return_t vol_notify_dev(dev_t block_dev, 
	dev_t raw_dev,
	const char *form_type,
   	int vol_state,				/* IND_VS_LABEL, etc. */
	const char *dev_str,
	int flags);
void vol_notify_cancel(dev_t device);
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
	int *tag);				/* RETURNED */
kern_return_t vol_panel_disk_num(vpt_func fnc,
	int volume_num,
	int drive_type,				/* PR_DRIVE_FLOPPY, etc. */
	int drive_num,
	void *param,
	boolean_t wrong_disk,
	int *tag);				/* RETURNED */
kern_return_t vol_panel_disk_label(vpt_func fnc,
	char *label,
	int drive_type,				/* PR_DRIVE_FLOPPY, etc. */
	int drive_num,
	void *param,
	boolean_t wrong_disk,
	int *tag);				/* RETURNED */
kern_return_t vol_panel_remove(int tag);

#ifndef	NULL
#define NULL 0
#endif  /* NULL */

#ifdef	KERNEL_PRIVATE
/*
 * Not used by driverkit-style drivers.
 */
extern kern_port_t panel_req_port;		/* PORT_NULL if no port to 
						 * which to send panel requests
						 * has been registered */
#endif	/* KERNEL_PRIVATE */

#if	SUPPORT_PORT_DEVICE

kern_return_t vol_notify_port(kern_port_t dev_port, 
	char *form_type,
   	int vol_state,				/* IND_VS_LABEL, etc. */
	int flags);

#endif	/* SUPPORT_PORT_DEVICE */


#endif	/* _VOLDEV_ */

/*
 * Support for manual "disk present" polling. Normally drives are polled
 * by the volCheck module once per second, but if the driver returns YES
 * for the needsManualPolling method, its drive will only be polled once 
 * for each time the DKIOCCHECKINSERT ioctl is executed on the vol 
 * driver. 
 */

/* 
 * Returns 1 if DKIOCCHECKINSERT has been executed since the last call,
 * else returns 0.
 */
int vol_check_manual_poll();

/*
 * Internal version of ioctl(DKIOCCHECKINSERT).
 */
void vol_check_set_poll();

#endif	/* DRIVER_PRIVATE */
