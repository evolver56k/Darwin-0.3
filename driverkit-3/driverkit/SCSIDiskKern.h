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
 * SCSIDiskKern.h
 *
 * HISTORY
 * 30-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#ifndef	_BSD_DEV_M88K_SCSIDISKKERN_H_
#define _BSD_DEV_M88K_SCSIDISKKERN_H_

#import <sys/types.h>
#import <sys/buf.h>
#import <objc/objc.h>
#import <sys/uio.h>

#define	SD_UNIT(dev)		(minor(dev) >> 3) 
#define	SD_PART(dev)		(minor(dev) & 0x7)	
#define NUM_SD_DEV		16
#define NUM_SD_PART		8		// probably too big...
#define SD_LIVE_PART		(NPART-1)

typedef struct {
	/*
	 * One per unit. Note that the physDevice (live partition), the raw
	 * device, and the block devices for a given disk all share the 
	 * same physbuf. Block devices don't use physbuf; arbitration for
	 * access to physbuf by the raw and live devices is done by physio().
	 */
	struct buf 	*physbuf;		/* for phys I/O */
	
} SCSIDisk_dev_t;

extern void sd_init_idmap();
extern IODevAndIdInfo *sd_idmap();

#endif	_BSD_DEV_M88K_SCSIDISKKERN_H_
