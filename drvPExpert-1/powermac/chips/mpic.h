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

#ifndef _INTERRUPT_MPIC_H_
#define _INTERRUPT_MPIC_H_

#include <sys/types.h>
#include <ppc/spl.h>

/* Physical address and size of IO control registers region */

/* Special things for Power Express / MPIC */

/* Stuff for Fat Man */
#define FM_MPIC_CTRL_REG    0x00000160
#define FM_MPIC_CTRL  ((v_u_long *)(POWERMAC_IO(powermac_io_info.mem_cntlr_base_phys + FM_MPIC_CTRL_REG)))

#define FM_MPIC_ENABLE      0x80000000
#define FM_MPIC_INT_SEL     0x60000000

/* Stuff for MPIC */
#define MPIC_SIZE              0x80000  /* 512k */

#define MPIC_GLOBAL_CFG_REG 0x00001020
#define MPIC_GLOBAL_CFG  ((v_u_long *)(POWERMAC_IO(powermac_io_info.int_cntlr_base_phys + MPIC_GLOBAL_CFG_REG)))
#define MPIC_CASCADE        0x00000020

#define MPIC_P0_CUR_TSK_PRI_REG  0x00020080
#define MPIC_P0_CUR_TSK_PRI ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_P0_CUR_TSK_PRI_REG))

#define MPIC_P0_INT_ACK_REG      0x000200a0
#define MPIC_P0_INT_ACK ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_P0_INT_ACK_REG))

#define MPIC_P0_EOI_REG          0x000200b0
#define MPIC_P0_EOI ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_P0_EOI_REG))


/* Interrupt Table Stuff */

#define EDGE     (0x00000000)
#define LVL      (0x00400000)
#define ACT_LOW  (0x00000000)
#define ACT_HI   (0x00800000)
#define UNMASKED (0x00000000)
#define MASKED   (0x80000000)
#define ACTIVE   (0x40000000)

#define INT_TBL(vec, pri, sen, pol, mask, dst)               \
  (mask | pol | sen | ((pri & 0xf) << 16) | (vec & 0xff)), (dst & 0xf)

#define MPIC_INT_CFG_REG    0x00010000
#define MPIC_INT_CFG ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_INT_CFG_REG))

#define MPIC_IPI_VEC_PRI_REG  0x000010a0
#define MPIC_IPI_VEC_PRI ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_IPI_VEC_PRI_REG))

#define MPIC_TMR_VEC_PRI_REG  0x00001120
#define MPIC_TMR_OFFSET       0x00000040
#define MPIC_TMR_VEC_PRI ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_TMR_VEC_PRI_REG))

#define MPIC_SPUR_INT_VEC_REG 0x000010e0
#define MPIC_SPUR_INT_VEC ((u_long)(powermac_io_info.int_cntlr_base_phys + MPIC_SPUR_INT_VEC_REG))

/* Storage for interrupt table pointers */
extern struct powermac_interrupt *mpic_interrupts;
extern struct powermac_interrupt *mpic_via1_interrupts;
extern u_long *mpic_int_mapping_tbl;
extern int *mpic_spl_to_pri;
extern int nmpic_via_interrupts;
extern int nmpic_interrupts;

#ifndef __ASSEMBLER__

void  mpic_interrupt_initialize(void);
void mpic_via1_interrupt(int device, void *ssp, void *arg);

#endif /* ndef __ASSEMBLER__ */

#endif /* _INTERRUPT_MPIC_H_ */
