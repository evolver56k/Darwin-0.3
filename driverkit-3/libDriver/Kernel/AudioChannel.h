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
 * AudioChannel.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver channel object.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import <kern/lock.h>
#import "audio_types.h"
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>

#import <objc/List.h>
#import <machkit/NXLock.h>
#import <kernserv/queue.h>

/*
 * DMA descriptor structure.
 */
typedef struct dma_desc {
    u_int		size;
    vm_address_t	mem;
    queue_chain_t	link;
} dma_desc_t;
    
@interface AudioChannel: Object
{
    id				audioDevice;
    id				streamClass;
    id				streamList;
    id				streamListLock;

    port_t			exclusiveUser;
    port_t			userChannelPort;
    port_t			userSndPort;

    BOOL			_isRead;
    
    queue_head_t		dmaQueue;
    queue_head_t		freeQueue;
#ifdef hppa
    // These are for the device to handle
    queue_head_t		devQueue;
    queue_head_t		dfreeQueue;
#endif
    
    u_int			enqueueCount;

    u_int			dmaSize;
    u_int			dmaCount;
    u_int			descriptorSize;

    u_int			localChannel;
    void *			channelBuffer;
    
    vm_offset_t			channelBufferPtr;
    
    boolean_t			peakEnabled;
    u_int			peakHistory;
    u_int			*peaksLeft;
    u_int			*peaksRight;
    u_int			currentPeak;
    u_int			clipCount;
}

+ addStream:stream;
+ streamForUserPort:(port_t)port;
+ streamForOwnerPort:(port_t)port;

- initOnDevice: device read: (BOOL)isRead;
- free;

- (u_int) descriptorSize;
- (u_int) setDescriptorSize:(u_int)size;

- (u_int) dmaSize;
- (void)setDMASize: (u_int)count;

- (u_int)dmaCount;

- (u_int)enqueueCount;

- (BOOL) createChannelBuffer;
- (void) destroyChannelBuffer;

- (void *) channelBuffer;
- (vm_address_t) channelBufferAddress;

- (BOOL) enqueueDescriptor: (u_int *) rate
	dataFormat: (IOAudioDataFormat *) format
	channelCount: (u_int *) count;

- (void) dequeueDescriptor;

- (void) freeDescriptors;
- (void) setLocalChannel: (u_int) channel;
- (u_int) localChannel;

- (BOOL) isRead;
- setUserChannelPort:(port_t)port;
- (port_t)userChannelPort;

- setUserSndPort:(port_t)port;
- (port_t)userSndPort;

- (port_t)streamUserForOwnerPort:(port_t)port;
- removeSndStreams;

- (void)setExclusiveUser:(port_t)streamPort;
- (port_t)exclusiveUser;

- (BOOL)checkOwner:(port_t)owner;

- (BOOL)addStreamTag:(int)tag user:(port_t *)userPort
      owner:(port_t)owner type:(u_int)type;
- (void)removeStream:stream;
- streamClass;
- (u_int)streamCount;

- (BOOL)isDetectingPeaks;
- (void)setDetectPeaks:(BOOL)flag;
- (void)getPeakLeft:(u_int *)leftPeak right:(u_int *)rightPeak;

- (void)controlStreams:(ACStreamControl)action;

- (u_int)clipCount;
- incrementClipCount:(u_int)count;

- audioDevice;
- (void) initializeFreeQueue;
#ifdef hppa
- (void) initializeDFreeQueue;
- (vm_offset_t) getDevDmaAddress;
#endif

@end
