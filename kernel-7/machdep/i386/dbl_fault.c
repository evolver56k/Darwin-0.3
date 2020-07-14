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
 * Handling of double faults.
 *
 * HISTORY
 *
 * 12 May 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <architecture/i386/table.h>

#import <machdep/i386/trap.h>
#import <machdep/i386/idt.h>
#import <machdep/i386/desc_inline.h>
#import <machdep/i386/table_inline.h>

tss_t			dbf_tss;
thread_saved_state_t	dbf_state;
#define DBF_STACK_SIZE	1024
unsigned char		dbf_stack[DBF_STACK_SIZE];

void			dbf_handler_(void);

void
dbf_init(void)
{
    tss_t	*tss = &dbf_tss;
    
    tss->cr3 = pmap_kernel()->cr3;
    tss->esp0 = (unsigned int)&dbf_stack[DBF_STACK_SIZE];
    tss->ss0 = KDS_SEL;
    tss->esp = tss->ebp = tss->esp0;
    tss->ss = KDS_SEL;
    tss->eip = (unsigned int)dbf_handler_;
    tss->cs = KCS_SEL;
    tss->ds = KDS_SEL;
    tss->es = KDS_SEL;
    tss->io_bmap = TSS_SIZE(0);
    
    map_tss(
	    sel_to_gdt_entry(DBLFLT_SEL),
		(vm_offset_t)&dbf_tss + KERNEL_LINEAR_BASE,
		    (vm_size_t)TSS_SIZE(0));
		    
    make_task_gate(
	    (task_gate_t *)&idt[T_DOUBLE_FAULT],
		DBLFLT_SEL, 0, KERN_PRIV);
}

void
dbf_handler(
    err_code_t		err
)
{
    thread_saved_state_t	*state = &dbf_state;
    tss_t			*tss = current_thread()->pcb->tss;

    while (TRUE) {
    	state->trapno = T_DOUBLE_FAULT;
	
	state->frame.err = err;
	state->frame.eip = tss->eip;
	state->frame.cs = tss->cs;
	state->frame.eflags = tss->eflags;
	
	state->regs.eax = tss->eax;
	state->regs.ecx = tss->ecx;
	state->regs.edx = tss->edx;
	state->regs.ebx = tss->ebx;
	state->regs.ebp = tss->ebp;
	state->regs.esi = tss->esi;
	state->regs.edi = tss->edi;
	state->regs.ds = tss->ds;
	state->regs.es = tss->es;
	state->regs.fs = tss->fs;
	state->regs.gs = tss->gs;
	
	kernel_trap(state);
    }
}
