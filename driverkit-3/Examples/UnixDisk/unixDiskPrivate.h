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
/*	unixDiskPrivate.h	1.0	02/07/91	(c) 1991 NeXT   
 *
 * unixDiskPrivate.h - Private methods for unixDisk class.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#ifndef	_UNIX_PRIVATE_
#define _UNIX_PRIVATE_

#import "unixDisk.h"
#import "unixThread.h"

@interface unixDisk(Private)

/*
 * Alloc and free IOBufs. As an optimization, we may want to keep free
 * lists of these around.
 */
- (IOBuf_t *)allocIOBuf;
- (void)freeIOBuf:(IOBuf_t *)IOBuf;

/*
 * Wakeup thread waiting on IOBuf.
 */
- (void)IOBufDone : (IOBuf_t *)IOBuf
		    status : (IOReturn)status;
		    
/*
 * readCommon: and writeCommon: are invoked by the exported flavors of
 * read: and write:.
 */

- (IOReturn) readCommon : (u_int)block 
	 	length : (u_int)length 
		buffer : (void *)bufp
		actualLength : (u_int *)actualLength 	// returned 
		pending : (void *)pending		// pending or ioKey
		caller : (id)caller			// for DO only
		IOBufRtn : (IOBuf_t **)IOBufRtn;
		
- (IOReturn) writeCommon : (u_int)block 
	 	length : (u_int)length 
		buffer : (void *)bufp
		actualLength : (u_int *)actualLength 	// returned
		pending : (void *)pending		// pending or ioKey
		caller : (id)caller			// for DO only
		IOBufRtn : (IOBuf_t **)IOBufRtn;


/*
 * These methods are invoked by a unix_thread; they do the actual work on 
 * unix_fd's.
 */
- (void)unixDeviceRead	: (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum;
- (void)unixDeviceWrite	: (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum;
- (void)deviceEject	: (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum;
- (void)unixDeviceAbort : (IOBuf_t *)IOBuf;
- (void)deviceCheckReady : (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum;
- (void)deviceProbe	: (IOBuf_t *)IOBuf;

@end

#endif	_UNIX_PRIVATE_


