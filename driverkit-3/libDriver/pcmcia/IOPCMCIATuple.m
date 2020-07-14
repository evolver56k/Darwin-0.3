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
 * Copyright 1994 NeXT Computer, Inc.
 * All rights reserved.
 */

#import <driverkit/i386/PCMCIATuple.h>
#import <driverkit/i386/IOPCMCIATuple.h>
#import <driverkit/i386/IOPCMCIATuplePrivate.h>

@implementation IOPCMCIATuple(Private)

struct _tuple_private {
    	id		kernTuple;
	unsigned char	code;
	unsigned	length;
	unsigned char	*data;
};

- initWithKernTuple:tuple
{
    struct _tuple_private *private;
    
    if (tuple == NULL)
	return [super free];

    [super init];
    private = (struct _tuple_private *)IOMalloc(sizeof(struct _tuple_private));
    _private = private;
    private->kernTuple = tuple;
    private->code = [tuple code];
    private->length = [tuple length];
    private->data = NULL;
    return self;
}

@end

@implementation IOPCMCIATuple

- free
{
    struct _tuple_private *private = (struct _tuple_private *)_private;
    if (private->data != NULL)
	IOFree(private->data, private->length);
    IOFree(private, sizeof(struct _tuple_private));
    return [super free];
}

- (unsigned char) code
{
    struct _tuple_private *private = (struct _tuple_private *)_private;
    return private->code;
}

- (unsigned) length
{
    struct _tuple_private *private = (struct _tuple_private *)_private;
    return private->length;
}

- (unsigned char *) data;
{
    struct _tuple_private *private = (struct _tuple_private *)_private;
    unsigned length;

    if (private->data == NULL) {
	private->data = IOMalloc(private->length);
	bcopy([private->kernTuple data], private->data, private->length);
    }
    return private->data;
}

@end
