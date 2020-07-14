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

#ifndef __DEBUG_H__
#define __DEBUG_H__


#define D_L_FATAL			0x00000001
#define D_L_ERROR			0x00000002
#define D_L_WARNING			0x00000004
#define D_L_INFO			0x00000008
#define D_L_VERBOSE			0x00000010
#define D_L_STARTUP     	0x00000020
#define D_L_STARTUP_LOW		0x00000040
#define D_L_SHUTDN			0x00000080
#define D_L_SHUTDN_LOW		0x00000100
#define D_L_INPUT			0x00000200
#define D_L_OUTPUT			0x00000400
#define D_L_STATS			0x00000800
#define D_L_STATE_CHG		0x00001000		/* re-aarp, ifState etc. */
#define D_L_ROUTING			0x00002000
#define D_L_DNSTREAM		0x00004000
#define D_L_UPSTREAM		0x00008000
#define D_L_STARTUP_INFO	0x00010000
#define D_L_SHUTDN_INFO		0x00020000
#define D_L_ROUTING_AT		0x00040000		/* atalk address routing */
#define D_L_USR1			0x01000000
#define D_L_USR2			0x02000000
#define D_L_USR3			0x04000000
#define D_L_USR4			0x08000000
#define D_L_TRACE			0x10000000


#define D_M_PAT				0x00000001
#define D_M_PAT_LOW			0x00000002
#define D_M_ELAP			0x00000004
#define D_M_ELAP_LOW		0x00000008
#define D_M_DDP				0x00000010
#define D_M_DDP_LOW			0x00000020
#define D_M_NBP				0x00000040
#define D_M_NBP_LOW			0x00000080
#define D_M_ZIP				0x00000100
#define D_M_ZIP_LOW			0x00000200
#define D_M_RTMP			0x00000400
#define D_M_RTMP_LOW		0x00000800
#define D_M_ATP				0x00001000
#define D_M_ATP_LOW			0x00002000
#define D_M_ADSP			0x00004000
#define D_M_ADSP_LOW		0x00008000
#define D_M_AEP				0x00010000
#define D_M_AARP			0x00020000
#define D_M_ASP				0x00040000
#define D_M_ASP_LOW			0x00080000
#define D_M_AURP				0x00100000
#define D_M_AURP_LOW			0x00200000
#define D_M_TRACE			0x10000000


#define DBG_SET				1		/* cmds for kernelDbg() */
#define DBG_SET_FROM_FILE	2
#define DBG_GET				3

#define DEBUG_STAT_FILE		"/tmp/dbgbits.dat"

	/* macros for working with atp data at the lap level. 
	 * These are for tracehook performance measurements only!!!
	 * It is assumed that the ddp & atp headers are at the top of the
	 * mblk, occupy contiguous memory and the atp headers are of the
	 * extended type only.
	 */

typedef struct dbgBits {
	unsigned long 	dbgMod;	/* debug module bitmap (used in dPrintf) */
	unsigned long 	dbgLev;	/* debug level bitmap */
} dbgBits_t;

extern dbgBits_t 	dbgBits;

	/* macros for debugging */
#ifdef DEBUG
#define dPrintf(mod, lev, p) \
	if (((mod) & dbgBits.dbgMod) && ((lev) & dbgBits.dbgLev)) {\
		 kprintf p;  \
	}
#define PRINTF		printf
#define FPRINTF		fprintf
#else
#define dPrintf(mod, lev, p)
#define PRINTF
#define FPRINTF
#endif
#endif /* __DEBUG_H__ */

