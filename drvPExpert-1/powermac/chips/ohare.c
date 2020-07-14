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
#include <machdep/ppc/exception.h>
#include <powermac.h>
#include <interrupts.h>
#include <chips/ohare.h>


/* Prototypes */

static int
ohare_find_entry(int device, struct powermac_interrupt **handler, int nentries);

static unsigned int
ohare_int_to_number(int index);

static void
ohare_register_int(int device, spl_t level,
		void (*handler)(int, void *, void *), void *arg);

static spl_t
ohare_set_priority_level(spl_t lvl);

static boolean_t
ohare_enable_irq(int irq);

static boolean_t
ohare_disable_irq(int irq);

static void
ohare_interrupt(int type, struct ppc_saved_state *ssp,
		unsigned int dsisr, unsigned int dar);

void ohare2_interrupt(int device, void *ssp, void *arg);

void ohare_via1_interrupt(int device, void *ssp, void *arg);

/* Storage for interrupt table pointers */
struct powermac_interrupt *ohare_interrupts;
struct powermac_interrupt *ohare2_interrupts;
struct powermac_interrupt *ohare_via1_interrupts;
int nohare_interrupts;
int nohare2_interrupts;
int nohare_via_interrupts;

/* DBDMA Channel Map */
powermac_dbdma_channels_t ohare_dbdma_channels =
{ -1,      // O'Hare does not have Curio
  0x00,    // Mesh
  0x01,    // Floppy
  -1,      // O'Hare does not have Ethernet Transmit
  -1,      // O'Hare does not have Ethernet Receive
  0x04,    // SCC Channel A Transmit
  0x05,    // SCC Channel A Receive
  0x06,    // SCC Channel B Transmit
  0x07,    // SCC Channel B Receive
  0x08,    // Audio Out
  0x09,    // Audio In
  0x0B,    // IDE 0
  0x0C     // IDE 1
};

/* Reset the hardware interrupt control */
void
ohare_interrupt_initialize(void)
{
	pmac_register_int       = ohare_register_int;
	pmac_int_to_number      = ohare_int_to_number;
//	pmac_set_priority_level = ohare_set_priority_level;
	pmac_enable_irq         = ohare_enable_irq;
	pmac_disable_irq        = ohare_disable_irq;
	pmac_interrupt          = ohare_interrupt;

	*OHARE_INTERRUPT_MASK_REG  = 0;	   /* Disable all interrupts */
	eieio();
	*OHARE_INTERRUPT_CLEAR_REG = 0xffffffff; /* Clear pending interrupts */
	eieio();
	*OHARE_INTERRUPT_MASK_REG  = 0;	   /* Disable all interrupts */
	eieio();
	*(v_u_char *)PCI_VIA1_PCR               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IER               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IFR               = 0x7f; eieio();
}

static int
ohare_find_entry(int device,
	       struct powermac_interrupt **handler,
	       int nentries)
{
	int	i;

//kprintf("ohare_find_entry: device: %d.\n", device);

	for (i = 0; i < nentries; i++, (*handler)++)
		if ( (*handler)->i_device == device)
			return i;

	*handler = NULL;
	return 0;
}


unsigned int
ohare_int_to_number(int index)
{

//kprintf("ohare_pmac_int_to_number: Int: %d\n", index);

  if (index < 0) return -1;

  // is it in the ohare table?
  if (index < nohare_interrupts)
    return (ohare_interrupts[index].i_device);

  index -= nohare_interrupts;

  // is it in the via table?
  if (index < nohare_via_interrupts)
    return (ohare_via1_interrupts[index].i_device);

  index -= nohare_via_interrupts;

  // is it in the ohare2 table?
  if (index < nohare2_interrupts)
    return (ohare2_interrupts[index].i_device);

  return -1;
}

void
ohare_register_int(int device, spl_t level, void (*handler)(int, void *, void *), 
		 void * arg)
{
	int	i;
	struct powermac_interrupt	*p;

//kprintf("ohare_register_int: device: %d\n", device);

	/* Check ohare interrupts */
	p = ohare_interrupts;
	i = ohare_find_entry(device, &p, nohare_interrupts);
	if (p) {
//kprintf("ohare_register_int: ohare irq: %d\n", i);
		if (p->i_handler) {
			panic("ohare_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;
			*OHARE_INTERRUPT_MASK_REG |= (1<<i);
			eieio();
			return;
		}
		return;
	}

	/* Check ohare 2 interrupts */
	p = ohare2_interrupts;
	i = ohare_find_entry(device, &p, nohare2_interrupts);
	if (p) {
//kprintf("ohare_register_int: ohare2 irq: %d\n", i);
		if (p->i_handler) {
			panic("ohare_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;
			*OHARE2_INTERRUPT_MASK_REG |= (1<<i);
			eieio();
			*OHARE_INTERRUPT_MASK_REG |= (1<<4);	/* enable */
			eieio();
			return;
		}
		return;
	}

	/* Check via1 interrupts */
	p = ohare_via1_interrupts;
	i = ohare_find_entry(device, &p, nohare_via_interrupts);
	if (p) {
//kprintf("ohare_register_int: via1 irq: %d\n", i);
		if (p->i_handler) {
			panic("ohare_register_int: "
				"Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;

			*((v_u_char *) PCI_VIA1_IER) |= (1 << i);
			eieio();
			*OHARE_INTERRUPT_MASK_REG |= (1<<10);	/* enable */
			eieio();
			return;
		}
		return;
	}

	panic("ohare_register_int: Interrupt %d not found", device);
}

static spl_t
ohare_set_priority_level(spl_t lvl)
{
}

static boolean_t
ohare_enable_irq(int irq)
{
  /* make sure the irq is in the ohare table and not via-cuda */
  if ((irq < 0) || (irq >= nohare_interrupts) || (irq == 10))
    return FALSE;

  /* Unmask the source for this irq on ohare */
  *OHARE_INTERRUPT_MASK_REG |= (1 << irq);
  eieio();

  return TRUE;
}

static boolean_t
ohare_disable_irq(int irq)
{
  /* make sure the irq is in the ohare table and not via-cuda */
  if ((irq < 0) || (irq >= nohare_interrupts) || (irq == 10))
    return FALSE;

  /* Mask the source for this irq on ohare */
  *OHARE_INTERRUPT_MASK_REG &= ~(1 << irq);
  eieio();

  return TRUE;
}

static void
ohare_interrupt( int type, struct ppc_saved_state *ssp,
	       unsigned int dsisr, unsigned int dar)
{
	unsigned long int		bit, irq;
	struct powermac_interrupt	*handler;

	irq = *OHARE_INTERRUPT_EVENTS_REG; eieio();
	*OHARE_INTERRUPT_CLEAR_REG = irq;	/* Clear out interrupts */
	eieio();

	/* Loop as long as there are bits set */
	while (irq) {
	  /* Find the bit position of the first set bit */
	  bit = 31 - cntlzw(irq);

	  /* Find the handler */
	  handler = &ohare_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  } else {
	    printf("{OHARE INT %d}", bit);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}

void ohare2_interrupt(int device, void *ssp, void * arg)
{
	unsigned long int		bit, irq;
	struct powermac_interrupt	*handler;

	irq = *OHARE2_INTERRUPT_EVENTS_REG; eieio();
	*OHARE2_INTERRUPT_CLEAR_REG = irq;	/* Clear out interrupts */
	eieio();

	/* Loop as long as there are bits set */
	while (irq) {
	  /* Find the bit position of the first set bit */
	  bit = 31 - cntlzw(irq);

//kprintf("ohare2_interrupt: irq: %d\n", bit);

	  /* Find the handler */
	  handler = &ohare2_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  } else {
	    printf("{OHARE2 INT %d}", bit);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}

void ohare_via1_interrupt(int device, void *ssp, void * arg)
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
	  handler = &ohare_via1_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}
