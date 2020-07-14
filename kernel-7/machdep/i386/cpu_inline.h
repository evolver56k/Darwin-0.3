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
 * Copyright (c) 1992, 1994 NeXT Computer, Inc.
 *
 * Intel386 Family:	Access to special processor functions.
 *
 * HISTORY
 *
 * 5 April 1992 ? at NeXT
 *	Created.
 */
 
#import <architecture/i386/cpu.h>

static inline
unsigned int
eflags(void)
{
    unsigned int	value;

    asm volatile("
	pushf
	popl	%0"
	    : "=r" (value));
	    
    return (value);
}

static inline
void
set_eflags(
    unsigned int	value
)
{
    asm volatile("
    	pushl	%0;
	popf"
	    :
	    : "r" (value));
}

static inline
void
cli(void)
{
    asm volatile("
	cli");
}

static inline
void
sti(void)
{
    asm volatile("
	sti");
}

static inline
cr0_t
cr0(void)
{
    cr0_t	value;
    
    asm volatile("
	mov %%cr0,%0"
	    : "=r" (value));
		    
    return (value);
}

static inline
void
set_cr0(
    cr0_t	value
)
{
    asm volatile("
	mov %0,%%cr0"
	    :
	    : "r" (value));
}

static inline
void
clts(void)
{
    asm volatile("
	clts");
}

static inline
void
setts(void)
{
    cr0_t	temp;
    
    temp = cr0();
    temp.ts = 1;
    
    set_cr0(temp);
}

static inline
void
ltr(void)
{
    asm volatile("ltr %0"
		    :
		    : "m" (TSS_SEL));
}

static inline
void
lldt(void)
{
    asm volatile("lldt %0"
		    :
		    : "m" (LDT_SEL));
}

static inline
vm_offset_t
cr2(void)
{
    vm_offset_t	value;
    
    asm volatile("
	mov %%cr2,%0"
	    : "=r" (value));
		    
    return (value);
}

static inline
vm_offset_t
cr3(void)
{
    vm_offset_t	value;
    
    asm volatile("
	mov %%cr3,%0"
	    : "=r" (value));
		    
    return (value);
}

static inline
void
set_cr3(
    vm_offset_t	value
)
{
    asm volatile("
	mov %0,%%cr3"
	    :
	    : "r" (value));
}

static inline
void
flush_tlb(void)
{
    set_cr3(
		cr3()
    );
}

static inline
void
invlpg(
    vm_offset_t		addr,
    boolean_t		kernel
)
{
    if (kernel)
	asm volatile("
	    invlpg %0"
		:
		: "m" (*(unsigned char *)addr));
    else
	asm volatile("
	    invlpg %%fs:%0"
		:
		: "m" (*(unsigned char *)addr));
}

static inline
void
flush_cache(void)
{
    asm volatile("wbinvd");
}

static inline
void
enable_cache(void)
{
    cr0_t	_cr0 = cr0();
    
    _cr0.cd = _cr0.nw = 0;
    flush_cache(); set_cr0(_cr0);
}

static inline
void
disable_cache(void)
{
    cr0_t	_cr0 = cr0();
    
    _cr0.cd = _cr0.nw = 1;
    set_cr0(_cr0); flush_cache();
}

static inline
void
freeze_cache(void)
{
    cr0_t	_cr0 = cr0();
    
    _cr0.cd = 1; _cr0.nw = 0;
    set_cr0(_cr0);
}

static inline
dr6_t
dr6(
    void
)
{
    dr6_t	value;
    
    asm volatile("
	mov %%db6,%0"
	    : "=r" (value));
		    
    return (value);
}

static inline
void
set_dr6(
    dr6_t	value
)
{
    asm volatile("
	mov %0,%%db6"
	    :
	    : "r" (value));
}
