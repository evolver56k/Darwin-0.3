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
 * AudioChannel.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver channel object.
 *
 * HISTORY
 *	06/22/94/wmg	HP modifications.
 *      11/17/93/rkd    Added XPRs.
 *      07/14/92/mtm    Original coding.
 */
 
#import "AudioChannel.h"
#import "AudioStream.h"
#import "InputStream.h"
#import "OutputStream.h"
#import <driverkit/IOAudio.h>
#import <driverkit/IOAudioPrivate.h>
#import "AudioCommand.h"
#import "audio_peak.h"
#import "audio_msgs.h"
#import "audioLog.h"
#import "AudioCommand.h"
#import "audio_kern_server.h"
#import <mach/vm_param.h>		// PAGE_SIZE
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>	// IOPhysicalFromVirtual / IOVmTaskSelf
#import <bsd/string.h>			// bzero

#if	i386
/*
 * Temporary function prototype for alloc_cnvmem
 * copied from <machdep/i386/i386_init.c)
 */
vm_offset_t alloc_cnvmem(vm_size_t size, vm_offset_t align);
#endif	i386
#if	hppa
#import <machdep/hppa/pmap.h>
extern hpaudiobuffer;
extern hpaudiosilent;
#endif	hppa
#if	sparc
//
// set in device init routine
//
u_int AudioIn_dmaSize = 0;
u_int AudioIn_dmaCount = 0;
caddr_t AudioIn_dmaBuf = 0;
u_int AudioOut_dmaSize = 0;
u_int AudioOut_dmaCount = 0;
caddr_t AudioOut_dmaBuf = 0;
#endif sparc

#if ppc
extern void *kernel_map;
#endif

@implementation AudioChannel

/* Factory variables */
static id allStreams = nil;

/*
 * Both input and output channels share one buffer. 
 */
static BOOL sharedChannelBuffer = YES;
static vm_offset_t sharedChannelBufferPtr = 0;

/*
 * Add a stream.
 */
+ addStream:stream
{
    if (!allStreams)
	allStreams = [[List alloc] init];
    [allStreams addObject:stream];
    return self;
}

/*
 * Remove a stream.
 */
+ removeStream:stream
{
    [allStreams removeObject:stream];
    return self;
}

/*
 * Search for a stream given a user port.
 */
+ streamForUserPort:(port_t)port
{
    int i;
    id stream;

    for (i = 0; i < [allStreams count]; i++) {
	stream = [allStreams objectAt:i];
	if ([stream userPort] == port)
	    return stream;
    }
    return nil;
}

/*
 * Search for a stream given an owner port.
 * Returns the FIRST such stream found.
 */
+ streamForOwnerPort:(port_t)port
{
    int i;
    id stream;

    for (i = 0; i < [allStreams count]; i++) {
	stream = [allStreams objectAt:i];
	if ([stream ownerPort] == port)
	    return stream;
    }
    return nil;
}

/*
 * Initialize.
 */
 
- initOnDevice: device read: (BOOL)isRead
{
    [super init];
    audioDevice = device;
    
    _isRead = isRead;
    
    if (isRead)
        streamClass = [InputStream class];
    else
        streamClass = [OutputStream class];
    
    exclusiveUser = PORT_NULL;
    streamList = [[List alloc] init];
    streamListLock = [[NXLock alloc] init];
    queue_init(&dmaQueue);
    queue_init(&freeQueue);
#if	hppa
    queue_init(&devQueue);
    queue_init(&dfreeQueue);
#endif	hppa
    peakHistory = 1;
    
    channelBufferPtr = 0;
    /*
     * Note:
     * Until the DMA buffer can be reallocated, the dmaSize cannot be changed.
     */
#if	i386
    if ([audioDevice isEISAPresent])	{
    	[self setDMASize: PAGE_SIZE * 16];
	(void) [self setDescriptorSize: PAGE_SIZE];
    } else {
    	[self setDMASize: PAGE_SIZE * 8];		// 64K limit
	(void) [self setDescriptorSize: PAGE_SIZE];
    }
#elif	hppa
	// uses 64k too
	dmaSize = PAGE_SIZE * 8;
	dmaCount = 16;
	(void) [self setDescriptorSize: (dmaSize/dmaCount)];
#elif sparc
    // dmaSize = PAGE_SIZE * 8;
    // dmaCount = 16;
	// 16 - 4K buffers
	if (isRead) {
		dmaSize = AudioIn_dmaSize;
		dmaCount = AudioIn_dmaCount;
	} else {
		dmaSize = AudioOut_dmaSize;
		dmaCount = AudioOut_dmaCount;
	}
	(void) [self setDescriptorSize: (dmaSize/dmaCount)];
#elif ppc
        dmaCount = 32;
        dmaSize = PAGE_SIZE * dmaCount;
        (void) [self setDescriptorSize: (dmaSize/dmaCount)];
#else
    dmaSize = PAGE_SIZE * 8;
    dmaCount = 16;
#endif

    
    return self;
}
    
/*
 * Currently cannot free, but if want to someday, worry about:
 * destroyDMABuffers; and
 * Free peak history buffers
 */
- free
{
    IOLog("AudioChannel: -free not supported\n");
    return [super free];
}

/*
 *
 */
- (u_int) descriptorSize
{
    return descriptorSize;
}

/*
 * Returns the desciptorSize which may be different from the one requested. 
 */
- (u_int) setDescriptorSize:(u_int) size
{
    dmaCount = [self dmaSize] / size;

#ifndef ppc
    if (dmaCount < 4 || dmaCount > 16) {
	dmaCount = 8;
	size = [self dmaSize] / dmaCount;
    }
#endif

    descriptorSize = size;
    
    return descriptorSize;
}

- (u_int) dmaSize
{
    return dmaSize;
}

- (void) setDMASize:(u_int) size
{
    dmaSize = size;
}

- (u_int)dmaCount
{
    return dmaCount;
}

- (BOOL) isRead
{
    return _isRead;
}

/*
 * Return number of outstanding (enqueued) dma descriptors on dmaQueue.
 */
- (u_int)enqueueCount
{
    return enqueueCount;
}

/*
 * Create channel dma buffers.
 * There must be no active streams when this is called.
 */
- (BOOL) createChannelBuffer
{
    BOOL status;
    int len;
#ifdef hppa
    extern pmap_t audio_pmap;  // defined in IOAudio.m
#endif
    
    /*
     * To Do: Until better versions of IOMallocLow and IOFreeLow are available,
     * the channel will use alloc_cnvmem (allocate conventional memory) which
     * can allocate more than one physically contiguous page. The memory cannot
     * be freed; therefore, the code checks to see if the channelBuffer has
     * been created.
     */   	
    if (channelBufferPtr)
    	return TRUE;
	
#if	i386
    if ((sharedChannelBuffer) && (sharedChannelBufferPtr))	{
	channelBufferPtr = sharedChannelBufferPtr;
	
	channelBuffer = [audioDevice createDMABufferFor: &channelBufferPtr
	    length: dmaSize
	    read:(BOOL) _isRead
	    needsLowMemory: TRUE
	    limitSize: NO];
    
	[self initializeFreeQueue];
	return TRUE;
    }

    /*
     * We treat ISA systems as special case since other systems do not have
     * DMA memory restrictions like it. For ISA we assign memory using
     * alloc_envmem() and for others we will use an IOBuffer object
     * (eventually). 
     */
    if (![audioDevice isEISAPresent])	{
        channelBufferPtr = alloc_cnvmem(dmaSize, 65536); 	// 64K align
	if (!channelBufferPtr)	{
	    IOLog("Audio: no memory for allocating buffers.\n");
	    return FALSE;
	}
    } else if ([audioDevice isEISAPresent])	{
        channelBufferPtr = alloc_cnvmem(dmaSize, PAGE_SIZE);
	if (!channelBufferPtr)	{
	    IOLog("Audio: no memory for allocating buffers.\n");
	    return FALSE;
	}
    }
#elif	hppa
    if (_isRead) {
	// if read it is read buffer
	channelBufferPtr = (vm_address_t)(&hpaudiosilent);
    } else {
	channelBufferPtr = (vm_address_t)(&hpaudiobuffer);
    }
#elif sparc
	if (_isRead) 
		channelBufferPtr = (vm_offset_t)AudioIn_dmaBuf;
	else 
		channelBufferPtr = (vm_offset_t)AudioOut_dmaBuf;
#elif ppc
   {
     int	kr;
     kr = kmem_alloc_wired( kernel_map, (vm_offset_t)&channelBufferPtr, dmaSize );
     if ( kr != KERN_SUCCESS )
     {
       IOLog("Audio: no memory for allocating buffers.\n");
       return FALSE;
     }
   }
#else
#warning	AudioChannel class only supports i386 , hppa or sparc
#endif

    sharedChannelBufferPtr = channelBufferPtr;
    
#if	i386
    channelBuffer = [audioDevice createDMABufferFor: &channelBufferPtr
        length: dmaSize
        read:(BOOL) _isRead
        needsLowMemory: TRUE
        limitSize: NO];
#else
    // for hppa and sparc there is no seperate setups for DMA
    channelBuffer = (void *)channelBufferPtr;
#endif

    [self initializeFreeQueue];
#if	hppa
    [self initializeDFreeQueue];
#endif
    return TRUE;
}

- (void) initializeFreeQueue
{
    dma_desc_t *ddp;
    int i;
    vm_address_t ptr;
    
    while (!queue_empty(&freeQueue)) {
	queue_remove_first(&freeQueue, ddp, dma_desc_t *, link);
	IOFree((void *)ddp, sizeof(dma_desc_t));
    }

    bzero((void *)channelBufferPtr, dmaSize);

    ptr = channelBufferPtr;

    for (i = 0; i < dmaCount; i++) {
	ddp = (dma_desc_t *)IOMalloc(sizeof(dma_desc_t));
	ddp->size = 0;
	ddp->mem = ptr;
	ptr += descriptorSize;
	queue_enter(&freeQueue, ddp, dma_desc_t *, link);
    }
}

#if	hppa
- (void) initializeDFreeQueue
{
    dma_desc_t *ddp;
    int i;
    vm_address_t ptr;
    
    while (!queue_empty(&dfreeQueue)) {
	queue_remove_first(&dfreeQueue, ddp, dma_desc_t *, link);
	IOFree((void *)ddp, sizeof(dma_desc_t));
    }

    bzero((void *)channelBufferPtr, dmaSize);

	pmap_flush_range(kernel_pmap, (vm_offset_t)channelBufferPtr, dmaSize);


    ptr = channelBufferPtr;

    for (i = 0; i < dmaCount; i++) {
		ddp = (dma_desc_t *)IOMalloc(sizeof(dma_desc_t));
		ddp->size = 0;
		ddp->mem = ptr;
		ptr += descriptorSize;
		queue_enter(&dfreeQueue, ddp, dma_desc_t *, link);
    }
}

/* returns the device dma address also releases the queue
 * 
 */

- (vm_offset_t) getDevDmaAddress
{
    dma_desc_t *ddp_d;	// for the dev queue
    vm_offset_t dev_dma_addr = (vm_offset_t)NULL;
    vm_offset_t addr;
     extern pmap_t audio_pmap; // defined in IOAudio.m

	if (queue_empty(&devQueue)) return (vm_offset_t)NULL;
    	queue_remove_first(&devQueue, ddp_d, dma_desc_t *, link);
	// now the buffer is a kernel buffer mapped 1 to 1
	// dev_dma_addr = ddp_d->mem;
	dev_dma_addr = pmap_extract(kernel_pmap,ddp_d->mem);
	
    	queue_enter(&dfreeQueue, ddp_d, dma_desc_t *, link);
	return dev_dma_addr;
}
#endif	hppa

/*
 * Destroy dma buffers.
 * There must be no active streams when this is called.
 */
- (void) destroyChannelBuffer
{
    
    /*
     *	The channelBuffer cannot be freed until a better version
     * 	of IOFreeLow is available.
     */
}

/*
 * enqueue a dma descriptor using the current device parameters.
 */
- (BOOL) enqueueDescriptor: (u_int *) rate
	dataFormat: (IOAudioDataFormat *) format
	channelCount: (u_int *) count
{
    id stream;
    int i;
    u_int bufferSize = 0;
    u_int streamCount;
    u_int streamSize;
    dma_desc_t *ddp;
#if	hppa
    dma_desc_t *ddp_d;	// for the dev queue
#endif	hppa

    xpr_audio_channel("AC: enqueueDescriptor: rate %d dataFormat %d "
    		"channelCount %d\n", rate, format, count, 4,5);
    [streamListLock lock];
    streamCount = [streamList count];
    if (streamCount == 0 || queue_empty(&freeQueue)) {
	[streamListLock unlock];
	return FALSE;
    }
    queue_remove_first(&freeQueue, ddp, dma_desc_t *, link);
    for (i = 0; i < streamCount; i++) {
	stream = [streamList objectAt:i];
	streamSize = [stream mixBuffer:ddp->mem maxCount:descriptorSize
		                  rate:rate format:format
		          channelCount:count descriptor:ddp
                                virgin:(bufferSize == 0)
                           streamCount:streamCount];
	if (streamSize > bufferSize)
	    bufferSize = streamSize;
    }
    [streamListLock unlock];
    if (bufferSize == 0) {
	queue_enter_first(&freeQueue, ddp, dma_desc_t *, link);
	return(FALSE);
    }
    
    ddp->size = bufferSize;	
#ifdef sparc
	vac_flush((caddr_t)ddp->mem, ddp->size);
#endif
#ifdef hppa
	pmap_flush_range(kernel_pmap,(vm_offset_t)ddp->mem,ddp->size);
#endif
#ifdef ppc
        flush_cache_v((vm_offset_t)ddp->mem,ddp->size);
#endif
    queue_enter(&dmaQueue, ddp, dma_desc_t *, link);
#ifdef hppa
    if (!queue_empty(&dfreeQueue)) {
	    queue_remove_first(&dfreeQueue, ddp_d, dma_desc_t *, link);
	    ddp_d->mem = ddp->mem;
	    ddp_d->size = ddp->size;
	    queue_enter(&devQueue, ddp_d, dma_desc_t *, link);
    }
#endif

    enqueueCount++;

    xpr_audio_channel("AC: enqueueDescriptor done\n",1,2,3,4,5);
    
    return(TRUE);
}


/*
 * dequeue a DMA descriptor
 */
- (void) dequeueDescriptor
{
    dma_desc_t *ddp;
    int i;
    id stream;
    u_int peak_left = 0, peak_right = 0;
    u_int numStreams;

    xpr_audio_channel("AC: dequeueDescriptor\n",1,2,3,4,5);
    
    queue_remove_first(&dmaQueue, ddp, dma_desc_t *, link);

    [streamListLock lock];
    numStreams = [streamList count];
    if (numStreams > 0) {
	if (peakEnabled) {
	    switch ([audioDevice dataEncoding]) {
	      case NX_SoundStreamDataEncoding_Linear16:
		audio_linear16_peak([audioDevice channelCount],
				    (short *)ddp->mem, ddp->size/2,
				    &peak_left, &peak_right);
		break;
		
	      case NX_SoundStreamDataEncoding_Linear8:
		audio_linear8_peak([audioDevice channelCount],
				   (char *)ddp->mem, ddp->size,
				   &peak_left, &peak_right);
		break;
		
	      case NX_SoundStreamDataEncoding_Mulaw8:
		audio_mulaw8_peak([audioDevice channelCount],
				  (unsigned char *)ddp->mem,
				  ddp->size, &peak_left, &peak_right);
		break;
		
	      default:
		/* FIXME: A-law and AES peak not supported */
		break;
	    }
	    audio_add_peak(peaksLeft, peak_left, &currentPeak, peakHistory);
	    audio_add_peak(peaksRight, peak_right, &currentPeak, peakHistory);
	}
	
	for (i = 0; i < numStreams; i++) {
	    stream = [streamList objectAt:i];
	    /*
	     * Note: transfer count is only valid for input channel
	     * (0 sent for output channel).
	     */
	    [stream dmaCompleteDescriptor:ddp transfered:ddp->size];
	    
	    /*
	     * In the original driver, the dequeueDMA operation would return
	     * the actually transfer count. We need to offer a similar
	     * solution at some point:
	     * [stream dmaCompleteDescriptor:ddp transfered:count];
	     */
	}
    }
    [streamListLock unlock];

    bzero((void *)ddp->mem, descriptorSize);

#ifdef ppc
    flush_cache_v((vm_offset_t)ddp->mem,descriptorSize);
#endif

    queue_enter(&freeQueue, ddp, dma_desc_t *, link);
    
    --enqueueCount;
    xpr_audio_channel("AC: dequeueDescriptor done\n",1,2,3,4,5);
}


/*
 * returns the channel buffer as a void *
 */
- (void *) channelBuffer
{
    return channelBuffer;
}

- (vm_address_t) channelBufferAddress
{
    return channelBufferPtr;
}

- (void) freeDescriptors
{
    /*
     * When a DMA operation is aborted, descriptors might still remain on
     * the DMA queue.
     */
    while (!queue_empty(&dmaQueue)) 
        [self dequeueDescriptor];

   [self initializeFreeQueue];
}

- (void) setLocalChannel:(u_int) channel
{
    localChannel = channel;
}

- (u_int) localChannel
{
    return localChannel;
}

/*
 * Get and set user channel and snd ports.
 */
- setUserChannelPort:(port_t)port
{
    userChannelPort = port;
    return self;
}
- (port_t)userChannelPort
{
    return userChannelPort;
}
- setUserSndPort:(port_t)port
{
    userSndPort = port;
    return self;
}
- (port_t)userSndPort
{
    return userSndPort;
}

/*
 * Search for the FIRST stream with the given owner port
 * (used by the snd interface, which has one stream per channel).
 * Returns the stream's user port.
 */
- (port_t)streamUserForOwnerPort:(port_t)port
{
    int i;
    id stream;

    for (i = 0; i < [streamList count]; i++) {
	stream = [streamList objectAt:i];
	if ([stream ownerPort] == port)
	    return [stream userPort];
    }
    return PORT_NULL;
}

/*
 * Remove all snd type streams.
 */
- removeSndStreams
{
    id stream;
    List *remList = [[List alloc] init];
    int i;

    [streamListLock lock];
    for (i = 0; i < [streamList count]; i++) {
	stream = [streamList objectAt:i];
	if ([stream type] != AS_TypeUser)
	    [remList addObject:stream];
    }
    [streamListLock unlock];

    for (i = 0; i < [remList count]; i++)
	[self removeStream:[remList objectAt:i]];

    [remList free];
    return self;
}

/*
 * Get and set exclusive user.
 */
- (void)setExclusiveUser:(port_t)streamPort
{
    int streamCount, i;

    exclusiveUser = streamPort;
    /*
     * Cause other owner's streams to abort with excluded messages.
     */
    [streamListLock lock];
    streamCount = [streamList count];
    if (streamCount == 0) {
	[streamListLock unlock];
	return;
    }
    for (i = 0; i < streamCount; i++)
	[[streamList objectAt:i] control:AC_ControlExclude];
    [streamListLock unlock];
}
- (port_t)exclusiveUser
{
    return exclusiveUser;
}

/*
 * Check prospective owner port against current exclusive user port.
 */
- (BOOL)checkOwner:(port_t)owner
{
    if (exclusiveUser != PORT_NULL && owner != exclusiveUser)
	return NO;
    else
	return YES;
}

/*
 * 
 */
- streamClass
{
    return streamClass;
}

/*
 * Return number of streams
 */
- (u_int)streamCount
{
    u_int count;

    [streamListLock lock];
    count = [streamList count];
    [streamListLock unlock];
    return count;
}

/*
 * When a stream is added, its user port must be registered with the
 * audio loadable kernel server.
 */
- (BOOL)addStreamTag:(int)tag user:(port_t *)userPort
      owner:(port_t)owner type:(u_int)type
{
    id aStream;

    xpr_audio_channel("AC: addStreamTag tag=%d user=%x owner=%x"
    		 " type=%d\n", tag, userPort, owner, type, 5);
    
    if (![audioDevice _channelWillAddStream])	{
        xpr_audio_channel("AC: audioDevice can not add stream.\n",1,2,3,4,5);
	return NO;
    }

    aStream = [[[self streamClass] alloc] initChannel:self tag:tag
	                                         user:userPort
	                                        owner:owner type:type];
    if (!aStream)	{
        xpr_audio_channel("AC: addStreamTag: can not add stream.\n",1,2,3,4,5);
	return NO;
    }

    [streamListLock lock];

    if ([streamList count] == 0) {
	if (![self createChannelBuffer]) {
	    [streamListLock unlock];
	    xpr_audio_channel("AC: addStreamTag: can not create"
	    	" channel buffer.\n",1,2,3,4,5);
	    return NO;
	}
    }

    [streamList addObject:aStream];
    [streamListLock unlock];
    [AudioChannel addStream:aStream];
    
    return audio_enroll_stream_port(*userPort, TRUE);
}

/*
 * Remove a stream.
 */
- (void)removeStream:stream
{
    unsigned int count;
    
    audio_enroll_stream_port([stream userPort], FALSE);
    [streamListLock lock];
    count = [streamList count];
    
    /*
     * When only one stream remains, DMA is aborted when the stream is removed.
     */
    if (count == 1) {
    
        /*
	 * The streamListLock needs to be unlocked to prevent deadlock. When
	 * the abort command is sent to the IO thread, the dequeueDescriptor
	 * method will be executed. This method needs to acquire the lock. 
	 */
	[streamListLock unlock];

        if ([self isRead])
	    [[audioDevice _audioCommand] send: abortInputChannel];
	else
	    [[audioDevice _audioCommand] send: abortOutputChannel];
	    
	[streamListLock lock];
	
	if (peaksLeft)
	    audio_clear_peaks(peaksLeft, MAX_PEAK_HISTORY);
	if (peaksRight)
	    audio_clear_peaks(peaksRight, MAX_PEAK_HISTORY);
	clipCount = 0;

    }

    [streamList removeObject:stream];
    [AudioChannel removeStream:stream];
    [stream free];
    [streamListLock unlock];

}


/*
 * Send stream control to all streams.
 */
- (void)controlStreams:(ACStreamControl)action
{
    int streamCount, i;

    [streamListLock lock];
    streamCount = [streamList count];
    if (streamCount == 0) {
	[streamListLock unlock];
	return;
    }
    for (i = 0; i < streamCount; i++)
	[[streamList objectAt:i] control:action];
    [streamListLock unlock];
}

- (BOOL)isDetectingPeaks
{
    return peakEnabled;
}

- (void)setDetectPeaks:(BOOL)flag;
{
    /*
     * FIXME: unsupport history.
     */
    peakEnabled = flag;
    if (peakEnabled && !peaksLeft) {
	peaksLeft = (u_int *)IOMalloc(MAX_PEAK_HISTORY*sizeof(u_int));
	peaksRight = (u_int *)IOMalloc(MAX_PEAK_HISTORY*sizeof(u_int));
	audio_clear_peaks(peaksLeft, MAX_PEAK_HISTORY);
	audio_clear_peaks(peaksRight, MAX_PEAK_HISTORY);
    }
}

/*
 * Get peaks and clip count.
 */
- (void)getPeakLeft:(u_int *)leftPeak right:(u_int *)rightPeak
{
    if (peakEnabled) {
	*leftPeak = audio_max_peak(peaksLeft, peakHistory);
	*rightPeak = audio_max_peak(peaksRight, peakHistory);
    } else
	*leftPeak = *rightPeak = 0;
}
- (u_int)clipCount
{
    return clipCount;
}
- incrementClipCount:(u_int)count
{
    clipCount += count;
    return self;
}

/*
 * Return device.
 */
- audioDevice
{
    return audioDevice;
}

@end
