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
 * audio_peak.c
 *
 * Copyright (c) 1992, NeXT Computer, Inc.  All rights reserved.
 *
 *	Peak calculation routines.
 *
 * HISTORY
 *	12/08/92/mtm	Original coding from 68k version.
 */

#import "audio_peak.h"
#import "audio_mulaw.h"

/*
 * Calculate peaks in a mulaw 8-bit buffer.
 * Returns peak as 16-bit linear int.
 */
void audio_mulaw8_peak(u_int chans, unsigned char *buf, u_int count,
		 u_int *peak_left, u_int *peak_right)
{
    int i;
    short samp;

    *peak_left = *peak_right = 0;
    if (chans == 1) {
	for (i = 0; i < count/4; i++) {
	    samp = audio_muLaw[*buf++];
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    buf += 3;
	}
	*peak_right = *peak_left;
    } else {
	for (i = 0; i < count/4; i++) {
	    samp = audio_muLaw[*buf++];
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    buf++;
	    samp = audio_muLaw[*buf++];
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_right)
		*peak_right = samp;
	    buf++;
	}
    }
}

/*
 * Calculate peaks in a linear 16-bit buffer.
 */
void audio_linear16_peak(u_int chans, short *buf, u_int count,
			 u_int *peak_left, u_int *peak_right)
{
    int i;
    short samp;

    *peak_left = *peak_right = 0;
    if (chans == 1) {
	for (i = 0; i < count/2; i++) {
	    samp = *buf++;
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    buf++;
	}
	*peak_right = *peak_left;
    } else {
	for (i = 0; i < count/2; i++) {
	    samp = *buf++;
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    samp = *buf++;
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_right)
		*peak_right = samp;
	}
    }
}
/*
 * Calculate peaks in a linear 8-bit buffer.
 * Returns peak as 16-bit linear int.
 */
void audio_linear8_peak(u_int chans, char *buf, u_int count, u_int *peak_left,
			u_int *peak_right)
{
    int i;
    char samp;

    *peak_left = *peak_right = 0;
    if (chans == 1) {
	for (i = 0; i < count/2; i++) {
	    samp = *buf++;
	    /* FIXME: iff devIsUnary */
	    /* convert to 2's comp. by xor'ing the high bit */
	    samp = (samp ^ 0x80) | (samp & 0x7f);
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    buf++;
	}
	*peak_right = *peak_left;
    } else {
	for (i = 0; i < count/2; i++) {
	    samp = *buf++;
	    /* FIXME: iff devIsUnary */
	    /* convert to 2's comp. by xor'ing the high bit */
	    samp = (samp ^ 0x80) | (samp & 0x7f);
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_left)
		*peak_left = samp;
	    samp = *buf++;
	    /* FIXME: iff devIsUnary */
	    /* convert to 2's comp. by xor'ing the high bit */
	    samp = (samp ^ 0x80) | (samp & 0x7f);
	    if (samp < 0)
		samp = -samp;
	    if (samp > *peak_right)
		*peak_right = samp;
	}
    }
    *peak_right <<= 8;
    *peak_left <<= 8;
}

/*
 * Clear peaks in peak buffer.
 */
void audio_clear_peaks(u_int *peak_buf, u_int count)
{
    while (count--)
	*peak_buf++ = 0;
}

/*
 * Return max peak in peak buffer.
 */
u_int audio_max_peak(u_int *peak_buf, u_int count)
{
    u_int peak, max = 0;

    while (count--) {
	peak = *peak_buf++;
	if (peak > max)
	    max = peak;
    }
    return max;
}

/*
 * Add a peak to circular peak buffer and bump cur index.
 */
void audio_add_peak(u_int *peak_buf, u_int peak, u_int *cur, u_int count)
{
    if (count == 0)
	return;
    peak_buf[*cur] = peak;
    *cur = *cur + 1;
    if (*cur == count)
	*cur = 0;
}
