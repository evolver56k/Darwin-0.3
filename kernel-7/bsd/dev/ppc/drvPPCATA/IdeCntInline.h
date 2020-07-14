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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/*
 * Copyright (c) 1991-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCntInline.h - included by IdeCnt.m -- misc functions
 *
 * HISTORY 
 */


#import <driverkit/kernelDriver.h>
#import "io_inline.h"


static __inline__
void 
rwBuffer(caddr_t addr, BOOL read, unsigned length, unsigned ideDataRegister)
{
    unsigned int len = length;
    unsigned short *dst;
    
    dst = (unsigned short *)addr;
    length /= 2;
    
    if (read) 
    {
	while (length-- > 0)
	    *dst++ = inw(ideDataRegister);
    } 
    else 
    {
	while (length-- > 0)
	    outw(ideDataRegister, *dst++);
    }
    
    if (len % 2 == 0)
    	return;

    /*
     * Extra byte. This can happen for ATAPI I/O requests.
     */
    if (read) 
    {
	*(unsigned char *)dst = inw(ideDataRegister);
    } 
    else 
    {
	outw(ideDataRegister, * (unsigned char *)dst);
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
	    unsigned length, ideRegsAddrs_t ideRegs )
{
    unsigned count, 		offset;
    unsigned short 		sw;
    extern struct vm_map 	*kernel_map;
    u_int32_t 			maddr0, maddr1; 
    vm_address_t 		xaddr0, xaddr1;

    /*
     * Simple case, no mapping required. 
     */
    if (client == kernel_map) 
    {
        rwBuffer(addr, read, length, ideRegs.data);
	return;
    }

    offset = (unsigned)addr & (PAGE_SIZE - 1);

    IOPhysicalFromVirtual( (vm_task_t)client, (vm_address_t)addr, &maddr0 );
    maddr0 = trunc_page(maddr0);

    if ((PAGE_SIZE - offset) < length) 		/* this is a pain */
    {
	count = PAGE_SIZE - offset;

        IOPhysicalFromVirtual( (vm_task_t)client, (vm_address_t)round_page(addr), &maddr1 );
        maddr1 = trunc_page(maddr1);

        IOMapPhysicalIntoIOTask( maddr0, PAGE_SIZE, &xaddr0 );
        IOMapPhysicalIntoIOTask( maddr1, PAGE_SIZE, &xaddr1 );

        xaddr0 = trunc_page(xaddr0) + offset;
        xaddr1 = trunc_page(xaddr1);

	if (count % 2) 
        {
            rwBuffer((unsigned char *)xaddr0, read, (count - 1), ideRegs.data);
           	    
	    if (read) 
            {
		sw = inw(ideRegs.data);
		((unsigned char *)xaddr0)[count-1] = (unsigned char)((sw & 0xff00) >> 8);
		((unsigned char *)xaddr1)[0]       = (unsigned char) (sw & 0x00ff);
	    } 
            else 
            {
		sw  = ((unsigned char *)xaddr0)[count - 1] << 8;
		sw |= ((unsigned char *)xaddr1)[0];
		outw(ideRegs.data, sw);
	    }
	    xaddr1++;
            rwBuffer((unsigned char *)xaddr1, read, (length - count - 1), ideRegs.data);
	} 
        else 
        {
            rwBuffer((unsigned char *)xaddr0, read, count, ideRegs.data );
            rwBuffer((unsigned char *)xaddr1, read, (length - count), ideRegs.data );
	}

        xaddr0 = trunc_page(xaddr0);
        xaddr1 = trunc_page(xaddr1);
        IOUnmapPhysicalFromIOTask( xaddr0, PAGE_SIZE );
        IOUnmapPhysicalFromIOTask( xaddr1, PAGE_SIZE );
    } 
    else 
    {
        IOMapPhysicalIntoIOTask( maddr0, PAGE_SIZE, &xaddr0 );
        xaddr0 = trunc_page(xaddr0) + offset;

        rwBuffer((unsigned char *) xaddr0, read, length, ideRegs.data );

        xaddr0 = trunc_page(xaddr0);
        IOUnmapPhysicalFromIOTask( xaddr0, length );
    }
}
