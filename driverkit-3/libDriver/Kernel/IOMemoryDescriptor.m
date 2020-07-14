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
 * IOMemoryDescriptor.m
 * Copyright 1997-98 Apple Computer Inc. All Rights Reserved.
 *
 * IOMemoryDescriptor describes the client memory needed to perform an I/O
 * request. It contains a single IOMemoryContainer that describes one or
 * more ranges of memory.  IOMemoryDescriptor is used directly for simple
 * (non-RAID) transfers, and is the underlying object used for striped or
 * mirrored requests. Logical and Physical memory cannot be mixed in a single
 * IOMemoryDescriptor. Several IOMemoryDescriptors may reference a single
 * IOMemoryContainer (by using the replicate method).
 */
#import <driverkit/IOMemoryDescriptor.h>
#import <mach/vm_param.h>
#import <limits.h>

/*
 * Compute the start of the next physical page
 */
#define next_page(x)	(trunc_page(((unsigned int) x) + page_size))

@interface IOMemoryDescriptor(Private)
/**
 * Initialize an IOMemoryDescriptor object.
 */
- (id)			initWithIOMemoryContainer
				: (IOMemoryContainer *) ioMemoryContainer;
/**
 * Free the descriptor and it's container.
 */
- (id)			free;
/*
 * validate is called when the state needs to be validated.
 */
- (void)		validate;
/*
 * Retrieve the current logical range (returns a zero-length range on failure).
 */
- (void)		getCurrentLogicalRange;

/*
 * Retrieve the current physical range. This can fail if the O.S. returns
 * an error status.
 */
- (IOReturn)		getCurrentPhysicalRange;

/*
 * Force the state to the end of the entire container (for reposition
 * errors).
 */
- (void)		setPositionAtEnd;
@end /* IOMemoryContainer(Private) */

@implementation IOMemoryDescriptor(Private)
/**
 * Create an empty IOMemoryDescriptor object.
 */
- (id)			initWithIOMemoryContainer
				: (IOMemoryContainer *) thisIOMemoryContainer
{
	ioMemoryContainer	= thisIOMemoryContainer;
	state.currentOffset	= 0;
	retainCount		= 1;
	valid			= FALSE;	/* Resets other values	    */
	[self setMaxSegmentCount : UINT_MAX];
	return (self);
}

/**
 * Dispose of the object. This decrements the IOMemoryContainer's
 * reference count and frees the IOMemoryContainer if this IOMemoryDescriptor
 * is the last referencer.
 */
- 				free
{
	[ioMemoryContainer release];
	return ([super free]);
}

/**
 * Extract the next logical range from the IOMemoryContainer.
 * If this fails because rangeIndex exceeds the container max,
 * a zero-length range will be returned.
 */
- (void)		getCurrentLogicalRange
{
    if ([ioMemoryContainer logicalRange: &state.ioRange
                                  index: state.rangeIndex] != IO_R_SUCCESS) {
	    state.ioRange.start		= 0xDEADBEEF;
	    state.ioRange.size		= 0xFFFFFFFF;
	}
	state.logicalOffset		= 0;
	state.physicalOffset		= 0;
}

/**
 * Return the physical address that corresponds to the current logical
 * address. The state is valid at this point. This method sets the
 * physicalPageLength to the maximumn number of bytes that can be
 * transferred in this page.
 */
- (IOReturn)		getCurrentPhysicalRange
{
   	IOReturn		ioReturn;
   	unsigned int		nextPageAddress;
   	
 	state.physicalOffset = 0;	/* At page start	*/
   	ioReturn	= IOPhysicalFromVirtual(
   				[ioMemoryContainer client],
   				state.ioRange.start + state.logicalOffset,
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
	unsigned int	totalByteCount = [ioMemoryContainer totalByteCount];
	unsigned int	totalRangeCount = [ioMemoryContainer rangeCount];

	if (state.currentOffset >= totalByteCount) {
	    [self setPositionAtEnd];
	}
	else {
	    unsigned int	offsetAtRangeStart = 0;
	    unsigned int	offsetAtRangeEnd;
	    for (state.rangeIndex = 0;
			state.rangeIndex < totalRangeCount;
			state.rangeIndex++) {

                /* Check for an invalid range or other problems */
		[self getCurrentLogicalRange];
		if (0xDEADBEEF == state.ioRange.start
		&&  0xFFFFFFFF == state.ioRange.size) {
		    IOPanic("IOMemoryDescriptor: "
                            "validate fell of the end of the world\n");
		}
		offsetAtRangeEnd = offsetAtRangeStart + state.ioRange.size;
		if (offsetAtRangeEnd > state.currentOffset) {
		    state.logicalOffset =
			state.currentOffset - offsetAtRangeStart;
		    break;			/* Normal exit		*/
		}
		offsetAtRangeStart = offsetAtRangeEnd;
	    }
	    if (state.rangeIndex >= totalRangeCount) {
		[self setPositionAtEnd];	/* Bug: can't happen	*/
	    }
	}
	state.physicalOffset = 0;		/* Invalidate physical	*/
	valid = TRUE;
}

/*
 * Force the state to the end of the entire container (for reposition
 * errors).
 */
- (void)		setPositionAtEnd
{
	state.rangeIndex = [ioMemoryContainer rangeCount] - 1;
    	[self getCurrentLogicalRange];
    	state.logicalOffset = state.ioRange.size;
}

@end /* IOMemoryDescriptor(Private) */

@implementation IOMemoryDescriptor

/**
 * Initialize an IOMemoryDescriptor object for a logical scatter-gather list.
 * The scatter-gather list is provided in DriverKit IORange format. If byReference
 * is TRUE, the associated IOMemoryContainer will hold a reference to the range
 * vector (which must remain addressable during the lifetime of the object). If
 * FALSE, the range vector will be copied into the IOMemoryContainer object.
 */
- (id)			initWithIORange
			: (const IORange *) ioRange
	count		: (unsigned int) count
	byReference	: (BOOL) byReference
{
	IOMemoryContainer	*thisIOMemoryContainer =
		[[IOMemoryContainer alloc] initWithIORange
					: ioRange
			count		: count
			byReference	: byReference
		    ];
	[self initWithIOMemoryContainer	: thisIOMemoryContainer];
	return (self);

}

/**
 * Initialize an IOMemoryDescriptor object for a single logical range.
 */
- (id)			initWithAddress
			: (void *) address
	length		: (unsigned int) length
{
	IOMemoryContainer	*thisIOMemoryContainer =
		[[IOMemoryContainer alloc] initWithAddress
					: address
			length		: length
		    ];
	[self initWithIOMemoryContainer	: thisIOMemoryContainer];
	return (self);
}


- (id)			initWithIOV
			: (const struct iovec *) iov
	count		: (unsigned int) count
{
	IOMemoryContainer	*thisIOMemoryContainer =
		[[IOMemoryContainer alloc] initWithIOV
					: iov
			count		: count
		    ];
	[self initWithIOMemoryContainer	: thisIOMemoryContainer];
	return (self);
}

/**
 * Manage the retain/release reference count. See NSObject for details.
 */
- (unsigned int)	retainCount
{
	return (retainCount);
}

- (id)			retain
{
	++retainCount;
	return (self);
}

- (oneway void)		release
{
	if (--retainCount == 0) {
	    [self free];
	}
}


/**
 * Return a copy of this IOMemoryDescriptor and its IOMemoryContainer.
 * The IOMemoryContainer's reference count will be incremented.
 * The current position is not duplicated: the clone will be reset
 * to position zero.
 */
- (id)			replicate
{
	IOMemoryDescriptor	*result;

	[ioMemoryContainer retain];
	result = [[self copy] initWithIOMemoryContainer
					: ioMemoryContainer
		];
	if (result) {
	    [result setMaxSegmentCount	: maxSegmentCount];
	    [result setClient		: [self client]];
	}
	return (result);
}

/**
 * Accessor methods
 */
- (unsigned int)	currentOffset
{
	return (state.currentOffset);
}

- (unsigned int)	totalByteCount
{
	return ([ioMemoryContainer totalByteCount]);
}

- (unsigned int)	maxSegmentCount
{
	return (maxSegmentCount);
}
/*
 * Retrieve the ioMemoryContainer
 */
- (id)			ioMemoryContainer
{
	return (ioMemoryContainer);
}



- (void)		setMaxSegmentCount
			: (unsigned int) newMaxSegmentCount
{
        maxSegmentCount	= (newMaxSegmentCount == 0)
			? [self totalByteCount]
			: newMaxSegmentCount;
}

- (vm_task_t)		client
{
	return ([ioMemoryContainer client]);
}

- (void)		setClient
			: (vm_task_t) client
{
	[ioMemoryContainer setClient : client];
}

- (void)		setIOMemoryContainer
			: (id) newIOMemoryContainer
{
	if (ioMemoryContainer != newIOMemoryContainer) {
	    [ioMemoryContainer release];
	    [newIOMemoryContainer retain]; 
	    ioMemoryContainer	= newIOMemoryContainer;
	}
	state.currentOffset	= 0;
	valid			= FALSE;	/* Resets other values	    */
}

/**
 * This retrieves all positioning information -- it is intended for
 * processing SCSI Save Data Pointers messages. Callers should treat
 * IOMemoryDescriptorState as an opaque object.
 */
- (void)		state
			: (IOMemoryDescriptorState *) statePtr
{
	/*
	 * If we're called without a valid state, setState will
	 * fail (as the new state will be invalid). Note that the
	 * state is invalid when the IOMemoryDescriptor is first
	 * created.
	 */
	if (valid == FALSE) {
	    [self validate];
	}
	*statePtr = state;
}


/**
 * Reposition the IOMemoryDescriptor's current access point. (The TECO "dot").
 * setState should be be called with the results of a previous state
 * method. It is very fast. The caller must not modify the state.
 */
- (void)		setState
			: (const IOMemoryDescriptorState *) statePtr
{
	state		= *statePtr;
	valid		= TRUE;
}

/**
 * Set the current access point to the specified byte index. This is an
 * absolute position within the IOMemoryDescriptor. This will be slow
 * for complex memory descriptors. setPosition does not check the parameter
 * for validity -- this is done by the nextPhysicalRange method.
 */
- (void)		setPosition
			: (unsigned int) newPosition
{
	if (state.currentOffset != newPosition) {
		state.currentOffset	= newPosition;
		valid			= FALSE;
	}
}

/**
 * Set the current access point to the relative position with respect
 * to the current position. This is equivalent to writing:
 *    [ioMemoryDescriptor setPosition
 *		: [ioMemoryDescriptor currentOffset] + offset];
 * This will be slow for complex memory descriptors. setOffset does not check
 * the parameter for validity -- this is done by the nextPhysicalRange method.
 */
- (void)		setOffset
			: (signed int) offset
{
	state.currentOffset	+= offset;
	valid			= FALSE;
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
	unsigned int	byteCount;
	unsigned int	transferCount	= 0;
	unsigned int	rangeCount;
	unsigned int	totalByteCount	= [ioMemoryContainer totalByteCount];

	if (valid == FALSE) {
	    [self validate];
	}
	for (rangeCount = 0; rangeCount < maxRanges; rangeCount++) {
	    byteCount = totalByteCount - state.currentOffset;
	    if (state.logicalOffset >= state.ioRange.size) {
		++state.rangeIndex;
		[self getCurrentLogicalRange];
	    }
	    if (byteCount > (state.ioRange.size - state.logicalOffset)) {
		byteCount = state.ioRange.size - state.logicalOffset;
	    }
	    if (byteCount > maxSegmentCount) {
		byteCount = maxSegmentCount;
	    }
	    if (byteCount > maxByteCount) {
	    	byteCount = maxByteCount;
	    }
	    if (byteCount == 0) {
		break;		/* Fell off the end	*/
	    }
	    logicalRanges->size		= byteCount;
	    logicalRanges->start	= state.ioRange.start
					+ state.logicalOffset;
	    ++logicalRanges;
	    state.logicalOffset		+= byteCount;
	    transferCount		+= byteCount;
	    state.currentOffset += transferCount;
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
 *				transfer count, use UINT_MAX (from limits.h).
 * @param newPosition		The new value of currentOffset (may be NULL)
 * @param actualRanges		The actual number of ranges retrieved
 *				(NULL if not needed)
 * @param physicalRanges	A vector of physical range elements.
 * Return the total number of bytes in all ranges. Return zero if the current
 * offset is beyond the end of the range.
 */
- (unsigned int)			getPhysicalRanges
			: (unsigned int) maxRanges
	maxByteCount	: (unsigned int) maxByteCount
	newPosition	: (unsigned int *) newPosition
	actualRanges	: (unsigned int *) actualRanges
	physicalRanges	: (PhysicalRange *) physicalRanges
{
	unsigned int	byteCount;
	unsigned int	transferCount	= 0;
	unsigned int	rangeCount;
	unsigned int	totalByteCount	= [ioMemoryContainer totalByteCount];

	if (valid == FALSE) {
	    [self validate];
	}
	for (rangeCount = 0; rangeCount < maxRanges; rangeCount++) {
	    byteCount = totalByteCount - state.currentOffset;
	    if (state.logicalOffset >= state.ioRange.size) {
		++state.rangeIndex;
		[self getCurrentLogicalRange];
	    }
	    if (byteCount > (state.ioRange.size - state.logicalOffset)) {
		byteCount = state.ioRange.size - state.logicalOffset;
	    }
	    if ((byteCount + transferCount) > maxSegmentCount) {
		byteCount = maxSegmentCount - transferCount;
	    }
	    if ((byteCount + transferCount) > maxByteCount) {
	    	byteCount = maxByteCount - transferCount;
	    }
	    if (byteCount == 0) {
		break;		/* Fell off the end	*/
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
	    ++physicalRanges;
	    transferCount		+= byteCount;
	    state.physicalOffset	+= byteCount;
	    state.logicalOffset		+= byteCount;
	    state.currentOffset		+= byteCount;
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
 * Copy bytes from the caller's address space to the IOMemoryContainer
 * client address space. This is an inefficient routine that should only
 * be used for "corner cases" such as handling unaligned transfers.
 * The copy begins at the current logical address. The current logical
 * position will be incremented.
 * @param buffer	The starting buffer address in the caller's
 *			address space.
 * @param count		The number of bytes to transfer.
 * @return		The actual number of bytes transferred.
 */
- (unsigned int)		writeToClient
			: (void *) buffer
		count	: (unsigned int) count
{
	unsigned int	totalByteCount;
	unsigned int	thisCount;
	PhysicalRange	range;
	vm_address_t	clientVirtualAddress;
	
	for (totalByteCount = 0; totalByteCount < count;) {
	    thisCount = [self getPhysicalRanges
					: 1	/* Only one range	*/
		    maxByteCount	: count - totalByteCount
		    newPosition		: NULL
		    actualRanges	: NULL
		    physicalRanges	: &range
		];
	    if (thisCount == 0) {
		break;	/* Ran off the end of the user buffer	*/
	    }
	    clientVirtualAddress = [self mapPhysicalAddressIntoIOTask : &range];
	    if (clientVirtualAddress == (vm_address_t) NULL) {
		[self setOffset : -range.length];
		break;
	    }
	    else {
		bcopy(buffer, clientVirtualAddress, range.length);
		((unsigned char *) buffer) += range.length;
		totalByteCount += range.length;
		[self unmapVirtualAddressFromIOTask
					: clientVirtualAddress
			length		: range.length
		    ];
	    }
	}
	return (totalByteCount);
}

/**
 * Copy bytes from the IOMemoryContainer client's address space to the
 * caller's address space. This is an inefficient routine that should only
 * be used for "corner cases" such as handling unaligned transfers.
 * The copy begins at the current logical address. The current logical
 * position will be incremented.
 * @param buffer	The starting buffer address in the caller's
 *			address space.
 * @param count		The number of bytes to transfer.
 * @return		The actual number of bytes transferred.
 */
- (unsigned int)		readFromClient
			: (void *) buffer
		count	: (unsigned int) count
{
	unsigned int	totalByteCount;
	unsigned int	thisCount;
	PhysicalRange	range;
	vm_address_t	clientVirtualAddress;
	
	for (totalByteCount = 0; totalByteCount < count;) {
	    thisCount = [self getPhysicalRanges
					: 1	/* Only one range	*/
		    maxByteCount	: count - totalByteCount
		    newPosition		: NULL
		    actualRanges	: NULL
		    physicalRanges	: &range
		];
	    if (thisCount == 0) {
		break;	/* Ran off the end of the user buffer	*/
	    }
	    clientVirtualAddress = [self mapPhysicalAddressIntoIOTask : &range];
	    if (clientVirtualAddress == (vm_address_t) NULL) {
		[self setOffset : -range.length];
		break;
	    }
	    else {
		bcopy(clientVirtualAddress, buffer, range.length);
		((unsigned char *) buffer) += range.length;
		totalByteCount += range.length;
		[self unmapVirtualAddressFromIOTask
					: clientVirtualAddress
			length		: range.length
		    ];
	    }
	}
	return (totalByteCount);
}

/**
 * mapPhysicalAddressIntoIOTask is used to map a potentially
 * unaligned physical address and range into our virtual address
 * space. It is a direct wrapper for IOMapPhysicalIntoIOTask
 * that handles unaligned physical addresses.
 */ 
- (vm_address_t) mapPhysicalAddressIntoIOTask
		: (const PhysicalRange *) range
{
	vm_address_t	clientVirtualAddress;
	unsigned int		offset;
	IOReturn	ioReturn;

	offset = ((unsigned int) range->address) & page_mask;
	ioReturn = IOMapPhysicalIntoIOTask(
		range->address - offset,
		range->length + offset,
		&clientVirtualAddress
	    );
	if (ioReturn != IO_R_SUCCESS) {
	    clientVirtualAddress = (vm_address_t) NULL;
	}
	else {
	    /*
	     * Ensure that clientVirtualAddress is on a page
	     * boundary and add in the actual offset.
	     */
	    clientVirtualAddress = (vm_address_t)
			(((unsigned int) clientVirtualAddress) & ~page_mask)
			+ offset;
	}
	return (clientVirtualAddress);
}
		
/**
 * Un-map a range that was mapped by mapPhysicalAddressIntoIOTask.
 * rangeLength must be identical to the range.length parameter passed
 * to mapPhysicalAddressIntoIOTask.
 */
- (void) unmapVirtualAddressFromIOTask
		: (vm_address_t) clientVirtualAddress
	length	: (unsigned int) rangeLength
{
	unsigned int		offset;
	vm_address_t	pageBaseAddress;

	offset = ((unsigned int) clientVirtualAddress) & page_mask;
	pageBaseAddress = (vm_address_t)
		((unsigned int) clientVirtualAddress) & ~page_mask;
	(void) IOUnmapPhysicalFromIOTask(
		pageBaseAddress, rangeLength + offset);
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
- (IOReturn)			wireMemory
			: (BOOL) forReading
{
	return ([ioMemoryContainer wireMemory : forReading]);
}

/**
 * Make the memory described by this underlying IOMemoryDescriptor pageable.
 * This is called by the virtual memory manager and/or file system after
 * completing an I/O request.  The IOMemoryDescriptor maintains a reference
 * count: the last caller frees the memory the others just decrement the count.
 * This method returns an error status if any range could not be freed,
 * but always tries to free all ranges. Return IO_R_VM_FAILURE if any range
 * can't be unwired (but there is no indication as to which range).
 */
- (IOReturn)			unwireMemory
{
	return ([ioMemoryContainer unwireMemory]);
}

/**
* Normalize cache coherency (if needed by this particular hardware
* architecture) before starting a DMA operation. This normalizes all
* memory described by this IOMemoryContainer.  This is used as follows:
*	mem = [IOMemoryDescriptor allocLogicalRange
*				: address
*			length	: length];
*	[mem makeResident];
*	[mem checkpoint : ioCheckpointInput];
*	... Extract physical ranges and do DMA I/O ...
*	[mem checkpoint : ioCheckpointComplete];
*	[mem makePageable];
*	[mem free];
* A checkpoint call may specify any combination of ioCheckpointInput,
* ioCheckpointOutput, or ioCheckpointNoDirection. After DMA completes,
* drivers must call checkpoint with ioCheckpointComplete as the only
* parameter. The actual operation of checkpoint is processor-specific.
 */
- (IOReturn)			checkpoint
			: (IOMemoryCheckpointOption) option
{
/*
 * *** Hmm: if we have multiple users, the checkpoint probably should be
 * *** inside the descriptor, not the container
 */
	return ([ioMemoryContainer checkpoint : option]);
}

@end /* IOMemoryDescriptor : IOObject */
