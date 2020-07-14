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

#include <sys/param.h>
#include <sys/systm.h>
#include <interrupts.h>
#include <powermac.h>
#include <kern/thread.h>
#include <kern/assert.h>
#include <kernserv/machine/spl.h>
#include <machdep/ppc/machspl.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/trap.h>
#include <machdep/ppc/exception.h>

#include <machine/setjmp.h>
#include <machine/label_t.h>

/* NMGS TODO this SPL stuff needs thinking out */

spl_t sploff(void)	{ return set_priority_level(SPLOFF); }
spl_t splhigh(void)	{ return set_priority_level(SPLHIGH); }
spl_t splsched(void)	{ return set_priority_level(SPLSCHED); }
spl_t splclock(void)	{ return set_priority_level(SPLCLOCK); }
spl_t splpower(void)	{ return set_priority_level(SPLPOWER); }
spl_t splvm(void)	{ return set_priority_level(SPLVM); }
spl_t splbio(void)	{ return set_priority_level(SPLBIO); }
spl_t spl3(void)	{ return set_priority_level(SPLBIO); }
spl_t splimp(void)	{ return set_priority_level(SPLIMP); }
spl_t spltty(void)	{ return set_priority_level(SPLTTY); }
spl_t splnet(void)	{ return set_priority_level(SPLNET); }
spl_t splsclk(void)	{ return set_priority_level(SPLSCLK); }

void  spllo(void)	{ (void) set_priority_level(SPLLO); }
void  splx(spl_t level) { (void) set_priority_level(level); }
void  splon(spl_t level){ (void) set_priority_level(level); }
void  spln(spl_t level) { (void) set_priority_level(level); }

spl_t spl0(void) { return set_priority_level(SPLLO); }
spl_t splsoftclock(void) { return set_priority_level(SPLSCLK); }
spl_t splstatclock(void) { return set_priority_level(SPLSCLK); }
spl_t splusclock(void) { return set_priority_level(SPLSCLK); }

spl_t current_priority = SPLHIGH;	/* MEB 11/2/95  */

#if DEBUG
vm_offset_t spl_addr;
#endif /* MACH_DEBUG */

void (*pmac_register_int)(int interrupt, spl_t level,
                     void (*handler)(int, void *, void *),
                     void *);

void (*pmac_interrupt)(int type, struct ppc_saved_state *ssp,
                    unsigned int dsisr, unsigned int dar);

unsigned int (*pmac_int_to_number)(int index);

boolean_t (*pmac_enable_irq)(int irq);
boolean_t (*pmac_disable_irq)(int irq);

// This is in identify_machine.c
extern void identify_via_irq(void);

/* 
 * set_priority_level implements spl()
 * The lower the number, the higher the priority. TODO NMGS change?? 
 */

#if MACH_DEBUG
#if DEBUG
struct {
	spl_t level;
	char* name;
} spl_names[] = {
	{ SPLHIGH,	"SPLHIGH" },
	{ SPLCLOCK,	"SPLCLOCK" },
	{ SPLPOWER,     "SPLPOWER" },
	{ SPLVM,	"SPLVM" },
	{ SPLBIO,	"SPLBIO" },
	{ SPLIMP,	"SPLIMP" },
	{ SPLTTY,	"SPLTTY" },
	{ SPLNET,	"SPLNET" },
	{ SPLSCLK,	"SPLSCLK" },
	{ SPLLO,	"SPLLO" },
};

#define SPL_NAMES_COUNT (sizeof (spl_names)/sizeof(spl_names[0]))

static char *spl_name(spl_t lvl);

/* A routine which, given the number of an spl, returns its name.
 * this uses a table set up in spl.h in order to make the association
 */
static char *spl_name(spl_t lvl)
{
	int i;
	for (i = 0; i < SPL_NAMES_COUNT; i++)
		if (spl_names[i].level == lvl)
			return spl_names[i].name;
	printf("UNKNOWN SPL %d",lvl);
	panic("");
	return ("UNKNOWN SPL");
}
#endif /* DEBUG */

#endif /* MACH_DEBUG */


/* The implementation of splxxx() */

spl_t set_priority_level(spl_t lvl)
{
	spl_t old_level;

/*	DPRINTF(("changing priority %s from %s to %s\n",
		 (lvl > current_priority) ? "DOWN" : "UP",
		 spl_name(current_priority),
		 spl_name(lvl)));
*/
	old_level = current_priority;

	/* MEB - 11/2/95 Interrupt Hacks */

	if (lvl == SPLLO) {
		current_priority = lvl;
		interrupt_enable();
	} else {
		interrupt_disable();
		current_priority = lvl;
	}


	return old_level;
}

static void quote_nmi_unquote_interrupt(int device, void *ssp, void * arg)
{
#if 0
        mini_mon("", "Kernel Debugger",ssp);
#else
	// Jumping straight in means the keyboard doesn't need
	// to be functional.
        kdp_raise_exception( 5, 0, 0, ssp);
#endif
}

void
initialize_interrupts(void)
{
	extern int	kdp_flag;

	interrupt_disable();

	(*(*powermac_init_p).machine_initialize_interrupt)();

	identify_via_irq();

	if (kdp_flag & 4)
            pmac_register_int(PMAC_DEV_NMI, SPLHIGH,
                        quote_nmi_unquote_interrupt, 0);

	current_priority = SPLLO;
	interrupt_enable();
}


struct ppc_saved_state * interrupt(
        int type,
        struct ppc_saved_state *ssp,
	unsigned int dsisr,
	unsigned int dar)
{
	int irq;
	spl_t		old_spl = current_priority;
	thread_t th;

	current_priority = SPLHIGH;

#ifdef notdef_next
#if DEBUG
	{
		/* make sure we're not near to overflowing intstack */
		unsigned int sp;
		extern unsigned int intstack;
		static unsigned int spmin = 0xFFFFFFFF;
		__asm__ volatile("mr	%0, 1" : "=r" (sp));
		if (sp < (intstack + PPC_PGBYTES)) {
			printf("INTERRUPT - LOW ON STACK!\n");
			call_kgdb_with_ctx(type, 0, ssp);
		}
#if 0
		if (sp < spmin) {
			printf("INTERRUPT - NEW LOW WATER 0x%08x\n",sp);
			spmin = sp;
		}
#endif /* 0 */
	}
#endif /* DEBUG */
#if DEBUG
	if (old_spl != SPLLO) {
		kgdb_printf("{Raised SPL=%s, iptr = 0x%08x, spl was from 0x%08x??}", spl_name(old_spl), ssp->srr0, spl_addr);
	}
#endif
#endif /* notdef_next */
	switch (type) {
	case EXC_DECREMENTER:
		rtclock_intr(0,ssp);  /* Hardcoded on decrementer interrupt */
		break;

#if DEBUG
	case EXC_PROGRAM:
		if ((ssp->srr1 & MASK(SRR1_PRG_TRAP)) == 0) {
			goto illegal;
		}
		/* drop through to raise_exception */

	case EXC_TRACE:
		enter_debugger(type, dsisr, dar, ssp, 0);
		break;
#endif /* DEBUG */

	case EXC_DATA_ACCESS:
#if DEBUG
		if (dsisr & MASK(DSISR_WATCH)) {
			printf("dabr ");
			enter_debugger(type, dsisr,dar,ssp, 1);
			break;
		}
#endif /* DEBUG */
		/*
		 * this is here so that copy_for_kdp can fault
		 * in debug form it ends up here because
		 * thandler vectors to ihandler.
		 */
		th = current_thread();
		if (th && th->recover) {
			label_t *l = (label_t *)th->recover;
			th->recover = (vm_offset_t)NULL;
			longjmp(l,1);
		} else {
			goto illegal;
		}
#if DEBUG
		goto illegal;
#else
		break;
#endif

	case EXC_INTERRUPT:
		/* Call the pmac interrupt routine */
		(*pmac_interrupt)(type, ssp, dsisr, dar);
		break;

	default:
#ifdef DEBUG
	illegal:
		printf("dsisr=0x%x, dar=0x%x\n", dsisr, dar);
		enter_debugger(type, dsisr, dar, ssp, 1);
#endif /* DEBUG */
		break;
	}

	current_priority = old_spl;
	return ssp;
}

int
curipl(void)
{
  return current_priority;
}
int
ipltospl(
    int		ipl
)
{
    return (ipl);
}
