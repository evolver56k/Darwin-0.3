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

#import <kern/thread.h>

#import <machdep/i386/bios.h>
#import <machdep/i386/seg.h>

/*
 * Maintain the recover field in the thread_t structure
 */
static inline void
set_recover(int (*vector)())
{
    current_thread()->recover = (vm_offset_t)vector;
    return;
}

static inline void
clear_recover()
{
    current_thread()->recover = 0;
}



int volatile
bios32(biosBuf_t *bb)
{
    static unsigned long 	ebp;
    static unsigned long 	ss;
    static boolean_t 		initialized;
    static lock_t 		lock;
    
#if	NOTYET			// this is failing on the NEC right now
    if (!initialized) {
	lock_init(lock, TRUE);
	initialized = TRUE;
    }
    
    lock_write(lock);
    set_recover(&&do_fault);

    /* Save stack pointer. */
    asm volatile("mov %%ebp, %0"
    		 : "=m" (ebp)
		 : /* no inputs */);
    asm volatile("mov %%ss, %0"
    		 : "=m" (ss)
		 : /* no inputs */);
#endif	NOTYET
    _bios32(bb);


#if	NOTYET
    /* Reload DS, ES, FS, GS and SS. */
    asm volatile("movw %0, %%ax;"
                 "mov %%ax, %%ds; mov %%ax, %%es;"
		 "mov %1, %%ax; mov %%ax, %%fs;"
		 "mov %2, %%ax; mov %%ax, %%gs;"
    		 : /* no outputs */
		 : "i" (KDS_SEL), "i" (LDATA_SEL), "i" (0));
    asm volatile("mov %0, %%ss"
    		 : /* no inputs */
		 : "m" (ss));
    /* Restore stack pointer. */
    asm volatile("mov %0, %%ebp"
    		 : /* no outputs */
		 : "m" (ebp));
    clear_recover();
    lock_done(lock);
#endif	NOTYET
    return 0;
#if	NOTYET
do_fault:
    /* Reload DS, ES, FS, GS and SS. */
    asm volatile("movw %0, %%ax;"
                 "mov %%ax, %%ds; mov %%ax, %%es;"
		 "mov %1, %%ax; mov %%ax, %%fs;"
		 "mov %2, %%ax; mov %%ax, %%gs;"
    		 : /* no outputs */
		 : "i" (KDS_SEL), "i" (LDATA_SEL), "i" (0));
    asm volatile("mov %0, %%ss"
    		 : /* no inputs */
		 : "m" (ss));
    /* Restore stack pointer. */
    asm volatile("mov %0, %%ebp"
    		 : /* no outputs */
		 : "m" (ebp));
    clear_recover();
    lock_done(lock);
    return -1;
#endif	NOTYET
}
