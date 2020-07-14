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

#include <mach/boolean.h>

#ifndef NULL
#define	NULL	((void *) 0)	/* lion@apple.com 2/12/97 */
#endif

//#include <mach_kgdb.h>
//#include <debug.h>
//#include <mach_debug.h>
//#include <kern/misc_protos.h>
//#include <kern/assert.h>
//
//#include <kgdb/gdb_defs.h>
//#include <kgdb/kgdb_defs.h>	/* For kgdb_printf */

//#include <machdep/ppc/spl.h>
#include <machdep/ppc/proc_reg.h>
//#include <machdep/ppc/misc_protos.h>
//#include <machdep/ppc/trap.h>
//#include <machdep/ppc/exception.h>
#include <powermac.h>
#include <interrupts.h>
#include <chips/mpic.h>

/* Prototypes */

static int
mpic_find_entry(int device, struct powermac_interrupt **handler, int nentries);

static unsigned int
mpic_int_to_number(int index);

static void
mpic_register_int(int device, spl_t level,
		  void (*handler)(int, void *, void *), void *arg);

static spl_t
mpic_set_priority_level(spl_t lvl);

static boolean_t
mpic_enable_irq(int irq);

static boolean_t
mpic_disable_irq(int irq);

static void
mpic_interrupt(int type, struct ppc_saved_state *ssp,
               unsigned int dsisr, unsigned int dar);

void mpic_via1_interrupt(int device, void *ssp, void *arg);

/* Storage for interrupt table pointers */
struct powermac_interrupt *mpic_interrupts;
struct powermac_interrupt *mpic_via1_interrupts;
u_long *mpic_int_mapping_tbl;
int *mpic_spl_to_pri;
int nmpic_via_interrupts;
int nmpic_interrupts;

/* Reset the hardware interrupt control */
void
mpic_interrupt_initialize(void)
{
        int    cnt, cnt2;
	u_long tmp;

//kprintf("mpic_interrupt_initialize: Entering.\n");

	pmac_int_to_number      = mpic_int_to_number;
	pmac_register_int       = mpic_register_int;
//	pmac_set_priority_level = mpic_set_priority_level;
	pmac_enable_irq         = mpic_enable_irq;
	pmac_disable_irq        = mpic_disable_irq;
	pmac_interrupt          = mpic_interrupt;

	*FM_MPIC_CTRL |= FM_MPIC_ENABLE;   /* Allow access to MPIC regs */
	eieio();


//tmp = lwbrx(0xf2041000);
//kprintf("mpic_interrupt_initialize: Features: 0x%x.\n", tmp);

  tmp = lwbrx(0xf2041080);
//kprintf("mpic_interrupt_initialize: Features: 0x%x.\n", tmp);

	*MPIC_GLOBAL_CFG |= MPIC_CASCADE;  /* Turn on 8259 Cascade mode. */
	eieio();

	/* Mask all MPIC Interrupts */
	stwbrx(0xf, MPIC_P0_CUR_TSK_PRI);
	eieio();

//kprintf("mpic_interrupt_initialize: Setting Int Priv addr: 0x%x, data: %d.\n", MPIC_P0_CUR_TSK_PRI, 0xf);

	/* Load the Interrupt Table */
	for (cnt = 0; cnt < nmpic_interrupts; cnt++) {

	  /* Set the Vector/Priority Reg */
	  tmp = mpic_int_mapping_tbl[cnt * 2];
	  stwbrx(tmp, MPIC_INT_CFG + cnt * 0x20);

	  /* Set the Destination Reg */
	  tmp = mpic_int_mapping_tbl[cnt * 2 + 1];
	  stwbrx(tmp, MPIC_INT_CFG + cnt * 0x20 + 0x10);
	}
	eieio();

	/* Dissable all of the Timer Interrupts */
	stwbrx(MASKED, MPIC_TMR_VEC_PRI + MPIC_TMR_OFFSET * 0);
	stwbrx(MASKED, MPIC_TMR_VEC_PRI + MPIC_TMR_OFFSET * 1);
	stwbrx(MASKED, MPIC_TMR_VEC_PRI + MPIC_TMR_OFFSET * 2);
	stwbrx(MASKED, MPIC_TMR_VEC_PRI + MPIC_TMR_OFFSET * 3);
	eieio();

	/* Dissable all of the IPI Interrupts */
	stwbrx(MASKED, MPIC_IPI_VEC_PRI + 0x10 * 0);
	stwbrx(MASKED, MPIC_IPI_VEC_PRI + 0x10 * 1);
	stwbrx(MASKED, MPIC_IPI_VEC_PRI + 0x10 * 2);
	stwbrx(MASKED, MPIC_IPI_VEC_PRI + 0x10 * 3);
	eieio();

	/* Set the Spurious Interrupt Vector to 0x31? */
	stwbrx(0x31, MPIC_SPUR_INT_VEC);
	eieio();

	/* Unmask all MPIC Interrupts */
	stwbrx(0x0, MPIC_P0_CUR_TSK_PRI);
	eieio();

//kprintf("mpic_interrupt_initialize: Setting Int Priv addr: 0x%x, data: %d.\n", MPIC_P0_CUR_TSK_PRI, 0x0);

	*FM_MPIC_CTRL |= FM_MPIC_INT_SEL;   /* Turn on Interrupts from MPIC */
	eieio();

	/* Clear Interrupts on MPIC */
	for (cnt = 0; cnt < nmpic_interrupts; cnt++) {
	  
	  /* Unmask the interrupt */
	  tmp = lwbrx(MPIC_INT_CFG + cnt * 0x20);
	  tmp &= ~MASKED;
	  stwbrx(tmp, MPIC_INT_CFG + cnt * 0x20);
	  eieio();

	  /* Wait for this interrupt to activate */
	  for (cnt2 = 0; cnt2 < 0x80; cnt2++) {
	    tmp = lwbrx(MPIC_INT_CFG + cnt * 0x20);
	    if (tmp & ACTIVE) break;
	  }

	  /* Clear it if it activates */
	  if (tmp & ACTIVE) {

	    /* Ack the interrupt then end it */
	    tmp = lwbrx(MPIC_P0_INT_ACK);
	    stwbrx(0, MPIC_P0_EOI);
	    
	    /* Waste some time? */
	    for (cnt2 = 0; cnt2 < 0x40; cnt2++) {
	      tmp = lwbrx(MPIC_INT_CFG +  cnt * 0x20);
	    }
	  }

	  /* Set the interrupt as masked again. */
	  tmp = lwbrx(MPIC_INT_CFG + cnt * 0x20);
	  tmp |= MASKED;
	  
#if 0
	  if (cnt == 25) tmp &= ~MASKED; /* Allow NMI */
#endif
	  stwbrx(tmp, MPIC_INT_CFG + cnt * 0x20);
	}

	/* Clear Interrupts on VIA1 */
	*(v_u_char *)PCI_VIA1_PCR               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IER               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IFR               = 0x7f; eieio();

//kprintf("mpic_interrupt_initialize: Leaving.\n");

}

static int
mpic_find_entry(int device,
	       struct powermac_interrupt **handler,
	       int nentries)
{
	int	i;

//kprintf("mpic_find_entry: device: %d.\n", device);

	for (i = 0; i < nentries; i++, (*handler)++)
		if ( (*handler)->i_device == device)
			return i;

	*handler = NULL;
	return 0;
}


static unsigned int
mpic_int_to_number(int index)
{

//kprintf("mpic_pmac_int_to_number: Int: %d\n", index);

	// This is temporary. DeviceTreeProbe always bit reverses
	// for compatibility with the existing config tables. Once
	// GC and the driver config tables agree, remove this.
	index ^= 0x18;

	if (index >= 0 && index < nmpic_interrupts)
	    return (mpic_interrupts[index].i_device);
	if (index < (nmpic_interrupts + nmpic_via_interrupts))
	    return (mpic_via1_interrupts[index - nmpic_interrupts].i_device);
	return (-1);
}

static void
mpic_register_int(int device,
		  spl_t level,
		  void (*handler)(int, void *, void *), 
		  void *arg)
{
	int	i;
	u_long  tmp;
	struct powermac_interrupt	*p;

//kprintf("mpic_register_int: device: %d\n", device);

	/* Check primary interrupts */
	p = mpic_interrupts;
	i = mpic_find_entry(device, &p, nmpic_interrupts);
	if (p) {
		if (p->i_handler) {
			panic("mpic_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;

			tmp = lwbrx(MPIC_INT_CFG + i * 0x20);
#if 0
			tmp &= ~(MASKED | (0x0f << 16)); /* un-mask, 0 pri */
			tmp |= spl_to_mpic[level] << 16; /* add in real pri */
#else
			tmp &= ~MASKED;
#endif
			stwbrx(tmp, MPIC_INT_CFG + i * 0x20);
			eieio();
			return;
		}
		return;
	}
	/* Check cascaded interrupts */
	p = mpic_via1_interrupts;
	i = mpic_find_entry(device, &p, nmpic_via_interrupts);
	if (p) {
		if (p->i_handler) {
			panic("mpic_register_int: "
				"Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;

			*((v_u_char *) PCI_VIA1_IER) |= (1 << i);
			eieio();
			tmp = lwbrx(MPIC_INT_CFG + 20 * 0x20);
#if 0
			tmp &= ~(MASKED | (0x0f << 16)); /* un-mask, 0 pri */
			tmp |= spl_to_mpic[SPLTTY] << 16; /* add in real pri */
#else
			tmp &= ~MASKED;
#endif
			stwbrx(tmp, MPIC_INT_CFG + 20 * 0x20);
			eieio();
			return;
		}
		return;
	}

	panic("mpic_register_int: Interrupt %d not found", device);
}

#if 0
static spl_t
mpic_set_priority_level(spl_t lvl)
{
  spl_t old_level;

  old_level = current_priority;
  
// kprintf("mpic_set_priority_level: Setting Int Priv addr: 0x%x, data: %d.\n",
//	   MPIC_P0_CUR_TSK_PRI, spl_to_mpic[lvl]);

  if (lvl == SPLOFF) interrupt_disable();

  if (lvl < old_level) current_priority = lvl;

  /* Set new interrupt level in MPIC */
  stwbrx(spl_to_mpic[lvl], MPIC_P0_CUR_TSK_PRI);
  eieio();

  if (lvl >= old_level) current_priority = lvl;

  if (old_level == SPLOFF) interrupt_enable();

  return old_level;
}
#endif

static boolean_t
mpic_enable_irq(int irq)
{
  u_long    tmp;

    // This is temporary. DeviceTreeProbe always bit reverses
    // for compatibility with the existing config tables. Once
    // GC and the driver config tables agree, remove this.
    irq ^= 0x18;

  /* make sure the irq is in the mpic table and not via-cuda */
  if ((irq < 0) || (irq >= nmpic_interrupts) || (irq == 20))
    return FALSE;

  /* Unmask the source for this irq on mpic */
  tmp = lwbrx(MPIC_INT_CFG + irq * 0x20);
  tmp &= ~MASKED;
  stwbrx(tmp, MPIC_INT_CFG + irq * 0x20);
  eieio();

  return TRUE;
}

static boolean_t
mpic_disable_irq(int irq)
{
  u_long    tmp;

    // This is temporary. DeviceTreeProbe always bit reverses
    // for compatibility with the existing config tables. Once
    // GC and the driver config tables agree, remove this.
    irq ^= 0x18;

  /* make sure the irq is in the mpic table and not via-cuda */
  if ((irq < 0) || (irq >= nmpic_interrupts) || (irq == 20))
    return FALSE;

  /* Mask the source for this irq on mpic */
  tmp = lwbrx(MPIC_INT_CFG + irq * 0x20);
  tmp |= MASKED;
  stwbrx(tmp, MPIC_INT_CFG + irq * 0x20);
  eieio();

  return TRUE;
}

static void
mpic_interrupt(int type, struct ppc_saved_state *ssp,
	       unsigned int dsisr, unsigned int dar)
{
	unsigned long int	        irq;
	struct powermac_interrupt	*handler;
	
//kprintf("mpic_interrupt: Entering\n");

	/* Loop until all interrupts have been processed */
	while (1) {
	  /* Get the vector/irq number */
	  irq = lwbrx(MPIC_P0_INT_ACK);

	  /* Is this vector/irq really active? */
	  if (lwbrx(MPIC_INT_CFG + irq * 0x20) & ACTIVE) {

            /* Set the End of Interrupt Bit */
	    stwbrx(0, MPIC_P0_EOI);
	    
//kprintf("mpic_interrupt: IRQ: %d\n", irq);

//if (irq == 14) kprintf("IRQ14\n");

	    handler = &mpic_interrupts[irq];

            /* Handle the interrupt */
	    if (handler->i_handler)
	      handler->i_handler(handler->i_device, ssp, handler->i_arg);
	    else
	      printf("{MPIC INT %d}", irq);
	    
          } else break; /* No more interrupts so bail. */
	}

//if (irq == 14) kprintf("mpic_interrupt: Leaving\n");
}

void mpic_via1_interrupt(int device, void *ssp, void *arg)
{
	register unsigned long irq, bit;
	struct powermac_interrupt	*handler;

	irq = via_reg(PCI_VIA1_IFR); eieio();   /* get interrupts pending */
	irq &= via_reg(PCI_VIA1_IER); eieio();	/* only care about enabled */

	if (irq == 0)
		return;

	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(PCI_VIA1_IFR) = irq;
	eieio();

	/* Loop as long as there are bits set */
	while (irq) {
	  /* Find the bit position of the first set bit */
	  bit = 31 - cntlzw(irq);
	  
	  /* Find the handler */
	  handler = &mpic_via1_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}
