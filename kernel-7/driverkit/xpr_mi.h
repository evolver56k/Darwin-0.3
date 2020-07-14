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
 * driverkit/xpr_mi.h - machine independent XPR definitions for 
 *		     driverkit-based drivers.
 *
 * HISTORY
 * 25-Nov-92    Doug Mitchell at NeXT
 *      Cloned from machdep/nrw/xpr.h.
 */

#ifdef	DRIVER_PRIVATE

#ifdef	KERNEL_BUILD
#import "uxpr.h"
#import "xpr_debug.h"
#else	/* KERNEL_BUILD */
#import <mach/features.h>
#endif	/* KERNEL_BUILD */
#ifndef	DDM_DEBUG
#define DDM_DEBUG	XPR_DEBUG
#endif	/* DDM_DEBUG */

#import <driverkit/debugging.h>
#import <driverkit/Device_ddm.h>

/*
 * SCSI.
 */
#define XPR_SD		0x00000400	// SCSI Disk
#define XPR_SDEV	0x00000800	// other SCSI devices

#define xpr_sd(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_SD, x, a, b, c, d, e)

#define xpr_sdev(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_SDEV, x, a, b, c, d, e)

/*
 * Floppy.
 */
#define XPR_FDD		0x00001000	// disk level
#define XPR_FADDRS	0x00008000	// each disk address

#define xpr_fd(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FDD, x, a, b, c, d, e)

#define xpr_addr(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FADDRS, x, a, b, c, d, e)

#endif	/* DRIVER_PRIVATE */
