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
 * volCheckPrivate.h - internal typedefs for volCheck logic.
 *
 * HISTORY
 * 07-May-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <driverkit/IODisk.h>
#import <driverkit/IODiskPartition.h>
#import <driverkit/volCheck.h>
#import <kernserv/insertmsg.h>
#import <kernserv/queue.h>
#ifdef	KERNEL
#import <kernserv/prototypes.h>
#else	KERNEL
#import <bsd/libc.h>
#endif	KERNEL

#undef	DRIVER_PRIVATE
#define DRIVER_PRIVATE
#import <bsd/dev/voldev.h>

/*
 * state per registered removable drive.
 */
typedef struct {
	id		diskObj;
	dev_t 		blockDev;
	dev_t		rawDev;
	int 		ejectCounter;
	BOOL	 	ejectRequestPending;
	BOOL	 	diskRequestPending;
	int		tag;
	int 		diskType;		// PR_DRIVE_FLOPPY, etc.
	queue_chain_t	link;
} volCheckEntry_t;

/*
 * Command to be queued in volCheckCmdQ.
 */
typedef enum {

	VC_REGISTER,
	VC_UNREGISTER,
	VC_REQUEST,
	VC_EJECTING,
	VC_NOTREADY,
	VC_RESPONSE,
	
} volCheckOp_t;

typedef struct {
	volCheckOp_t	op;
	id		diskObj;
	int		diskType;
	dev_t		blockDev;
	dev_t		rawDev;
	queue_chain_t	link;
} volCheckCmd_t;

/*
 * Delay in seconds between executing "eject" and requiring a "not ready"
 * state from the drive.
 */
#define VC_EJECT_DELAY		5

