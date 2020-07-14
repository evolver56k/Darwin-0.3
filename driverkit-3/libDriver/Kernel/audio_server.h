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
 * audio_server.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Server routine prototypes.
 *
 * HISTORY
 *      06/26/92/mtm    Original coding.
 */

#import "AudioChannel.h"
#import "InputStream.h"
#import "OutputStream.h"
#import <bsd/dev/audioTypes.h>
#import <mach/mach_types.h>

extern boolean_t audio_server(msg_header_t *InHeadP, msg_header_t *OutHeadP);
extern void audio_port_gone(port_t port);

#define AUDIO_USER_MSG_BASE 700		/* MUST MATCH audio.defs! */

/*
 * snd_server interface needs these prototypes.
 */
extern kern_return_t _NXAudioGetExclusiveUser(AudioChannel *chan,
					      port_t *stream_owner);
extern kern_return_t _NXAudioSetExclusiveUser(AudioChannel *chan,
					      port_t stream_owner);
extern kern_return_t _NXAudioGetBufferOptions(AudioChannel *chan, u_int *size,
					      u_int *count);
extern kern_return_t _NXAudioSetBufferOptions(AudioChannel *chan,
					      port_t stream_owner,
					      u_int size, u_int count);
extern kern_return_t _NXAudioControlStreams(AudioChannel *chan,
					    port_t stream_owner,
					    int action);
extern kern_return_t _NXAudioAddStream(AudioChannel *chan, port_t *stream_port,
				       port_t stream_owner, int stream_id,
				       u_int stream_type);
extern kern_return_t _NXAudioGetDevicePeakOptions(AudioChannel *chan,
						  u_int *enabled,
						  u_int *history);
extern kern_return_t _NXAudioSetDevicePeakOptions(AudioChannel *chan,
						  port_t stream_owner,
						  u_int enabled,
						  u_int history);
extern kern_return_t _NXAudioGetDevicePeak(AudioChannel *chan,
					   u_int *peak_left,
					   u_int *peak_right);
extern kern_return_t _NXAudioGetClipCount(AudioChannel *chan, u_int *count);
extern kern_return_t _NXAudioGetSndoutOptions(AudioChannel *chan,
					      u_int *bits);
extern kern_return_t _NXAudioSetSndoutOptions(AudioChannel *chan,
					      port_t stream_owner,
					      u_int bits);
extern kern_return_t _NXAudioGetSpeaker(AudioChannel *chan, int *left,
					int *right);
extern kern_return_t _NXAudioSetSpeaker(AudioChannel *chan,
					port_t stream_owner,
					int left, int right);
extern kern_return_t _NXAudioSetStreamGain(OutputStream *stream, int left,
					   int right);
extern kern_return_t _NXAudioChangeStreamOwner(AudioStream *stream,
					       port_t stream_owner);
extern kern_return_t _NXAudioStreamControl(AudioStream *stream, int action,
					   audio_time_t time);
extern kern_return_t _NXAudioStreamInfo(AudioStream *stream,
					u_int *bytes_processed,
					u_int *time_stamp);
extern kern_return_t _NXAudioRemoveStream(AudioStream *stream);
extern kern_return_t _NXAudioPlayStream(OutputStream *stream, pointer_t data,
					u_int count, int tag,
					int channel_count,
					int sample_rate,
					u_int left_gain, u_int right_gain,
					u_int low_water, u_int high_water,
					port_t reply_port, u_int messages);
extern kern_return_t _NXAudioPlayStreamData(OutputStream *stream,
					    pointer_t data,
					    u_int count, int tag,
					    port_t reply_port, u_int messages);
extern kern_return_t _NXAudioSetStreamPeakOptions(OutputStream *stream,
						  u_int enabled,
						  u_int history);
extern kern_return_t _NXAudioGetStreamPeak(OutputStream *stream,
					   u_int *peak_left,
					   u_int *peak_right);
extern kern_return_t _NXAudioRecordStream(InputStream *stream, u_int count,
					  int tag,
					  u_int low_water, u_int high_water,
					  port_t reply_port, u_int messages);
extern kern_return_t _NXAudioRecordStreamData(InputStream *stream, u_int count,
					      int tag,
					      port_t reply_port,
					      u_int messages);
extern kern_return_t _NXAudioSetStreamParameters(AudioStream *stream,
						 audio_array_t params,
						 u_int count,
						 audio_array_t values);
extern kern_return_t _NXAudioGetSamplingRates(AudioChannel *chan,
					      int *continuous,
					      int *low, int *high,
					      audio_array_t rates,
					      u_int *count);
extern kern_return_t _NXAudioGetDataEncodings(AudioChannel *chan,
					      audio_array_t encodings,
					      u_int *count);
extern kern_return_t _NXAudioGetChannelCountLimit(AudioChannel *chan,
						  u_int *count);
