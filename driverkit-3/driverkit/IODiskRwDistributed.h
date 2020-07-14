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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * IODiskRwDistrubuted.h - Disk R/W methods, Distributed Objects version.
 *
 * HISTORY
 * 13-Jul-92    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/return.h>
#import <machkit/NXData.h>

/*
 * NXData subclass which optionally frees itself after encoding.
 */
@interface IOData : NXData
{
	BOOL	freeFlag;
}

- (void)setFreeFlag 		: (BOOL)value;
@end

@protocol DiskRwDistributed

/*
 * FIXME - both readAt and the rreadtAt methods currently leak memory
 *  because (apparently) the returned NXData object is not being freed
 * at method return time. How is this supposed to work???
 */
- (IOReturn) readAt		: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (out IOData **)data
				  actualLength : (out unsigned *)actualLength;
				  
/*
 * Temporary...
 */
- (IOData *)rreadAt		: (unsigned)offset
				  length : (unsigned)length
				  rtn : (out IOReturn *)rtn
				  actualLength : (out unsigned *)actualLength;

- (IOReturn) readAsyncAt	: (unsigned)offset
				  length : (unsigned)length
				  ioKey : (unsigned)ioKey
				  caller : (id)caller;
				  
- (IOReturn) writeAt		: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (in IOData *)data
				  actualLength : (out unsigned *)actualLength;

- (IOReturn)writeAsyncAt	: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (in IOData *)data
				  ioKey : (unsigned)ioKey
				  caller : (id)caller;
@end

/*
 * Callbacks for I/O complete of async read and write.
 */
@protocol DiskRwIoComplete

- (IOReturn)readIoComplete	: (unsigned)ioKey
				  device : (id)device
				  data : (in IOData *)data
				  status : (IOReturn)status;
				  
- (IOReturn)writeIoComplete	: (unsigned)ioKey
				  device : (id)device
				  actualLength: (unsigned)actualLength
				  status : (IOReturn)status;
@end
