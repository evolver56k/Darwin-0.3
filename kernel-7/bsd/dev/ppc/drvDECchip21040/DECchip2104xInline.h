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
 * Copyright (c) 1995-1996 NeXT Software, Inc.
 *
 * Inline definitions for the DECchip 21X40.
 *
 * HISTORY
 *
 * 26-Apr-95	Rakesh Dubey (rdubey) at NeXT 
 *	Created.
 */

#import <machdep/ppc/proc_reg.h>
#import "DECchip2104xRegisters.h"

//#define DEBUG

static __inline__
unsigned int readCsr(IOPPCAddress base, unsigned short offset)
{
    unsigned int data;
//kprintf("R%8x:", base + offset);
    data = lwbrx(base + offset);
//kprintf("%8x ", data);
    
    return data;
}

static __inline__
void writeCsr(IOPPCAddress base, unsigned short offset, 
		unsigned int value)
{
//kprintf("W%8x:%8x ", base + offset, value);
    stwbrx(value, base + offset);
    eieio();
}

static __inline__
void getStationAddress(IOPPCAddress base, enet_addr_t *ea)
{
    int i;
    csrRegUnion reg;
    
    writeCsr(base, DEC_21X40_CSR9, 0x0);
    for (i = 0; i < sizeof (*ea); i++)	{
    	do	{
	    reg.data = readCsr(base, DEC_21X40_CSR9);
	    IOSleep(1);
	} while (reg.csr90.dtnv == YES);
	ea->ea_byte[i] = reg.csr90.dt;
    }
}


/*
 * If this procedure for reading EEPROM seems arcane to you, see the DEC's
 * Hardware Reference Manual for DECchip21140. 
 */
static __inline__
unsigned char clock_out_bit(IOPPCAddress base)
{
    csrRegUnion reg;
    unsigned char val;
    
    reg.data = 0x00004800;
    
    reg.csr91.scs = 1; reg.csr91.sclk = 1;
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);
    
    reg.data = readCsr(base, DEC_21X40_CSR9);
    IODelay(250);
    val = reg.csr91.sdo;

    reg.data = 0x00004800;
    reg.csr91.scs = 1; reg.csr91.sclk = 0;
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);
    
    return val;
}

static __inline__
void clock_in_bit(IOPPCAddress base , unsigned int val)
{
    csrRegUnion reg;
    
    reg.data = 0x00004800;
    
    if (val != 0 && val != 1)	{
    	IOLog("bogus data in clock_in_bit\n");
	return;
    }
    
    reg.csr91.sdi = val;
    
    reg.csr91.scs = 1; reg.csr91.sclk = 0;
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);
    
    reg.csr91.scs = 1; reg.csr91.sclk = 1;
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);

    reg.csr91.scs = 1; reg.csr91.sclk = 0;
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);
}

static __inline__
void reset_and_select_srom(IOPPCAddress base)
{
    csrRegUnion reg;
    
    reg.data = 0x00004800;
    
    /* first reset */
    writeCsr(base, DEC_21X40_CSR9, reg.data);
    IODelay(250);
    
    /* select the serial rom */
    clock_in_bit(base, 0);
    
    /* send it the read command (110) */
    clock_in_bit(base, 1);
    clock_in_bit(base, 1);
    clock_in_bit(base, 0);
}

static __inline__
unsigned short read_srom(IOPPCAddress base, unsigned int addr,
	unsigned int addr_len)
{
    unsigned short data, val;
    int i;
    
    /* send out the address we want to read from */
    for (i = 0; i < addr_len; i++)	{
	val = addr >> (addr_len-i-1);
	clock_in_bit(base, val & 1);
    }
    
    /* Now read in the 16-bit data */
    data = 0;
    for (i = 0; i < 16; i++)	{
	val = clock_out_bit(base);
	data <<= 1;
	data |= val;
    }
    
    return data;
}

static __inline__
unsigned int
_atoi(const char *s)
{
    char   *cptr;
    unsigned int val;

    cptr = (char *)s;
    while (*cptr && ((*cptr == ' ') || (*cptr == '\t') || (*cptr == '\n')))
	cptr++;

    val = 0;
    if (*cptr == '\0')
    	return 0;
		
    while ((*cptr != ' ') && (*cptr != '\t') && (*cptr != '\n'))	{
	
	if ((*cptr >= '0') && (*cptr <= '9'))
	    val = val * 10 + (*cptr - '0');
	    
	++cptr;
	
	if (*cptr == '\0')
	    break;
    }
    
    return val;
}


/*
 * Useful for testing. 
 */
static __inline__
void _dump_srom(IOPPCAddress base, unsigned char sromAddressBits)
{
    unsigned short data;
    int i;
	
    for (i = 0; i < 128; i++)	{
	reset_and_select_srom(base);
	data = read_srom(base, i, sromAddressBits);
	IOLog("%x = %x ", i, data);
	if (i % 10 == 0) IOLog("\n");
    }
}



