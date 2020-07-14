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
 * audioTypes.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *	Definitions for audio driver kernel server.
 */

#ifdef	DRIVER_PRIVATE

#ifndef _NXAUDIO_TYPES_
#define _NXAUDIO_TYPES_

#import <mach/mach_types.h>
#import <mach/message.h>
#import <bsd/sys/time.h>
#import <bsd/sys/types.h>

/*
 * Our names for bootstrap and netname lookup.
 */
#define _NXAUDIO_SOUNDIN_SERVER_NAME "_NXAudioIn"
#define _NXAUDIO_SOUNDOUT_SERVER_NAME "_NXAudioOut"

/*
 * Needed when compiling MIG-generated code.
 */
typedef port_t audio_device_t;
typedef port_t audio_stream_t;
extern audio_device_t audio_port_to_device(port_t port);
extern audio_stream_t audio_port_to_stream(port_t port);
typedef struct timeval audio_time_t;
typedef pointer_t dealloc_ptr;

#define _NXAUDIO_PARAM_MAX 256	/* must match audio.defs */
typedef int audio_array_t[_NXAUDIO_PARAM_MAX];
typedef int audio_var_array_t[_NXAUDIO_PARAM_MAX];
typedef char audio_name_t[_NXAUDIO_PARAM_MAX];

#define _NXAUDIO_TAG_NONE	(-1)

/*
 * Sample clock rates.
 */
#define _NXAUDIO_RATE_UNSET	(-1)
#define _NXAUDIO_RATE_CODEC	0
#define _NXAUDIO_RATE_22050	1
#define _NXAUDIO_RATE_44100	2
#define _NXAUDIO_RATE_48000	3

/*
 * Data formats.
 */
#define _NXAUDIO_FORMAT_UNSET		(-1)
#define _NXAUDIO_FORMAT_LINEAR_16	0
#define _NXAUDIO_FORMAT_MULAW_8		1
#define _NXAUDIO_FORMAT_ALAW_8		2

/*
 * Soundout option bits.
 */
#define _NXAUDIO_SNDOUT_ZEROFILL	(1<<0)
#define _NXAUDIO_SNDOUT_RAMPUP		(1<<1)
#define _NXAUDIO_SNDOUT_RAMPDOWN	(1<<2)
#define _NXAUDIO_SNDOUT_DEEMPHASIS	(1<<3)
#define _NXAUDIO_SNDOUT_SPEAKER_ON	(1<<4)

/*
 * Stream control actions.
 */
#define	_NXAUDIO_STREAM_PAUSE		0
#define	_NXAUDIO_STREAM_RESUME		1
#define	_NXAUDIO_STREAM_ABORT		2
#define	_NXAUDIO_STREAM_RETURN_DATA	3

/*
 * Stream messages.
 */
#define _NXAUDIO_MSG_STARTED		(1<<0)
#define _NXAUDIO_MSG_COMPLETED		(1<<1)
#define _NXAUDIO_MSG_PAUSED		(1<<2)
#define _NXAUDIO_MSG_RESUMED		(1<<3)
#define _NXAUDIO_MSG_ABORTED    	(1<<4)
#define _NXAUDIO_MSG_UNDERRUN		(1<<5)

/*
 * Stream status replies.
 */
#define _NXAUDIO_STATUS_STARTED		0
#define _NXAUDIO_STATUS_COMPLETED	1
#define _NXAUDIO_STATUS_PAUSED		2
#define _NXAUDIO_STATUS_RESUMED		3
#define _NXAUDIO_STATUS_ABORTED    	4
#define _NXAUDIO_STATUS_UNDERRUN	5
#define _NXAUDIO_STATUS_EXCLUDED	6

/*
 * Stream types.
 */
#define _NXAUDIO_STREAM_TYPE_USER		0
#define _NXAUDIO_STREAM_TYPE_SND		1
#define _NXAUDIO_STREAM_TYPE_SND_LINK		2
#define _NXAUDIO_STREAM_TYPE_SND_22		3
#define _NXAUDIO_STREAM_TYPE_SND_44		4
#define _NXAUDIO_STREAM_TYPE_SND_CODEC		5
#define _NXAUDIO_STREAM_TYPE_SND_MONO_22	6
#define _NXAUDIO_STREAM_TYPE_SND_MONO_44	7

/*
 * Error codes.
 * Kernel errors are less than 100,
 * old SoundDSP driver errors are less than 200,
 * so we start there.
 */
#define _NXAUDIO_SUCCESS		0
#define _NXAUDIO_ERR_BASE		200
#define _NXAUDIO_ERR_NOT_OWNER		(_NXAUDIO_ERR_BASE+0)
#define _NXAUDIO_ERR_ACTIVE_STREAMS	(_NXAUDIO_ERR_BASE+1)
#define _NXAUDIO_ERR_PORT		(_NXAUDIO_ERR_BASE+2)
#define _NXAUDIO_ERR_THREAD		(_NXAUDIO_ERR_BASE+3)
#define _NXAUDIO_ERR_SIZE		(_NXAUDIO_ERR_BASE+4)
#define _NXAUDIO_ERR_CHAN_COUNT		(_NXAUDIO_ERR_BASE+5)
#define _NXAUDIO_ERR_CONTROL		(_NXAUDIO_ERR_BASE+6)
#define _NXAUDIO_ERR_RATE		(_NXAUDIO_ERR_BASE+7)
#define _NXAUDIO_ERR_PEAK		(_NXAUDIO_ERR_BASE+8)
#define _NXAUDIO_ERR_FORMAT		(_NXAUDIO_ERR_BASE+9)
#define _NXAUDIO_ERR_PARAMETER		(_NXAUDIO_ERR_BASE+10)

/*
 * Prototypes for device port lookup functions.
 */
#ifndef KERNEL
extern kern_return_t _NXAudioSoundinLookup(const char *host,
					   port_t *serverPort);
extern kern_return_t _NXAudioSoundoutLookup(const char *host,
					    port_t *serverPort);
#endif /* KERNEL */

#endif	/* _NXAUDIO_TYPES_ */

#endif	/* DRIVER_PRIVATE */
