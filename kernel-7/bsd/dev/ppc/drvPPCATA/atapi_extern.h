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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/*
 * Copyright (c) 1994-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * atapi_external.h - External definitions for ATAPI driver. 
 *
 * HISTORY 
 * 1-Sep-1994	 Rakesh Dubey at NeXT
 *	Created.
 */


#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_ATAPI_EXTERN_
#define _BSD_DEV_ATAPI_EXTERN_

#import <sys/types.h>
#import <sys/ioctl.h>
#if (IO_DRIVERKIT_VERSION == 330)
#ifdef	KERNEL
#import <machdep/machine/pmap.h>
#import <vm/vm_map.h>
#endif	KERNEL
#endif
#import <mach/boolean.h>
#import <mach/vm_param.h>

/*
 * How does the device respond to an ATAPI packet command. 
 */
#define ATAPI_CMD_DRQ_INT			0x01
#define ATAPI_CMD_DRQ_SLOW			0x00
#define ATAPI_CMD_DRQ_FAST			0x02

#define ATAPI_DEVICE_DIRECT_ACCESS		0x00
#define ATAPI_DEVICE_CD_ROM			0x05
#define ATAPI_DEVICE_OPTICAL			0x07
#define ATAPI_DEVICE_TAPE			0x01
#define ATAPI_DEVICE_UNKNOWN			0x1f

/*
 * ATAPI commands -- as defined in the SFF-8020, Revison 1.2 
 */

/*
 * Mandatory commands. 
 */
#define 	ATAPI_PACKET			0xa0
#define 	ATAPI_IDENTIFY_DRIVE		0xa1
#define 	ATAPI_SOFT_RESET		0x08


typedef	int atapi_return_t;

/*
 * Defines for ATAPI error register. Upper four bits are sense key.
 */
#define MEDIA_CHANGE_REQUEST			0x08
#define CMD_ABORTED				0x04
#define END_OF_MEDIA				0x02
#define ILLEGAL_LENGTH				0x01

/*
 * Defines for the feature register. 
 */
#define DMA_TRANSFERS				0x01

/*
 * Defines for the interrupt reason register. 
 */
#define IO_DIRECTION				0x02	/* 1 --> to host */
#define CMD_OR_DATA				0x01	/* 0 --> user data */

/*
 * Other defines are same as in the case of ATA standard. TBD: Should we
 * redefine all ATAPI registers to keep things completely separate? 
 */


/*
 * ATAPI General configuration register. 
 */
typedef	struct	_atapiGenConfig {
    unsigned short
	protocolType:2,	       		/* */
	rsvd2:1,   			/* */
	deviceType:5, 	 		/* */
	removable:1,   			/* */
	cmdDrqType:2,   		/* */
	rsvd1:3,   			/* */
	cmdPacketSize:2;   		/* */
} atapiGenConfig_t;

/*
 * ATAPI sense keys, page 157 SFF-8020, Rev 1.2 
 */
#define ATAPI_SENSE_NONE			0x00
#define ATAPI_SENSE_RECOVERED_ERROR		0x01
#define ATAPI_SENSE_NOT_READY			0x02
#define ATAPI_SENSE_MEDIUM_ERROR		0x03
#define ATAPI_SENSE_HARDWARE_ERROR		0x04
#define ATAPI_SENSE_ILLEGAL_REQUEST		0x05
#define ATAPI_SENSE_UNIT_ATTENTION		0x06
#define ATAPI_SENSE_DATA_PROTECT		0x07
#define ATAPI_SENSE_ABORTED_COMMAND		0x0b
#define ATAPI_SENSE_MISCOMPARE			0x0e

#endif	_BSD_DEV_ATAPI_EXTERN_

#endif	DRIVER_PRIVATE

