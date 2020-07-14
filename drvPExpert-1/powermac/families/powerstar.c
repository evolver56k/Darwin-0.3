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

#include <machdep/ppc/interrupts.h>
#include <families/powerstar.h>
#include <chips/ohare.h>

// in identify_machine.c
extern int set_ethernet_irq(int newIRQ);

powermac_init_t powerstar_init = {
	configure_powerstar,		// configure_machine
	ohare_interrupt_initialize,	// machine_initialize_interrupts
	NO_ENTRY,			// machine_initialize_network
	NO_ENTRY,			// machine_initialize_processors
	rtc_init,			// machine_initialize_rtclock
	&ohare_dbdma_channels,          // struct for dbdma channels
};

#define NPOWERSTAR_VIA1_INTERRUPTS 7
struct powermac_interrupt powerstar_via1_interrupts[NPOWERSTAR_VIA1_INTERRUPTS] = {
	{ 0,	0,	0,	-1},			/* Cascade */
	{ 0,	0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	0,	PMAC_DEV_VIA1},
	{ 0,	0,	0,	PMAC_DEV_VIA2},         /* VIA Data */
	{ 0,	0,	0,	PMAC_DEV_VIA3},         /* VIA CLK Source */
	{ 0,	0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	0,	PMAC_DEV_TIMER1}
};

#define NPOWERSTAR2_INTERRUPTS 32
/* This structure is little-endian formatted... */

struct powermac_interrupt  powerstar2_interrupts[NPOWERSTAR2_INTERRUPTS] = {
	{ 0, 	0, 0, -1},		  /* Bit 24 - Ext Int 4 */
	{ 0, 	0, 0, -1},		  /* Bit 25 - Ext Int 5 */
	{ 0, 	0, 0, -1},		  /* Bit 26 - Ext Int 6 */
	{ 0, 	0, 0, -1},		  /* Bit 27 - Ext Int 7 */
	{ 0, 	0, 0, PMAC_DEV_CARD10},	  /* Bit 28 - Ext Int 8 */
	{ 0, 	0, 0, -1},		  /* Bit 29 - Ext Int 9 */
	{ 0, 	0, 0, -1},		  /* Bit 30 - Ext Int 10 */
	{ 0, 	0, 0, -1},		  /* Bit 31 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 16 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 17 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 18 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 19 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 20 - Ext Int 0 */
	{ 0, 	0, 0, -1},		  /* Bit 21 - Ext Int 1 */
	{ 0, 	0, 0, -1},		  /* Bit 22 - Ext Int 2 */
	{ 0, 	0, 0, -1},		  /* Bit 23 - Ext Int 3 */
	{ 0, 	0, 0, -1},		  /* Bit  8 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  9 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 10 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 11 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 12 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 13 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 14 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit 15 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  0 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  1 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  2 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  3 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  4 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  5 - Reserved */
	{ 0, 	0, 0, -1},		  /* Bit  6 - Reserved */
	{ 0, 	0, 0, -1}		  /* Bit  7 - Reserved */
};

#define NPOWERSTAR_INTERRUPTS 32

/* This structure is little-endian formatted... */

struct powermac_interrupt  powerstar_interrupts[NPOWERSTAR_INTERRUPTS] = {
	{ 0, 	0, 0, PMAC_DEV_CARD4},	  /* Bit 24 - Ext Int 4 - Video */
	{ 0, 	0, 0, PMAC_DEV_CARD5},	  /* Bit 25 - Ext Int 5 - PCI Slot */
	{ 0, 	0, 0, PMAC_DEV_CARD6},	  /* Bit 26 - Ext Int 6 - Media Bay */
	{ 0, 	0, 0, PMAC_DEV_CARD7},	  /* Bit 27 - Ext Int 7 - Aragon Eth */
	{ 0, 	0, 0, PMAC_DEV_CARD8},	  /* Bit 27 - Ext Int 7 - O'Hare 2 */
	{ 0, 	0, 0, PMAC_DEV_CARD9},	  /* Bit 29 - Reserved */
	{ 0, 	0, 0, PMAC_DEV_IN},	  /* Bit 30 - DevIn */
	{ 0, 	0, 0, -1},		  /* Bit 31 - Reserved */
	{ 0,	0, 0, PMAC_DEV_SCC_B},	  /* Bit 16 - SCC Channel B */
	{ 0,	0, 0, PMAC_DEV_AUDIO}, 	  /* Bit 17 - Audio */
	{ ohare_via1_interrupt, 0, 0, -1},/* Bit 18 - VIA (cuda/pmu) */
	{ 0,	0, 0, PMAC_DEV_FLOPPY},	  /* Bit 19 - SwimIII/Floppy */
	{ 0, 	0, 0, PMAC_DEV_NMI},	  /* Bit 20 - Ext Int 0 */
	{ 0, 	0, 0, PMAC_DEV_CARD1},	  /* Bit 21 - Ext Int 1 */
	{ 0, 	0, 0, PMAC_DEV_CARD2},	  /* Bit 22 - Ext Int 2 - PC Card 1 */
	{ 0, 	0, 0, PMAC_DEV_CARD3},	  /* Bit 23 - Ext Int 3 - PC Card 2 */
	{ 0,	0, 0, PMAC_DMA_AUDIO_OUT},/* Bit 8 - DMA Audio Out */
	{ 0,	0, 0, PMAC_DMA_AUDIO_IN}, /* Bit 9 - DMA Audio In */
	{ 0,	0, 0, -1},	          /* Bit 10 - Reserved */
	{ 0,	0, 0, -1},		  /* Bit 11 - AnyButton */
	{ 0,	0, 0, PMAC_DEV_SCSI0},	  /* Bit 12 - SCSI 0 */
	{ 0,	0, 0, PMAC_DEV_IDE0},	  /* Bit 13 - IDE 0 */
	{ 0,	0, 0, PMAC_DEV_IDE1},     /* Bit 14 - IDE 1 */
	{ 0,	0, 0, PMAC_DEV_SCC_A},	  /* Bit 15 - SCC Channel A */
	{ 0,	0, 0, PMAC_DMA_SCSI0},	  /* Bit 0 - DMA SCSI 0 */
	{ 0,	0, 0, PMAC_DMA_FLOPPY},	  /* Bit 1 - DMA Floppy */
	{ 0,	0, 0, PMAC_DMA_IDE0},     /* Bit 2 - DMA IDE0 */
	{ 0,	0, 0, PMAC_DMA_IDE1},     /* Bit 3 - DMA IDE1 */
	{ 0,	0, 0, PMAC_DMA_SCC_A_TX}, /* Bit 4 - DMA SCC Channel A TX */
	{ 0,	0, 0, PMAC_DMA_SCC_A_RX}, /* Bit 5 - DMA SCC Channel A RX */
	{ 0,	0, 0, PMAC_DMA_SCC_B_TX}, /* Bit 6 - DMA SCC Channel B TX */
	{ 0,	0, 0, PMAC_DMA_SCC_B_RX}  /* Bit 7 - DMA SCC Channel B RX */
};

void configure_powerstar(void)
{
  int hasOHare2;
  
  ohare_interrupts = &powerstar_interrupts;
  ohare2_interrupts     = &powerstar2_interrupts;
  ohare_via1_interrupts = &powerstar_via1_interrupts;
  
  nohare_interrupts = NPOWERSTAR_INTERRUPTS;
  nohare2_interrupts    = NPOWERSTAR2_INTERRUPTS;
  nohare_via_interrupts = NPOWERSTAR_VIA1_INTERRUPTS;

  powermac_info.viaIRQ  = NPOWERSTAR_INTERRUPTS + 2;
  
  if (HasPMU()) {
    hasOHare2 = set_ethernet_irq(NPOWERSTAR_INTERRUPTS +
				 NPOWERSTAR_VIA1_INTERRUPTS + 4);
    
    if (hasOHare2) {
      // add the O'Hare 2 interrupt hander to the O'Hare 1 table.
      ohare_interrupts[4].i_handler = ohare2_interrupt;
      ohare_interrupts[4].i_device  = -1;
    }
  } 
}
