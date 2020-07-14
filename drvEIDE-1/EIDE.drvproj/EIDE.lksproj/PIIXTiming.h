/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1998 by Apple Computer, Inc., All rights reserved.
 *
 * Intel PIIX/PIIX3/PIIX4 PCI IDE controller timing tables.
 *
 * HISTORY:
 * 1-Feb-1998	Joe Liu at Apple
 *	Created.
 */
#import "PIIX.h"

typedef ideTransferType_t PIIXTransferType_t;

/*
 * PIIX PIO/DMA timing table.
 */

typedef struct {
	u_char	pio_mode;
	u_char	swdma_mode;
	u_char	mwdma_mode;
	u_char	isp;	// IORDY sample point in PCI clocks
	u_char	rct;	// Recovery time in PCI clocks
	u_short	cycle;	// cycle time in ns
} PIIXTiming;

#define INV		255			// invalid mode
#define PIIX_TIMING_TABLE_SIZE	6

static const
PIIXTiming PIIXTimingTable[] = {
//   PIO    SW     MW
	{0,     0,     0,     5, 4, 600},	// compatible timing
	{1, 	1,     INV,   5, 4, 600},
	{2,     2,     INV,   4, 4, 240},
	{3,     INV,   1,     3, 3, 180},
    {4,     INV,   2,     3, 1, 120},
	{5,     INV,   2,     3, 1, 120},	// Isn't this 90ns?
};

/*
 * PIIX Ultra DMA/33 timing table.
 */
typedef struct {
	u_char	mode;
	u_char	ct;		// Cycle time in PCI clocks
	u_char	rp;		// Ready to Pause time in PCI clocks
	u_char	bits;	// bit settings
	u_short	strobe;	// strobe period in ns
} PIIXUltraDMATiming;

static const
PIIXUltraDMATiming PIIXUltraDMATimingTable[] = {
	{0,     4,     6,     0,     120},
	{1, 	3,     5,     1,     90},
	{2,     2,     4,     2,     60},
};

/*
 * Given a transfer mode/type, return the index for the
 * entry in PIIXTimingTable[] which matches the mode.
 */
static __inline__
u_char
PIIXFindModeInTable(u_char mode, PIIXTransferType_t type)
{
	int i;	
	for (i = (PIIX_TIMING_TABLE_SIZE - 1); i  >= 0; i--) {
		u_char m;
		
		switch (type) {
			case IDE_TRANSFER_ULTRA_DMA:
			case IDE_TRANSFER_MW_DMA:
				m = PIIXTimingTable[i].mwdma_mode;
				break;
			case IDE_TRANSFER_SW_DMA:
				m = PIIXTimingTable[i].swdma_mode;
				break;
			case IDE_TRANSFER_PIO:
			default:
				m = PIIXTimingTable[i].pio_mode;
		}
		
		if (mode == m)
			return (i);
	}
	
	// not found, return compatible timing
	return (0);
}

/*
 * Given a transfer mode/type, return the ISP value.
 */
static __inline__
u_char
PIIXGetISPForMode(u_char mode, PIIXTransferType_t type)
{
	u_char index = PIIXFindModeInTable(mode, type);
	return (PIIX_CLK_TO_ISP(PIIXTimingTable[index].isp));
}

/*
 * Given a transfer mode/type, return the RCT value.
 */
static __inline__
u_char
PIIXGetRCTForMode(u_char mode, PIIXTransferType_t type)
{
	u_char index = PIIXFindModeInTable(mode, type);
	return (PIIX_CLK_TO_RCT(PIIXTimingTable[index].rct));
}

/*
 * Given a transfer mode/type, return the cycle time in ns.
 */
static __inline__
u_short
PIIXGetCycleForMode(u_char mode, PIIXTransferType_t type)
{
	u_char index = PIIXFindModeInTable(mode, type);	
	return (PIIXTimingTable[index].cycle);
}
