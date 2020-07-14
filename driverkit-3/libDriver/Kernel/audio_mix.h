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
 * audio_mix.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver mixing prototypes.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import "audio_types.h"
#import <bsd/sys/types.h>

extern void audio_swapSamples(short *src, short *dest, u_int count);
extern void audio_twosComp8ToUnary(char *src, char *dest, u_int count);
extern u_int audio_scaleSamples(char *src, char *dest, u_int count,
				IOAudioDataFormat format, u_int chanCount,
				int leftGain, int rightGain);
extern void audio_resample22To44(char *src, char *dest, u_int count,
				 IOAudioDataFormat format, u_int channelCount);
extern void audio_resample44To22(char *src, char *dest, u_int count,
				 IOAudioDataFormat format, u_int *phase);
extern void audio_convertMonoToStereo(char *src, char *dest, u_int count,
				      IOAudioDataFormat format);
extern void audio_convertStereoToMono(char *src, char *dest, u_int count,
				      IOAudioDataFormat format, u_int *phase);
extern void audio_convertLinear8ToLinear16(char *src, short *dest,
					   u_int count);
extern void audio_convertLinear8ToMulaw8(char *src, char *dest, u_int count);
extern void audio_convertLinear16ToLinear8(short *src, char *dest, u_int count,
					   u_int *phase);
extern void audio_convertLinear16ToMulaw8(short *src, char *dest, u_int count,
					  u_int *phase);
extern void audio_convertMulaw8ToLinear16(char *src, short *dest, u_int count);
extern void audio_convertMulaw8ToLinear8(char *src, char *dest, u_int count);
extern u_int audio_mix(char *src, char *dest, u_int count,
		       IOAudioDataFormat format, boolean_t virgin);
