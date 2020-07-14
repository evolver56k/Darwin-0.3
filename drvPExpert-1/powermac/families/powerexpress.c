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
#include <families/powerexpress.h>
#include <chips/mpic.h>
#include <machdep/ppc/proc_reg.h>

powermac_init_t powerexpress_init = {
	configure_powerexpress,		// configure_machine
	mpic_interrupt_initialize,	// machine_initialize_interrupts
	NO_ENTRY,			// machine_initialize_network
	pex_initialize_bats,		// machine_initialize_processors
	rtc_init,			// machine_initialize_rtclock
	&heathrow_dbdma_channels,       // struct for dbdma channels
};

#define NPOWEREXPRESS_VIA1_INTERRUPTS 7
struct powermac_interrupt powerexpress_via1_interrupts[NPOWEREXPRESS_VIA1_INTERRUPTS] = {
	{ 0,	0,	0,	-1},			/* Cascade */
	{ 0,	0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	0,	PMAC_DEV_VIA1},		
	{ 0,	0, 	0,	-1},			/* VIA Data */
	{ 0,	0, 	0,	-1},			/* VIA CLK Source */
	{ 0,	0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	0,	PMAC_DEV_TIMER1}
};

#define NPOWEREXPRESS_INTERRUPTS 38

/* This structure is little-endian formatted... */

struct powermac_interrupt  powerexpress_interrupts[NPOWEREXPRESS_INTERRUPTS] = {
        { 0,    0, 0, -1},                   /* IRQ0  - SIOInt               */
	{ 0,    0, 0, PMAC_DMA_SCSI0},       /* IRQ1  - DMA SCSI 0 Mesh      */
	{ 0,    0, 0, PMAC_DMA_FLOPPY},      /* IRQ2  - DMA Floppy           */
	{ 0,    0, 0, PMAC_DMA_ETHERNET_TX}, /* IRQ3  - DMA Ethernet Tx      */
	{ 0,    0, 0, PMAC_DMA_ETHERNET_RX}, /* IRQ4  - DMA Ethernet Rx      */
	{ 0,    0, 0, PMAC_DMA_SCC_A_TX},    /* IRQ5  - DMA SCC Channel A Tx */
	{ 0,    0, 0, PMAC_DMA_SCC_A_RX},    /* IRQ6  - DMA SCC Channel A Rx */
	{ 0,    0, 0, PMAC_DMA_SCC_B_TX},    /* IRQ7  - DMA SCC Channel B Tx */
	{ 0,    0, 0, PMAC_DMA_SCC_B_RX},    /* IRQ8  - DMA SCC Channel B Rx */
	{ 0,    0, 0, PMAC_DMA_AUDIO_OUT},   /* IRQ9  - DMA Audio Out        */
	{ 0,    0, 0, PMAC_DMA_AUDIO_IN},    /* IRQ10 - DMA Audio In         */
	{ 0,    0, 0, PMAC_DMA_IDE0},        /* IRQ11 - DMA IDE 0            */
	{ 0,    0, 0, PMAC_DMA_IDE1},        /* IRQ12 - DMA IDE 1            */
	{ 0,    0, 0, -1},                   /* IRQ13 - AnyButton Dev        */
	{ 0,    0, 0, PMAC_DEV_SCSI0},       /* IRQ14 - SCSI0 Dev            */
	{ 0,    0, 0, PMAC_DEV_IDE0},        /* IRQ15 - IDE 0 Dev            */
	{ 0,    0, 0, PMAC_DEV_IDE1},        /* IRQ16 - IDE 1 Dev            */
	{ 0,    0, 0, PMAC_DEV_SCC_A},       /* IRQ17 - SCC Channel A Dev    */
	{ 0,    0, 0, PMAC_DEV_SCC_B},       /* IRQ18 - SCC Channel B Dev    */
	{ 0,    0, 0, PMAC_DEV_AUDIO},       /* IRQ19 - Audio Dev            */
	{ mpic_via1_interrupt, 0, 0, -1},    /* IRQ20 - VIA (Cuda)           */
	{ 0,    0, 0, PMAC_DEV_FLOPPY},      /* IRQ21 - Floppy Dev           */
	{ 0,    0, 0, PMAC_DEV_ETHERNET},    /* IRQ22 - Ethernet Dev         */
	{ 0,    0, 0, -1},                   /* IRQ23 - ADB                  */
	{ 0,    0, 0, -1},                   /* IRQ24 - MBDevIn (Control)    */
	{ 0,    0, 0, -1},                   /* IRQ25 - NMI                  */
	{ 0,    0, 0, PMAC_DEV_CARD0},       /* IRQ26 - ExtInt0  - Denali 2  */
	{ 0,    0, 0, PMAC_DEV_CARD1},       /* IRQ27 - ExtInt1  - UF SCSI   */
	{ 0,    0, 0, PMAC_DEV_CARD2},       /* IRQ28 - ExtInt2  - Denali 1  */
	{ 0,    0, 0, PMAC_DEV_CARD3},       /* IRQ29 - ExtInt3  - PCI1 A    */
	{ 0,    0, 0, PMAC_DEV_CARD4},       /* IRQ30 - ExtInt4  - PCI1 B    */
	{ 0,    0, 0, PMAC_DEV_CARD5},       /* IRQ31 - ExtInt5  - PCI1 C    */
	{ 0,    0, 0, PMAC_DEV_CARD6},       /* IRQ32 - ExtInt6  - Control   */
	{ 0,    0, 0, PMAC_DEV_CARD7},       /* IRQ33 - ExtInt7  - Ninety9   */
	{ 0,    0, 0, PMAC_DEV_CARD8},       /* IRQ34 - ExtInt8  - PlanB     */
	{ 0,    0, 0, PMAC_DEV_CARD9},       /* IRQ35 - ExtInt9  - PCI2 D    */
	{ 0,    0, 0, PMAC_DEV_CARD10},      /* IRQ36 - ExtInt10 - PCI2 E    */
	{ 0,    0, 0, PMAC_DEV_CARD11}       /* IRQ37 - ExtInt11 - PCI2 F    */
};

static u_long powerexpress_int_mapping_tbl[NPOWEREXPRESS_INTERRUPTS * 2] = {
/*      Vector|Level|Sence|Polarity | Mask  | Dest (CPU)   */
INT_TBL(  0,     0,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ0  - SIOInt          */
INT_TBL(  1,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ1  - SCSI DMA        */
INT_TBL(  2,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ2  - Floppy DMA      */
INT_TBL(  3,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ3  - Eth Tx DMA      */
INT_TBL(  4,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ4  - Eth Rx  DMA     */
INT_TBL(  5,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ5  - SCC Tx A DMA    */
INT_TBL(  6,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ6  - SCC Rx A DMA    */
INT_TBL(  7,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ7  - SCC Tx B DMA    */
INT_TBL(  8,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ8  - SCC Rx B DMA    */
INT_TBL(  9,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ9  - Audio Out DMA   */
INT_TBL( 10,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ10 - Audio In  DMA   */
INT_TBL( 11,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ11 - IDE 0 DMA       */
INT_TBL( 12,     4,   EDGE, ACT_HI,   MASKED, 1), /* IRQ12 - IDE 1 DMA       */
INT_TBL( 13,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ13 - Any Button Dev  */
INT_TBL( 14,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ14 - SCSI Dev        */
INT_TBL( 15,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ15 - IDE 0 Dev       */
INT_TBL( 16,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ16 - IDE 1 Dev       */
INT_TBL( 17,     4,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ17 - SCC A Dev       */
INT_TBL( 18,     4,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ18 - SCC B Dev       */
INT_TBL( 19,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ19 - Audio Dev       */
INT_TBL( 20,     1,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ20 - VIA Dev         */
INT_TBL( 21,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ21 - Floppy Dev      */
INT_TBL( 22,     3,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ22 - Ethernet Dev    */
INT_TBL( 23,     1,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ23 - ADB             */
INT_TBL( 24,     2,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ24 - MBDevIn         */
INT_TBL( 25,     7,   EDGE, ACT_LOW,  MASKED, 1), /* IRQ25 - NMI             */
INT_TBL( 26,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ26 - ExtInt0  -Den2  */
INT_TBL( 27,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ27 - ExtInt1  -UFSCSI*/
INT_TBL( 28,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ28 - ExtInt2  -Den1  */
INT_TBL( 29,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ29 - ExtInt3  -PCI1A */
INT_TBL( 30,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ30 - ExtInt4  -PCI1B */
INT_TBL( 31,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ31 - ExtInt5  -PCI1C */
INT_TBL( 32,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ32 - ExtInt6  -Cntrl */
INT_TBL( 33,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ33 - ExtInt7  -99    */
INT_TBL( 34,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ34 - ExtInt8  -PlanB */
INT_TBL( 35,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ35 - ExtInt9  -PCI2D */
INT_TBL( 36,     2,    LVL, ACT_LOW,  MASKED, 1), /* IRQ36 - ExtInt10 -PCI2E */
INT_TBL( 37,     2,    LVL, ACT_LOW,  MASKED, 1)  /* IRQ37 - ExtInt11 -PCI2F */
};

/* Table for converting from spl to mpic priority levels */
static int powerexpress_spl_to_pri[33] =  /* Yes 33: 0 through 32 */
{
  0,  0, 0,  0, /* SPLLO,           1,       2,       3,        */
  0,  4, 0,  0, /* 4,               SPLSCLK, 6,       7,        */
  8,  0, 0,  0, /* SPLNET,          9,       10,      11,       */
  0,  0, 0,  0, /* 12,              13,      14,      15,       */
  12, 0, 0,  0, /* SPLIMP & SPLBIO, 17,      18,      19,       */
  0,  0, 0,  0, /* 20,              21,      22,      23,       */
  0,  0, 0,  0, /* 24,              25,      26,      27,       */
  0,  0, 15, 0, /* SPLVM,           29,      SPLHIGH, SPLPOWER, */
  15            /* SPLCLOCK & SPLOFF */
};

void configure_powerexpress(void)
{
  mpic_interrupts = &powerexpress_interrupts;
  mpic_via1_interrupts = &powerexpress_via1_interrupts;
  mpic_int_mapping_tbl = &powerexpress_int_mapping_tbl;
  mpic_spl_to_pri = &powerexpress_spl_to_pri;
  
  nmpic_interrupts = NPOWEREXPRESS_INTERRUPTS;
  nmpic_via_interrupts = NPOWEREXPRESS_VIA1_INTERRUPTS;

  powermac_info.viaIRQ = (NPOWEREXPRESS_INTERRUPTS + 2) ^ 0x18;
}

void pex_initialize_bats()
{

#ifndef UseOpenFirmware

	PEMapSegment( 0xc0000000, 0x10000000);

#endif
}
