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
/*	IOStubThread.h		1.0	02/07/91	(c) 1991 NeXT   
 *
 * IOStubThread.h - IOStub device I/O thread methods.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#import "IOStub.h"
#import "IOStubPrivate.h"

@interface IOStub(Thread)

- (void)enqueueIoBuf 		: (IOBuf *)buf;
- (IOBuf *)dequeueIoBuf;
- (void)deviceRead		: (IOBuf *)IOBuf;
- (void)deviceWrite		: (IOBuf *)IOBuf;

@end

extern void IOStub_thread(id stub);

