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
 * SCSIDiskThread.h - SCSIDisk I/O Thread methods.
 *
 * HISTORY
 * 04-Mar-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <sys/types.h>
#import <driverkit/SCSIDisk.h>
#import <driverkit/SCSIDiskTypes.h>
#import <bsd/dev/scsireg.h>

@interface SCSIDisk(Thread)

- (void)doSdBuf			: (sdBuf_t *)sdBuf;
- (void)logOpInfo 		: (sdBuf_t *)sdBuf
				  sense : (esense_reply_t *)senseReply;

- (void)genRwCdb 		: (cdb_t *)cdbp
				  readFlag : (BOOL)readFlag
				  block : (u_int)block
				  blockCnt : (u_int)blockCnt;
- (sc_status_t)setupScsiReq	: (sdBuf_t *)sdBuf
				  scsiReq : (IOSCSIRequest *)scsiReq;
- (sc_status_t)reqSense 	: (esense_reply_t *)esenseReply;
- (void)sdIoComplete 		: (sdBuf_t *)sdBuf;
- (void)unlockIoQLock;

@end

extern volatile void sdIoThread(id me);

