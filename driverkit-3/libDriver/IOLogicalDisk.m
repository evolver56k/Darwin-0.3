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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOLogicalDisk.m
 *
 * HISTORY
 * 05-Mar-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <mach/mach_interface.h>
#import <driverkit/kernelDiskMethods.h>
#else	KERNEL
#import <mach/mach.h>
#import <bsd/libc.h>
#endif	KERNEL
#import <driverkit/IOLogicalDisk.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/SCSIDisk.h>

@interface IOLogicalDisk(private)
- (IOReturn)_diskParamCommon	:(u_int)logicalOffset
                                 length : (u_int)bytesReq
                                 deviceOffset : (unsigned *)deviceOffset	
                                 bytesToMove : (u_int *)bytesToMove;

- (IOReturn)computePartition	:(u_int)logicalOffset
				 length : (u_int)bytesReq
				 byteOffset : (unsigned long long *)byteOffset	
				 bytesToMove : (u_int *)bytesToMove;

/* This must be a class method since it's called by
 * a class method in DiskPartition.
 */

+ (IOReturn) reblock		: (id)physicalDisk
				: (BOOL)read
    				: (unsigned long long)offset	/* bytes */
				: (unsigned)length 		/* bytes */
				: (unsigned char *)buffer
				: (vm_task_t)client
				: (void *)pending
				: (unsigned)physsize;	/* phys blocksize */
@end

@implementation IOLogicalDisk

/*
 * Standard IODiskDeviceReadingAndWriting methods. We translate into raw device 
 * parameters and pass along to _physicalDisk.
 */
#ifdef	KERNEL

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
				  client : (vm_task_t)client
{
    IOReturn rtn;
    unsigned bytesToMove;
    unsigned long long byteposition;
    
    rtn = [self computePartition:offset 
			    length : length
			byteOffset : &byteposition
			bytesToMove:&bytesToMove];

    if (rtn) {
	return(rtn);
    }

    /* If the request is aligned with the physical block size, we
     * send it directly to the driver. If not, we have to use the
     * (synchronous) reblocking IO function to handle size and/or
     * alignment mismatches.
     */

    if (byteposition % _physicalBlockSize || length % _physicalBlockSize) {
    
	return([[self class] commonReadWrite
			    : _physicalDisk
			    : YES			/* it's a read */
			    : byteposition		 /* byte offset */
			    : (unsigned)bytesToMove 	/* bytes */
			    : (unsigned char *)buffer
			    : (vm_task_t)client
			    : (void *)NULL
			    : (u_int *)actualLength]);

    } else {			/* neatly aligned */

	return([_physicalDisk readAt:(byteposition / _physicalBlockSize)
		    length:bytesToMove
		    buffer:buffer
		    actualLength:actualLength
		    client:client]);
    }
}
				  
- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
    IOReturn rtn;
    unsigned bytesToMove;
    unsigned long long byteposition;

    rtn = [self computePartition:offset 
			    length : length
			byteOffset : &byteposition
			bytesToMove:&bytesToMove];

	if (rtn) {
	    return(rtn);	
	}

    /* If the request is aligned with the physical block size, we
     * send it directly to the driver. If not, we have to use the
     * (synchronous) reblocking IO function to handle size and/or
     * alignment mismatches.
     */

    if (byteposition % _physicalBlockSize || length % _physicalBlockSize) {
    
	return([[self class] commonReadWrite
			    : _physicalDisk
			    : YES
			    : byteposition 		/* byte offset */
			    : (unsigned)bytesToMove 	/* bytes */
			    : (unsigned char *)buffer
			    : (vm_task_t)client
			    : (void *)pending
			    : (u_int *)NULL]);

    } else {				/* neatly aligned */
	return([_physicalDisk readAsyncAt:(byteposition / _physicalBlockSize)
		length:bytesToMove
		buffer:buffer
		pending:pending
		client:client]);
    }
}
				  
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
				  client : (vm_task_t)client
{
    unsigned deviceBlock;
    IOReturn rtn;
    unsigned bytesToMove;
    unsigned long long byteposition;
    
    if ([self isWriteProtected]) {
	    return IO_R_NOT_WRITABLE;
    }

    rtn = [self computePartition:offset 
			    length : length
			byteOffset : &byteposition
			bytesToMove:&bytesToMove];

    if (rtn) {
	return(rtn);
    }

    /* If the request is aligned with the physical block size, we
     * send it directly to the driver. If not, we have to use the
     * (synchronous) reblocking IO function to handle size and/or
     * alignment mismatches.
     */

    if (byteposition % _physicalBlockSize || length % _physicalBlockSize) {
    
	return([[self class] commonReadWrite
			    : _physicalDisk
			    : NO			/* it's a write */
			    : byteposition		 /* byte offset */
			    : (unsigned)bytesToMove 	/* bytes */
			    : (unsigned char *)buffer
			    : (vm_task_t)client
			    : (void *)NULL
			    : (u_int *)actualLength]);

    } else {			/* neatly aligned */

	return([_physicalDisk writeAt:(byteposition / _physicalBlockSize)
		    length:bytesToMove
		    buffer:buffer
		    actualLength:actualLength
		    client:client]);
    }
}
				  	
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
    unsigned deviceBlock;
    IOReturn rtn;
    unsigned bytesToMove;
    unsigned long long byteposition;

    if ([self isWriteProtected]) {
	return IO_R_NOT_WRITABLE;
    }

    rtn = [self computePartition:offset 
			    length : length
			byteOffset : &byteposition
			bytesToMove:&bytesToMove];

    if (rtn) {
	return(rtn);	
    }

    /* If the request is aligned with the physical block size, we
     * send it directly to the driver. If not, we have to use the
     * (synchronous) reblocking IO function to handle size and/or
     * alignment mismatches.
     */

    if (byteposition % _physicalBlockSize || length % _physicalBlockSize) {
    
	return([[self class] commonReadWrite
			    : _physicalDisk
			    : NO
			    : byteposition 		/* byte offset */
			    : (unsigned)bytesToMove 	/* bytes */
			    : (unsigned char *)buffer
			    : (vm_task_t)client
			    : (void *)pending
			    : (u_int *)NULL]);

    } else {				/* neatly aligned */

	return([_physicalDisk writeAsyncAt:(byteposition / _physicalBlockSize)
		length:bytesToMove
		buffer:buffer
		pending:pending
		client:client]);
    }
}

#else	KERNEL

//xxxxx These copies of the routines have not been updated for reblocking.
// They aren't used.

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
{
	unsigned deviceBlock;
	IOReturn rtn;
	unsigned bytesToMove;
	
	if(rtn = [self _diskParamCommon:offset 
	    length : length
	    deviceOffset : &deviceBlock
	    bytesToMove:&bytesToMove])
		return(rtn);	
	return([_physicalDisk readAt:deviceBlock
		length:bytesToMove
		buffer:buffer
		actualLength:actualLength]);
}
				  
- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	unsigned deviceBlock;
	IOReturn rtn;
	unsigned bytesToMove;
	
	if(rtn = [self _diskParamCommon:offset 
	    length : length
	    deviceOffset : &deviceBlock
	    bytesToMove:&bytesToMove])
		return(rtn);	
	return([_physicalDisk readAsyncAt:deviceBlock
		length:bytesToMove
		buffer:buffer
		pending:pending]);
}
				  
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
{
	unsigned deviceBlock;
	IOReturn rtn;
	unsigned bytesToMove;
	
	if ([self isWriteProtected])
		return IO_R_NOT_WRITABLE;
	if(rtn = [self _diskParamCommon:offset 
	    length : length
	    deviceOffset : &deviceBlock
	    bytesToMove:&bytesToMove])
		return(rtn);	
	return([_physicalDisk writeAt:deviceBlock
		length:bytesToMove
		buffer:buffer
		actualLength:actualLength]);
}				  
				  	
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	unsigned deviceBlock;
	IOReturn rtn;
	unsigned bytesToMove;
	
	if ([self isWriteProtected])
		return IO_R_NOT_WRITABLE;
	if(rtn = [self _diskParamCommon:offset 
	    length : length
	    deviceOffset : &deviceBlock
	    bytesToMove:&bytesToMove])
		return(rtn);
	return([_physicalDisk writeAsyncAt:deviceBlock
		length:bytesToMove
		buffer:buffer
		pending:pending]);
}			  	

#endif	KERNEL

- (IODiskReadyState)updateReadyState
{
	return [_physicalDisk updateReadyState];
}

/*
 * Open a connection to physDevice. FIXME: when do we close??
 */
- (IOReturn)connectToPhysicalDisk : physicalDisk
{
	IOReturn rtn = IO_R_SUCCESS;
	
	xpr_disk("connectToPhysicalDiskice\n", 1,2,3,4,5);
	_physicalDisk = physicalDisk;
	[self setIsPhysical:0];
	
	/*
	 * Get some instance variables which must be the same as the
	 * physDevice's.
	 */
	_physicalBlockSize = [physicalDisk blockSize];
	[self setRemovable:[physicalDisk isRemovable]];
	[self setFormattedInternal:[physicalDisk isFormatted]];
	[self setWriteProtected:[physicalDisk isWriteProtected]];
	[self setLogicalDisk:nil];
#ifdef	KERNEL
	[self setDevAndIdInfo:[physicalDisk devAndIdInfo]];
#endif	KERNEL
	return(rtn);
}

- (void)setPartitionBase	: (unsigned)partBase
{
	_partitionBase = partBase;
}


/*
 * Determine if this LogicalDisk (or any other LogicalDisks in the
 * logicalDisk list) are currently open. Returns NO if none open, else
 * YES.
 */
- (BOOL)isOpen
{
	if([self isInstanceOpen])
		return(YES);
	else if([self nextLogicalDisk])
		return([[self nextLogicalDisk] isOpen]);
	else
		return(NO);
}

/*
 * Free self and any objects in logicalDisk chain.
 */
- free
{
	id nextLd;
	
	nextLd = [self nextLogicalDisk];
	if(nextLd) {
		[nextLd free];
	}
	return([super free]);
}

- physicalDisk
{
	return(_physicalDisk);
}

- (unsigned)physicalBlockSize
{
	return(_physicalBlockSize);
}

- (void)setPhysicalBlockSize : (unsigned)size
{
	_physicalBlockSize = size;
}

/*
 * Pass this on to _physicalDisk.
 */
- (IOReturn)isDiskReady	: (BOOL)prompt
{
	return [_physicalDisk isDiskReady:prompt];
}

/*
 * Determine if any logical disks in the _logicalDisk chain except
 * for self are open.
 */
- (BOOL)isAnyOtherOpen
{
	id logDisk = [_physicalDisk nextLogicalDisk];
	while(logDisk) {
		if((logDisk != self) && [logDisk isInstanceOpen]) {
			return YES;
		}
		logDisk = [logDisk nextLogicalDisk];
	}
	return NO;
}

- (BOOL)isInstanceOpen
{
	return _instanceOpen;
}

- (void)setInstanceOpen : (BOOL)isOpen
{
	_instanceOpen = (isOpen ? YES : NO);
}

/*-----------------------------------------*/

/* Common read/write function; handles reblocking of misaligned requests. */

+ (IOReturn) commonReadWrite	: (id)physicalDisk
    				: (BOOL)read
				: (unsigned long long)byteposition
				: (unsigned)length 		/* bytes */
				: (unsigned char *)buffer
				: (vm_task_t)client
				: (void *)pending
				: (u_int *)actualLength
{
    IOReturn rtn = IO_R_SUCCESS;
    unsigned long aligncount;		/* bytes */
    unsigned long integralblocks;
    unsigned long integralbytes;
    unsigned long transfer;
    unsigned long originallength;
    unsigned long physsize;			/* physical blocksize */
   // unsigned char *origbuf;
    u_int tempactual;

    //origbuf = buffer;
    originallength = length;
    tempactual = 0;
    physsize = [physicalDisk blockSize];
    
    /* If the beginning of the request is not properly aligned
     * with the device block size, or it's less than a block,
     * we have to reblock it here to get ourselves aligned.
     */

    if ((byteposition % physsize) || (length < physsize)) {

	/* Compute the number of bytes needed
	 * to bring us into sync with the real size.
	 */
	aligncount = physsize - (byteposition % physsize);
	if (aligncount > length) { /* a short read of less than a real block */
	    aligncount = length;
	}

	rtn = [[IOLogicalDisk class] reblock
		: physicalDisk
		: read				/* read or write */
		: byteposition			/* start byte position */
		: aligncount			/* byte count */
		: buffer
		: client
		: pending
		: physsize];
	length		-= aligncount;		/* update length (bytes) */
	byteposition	+= aligncount;		/* update start byte position */
	buffer		+= aligncount;		/* update buffer */
    }
    
    /* Now we're aligned. Transfer any integral-block "middle" part
     * of the request. The integer divide below excludes any "odd" bytes
     * at the end of the request which aren't part of an integral number
     * of device blocks.
     */

    if (length > 0 && (rtn == IO_R_SUCCESS)) {	/* there's more left to do */

	integralblocks = length / physsize;	/* zero if no full blocks */

	if (integralblocks) {		/* transfer this many aligned blocks */
	    integralbytes = integralblocks * physsize;

	    if (read) {
		rtn = [physicalDisk readAt:(byteposition / physsize)
		    length:integralbytes
		    buffer:buffer
		    actualLength:&tempactual
		    client:client];
	    } else {
		rtn = [physicalDisk writeAt:(byteposition / physsize)
		    length:integralbytes
		    buffer:buffer
		    actualLength:&tempactual
		    client:client];
	    }

	    length	 -= integralbytes;	/* update length (bytes) */
	    byteposition += integralbytes;	/* update start block */
	    buffer	 += integralbytes;	/* update buffer */
	}
    }

    /* If there are any "odd" bytes at the end of the request,
     * transfer these now via reblocking.
     */

    if (length > 0 && (rtn == IO_R_SUCCESS)) {	/* there's more to do */

	rtn = [[IOLogicalDisk class] reblock
			: physicalDisk
			: read
			: byteposition
			: length
			: buffer
			: client
			: pending
			: physsize];
    }    

    /* We only remember the actual length if the transfer succeeds. */

    if (rtn == IO_R_SUCCESS) {
	tempactual = originallength;
    }

    /* Since we've been doing sync IOs here, we have to force a
     * completion if the caller's request was async. Done here,
     * this is ugly, but we have little choice with the current design.
     */

    if (pending) {		/* async: force an I/O completion (ugly) */
	[physicalDisk completeTransfer
			: pending
	    withStatus	: rtn
	    actualLength : tempactual];
    } else {			/* sync */
       *actualLength = tempactual;
    }

    return(rtn);
}
@end

/*
 * Generate device-specific parameters from logical parameters.
 */
@implementation IOLogicalDisk(private)

- (IOReturn)_diskParamCommon	:(unsigned)logicalOffset
                                 length : (unsigned)length
                deviceOffset : (unsigned*)deviceOffset	
                                 bytesToMove : (unsigned *)bytesToMove
{
        unsigned ratio = 0;	// bogus compiler warning
	int deviceBlocks;
	int logicalBlocks;
        unsigned block_size;
	unsigned dev_size;
        IOReturn rtn;
        const char *name = [self name];

        /*
         * Note we have to 'fault in' a possible non-present physical disk in
         * order to get its physical parameters...
         */
        rtn = [_physicalDisk isDiskReady:YES];
        switch(rtn) {
            case IO_R_SUCCESS:
                break;
            case IO_R_NO_DISK:
                xpr_err("%s diskParamCommon: disk not present\n",
                        name, 2,3,4,5);
                return(rtn);
            default:
                IOLog("%s deviceRwCommon: bogus return from isDiskReady "
                        "(%s)\n",
                        name, [self stringFromReturn:rtn]);
                return(rtn);
        }

        block_size = [self blockSize];
        dev_size = [self diskSize];

        if(length % block_size) {
                IOLog("%s: Bytes requested not multiple of block size\n",
                        name);
                return(IO_R_INVALID_ARG);
        }
        logicalBlocks = length / block_size;
        if((logicalOffset + logicalBlocks) > dev_size) {
                if(logicalOffset >= dev_size)
                        return(IO_R_INVALID_ARG);

                /*
                 * Truncate.
                 */
                logicalBlocks = dev_size - logicalOffset;
        }
        ratio = block_size / _physicalBlockSize;
        deviceBlocks = logicalBlocks * ratio;
        *deviceOffset = (logicalOffset * ratio) + _partitionBase;
        *bytesToMove = deviceBlocks * _physicalBlockSize;
        return(IO_R_SUCCESS);
}

- (IOReturn)computePartition	:(unsigned)logicalOffset
				 length : (unsigned)length
				 byteOffset : (unsigned long long *)byteOffset	
				 bytesToMove : (unsigned *)bytesToMove
{
    unsigned physicalBlocksize;
    unsigned logicalBlocksize;
    IOReturn rtn;
    const char *name = [self name];

    unsigned long long partbase;
    unsigned long long partsize;
    unsigned long long partend;
    unsigned long long reqbase;
    
    /*
     * Note we have to 'fault in' a possible non-present physical disk in 
     * order to get its physical parameters...
     */
    rtn = [_physicalDisk isDiskReady:YES];
    switch(rtn) {
	case IO_R_SUCCESS:
	    break;
	case IO_R_NO_DISK:
	    xpr_err("%s diskParamCommon: disk not present\n",
		    name, 2,3,4,5);
	    return(rtn);
	default:
	    IOLog("%s deviceRwCommon: bogus return from isDiskReady "
		    "(%s)\n",
		    name, [self stringFromReturn:rtn]);
	    return(rtn);
    }

    /* We compute partition information here based on the notion of
     * block size, but for better results these boundaries should
     * have been set up in bytes from the start.
     */
    physicalBlocksize = [_physicalDisk blockSize];
    logicalBlocksize  = [self blockSize];

    /* If we don't (yet) know our logical block size, we have to
     * default to the physical block size.
     */
    if (logicalBlocksize == 0) {	/* we don't know (yet)*/
	logicalBlocksize = physicalBlocksize;
    }

    /* The partition base is usually in physical blocks, except when
     * we are reblocking HFS on CD-ROM. In that case, all computations
     * are done in 512-byte logical blocks.
     *
     * Is there a better way to detect that we're an HFS partition?
     */
    if (logicalBlocksize != physicalBlocksize &&
	logicalBlocksize == 512) {			/* must be HFS */
	partbase = (unsigned long long)_partitionBase * 
			(unsigned long long)logicalBlocksize;
    } else {
	partbase = (unsigned long long)_partitionBase *
			(unsigned long long)physicalBlocksize;
    }
    partsize = (unsigned long long)[self diskSize] *
		    (unsigned long long)logicalBlocksize;
    partend  = partbase + partsize;

    /* The caller's block offset is in units expressed by the
     * partitioning scheme. NeXT labeled disks are usually 1024 bytes,
     * HFS is always 512.)
     */
    reqbase = ((unsigned long long)logicalOffset *
		    (unsigned long long)logicalBlocksize) + partbase;
    
    if (reqbase < partbase || reqbase > partend) {
	return(IO_R_INVALID_ARG);
    }

    if ((reqbase + length) > partend) {
	length = partend - reqbase;	/* truncate the request */
    }

    *byteOffset = reqbase;
    *bytesToMove = length;
    return(IO_R_SUCCESS);
}

/* Copy data to (write) or from (read) a misaligned part of a physical block: */

#define MAX_REBLOCK_SIZE 2048

+ (IOReturn) reblock		: (id)physicalDisk
				: (BOOL)read
    				: (unsigned long long)offset	/* bytes */
				: (unsigned)length 		/* bytes */
				: (unsigned char *)buffer
				: (vm_task_t)client
				: (void *)pending
				: (unsigned)physsize	/* phys blocksize */
{
    IOReturn rtn;
    unsigned long physblock;
    int bufoffset;
    u_int tempactual;
    id controller;
    char *reblockbuf;
    void *freePtr;
    unsigned freeCnt;

    /* Allocate a buffer to hold the (larger) device block. This isn't
     * efficient since we allocate and free the buffer each time through,
     * but we're just trying to make the thing work for now.
     */

    controller = [physicalDisk controller];
    reblockbuf = (char *)[controller allocateBufferOfLength:MAX_REBLOCK_SIZE
			    actualStart:&freePtr
			    actualLength:&freeCnt];

    bzero(reblockbuf,MAX_REBLOCK_SIZE);

    /* Figure out which physical block we need, and load it: */

    physblock = offset / physsize;

    rtn = [physicalDisk readAt
		    :physblock
	    length:physsize
	    buffer:reblockbuf
	    actualLength:&tempactual
	    client:client];

    if (rtn != IO_R_SUCCESS) {
	return(rtn);
    }
    
    /* Now copy what the client wants, in the direction specified: */

    bufoffset = offset % physsize;

    if (read) {
	bcopy(&reblockbuf[bufoffset],buffer,length); /* read : buf to client */
    } else {
	bcopy(buffer,&reblockbuf[bufoffset],length); /* write: client to buf */
	    
	/* If writing, flush the buffer to the device: */
	rtn = [physicalDisk writeAt:physblock
		    length:physsize
		    buffer:reblockbuf
		    actualLength:&tempactual
		    client:client];
    }

    IOFree(freePtr,freeCnt);
    return(IO_R_SUCCESS);
}

@end
