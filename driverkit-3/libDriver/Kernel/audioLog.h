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
 * audioLog.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver message logging.
 *
 * HISTORY
 *      11/29/93/rkd    Fixed XPRs.
 *      08/07/92/mtm    Original coding.
 */

//#define DDM_DEBUG 1

#import <driverkit/generalFuncs.h>

/* FIXME: not in the kernel */
#define mach_error_string(n) "MACH ERR"

#define log_msg(f)	IOLog f
#define log_error(f)	IOLog f

#if 0
#define	audio_panic()	{IOLog("Audio Driver spins for panic...\n"); \
			     while(1) IOSleep(1000);}
#else
#define	audio_panic()	IOLog("Audio Driver error")
#endif

#ifdef	DEBUG
#define log_debug(f)	IOLog f
#else	DEBUG
#define log_debug(f)
#endif	DEBUG

#ifdef	DDM_DEBUG
#define XPR_IODEVICE_INDEX 0
#import <driverkit/debugging.h>

#define AUDIO_NUM_XPR_BUFS	1024

/*
 * Should be different than other user drivers at XPR_IODEVICE_INDEX.
 * c.f. printerdriver/printerTypes.h.
 */
#define XPR_AUDIO_USER		0x00001000	/* messages to/from user */
#define XPR_AUDIO_DEVICE	0x00002000	/* audio device */
#define XPR_AUDIO_CHANNEL	0x00004000	/* input and output channels */
#define XPR_AUDIO_STREAM	0x00008000	/* record and play streams */
#define	XPR_AUDIO_ALL		(XPR_AUDIO_USER|XPR_AUDIO_DEVICE|\
				 XPR_AUDIO_CHANNEL|XPR_AUDIO_STREAM)

#define xpr_audio_user(x, a, b, c, d, e) \
    IODEBUG(XPR_IODEVICE_INDEX, XPR_AUDIO_USER, x, a, b, c, d, e)
#define xpr_audio_device(x, a, b, c, d, e) \
    IODEBUG(XPR_IODEVICE_INDEX, XPR_AUDIO_DEVICE, x, a, b, c, d, e)
#define xpr_audio_channel(x, a, b, c, d, e) \
    IODEBUG(XPR_IODEVICE_INDEX, XPR_AUDIO_CHANNEL, x, a, b, c, d, e)
#define xpr_audio_stream(x, a, b, c, d, e) \
    IODEBUG(XPR_IODEVICE_INDEX, XPR_AUDIO_STREAM, x, a, b, c, d, e)

#else	DDM_DEBUG
#define xpr_audio_user(x, a, b, c, d, e)
#define xpr_audio_device(x, a, b, c, d, e)
#define xpr_audio_channel(x, a, b, c, d, e)
#define xpr_audio_stream(x, a, b, c, d, e)
#endif	DDM_DEBUG


