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
 * IOMemoryContainer.m
 * Copyright 1997-98 Apple Computer Inc. All Rights Reserved.
 *
 * IOMemoryContainer describes client memory. It is constructed and manipulated
 * by MemoryDescriptor classes. It is not "visible" to client and server
 * code. Note that IOMemoryContainer objects do not provide enumeration or
 * positioning services: they only describe a particular range of memory.
 *
 * IOMemoryContainer may be used in both user space and kernel space.
 * The memory described by a user-space IOMemoryContainer can be
 * accessed by the creating task as a collection of logical extents.
 * Kernel processes, after making the memory resident, can access
 * the container memory as either logical or physical extents.
 */
#import <driverkit/IOMemoryContainer.h>
#import <driverkit/generalFuncs.h>
#import <mach/vm_param.h>

/*
 * This is the information that is transferred by the serialization
 * methods. This fixed-length header will be followed by <rangeCount>
 * IORange values.
 */
typedef struct {
    vm_task_t		client;
    unsigned int	rangeCount;
} IOMemoryContainerSerialization;
#define kSerializationSize	(sizeof (IOMemoryContainerSerialization))

@interface IOMemoryContainer(Private)
- (id)			init;
- (id)			free;
@end /* IOMemoryContainer(Private) */

@implementation IOMemoryContainer(Private)
/**
 * Initialize an empty IOMemoryContainer object.
 */
- (id)			init
{
	/*
	 * Initialize all fields to zero (but with one reference).
	 */
	options			= 0;
	retainCount		= 1;
	residencyCount		= 0;
#ifdef KERNEL
	client			= IOVmTaskSelf();
#else
	client			= IO_NULL_VM_TASK;
#endif /* KERNEL */
	rangeCount		= 0;
	totalByteCount		= 0;
	range.logical.start	= 0;
	range.logical.size	= 0;
	return (self);
}

/**
 * Dispose of the object. Note that free ignores the retainCount and
 * referenceCount and always frees the IOMemoryContainer object.
 */
- (id)			free
{
	if (rangeCount > 1 && range.vector != NULL) {
	    if ((options & ioRangeByReference) == 0) {
		IOFree((void *) range.vector, rangeCount * sizeof (IORange));
	    }
	    rangeCount = 0;
	    range.vector = NULL;
	}
	return [super free];
}

@end /* IOMemoryContainer(Private) */


@implementation IOMemoryContainer

/**
 * Create an IOMemoryContainer object for a logical scatter-gather list in the
 * caller's address map. The scatter-gather list is provided in DriverKit
 * IORange format. If byReference is TRUE, the IOMemoryContainer will hold
 * a reference to the range vector (which must remain addressable during the
 * lifetime of the object). If FALSE, the range vector will be copied into
 * the IOMemoryContainer object.
 */
- (id)			initWithIORange
			: (const IORange *) ioRange
	count		: (unsigned int) thisRangeCount
	byReference	: (BOOL) initByReference
{
	unsigned int	size;
	unsigned int	i;
	
	switch (thisRangeCount) {
	case 0:
	    [self initWithAddress
	    			: NULL
	    	    length	: 0
	    	];
	    break;
	case 1:
	    [self initWithAddress
	    			: (void *) ioRange->start
	    	    length	: ioRange->size
	    	];
	    break;
	default:
	    [self init];
	    rangeCount		= thisRangeCount;
	    size		= thisRangeCount * sizeof (IORange);
	    if (initByReference) {
		options		|= ioRangeByReference;
		range.vector	= ioRange;
	    }
	    else {
		range.vector	= IOMalloc(size);
		IOCopyMemory(
		    (void *) ioRange, 		/* Copy from here	*/
		    (void *) range.vector,	/* Copy to here		*/
		    size,			/* Total byte count	*/
		    4				/* Bytes per transfer	*/
		);
	    }
	    for (i = 0; i < thisRangeCount; i++) {
		totalByteCount += ioRange[i].size;
	    }
	    break;
	}
	return (self);
}

/**
 * Create an IOMemoryContainer object for a single logical range.
 */
- (id)			initWithAddress
			: (void *) address
	length		: (unsigned int) length
{
	[self init];
	rangeCount		= 1;
	range.logical.start	= (unsigned int) address;
	range.logical.size	= length;
	totalByteCount		= length;
	return (self);
}


/**
 * Create an IOMemoryContainer object for a logical scatter-gather list in the
 * caller's address map. The scatter-gather list is provided in BSD
 * Unix iov format.
 */
- (id)			initWithIOV
			: (const struct iovec *) iov
	count		: (unsigned int) iovCount
{
	unsigned int	size;
	unsigned int	i;

	switch (iovCount) {
	case 0:
	    [self initWithAddress : NULL
	    	    length	: 0
	    	];
	    break;
	case 1:
	    [self initWithAddress : iov->iov_base
	    	    length	: iov->iov_len
	    	];
	    break;
	default:
	    [self init];
	    range.vector	= IOMalloc(size);
	    rangeCount		= iovCount;
	    size		= iovCount * sizeof (IORange);
	    for (i = 0; i < iovCount; i++, iov++) {
		((IORange *) range.vector)[i].start =
				(unsigned int) iov->iov_base;
		((IORange *) range.vector)[i].size = iov->iov_len;
		totalByteCount	+= iov->iov_len;
	    }
	    break;
	}
	return (self);
}

/**
 * Accessor methods
 */
- (unsigned int)	rangeCount
{
	return (rangeCount);
}

- (unsigned int)	totalByteCount
{
	return (totalByteCount);
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
 * Return one range segment. These return errors if the parameter is
 * incorrect (calling logicalRange on physicalRanges, index out of bounds).
 */

/**
 * Return the logical address and length for the i'th logical range.
 * Return IO_R_INVALID_ARG if the index is outside the allocated range.
 */
- (IOReturn)		logicalRange
			: (IORange *) logicalRange
	index		: (unsigned int) thisIndex
{
	IOReturn	ioReturn;
	
	if (thisIndex >= rangeCount || logicalRange == NULL) {
	    ioReturn		= IO_R_INVALID_ARG;
	}
	else {
	    ioReturn		= IO_R_SUCCESS;
	    if (rangeCount == 1) {
	        *logicalRange = range.logical;
	    }
	    else {
		*logicalRange = range.vector[thisIndex];
	    }
	}
	return (ioReturn);
}
@end /* IOMemoryContainer : Object */

#ifdef KERNEL
@implementation IOMemoryContainer(Kernel)

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
 * Make the memory described by this IOMemoryContainer resident.
 * This is called by the virtual memory manager and/or file system before
 * starting an I/O request. Residency is an all-or-nothing process. The
 * IOMemoryContainer maintains a reference count: the first caller makes the
 * memory resident; the others just increment the count. This method returns
 * an error status if any range cannot be made resident and all memory will
 * be made pageable. This method may only be called by kernel servers.
 */
- (IOReturn)		wireMemory
			: (BOOL) forReading
{
	IOReturn	ioReturn = IO_R_SUCCESS;
	kern_return_t	status;
	unsigned int	i;
	unsigned int	successCount;
	IORange		thisRange;
	vm_offset_t	rangeStart;
	vm_offset_t	rangeEnd;
	
	/*
	 * Increment the residency counter. If it was not zero when we were
	 * called, we have already made this memory resident, so just exit.
	 * If it was zero, this is the first call and we must make all ranges
	 * resident.
	 */
	if (forReading)
	    options |=  ioWiredForRead;
	else
	    options &= ~ioWiredForRead;

	if (residencyCount++ == 0) {	/* NOTE: AtomicIncrement */
	    successCount = 0;
	    for (i = 0; i < rangeCount && ioReturn == IO_R_SUCCESS; i++) {
		ioReturn = [self logicalRange	: &thisRange
				index		: i
			    ];
		if (ioReturn == IO_R_SUCCESS) {
		    rangeStart	= trunc_page(thisRange.start);
		    rangeEnd	= round_page(thisRange.start + thisRange.size);
		    status	= vm_map_pageable(
					client,
					rangeStart,
					rangeEnd,
					FALSE		/* Wire range	*/
				    );
		    if (status != KERN_SUCCESS)
			ioReturn = IO_R_CANT_WIRE;	/* Wire failed	*/
		    else {
		    	if (forReading == FALSE) {
		    	    /*
		    	     * Is this really needed?
		    	     */
			    // flush_cache_v(rangeStart, rangeEnd - rangeStart);
			}
			++successCount;
		    }
		}
	    }
	}
	if (ioReturn != IO_R_SUCCESS) {
	    /*
	     * Unwire partial preparations.
	     */
	    for (i = 0; i < successCount; i++) {
		[self logicalRange : &thisRange index : i];
		rangeStart	= trunc_page(thisRange.start);
		rangeEnd	= round_page(thisRange.start + thisRange.size);
		(void) vm_map_pageable(
			client,
			rangeStart,
			rangeEnd,
			TRUE		/* Unwire range	*/
		    );
	    }
	    --residencyCount;
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
- (IOReturn)		unwireMemory
{
	IOReturn	ioReturn = IO_R_SUCCESS;
	IOReturn	finalResult = IO_R_SUCCESS;
	kern_return_t	status;
	unsigned int	i;
	IORange		thisRange;
	vm_offset_t	rangeStart;
	vm_offset_t	rangeEnd;
	
	/*
	 * Decrement the residency counter. If is zero after decrement, this
	 * is the last caller, so make the ranges pageable.
	 */
	if (--residencyCount == 0) {	/* NOTE: AtomicDecrement */
	    for (i = 0; i < rangeCount; i++) {
		ioReturn = [self logicalRange	: &thisRange
				index		: i
			    ];
		if (finalResult != IO_R_SUCCESS) {
		    finalResult = ioReturn;
		}
		else {
		    rangeStart	= trunc_page(thisRange.start);
		    rangeEnd	= round_page(thisRange.start + thisRange.size);
		    status = vm_map_pageable(
				client,
				rangeStart,
				rangeEnd,
				TRUE		/* Make range pageable	*/
			    );
		    if (status != KERN_SUCCESS && finalResult == IO_R_SUCCESS) {
			finalResult = IO_R_VM_FAILURE;
		    }
		}
	    }
	}
	return (finalResult);
}

/**
 * Normalize cache coherency (if needed by this particular hardware
 * architecture) before starting a DMA operation. This normalizes all
 * memory described by this IOMemoryContainer.  This is used as follows:
 *	mem = [[IOMemoryDescriptor alloc] initWithAddress
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
 * Note that wireMemory and unwireMemory will call checkpoint with appropriate
 * options: it need only be called explicitly if the same buffer is used
 * for multiple I/O requests, as might be the case for a video-to-disk
 * process.
 */
- (IOReturn)		checkpoint
			: (IOMemoryCheckpointOption) option
{
	/* Override if necessary */
	return (IO_R_SUCCESS);
}
			
@end /* IOMemoryContainer(Kernel) */
#endif /* KERNEL */

