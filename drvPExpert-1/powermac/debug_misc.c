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
#include <machdep/ppc/proc_reg.h>
#include <stdarg.h>
#include <machdep/ppc/boot.h>
#include <machdep/ppc/DeviceTree.h>

#if DEBUG

/* Local routines */
void print_bat(int d, int n, bat_t *bat);
void dump_bats();
void dump_msr();
void dump_sdr1();
void print_sr();
void dump_segment_registers();
void dump_hid0();
void dump_processor_info();
void dump_regions();
void dump_mbuf();
static void dumpTree();

void
print_bat(int d, int n, bat_t *bat)
{
	if (d != 0) {
		printf("DBAT%d: ", n);
	} else {
		printf("IBAT%d: ", n);
	}
	printf("0x%08x.%08x  ", bat->upper.word, bat->lower.word);
	if (bat->upper.word == 0) {
		printf("Not set.\n");
	} else {
		printf("bepi<< %x, bl %x, Vs %d, Vp %d, bprn<< %x, wimg %x, pp %d\n",
			bat->upper.bits.bepi<<1,
			bat->upper.bits.bl,
			bat->upper.bits.vs,
			bat->upper.bits.vp,
			bat->lower.bits.brpn<<1,
			bat->lower.bits.wimg,
			bat->lower.bits.pp);
	}
}

void
dump_bats()
{
	bat_t		      bat;

#ifdef notdef_next
#define PRINT_DBAT(n)	mfdbatl(bat.lower.word, n); mfdbatu(bat.upper.word, n); print_bat(1, n, &bat)
#else /* notdef_next */
#define PRINT_DBAT(n)	mfspr(bat.upper.word, 536+2*n);mfspr(bat.lower.word, 537+2*n);print_bat(1,n,&bat);
#endif

#define PRINT_IBAT(n)	mfibatl(bat.lower.word, n); mfibatu(bat.upper.word, n); print_bat(0, n, &bat)

	PRINT_DBAT(0);
	PRINT_DBAT(1);
	PRINT_DBAT(2);
	PRINT_DBAT(3);
	PRINT_IBAT(0);
	PRINT_IBAT(1);
	PRINT_IBAT(2);
	PRINT_IBAT(3);
}

typedef	union {
	unsigned int word;
	struct {
		unsigned int reserved1	: 13;
		unsigned int pow	: 1;
		unsigned int tgpr	: 1;
		unsigned int ile	: 1;
		unsigned int ee		: 1;
		unsigned int pr		: 1;
		unsigned int fp		: 1;
		unsigned int me		: 1;
		unsigned int fe0	: 1;
		unsigned int se		: 1;
		unsigned int be		: 1;
		unsigned int fe1	: 1;
		unsigned int reserved2	: 1;
		unsigned int ip		: 1;
		unsigned int ir		: 1;
		unsigned int dr		: 1;
		unsigned int reserved3	: 2;
		unsigned int ri		: 1;
		unsigned int le		: 1;
	} bits;
} msr_t;

void
dump_msr()
{
	msr_t	value;

	value.word = mfmsr();
	printf("msr: 0x%08x ", value.word);
#define PRINT_MSR_BIT(name,string)	if (value.bits.name) {printf(string);}
	PRINT_MSR_BIT(pr,"pr ");

	PRINT_MSR_BIT(ir,"ir ");
	PRINT_MSR_BIT(dr,"dr ");

	PRINT_MSR_BIT(ip,"ip ");
	PRINT_MSR_BIT(ee,"ee ");
	PRINT_MSR_BIT(me,"me ");

	PRINT_MSR_BIT(pow,"pow ");
	PRINT_MSR_BIT(fp,"fp ");

	PRINT_MSR_BIT(le,"le ");
	PRINT_MSR_BIT(ile,"ile ");

	PRINT_MSR_BIT(fe0,"fe0 ");
	PRINT_MSR_BIT(fe1,"fe1 ");

	PRINT_MSR_BIT(tgpr,"tgpr ");
	PRINT_MSR_BIT(ri,"ri ");

	PRINT_MSR_BIT(se,"se ");
	PRINT_MSR_BIT(be,"be ");
	printf("\n");
}

void
dump_sdr1()
{
	sdr1_t	value;

	mfspr(value.word, 25);
	printf("sdr1:  0x%08x  htaborg=%x htabmask=%x\n",
		value.word, value.bits.htaborg, value.bits.htabmask);
}

typedef	union {
	unsigned int word;
	struct {
		unsigned int T	: 1;
		unsigned int Ks	: 1;
		unsigned int Kp	: 1;
		unsigned int fill1	: 29;
	} bits;
	struct {
		unsigned int fill2	: 3;
		unsigned int N	: 1;
		unsigned int fill3	: 4;
		unsigned int vsid	: 24;
	} T0;
	struct {
		unsigned int fill4	: 3;
		unsigned int buid	: 9;
		unsigned int info	: 20;
	} T1;
} sr_t;

void
print_sr(int n, sr_t value)
{
#define PRINT_SR_BIT(section,name,string)	if (value.section.name) {printf(string);}
	printf("sr%d: 0x%08x  ", n, value.word);
	PRINT_SR_BIT(bits,Ks,"Ks ");
	PRINT_SR_BIT(bits,Kp,"Kp ");
	if (value.bits.T) {
		printf("buid=%x info=%x\n", value.T1.buid, value.T1.info);
	} else {
		PRINT_SR_BIT(T0,N,"N ");
		printf("vsid=%x\n", value.T0.vsid);
	}
}

void
dump_segment_registers()
{
	sr_t	value;

#define PRINT_SR(n)	mfsr(value.word, n); print_sr(n, value)
	PRINT_SR(0);
	PRINT_SR(1);
	PRINT_SR(2);
	PRINT_SR(3);
	PRINT_SR(4);
	PRINT_SR(5);
	PRINT_SR(6);
	PRINT_SR(7);
	PRINT_SR(8);
	PRINT_SR(9);
	PRINT_SR(10);
	PRINT_SR(11);
	PRINT_SR(12);
	PRINT_SR(13);
	PRINT_SR(14);
	PRINT_SR(15);
}

typedef	union {
	unsigned int word;
	struct {
		unsigned int emcp		: 1;	/* 0 */
		unsigned int ecpc		: 1;	/* 1 */
		unsigned int emc_abp		: 1;	/* 2 */
		unsigned int emc_dbp		: 1;	/* 3 */
		unsigned int reserved4		: 3;	/* 4,5,6 */
		unsigned int dsrhsr		: 1;	/* 7 */
		unsigned int reserved8		: 4;	/* 8,9,10,11 */
		unsigned int reserved12		: 1;	/* 12 - 604e*/
		unsigned int reserved13		: 2;	/* 13,14 */
		unsigned int nhr		: 1;	/* 15 */
		unsigned int ice		: 1;	/* 16 */
		unsigned int dce		: 1;	/* 17 */
		unsigned int icl		: 1;	/* 18 */
		unsigned int dcl		: 1;	/* 19 */
		unsigned int icia		: 1;	/* 20 */
		unsigned int dcia		: 1;	/* 21 */
		unsigned int reserved22		: 1;	/* 22 */
		unsigned int cife		: 1;	/* 23 - 604e*/
		unsigned int sied		: 1;	/* 24 */
		unsigned int reserved25		: 4;	/* 25,26,27,28 */
		unsigned int bhte		: 1;	/* 29 */
		unsigned int btacd		: 1;	/* 30 - 604e*/
		unsigned int reserved31		: 1;	/* 31 */
	} bits;
} hid0_t;

void
dump_hid0()
{
	hid0_t	reg;

	mfspr(reg, 1008);

	printf("hid0: 0x%08x ", reg.word);
#define PRINT_HID0_BIT(name,string)	if (reg.bits.name) {printf(string);}
	PRINT_HID0_BIT(emcp,"emcp ");
	PRINT_HID0_BIT(ecpc,"ecpc ");
	PRINT_HID0_BIT(emc_abp,"emc_abp ");
	PRINT_HID0_BIT(emc_dbp,"emc_dbp ");
	PRINT_HID0_BIT(dsrhsr,"dsrhsr ");
	PRINT_HID0_BIT(nhr,"nhr ");
	PRINT_HID0_BIT(ice,"ice ");
	PRINT_HID0_BIT(dce,"dce ");
	PRINT_HID0_BIT(icl,"icl ");
	PRINT_HID0_BIT(dcl,"dcl ");
	PRINT_HID0_BIT(icia,"icia ");
	PRINT_HID0_BIT(dcia,"dcia ");
	PRINT_HID0_BIT(cife,"cife ");
	PRINT_HID0_BIT(sied,"sied ");
	PRINT_HID0_BIT(bhte,"bhte ");
	PRINT_HID0_BIT(btacd,"btacd ");
	printf("\n");
}

void
dump_processor_info()
{
	dump_msr();
	dump_bats();
	dump_hid0();
	dump_sdr1();
	dump_segment_registers();
}

#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>
#include <vm/pmap.h>

void
dump_regions()
{
	int i;
	extern		struct mem_region mem_region[];
	extern int	num_regions;

	printf("pmap_mem_regions:\n");
	printf("pmap_mem_regions_count = %d\n", pmap_mem_regions_count);
	for (i = 0; i < pmap_mem_regions_count; i++) {
		printf("%d: start %x, end %x, phys_table %x\n",
			i,
			pmap_mem_regions[i].start,
			pmap_mem_regions[i].end,
			pmap_mem_regions[i].phys_table);
	}
	printf("free_regions_count = %d\n", free_regions_count);
	for (i = 0; i < free_regions_count; i++) {
		printf("%d: start %x, end %x, phys_table %x\n",
			i,
			free_regions[i].start,
			free_regions[i].end,
			free_regions[i].phys_table);
	}
	printf("mem_region:\n");
	printf("num_regions = %d\n", num_regions);
	for (i = 0; i < num_regions; i++) {
		printf("%d: base %x, first %x, end %x\n",
			i,
			mem_region[i].base_phys_addr,
			mem_region[i].first_phys_addr,
			mem_region[i].last_phys_addr);
	}
}

#include <bsd/sys/param.h>
#include <bsd/sys/mbuf.h>

void
dump_mbuf(struct mbuf *m)
{
	printf("m = 0x%08x   m_next=0x%08x, m_nextpkt=0x%08x\n",
		m, m->m_next, m->m_nextpkt);
	printf("    m_len=0x%08x, m_data=0x%08x, m_type=%d, m_flags=0x%x\n",
		m->m_len, m->m_data, m->m_type, m->m_flags);
}

static int indent = 0;
static char *cursorP;

static void dumpTree ()
{
	DeviceTreeNode *nodeP = (DeviceTreeNode *) cursorP;
	int k;

	if (nodeP->nProperties == 0) return;	// End of the list of nodes
	cursorP = (char *) (nodeP + 1);

	// Dump properties (only name for now)
	for (k = 0; k < nodeP->nProperties; ++k) {
		DeviceTreeNodeProperty *propP = (DeviceTreeNodeProperty *) cursorP;

		cursorP += sizeof (*propP) + ((propP->length + 3) & -4);

		if (strcmp (propP->name, "name") == 0) {
			int n;

			// I know printf can do all of this in one call but my printf is lame
			for (n = 0; n < indent; ++n) printf ("  ");
			printf ("%s\n", propP + 1);
		}
	}

	// Dump child nodes
	++indent;
	for (k = 0; k < nodeP->nChildren; ++k) dumpTree ();
	--indent;
}
#endif

