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
 * IOSimpleMemoryDescriptor.h
 * Copyright 1997-98 Apple Computer Inc. All Rights Reserved.
 *
 * IOSimpleMemoryDescriptor provides a limited subset of IOMemoryDescriptor
 * functionality for the benefit of low-level drivers (such as the SCSI bus
 * interface driver). The IOSimpleMemoryDescriptor supports a single
 * input logical range. It cannot be replicated, and does not have a separate
 * IOMemoryContainer. It supports a subset of IOMemoryDescriptor methods
 * and fails (with an IOPanic) if the following methods are used with
 * more than a single IORange. the "byReference" parameter is ignored.
 *	initWithIORange
 *	initWithIOV
 *	initWithSerialization 
 * The following methods always fail:
 *	ioMemoryContainer
 *	setIOMemoryContainer
 */
#ifdef KERNEL
#import <driverkit/IOMemoryDescriptor.h>

@interface IOSimpleMemoryDescriptor : IOMemoryDescriptor
{
@protected
    vm_task_t		client;			/* Who owns this memory?    */
    unsigned int	options;		/* Configuration flags	    */
    volatile unsigned int residencyCount;	/* Calls to makeResident    */
}
@end /* IOSimpleMemoryDescriptor : IOMemoryDescriptor */
#endif /* KERNEL */


