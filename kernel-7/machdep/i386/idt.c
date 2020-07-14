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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Interrupt descriptor table.
 *
 * HISTORY
 *
 * 3 April 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <architecture/i386/table.h>

#import <machdep/i386/idt.h>
#import <machdep/i386/desc_inline.h>

extern trp_divr(), trp_debg(), int__nmi(), trp_brpt(); 
extern trp_over(), flt_bnds(), flt_opcd(), flt_ncpr(); 
extern abt_dblf(), abt_over(), flt_btss(), flt_bseg();
extern flt_bstk(), flt_prot(), trp_0x0F(), flt_page();
extern flt_bcpr(), trp_0x11(), trp_0x12(), trp_0x13();
extern trp_0x14(), trp_0x15(), trp_0x16(), trp_0x17();
extern trp_0x18(), trp_0x19(), trp_0x1A(), trp_0x1B();
extern trp_0x1C(), trp_0x1D(), trp_0x1E(), trp_0x1F();
extern trp_0x20(), trp_0x21(), trp_0x22(), trp_0x23();
extern trp_0x24(), trp_0x25(), trp_0x26(), trp_0x27();
extern trp_0x28(), trp_0x29(), trp_0x2A(), trp_0x2B();
extern trp_0x2C(), trp_0x2D(), trp_0x2E(), trp_0x2F();
extern trp_0x30(), trp_0x31(), trp_0x32(), trp_0x33();
extern trp_0x34(), trp_0x35(), trp_0x36(), trp_0x37();
extern trp_0x38(), trp_0x39(), trp_0x3A(), trp_0x3B();
extern trp_0x3C(), trp_0x3D(), trp_0x3E(), trp_0x3F();

extern int_0x40(), int_0x41(), int_0x42(), int_0x43();
extern int_0x44(), int_0x45(), int_0x46(), int_0x47();
extern int_0x48(), int_0x49(), int_0x4A(), int_0x4B();
extern int_0x4C(), int_0x4D(), int_0x4E(), int_0x4F();
extern int_0x50(), int_0x51(), int_0x52(), int_0x53();
extern int_0x54(), int_0x55(), int_0x56(), int_0x57();
extern int_0x58(), int_0x59(), int_0x5A(), int_0x5B();
extern int_0x5C(), int_0x5D(), int_0x5E(), int_0x5F();
extern int_0x60(), int_0x61(), int_0x62(), int_0x63();
extern int_0x64(), int_0x65(), int_0x66(), int_0x67();
extern int_0x68(), int_0x69(), int_0x6A(), int_0x6B();
extern int_0x6C(), int_0x6D(), int_0x6E(), int_0x6F();
extern int_0x70(), int_0x71(), int_0x72(), int_0x73();
extern int_0x74(), int_0x75(), int_0x76(), int_0x77();
extern int_0x78(), int_0x79(), int_0x7A(), int_0x7B();
extern int_0x7C(), int_0x7D(), int_0x7E(), int_0x7F();
extern int_0x80(), int_0x81(), int_0x82(), int_0x83();
extern int_0x84(), int_0x85(), int_0x86(), int_0x87();
extern int_0x88(), int_0x89(), int_0x8A(), int_0x8B();
extern int_0x8C(), int_0x8D(), int_0x8E(), int_0x8F();
extern int_0x90(), int_0x91(), int_0x92(), int_0x93();
extern int_0x94(), int_0x95(), int_0x96(), int_0x97();
extern int_0x98(), int_0x99(), int_0x9A(), int_0x9B();
extern int_0x9C(), int_0x9D(), int_0x9E(), int_0x9F();
extern int_0xA0(), int_0xA1(), int_0xA2(), int_0xA3();
extern int_0xA4(), int_0xA5(), int_0xA6(), int_0xA7();
extern int_0xA8(), int_0xA9(), int_0xAA(), int_0xAB();
extern int_0xAC(), int_0xAD(), int_0xAE(), int_0xAF();
extern int_0xB0(), int_0xB1(), int_0xB2(), int_0xB3();
extern int_0xB4(), int_0xB5(), int_0xB6(), int_0xB7();
extern int_0xB8(), int_0xB9(), int_0xBA(), int_0xBB();
extern int_0xBC(), int_0xBD(), int_0xBE(), int_0xBF();
extern int_0xC0(), int_0xC1(), int_0xC2(), int_0xC3();
extern int_0xC4(), int_0xC5(), int_0xC6(), int_0xC7();
extern int_0xC8(), int_0xC9(), int_0xCA(), int_0xCB();
extern int_0xCC(), int_0xCD(), int_0xCE(), int_0xCF();
extern int_0xD0(), int_0xD1(), int_0xD2(), int_0xD3();
extern int_0xD4(), int_0xD5(), int_0xD6(), int_0xD7();
extern int_0xD8(), int_0xD9(), int_0xDA(), int_0xDB();
extern int_0xDC(), int_0xDD(), int_0xDE(), int_0xDF();
extern int_0xE0(), int_0xE1(), int_0xE2(), int_0xE3();
extern int_0xE4(), int_0xE5(), int_0xE6(), int_0xE7();
extern int_0xE8(), int_0xE9(), int_0xEA(), int_0xEB();
extern int_0xEC(), int_0xED(), int_0xEE(), int_0xEF();
extern int_0xF0(), int_0xF1(), int_0xF2(), int_0xF3();
extern int_0xF4(), int_0xF5(), int_0xF6(), int_0xF7();
extern int_0xF8(), int_0xF9(), int_0xFA(), int_0xFB();
extern int_0xFC(), int_0xFD(), int_0xFE(), int_0xFF();

void
locate_idt(
    vm_offset_t		base
)
{
    extern
	unsigned int	idt_base;
    extern
	unsigned short	idt_limit;
	
    idt_base = base;
    idt_limit = (IDTSZ * sizeof (idt_entry_t)) - 1;
    
    asm volatile("lidt %0"
		    :
		    : "m" (idt_limit));
}


typedef struct {
    vm_offset_t		offset;
    sel_t		seg;
    unsigned short	type	:5,
    			dpl	:2,
				:9;
} idt_pseudo_entry_t;

idt_pseudo_entry_t	idt_pseudo[];

void
idt_init(void)
{
    idt_pseudo_entry_t	tmp;
    int			i;

    for (i = 0; i < IDTSZ; i++) {
	tmp = idt_pseudo[i];
	if (tmp.type == DESC_TRAP_GATE)
	    make_trap_gate((trap_gate_t *)&idt[i],
					    tmp.seg,
					    tmp.offset,
					    tmp.dpl);
	else
	if (tmp.type == DESC_INTR_GATE)
	    make_intr_gate((intr_gate_t *)&idt[i],
					    tmp.seg,
					    tmp.offset,
					    tmp.dpl);
	else
	if (tmp.type == DESC_TASK_GATE)
	    make_task_gate((task_gate_t *)&idt[i],
					    tmp.seg,
					    tmp.offset,
					    tmp.dpl);
    }
    
    locate_idt((vm_offset_t) idt);
}

#define IPE(o, t, p)	\
    { (vm_offset_t)(o), KCS_SEL, (t), (p) }

#define TG	DESC_TRAP_GATE
#define IG	DESC_INTR_GATE
#define K	KERN_PRIV
#define U	USER_PRIV

idt_pseudo_entry_t	idt_pseudo[IDTSZ] = {
    IPE(trp_divr,	TG, K),
    IPE(trp_debg,	IG, K),
    IPE(int__nmi,	IG, K),
    IPE(trp_brpt,	TG, U),
    IPE(trp_over,	TG, U),
    IPE(flt_bnds,	TG, U),
    IPE(flt_opcd,	TG, K),
    IPE(flt_ncpr,	TG, K),
    IPE(abt_dblf,	TG, K),
    IPE(abt_over,	TG, K),
    IPE(flt_btss,	TG, K),
    IPE(flt_bseg,	TG, K),
    IPE(flt_bstk,	TG, K),
    IPE(flt_prot,	TG, K),
    IPE(flt_page,	TG, K),
    IPE(trp_0x0F,	TG, K),
    IPE(flt_bcpr,	TG, K),
    IPE(trp_0x11,	TG, K),
    IPE(trp_0x12,	TG, K),
    IPE(trp_0x13,	TG, K),
    IPE(trp_0x14,	TG, K),
    IPE(trp_0x15,	TG, K),
    IPE(trp_0x16,	TG, K),
    IPE(trp_0x17,	TG, K),
    IPE(trp_0x18,	TG, K),
    IPE(trp_0x19,	TG, K),
    IPE(trp_0x1A,	TG, K),
    IPE(trp_0x1B,	TG, K),
    IPE(trp_0x1C,	TG, K),
    IPE(trp_0x1D,	TG, K),
    IPE(trp_0x1E,	TG, K),
    IPE(trp_0x1F,	TG, K),
    IPE(trp_0x20,	TG, K),
    IPE(trp_0x21,	TG, K),
    IPE(trp_0x22,	TG, K),
    IPE(trp_0x23,	TG, K),
    IPE(trp_0x24,	TG, K),
    IPE(trp_0x25,	TG, K),
    IPE(trp_0x26,	TG, K),
    IPE(trp_0x27,	TG, K),
    IPE(trp_0x28,	TG, K),
    IPE(trp_0x29,	TG, K),
    IPE(trp_0x2A,	TG, K),
    IPE(trp_0x2B,	TG, K),
    IPE(trp_0x2C,	TG, K),
    IPE(trp_0x2D,	TG, K),
    IPE(trp_0x2E,	TG, K),
    IPE(trp_0x2F,	TG, K),
    IPE(trp_0x30,	TG, K),
    IPE(trp_0x31,	TG, K),
    IPE(trp_0x32,	TG, K),
    IPE(trp_0x33,	TG, K),
    IPE(trp_0x34,	TG, K),
    IPE(trp_0x35,	TG, K),
    IPE(trp_0x36,	TG, K),
    IPE(trp_0x37,	TG, K),
    IPE(trp_0x38,	TG, K),
    IPE(trp_0x39,	TG, K),
    IPE(trp_0x3A,	TG, K),
    IPE(trp_0x3B,	TG, K),
    IPE(trp_0x3C,	TG, K),
    IPE(trp_0x3D,	TG, K),
    IPE(trp_0x3E,	TG, K),
    IPE(trp_0x3F,	TG, K),

    IPE(int_0x40,	IG, K),
    IPE(int_0x41,	IG, K),
    IPE(int_0x42,	IG, K),
    IPE(int_0x43,	IG, K),
    IPE(int_0x44,	IG, K),
    IPE(int_0x45,	IG, K),
    IPE(int_0x46,	IG, K),
    IPE(int_0x47,	IG, K),
    IPE(int_0x48,	IG, K),
    IPE(int_0x49,	IG, K),
    IPE(int_0x4A,	IG, K),
    IPE(int_0x4B,	IG, K),
    IPE(int_0x4C,	IG, K),
    IPE(int_0x4D,	IG, K),
    IPE(int_0x4E,	IG, K),
    IPE(int_0x4F,	IG, K),
    IPE(int_0x50,	IG, K),
    IPE(int_0x51,	IG, K),
    IPE(int_0x52,	IG, K),
    IPE(int_0x53,	IG, K),
    IPE(int_0x54,	IG, K),
    IPE(int_0x55,	IG, K),
    IPE(int_0x56,	IG, K),
    IPE(int_0x57,	IG, K),
    IPE(int_0x58,	IG, K),
    IPE(int_0x59,	IG, K),
    IPE(int_0x5A,	IG, K),
    IPE(int_0x5B,	IG, K),
    IPE(int_0x5C,	IG, K),
    IPE(int_0x5D,	IG, K),
    IPE(int_0x5E,	IG, K),
    IPE(int_0x5F,	IG, K),
    IPE(int_0x60,	IG, K),
    IPE(int_0x61,	IG, K),
    IPE(int_0x62,	IG, K),
    IPE(int_0x63,	IG, K),
    IPE(int_0x64,	IG, K),
    IPE(int_0x65,	IG, K),
    IPE(int_0x66,	IG, K),
    IPE(int_0x67,	IG, K),
    IPE(int_0x68,	IG, K),
    IPE(int_0x69,	IG, K),
    IPE(int_0x6A,	IG, K),
    IPE(int_0x6B,	IG, K),
    IPE(int_0x6C,	IG, K),
    IPE(int_0x6D,	IG, K),
    IPE(int_0x6E,	IG, K),
    IPE(int_0x6F,	IG, K),
    IPE(int_0x70,	IG, K),
    IPE(int_0x71,	IG, K),
    IPE(int_0x72,	IG, K),
    IPE(int_0x73,	IG, K),
    IPE(int_0x74,	IG, K),
    IPE(int_0x75,	IG, K),
    IPE(int_0x76,	IG, K),
    IPE(int_0x77,	IG, K),
    IPE(int_0x78,	IG, K),
    IPE(int_0x79,	IG, K),
    IPE(int_0x7A,	IG, K),
    IPE(int_0x7B,	IG, K),
    IPE(int_0x7C,	IG, K),
    IPE(int_0x7D,	IG, K),
    IPE(int_0x7E,	IG, K),
    IPE(int_0x7F,	IG, K),
    IPE(int_0x80,	IG, K),
    IPE(int_0x81,	IG, K),
    IPE(int_0x82,	IG, K),
    IPE(int_0x83,	IG, K),
    IPE(int_0x84,	IG, K),
    IPE(int_0x85,	IG, K),
    IPE(int_0x86,	IG, K),
    IPE(int_0x87,	IG, K),
    IPE(int_0x88,	TG, K),
    IPE(int_0x89,	TG, K),
    IPE(int_0x8A,	TG, K),	
    IPE(int_0x8B,	TG, K),
    IPE(int_0x8C,	TG, K),
    IPE(int_0x8D,	TG, K),
    IPE(int_0x8E,	TG, K),
    IPE(int_0x8F,	TG, K),
    IPE(int_0x90,	TG, K),
    IPE(int_0x91,	TG, K),
    IPE(int_0x92,	TG, K),
    IPE(int_0x93,	TG, K),
    IPE(int_0x94,	TG, K),
    IPE(int_0x95,	TG, K),
    IPE(int_0x96,	TG, K),
    IPE(int_0x97,	TG, K),
    IPE(int_0x98,	TG, K),
    IPE(int_0x99,	TG, K),
    IPE(int_0x9A,	TG, K),
    IPE(int_0x9B,	TG, K),
    IPE(int_0x9C,	TG, K),
    IPE(int_0x9D,	TG, K),
    IPE(int_0x9E,	TG, K),
    IPE(int_0x9F,	TG, K),
    IPE(int_0xA0,	TG, K),
    IPE(int_0xA1,	TG, K),
    IPE(int_0xA2,	TG, K),
    IPE(int_0xA3,	TG, K),
    IPE(int_0xA4,	TG, K),
    IPE(int_0xA5,	TG, K),
    IPE(int_0xA6,	TG, K),
    IPE(int_0xA7,	TG, K),
    IPE(int_0xA8,	TG, K),
    IPE(int_0xA9,	TG, K),
    IPE(int_0xAA,	TG, K),
    IPE(int_0xAB,	TG, K),
    IPE(int_0xAC,	TG, K),
    IPE(int_0xAD,	TG, K),
    IPE(int_0xAE,	TG, K),
    IPE(int_0xAF,	TG, K),	
    IPE(int_0xB0,	TG, K),	
    IPE(int_0xB1,	TG, K),	
    IPE(int_0xB2,	TG, K),	
    IPE(int_0xB3,	TG, K),	
    IPE(int_0xB4,	TG, K),	
    IPE(int_0xB5,	TG, K),	
    IPE(int_0xB6,	TG, K),	
    IPE(int_0xB7,	TG, K),	
    IPE(int_0xB8,	TG, K),	
    IPE(int_0xB9,	TG, K),	
    IPE(int_0xBA,	TG, K),	
    IPE(int_0xBB,	TG, K),	
    IPE(int_0xBC,	TG, K),	
    IPE(int_0xBD,	TG, K),	
    IPE(int_0xBE,	TG, K),	
    IPE(int_0xBF,	TG, K),	
    IPE(int_0xC0,	TG, K),	
    IPE(int_0xC1,	TG, K),	
    IPE(int_0xC2,	TG, K),	
    IPE(int_0xC3,	TG, K),	
    IPE(int_0xC4,	TG, K),	
    IPE(int_0xC5,	TG, K),
    IPE(int_0xC6,	TG, K),
    IPE(int_0xC7,	TG, K),
    IPE(int_0xC8,	TG, K),
    IPE(int_0xC9,	TG, K),
    IPE(int_0xCA,	TG, K),
    IPE(int_0xCB,	TG, K),
    IPE(int_0xCC,	TG, K),
    IPE(int_0xCD,	TG, K),
    IPE(int_0xCE,	TG, K),
    IPE(int_0xCF,	TG, K),
    IPE(int_0xD0,	TG, K),
    IPE(int_0xD1,	TG, K),
    IPE(int_0xD2,	TG, K),
    IPE(int_0xD3,	TG, K),
    IPE(int_0xD4,	TG, K),
    IPE(int_0xD5,	TG, K),
    IPE(int_0xD6,	TG, K),
    IPE(int_0xD7,	TG, K),
    IPE(int_0xD8,	TG, K),
    IPE(int_0xD9,	TG, K),
    IPE(int_0xDA,	TG, K),
    IPE(int_0xDB,	TG, K),
    IPE(int_0xDC,	TG, K),
    IPE(int_0xDD,	TG, K),
    IPE(int_0xDE,	TG, K),
    IPE(int_0xDF,	TG, K),
    IPE(int_0xE0,	TG, K),
    IPE(int_0xE1,	TG, K),
    IPE(int_0xE2,	TG, K),
    IPE(int_0xE3,	TG, K),
    IPE(int_0xE4,	TG, K),
    IPE(int_0xE5,	TG, K),
    IPE(int_0xE6,	TG, K),
    IPE(int_0xE7,	TG, K),
    IPE(int_0xE8,	TG, K),
    IPE(int_0xE9,	TG, K),
    IPE(int_0xEA,	TG, K),
    IPE(int_0xEB,	TG, K),
    IPE(int_0xEC,	TG, K),
    IPE(int_0xED,	TG, K),
    IPE(int_0xEE,	TG, K),
    IPE(int_0xEF,	TG, K),
    IPE(int_0xF0,	TG, K),
    IPE(int_0xF1,	TG, K),
    IPE(int_0xF2,	TG, K),
    IPE(int_0xF3,	TG, K),
    IPE(int_0xF4,	TG, K),
    IPE(int_0xF5,	TG, K),
    IPE(int_0xF6,	TG, K),
    IPE(int_0xF7,	TG, K),
    IPE(int_0xF8,	TG, K),
    IPE(int_0xF9,	TG, K),
    IPE(int_0xFA,	TG, K),
    IPE(int_0xFB,	TG, K),
    IPE(int_0xFC,	TG, K),
    IPE(int_0xFD,	TG, K),
    IPE(int_0xFE,	TG, K),
    IPE(int_0xFF,	TG, K),
};

idt_t *		idt = (idt_entry_t *)idt_pseudo;

void
idt_copy(
    vm_offset_t		new_idt,
    vm_size_t		length
)
{
    if (length < (IDTSZ * sizeof (idt_entry_t)))
	panic("idt_copy");

    (void) memcpy((void *)new_idt, idt, IDTSZ * sizeof (idt_entry_t));
    (vm_offset_t)idt = new_idt;

    locate_idt((vm_offset_t) idt);
}
