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
/*
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PCMCIA device description class.
 *
 * HISTORY
 *
 * 10 August 1994 Curtis Galloway at NeXT
 *	Created.
 */
 
#define KERNEL_PRIVATE	1

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/i386/IOPCMCIADeviceDescription.h>
#import <driverkit/i386/IOPCMCIADeviceDescriptionPrivate.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/i386/IOEISADeviceDescriptionPrivate.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/IOPCMCIATuple.h>
#import <driverkit/i386/IOPCMCIATuplePrivate.h>
#import <driverkit/i386/PCMCIAKernBus.h>

struct _pcmcia_private {
    unsigned	tupleCount;
    id		*tupleList;
};

@implementation IOPCMCIADeviceDescription(Private)

- _initWithDelegate: delegate
{
    struct _pcmcia_private *private;

    [super _initWithDelegate:delegate];
    private = _pcmcia_private = 
	(struct _pcmcia_private *)IOMalloc(sizeof(struct _pcmcia_private));
    private->tupleCount = 0;
    private->tupleList = NULL;
    return self;	
}

@end

@implementation IOPCMCIADeviceDescription

- free
{
    struct _pcmcia_private *private =
	(struct _pcmcia_private *)_pcmcia_private;

    if 	(private->tupleList) {
	int i;
	for (i=0; i < private->tupleCount; i++) {
	    [private->tupleList[i] free];
	}
	IOFree(private->tupleList, private->tupleCount * sizeof(id));
    }
    IOFree(private, sizeof(struct _pcmcia_private));
    return [super free];
}

- (unsigned) numTuples
{
    struct _pcmcia_private *private =
	(struct _pcmcia_private *)_pcmcia_private;

    if (private->tupleCount == 0) {
	(void)[self tupleList];
    }
    return private->tupleCount;
}

- (id *) tupleList
{
    struct _pcmcia_private *private = 
	(struct _pcmcia_private *)_pcmcia_private;
    id 	list;
    int i;

    if (private->tupleList == NULL) {
	list = [[self _delegate] resourcesForKey:PCMCIA_TUPLE_LIST];
	if (list) {
	    private->tupleCount = [list count];
	    private->tupleList = 
		(id *)IOMalloc(private->tupleCount * sizeof(id));
	    for (i=0; i<private->tupleCount; i++) {
		id ioTuple, tuple;

		tuple = [list objectAt:i];
		ioTuple = [[IOPCMCIATuple alloc] initWithKernTuple:tuple];
		private->tupleList[i] = ioTuple;
	    }
	}
    }
    return private->tupleList;
}

@end
