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
/**
 * IOSimpleMemoryDescriptor.m
 * Copyright 1997-98 Apple Computer Inc. All Rights Reserved.
 *
 * IOSimpleMemoryDescriptor provides a limited subset of IOMemoryDescriptor
 * functionality for the benefit of low-level drivers (such as the SCSI bus
 * interface driver). The IOSimpleMemoryDescriptor supports a single
 * input logical range. It cannot be replicated, and does not have a separate
 * IOMemoryContainer. It supports the following IOMemoryDescriptor methods:
 *	allocSingleRange
 *	free
 *	currentOffset
 *	totalByteCount
 *	state
 *	setState
 *	setPosition
 *	setOffset
 *	getLogicalRanges
 *	getPhysicalRanges
 *	checkpoint
 * Other methods must not be used. They fail with an IOPanic.
 */
#ifdef KERNEL
#import <driverkit/IOSimpleMemoryDescriptor.h>
#import <mach/vm_param.h>

/*
 * Compute the start of the next physical page
 */
#define next_page(x)	(trunc_page(((unsigned int) x) + page_size))

@interface IOSimpleMemoryDescriptor(Private)
- (IOReturn)		getCurrentPhysicalRange;
- (void)		validate;
- (void)		setPositionAtEnd;
@end /* IOSimpleMemoryDescriptor(Private) */

@implementation IOSimpleMemoryDescriptor(Private)

/**
 * Return the physical address that corresponds to the current logical
 * address. The state is valid at this point. This method sets the
 * physicalPageLength to the maximumn number of bytes that can be
 * transferred in this page.
 */
- (IOReturn)		getCurrentPhysicalRange
{
   	IOReturn	ioReturn;
   	unsigned int	nextPageAddress;
   	
 	state.physicalOffset = 0;	/* At page start	*/
  	ioReturn	= IOPhysicalFromVirtual(
   				client,
   				state.ioRange.start + state.currentOffset,
   				(vm_offset_t *) &state.physical.address
   			);
   	if (ioReturn != IO_R_SUCCESS) {
       	     state.physical.address	= 0;
   	     state.physical.length	= 0;
   	}
	else {
	    /*
	     * The amount we can transfer is the number of bytes
	     * from the current start index to the next page
	     * (limited by scsiReq->maxTransfer). Mach lacks
	     * a method for determining the page size. We'll
	     * use 4096 for convenience (the only cost is extra
	     * cycles through the for loop).
	     */
	    nextPageAddress = next_page(state.physical.address);
	    state.physical.length =
			nextPageAddress - ((unsigned int) state.physical.address);
	}
	return (ioReturn);
}

/**
 * Validate the state so that all state values agree with
 * the currentOffset.
 */
- (void)		validate
{
	if (state.currentOffset >= [self totalByteCount]) {
	    [self setPositionAtEnd];
	}
	state.physicalOffset = 0;
	valid = TRUE;
}

/*
 * Force the state to the end of the entire container (for reposition
 * errors).
 */
- (void)		setPositionAtEnd
{
    	state.currentOffset = state.ioRange.size;
}

@end /* IOSimpleMemoryDescriptor(Private) */

@implementation IOSimpleMemoryDescriptor

/**
 * Destroy any existing data, replacing it with the specified data.
 */
- (id)			initWithAddress
			: (void *) address
	length		: (unsigned int) length
{
	retainCount		= 1;
	ioMemoryContainer	= NULL;
	state.currentOffset	= 0;
	state.rangeIndex	= (length == 0) ? 0 : 1;
	state.ioRange.start	= (unsigned int) address;
	state.ioRange.size	= length;
	state.logicalOffset	= 0xDEADBEEF; /* Unused	*/
	state.physicalOffset	= 0;	/* Invalidate	*/
	valid			= TRUE;
	options			= 0;
	maxSegmentCount		= length;
	client			= IOVmTaskSelf();
	return (self);
}

- (id)			initWithIORange
			: (const IORange *) ioRange
	count		: (unsigned int) count
	byReference	: (BOOL) byReference
{
	switch (count) {
	case 0:
	    [self initWithAddress: (void *) NULL length: 0];
	    break;
	case 1:
	    [self initWithAddress: (void *) ioRange->start
			   length: ioRange->size];
	    break;
	default:
	    IOPanic("IOSimpleMemoryDescriptor : "
		    "initWithIORange (only one range allowed)");
	    return [self free];
	}
	return (self);
}

- (id)			initWithIOV
			: (const struct iovec *) iov
	count		: (unsigned int) iovCount
{
	switch (iovCount) {
	    [self initWithAddress: (void *) NULL length: 0];
	    break;
	case 1:
	    [self initWithAddress: (void *) iov->iov_base
			   length: iov->iov_len];
	    break;
	default:
	    IOPanic("IOSimpleMemoryDescriptor : "
		    "initWithIOV (only one range allowed)");
	    return [self free];
	}
	return (self);
}

/**
 * Return a copy of this IOMemoryDescriptor and its IOMemoryContainer.
 * The IOMemoryContainer's reference count will be incremented.
 * The current position is not duplicated.
 */
- (id)			replicate
{
	IOPanic("IOSimpleMemoryDescriptor : replicate invalid");
	return (NULL);
}

/**
 * Accessor methods
 */
- (unsigned int)	rangeCount
{
	return (state.rangeIndex);
}

- (unsigned int)	totalByteCount
{
	return (state.ioRange.size);
}

/*
 * Kernel-specific methods. By default, non-kernel objects contain a NULL
 * vm_task_t value. Kernel tasks will set the client to the task that
 * provided this memory.
 */

- (vm_task_t)		client
{
	return (client);
}

- (void)		setClient
			: (vm_task_t) thisClient
{
	client = thisClient;
}


/**
 * Return one or more logical ranges. Return zero if the transfer is outside
 * of the defined range (I.e., if all data has been transferred).
 * @param maxRanges		The maximum number of ranges to retrieve.
 * @param maxByteCount		The maximum number of bytes to retrieve
 *				in the entire sequence. To use the remaining
 *				transfer count, specify UINT_MAX (from limits.h).
 * @param newPosition		The new value of currentOffset (may be NULL)
 * @param actualRanges		The actual number of ranges retrieved
 *				(NULL if not needed)
 * @param logicalRanges		A vector of logical range elements.
 * Return the total number of bytes in all ranges. Return zero if the current
 * offset is beyond the end of the range.
 */
- (unsigned int)	getLogicalRanges
			: (unsigned int) maxRanges
	maxByteCount	: (unsigned int) maxByteCount
	newPosition	: (unsigned int *) newPosition
	actualRanges	: (unsigned int *) actualRanges
	logicalRanges	: (IORange *) logicalRanges
{
	unsigned int	transferCount	= 0;
	unsigned int	byteCount;
	unsigned int	rangeCount;

	if (valid == FALSE) {
	    [self validate];
	}
	for (rangeCount = 0; rangeCount < maxRanges; rangeCount++) {
	    byteCount = state.ioRange.size - state.currentOffset;
	    if (byteCount > maxSegmentCount) {
		byteCount = maxSegmentCount;
	    }
	    if (byteCount + transferCount > maxByteCount) {
		byteCount = maxByteCount - transferCount;
	    }
	    if (byteCount == 0) {
		break;
	    }
	    logicalRanges->size		= byteCount;
	    logicalRanges->start	=
			(state.ioRange.start + state.currentOffset);
	    logicalRanges++;
	    transferCount		+= byteCount;
	    state.currentOffset	+= byteCount;
	}
	if (actualRanges != NULL) {
	    *actualRanges		= rangeCount;
	}
	if (newPosition != NULL) {
	    *newPosition		= state.currentOffset;
	}
	state.physicalOffset		= 0;	/* Invalid physical range    */
	return (transferCount);
}

/**
 * Return one or more physical ranges. Return zero if the transfer is outside
 * of the defined range (I.e., if all data has been transferred).
 * @param maxRanges		The maximum number of ranges to retrieve.
 * @param maxByteCount		The maximum number of bytes to retrieve
 *				in the entire sequence. To use the remaining
 *				transfer count, specify UINT_MAX (from limits.h).
 * @param newPosition		The new value of currentOffset (may be NULL)
 * @param actualRanges		The actual number of ranges retrieved
 *				(NULL if not needed)
 * @param physicalRanges	A vector of physical range elements.
 * Return the total number of bytes in all ranges. Return zero if the current
 * offset is beyond the end of the range.
 */
- (unsigned int)	getPhysicalRanges
			: (unsigned int) maxRanges
	maxByteCount	: (unsigned int) maxByteCount
	newPosition	: (unsigned int *) newPosition
	actualRanges	: (unsigned int *) actualRanges
	physicalRanges	: (PhysicalRange *) physicalRanges
{
	unsigned int	transferCount	= 0;
	unsigned int	byteCount;
	unsigned int	rangeCount;

	if (valid == FALSE) {
	    [self validate];
	}
	for (rangeCount = 0; rangeCount < maxRanges; rangeCount++) {
	    byteCount = state.ioRange.size - state.currentOffset;
	    if (byteCount > maxSegmentCount) {
		byteCount = maxSegmentCount;
	    }
	    if (byteCount + transferCount > maxByteCount) {
		byteCount = maxByteCount - transferCount;
	    }
	    if (byteCount == 0) {
		break;
	    }
	    if (state.physicalOffset == 0
	     || state.physicalOffset >= state.physical.length) {
	        if ([self getCurrentPhysicalRange] != IO_R_SUCCESS) {
		    break;
		}
	    }
	    if (byteCount > (state.physical.length - state.physicalOffset)) {
		byteCount = state.physical.length - state.physicalOffset;
	    }
	    physicalRanges->length	= byteCount;
	    physicalRanges->address	= (void *)
		(((unsigned int) state.physical.address)
			+ state.physicalOffset);
	    physicalRanges++;
	    transferCount		+= byteCount;
	    state.currentOffset	+= byteCount;
	    state.physicalOffset	+= byteCount;
	}
	if (actualRanges != NULL) {
	    *actualRanges		= rangeCount;
	}
	if (newPosition != NULL) {
	    *newPosition		= state.currentOffset;
	}
	return (transferCount);
}

/**
 * Make the memory described by this IOMemoryDescriptor resident.
 * This is called by the virtual memory manager and/or file system before
 * starting an I/O request. Residency is an all-or-nothing process. The
 * IOMemoryDescriptor maintains a reference count: the first caller makes the
 * memory resident; the others just increment the count. This method returns
 * an error status if any range cannot be made resident and all memory will
 * be made pageable. This method may only be called by kernel servers.
 */
- (IOReturn)		wireMemory
			: (BOOL) forReading;
{
	IOReturn		ioReturn = IO_R_SUCCESS;
	kern_return_t		status;
	vm_offset_t		rangeStart;
	vm_offset_t		rangeEnd;

	if (forReading) {
	    options	|= ioWiredForRead;
	}
	else {
	    options	&= ~ioWiredForRead;
	}
	if (residencyCount++ == 0) {	/* Note: AtomicIncrement	*/
	    rangeStart	= trunc_page(state.ioRange.start);
	    rangeEnd	= round_page(
				state.ioRange.start + state.ioRange.size);
	    status	= vm_map_pageable(
				/* (vm_map_t) */ client,
				rangeStart,
				rangeEnd,
				FALSE		/* Wire range	*/
			    );
	    if (status != KERN_SUCCESS) {
		ioReturn = IO_R_CANT_WIRE;
		--residencyCount;
	    }
	    else if (forReading == FALSE) {
		/*
		 * Is this really needed?
		 */
		// flush_cache_v(rangeStart, rangeEnd - rangeStart);
	    }
	}
	return (ioReturn);
}

/**
 * Make the memory described by this underlying IOMemoryContainer pageable.
 * This is called by the virtual memory manager and/or file system after
 * completing an I/O request.  The IOMemoryContainer maintains a reference
 * count: the last caller frees the memory the others just decrement the count.
 * This method returns an error status if any range could not be freed,
 * but always tries to free all ranges. Return IO_R_VM_FAILURE if any range
 * can't be unwired (but there is no indication as to which range).
 */
- (IOReturn)			unwireMemory
{
	IOReturn		ioReturn = IO_R_SUCCESS;
	kern_return_t		status;
	vm_offset_t		rangeStart;
	vm_offset_t		rangeEnd;

	if (residencyCount-- == 1) {	/* NOTE: AtomicDecrement */
	    rangeStart	= trunc_page(state.ioRange.start);
	    rangeEnd	= round_page(
				state.ioRange.start + state.ioRange.size);
	    status	= vm_map_pageable(
				/* (vm_map_t) */ client,
				rangeStart,
				rangeEnd,
				TRUE		/* Make pageable */
			    );
	    if (status != KERN_SUCCESS) {
		ioReturn = IO_R_CANT_WIRE;
	    }
	}
	return (ioReturn);
}

@end /* IOSimpleMemoryDescriptor : IOMemoryDescriptor */
#endif /* KERNEL */

