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
#include <chips/grand_central.h>


/* Prototypes */

static int
gc_find_entry(int device, struct powermac_interrupt **handler, int nentries);

static unsigned int
gc_int_to_number(int index);

static void
gc_register_int(int device, spl_t level,
		void (*handler)(int, void *, void *), void *arg);

static spl_t
gc_set_priority_level(spl_t lvl);

static boolean_t
gc_enable_irq(int irq);

static boolean_t
gc_disable_irq(int irq);

static void
gc_interrupt(int type, struct ppc_saved_state *ssp,
	     unsigned int dsisr, unsigned int dar);

void gc_via1_interrupt(int device, void *ssp, void *arg);

/* Storage for interrupt table pointers */
struct powermac_interrupt *gc_interrupts;
struct powermac_interrupt *gc_via1_interrupts;
int ngc_via_interrupts;
int ngc_interrupts;

/* DBDMA Channel Map */
powermac_dbdma_channels_t gc_dbdma_channels =
{ 0x00,    // Curio
  0x0A,    // Mesh
  0x01,    // Floppy
  0x02,    // Ethernet Transmit
  0x03,    // Ethernet Receive
  0x04,    // SCC Channel A Transmit
  0x05,    // SCC Channel A Receive
  0x06,    // SCC Channel B Transmit
  0x07,    // SCC Channel B Receive
  0x08,    // Audio Out
  0x09,    // Audio In
  -1,      // Grand Central does not have IDE0
  -1       // Grand Central does not have IDE1
};

static void	gc_interrupt(int type,
			struct ppc_saved_state *ssp,
			unsigned int dsisr, unsigned int dar);

/* Reset the hardware interrupt control */
void
gc_interrupt_initialize(void)
{
	pmac_register_int       = gc_register_int;
	pmac_int_to_number      = gc_int_to_number;
//	pmac_set_priority_level = gc_set_priority_level;
	pmac_enable_irq         = gc_enable_irq;
	pmac_disable_irq        = gc_disable_irq;
	pmac_interrupt          = gc_interrupt;

	*GC_INTERRUPT_MASK_REG  = 0;	   /* Disable all interrupts */
	eieio();
	*GC_INTERRUPT_CLEAR_REG = 0xffffffff; /* Clear pending interrupts */
	eieio();
	*GC_INTERRUPT_MASK_REG  = 0;	   /* Disable all interrupts */
	eieio();
	*(v_u_char *)PCI_VIA1_PCR               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IER               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IFR               = 0x7f; eieio();
}

static int
gc_find_entry(int device,
	       struct powermac_interrupt **handler,
	       int nentries)
{
	int	i;

	for (i = 0; i < nentries; i++, (*handler)++)
		if ( (*handler)->i_device == device)
			return i;

	*handler = NULL;
	return 0;
}


unsigned int
gc_int_to_number(int index)
{
	if (index >= 0 && index < ngc_interrupts)
	    return (gc_interrupts[index].i_device);
	if (index < (ngc_interrupts + ngc_via_interrupts))
	    return (gc_via1_interrupts[index - ngc_interrupts].i_device);
	return (-1);
}

void
gc_register_int(int device, spl_t level, void (*handler)(int, void *, void *), 
		 void * arg)
{
	int	i;
	struct powermac_interrupt	*p;

	/* Check primary interrupts */
	p = gc_interrupts;
	i = gc_find_entry(device, &p, ngc_interrupts);
	if (p) {
		if (p->i_handler) {
			panic("gc_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;
			*GC_INTERRUPT_MASK_REG |= (1<<i);
			eieio();
			return;
		}
		return;
	}
	/* Check cascaded interrupts */
	p = gc_via1_interrupts;
	i = gc_find_entry(device, &p, ngc_via_interrupts);
	if (p) {
		if (p->i_handler) {
			panic("gc_register_int: "
				"Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			p->i_arg = arg;

			*((v_u_char *) PCI_VIA1_IER) |= (1 << i);
			eieio();
			*GC_INTERRUPT_MASK_REG |= (1<<10);	/* enable */
			eieio();
			return;
		}
		return;
	}

	panic("gc_register_int: Interrupt %d not found", device);
}

static spl_t
gc_set_priority_level(spl_t lvl)
{
}

static boolean_t
gc_enable_irq(int irq)
{
  /* make sure the irq is in the gc table and not via-cuda */
  if ((irq < 0) || (irq >= ngc_interrupts) || (irq == 10))
    return FALSE;

  /* Unmask the source for this irq on gc */
  *GC_INTERRUPT_MASK_REG |= (1 << irq);
  eieio();

  return TRUE;
}

static boolean_t
gc_disable_irq(int irq)
{
  /* make sure the irq is in the gc table and not via-cuda */
  if ((irq < 0) || (irq >= ngc_interrupts) || (irq == 10))
    return FALSE;

  /* Mask the source for this irq on gc */
  *GC_INTERRUPT_MASK_REG &= ~(1 << irq);
  eieio();

  return TRUE;
}

static void
gc_interrupt( int type, struct ppc_saved_state *ssp,
	       unsigned int dsisr, unsigned int dar)
{
	unsigned long int		bit, irq;
	struct powermac_interrupt	*handler;

	irq = *GC_INTERRUPT_EVENTS_REG; eieio();
	*GC_INTERRUPT_CLEAR_REG = irq;	/* Clear out interrupts */
	eieio();

	/* Loop as long as there are bits set */
	while (irq) {
	  /* Find the bit position of the first set bit */
	  bit = 31 - cntlzw(irq);
	  
	  /* Find the handler */
	  handler = &gc_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  } else {
	    printf("{GC INT %d}", bit);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}

void gc_via1_interrupt(int device, void *ssp, void * arg)
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
	  handler = &gc_via1_interrupts[bit];
	  
	  if (handler->i_handler) {
	    handler->i_handler(handler->i_device, ssp, handler->i_arg);
	  }
	  
	  /* Clear the bit in irq that we just dispached. */
	  irq &= ~(1<<bit);
	}
}

