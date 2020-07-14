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

#ifndef _POWERMAC_DBDMA_H_
#define _POWERMAC_DBDMA_H_

#define	DBDMA_CMD_OUT_MORE	0
#define	DBDMA_CMD_OUT_LAST	1
#define	DBDMA_CMD_IN_MORE	2
#define	DBDMA_CMD_IN_LAST	3
#define	DBDMA_CMD_STORE_QUAD	4
#define	DBDMA_CMD_LOAD_QUAD	5
#define	DBDMA_CMD_NOP		6
#define	DBDMA_CMD_STOP		7

/* Keys */

#define	DBDMA_KEY_STREAM0	0
#define	DBDMA_KEY_STREAM1	1
#define	DBDMA_KEY_STREAM2	2
#define	DBDMA_KEY_STREAM3	3

/* value 4 is reserved */
#define	DBDMA_KEY_REGS		5
#define	DBDMA_KEY_SYSTEM	6
#define	DBDMA_KEY_DEVICE	7

#define	DBDMA_INT_NEVER		0
#define	DBDMA_INT_IF_TRUE	1
#define	DBDMA_INT_IF_FALSE	2
#define	DBDMA_INT_ALWAYS	3

#define	DBDMA_BRANCH_NEVER	0
#define	DBDMA_BRANCH_IF_TRUE	1
#define	DBDMA_BRANCH_IF_FALSE	2
#define	DBDMA_BRANCH_ALWAYS	3

#define	DBDMA_WAIT_NEVER	0
#define	DBDMA_WAIT_IF_TRUE	1
#define DBDMA_WAIT_IF_FALSE	2
#define	DBDMA_WAIT_ALWAYS	3


/* DBDMA Channels Macros */

#define	DBDMA_CURIO_SCSI  (powermac_dbdma_channels->dbdma_channel_curio)
#define	DBDMA_MESH_SCSI   (powermac_dbdma_channels->dbdma_channel_mesh)
#define	DBDMA_FLOPPY	  (powermac_dbdma_channels->dbdma_channel_floppy)
#define	DBDMA_ETHERNET_TX (powermac_dbdma_channels->dbdma_channel_ethernet_tx)
#define	DBDMA_ETHERNET_RV (powermac_dbdma_channels->dbdma_channel_ethernet_rx)
#define	DBDMA_SCC_XMIT_A  (powermac_dbdma_channels->dbdma_channel_scc_xmit_a)
#define	DBDMA_SCC_RECV_A  (powermac_dbdma_channels->dbdma_channel_scc_recv_a)
#define	DBDMA_SCC_XMIT_B  (powermac_dbdma_channels->dbdma_channel_scc_xmit_b)
#define	DBDMA_SCC_RECV_B  (powermac_dbdma_channels->dbdma_channel_scc_recv_b)
#define	DBDMA_AUDIO_OUT	  (powermac_dbdma_channels->dbdma_channel_audio_out)
#define	DBDMA_AUDIO_IN	  (powermac_dbdma_channels->dbdma_channel_audio_in)
#define	DBDMA_IDE0	  (powermac_dbdma_channels->dbdma_channel_ide0)
#define	DBDMA_IDE1	  (powermac_dbdma_channels->dbdma_channel_ide1)

/* Control register values (in little endian) */

#define	DBDMA_STATUS_MASK	0x000000ff	/* Status Mask */
#define	DBDMA_CNTRL_BRANCH	0x00000100
				/* 0x200 reserved */
#define	DBDMA_CNTRL_ACTIVE	0x00000400
#define	DBDMA_CNTRL_DEAD	0x00000800
#define	DBDMA_CNTRL_WAKE	0x00001000
#define	DBDMA_CNTRL_FLUSH	0x00002000
#define	DBDMA_CNTRL_PAUSE	0x00004000
#define	DBDMA_CNTRL_RUN		0x00008000

#define	DBDMA_SET_CNTRL(x)	( ((x) | (x) << 16) )
#define	DBDMA_CLEAR_CNTRL(x)	( (x) << 16)


#define	DBDMA_REGMAP(channel) \
		(dbdma_regmap_t *)((v_u_char *) POWERMAC_IO(PCI_DMA_BASE_PHYS) \
				+ (channel << 8))


/* powermac_dbdma_channels hold the physical channel numbers for
 * each dbdma device
 */

typedef struct powermac_dbdma_channels {
        int             dbdma_channel_curio;
        int             dbdma_channel_mesh;
        int             dbdma_channel_floppy;
        int             dbdma_channel_ethernet_tx;
        int             dbdma_channel_ethernet_rx;
        int             dbdma_channel_scc_xmit_a;
        int             dbdma_channel_scc_recv_a;
        int             dbdma_channel_scc_xmit_b;
        int             dbdma_channel_scc_recv_b;
        int             dbdma_channel_audio_out;
        int             dbdma_channel_audio_in;
        int             dbdma_channel_ide0;
        int             dbdma_channel_ide1;
} powermac_dbdma_channels_t;

extern powermac_dbdma_channels_t *powermac_dbdma_channels;

/* This struct is layout in little endian format */

struct dbdma_command {
	unsigned long	d_cmd_count;
	unsigned long	d_address;
	unsigned long	d_cmddep;
	unsigned long	d_status_resid;
};

typedef struct dbdma_command dbdma_command_t;

#define	DBDMA_BUILD(d, cmd, key, count, address, interrupt, wait, branch) {\
		DBDMA_ST4_ENDIAN(&d->d_address, address); \
		(d)->d_status_resid = 0; \
		(d)->d_cmddep = 0; \
		DBDMA_ST4_ENDIAN(&d->d_cmd_count, \
				((cmd) << 28) | ((key) << 24) |\
				((interrupt) << 20) |\
				((branch) << 18) | ((wait) << 16) | \
				(count)); \
	}

static __inline__ unsigned 
endianswap32bit(unsigned value)
{
    register unsigned	temp;
	
    temp = ((value & 0xFF000000) >> 24);
    temp |= ((value & 0x00FF0000) >> 8);
    temp |= ((value & 0x0000FF00) << 8);
    temp |= ((value & 0x000000FF) << 24);
    return (temp);
}

static __inline__ void
dbdma_st4_endian(volatile unsigned long *a, unsigned long x)
{
    *a = endianswap32bit(x);
#if 0
	__asm__ volatile
		("stwbrx %0,0,%1" : : "r" (x), "r" (a) : "memory");
#endif
    return;
}

static __inline__ unsigned long
dbdma_ld4_endian(volatile unsigned long *a)
{
#if 0
    unsigned long swap;
#endif

    return (endianswap32bit(*a));
#if 0
	__asm__ volatile
		("lwbrx %0,0,%1" :  "=r" (swap) : "r" (a));

	return	swap;
#endif
}

#define	DBDMA_LD4_ENDIAN(a) 	dbdma_ld4_endian(a)
#define	DBDMA_ST4_ENDIAN(a, x) 	dbdma_st4_endian(a, x)

/*
 * DBDMA Channel layout
 *
 * NOTE - This structure is in little-endian format. 
 */

struct dbdma_regmap {
	volatile unsigned long	d_control;	/* Control Register */
	volatile unsigned long	d_status;	/* DBDMA Status Register */
	volatile unsigned long	d_cmdptrhi;	/* MSB of command pointer (not used yet) */
	volatile unsigned long	d_cmdptrlo;	/* LSB of command pointer */
	volatile unsigned long	d_intselect;	/* Interrupt Select */
	volatile unsigned long	d_branch;	/* Branch selection */
	volatile unsigned long	d_wait;		/* Wait selection */
	volatile unsigned long	d_transmode;	/* Transfer modes */
	volatile unsigned long	d_dataptrhi;	/* MSB of Data Pointer */
	volatile unsigned long	d_dataptrlo;	/* LSB of Data Pointer */
	volatile unsigned long	d_reserved;	/* Reserved for the moment */
	volatile unsigned long	d_branchptrhi;	/* MSB of Branch Pointer */
	volatile unsigned long	d_branchptrlo;	/* LSB of Branch Pointer */
	/* The remaining fields are undefinied and unimplemented */
};

typedef struct dbdma_regmap dbdma_regmap_t;

/* DBDMA routines */

void	dbdma_start(int channel, dbdma_command_t *commands);
void	dbdma_stop(int channel);	
void	dbdma_flush(int channel);
void	dbdma_reset(int channel);
void	dbdma_continue(int channel);
void	dbdma_pause(int channel);

dbdma_command_t	*dbdma_alloc(int);	/* Allocate command structures */

#endif /* !defined(_POWERMAC_DBDMA_H_) */
