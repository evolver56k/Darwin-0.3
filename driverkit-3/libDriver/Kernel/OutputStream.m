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
 * OutputStream.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver output stream object.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import	<kern/lock.h>
#import "OutputStream.h"
#import "AudioChannel.h"
#import <driverkit/IOAudioPrivate.h>
#import "audioLog.h"
#import "audio_types.h"
#import "audio_mix.h"
#import "audio_peak.h"
#import <mach/mach_types.h>
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>
#import <mach/vm_param.h>		// PAGE_SIZE
#import <kernserv/kern_server_types.h>
#import <kernserv/prototypes.h>
#import <architecture/byte_order.h>
#import <driverkit/kernelDriver.h>

/*
 * mix buffers contain enough space to support conversions
 * that require up to four times the max dma size.
 */

/*
 * This size depends unpon current descriptor size. We need a method to do
 * this instead of a #define. 
 */

// With 8K size buffers.
#define MIX_BUFFER1_SIZE		(PAGE_SIZE*8)
#define MIX_BUFFER2_SIZE		(PAGE_SIZE*4)

@implementation OutputStream

/*
 * Overridden from superclass.
 */
- initChannel:chan tag:(int)aTag user:(port_t *)user
        owner:(port_t)owner type:(u_int)aType
{
    int i;
    xfer_record_t *xfer_rec;

    if (![super initChannel:chan tag:aTag user:user owner:owner type:aType])
        return nil;
    leftGain = rightGain = UNITY_GAIN;
    peakHistory = 1;
    queue_init(&xferQueue);
    for (i = 0; i < [chan dmaCount]; i++) {
        xfer_rec = (xfer_record_t *)IOMalloc(sizeof(xfer_record_t));
        xfer_rec->ddp = 0;
        xfer_rec->size = 0;
        xfer_rec->peak_left = xfer_rec->peak_right = 0;
        xfer_rec->clips = 0;
        queue_enter(&xferQueue, xfer_rec, xfer_record_t *, link);
    }
    mixBuffer1 = (char *)IOMalloc(MIX_BUFFER1_SIZE);
    mixBuffer2 = (char *)IOMalloc(MIX_BUFFER2_SIZE);
    return self;
}

/*
 * Write data in our task to kernel memory.
 */
static BOOL writeToKernel(vm_address_t data, u_int size, vm_address_t *kmem)
{
    kern_return_t kerr;
    vm_address_t data_page = trunc_page(data);
    u_int offset = data - data_page;
    u_int size_page = round_page(size + offset);
    vm_task_t kernelTask = (vm_task_t)kern_serv_kernel_task_port();

    /*
     * Read-protect the region so vm_write does not do a physical copy.
     */
    kerr = vm_protect(task_self(), data_page, size_page, FALSE, VM_PROT_READ);
    if (kerr != KERN_SUCCESS) {
        IOLog("Audio: vm_protect returned %d\n", kerr);
        //IOPanic("Audio: vm_protect\n");
    }

    /*
     * Allocate space in the kernel map.
     */
    kerr = vm_allocate(kernelTask, kmem, size_page, TRUE);
    if (kerr != KERN_SUCCESS)
	return NO;

    /*
     * Write data to kernel map.
     */
    kerr = vm_write(kernelTask, *kmem, data_page, size_page);
    if (kerr != KERN_SUCCESS) {
	IOLog("Audio: vm_write returned %d\n", kerr);
	//IOPanic("Audio: vm_write\n");
    }

    /*
     * Deallocate user data.
     */
    kerr = vm_deallocate(task_self(), data_page, size_page);
    if (kerr != KERN_SUCCESS) {
        IOLog("Audio: vm_deallocate returned %d\n", kerr);
        //IOPanic("Audio: vm_deallocate\n");
    }
    /*
     * Return offset of the data in kernel memory.
     */
    *kmem += offset;
    return YES;
}

/*
 * Enqueue buffer to region queue.
 */
- (BOOL)playBuffer:(void *)data size:(u_int)byteCount
                           tag:(int)aTag
                       replyTo:(port_t)replyPort
                     replyMsgs:(ASMsgRequest)messages
{
    region_t *region;
    port_t kernReplyPort;
    vm_address_t kmem;

    kernReplyPort = IOConvertPort(replyPort, IO_CurrentTask, IO_Kernel);
    if(kernReplyPort == PORT_NULL)
	messages = 0;	// No messages if reply port has already gone away!
    /*
     * Truncate count if necessary.
     */
    if (dataFormat == IOAudioDataFormatLinear16)
        byteCount &= (channelCount == 1 ? ~(2-1) : ~(4-1));

    if (!writeToKernel((vm_address_t)data, byteCount, &kmem)) {
        IOLog("Audio: playback request (%d bytes) too large\n", byteCount);
	return NO;
    }

    region = [self newRegion];
    region->data = region->enq_ptr = kmem;
    region->end = region->data + byteCount;
    region->count = byteCount;
    region->tag = aTag;
    region->reply_port = kernReplyPort;
    region->messages = messages;
    /*
     * Some reply messages are per stream in NXSound interface.
     * FIXME: these only get set if data sent to stream.
     */
    userReplyMessages = messages;
    userReplyPort = kernReplyPort;

    xpr_audio_stream("OS: playbuffer: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    queue_enter(&regionQueue, region, region_t *, link);
    [regionQueueLock unlock];
    xpr_audio_stream("OS: playbuffer: unlocked regionQueue\n", 1,2,3,4,5);

    [device _dataPendingForChannel: [self channel]];
    return YES;
}

/*
 * Overridden from superclass to allow some conversions.
 */
- (BOOL)canConvertRegion:(region_t *)region
             rate:(u_int)srate format:(IOAudioDataFormat)format
             channelCount:(u_int)chans;
{

    if ([super canConvertRegion:region rate:srate
       format:format channelCount:chans])
        return YES;
    /*
     * FIXME: support other format mixing and scaling.
     */
    if ((format == IOAudioDataFormatAlaw8) || (format == IOAudioDataFormatAES))
        return NO;
    /*
     * FIXME: support other rate conversions.
     */
    if (!((srate == 22050 && samplingRate == 44100) ||
        (srate == 44100 && samplingRate == 22050)))
        return NO;
    return YES;
}

/*
 * Clear to end of buffer so next stream can mix in.
 */
- clearForMix:(char *)buf size:(u_int)count format:(IOAudioDataFormat)format
{
    xpr_audio_stream("OS: clearForMix: %d bytes from 0x%x\n", count, buf,
                     3,4,5);
    if (format == IOAudioDataFormatLinear8)  /* && devIsUnary8 */
        while (count--)
            *buf++ = UNARY8_SILENCE;
    else if (format == IOAudioDataFormatMulaw8)
        while (count--)
            *buf++ = MULAW8_SILENCE;
    else
        bzero(buf, count);
    return self;
}

/*
 * Add an xfer record to a stream's xfer count queue.
 */
static void add_xfer_record(queue_t xferq, dma_desc_t *ddp, u_int count,
                            u_int peak_left, u_int peak_right, u_int clips)
{
    xfer_record_t *xfer_rec;

    if (!queue_empty(xferq)) {
        xfer_rec = (xfer_record_t *)queue_first(xferq);
        while (!queue_end(xferq, (queue_entry_t)xfer_rec)) {
            if ((ddp == xfer_rec->ddp) || !xfer_rec->ddp) {
                xfer_rec->ddp = ddp;
                xfer_rec->size = count;
                xfer_rec->peak_left = peak_left;
                xfer_rec->peak_right = peak_right;
                xfer_rec->clips = clips;
                xpr_audio_stream("OS: add_xfer_record: ddp=0x%x size=%d "
                                 "pl=%d pr=%d clips=%d\n", ddp, count,
                                 peak_left, peak_right, clips);
                return;
            }
            xfer_rec = (xfer_record_t *)queue_next(&xfer_rec->link);
        }
    }
}

/*
 * Return stream's xfer count on ddp.
 * Reset count to 0.
 */
static u_int xfer_count(queue_t xferq, dma_desc_t *ddp, u_int *peak_left,
                        u_int *peak_right, u_int *clips)
{
    xfer_record_t *xfer_rec;
    u_int count;

    if (!queue_empty(xferq)) {
        xfer_rec = (xfer_record_t *)queue_first(xferq);
        while (!queue_end(xferq, (queue_entry_t)xfer_rec)) {
            if (ddp == xfer_rec->ddp) {
                count = xfer_rec->size;
                xfer_rec->size = 0;
                *peak_left = xfer_rec->peak_left;
                *peak_right = xfer_rec->peak_right;
                *clips = xfer_rec->clips;
                xpr_audio_stream("OS: xfer_count: ddp=0x%x count=%d "
                                 "pl=%d pr=%d clips=%d\n", ddp, count,
                                 *peak_left, *peak_right, *clips);
                return count;
            }
            xfer_rec = (xfer_record_t *)queue_next(&xfer_rec->link);
        }
    }
    return 0;
}

#define CONV_NONE               0
#define CONV_SWAP               (1<<0)
#define CONV_SCALE              (1<<1)
#define CONV_22_44              (1<<2)
#define CONV_44_22              (1<<3)
#define CONV_MONO_STEREO        (1<<4)
#define CONV_STEREO_MONO        (1<<5)
#define CONV_LINEAR8_LINEAR16   (1<<6)
#define CONV_LINEAR8_MULAW8     (1<<7)
#define CONV_LINEAR16_LINEAR8   (1<<8)
#define CONV_LINEAR16_MULAW8    (1<<9)
#define CONV_MULAW8_LINEAR16    (1<<10)
#define CONV_MULAW8_LINEAR8     (1<<11)

/*
 * Mix playback data from region into buffer, returning count of
 * bytes mixed.
 */
- (u_int)mixRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            buffer:(vm_address_t)data maxCount:(u_int)max
            virgin:(BOOL)isVirgin rate:(u_int)srate
            format:(IOAudioDataFormat)format 
            channelCount:(u_int)chans
{
    u_int scale_clips = 0, peak_left = 0, peak_right = 0;
    u_int mix_clips = 0;
    u_int in_count, avail, out_count;
    u_int conversions = CONV_NONE;
    boolean_t doMix = TRUE;
    IOAudioDataFormat streamDataFormat = dataFormat;
    /*
     * NOTE: src is read-only - always write into aux1 or aux2.
     * aux1 is MIX_BUFFER1_SIZE, aux2 is MIX_BUFFER2_SIZE.
     */
    char *src, *aux1 = mixBuffer1, *aux2 = mixBuffer2;
    
    src = (char *)region->enq_ptr;
    in_count = out_count = max;
    avail = region->end - region->enq_ptr;
    if (in_count > avail)
        in_count = out_count = avail;

    /*
     * Determine what conversions are required.
     */
    if (streamDataFormat == IOAudioDataFormatLinear16)
        conversions |= CONV_SWAP;

    /*
     * FIXME: this breaks mono pan - need new API.
     */
    if (leftGain != UNITY_GAIN || rightGain != UNITY_GAIN)
        conversions |= CONV_SCALE;
    if (channelCount == 1 && chans == 2)
        conversions |= CONV_MONO_STEREO;
    else if (channelCount == 2 && chans == 1)
        conversions |= CONV_STEREO_MONO;
    if (samplingRate == 22050 && srate == 44100)
        conversions |= CONV_22_44;
    else if (samplingRate == 44100 && srate == 22050)
        conversions |= CONV_44_22;
    if (streamDataFormat == IOAudioDataFormatLinear8 &&
        format == IOAudioDataFormatLinear16)
        conversions |= CONV_LINEAR8_LINEAR16;
    else if (streamDataFormat == IOAudioDataFormatLinear8 &&
             format == IOAudioDataFormatMulaw8)
        conversions |= CONV_LINEAR8_MULAW8;
    else if (streamDataFormat == IOAudioDataFormatLinear16 &&
             format == IOAudioDataFormatLinear8)
        conversions |= CONV_LINEAR16_LINEAR8;
    else if (streamDataFormat == IOAudioDataFormatLinear16 &&
             format == IOAudioDataFormatMulaw8)
        conversions |= CONV_LINEAR16_MULAW8;
    else if (streamDataFormat == IOAudioDataFormatMulaw8 &&
             format == IOAudioDataFormatLinear16)
        conversions |= CONV_MULAW8_LINEAR16;
    else if (streamDataFormat == IOAudioDataFormatMulaw8 &&
             format == IOAudioDataFormatLinear8)
        conversions |= CONV_MULAW8_LINEAR8;

    /* FIXME: can phase variables just keep wrapping around or
       do they need to be set to 0 at some point? */

    /*
     * Perform conversion sequences.
     */
    switch (conversions) {
      case CONV_NONE:
        break;
      case CONV_SCALE:
        scale_clips = audio_scaleSamples(src, aux1, in_count, streamDataFormat,
                                         channelCount, (int)leftGain,
                                         (int)rightGain);
        src = aux1;
        break;
      case CONV_MONO_STEREO:
        in_count /= 2;
        audio_convertMonoToStereo(src, aux1, in_count, streamDataFormat);
        src = aux1;
        break;
      case CONV_STEREO_MONO:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        audio_convertStereoToMono(src, aux1, in_count, streamDataFormat,
                                  &channelPhase);
        src = aux1;
        break;
      case CONV_22_44:
        in_count /= 2;
        audio_resample22To44(src, aux1, in_count, streamDataFormat, channelCount );
        src = aux1;
        break;
      case CONV_44_22:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        audio_resample44To22(src, aux1, in_count, streamDataFormat,
			     &resamplePhase);
        src = aux1;
        break;
      case CONV_MONO_STEREO | CONV_22_44:
        in_count /= 4;
        audio_convertMonoToStereo(src, aux1, in_count, streamDataFormat);
        audio_resample22To44(aux1, aux2, in_count*2, streamDataFormat, channelCount);
	src = aux2;
        break;
      case CONV_STEREO_MONO | CONV_44_22:
        in_count *= 4;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 4;
        audio_convertStereoToMono(src, aux1, in_count, streamDataFormat,
				  &channelPhase);
        audio_resample44To22(aux1, aux2, in_count/2, streamDataFormat,
			     &resamplePhase);
	src = aux2;
        break;
      case CONV_MONO_STEREO | CONV_44_22:
        audio_convertMonoToStereo(src, aux1, in_count, streamDataFormat);
        audio_resample44To22(aux1, aux2, in_count*2, streamDataFormat,
			     &resamplePhase);
	src = aux2;
        break;
      case CONV_STEREO_MONO | CONV_22_44:
        audio_convertStereoToMono(src, aux1, in_count, streamDataFormat,
				  &channelPhase);
        audio_resample22To44(aux1, aux2, in_count/2, streamDataFormat, channelCount);
	src = aux2;
        break;
      case CONV_MULAW8_LINEAR16:
        in_count /= 2;
	audio_convertMulaw8ToLinear16(src, (short *)aux1, in_count);
	streamDataFormat = IOAudioDataFormatLinear16;
	src = aux1;
	break;
      case CONV_MULAW8_LINEAR8:
	audio_convertMulaw8ToLinear8(src, aux1, in_count);
	streamDataFormat = IOAudioDataFormatLinear8;
	src = aux1;
	break;
      case CONV_LINEAR8_LINEAR16:
        in_count /= 2;
	audio_convertLinear8ToLinear16(src, (short *)aux1, in_count);
	streamDataFormat = IOAudioDataFormatLinear16;
	src = aux1;
	break;
      case CONV_LINEAR8_MULAW8:
	audio_convertLinear8ToMulaw8(src, aux1, in_count);
	streamDataFormat = IOAudioDataFormatMulaw8;
	src = aux1;
	break;

      case CONV_SWAP:
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        src = aux1;
        break;
      case CONV_SWAP | CONV_SCALE:
	audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        scale_clips = audio_scaleSamples(aux1, aux2, in_count,
					 streamDataFormat,
                                         channelCount, (int)leftGain,
                                         (int)rightGain);
        src = aux2;
        break;
      case CONV_SWAP | CONV_MONO_STEREO:
        in_count /= 2;
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertMonoToStereo(aux1, aux2, in_count, streamDataFormat);
        src = aux2;
        break;
      case CONV_SWAP | CONV_STEREO_MONO:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        /*
         * StereoToMono looks at bits, must swap first.
         */
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertStereoToMono(aux1, aux2, in_count, streamDataFormat,
                                  &channelPhase);
        src = aux2;
        break;
      case CONV_SWAP | CONV_22_44:
        if ( in_count*2 >= max )
        {
            in_count  = max/2;
            out_count = max;
        }
        else
        {
            out_count = avail*2;
            in_count  = avail;
        }

//        in_count /= 2;

        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_resample22To44(aux1, aux2, in_count, streamDataFormat, channelCount);
        src = aux2;
        break;
      case CONV_SWAP | CONV_44_22:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        /*
         * Resampling currently doesn't look at bits, so we can swap last.
         */
        audio_resample44To22(src, aux1, in_count, streamDataFormat,
			     &resamplePhase);
        audio_swapSamples((short *)aux1, (short *)aux2, out_count/2);
        src = aux2;
        break;
      case CONV_SWAP | CONV_MONO_STEREO | CONV_22_44:
        in_count /= 4;
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertMonoToStereo(aux1, aux2, in_count, streamDataFormat);
        audio_resample22To44(aux2, aux1, in_count*2, streamDataFormat, channelCount);
	src = aux1;
        break;
      case CONV_SWAP | CONV_STEREO_MONO | CONV_44_22:
        in_count *= 4;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 4;
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertStereoToMono(aux1, aux2, in_count, streamDataFormat,
				  &channelPhase);
        audio_resample44To22(aux2, aux1, in_count/2, streamDataFormat,
			     &resamplePhase);
	src = aux1;
        break;
      case CONV_SWAP | CONV_MONO_STEREO | CONV_44_22:
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertMonoToStereo(aux1, aux2, in_count, streamDataFormat);
        audio_resample44To22(aux2, aux1, in_count*2, streamDataFormat,
			     &resamplePhase);
	src = aux1;
        break;
      case CONV_SWAP | CONV_STEREO_MONO | CONV_22_44:
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
        audio_convertStereoToMono(aux1, aux2, in_count, streamDataFormat,
				  &channelPhase);
        audio_resample22To44(aux2, aux1, in_count/2, streamDataFormat, channelCount);
	src = aux1;
        break;
      case CONV_SWAP | CONV_LINEAR16_LINEAR8:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
	audio_convertLinear16ToLinear8((short *)aux1, aux2, in_count/2,
				       &formatPhase);
	streamDataFormat = IOAudioDataFormatLinear8;
	src = aux2;
	break;
      case CONV_SWAP | CONV_LINEAR16_MULAW8:
        in_count *= 2;
        if (in_count > avail)
            in_count = avail;
        out_count = in_count / 2;
        audio_swapSamples((short *)src, (short *)aux1, in_count/2);
	audio_convertLinear16ToMulaw8((short *)aux1, aux2, in_count/2,
				      &formatPhase);
	streamDataFormat = IOAudioDataFormatMulaw8;
	src = aux2;
	break;

      default:
        IOLog("Audio: unsupported mixing conversion 0x%x\n", conversions);
	doMix = FALSE;
        break;
    }

    if (doMix) {
	if (peakEnabled) {
	    if (streamDataFormat == IOAudioDataFormatLinear16)
		audio_linear16_peak(channelCount, (short *)src, out_count,
				    &peak_left, &peak_right);
	    else if (streamDataFormat == IOAudioDataFormatLinear8)
		audio_linear8_peak(channelCount, (char *)src, out_count,
				   &peak_left, &peak_right);
	    else if (streamDataFormat == IOAudioDataFormatMulaw8)
		audio_mulaw8_peak(channelCount, (unsigned char *)src,
				  out_count,
				  &peak_left, &peak_right);
	}
	
	mix_clips = audio_mix(src, (char *)data, out_count, streamDataFormat,
			      isVirgin);
    }
    
    region->enq_ptr += in_count;
    add_xfer_record(&xferQueue, ddp, in_count,
                    peak_left, peak_right,
                    (scale_clips > mix_clips ? scale_clips : mix_clips));
    xpr_audio_stream("OS: mixRegion: mixed in %d bytes\n", out_count, 2,3,4,5);

    return out_count;
}

/*
 * Update peak, transfer and clip counts.
 */
- completeRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            size:(u_int)xfer used:(u_int *)used
{
    u_int peak_left, peak_right, clips;

    bytesProcessed += xfer_count(&xferQueue, ddp, &peak_left, &peak_right,
                                 &clips);
    xpr_audio_stream("OS: completeRegion: play stream nxfer = %d "
                     "clips=%d\n", bytesProcessed, clips, 3,4,5);
    [channel incrementClipCount:clips];

    if (peakEnabled) {
        audio_add_peak(peaksLeft, peak_left, &currentPeak, peakHistory);
        audio_add_peak(peaksRight, peak_right, &currentPeak, peakHistory);
        xpr_audio_stream("OS: completeRegion: stream peaks %d %d\n", peak_left,
                         peak_right, 3,4,5);
    }
    return self;
}

- (unsigned int)gainLeft
{
    return leftGain;
}
- (unsigned int)gainRight
{
    return rightGain;
}
- (void)setGainLeft:(unsigned int)gain
{
    leftGain = gain;
}
- (void)setGainRight:(unsigned int)gain
{
    rightGain = gain;
}

/*
 * Get and set peak parameters.
 */
- (BOOL)isDetectingPeaks
{
    return peakEnabled;
}

- (void)setDetectPeaks:(BOOL)flag;
{
    peakEnabled = flag;
    if (peakEnabled && !peaksLeft) {
        peaksLeft = (u_int *)IOMalloc(MAX_PEAK_HISTORY*sizeof(u_int));
        peaksRight = (u_int *)IOMalloc(MAX_PEAK_HISTORY*sizeof(u_int));
        audio_clear_peaks(peaksLeft, MAX_PEAK_HISTORY);
        audio_clear_peaks(peaksRight, MAX_PEAK_HISTORY);
    }
}

/*
 * Get peaks.
 */
- getPeakLeft:(u_int *)leftPeak right:(u_int *)rightPeak
{
    if (peakEnabled) {
        *leftPeak = audio_max_peak(peaksLeft, peakHistory);
        *rightPeak = audio_max_peak(peaksRight, peakHistory);
    } else
        *leftPeak = *rightPeak = 0;
    return self;
}

/*
 * Free a region.
 */
- freeRegion:(region_t *)region
{
    kern_return_t krtn;
    vm_task_t kernelTask = (vm_task_t)kern_serv_kernel_task_port();

    xpr_audio_stream("OS: freeRegion: dealloc %d bytes at 0x%x from region "
    		     "0x%x\n", region->count, region->data, region, 4,5);
    krtn = vm_deallocate(kernelTask, region->data, region->count);
    if (krtn != KERN_SUCCESS)
        IOLog("Audio: stream vm_deallocate error %d\n", krtn);

    return [super freeRegion:region];
}

/*
 * Overriden from superclass.
 */
- free
{
    xfer_record_t *xfer_rec;

    if (peaksRight)
        IOFree(peaksRight, MAX_PEAK_HISTORY*sizeof(u_int));
    if (peaksLeft)
        IOFree(peaksLeft, MAX_PEAK_HISTORY*sizeof(u_int));

    while(!queue_empty(&xferQueue)) {
        queue_remove_first(&xferQueue, xfer_rec, xfer_record_t *, link);
        IOFree(xfer_rec, sizeof(xfer_record_t));
    }
    if (mixBuffer1)
        IOFree(mixBuffer1, MIX_BUFFER1_SIZE);
    if (mixBuffer2)
        IOFree(mixBuffer2, MIX_BUFFER2_SIZE);
    return [super free];
}

@end
