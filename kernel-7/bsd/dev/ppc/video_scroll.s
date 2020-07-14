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
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MKLINUX-1.0DR2
 */

/* Routines to perform high-speed scrolling, assuming that the memory is
 * non-cached, and that the amount of memory to be scrolled is a multiple
 * of (at least) 16.
 */

#if 1 //notdef_next
#include <ppc/asm.h>
#include <ppc/proc_reg.h>

/*
 * void video_scroll_up(unsigned long start,
 *              unsigned long end,
 *              unsigned long dest)
 */

ENTRY(video_scroll_up, TAG_NO_FRAME_USED)
    
    /*
     * Turn the FPU on, save regs
     */
    mfmsr	ARG4
    ori	ARG3,	ARG4,	MASK(MSR_FP)	/* bit is in low-order 16 */
    mtmsr	ARG3
    isync

    stfd	f0, -16(r1)
    stfd	f1, -8(r1)

    /* ok, now we can use the FPU registers to do some fast copying,
     * maybe this can be more optimal but it's not bad
     */

.L_vscr_up_loop:
    lfd f0, 0(ARG0)
    lfd f1, 8(ARG0)

    addi    ARG0,   ARG0,   16
    
    stfd    f0, 0(ARG2)

    cmp    CR0,0,    ARG0,   ARG1

    stfd    f1, 8(ARG2)

    addi    ARG2,   ARG2,   16

    blt+    CR0,    .L_vscr_up_loop

    lfd		f0, -16(r1)
    lfd		f1, -8(r1)

    /* re-disable the FPU */
    sync
    mtmsr   ARG4
    isync
    
    blr

/*
 * void video_scroll_down(unsigned long start,   HIGH address to scroll from
 *                unsigned long end,     LOW address 
 *                unsigned long dest)    HIGH address
 */

ENTRY(video_scroll_down, TAG_NO_FRAME_USED)

    /*
     * Turn the FPU on, save regs
     */
    mfmsr	ARG4
    ori	ARG3,	ARG4,	MASK(MSR_FP)	/* bit is in low-order 16 */
    mtmsr	ARG3
    isync

    stfd	f0, -16(r1)
    stfd	f1, -8(r1)

    /* ok, now we can use the FPU registers to do some fast copying,
     * maybe this can be more optimal but it's not bad
     */

.L_vscr_down_loop:
    lfd f0, -16(ARG0)
    lfd f1, -8(ARG0)

    subi    ARG0,   ARG0,   16
    
    stfd    f0, -16(ARG2)

    cmp    CR0,0,    ARG0,   ARG1

    stfd    f1, -8(ARG2)

    subi    ARG2,   ARG2,   16

    bgt+    CR0,    .L_vscr_down_loop

    lfd		f0, -16(r1)
    lfd		f1, -8(r1)

    /* re-disable the FPU */
    sync
    mtmsr   ARG4
    isync
    
    blr
#endif notdef_next

