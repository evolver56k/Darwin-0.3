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
 * snd_reply.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Snd reply message routine prototypes.
 *
 * HISTORY
 *      08/07/92/mtm    Original coding.
 */

#import <mach/mach_types.h>
#import <mach/message.h>
#import <bsd/sys/types.h>

extern void audio_snd_reply_ret_device (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	port_name_t	device_port);	// returned port.
extern void audio_snd_reply_ret_stream (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	port_name_t	stream_port);	// returned port.
extern void audio_snd_reply_illegal_msg (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	local_port,	// returned port of interest
	port_name_t	remote_port,	// who to send it to.
	int		msg_id,		// message id with illegal syntax
	int		error);		// error code	
extern void audio_snd_reply_recorded_data (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to reply to
	int		data_tag,	// tag from region
	pointer_t	data,		// recorded data
	int		nbytes);	// number of bytes of data to send
extern void audio_snd_reply_timed_out (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// tag from region
extern void audio_snd_reply_ret_samples (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		nsamples,	// number of bytes of data to record
	int		timeStamp);
extern void audio_snd_reply_overflow (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_started (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_completed (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_aborted (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_paused (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_resumed (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	int		data_tag);	// from region
extern void audio_snd_reply_ret_parms (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	u_int		parms);
extern void audio_snd_reply_ret_volume (
	msg_header_t    *out_msg,	// allocated message
	port_name_t	remote_port,	// who to send it to.
	u_int		volume);	
extern void audio_snd_reply_ret_formats(msg_header_t *msg,
				  port_name_t remote_port,
				  u_int rates, u_int low, u_int high,
				  u_int encodings, u_int chans);
