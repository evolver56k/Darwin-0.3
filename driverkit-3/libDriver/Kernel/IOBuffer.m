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
 */

#if NOTYET

/* Clean this up later. */
#define KERNEL_PRIVATE	1 
#define ARCH_PRIVATE	1 

#import <driverkit/IOBuffer.h>
#import <driverkit/generalFuncs.h>
#import <vm/vm_map.h>
#import <mach/vm_param.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/*
 * Helper functions
 */
static BOOL checkAlignmentFromVirtual(
    IOVirtualAddress	vaddr,
    IOAddressMap	map,
    int			size,
    IOAlignment	align
);
static BOOL checkAlignmentFromPhysical(
    IOPhysicalAddress	paddr,
    int			size,
    IOAlignment	align
);
static BOOL checkAlignmentFromSize(
    int			size,
    IOAlignment	align
);
static unsigned int
maskFromAlignment(
    IOAlignment		align
);
static unsigned int
alignmentFromAddress(
    IOVirtualAddress	addr
);
static void
copyData(
    IOVirtualAddress src,
    IOAddressMap srcMap,
    IOVirtualAddress dst,
    IOAddressMap dstMap,
    unsigned int totalLength
);



@implementation IOBuffer

-initWithSize:(unsigned int)newSize
{
    return [self initWithSize:newSize attributes:IOBuf_None
            alignment:IOBufferAlignMin];
}

-initWithSize:(unsigned int)newSize
 attributes:(IOBufferAttributes)newAttrs
 alignment:(IOAlignment)align
{
    if (checkAlignmentFromSize(newSize, align) == NO) {
	return nil;
    }
    
    size = newSize;
    alignment = align;
    if ([[self class] allocMemoryWithSize:newSize
		attributes:newAttrs
		alignment:align
		allocedAddress:&allocedData
		dataAddress:(IOVirtualAddress *)&data
		allocedSize:&allocedSize
		newAttributes:&attributes
		newMap:&map] == NO) {
	return nil;
    }

    attributes = newAttrs | IOBuf_Wired | IOBuf_Contiguous | IOBuf_WiredStatic;

    map = (IOAddressMap)IOVmTaskSelf();
    
    return self;
}


-initFromVirtualAddress:(IOVirtualAddress)addr
 addressMap:(IOAddressMap)newMap
 size:(unsigned int)newSize
{
    data = (void *)addr;
    map = newMap;

    alignment = MIN(alignmentFromAddress(addr),
    		    alignmentFromAddress(newSize));
    size = newSize;
	
    return self;
}


-initFromPhysicalAddress:(IOPhysicalAddress)addr
 size:(unsigned int)newSize
{
    data = (void *)addr;
    
    alignment = MIN(alignmentFromAddress(addr),
                    alignmentFromAddress(newSize));
    return nil;
}

-free
{
    (void)[self unwireMemory];
    if (allocedData) {
	[[self class] freeMemory:allocedData
	      size:allocedSize
	      map:map
	      attributes:attributes];
    }
    return [super free];
}

-(BOOL)copyFromBuffer:(IOBuffer *)sourceBuffer
{
    int length, sourceSize;
    BOOL rtn;
    IORange source, dest;
    
    sourceSize = [sourceBuffer size];
    source.size = dest.size = MIN(sourceSize, size);
    source.start = dest.start = 0;
    rtn = [self copyFromBuffer:sourceBuffer
		source:source
		destination:dest];
    if ((rtn == YES) && (sourceSize < source.size)) {
	bzero((char *)data + sourceSize, source.size - sourceSize);
    }
    return rtn;
}

-(BOOL)copyFromBuffer:(IOBuffer *)sourceBuffer
       source:(IORange)srcRange
       destination:(IORange)dstRange
{
    int sourceSize, destSize;
    IOVirtualRange sourceAddrRange, destAddrRange;
    
    if ([self isWired] == NO || [sourceBuffer isWired] == NO)
	return NO;

    sourceSize = [sourceBuffer size] - srcRange.start;
    destSize = [self size] - dstRange.start;
    if ((srcRange.size > sourceSize) || (srcRange.size > destSize))
	return NO;
	
    if ([sourceBuffer getVirtualAddress:&sourceAddrRange
		      at:srcRange.start] == NO) {
	return NO;
    }
    if ([self getVirtualAddress:&destAddrRange
		      at:dstRange.start] == NO) {
	return NO;
    }
    copyData(sourceAddrRange.start, [sourceBuffer addressMap],
	     destAddrRange.start, map, srcRange.size);

    return YES;
}

-(IOAddressMap)addressMap
{
    return map;
}


-(BOOL)wireMemory
{
    if ([self isWired])
	return YES;
    
    if (vm_map_pageable(map, trunc_page((char *)data),
		             round_page((char *)data + size), FALSE)
	    == KERN_SUCCESS) {
	attributes |= IOBuf_Wired;
	return YES;
    }

    return NO;
}

-(BOOL)unwireMemory
{
    if ([self isWired] == NO)
	return YES;
    if (!(attributes & IOBuf_WiredStatic) &&
        (vm_map_pageable(map, trunc_page((char *)data),
	                      round_page((char *)data + size), TRUE)
	    == KERN_SUCCESS)) {
	attributes &= ~IOBuf_Wired;
	return YES;
    }
    return NO;
}

-(BOOL)getVirtualAddress:(IOVirtualRange *)virtualRangePtr
 at:(unsigned int)offset
{
    if (offset >= size)
	return NO;
    if (virtualRangePtr != NULL) {
	virtualRangePtr->start = (IOVirtualAddress)((char *)data + offset);
	virtualRangePtr->size = size - offset;
    }
    return YES;
}

-(BOOL)getPhysicalAddress:(IOPhysicalRange *)physicalRangePtr
 at:(unsigned int)offset
{
    IOPhysicalAddress paddr;
    char *vaddr;
    int cbytes = 0;
    IOPhysicalAddress nextContPage, nextPhysPage;
    char *nextVirtPage, *curVirtPage;
    int bytesLeftInPage, curByteCount;
    
    if ((offset >= size) || ![self isWired])
	return NO;
	
    vaddr = (char *)data + offset;
    paddr = (IOPhysicalAddress) pmap_resident_extract(
	vm_map_pmap((vm_map_t)map), vaddr);

    if (physicalRangePtr != NULL) {
	curByteCount = size - offset;
	curVirtPage = (char *)trunc_page(vaddr);
	nextContPage = (IOPhysicalAddress)trunc_page(paddr) + PAGE_SIZE;
	bytesLeftInPage = nextContPage - paddr;
	
	while (curByteCount > 0) {
	    if (bytesLeftInPage >= curByteCount ){
		    cbytes += curByteCount;
		    break;
	    } else {
		cbytes += bytesLeftInPage;

		nextPhysPage = (IOPhysicalAddress) pmap_resident_extract(
		    vm_map_pmap((vm_map_t)map),
		    (curVirtPage + PAGE_SIZE));

		if (nextPhysPage == nextContPage){
		    curByteCount -= bytesLeftInPage;
		    bytesLeftInPage = PAGE_SIZE;
		    curVirtPage += PAGE_SIZE;
		    nextContPage += PAGE_SIZE;				
		} else {
		    break;
		}
	    } 
	}
    }
    if (physicalRangePtr != NULL) {
	physicalRangePtr->start = paddr;
	physicalRangePtr->size = cbytes;
    }
    return YES;
}

-(IOBufferAttributes)attributes
{
    return attributes;
}

-(BOOL)isContiguous
{
    return ((attributes & IOBuf_Contiguous) ? YES : NO);
}

-(BOOL)isWired
{
    return ((attributes & IOBuf_Wired) ? YES : NO);
}

-(unsigned int)size
{
    return size;
}

-(void *)data
{
    return data;
}

-(IOAlignment)alignment
{
    return alignment;
}

-(IORange)range
{
    return userRange;
}


-(void)setRange:(IORange)newRange
{
    userRange = newRange;
}


+(BOOL)allocMemoryWithSize:(int)memSize
       attributes:(IOBufferAttributes)attr
       alignment:(IOAlignment)align
       allocedAddress:(IOVirtualAddress *)allocPtr
       dataAddress:(IOVirtualAddress *)dataPtr
       allocedSize:(int *)sizePtr
       newAttributes:(IOBufferAttributes *)newAttrPtr
       newMap:(IOAddressMap *)mapPtr
{
    IOVirtualAddress vaddr;
    int alignSize, newSize;
    
    alignSize = IOAlignmentToSize(align);
    newSize = memSize + 2 * alignSize;
    
    /* an optimization that relies on IOMalloc returning aligned pages. */
    if (newSize > PAGE_SIZE && memSize <= PAGE_SIZE)
	newSize = PAGE_SIZE;
    
    vaddr = (IOVirtualAddress)IOMalloc(newSize);
    if (vaddr == 0)
	return FALSE;

    *allocPtr = vaddr;
    if (newSize != memSize)
	*dataPtr = (vaddr + alignSize - 1) & ~(alignSize - 1);
    else
	*dataPtr = vaddr;
    *sizePtr = newSize;
    *newAttrPtr = IOBuf_Wired | IOBuf_WiredStatic | IOBuf_Contiguous;
    *mapPtr = (IOAddressMap)IOVmTaskSelf();
    return TRUE;
}


+(BOOL)freeMemory:(IOVirtualAddress)addr
       size:(int)memsize
       map:(IOAddressMap)memmap
       attributes:(IOBufferAttributes)attr
{
    IOFree((void *)addr, memsize);
    return TRUE;
}

@end

/*
 * Helper functions
 */
 
/*
 * Returns TRUE if the address meets the specified aligment requirement.
 */
static BOOL checkAlignmentFromVirtual(
    IOVirtualAddress	vaddr,
    IOAddressMap	map,
    int			size,
    IOAlignment	align
)
{
    IOPhysicalAddress paddr;
    
    paddr = (IOPhysicalAddress) pmap_resident_extract(
	vm_map_pmap((vm_map_t)map), vaddr);

    return checkAlignmentFromPhysical(paddr, size, align);
}

static BOOL checkAlignmentFromPhysical(
    IOPhysicalAddress	paddr,
    int			size,
    IOAlignment		align
)
{
    unsigned int mask;
    
    if (align > IOBufferAlignMax)
	return NO;

    mask = maskFromAlignment(align);
    if ((paddr & mask) || (size & mask))
	return NO;

    return YES;
}

static BOOL checkAlignmentFromSize(
    int			size,
    IOAlignment	align
)
{
    unsigned int mask;
    if (align > IOBufferAlignMax)
	return NO;
    mask = maskFromAlignment(align);
    if (size & mask)
	return NO;
    return YES;
}

static unsigned int
maskFromAlignment(
    IOAlignment		align
)
{
    unsigned int mask;
    
    for (mask = 0; align; align--) {
	mask = (mask << 1) | 1;
    }
    return mask;
}

static unsigned int
alignmentFromAddress(
    unsigned int	addr
)
{
    unsigned int alignment;
    
    for (alignment=0; alignment < PAGE_SHIFT; alignment++) {
	if (addr & 0x01)
	    return alignment;
	addr >>= 1;
    }
    return alignment;
}

static void
copyData(
    IOVirtualAddress src,
    IOAddressMap srcMap,
    IOVirtualAddress dst,
    IOAddressMap dstMap,
    unsigned int totalLength
)
{
    IOPhysicalAddress physSrc, physDst;
    int currentLength;
    int leftInSrc, leftInDst;
    const unsigned int pageMask = PAGE_SIZE - 1;

    while (totalLength) {
	physSrc = (IOPhysicalAddress) pmap_resident_extract(
	    vm_map_pmap((vm_map_t)srcMap), src);
	physDst = (IOPhysicalAddress) pmap_resident_extract(
	    vm_map_pmap((vm_map_t)dstMap), dst);
	leftInSrc = PAGE_SIZE - (physSrc & pageMask);
	leftInDst = PAGE_SIZE - (physDst & pageMask);
	currentLength = MIN(leftInSrc, leftInDst);
	currentLength = MIN(totalLength, currentLength);
	bcopy(pmap_phys_to_kern(physSrc),
	      pmap_phys_to_kern(physDst),
	      currentLength);
	totalLength -= currentLength;
	src += currentLength;
	dst += currentLength;
    }
}

#endif NOTYET

