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
 * Device_ddm.h - common (private) ddm support for I/O devices.
 *
 * HISTORY
 * 07-Mar-91    Doug Mitchell at NeXT
 *      Created. 
 */

#ifdef	KERNEL
#ifdef	KERNEL_BUILD
#import <xpr_debug.h>
#define DDM_DEBUG XPR_DEBUG
#endif	KERNEL_BUILD
#endif	KERNEL
#import <driverkit/debugging.h>

/*
 * The index into IODDMMasks[] used by IODevice-style drivers.
 */
#define XPR_IODEVICE_INDEX	0

/*
 * bitmasks for Common superclasses.
 */
#define XPR_DEVICE	0x00000001	// IODevice
#define XPR_DISK	0x00000002	// DiskObject, LogicalDisk
#define XPR_NET		0x00000004	// NetDriver
#define XPR_ERR		0x00000008	// errors
#define XPR_VC		0x00000010	// DiskObject's volCheck logic
#define XPR_NDMA	0x00000020	// DMA logic
#define XPR_DEVCONF	0x00000040	// autoconfig
#define XPR_LIBIO	0x00000080	// generalFuncs

#define xpr_dev(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_DEVICE, x, a, b, c, d, e)
	
#define xpr_disk(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_DISK, x, a, b, c, d, e)
	
#define xpr_net(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_NET, x, a, b, c, d, e)
	
#define xpr_err(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_ERR, x, a, b, c, d, e)
	
#define xpr_vc(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_VC, x, a, b, c, d, e)
	
#define xpr_dma(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_NDMA, x, a, b, c, d, e)
	
#define xpr_conf(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_DEVCONF, x, a, b, c, d, e)
	
#define xpr_libio(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_LIBIO, x, a, b, c, d, e)
	
