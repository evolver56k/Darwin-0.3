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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Global descriptor table.
 *
 * HISTORY
 *
 * 3 April 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <machdep/i386/seg.h>
#import <machdep/i386/table_inline.h>
#import <machdep/i386/desc_inline.h>

void
locate_gdt(
    vm_offset_t		base
)
{
    extern
	unsigned int	gdt_base;
    extern
	unsigned short	gdt_limit;
	
    gdt_base = base;
    gdt_limit = (GDTSZ * sizeof (gdt_entry_t)) - 1;
    
    asm volatile("lgdt %0"
		    :
		    : "m" (gdt_limit));
}

void
gdt_init(
    void
)
{
    extern void		unix_syscall_(void),
    			mach_kernel_trap_(void),
    			machdep_call_(void);
    
    map_code((code_desc_t *) sel_to_gdt_entry(KCS_SEL),
	     KERNEL_LINEAR_BASE,
	     (vm_size_t)(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS),
	     KERN_PRIV,
	     FALSE
	    );

    map_data((data_desc_t *) sel_to_gdt_entry(KDS_SEL),
	     KERNEL_LINEAR_BASE,
	     (vm_size_t)(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS),
	     KERN_PRIV,
	     FALSE
	    );

    map_code((code_desc_t *) sel_to_gdt_entry(LCODE_SEL),
	     (vm_offset_t) VM_MIN_ADDRESS,
	     (vm_size_t) (VM_MAX_ADDRESS - VM_MIN_ADDRESS),
	     KERN_PRIV,
	     FALSE
	    );

    map_data((data_desc_t *) sel_to_gdt_entry(LDATA_SEL),
	     (vm_offset_t) VM_MIN_ADDRESS,
	     (vm_size_t) (VM_MAX_ADDRESS - VM_MIN_ADDRESS),
	     KERN_PRIV,
	     FALSE
	    );

    map_code((code_desc_t *) sel_to_gdt_entry(UCODE_SEL),
	     (vm_offset_t) VM_MIN_ADDRESS,
	     (vm_size_t) (VM_MAX_ADDRESS - VM_MIN_ADDRESS),
	     USER_PRIV,
	     FALSE
	    );

    map_data((data_desc_t *) sel_to_gdt_entry(UDATA_SEL),
	     (vm_offset_t) VM_MIN_ADDRESS,
	     (vm_size_t) (VM_MAX_ADDRESS - VM_MIN_ADDRESS),
	     USER_PRIV,
	     FALSE
	    );

/* XXX This is needed for protected mode WINDOWS support */	    
    map_data((data_desc_t *) sel_to_gdt_entry(BIOSDATA_SEL),
	     (vm_offset_t) 0x400,
	     (vm_size_t) 0x300,
	     USER_PRIV,
	     FALSE
	    );
/* XXX */

    make_call_gate(sel_to_gdt_entry(SCALL_SEL),
				    KCS_SEL, unix_syscall_,
				    USER_PRIV,
				    1				/* argcnt */
		    );

    make_call_gate(sel_to_gdt_entry(MACHCALL_SEL),
				    KCS_SEL, mach_kernel_trap_,
				    USER_PRIV,
				    1				/* argcnt */
		    );

    make_call_gate(sel_to_gdt_entry(MDCALL_SEL),
				    KCS_SEL, machdep_call_,
				    USER_PRIV,
				    1				/* argcnt */
		    );

    locate_gdt((vm_offset_t) gdt);
}

static gdt_t	gdt_store[GDTSZ] = { 0 };

gdt_t		*gdt = gdt_store;
