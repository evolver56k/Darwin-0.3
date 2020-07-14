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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * evsio.h - Get/Set parameter calls for Event Status Driver.
 *
 *	CAUTION: Developers should stick to the API exported in
 *		<drivers/event_status_driver.h> to guarantee
 *		binary compatability of their applications in future
 *		releases.
 *
 * HISTORY
 * 22 May 1992    Mike Paquette at NeXT
 *      Created. 
 */
#ifndef _DEV_EVSIO_
#define _DEV_EVSIO_

#import <bsd/dev/ev_types.h>	  /* Public type definitions. */
#import <bsd/dev/event.h>	  /* Public type definitions. */
#import <kernserv/clock_timer.h>	/* For ns_time_t */

/*
 * Identify this driver as one that uses the new driverkit and messaging API
 */
#ifndef _NeXT_MACH_EVENT_DRIVER_
#define _NeXT_MACH_EVENT_DRIVER_	(1)
#endif /* _NeXT_MACH_EVENT_DRIVER_ */

/*
 * Time values, as ns_time_t quantities, are passed in packed as
 * integer arrays.  The following union is provided
 * to assist with packing and unpacking.
 */
typedef union
{
    ns_time_t	tval;
    unsigned	itval[(sizeof(ns_time_t)+sizeof(unsigned)-1)/sizeof(unsigned)];
} _NX_packed_time_t;
#define EVS_PACKED_TIME_SIZE	(sizeof (_NX_packed_time_t) / sizeof (int))

#define EVS_PREFIX	"Evs_"	/* All EVS calls start with this string */
/* WaitCursor-related ioctls */

#define EVSIOSWT "Evs_SetWaitThreshold"
#define EVSIOSWT_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOSWS "Evs_SetWaitSustain"
#define EVSIOSWS_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOSWFI "Evs_SetWaitFrameInterval"
#define EVSIOSWFI_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOCWINFO	"Evs_CurrentWaitCursorInfo"
#define EVSIOCWINFO_THRESH	0
#define EVSIOCWINFO_SUSTAIN	(EVSIOCWINFO_THRESH + EVS_PACKED_TIME_SIZE)
#define EVSIOCWINFO_FINTERVAL	(EVSIOCWINFO_SUSTAIN + EVS_PACKED_TIME_SIZE)
#define EVSIOCWINFO_SIZE	(EVSIOCWINFO_FINTERVAL + EVS_PACKED_TIME_SIZE)


/* Device control ioctls. Levels specified may be in the range 0 - 64. */

#define EVSIOSB	  "Evs_SetBrightness"
#define EVSIOSB_SIZE	1

#define EVSIOSA	  "Evs_SetAttenuation"
#define EVIOSA_SIZE	1

#define EVSIOSADB "Evs_SetAutoDimBrightness"
#define EVSIOSADB_SIZE	1

#define EVSIO_DCTLINFO	"Evs_DeviceControlInfo"
typedef enum {
	EVSIO_DCTLINFO_BRIGHT,
	EVSIO_DCTLINFO_ATTEN,
	EVSIO_DCTLINFO_AUTODIMBRIGHT
} evsio_DCTLINFOIndices;
#define EVSIO_DCTLINFO_SIZE	(EVSIO_DCTLINFO_AUTODIMBRIGHT + 1)

/*
 * Device status request
 */
#define	EVSIOINFO  NX_EVS_DEVICE_INFO


/* Keyboard-related ioctls - implemented within Event Sources */

#define EVSIOSKR  "Evs_SetKeyRepeat"
#define EVSIOSKR_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOSIKR "Evs_SetInitialKeyRepeat"
#define EVSIOSIKR_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIORKBD "Evs_ResetKeyboard"
#define EVSIORKBD_SIZE	1

#define EVSIOCKR  "Evs_CurrentKeyRepeat"
#define	EVSIOCKR_INITIAL	0
#define	EVSIOCKR_BETWEEN	(EVSIOCKR_INITIAL + EVS_PACKED_TIME_SIZE)
#define EVSIOCKR_SIZE		(EVSIOCKR_BETWEEN + EVS_PACKED_TIME_SIZE)

#define	EVSIOCKML "Evs_CurrentKeyMappingLength"
#define EVSIOCKML_SIZE	1

/* For GetIntValues. Length is EVK_NUNITS from KeyMap.h */
#define	EVSIOGKEYS "Evs_CurrentKeyState"

/* The following two tokens are for use with the get/set character routines. */
#define EVSIOSKM  "Evs_SetKeyMapping"
#define EVSIOSKM_SIZE	4096

#define	EVSIOCKM  "Evs_CurrentKeyMapping"
#define EVSIOCKM_SIZE	4096

/* Mouse-related ioctls - implemented within Event Sources */

#define	EVSIOSMS  "Evs_SetMouseScaling"
#define	EVSIOSMS_NSCALINGS	0
	/* The data consists of NX_MAXMOUSESCALINGS threshold/factor pairs. */
#define	EVSIOSMS_DATA		(EVSIOSMS_NSCALINGS + 1)
#define	EVSIOSMS_SIZE		(EVSIOSMS_DATA + (2 * NX_MAXMOUSESCALINGS))

#define	EVSIOCMS  "Evs_CurrentMouseScaling"
#define	EVSIOCMS_NSCALINGS	0
	/* The data consists of NX_MAXMOUSESCALINGS threshold/factor pairs. */
#define	EVSIOCMS_DATA		(EVSIOCMS_NSCALINGS + 1)
#define	EVSIOCMS_SIZE		(EVSIOCMS_DATA + (2 * NX_MAXMOUSESCALINGS))

#define EVSIOSMH  "Evs_SetMouseHandedness"
#define EVSIOSMH_SIZE	1		// value from NXMouseButton enum

#define EVSIOCMH  "Evs_CurrentMouseHandedness"
#define EVSIOCMH_SIZE	1


/* Generic pointer device controls, implemented by the Event Driver. */
#define	EVSIOSCT  "Evs_SetClickTime"
#define EVSIOSCT_SIZE	EVS_PACKED_TIME_SIZE

#define	EVSIOSCS  "Evs_SetClickSpace"
typedef enum {
	EVSIOSCS_X,
	EVSIOSCS_Y
} evsioEVSIOSCSIndices;
#define EVSIOSCS_SIZE	(EVSIOSCS_Y + 1)

#define EVSIOSADT "Evs_SetAutoDimTime"
#define EVSIOSADT_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOSADS "Evs_SetAutoDimState"
#define EVSIOSADS_SIZE	1

#define EVSIORMS  "Evs_ResetMouse"
#define EVSIORMS_SIZE	1

#define	EVSIOCCT  "Evs_CurrentClickTime"
#define EVSIOCCT_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOCADT "Evs_CurrentAutoDimTime"
#define EVSIOCADT_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOGDADT "Evs_GetDeltaAutoDimTime"
#define EVSIOGDADT_SIZE	EVS_PACKED_TIME_SIZE

#define EVSIOIDLE "Evs_GetIdleTime"
#define EVSIOIDLE_SIZE	EVS_PACKED_TIME_SIZE

#define	EVSIOCCS  "Evs_CurrentClickSpace"
typedef enum {
	EVSIOCCS_X,
	EVSIOCCS_Y
} evsioEVSIOCCSIndices;
#define EVSIOCCS_SIZE	(EVSIOCCS_Y + 1)

#define EVSIOCADS "Evs_AutoDimmed"
#define EVSIOCADS_SIZE	1

#endif /* _DEV_EVSIO_ */

