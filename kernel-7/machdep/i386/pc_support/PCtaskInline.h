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
 * Inline functions to access the task address space.
 *
 * HISTORY
 *
 * 15 Jan 1993 ? at NeXT
 *	Created.
 */

#if	KERNEL_PRIVATE

#import <mach/mach_types.h>

static __inline__
boolean_t
fetchByte(
    thread_t			thread,
    vm_offset_t			address,
    unsigned char		*data
)
{    
    thread->recover = (vm_offset_t)&&fault;

    asm volatile(
    	"movb %%fs:%1,%0"
		: "=q" (*data)
		: "m" (*(unsigned char *)address));
				
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

static __inline__
boolean_t
fetch2Byte(
    thread_t			thread,
    vm_offset_t			address,
    unsigned short		*data
)
{    
    thread->recover = (vm_offset_t)&&fault;
    
    asm volatile(
    	"movw %%fs:%1,%0"
		: "=q" (*data)
		: "m" (*(unsigned short *)address));
		
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

static __inline__
boolean_t
store2Byte(
    thread_t			thread,
    vm_offset_t			address,
    unsigned short		data
)
{
    thread->recover = (vm_offset_t)&&fault;
    
    asm volatile(
    	"movw %0,%%fs:%1"
		:
		: "r" (data), "m" (*(unsigned short *)address));
		
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

static __inline__
boolean_t
fetch4Byte(
    thread_t			thread,
    vm_offset_t			address,
    unsigned long		*data
)
{    
    thread->recover = (vm_offset_t)&&fault;
    
    asm volatile(
    	"movl %%fs:%1,%0"
		: "=r" (*data)
		: "m" (*(unsigned long *)address));
		
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

static __inline__
boolean_t
store4Byte(
    thread_t			thread,
    vm_offset_t			address,
    unsigned long		data
)
{
    thread->recover = (vm_offset_t)&&fault;
    
    asm volatile(
    	"movl %0,%%fs:%1"
		:
		: "r" (data), "m" (*(unsigned long *)address));
		
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

static __inline__
boolean_t
fetch8Byte(
    thread_t			thread,
    vm_offset_t			address,
    void			*data
)
{
    thread->recover = (vm_offset_t)&&fault;
    
    asm volatile(
    	"movl %%fs:%1,%0"
		: "=r" ( *(unsigned long *)data )
		: "m" ( *(unsigned long *)address ));
    asm volatile(
    	"movl %%fs:%1,%0"
		: "=r" ( *(((unsigned long *)data)+1) )
		: "m" ( *(((unsigned long *)address)+1) ));
		
    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
}

#endif
