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

/******************************************************************************
	ev_types.h
	Data types for the events status driver.
	This file contains public API.
	mpaque 11Oct91
	
	Copyright 1991 NeXT Computer, Inc.
	
	Modified:
	
******************************************************************************/

#ifndef _DEV_EV_TYPES_
#define _DEV_EV_TYPES_

#import <mach/boolean.h>

/* Maximum length of SetMouseScaling arrays */
#define NX_MAXMOUSESCALINGS 20

typedef struct evsioKeymapping		/* Match old struct names in kernel */
{
    int size;
    char *mapping;
} NXKeyMapping;

typedef struct evsioMouseScaling	/* Match old struct names in kernel */
{
    int numScaleLevels;
    short scaleThresholds[NX_MAXMOUSESCALINGS];
    short scaleFactors[NX_MAXMOUSESCALINGS];
} NXMouseScaling;

typedef enum {
    NX_OneButton,
    NX_LeftButton,
    NX_RightButton
} NXMouseButton;


/*
 * NXEventSystemInfo() information structures.  These are designed to
 * allow for expansion.
 *
 * The current implementation of NXEventSystemInfo() uses an ioctl call.
 * THIS WILL CHANGE.
 */
 
/*
 * Generic query max size and typedefs.
 *
 *	The maximum size is selected to support anticipated future extensions
 *	of request flavors.  Certain flavors planned for future releases may 
 *	require roughtly 800 ints to represent.  We allow a little extra, in
 *	case further growth is needed.
 */
typedef int *NXEventSystemInfoType;
#define NX_EVS_INFO_MAX		(1024)	/* Max array size */
typedef int NXEventSystemInfoData[NX_EVS_INFO_MAX];

/* Event System Devices query */
#define NX_EVS_DEVICE_MAX	16

	/* Interface types */
#define NX_EVS_DEVICE_INTERFACE_OTHER		0
#define NX_EVS_DEVICE_INTERFACE_NeXT		1 // NeXT custom, in older sys
#define NX_EVS_DEVICE_INTERFACE_ADB		2 // NeXT/fruit keybds/mice
#define NX_EVS_DEVICE_INTERFACE_ACE		3 // For x86 PC keyboards
#define NX_EVS_DEVICE_INTERFACE_SERIAL_ACE	4 // For PC serial mice 
#define NX_EVS_DEVICE_INTERFACE_BUS_ACE		5 // For PC bus mice 
#define NX_EVS_DEVICE_INTERFACE_HIL		6 // For HIL hp keyboard 
#define NX_EVS_DEVICE_INTERFACE_TYPE5		7 // For Sun Type5 keyboard
#define NX_EVS_DEVICE_INTERFACE_APPLE_ADB	8 // For Apple ADB keyboards

/*
 * Note! if any new interface types are added above, the following
 * definition of the number of interfaces supported must reflect this.
 * This is used in the libkeymap project (storemap.c module) which needs
 * to be cognizant of the number of new devices coming online
 * via support for heterogeneous architecture platforms.
 * e.g., PCs, HP's HIL, Sun's Type5 keyboard,...
 */
#define NUM_SUPPORTED_INTERFACES	(NX_EVS_DEVICE_INTERFACE_APPLE_ADB + 1)
					// Other, NeXT, ADB, ACE,...

	/* Device types */
#define NX_EVS_DEVICE_TYPE_OTHER	0
#define NX_EVS_DEVICE_TYPE_KEYBOARD	1
#define NX_EVS_DEVICE_TYPE_MOUSE	2	// Relative position devices
#define NX_EVS_DEVICE_TYPE_TABLET	3	// Absolute position devices

typedef struct {
	int	interface;	/* NeXT, ADB, other */
	int	interface_addr;	/* Device address on the interface */
	int	dev_type;	/* Keyboard, mouse, tablet, other */
	int	id;		/* manufacturer's device handler ID */
} NXEventSystemDevice;

typedef struct {
	NXEventSystemDevice	dev[NX_EVS_DEVICE_MAX];
} NXEventSystemDeviceList;

#define __OLD_NX_EVS_DEVICE_INFO		1
#define NX_EVS_DEVICE_INFO			"Evs_EventDeviceInfo"
#define NX_EVS_DEVICE_INFO_COUNT \
	(sizeof (NXEventSystemDeviceList) / sizeof (int))

/*
 * Multiprocessor locks used within the shared memory area between the
 * kernel and event system.  These must work in both user and kernel mode.
 * 
 * These routines are public, for the purpose of writing frame buffer device
 * drivers which handle their own cursors.  Certain architectures define a
 * generic display class which handles cursor drawing and is subclassed by
 * driver writers.  These drivers need not be concerned with the following
 * types and definitions.
 *
 * The ev_lock(), ev_unlock(), and ev_try_lock() functions are available only
 * to drivers built in or dynamically loaded into the kernel, and to DPS
 * drivers built in or dynamically loaded into the Window Server.  They do not
 * exist in any shared library.
 */
#ifdef hppa
/*
** On HPPA, the load&clear command is used for semaphores.  This command
** requires that the address referenced be 16 byte aligned.  Therefore,
** make ev_lock_data_t an array of 4 words so that we are sure to have an
** aligned word to use.
*/
typedef volatile struct {
    int lock[4];  /* 4 words so that we have one aligned to 16 bytes! */
} ev_lock_data_t;
typedef ev_lock_data_t	*ev_lock_t;

#define ev_init_lock(x)	(*((volatile int *)(((int)(x) + 0x0c) & ~0x0f))  = 1)
#define ev_is_locked(x)	(*((volatile int *)(((int)(x) + 0x0c) & ~0x0f)) == 0)
#else /* hppa */
typedef volatile int	ev_lock_data_t;
typedef ev_lock_data_t	*ev_lock_t;

#define ev_init_lock(l)		(*(l) = (ev_lock_data_t)0)
#define ev_is_locked(l)		(*(l) != (ev_lock_data_t)0)
#endif /* hppa */

extern void ev_lock(ev_lock_t l);		// Spin lock!
extern void ev_unlock(ev_lock_t l);
extern boolean_t ev_try_lock(ev_lock_t l);


/* The following typedefs are defined here for compatibility with PostScript */

#if !defined(__Point__) && !defined(BINTREE_H)
#define __Point__
typedef struct { short x, y; } Point;
#endif

#if !defined(__Bounds__) && !defined(BINTREE_H)
#define __Bounds__
typedef struct { short minx, maxx, miny, maxy; } Bounds;
#endif

/*
 * Types used in evScreen protocol compliant operations.
 */

typedef enum {EVNOP, EVHIDE, EVSHOW, EVMOVE, EVLEVEL} EvCmd; /* Cursor state */

#define EV_SCREEN_MIN_BRIGHTNESS	0
#define EV_SCREEN_MAX_BRIGHTNESS	64
/* Scale should lie between MIN_BRIGHTNESS and MAX_BRIGHTNESS */
#define EV_SCALE_BRIGHTNESS( scale, datum ) \
	((((unsigned long)(datum))*((unsigned long)scale)) >> 6)

/*
 * Definition of a tick, as a time in milliseconds. This controls how
 * often the event system periodic jobs are run.  All actual tick times
 * are derived from the nanosecond timer.  These values are typically used
 * as part of computing mouse velocity for acceleration purposes.
 */
#define EV_TICK_TIME		16			/* 16 milliseconds */
#define EV_TICKS_PER_SEC	(1000/EV_TICK_TIME)	/* ~ 62 Hz */
#define EV_NS_TO_TICK(ns)	((ns)>>24)
#define EV_TICK_TO_NS(tick)	(((ns_time_t)(tick))<<24)

/* Mouse Button bits, as passed from an EventSrc to the Event Driver */
#define EV_RB		(0x01)
#define EV_LB		(0x04)
#define EV_MOUSEBUTTONMASK	(EV_LB | EV_RB)

/* Tablet Pressure Constants, as passed from an EventSrc to the Event Driver */
#define EV_MINPRESSURE 0
#define EV_MAXPRESSURE 255

/* Cursor size in pixels */
#define EV_CURSOR_WIDTH		16
#define EV_CURSOR_HEIGHT	16


#endif /* _DEV_EV_TYPES_ */

