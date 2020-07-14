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

/* 	Copyright (c) 1991,1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * machdep/hppa/xpr.h - hppa specific XPR definitions.
 *
 * HISTORY
 * 14-Jul-93	Mac Gillon at NeXT
 *	Ported to hppa.
 *
 * 21-Jan-92    Doug Mitchell at NeXT
 *      Created. 
 */

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
#import <driverkit/ddmPrivate.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/xpr_mi.h>

/*
 * Compile-time flags:
 *
 * UXPR - this enables driverkit-style xpr functionality. The module which
 *	  implements this, machdep/hppa/ddm.c, is compiled into the kernel 
 *	  when this flag is true. Currently this is true in all nrw builds.
 * DDM_DEBUG - enables actual uxpr() macros. Currrently this is true only
 *	  in hppa DEBUG builds. 
 *
 * RELEASE kernels compile in ddm.c to allow loadable servers to use the 
 *	  uxpr mechanism.
 */
 
#define XPR_TIMESTAMP	event_get()

/*
 * SCSI. See mk/driverkit/xpr_mi.h for machine-independnet SCSI definitions.
 */
#define XPR_SC		0x00000100	// controller h/w independent
#define XPR_SCHW	0x00000200	// controller h/w dependent

#define xpr_sc(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_SC, x, a, b, c, d, e)

#define xpr_hw(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_SCHW, x, a, b, c, d, e)

/*
 * Floppy.
 */
#define XPR_FC		0x00002000	// controller level
#define XPR_FPIO	0x00004000	// PIO bytes
#define XPR_FTIME	0x00010000	// timer

#define xpr_fc(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FC, x, a, b, c, d, e)

#define xpr_pio(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FPIO, x, a, b, c, d, e)

#define xpr_ftime(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FTIME, x, a, b, c, d, e)

/*
 * Ethernet.
 */
#define XPR_ENTX	0x00100000	// transmit
#define XPR_ENRX	0x00200000	// receive
#define XPR_ENCOM	0x00400000	// common 
#define XPR_ENBUF	0x00800000	// buffer allocation

#define xpr_entx(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_ENTX, x, a, b, c, d, e)

#define xpr_enrx(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_ENRX, x, a, b, c, d, e)

#define xpr_encom(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_ENCOM, x, a, b, c, d, e)

#define xpr_enbuf(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_ENBUF, x, a, b, c, d, e)

/*
 * Interrupt logic.
 */
#define XPR_INTR	0x02000000

#define xpr_intr(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_INTR, x, a, b, c, d, e)

/*
 * Callouts.
 */
#define XPR_CALL	0x04000000

#define xpr_call(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_CALL, x, a, b, c, d, e)

/*
 * Frame buffer.
 */
#define XPR_FB		0x08000000

#define xpr_fb(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_FB, x, a, b, c, d, e)

