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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 * HISTORY
 *
 *	umeshv@apple.com
 *	Fixed sleep integration problem. sleep was not properly
 *	handling thread states of THREAD_INTERRUPTED and 
 *	THREAD_MUST_TERMINATE, so callers of sleep were getting
 *	confused and many times looping.  This fixes the (in)famous
 *	unkillable gdb problem, the PB (and other processes) don't
 *	terminate, and more. Cleaned up ast code to be just like
 *  i386 version.
 *
 */

#import <mach/mach_types.h>
#import <mach/exception.h>

#import <sys/param.h>
#import <sys/proc.h>
#import <sys/user.h>
#include <kern/ast.h>


#ifdef	MACHINE_AST
void
astoff(int mycpu)
{
	printf("astoff: turning off needast, cpu=%x\n",mycpu);
}



void
aston(int mycpu)
{
	printf("aston: setting needast, cpu=%x\n",mycpu);
}
#endif



void
check_for_ast(void)
{
    thread_t    thread;
    struct proc	*p;
    ppc_saved_state_t	*saved_state;


    thread = current_thread();
    p = current_proc();
    saved_state = &thread->pcb->ss;

    while (TRUE) {
	ast_t	reasons;
	int	mycpu = cpu_number();
	int	signum;
	
	reasons = ast_needed(mycpu);

	if (p) {
	    if ((p->p_flag & P_OWEUPC) && (p->p_flag & P_PROFIL)) {
	    		addupc_task(p, saved_state->srr0, 1);
	    		p->p_flag &= ~P_OWEUPC;
		}

	    ast_off(mycpu, AST_UNIX);

	    if (CHECK_SIGNALS(p, thread, thread->_uthread)) {
			signum = issignal(p);
			if (signum) {
		    	postsig(signum);
			}
	    }
	}

	ast_off(mycpu, reasons);

	if (thread_should_halt(thread))
	    		thread_halt_self();
	else if ((reasons & AST_BLOCK) || 
		csw_needed(thread, current_processor())) {
		p->p_stats->p_ru.ru_nivcsw++;
   		thread_block_with_continuation(thread_exception_return);
	}
	else
   		break;
    }
	
}
