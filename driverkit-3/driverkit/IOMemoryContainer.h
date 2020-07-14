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
 * IOMemoryContainer.h
 * Copyright 1997-98 Apple Computer Inc. All Rights Reserved.
 *
 * IOMemoryContainer describes client memory. It is constructed and manipulated
 * by MemoryDescriptor classes. It is not "visible" to client and server
 * code. Note that IOMemoryContainer objects do not provide enumeration or
 * positioning services: they only describe a specific logial memory range.
 *
 * IOMemoryContainer may be used in both user space and kernel space.
 * The memory described by a user-space IOMemoryContainer can be
 * accessed by the creating task as a collection of logical extents.
 * Kernel processes, after making the memory resident, can access
 * the container memory as either logical or physical extents.
 *
 * Serialization does not retain information on residency or multiple-descriptor
 * referencing. When an IOMemoryContainer is reconstructed from a serialization,
 * only the memory ranges are preserved: residency and reference counts will
 * be set to zero. 
 */
#import <objc/Object.h>
#import <sys/types.h>
#import <mach/mach_types.h>
#import <driverkit/driverTypes.h>
#import <driverkit/return.h>
#import <sys/uio.h>


typedef enum IOMemoryCheckpointOption {
    ioCheckpointNoDirection	= 0x00000000,	/* No I/O direction given   */
    ioCheckpointInput	 	= 0x00000001,	/* I/O reads into memory    */
    ioCheckpointOutput		= 0x00000002,	/* I/O writes from memory   */
    ioCheckpointComplete	= 0x80000000,	/* No more I/O on this data */
/*
 * These bits may be set in the option instance variable.
 */
    ioRangeByReference		= 0x0001000,	/* External IORange	    */
    ioWiredForRead		= 0x0002000	/* wireMemory : TRUE	    */
} IOMemoryCheckpointOption;

@interface IOMemoryContainer : Object
{
@protected
    volatile unsigned int retainCount;		/* Number of references     */
    volatile unsigned int residencyCount;	/* Calls to makeResident    */
    vm_task_t		client;			/* Who owns this memory?    */
    unsigned int	options;		/* Configuration flags	    */
    unsigned int	rangeCount;		/* One range is special	    */
    unsigned int	totalByteCount;		/* Total number of bytes    */
    union {
    	IORange		logical;		/* One logical range	    */
    	const IORange	*vector;		/* Scatter-gather list	    */
    } range;
    int			padding[16];		/* Reserved for future use  */
}
 
/**
 * Initialize an IOMemoryContainer object. Memory is always
 * referenced to a particular creator task. Since kernel device drivers
 * operate on behalf of a user task, the memory addresses are always
 * with reference to the original user task. A kernel process receiving
 * memory references in a logical scatter-gather list (presumably through
 * a Mach message) should proceed as follows:
 * 1. The Mach message should contain a MSG_TYPE_PORT value that the
 *    caller sets to task_self(). 
 * 2. The kernel service that creates the MemoryContainer then converts
 *    the port_t value to the equivalent vm_map_t by calling
 *	#import <kern/ipc_tt.h>
 *	vm_map_t target_map = convert_port_to_map((ipc_port_t) userPort);
 *	if (target_map == VM_MAP_NULL) { return error; }
 * 3. The target_task value is passed to the IOMemoryContainer allocator
 *	cast to (vm_task_t).
 * The constructors that do not take a specific task parameter use the
 * current task's map, expressed as IOVMTaskSelf(). Note that a vm_map_t
 * is identical to a vm_task_t.
 */

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
	count		: (unsigned int) count
	byReference	: (BOOL) byReference;

/**
 * Create an IOMemoryContainer object for a single logical range in the
 * caller's address map.
 */
- (id)			initWithAddress
			: (void *) address
	length		: (unsigned int) length;

/**
 * Create an IOMemoryContainer object for a logical scatter-gather list in the
 * caller's address map. The scatter-gather list is provided in BSD
 * Unix iov format.
 */
- (id)			initWithIOV
			: (const struct iovec *) iov
	count		: (unsigned int) count;

/**
 * Accessor methods
 */
- (unsigned int)	rangeCount;
- (unsigned int)	totalByteCount;

/**
 * Manage the retain/release reference count. See NSObject for details.
 */
- (unsigned int)	retainCount;
- (id)			retain;
- (oneway void)		release;

/**
 * Return the logical address and length for the i'th logical range.
 * Return IO_R_INVALID_ARG if the index is outside the allocated range.
 */
- (IOReturn)		logicalRange
			: (IORange *) logicalRange
	index		: (unsigned int) index;


@end /* IOMemoryContainer : Object */

#ifdef KERNEL
@interface IOMemoryContainer(Kernel)

/*
 * Kernel-specific methods. By default, non-kernel objects contain a NULL
 * vm_task_t value. Kernel tasks will set the client to the task that
 * provided this memory.
 */

- (vm_task_t)		client;
- (void)		setClient
			: (vm_task_t) client;

/**
 * Make the memory described by this IOMemoryContainer resident.
 * This is called by the virtual memory manager and/or file system before
 * starting an I/O request. Residency is an all-or-nothing process. The
 * IOMemoryContainer maintains a reference count: the first caller makes the
 * memory resident; the others just increment the count. This method returns
 * an error status if any range cannot be made resident and all memory will
 * be made pageable. This method may only be called by kernel servers.
 */
- (IOReturn)			wireMemory
			: (BOOL) forReading;

/**
 * Make the memory described by this underlying IOMemoryContainer pageable.
 * This is called by the virtual memory manager and/or file system after
 * completing an I/O request.  The IOMemoryContainer maintains a reference
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
- (IOReturn)			checkpoint
			: (IOMemoryCheckpointOption) option;
			
@end /* IOMemoryContainer(Kernel) */
#endif /* KERNEL */




