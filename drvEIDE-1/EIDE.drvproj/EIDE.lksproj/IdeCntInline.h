/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdeCntInline.h - included by IdeCnt.m -- misc functions
 *
 * HISTORY 
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */

/*
 * FIXME: must get rid of this. 
 */
extern vm_offset_t
pmap_resident_extract(
    pmap_t		pmap,
    vm_offset_t		va
);


static __inline__
void
outw_fast(
    io_addr_t		port,
    unsigned short	data
)
{
    asm volatile(
    	"outw %1,%0"
	: 
	: "d" (port), "a" (data)
	: "cc");
}

/*
 * This is same as one in standard header files except that the header file
 * that gets pulled in has wrong code for this function. FIXME. 10/5/94.
 */
static __inline__
unsigned long
inl_mine(
    IOEISAPortAddress	port
)
{
    unsigned long	data;
    
    asm volatile(
    	"inl %1,%0"
	: "=a" (data)
	: "d" (port));
	
    return (data);
}

static __inline__
void
outl_fast(
    IOEISAPortAddress	port,
    unsigned long	data
)
{
    asm volatile(
    	"outl %1,%0"
	:
	: "d" (port), "a" (data)
	: "cc");
}

static __inline__
void 
rwBuffer_16(caddr_t addr, BOOL read, unsigned length, 
		unsigned ideDataRegister)
{
    unsigned int len = length;
    unsigned short *dst;
    
    dst = (unsigned short *)addr;
    length /= 2;
    
    if (read) {
	while (length-- > 0)
	    *dst++ = inw(ideDataRegister);
    } else {
	while (length-- > 0)
	    outw_fast(ideDataRegister, *dst++);
    }
    
    if (len % 2 == 0)
    	return;

    /*
     * Extra byte. This can happen for ATAPI I/O requests.
     */
    if (read) {
	* (unsigned char *)dst = inw(ideDataRegister);
    } else {
	outw(ideDataRegister, * (unsigned char *)dst);
    }
}

/*
 * Using 32-bit access.
 */
static __inline__
void 
rwBuffer_32(caddr_t addr, BOOL read, unsigned length, 
		unsigned ideDataRegister)
{
    unsigned int len = length;
    unsigned int *dst;
    
    dst = (unsigned int *)addr;
    length /= 4;
    
    if (read) {
	while (length-- > 0)
	    *dst++ = inl_mine(ideDataRegister);
    } else {
	while (length-- > 0)
	    outl_fast(ideDataRegister, *dst++);
    }
    
    if (len % 4 == 0)
    	return;

    /*
     * Hand off the remainder to the standard routine. 
     */
    rwBuffer_16((caddr_t) dst, read, len % 4, ideDataRegister);
}

static __inline__
void
rwBuffer(caddr_t addr, BOOL read, unsigned length,
                unsigned ideDataRegister, ideTransferWidth_t transferWidth)
{
    if (transferWidth == IDE_TRANSFER_32_BIT)	{
        rwBuffer_32(addr, read, length, ideDataRegister);
    } else {
        rwBuffer_16(addr, read, length, ideDataRegister);
    }
}

/*
 * Note: length is <= PAGE_SIZE while calling this function. If a request is
 * of odd size and is split over two pages then we always make the second one
 * odd sized. 
 */

static  __inline__
void    
ideXferData(caddr_t addr, BOOL read, struct vm_map *client,
	    unsigned length, ideRegsAddrs_t ideRegs, 
	    ideTransferWidth_t transferWidth)
{
    unsigned count, offset;
    unsigned short sw;
    extern struct vm_map *kernel_map;
    caddr_t maddr0, maddr1;

    /*
     * Simple case, no mapping required. 
     */
    if (client == kernel_map) {
        rwBuffer(addr, read, length, ideRegs.data, transferWidth);
		return;
    }

    /*
     * Get the physical address here. 
     */
    offset = (unsigned)addr & (PAGE_SIZE - 1);
    maddr0 = (caddr_t) pmap_resident_extract(
		vm_map_pmap_EXTERNAL((struct vm_map *) client), 
		(vm_offset_t) addr);
		    
    if ((PAGE_SIZE - offset) < length) {	/* this is a pain */
	count = PAGE_SIZE - offset;
	maddr1 = (caddr_t) pmap_resident_extract(
		    vm_map_pmap_EXTERNAL((struct vm_map *) client),
		    (vm_offset_t) addr + count);
	if (count % 2) {
            rwBuffer((unsigned char *)pmap_phys_to_kern(maddr0),
                            read, (count - 1), ideRegs.data, transferWidth);
	    
	    if (read) {
		sw = inw(ideRegs.data);
		*((unsigned char *)pmap_phys_to_kern(maddr0 + count - 1)) =
		    (unsigned char)(sw & 0xff);
		*((unsigned char *)pmap_phys_to_kern(maddr1)) = 
			(unsigned char)((sw & 0xff) >> 8);
	    } else {
		sw = *((unsigned char *)pmap_phys_to_kern(maddr0+count - 1));
		sw |= *((unsigned char *)pmap_phys_to_kern(maddr1)) << 8;
		outw_fast(ideRegs.data, sw);
	    }
	    maddr1++;
            rwBuffer((unsigned char *)pmap_phys_to_kern(maddr1),
                            read, (length - count - 1), 
			    ideRegs.data, transferWidth);
	} else {
            rwBuffer((unsigned char *)pmap_phys_to_kern(maddr0),
                            read, count, ideRegs.data, transferWidth);
            rwBuffer((unsigned char *)pmap_phys_to_kern(maddr1),
                            read, (length - count), 
 			    ideRegs.data, transferWidth);
	}
    } else {
        rwBuffer((unsigned char *) pmap_phys_to_kern(maddr0),
                        read, length, ideRegs.data, transferWidth);
    }
}
