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
 * audio_peak.h
 *
 * Copyright (c) 1992, NeXT Computer, Inc.  All rights reserved.
 *
 *     	Prototypes for peak routines.
 */

#import <bsd/sys/types.h>

extern void audio_mulaw8_peak(u_int chans, unsigned char *buf, u_int count,
			      u_int *peak_left, u_int *peak_right);
extern void audio_linear8_peak(u_int chans, char *buf, u_int count,
			       u_int *peak_left, u_int *peak_right);
extern void audio_linear16_peak(u_int chans, short *buf, u_int count,
				u_int *peak_left, u_int *peak_right);
extern void audio_clear_peaks(u_int *peak_buf, u_int count);
extern u_int audio_max_peak(u_int *peak_buf, u_int count);
extern void audio_add_peak(u_int *peak_buf, u_int peak, u_int *cur,
			   u_int count);
