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
#include <families/powersurge.h>
#include <chips/grand_central.h>

powermac_init_t powersurge_init = {
	configure_powersurge,		// configure_machine
	gc_interrupt_initialize,	// machine_initialize_interrupts
	init_mace,			// machine_initialize_network
	NO_ENTRY,			// machine_initialize_processors
	rtc_init,			// machine_initialize_rtclock
	&gc_dbdma_channels,             // struct for dbdma channels
};

#define NPOWERSURGE_VIA1_INTERRUPTS 7
struct powermac_interrupt powersurge_via1_interrupts[NPOWERSURGE_VIA1_INTERRUPTS] = {
	{ 0,	0,	0,	-1},			/* Cascade */
	{ 0,	0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	0,	PMAC_DEV_VIA1},		
	{ 0,	0, 	0,	-1},			/* VIA Data */
	{ 0,	0, 	0,	-1},			/* VIA CLK Source */
	{ 0,	0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	0,	PMAC_DEV_TIMER1}
};

#define NPOWERSURGE_INTERRUPTS 32

/* This structure is little-endian formatted... */

struct powermac_interrupt  powersurge_interrupts[NPOWERSURGE_INTERRUPTS] = {
	{ 0, 	0, 0, PMAC_DEV_CARD4},	  /* Bit 24 - External Int 4 */
	{ 0, 	0, 0, PMAC_DEV_CARD5},	  /* Bit 25 - External Int 5 */
	{ 0, 	0, 0, PMAC_DEV_CARD6},	  /* Bit 26 - External Int 6 */
	{ 0, 	0, 0, PMAC_DEV_CARD7},	  /* Bit 27 - External Int 7 */
	{ 0, 	0, 0, PMAC_DEV_CARD8},	  /* Bit 28 - External Int 8 */
	{ 0, 	0, 0, PMAC_DEV_CARD9},	  /* Bit 29 - External Int 9 */
	{ 0, 	0, 0, PMAC_DEV_CARD10},	  /* Bit 30 - External Int 10 */
	{ 0, 	0, 0, -1},			  /* Bit 31 - Reserved */
	{ 0,	0, 0, PMAC_DEV_SCC_B},	  /* Bit 16 - SCC Channel B */
	{ 0,	0, 0, PMAC_DEV_AUDIO}, 	  /* Bit 17 - Audio */
	{ gc_via1_interrupt, 0, 0, -1} ,  /* Bit 18 - VIA (cuda) */
	{ 0,	0, 0, PMAC_DEV_FLOPPY},	  /* Bit 19 - SwimIII/Floppy */

	{ 0, 	0, 0, PMAC_DEV_NMI},	  /* Bit 20 - External Int 0 */
	{ 0, 	0, 0, PMAC_DEV_CARD1},	  /* Bit 21 - External Int 1 */
	{ 0, 	0, 0, PMAC_DEV_CARD2},	  /* Bit 22 - External Int 2 */
	{ 0, 	0, 0, PMAC_DEV_CARD3},	  /* Bit 23 - External Int 3 */
	{ 0,	0, 0, PMAC_DMA_AUDIO_OUT},	  /* Bit 8 - DMA Audio Out */
	{ 0,	0, 0, PMAC_DMA_AUDIO_IN},	  /* Bit 9 - DMA Audio In */
	{ 0,	0, 0, PMAC_DMA_SCSI1},	  /* Bit 10 - DMA SCSI 1 */
	{ 0,	0, 0, -1},			  /* Bit 11 - Reserved */
	{ 0,	0, 0, PMAC_DEV_SCSI0},	  /* Bit 12 - SCSI 0 */
	{ 0,	0, 0, PMAC_DEV_SCSI1},	  /* Bit 13 - SCSI 1 */
	{ 0,	0, 0, PMAC_DEV_ETHERNET},	  /* Bit 14 - MACE/Ethernet */
	{ 0,	0, 0, PMAC_DEV_SCC_A},	  /* Bit 15 - SCC Channel A */
	{ 0,	0, 0, PMAC_DMA_SCSI0},	  /* Bit 0 - DMA SCSI 0 */
	{ 0,	0, 0, PMAC_DMA_FLOPPY},	  /* Bit 1 - DMA Floppy */
	{ 0,	0, 0, PMAC_DMA_ETHERNET_TX}, /* Bit 2 - DMA Ethernet Tx */
	{ 0,	0, 0, PMAC_DMA_ETHERNET_RX}, /* Bit 3 - DMA Ethernet Rx */
	{ 0,	0, 0, PMAC_DMA_SCC_A_TX},	  /* Bit 4 - DMA SCC Channel A TX */
	{ 0,	0, 0, PMAC_DMA_SCC_A_RX},	  /* Bit 5 - DMA SCC Channel A RX */
	{ 0,	0, 0, PMAC_DMA_SCC_B_TX},	  /* Bit 6 - DMA SCC Channel B TX */
	{ 0,	0, 0, PMAC_DMA_SCC_B_RX}	  /* Bit 7 - DMA SCC Channel B RX */
};

void configure_powersurge(void)
{
  gc_interrupts = &powersurge_interrupts;
  gc_via1_interrupts = &powersurge_via1_interrupts;

  ngc_interrupts = NPOWERSURGE_INTERRUPTS;
  ngc_via_interrupts = NPOWERSURGE_VIA1_INTERRUPTS;

  powermac_info.viaIRQ = NPOWERSURGE_INTERRUPTS + 2;
}
