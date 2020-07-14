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
 *
 * HISTORY
 * 23-Feb-94    Curtis Galloway at NeXT
 *      Created. 
 */

#import <objc/Object.h>
#import <driverkit/driverTypes.h>

typedef struct {
    IOPhysicalAddress	start;
    unsigned int	size;
} IOPhysicalRange;

typedef struct {
    IOVirtualAddress	start;
    unsigned int	size;
} IOVirtualRange;

typedef enum {
    IOBuf_None		= 0x00,
    IOBuf_Contiguous	= 0x01,		/* Physically contiguous memory */
    IOBuf_Wired		= 0x02,		/* Memory is wired */
    IOBuf_WiredStatic	= 0x04,		/* Memory cannot be unwired */
    IOBuf_Uncacheable	= 0x08		/* Hardware cache should be disabled */
} IOBufferAttributes;

#define IOBufferAlignMin 0		/* No alignment */
#define IOBufferAlignMax 32		/* 2 gigabytes */

@interface IOBuffer : Object
{
@private
    IOBufferAttributes	attributes;
    unsigned int	size;
    void		*data;
    IOAddressMap	map;
    IOVirtualAddress	allocedData;	/* For alignment */
    int			allocedSize;
    IOAlignment		alignment;

    IORange		userRange;
}

/*
 * Initialize a buffer and allocate memory for it.
 */
-initWithSize:(unsigned int)size;
-initWithSize:(unsigned int)size
 attributes:(IOBufferAttributes)attrs
 alignment:(IOAlignment)align;

/*
 * Initialize a buffer from existing memory.
 * The alignment of the buffer will reflect the alignment of
 * the existing memory.
 */
-initFromVirtualAddress:(IOVirtualAddress)addr
 addressMap:(IOAddressMap)map
 size:(unsigned int)size;

-initFromPhysicalAddress:(IOPhysicalAddress)addr
 size:(unsigned int)size;

/*
 * Copy data from another buffer.
 * If the source buffer is smaller than the destination buffer,
 * the remainder will be padded with zeros.
 * Both buffers must be wired.
 */
-(BOOL)copyFromBuffer:(IOBuffer *)sourceBuffer;

/*
 * Copy part of one buffer to another.
 * Both buffers must be wired.
 */
-(BOOL)copyFromBuffer:(IOBuffer *)sourceBuffer
       source:(IORange)srcRange
       destination:(IORange)dstRange;

-(IOAddressMap)addressMap;

-(unsigned int)size;
-(void *)data;
-(IOAlignment)alignment;

/*
 * Wire and unwire the memory in a buffer.  Memory allocated
 * by an -init method may be permanently wired, in which case
 * -unwireMemory will never succeed.
 */
-(BOOL)wireMemory;
-(BOOL)unwireMemory;

/*
 * Get the virtual address at an offset in the buffer.
 * validLength is the length of the remaining portion of the buffer.
 */
-(BOOL)getVirtualAddress:(IOVirtualRange *)virtRangePtr
 at:(unsigned)offset;

/*
 * Get the physical address at an offset in the buffer.
 * validLength is the number of physically contiguous bytes
 * in the buffer starting at that physical address.
 * The buffer must be wired to use this method.
 */
-(BOOL)getPhysicalAddress:(IOPhysicalRange *)physRangePtr
 at:(unsigned)offset;

-(IOBufferAttributes)attributes;
-(BOOL)isContiguous;
-(BOOL)isWired;

/*
 * For the user's convenience... head and tail offset variables that are
 * not used in any way by the internal implementation of the class.
 */
 
-(IORange)range;
-(void)setRange:(IORange)newRange;

/*
 * For subclasses to override...
 */
 
+(BOOL)allocMemoryWithSize:(int)size
       attributes:(IOBufferAttributes)attr
       alignment:(IOAlignment)align
       allocedAddress:(IOVirtualAddress *)allocedPtr
       dataAddress:(IOVirtualAddress *)dataPtr
       allocedSize:(int *)allocedSize
       newAttributes:(IOBufferAttributes *)newAttrPtr
       newMap:(IOAddressMap *)mapPtr;
+(BOOL)freeMemory:(IOVirtualAddress)addr
       size:(int)size
       map:(IOAddressMap)map
       attributes:(IOBufferAttributes)attr;
@end

