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

/* 
 * Copyright (c) 1987, 1988 NeXT, Inc.
 *
 * HISTORY
 * 28-Sep-88  Avadis Tevanian, Jr. (avie) at NeXT
 *	Made conditional on eventmeter option.
 *
 * 07-Mar-88  John Seamons (jks) at NeXT
 *	Created.
 */

#ifndef	_I386_EM_
#define	_I386_EM_

#ifdef	KERNEL_BUILD
#import <eventmeter.h>
#else	/* KERNEL_BUILD */
#import <mach/features.h>
#endif	/* KERNEL_BUILD */

#if	EVENTMETER

#import <mach/vm_statistics.h>
#import <bsd/dev/i386/video.h>

/* key codes */
#define	EM_KEY_UP	1
#define	EM_KEY_DOWN	-1

/* events */
#define	EM_INTR		6
#define	EM_DMA		5
#define EM_DISK		4
#define	EM_ENET		3
#define	EM_SYSCALL	2
#define	EM_PAGER	1
#define	EM_PAGE_FAULT	0

/* display geometry */
#define	EM_L		30
#define	EM_R		(EM_L + 1026)
#define	EM_W		1024
#define	EM_Y_OD		(VIDEO_H + 43)
#define	EM_Y_SD		(VIDEO_H + 59)
#define	EM_Y_EVTOP	(VIDEO_H + 25)
#define	EM_Y_EVBOT	(VIDEO_H + 25 + 15)

/* states */
#define	EM_OFF		0
#define	EM_MISC		1
#define	EM_VM		2
#define	EM_NSTATE	2

/* video "start" values */
#define	EM_VID_NORM	0x0
#define	EM_VID_UP	0x1b
#define	EM_VID_DOWN	0xe5

/* log graph */
#define	EM_VMPF		0
#define	EM_VMPA		1
#define	EM_VMPI		2
#define	EM_VMPW		3
#define	EM_FAULT	4
#define	EM_PAGEIN	5
#define	EM_PAGEOUT	6
#define	EM_NEVENT	7

/* disk */
#define	EM_OD		0
#define	EM_SD		1
#define	EM_NDISK	2
#define	EM_READ		0x30000000
#define	EM_WRITE	0x03000000

/* vm */
#define	EM_NVM		3

/* misc */
#define	EM_RATE		1		/* update rate 1 sec */
#define	EM_UPDATE	(hz/EM_RATE)

struct em {
	int	state;
	int	flags;
	int	event_x;
	int	last[EM_NEVENT];
	int	disk_last[EM_NDISK];
	int	disk_rw[EM_NDISK];
	vm_statistics_data_t vm_stat;
	int	pid[EM_NVM];
} em;

/* flags */
#define	EMF_LOCK	0x00000001
#define	EMF_LEDS	0x00000002

#define	event_meter(event) \
	if (em.state == EM_MISC && (em.flags & EMF_LOCK) == 0) { \
		em.flags |= EMF_LOCK; \
		event_vline (em.event_x, EM_Y_EVTOP, EM_Y_EVBOT, \
			0x44444444 | (3 << (((event) << 2) + 4))); \
		event_vline (em.event_x + 1, EM_Y_EVTOP, EM_Y_EVBOT, WHITE); \
		if (++em.event_x > EM_R) \
			em.event_x = EM_L; \
		em.flags &= ~EMF_LOCK; \
	}

#else	/* EVENTMETER */
#define	event_meter(event)
#endif	/* EVENTMETER */
#endif	/* _I386_EM_ */
