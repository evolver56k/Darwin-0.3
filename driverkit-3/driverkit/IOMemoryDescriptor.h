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
 * IOMemoryDescriptor.h
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
#ifdef KERNEL
#import <driverkit/IOMemoryContainer.h>

typedef struct PhysicalRange {
    void *		address;		/* Physical range start    */
    unsigned int	length;			/* Length of this range    */
} PhysicalRange;

/*
 * IOMemoryDescriptorState describes the "location" of a transfer. It may
 * be saved and restored by the state and setState methods. The contents
 * must not be modified by the caller, who should treat the entire object
 * as an opaque data record.
 */
struct IOMemoryDescriptorState {
    /*
     * currentOffset is the byte index into the entire transfer -- for all
     * memory ranges. It is modified implicitly when a range is delivered
     * to a caller, and explicitly by the setPosition and setOffset methods.
     */
    unsigned int	currentOffset;		/* Place in transfer	*/
    /*
     * The following values drive the nextPhysicalRange and nextLogicalRange
     * methods. They can be saved and restored for speed. The caller must
     * not modify any values in the IOMemoryDescriptorState structure.
     *
     *	rangeIndex		The index into the container's range vector
     *	logicalOffset		The offset into the current logical range
     *	logical			A copy of the current logical range.
     *	physicalOffset		The offset into the current physical range
     *				Zero means the physical page is invalid.
     *	physical		The current physical range, if known.
     */
    unsigned int	rangeIndex;		/* Current range index	    */
    IORange		ioRange;		/* Current range	    */
    unsigned int	logicalOffset;		/* Place in this range	    */
    PhysicalRange	physical;		/* Current range	    */
    unsigned int	physicalOffset;		/* Place in this range	    */
    int			pad[16];		/* Reserved for future use  */
};
typedef struct IOMemoryDescriptorState IOMemoryDescriptorState;

@interface IOMemoryDescriptor : Object
{
@protected
    unsigned int	retainCount;		/* For retain/release	    */
    IOMemoryContainer	*ioMemoryContainer;	/* Container vector	    */
    IOMemoryDescriptorState state;		/* Transfer location	    */
    /*
     * valid is TRUE when the currentOffset matches the state values.
     * valid is set FALSE when currentOffset is moved by setPosition or
     * setOffset.
     */
    BOOL		valid;			/* Do all offsets match	    */
    unsigned int	maxSegmentCount;	/* Never zero		    */
    int			pad[16];		/* Reserved for future use  */
}
 
/**
 * Initialize an IOMemoryDescriptor object.
 * @return			Return the IOMemoryDescriptor.
 *
 * Note that maxSegmentCount will be initialized to UINT_MAX and the client task
 * will be initialized to IO_NULL_VM_TASK.
 */

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
	byReference	: (BOOL) byReference;

/**
 * Initialize an IOMemoryDescriptor object for a single logical range.
 */
- (id)			initWithAddress
			: (void *) address
	length		: (unsigned int) length;

/**
 * Initialize an IOMemoryDescriptor object for a logical scatter-gather list.
 * The scatter-gather list is provided in BSD Unix iov format.
 */
- (id)			initWithIOV
			: (const struct iovec *) iov
	count		: (unsigned int) count;

/**
 * Manage the retain/release reference count. See NSObject for details.
 */
- (unsigned int)	retainCount;
- (id)			retain;
- (oneway void)		release;

/**
 * Accessor methods
 */
- (unsigned int)	currentOffset;
- (unsigned int)	totalByteCount;
- (unsigned int)	maxSegmentCount;
- (id)			ioMemoryContainer;
- (vm_task_t)		client;

/**
 * Zero or any value >= the transfer length (such as UINT_MAX from limits.h)
 * means no interesting maxSegmentCount
 */
- (void)		setMaxSegmentCount
			: (unsigned int) newMaxSegmentCount;

- (void)		setClient
			: (vm_task_t) client;

/**
 * setIOMemoryContainer has the following effect:
 * If the new and old memory containers differ, the old is
 *	released and the new retained. The new then replaces
 *	the old container.
 * The current transfer position is always reset to the start
 * of the transfer (even if old and new are the same).
 * The maxTransferCount is not affected.
 */
- (void)		setIOMemoryContainer
			: (id) ioMemoryContainer;

/**
 * Retrieve all positioning information -- this is intended for
 * processing SCSI Save Data Pointers messages. Callers should treat
 * IOMemoryDescriptorState as an opaque object.
 */
- (void)		state
			: (IOMemoryDescriptorState *) statePtr;

/**
 * Reposition the IOMemoryDescriptor's current access point. (The TECO "dot").
 * setState should be be called with the results of a previous state
 * method. It is very fast. The caller must not modify the state.
 */
- (void)		setState
			: (const IOMemoryDescriptorState *) statePtr;
/**
 * Set the current access point to the specified byte index. This is an
 * absolute position within the IOMemoryDescriptor. This will be slow
 * for complex memory descriptors. setPosition does not check the parameter
 * for validity -- this is done by the nextPhysicalRange method.
 */
- (void)		setPosition
			: (unsigned int) newPosition;
/**
 * Set the current access point to the relative position with respect
 * to the current position. This is equivalent to writing:
 *    [ioMemoryDescriptor setPosition
 *		: [ioMemoryDescriptor currentOffset] + offset];
 * This will be slow for complex memory descriptors. setOffset does not check
 * the parameter for validity -- this is done by the nextPhysicalRange method.
 */
- (void)		setOffset
			: (signed int) offset;

/**
 * Return one or more logical ranges. Return zero if the transfer is outside
 * of the defined range (I.e., if all data has been transferred).
 * @param maxRanges		The maximum number of ranges to retrieve.
 * @param maxByteCount		The maximum number of bytes to retrieve
 *				in the entire sequence. Use UINT_MAX (from
 *				limits.h) to use the remaining transfer count,
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
	logicalRanges	: (IORange *) logicalRanges;

/*
 * Return one or more physical ranges. Return zero if the transfer is outside
 * of the defined range (I.e., if all data has been transferred).
 * @param maxRanges		The maximum number of ranges to retrieve.
 * @param maxByteCount		The maximum number of bytes to retrieve
 *				in the entire sequence. Use UINT_MAX (from
 *				limits.h) to use the remaining transfer count,
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
	physicalRanges	: (PhysicalRange *) physicalRanges;

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
- (unsigned int)	writeToClient
			: (void *) buffer
	count		: (unsigned int) count;

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
- (unsigned int)	readFromClient
			: (void *) buffer
		count	: (unsigned int) count;

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
			: (BOOL) forReading;

/**
 * Make the memory described by this underlying IOMemoryDescriptor pageable.
 * This is called by the virtual memory manager and/or file system after
 * completing an I/O request.  The IOMemoryDescriptor maintains a reference
 * count: the last caller frees the memory the others just decrement the count.
 * This method returns an error status if any range could not be freed,
 * but always tries to free all ranges. Return IO_R_VM_FAILURE if any range
 * can't be unwired (but there is no indication as to which range).
 */
- (IOReturn)			unwireMemory;

/**
 * Normalize cache coherency (if needed by this particular hardware
 * architecture) before starting a DMA operation. This normalizes all
 * memory described by this IOMemoryContainer.  This is used as follows:
 *	mem = [IOMemoryDescriptor alloc
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
			: (IOMemoryCheckpointOption) option;

/* 
 * * * *
 * * * * The following should only be called by IOMemoryDescriptor
 * * * * classes
 * * * *
 */
/**
 * mapPhysicalAddressIntoIOTask is used to map a potentially
 * unaligned physical address and range into our virtual address
 * space. It is a direct wrapper for IOMapPhysicalIntoIOTask
 * that handles unaligned physical addresses.
 * @return The correct virtual address, or NULL if mapping failed.
 */ 
- (vm_address_t) 	mapPhysicalAddressIntoIOTask
			: (const PhysicalRange *) range;
/**
 * Un-map a range that was mapped by mapPhysicalAddressIntoIOTask.
 * rangeLength must be identical to the range.length parameter passed
 * to mapPhysicalAddressIntoIOTask.
 */
- (void)		unmapVirtualAddressFromIOTask
			: (vm_address_t) clientVirtualAddress
	length		: (unsigned int) rangeLength;

@end /* IOMemoryDescriptor : Object */
#endif /* KERNEL */
