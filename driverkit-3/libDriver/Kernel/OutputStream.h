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
 * OutputStream.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver output stream object.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import "AudioStream.h"
#import "audio_types.h"

typedef struct xfer_record {
    dma_desc_t		*ddp;
    u_int		size;
    u_int		peak_left;
    u_int		peak_right;
    u_int		clips;
    queue_chain_t	link;
} xfer_record_t;

@interface OutputStream: AudioStream
{
    u_int		leftGain;
    u_int		rightGain;
    int			resamplePhase;
    int			channelPhase;
    int			formatPhase;
    queue_head_t	xferQueue;
    BOOL		peakEnabled;
    u_int		peakHistory;
    u_int		*peaksLeft;
    u_int		*peaksRight;
    u_int		currentPeak;
}

- (BOOL)playBuffer:(void *)data size:(u_int)byteCount
                           tag:(int)tag
                       replyTo:(port_t)replyPort
                     replyMsgs:(ASMsgRequest)messages;
- (BOOL)canConvertRegion:(region_t *)region rate:(u_int)srate
    format:(IOAudioDataFormat)format channelCount:(u_int)chans;
- clearForMix:(char *)buf size:(u_int)count format:(IOAudioDataFormat)format;
- (u_int)mixRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            buffer:(vm_address_t)data maxCount:(u_int)max
            virgin:(BOOL)isVirgin rate:(u_int)srate format:(IOAudioDataFormat)format
	    channelCount:(u_int)chans;
- completeRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            size:(u_int)xfer used:(u_int *)used;

- (BOOL)isDetectingPeaks;
- (void)setDetectPeaks:(BOOL)flag;
- getPeakLeft:(u_int *)leftPeak right:(u_int *)rightPeak;
- (unsigned int)gainLeft;
- (unsigned int)gainRight;
- (void)setGainLeft:(unsigned int)gain;
- (void)setGainRight:(unsigned int)gain;
- freeRegion:(region_t *)region;
- free;

@end
