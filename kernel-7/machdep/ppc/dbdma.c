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

#import <mach/mach_types.h>
#import <ppc/powermac.h>
#import <ppc/proc_reg.h>
#import "dbdma.h"
#import <sys/param.h>

#define PG_SIZE		NBPG
static unsigned long dbdma_area[(PG_SIZE + 16)/sizeof(long)];

//#define KVTOPHYS	kvtophys

static __inline__ vm_offset_t
KVTOPHYS(vm_offset_t v)
{
    return (v);
}

static int	dbdma_alloc_index = 0;
dbdma_command_t	*dbdma_alloc_commands = 0;

void
dbdma_start(int channel, dbdma_command_t *commands)
{
	unsigned long addr = KVTOPHYS((vm_offset_t) commands);
	volatile dbdma_regmap_t	*dmap = DBDMA_REGMAP(channel);

	eieio();
	if (addr & 0xf)
		panic("dbdma_start command structure not 16-byte aligned");

	dmap->d_intselect = 0xff; /* Endian magic - clear out interrupts */
	eieio();

	DBDMA_ST4_ENDIAN(&dmap->d_control,
			 DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_ACTIVE
					   | DBDMA_CNTRL_DEAD
					   | DBDMA_CNTRL_WAKE
					   | DBDMA_CNTRL_FLUSH
					   | DBDMA_CNTRL_PAUSE
					   | DBDMA_CNTRL_RUN)); eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		eieio();

	dmap->d_cmdptrhi = 0;	eieio();/* 64-bit not yet */
	DBDMA_ST4_ENDIAN(&dmap->d_cmdptrlo, addr); eieio();

	DBDMA_ST4_ENDIAN(&dmap->d_control, 
			 DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN | DBDMA_CNTRL_WAKE));
	eieio();

}

void
dbdma_stop(int channel)
{
	volatile dbdma_regmap_t *dmap = DBDMA_REGMAP(channel);

	eieio();
	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_RUN) |
			  DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH)); eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & (DBDMA_CNTRL_ACTIVE|DBDMA_CNTRL_FLUSH))
		eieio();
}

void
dbdma_flush(int channel)
{
	volatile dbdma_regmap_t *dmap = DBDMA_REGMAP(channel);


	eieio();
	DBDMA_ST4_ENDIAN(&dmap->d_control,DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH));
	eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & (DBDMA_CNTRL_FLUSH))
		eieio();
}


void
dbdma_reset(int channel)
{
	volatile dbdma_regmap_t *dmap = DBDMA_REGMAP(channel);


	eieio();
	DBDMA_ST4_ENDIAN(&dmap->d_control,
			 DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_ACTIVE
					   | DBDMA_CNTRL_DEAD
					   | DBDMA_CNTRL_WAKE
					   | DBDMA_CNTRL_FLUSH
					   | DBDMA_CNTRL_PAUSE
					   | DBDMA_CNTRL_RUN)); eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_RUN)
		eieio();
}

void
dbdma_continue(int channel)
{
	volatile dbdma_regmap_t *dmap = DBDMA_REGMAP(channel);

	eieio();
	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN|DBDMA_CNTRL_WAKE) | DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_PAUSE|DBDMA_CNTRL_DEAD));
	eieio();
}

void
dbdma_pause(int channel)
{
	volatile dbdma_regmap_t *dmap = DBDMA_REGMAP(channel);

	eieio();
	DBDMA_ST4_ENDIAN(&dmap->d_control,DBDMA_SET_CNTRL(DBDMA_CNTRL_PAUSE));
	eieio();

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		eieio();
}

dbdma_command_t	*
dbdma_alloc(int count)
{
	dbdma_command_t	*dbdmap;

#define ALIGN_MASK	0xfffffff0UL

#if 0
	if (dbdma_alloc_index == 0) 
		dbdma_alloc_commands = (dbdma_command_t *) io_map(0, PAGE_SIZE);
#endif
	if (dbdma_alloc_index == 0) {
	    dbdma_alloc_commands = (dbdma_command_t *)
		((unsigned long)(dbdma_area + 15) & ALIGN_MASK);
	}
	
	if ((dbdma_alloc_index+count) >= PG_SIZE / sizeof(dbdma_command_t)) 
		panic("Too many dbdma command structures!");

	dbdmap = &dbdma_alloc_commands[dbdma_alloc_index];
	dbdma_alloc_index += count;
	return	dbdmap;
}
