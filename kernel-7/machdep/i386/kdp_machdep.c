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
 * Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved.
 *
 * kdp_machdep.c -- Machine-dependent code for Remote Debugging Protocol
 *
 * History:
 *
 * 10 Nov 1997	Herb Ruth [ruth1@apple.com]
 *	Added extern declarations for implicit function
 *	declarations. Changed dprintf macro to call kprintf()
 *	instead of safe_prf() when KDP_TEST_HARNESS==1.
 */
 
#import <sys/param.h>
#import <sys/systm.h>
#import <sys/mbuf.h>
#import <sys/socket.h>
#import <net/if.h>
#import <net/route.h>
#import <netinet/in.h>
#import <netinet/in_systm.h>
#import <netinet/ip.h>
#import <netinet/ip_var.h>
#import <netinet/in_pcb.h>

#import <mach/mach_types.h>

#import <kern/kdp_internal.h>
#import <kern/miniMon.h>

#import <machdep/i386/sel_inline.h>
#import <machdep/i386/intr_exported.h>

#import <bsd/sys/conf.h>

#define KDP_TEST_HARNESS 0
#if KDP_TEST_HARNESS
/*#define dprintf(x) safe_prf(x)*/
#define dprintf(x) kprintf x
#else
#define dprintf(x)
#endif

void
kdp_getstate(
    i386_thread_state_t		*state
)
{
    thread_saved_state_t	*saved_state;
    
    saved_state = (thread_saved_state_t *)kdp.saved_state;
    
    *state = (i386_thread_state_t) { 0 };	
    state->eax = saved_state->regs.eax;
    state->ebx = saved_state->regs.ebx;
    state->ecx = saved_state->regs.ecx;
    state->edx = saved_state->regs.edx;
    state->edi = saved_state->regs.edi;
    state->esi = saved_state->regs.esi;
    state->ebp = saved_state->regs.ebp;
    state->esp = (unsigned int)&saved_state->frame.esp;	/* XXX */
    state->ss = sel_to_selector(saved_state->frame.ss);
    state->eflags = saved_state->frame.eflags;
    state->eip = saved_state->frame.eip;
    state->cs = sel_to_selector(saved_state->frame.cs);
    state->ds = sel_to_selector(saved_state->regs.ds);
    state->es = sel_to_selector(saved_state->regs.es);
    state->fs = sel_to_selector(saved_state->regs.fs);
    state->gs = sel_to_selector(saved_state->regs.gs);
}


void
kdp_setstate(
    i386_thread_state_t		*state
)
{
    thread_saved_state_t	*saved_state;
    
    saved_state = (thread_saved_state_t *)kdp.saved_state;

    saved_state->regs.eax = state->eax;
    saved_state->regs.ebx = state->ebx;
    saved_state->regs.ecx = state->ecx;
    saved_state->regs.edx = state->edx;
    saved_state->regs.edi = state->edi;
    saved_state->regs.esi = state->esi;
    saved_state->regs.ebp = state->ebp;
    saved_state->frame.eflags = state->eflags;
#if	0
    saved_state->frame.eflags &= ~( EFL_VM | EFL_NT | EFL_IOPL | EFL_CLR );
    saved_state->frame.eflags |=  ( EFL_IF | EFL_SET );
#endif
    saved_state->frame.eip = state->eip;
    saved_state->regs.fs = selector_to_sel(state->fs);
    saved_state->regs.gs = selector_to_sel(state->gs);
}


void
kdp_exception(
    unsigned char	*pkt,
    int			*len,
    unsigned short	*remote_port,
    unsigned int	exception,
    unsigned int	code,
    unsigned int	subcode
)
{
    kdp_exception_t	*rq = (kdp_exception_t *)pkt;

    rq->hdr.request = KDP_EXCEPTION;
    rq->hdr.is_reply = 0;
    rq->hdr.seq = kdp.exception_seq;
    rq->hdr.key = 0;
    rq->hdr.len = sizeof (*rq);
    
    rq->n_exc_info = 1;
    rq->exc_info->cpu = 0;
    rq->exc_info->exception = exception;
    rq->exc_info->code = code;
    rq->exc_info->subcode = subcode;
    
    rq->hdr.len += rq->n_exc_info * sizeof (kdp_exc_info_t);
    
    kdp.exception_ack_needed = TRUE;
    
    *remote_port = kdp.exception_port;
    *len = rq->hdr.len;
}

void
kdp_exception_ack(
    unsigned char	*pkt,
    int			len
)
{
    kdp_exception_ack_t	*rq = (kdp_exception_ack_t *)pkt;

    if (len < sizeof (*rq))
	return;
	
    if (!rq->hdr.is_reply || rq->hdr.request != KDP_EXCEPTION)
    	return;
	
    dprintf(("kdp_exception_ack seq %x %x\n", rq->hdr.seq, kdp.exception_seq));
	
    if (rq->hdr.seq == kdp.exception_seq) {
	kdp.exception_ack_needed = FALSE;
	kdp.exception_seq++;
    }
}

kdp_error_t
kdp_machine_read_regs(
    unsigned int cpu,
    unsigned int flavor,
    char *data,
    int *size
)
{
    switch (flavor) {

    case i386_THREAD_STATE:

	dprintf(("kdp_readregs THREAD_STATE\n"));
	kdp_getstate((i386_thread_state_t *)data);
	*size = sizeof (i386_thread_state_t);
	return KDPERR_NO_ERROR;
	
    case i386_THREAD_FPSTATE:
	dprintf(("kdp_readregs THREAD_FPSTATE\n"));
	*(i386_thread_fpstate_t *)data = (i386_thread_fpstate_t) { 0 };	
	*size = sizeof (i386_thread_fpstate_t);
	return KDPERR_NO_ERROR;
	
    default:
	dprintf(("kdp_readregs bad flavor %d\n"));
	return KDPERR_BADFLAVOR;
    }
}

kdp_error_t
kdp_machine_write_regs(
    unsigned int cpu,
    unsigned int flavor,
    char *data,
    int *size
)
{
    switch (flavor) {

    case i386_THREAD_STATE:
	dprintf(("kdp_writeregs THREAD_STATE\n"));
	kdp_setstate((i386_thread_state_t *)data);
	return KDPERR_NO_ERROR;
	
    case i386_THREAD_FPSTATE:
	dprintf(("kdp_writeregs THREAD_FPSTATE\n"));
	return KDPERR_NO_ERROR;
	
    default:
	dprintf(("kdp_writeregs bad flavor %d\n"));
	return KDPERR_BADFLAVOR;
    }
}

void
kdp_machine_hostinfo(
    kdp_hostinfo_t *hostinfo
)
{
    hostinfo->cpus_mask = 1;
    hostinfo->cpu_type = CPU_TYPE_I386;
    hostinfo->cpu_subtype = CPU_SUBTYPE_486;
}

void
kdp_panic(
    const char		*msg
)
{
    safe_prf("kdp panic: %s\n", msg);
    
    asm volatile("hlt");
}


void
kdp_reboot(void)
{
    keyboard_reboot();
}

int
kdp_intr_disbl(void)
{
    return intr_disbl();
}

void
kdp_intr_enbl(int s)
{
    intr_enbl(s);
}

void
kdp_us_spin(int usec)
{
    us_spin(usec);
}

void
kdp_flush_cache(void)
{
    /* no-op on i386 */
}

