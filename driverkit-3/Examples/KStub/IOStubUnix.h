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
 * IOStubUnix.h
 *
 * HISTORY
 * 5-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h>
#import <bsd/sys/buf.h>
#import <objc/objc.h>
#import <bsd/sys/uio.h>

#define	STUB_UNIT(dev)		(minor(dev) >> 3) 
#define STUB_MAX_PHYS_IO	(1024 * 1024)

typedef struct {
	/*
	 * One per unit. Each one of these maps to one node in
	 * /dev/ and to one instance of the IOStub object.
	 */
	struct buf *physbuf;		/* for phys I/O */
	id stub_id;			/* id of IOStub object */
	
} stub_object_t;

extern stub_object_t stub_object[];

extern int stub_open(dev_t dev, int flag);
extern int stub_close(dev_t dev);
extern int stub_read(dev_t dev, struct uio *uiop);
extern int stub_write(dev_t dev, struct uio *uiop);
extern int stub_strategy(register struct buf *bp);
extern int stub_ioctl(dev_t dev, 
	int cmd, 
	caddr_t data, 
	int flag);

