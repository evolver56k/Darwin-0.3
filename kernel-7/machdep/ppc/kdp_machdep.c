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
 * Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 1994 NeXT Computer, Inc.  All rights reserved.
 *
 * machdep/ppc/kdp_machdep.c
 *
 * Machine-dependent code for Remote Debugging Protocol
 *
 * History:
 * March, 1997	Umesh Vaishampayan [umeshv@NeXT.com]
 *		Created.
 * 03 Nov 1997	Herb Ruth [ruth1@apple.com]
 *	Added extern declarations for implicit function
 *	declarations. Changed dprintf macro to call kprintf()
 *	instead of kdp_printf() when KDP_TEST_HARNESS==1
 *	Added sync and isync ppc instructions to kdp_flush_cache().
 *
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
#import <bsd/sys/conf.h>
#import <bsd/sys/reboot.h>
#include <stdarg.h>
#import <sys/subr_prf.h>
#include <mach/exception.h>

#define KDP_TEST_HARNESS 0
#if KDP_TEST_HARNESS
/*#define dprintf(x) kdp_printf x*/
#define dprintf(x) kprintf x
#else
#define dprintf(x)
#endif

/* 03 Nov 1997 */
#warning extern declarations -- FIXME  XXX

extern int cnputc(char c);
extern void kprintf(const char *format, ...);
extern boot(int paniced, int howto, char *command);

extern unsigned splhigh(void);
extern void splx(unsigned level);
extern void us_spin(int usec);

void
kdp_printf(
    const char		*format,
    ...
)
{
    va_list		ap;
    static char		rdpPrintfBuf[512];
    char		*p = rdpPrintfBuf;

    va_start(ap, format);
    prf(format, ap, TOSTR, (struct tty *)&p);
    va_end(ap);
    *p++ = '\0';

    p = rdpPrintfBuf;
    while (*p)
   		cnputc(*p++);
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
    struct {
    	kdp_exception_t	pkt;
	kdp_exc_info_t	exc;
    } aligned_pkt;

    kdp_exception_t	*rq = (kdp_exception_t *)&aligned_pkt;

    bcopy(pkt, rq, sizeof(*rq));
    rq->hdr.request = KDP_EXCEPTION;
    rq->hdr.is_reply = 0;
    rq->hdr.seq = kdp.exception_seq;
    rq->hdr.key = 0;
    rq->hdr.len = sizeof (*rq) + sizeof(kdp_exc_info_t);
    
    rq->n_exc_info = 1;
    rq->exc_info[0].cpu = 0;
    rq->exc_info[0].exception = exception;
    rq->exc_info[0].code = code;
    rq->exc_info[0].subcode = subcode;
    
    rq->hdr.len += rq->n_exc_info * sizeof (kdp_exc_info_t);
    
    bcopy(rq, pkt, rq->hdr.len);

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
    kdp_exception_ack_t	aligned_pkt;
    kdp_exception_ack_t	*rq = (kdp_exception_ack_t *)&aligned_pkt;

    if (len < sizeof (*rq))
	return;
	
    bcopy(pkt, rq, sizeof(*rq));

    if (!rq->hdr.is_reply || rq->hdr.request != KDP_EXCEPTION)
    	return;
	
    dprintf(("kdp_exception_ack: seq 0x%08x 0x%08x\n",
				rq->hdr.seq, kdp.exception_seq));
	
    if (rq->hdr.seq == kdp.exception_seq) {
	kdp.exception_ack_needed = FALSE;
	kdp.exception_seq++;
    }
}

static void
kdp_getintegerstate(
    struct ppc_thread_state		*state
)
{
    struct ppc_thread_state	*saved_state;
   
    saved_state = kdp.saved_state;
   
    *state = (struct ppc_thread_state) { 0 };

    state->srr0  = saved_state->srr0;
    state->srr1  = saved_state->srr1;
    state->r0  = saved_state->r0;
    state->r1  = saved_state->r1;
    state->r2  = saved_state->r2;
    state->r3  = saved_state->r3;
    state->r4  = saved_state->r4;
    state->r5  = saved_state->r5;
    state->r6  = saved_state->r6;
    state->r7  = saved_state->r7;
    state->r8  = saved_state->r8;
    state->r9  = saved_state->r9;
    state->r10  = saved_state->r10;
    state->r11  = saved_state->r11;
    state->r12  = saved_state->r12;
    state->r13  = saved_state->r13;
    state->r14  = saved_state->r14;
    state->r15  = saved_state->r15;
    state->r16  = saved_state->r16;
    state->r17  = saved_state->r17;
    state->r18  = saved_state->r18;
    state->r19  = saved_state->r19;
    state->r20  = saved_state->r20;
    state->r21  = saved_state->r21;
    state->r22  = saved_state->r22;
    state->r23  = saved_state->r23;
    state->r24  = saved_state->r24;
    state->r25  = saved_state->r25;
    state->r26  = saved_state->r26;
    state->r27  = saved_state->r27;
    state->r28  = saved_state->r28;
    state->r29  = saved_state->r29;
    state->r30  = saved_state->r30;
    state->r31  = saved_state->r31;
    state->cr  = saved_state->cr;
    state->xer  = saved_state->xer;
    state->lr  = saved_state->lr;
    state->ctr  = saved_state->ctr;
    state->mq  = saved_state->mq; /* This is BOGUS ! (601) ONLY */
    state->pad  = saved_state->pad;
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

    case PPC_THREAD_STATE:
//		dprintf(("kdp_machine_read_regs: THREAD_STATE\n"));
		dprintf(("kdp_machine_read_regs\n"));
		kdp_getintegerstate((struct ppc_thread_state *)data);
		*size = sizeof (struct ppc_thread_state);
		return KDPERR_NO_ERROR;
	
    case PPC_FLOAT_STATE:
		dprintf(("kdp_machine_read_regs: THREAD_FPSTATE\n"));
		*(struct ppc_float_state *)data = (struct ppc_float_state) { {0.0}, 0, 0 };	
		*size = sizeof (struct ppc_float_state);
		return KDPERR_NO_ERROR;
	
    default:
		dprintf(("kdp_machine_read_regs: bad flavor %d\n"));
		return KDPERR_BADFLAVOR;
    }
}

static void
kdp_setintegerstate(
    struct ppc_thread_state		*state
)
{
    struct ppc_thread_state	*saved_state;
   
    saved_state = kdp.saved_state;

    saved_state->srr0 = state->srr0  ;
    saved_state->srr1 = state->srr1  ;
    saved_state->r0 = state->r0  ;
    saved_state->r1 = state->r1  ;
    saved_state->r2 = state->r2  ;
    saved_state->r3 = state->r3  ;
    saved_state->r4 = state->r4  ;
    saved_state->r5 = state->r5  ;
    saved_state->r6 = state->r6  ;
    saved_state->r7 = state->r7  ;
    saved_state->r8 = state->r8  ;
    saved_state->r9 = state->r9  ;
    saved_state->r10 = state->r10  ;
    saved_state->r11 = state->r11  ;
    saved_state->r12 = state->r12  ;
    saved_state->r13 = state->r13  ;
    saved_state->r14 = state->r14  ;
    saved_state->r15 = state->r15  ;
    saved_state->r16 = state->r16  ;
    saved_state->r17 = state->r17  ;
    saved_state->r18 = state->r18  ;
    saved_state->r19 = state->r19  ;
    saved_state->r20 = state->r20  ;
    saved_state->r21 = state->r21  ;
    saved_state->r22 = state->r22  ;
    saved_state->r23 = state->r23  ;
    saved_state->r24 = state->r24  ;
    saved_state->r25 = state->r25  ;
    saved_state->r26 = state->r26  ;
    saved_state->r27 = state->r27  ;
    saved_state->r28 = state->r28  ;
    saved_state->r29 = state->r29  ;
    saved_state->r30 = state->r30  ;
    saved_state->r31 = state->r31  ;
    saved_state->cr = state->cr  ;
    saved_state->xer = state->xer  ;
    saved_state->lr = state->lr  ;
    saved_state->ctr = state->ctr  ;
    saved_state->mq = state->mq  ; /* BOGUS! (601)ONLY */
    saved_state->pad = state->pad  ;
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

    case PPC_THREAD_STATE:
//		dprintf(("kdp_machine_write_regs: THREAD_STATE\n"));
		dprintf(("kdp_machine_write_regs\n"));
		kdp_setintegerstate((struct ppc_thread_state *)data);
		return KDPERR_NO_ERROR;
	
    case PPC_FLOAT_STATE:
		dprintf(("kdp_machine_write_regs: THREAD_FPSTATE\n"));
		return KDPERR_NO_ERROR;
	
    default:
		dprintf(("kdp_machine_write_regs: bad flavor %d\n"));
		return KDPERR_BADFLAVOR;
    }
}

void
kdp_machine_hostinfo(
    kdp_hostinfo_t *hostinfo
)
{
    machine_slot_t	m;
    int			i;

    hostinfo->cpus_mask = 0;

    for (i = 0; i < machine_info.max_cpus; i++) {
        m = &machine_slot[i];
        if (!m->is_cpu)
            continue;
	
        hostinfo->cpus_mask |= (1 << i);
        if (hostinfo->cpu_type == 0) {
            hostinfo->cpu_type = m->cpu_type;
            hostinfo->cpu_subtype = m->cpu_subtype;
        }
    }
}

void
kdp_panic(
    const char		*msg
)
{
    dprintf(("kdp_panic: exception, code & subcode not passed to safe_prf.\n"));
    safe_prf("kdp panic: %s\n", msg);
    while(1) {}
}


void
kdp_reboot(void)
{
    boot(RB_BOOT, RB_AUTOBOOT | RB_NOSYNC, "");
}

int
kdp_intr_disbl(void)
{
    return splhigh();
}

void
kdp_intr_enbl(int s)
{
    splx(s);
}

void
kdp_us_spin(int usec)
{
    us_spin(usec);
}

void print_saved_state(void *state)
{
    struct ppc_thread_state	*saved_state;

    saved_state = state;

	safe_prf("pc = 0x%x\n", saved_state->srr0);
	safe_prf("msr = 0x%x\n", saved_state->srr1);
	safe_prf("rp = 0x%x\n", saved_state->lr);
	safe_prf("sp = 0x%x\n", saved_state->r1);
}

void
kdp_flush_cache(void)
{
    __asm__ volatile ("sync");
    __asm__ volatile ("isync");
}

#if 1
void
kdp_init_integerstate(struct ppc_thread_state *instate)
{
	__asm__ volatile (".set SS_R0,8");
	__asm__ volatile (".set SS_R1,12");
	__asm__ volatile (".set SS_R2,16");
	__asm__ volatile (".set SS_R3,20");
	__asm__ volatile (".set SS_R4,24");
	__asm__ volatile (".set SS_R5,28");
	__asm__ volatile (".set SS_R6,32");
	__asm__ volatile (".set SS_R7,36");
	__asm__ volatile (".set SS_R8,40");
	__asm__ volatile (".set SS_R9,44");
	__asm__ volatile (".set SS_R10,48");
	__asm__ volatile (".set SS_R11,52");
	__asm__ volatile (".set SS_R12,56");
	__asm__ volatile (".set SS_R13,60");
	__asm__ volatile (".set SS_R14,64");
	__asm__ volatile (".set SS_R15,68");
	__asm__ volatile (".set SS_R16,72");
	__asm__ volatile (".set SS_R17,76");
	__asm__ volatile (".set SS_R18,80");
	__asm__ volatile (".set SS_R19,84");
	__asm__ volatile (".set SS_R20,88");
	__asm__ volatile (".set SS_R21,92");
	__asm__ volatile (".set SS_R22,96");
	__asm__ volatile (".set SS_R23,100");
	__asm__ volatile (".set SS_R24,104");
	__asm__ volatile (".set SS_R25,108");
	__asm__ volatile (".set SS_R26,112");
	__asm__ volatile (".set SS_R27,116");
	__asm__ volatile (".set SS_R28,120");
	__asm__ volatile (".set SS_R29,124");
	__asm__ volatile (".set SS_R30,128");
	__asm__ volatile (".set SS_R31,132");
	__asm__ volatile (".set SS_CR,136");
	__asm__ volatile (".set SS_XER,140");
	__asm__ volatile (".set SS_LR,144");
	__asm__ volatile (".set SS_CTR,148");
	__asm__ volatile (".set SS_SRR0,0");
	__asm__ volatile (".set SS_SRR1,4");

	__asm__ volatile("stw	r0,	SS_R0(r3)");
	__asm__ volatile("stw	r5,	SS_R5(r3)");

	__asm__ volatile("addis r5, 0, hi16(L_kdbmarker)");
	__asm__ volatile("ori r5, r5, lo16(L_kdbmarker)");

	__asm__ volatile("lwz	r0,	0(r1)");
	__asm__ volatile("stw	r0,	SS_R1(r3)");
	__asm__ volatile(".noflag_reg 2");
	__asm__ volatile("stw	r2,	SS_R2(r3)");
	__asm__ volatile(".flag_reg 2");
	__asm__ volatile("stw	r3,	SS_R3(r3)");
	__asm__ volatile("stw	r4,	SS_R4(r3)");
	__asm__ volatile("stw	r6,	SS_R6(r3)");
	__asm__ volatile("stw	r7,	SS_R7(r3)");
	__asm__ volatile("stw	r8,	SS_R8(r3)");
	__asm__ volatile("stw	r9,	SS_R9(r3)");
	__asm__ volatile("stw	r10,	SS_R10(r3)");
	__asm__ volatile("stw	r11,	SS_R11(r3)");
	__asm__ volatile("stw	r12,	SS_R12(r3)");
	__asm__ volatile(".noflag_reg 13");
	__asm__ volatile("stw	r13,	SS_R13(r3)");
	__asm__ volatile(".flag_reg 13");
	__asm__ volatile("stw	r14,	SS_R14(r3)");
	__asm__ volatile("stw	r15,	SS_R15(r3)");
	__asm__ volatile("stw	r16,	SS_R16(r3)");
	__asm__ volatile("stw	r17,	SS_R17(r3)");
	__asm__ volatile("stw	r18,	SS_R18(r3)");
	__asm__ volatile("stw	r19,	SS_R19(r3)");
	__asm__ volatile("stw	r20,	SS_R20(r3)");
	__asm__ volatile("stw	r21,	SS_R21(r3)");
	__asm__ volatile("stw	r22,	SS_R22(r3)");
	__asm__ volatile("stw	r23,	SS_R23(r3)");
	__asm__ volatile("stw	r24,	SS_R24(r3)");
	__asm__ volatile("stw	r25,	SS_R25(r3)");
	__asm__ volatile("stw	r26,	SS_R26(r3)");
	__asm__ volatile("stw	r27,	SS_R27(r3)");
	__asm__ volatile("stw	r28,	SS_R28(r3)");
	__asm__ volatile("stw	r29,	SS_R29(r3)");
	__asm__ volatile("stw	r30,	SS_R30(r3)");
	__asm__ volatile("stw	r31,	SS_R31(r3)");

	__asm__ volatile("mfcr	r0");
	__asm__ volatile("stw	r0,	SS_CR(r3)");

	__asm__ volatile("stw	r5,	SS_SRR0(r3)");

//	__asm__ volatile("mfsrr0	r0");
//	__asm__ volatile("stw	r0,	SS_SRR0(r3)");
	__asm__ volatile("mfsrr1	r0");
	__asm__ volatile("stw	r0,	SS_SRR1(r3)");
	__asm__ volatile("mfxer	r0");
	__asm__ volatile("stw	r0,	SS_XER(r3)");
	__asm__ volatile("mflr	r0");
	__asm__ volatile("stw	r0,	SS_LR(r3)");
	__asm__ volatile("mfctr	r0");
	__asm__ volatile("stw	r0,	SS_CTR(r3)");

}


void
call_kdp()
{
	static struct ppc_thread_state __kdp_space; 
	kdp_init_integerstate(&__kdp_space);
	kdp_raise_exception(EXC_SOFTWARE, 0, 0, &__kdp_space);

	__asm__ volatile("L_kdbmarker:");
	__asm__ volatile("nop");

}
#endif

/*
 * table to convert system specific code to generic codes for kdb
 */
int kdp_trap_codes[] = {
	EXC_BAD_ACCESS,	/* 0x0000  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x0100  System reset */
	EXC_BAD_ACCESS,	/* 0x0200  Machine check */
	EXC_BAD_ACCESS,	/* 0x0300  Data access */
	EXC_BAD_ACCESS,	/* 0x0400  Instruction access */
	EXC_BAD_ACCESS,	/* 0x0500  External interrupt */
	EXC_BAD_ACCESS,	/* 0x0600  Alignment */
	EXC_BREAKPOINT,	/* 0x0700  Program - fp exc, ill/priv instr, trap */
	EXC_ARITHMETIC,	/* 0x0800  Floating point disabled */
	EXC_SOFTWARE,	/* 0x0900  Decrementer */
	EXC_BAD_ACCESS,	/* 0x0A00  I/O controller interface */
	EXC_BAD_ACCESS,	/* 0x0B00  INVALID EXCEPTION */
	EXC_SOFTWARE,	/* 0x0C00  System call exception */
	EXC_BREAKPOINT,	/* 0x0D00  Trace */
	EXC_SOFTWARE,	/* 0x0E00  FP assist */
	EXC_SOFTWARE,	/* 0x0F00  Performance monitoring */
	EXC_BAD_ACCESS,	/* 0x1000  Instruction PTE miss */
	EXC_BAD_ACCESS,	/* 0x1100  Data load PTE miss */
	EXC_BAD_ACCESS,	/* 0x1200  Data store PTE miss */
	EXC_BREAKPOINT,	/* 0x1300  Instruction bkpt */
	EXC_SOFTWARE,	/* 0x1400  System management */
	EXC_BAD_ACCESS,	/* 0x1500  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1600  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1700  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1800  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1900  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1A00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1B00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1C00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1D00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1E00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x1F00  INVALID EXCEPTION */
	EXC_BREAKPOINT,	/* 0x2000  Run Mode/Trace */
	EXC_BAD_ACCESS,	/* 0x2100  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2200  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2300  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2400  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2500  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2600  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2700  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2800  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2900  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2A00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2B00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2C00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2D00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2E00  INVALID EXCEPTION */
	EXC_BAD_ACCESS,	/* 0x2F00  INVALID EXCEPTION */
	EXC_SOFTWARE	/* 0x3000  AST trap (software) */
};

int kdp_segments[16] = {
	 -1,  -1,   0,   3,
	  4,   5, 0xB,  -1,
	 -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1
};

int kdp_space;

int
kdp_map_segment(unsigned int segment)
{
	segment &= 0xF;
	return kdp_segments[segment];
}
