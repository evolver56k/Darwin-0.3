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
 * IOStubUnix.m - UNIX front end for kernel IOStub device. 
 *
 * HISTORY
 * 22-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

/*
 * No IOTask RPCs here.
 */
#undef	MACH_USER_API
#define KERNEL_PRIVATE	1

#import "IOStub.h"
#import "IOStubPrivate.h"
#import "IOStubUnix.h"
#import <bsd/sys/buf.h>
#import <bsd/sys/conf.h>
#import <bsd/sys/uio.h>
#import <bsd/dev/ldd.h>
#import <bsd/sys/errno.h>
#import <bsd/sys/proc.h>
#import <vm/vm_kern.h>

stub_object_t stub_object[NUM_IOSTUBS] = {
	{NULL, nil},
	{NULL, nil},
};

/*
 * prototypes for internal functions
 */
static unsigned stub_minphys(struct buf *bp);


int stub_open(dev_t dev, int flag)
{
	int unit = STUB_UNIT(dev);
	id diskObj = stub_object[unit].stub_id;
	
	xpr_stub("stub_open: unit %d\n", unit, 2,3,4,5);
	if((unit >= NUM_IOSTUBS) || (diskObj == nil))
		return(ENXIO);
	/*
	 * Other device-dependent open stuff here. This device doesn't 
	 * have any, so...
	 */
	return(0);
	
} /* stub_open() */

int stub_close(dev_t dev)
{
	int unit = STUB_UNIT(dev);
	id diskObj = stub_object[unit].stub_id;
	
	xpr_stub("stub_close: unit %d\n", unit, 2,3,4,5);
	if((unit >= NUM_IOSTUBS) || (diskObj == nil))
		return(ENXIO);
	/*
	 * Device-dependent close stuff here. This device doesn't have any,
	 * so...
	 */
	return(0);

} /* stub_close() */

/*
 * Raw I/O use standard UNIX physio routine, resulting in async I/O requests
 * via stub_strategy().
 */
int stub_read(dev_t dev, struct uio *uiop)
{
	int unit = STUB_UNIT(dev);
	stub_object_t *stub = &stub_object[unit];
	u_int block_size;
	char *localBuf;
	int rtn;
	void *userPtr;
	int userCnt;
	struct iovec *iov;
	
	if((unit >= NUM_IOSTUBS) || (stub->stub_id == nil))
		return(ENXIO);
	block_size = [stub->stub_id blockSize];
	
	/*
	 * Have IOStub device read into kernel memory, then copyout.
	 */
	iov = uiop->uio_iov;
	userPtr = iov->iov_base;
	userCnt = iov->iov_len;
	localBuf = IOMalloc(userCnt);
	iov->iov_base = (caddr_t)localBuf;
	uiop->uio_segflg = UIO_SYSSPACE;
	rtn = physio(stub_strategy, 
		stub->physbuf, 
		dev, 
		B_READ, 
		stub_minphys, 
		uiop, 
		block_size);
	copyout(localBuf, userPtr, userCnt);
	IOFree(localBuf, userCnt);
	return rtn;
	
} /* stub_read() */

int stub_write(dev_t dev, struct uio *uiop)
{
	int unit = STUB_UNIT(dev);
	stub_object_t *stub = &stub_object[unit];
	u_int block_size;
	char *localBuf;
	int rtn;
	void *userPtr;
	int userCnt;
	struct iovec *iov;
	
	if((unit >= NUM_IOSTUBS) || (stub->stub_id == nil))
		return(ENXIO);
	block_size = [stub->stub_id blockSize];
	
	/*
	 * Copy user's data into local memory.
	 */
	iov = uiop->uio_iov;
	userPtr = iov->iov_base;
	userCnt = iov->iov_len;
	localBuf = IOMalloc(userCnt);
	iov->iov_base = (caddr_t)localBuf;
	uiop->uio_segflg = UIO_SYSSPACE;
	copyin(userPtr, localBuf, userCnt);
	rtn = physio(stub_strategy, 
		stub->physbuf, 
		dev, 
		B_WRITE, 
		stub_minphys, 
		uiop, 
		block_size);
	IOFree(localBuf, userCnt);
	return rtn;
	
} /* stub_write() */

int stub_strategy(struct buf *bp)
{
	int unit = STUB_UNIT(bp->b_dev);
	id diskObj = stub_object[unit].stub_id;
	u_int offset;
	u_int bytes_req;
	void *bufp;
	u_int block_size;
	IOReturn rtn;
	vm_map_t client;
	
	/*
	 * returns 0/-1.
	 */
	xpr_stub("stub_strategy: unit %d\n", unit, 2,3,4,5);
	if((unit >= NUM_IOSTUBS) || (diskObj == nil)) {
		xpr_stub("stub_strategy: bad unit\n", 1,2,3,4,5);
		bp->b_error = ENXIO;
		goto bad;
	}
	if((bp->b_flags & (B_PHYS|B_KERNSPACE)) == B_PHYS) {
		/*
		 * Physical I/O to user space.
		 */
	    	client = bp->b_proc->task->map;
	}
	else {
		/*
		 * Either block I/O (always kernel space) or physical I/O
		 * to kernel space (e.g., loadable file system).
		 */
	    	client = kernel_map;
	}
	block_size = [diskObj blockSize];
	offset = bp->b_blkno;
	bytes_req = bp->b_bcount;
	bufp = bp->b_un.b_addr;
	if(bp->b_flags & B_READ) {
		rtn = [diskObj readAsyncAt:offset
			length:bytes_req
			buffer:bufp
			pending:(unsigned)bp
			client:client];
	}
	else {
		rtn = [diskObj writeAsyncAt:offset
			length:bytes_req
			buffer:bufp
			pending:(unsigned)bp
			client:client];
	}
	if(rtn) {
		bp->b_error = EINVAL;
		goto bad;
	}
	xpr_stub("stub_strategy: SUCCESS\n", 1,2,3,4,5);
	return(0);
	
bad:
	bp->b_flags |= B_ERROR;
	rtn = -1;
	biodone(bp);
	xpr_stub("stub_strategy: COMMAND REJECT\n", 1,2,3,4,5);
	return(rtn);

} /* stub_strategy() */

int stub_ioctl(dev_t dev, 
	int cmd, 
	caddr_t data, 
	int flag)
{
	int unit = STUB_UNIT(dev);
	id diskObj = stub_object[unit].stub_id;
	int rtn = 0;
	
	xpr_stub("stub_ioctl: unit %d cmd = %d\n", unit, cmd, 3,4,5);

	switch (cmd) {
	    case DKIOCGFORMAT:
		*(int *)data = [diskObj formatted];
		break;
		
	    case DKIOCGLABEL:
	    	/* no label */
	    	rtn = ENXIO;
		break;

	    case DKIOCINFO:
	        {
			struct drive_info info;
			
			bzero(&info, sizeof(info));
			strcpy(info.di_name, [diskObj driveName]);
			info.di_devblklen = [diskObj blockSize];
			info.di_maxbcount = STUB_MAX_PHYS_IO;
			*(struct drive_info *)data = info;
			break;
		}
		
	    case DKIOCBLKSIZE:
	   	*(int *)data = [diskObj blockSize];
		break;
		
	    case DKIOCNUMBLKS:
	   	*(int *)data = [diskObj deviceSize];
		break;
		
	    default:
		rtn = EINVAL;
	}
	return rtn;
} /* stub_ioctl() */

/*
 * Called out from physio().
 */
static unsigned stub_minphys(struct buf *bp)
{
	if (bp->b_bcount > STUB_MAX_PHYS_IO)
		bp->b_bcount = STUB_MAX_PHYS_IO;
	return(bp->b_bcount);
}

/* end of IOStubUnix.m */

