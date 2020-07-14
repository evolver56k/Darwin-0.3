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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * ISA/EISA device description class.
 *
 * HISTORY
 *
 * 18Jan93 Brian Pinkerton at NeXT
 *	Created.
 */
 
#define KERNEL_PRIVATE	1

#import <objc/List.h>

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/i386/EISAKernBus.h>

#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/i386/IOEISADeviceDescriptionPrivate.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/IOProperties.h>

struct _private {
    unsigned int	*channels;
    unsigned int	numChannels;
    IORange		*portRanges;
    unsigned int	numPortRanges;
    BOOL		valid;
    unsigned int	slotNum;
    unsigned long	slotID;
};

@implementation IOEISADeviceDescription

- free
{
    struct _private	*private = _eisa_private;
    
    if (private->numChannels > 0)
    	IOFree(private->channels,
		private->numChannels * sizeof (*private->channels));
    if (private->numPortRanges > 0)
    	IOFree(private->portRanges,
		private->numPortRanges * sizeof (*private->portRanges));
			
    IOFree(private, sizeof (*private));
    
    return [super free];
}

- (unsigned int) channel
{
    return [self channelList][0];
}


- (unsigned int *) channelList
{
    struct _private	*private = _eisa_private;

    if (private->numChannels == 0)
    	private->channels = [self
				_fetchItemList:[[self _delegate] 
					resourcesForKey:DMA_CHANNELS_KEY]
				    returnedNum:&private->numChannels];
    return private->channels;
}

- (unsigned int) numChannels
{
    struct _private	*private = _eisa_private;

    if (private->numChannels == 0)
    	private->channels = [self
				_fetchItemList:[[self _delegate] 
					resourcesForKey:DMA_CHANNELS_KEY]
				    returnedNum:&private->numChannels];
    return private->numChannels;
}

- (IORange *) portRangeList
{
    struct _private	*private = _eisa_private;

    if (private->numPortRanges == 0)
    	private->portRanges = [self
				_fetchRangeList:[[self _delegate] 
					resourcesForKey:IO_PORTS_KEY]
				    returnedNum:&private->numPortRanges];
    return private->portRanges;
}

- (unsigned int) numPortRanges
{
    struct _private	*private = _eisa_private;

    if (private->numPortRanges == 0)
    	private->portRanges = [self
				_fetchRangeList:[[self _delegate] 
					resourcesForKey:IO_PORTS_KEY]
				    returnedNum:&private->numPortRanges];
    return private->numPortRanges;
}


- (IOReturn) setChannelList:(unsigned int *)aList num:(unsigned int) length
{
    IOReturn ret = IO_R_RESOURCE;
    
    if ([[self _delegate]
	allocateItems:aList numItems:length
	forKey:DMA_CHANNELS_KEY] != nil) {
	
	struct _private *private = (struct _private *)_eisa_private;
	if (private->numChannels > 0)
	    IOFree(private->channels,
		private->numChannels * sizeof (*private->channels));
	private->numChannels = 0;
	ret = IO_R_SUCCESS;
    }
    return ret;
}

- (IOReturn) setPortRangeList:(IORange *)aList num:(unsigned int) length
{
    IOReturn ret = IO_R_RESOURCE;
    Range *ranges;
    int i;
    
    ranges = (Range *)IOMalloc(sizeof(Range) * length);
    for (i=0; i<length; i++) {
	ranges[i].base = aList[i].start;
	ranges[i].length = aList[i].size;
    }
    if ([[self _delegate]
	allocateRanges:ranges numRanges:length forKey:IO_PORTS_KEY] != nil) {
	
	struct _private *private = (struct _private *)_eisa_private;
	if (private->numPortRanges > 0)
	    IOFree(private->portRanges,
		private->numPortRanges * sizeof (*private->portRanges));
	private->numPortRanges = 0;
	ret = IO_R_SUCCESS;
    }
    IOFree(ranges, sizeof(Range) * length);
    return ret;
}

- (IOReturn) getEISASlotNumber : (unsigned int *) slotNum
{
    struct _private	*private = _eisa_private;

    if (private->valid) {
	if (slotNum) *slotNum = private->slotNum;
	return IO_R_SUCCESS;
    }
    return IO_R_NO_DEVICE;
}


- (IOReturn) getEISASlotID : (unsigned long *) slotID
{
    struct _private	*private = _eisa_private;

    if (private->valid) {
	if (slotID) *slotID = private->slotID;
	return IO_R_SUCCESS;
    }
    return IO_R_NO_DEVICE;
}

@end

@implementation IOEISADeviceDescription(Private)

- _initWithDelegate:delegate
{
    struct _private	*private;
    id			 theEISABus = [KernBus lookupBusInstanceWithName:"EISA" busId:0];

    [super _initWithDelegate:delegate];
    _eisa_private = (void *)IOMalloc(sizeof (struct _private));
    bzero(_eisa_private, sizeof (struct _private));
    private = _eisa_private;

    private->valid = ( (theEISABus != nil) &&
	([theEISABus getEISASlotNumber: &(private->slotNum)
				slotID: &(private->slotID)
		usingDeviceDescription: delegate] == IO_R_SUCCESS) );

    return self;
}

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    strcpy( types, IOTypePCComp);
    return( self);
}

@end
