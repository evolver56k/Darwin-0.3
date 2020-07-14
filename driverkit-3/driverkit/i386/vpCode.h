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
 * Definition file for video ROM pseudo-code opcodes.
 */
 
#ifndef __VPCODE_H__
#define __VPCODE_H__

#import <driverkit/driverTypes.h>

#define VPCODE_MAGIC (0xCAFEBEEF)

#define VP_FILENAME_SIZE	IO_MAX_PARAMETER_ARRAY_LENGTH
#define VP_GET_VPCODE_FILENAME	"VPGetVPCodeFilename"
#define VP_SET_VPCODE_SIZE	"VPSetVPCodeSize"
#define VP_SET_VPCODE		"VPSetVPCode"
#define VP_END_VPCODE		"VPEndVPCode"

/*
 * Each opcode name is followed by letters encoding the source(s) and
 * destination.  The encoding is as follows:
 *
 *	A	address
 *	C	constant
 *	T	label
 *	I	indirect address (in register)
 *	R	register
 */
#define VP_WORD		0
#define VP_DEBUG	2
#define VP_LOAD_CR	(1 | VP_IMMEDIATE)
#define VP_LOAD_TR	(2 | VP_IMMEDIATE)
#define	VP_LOAD_AR	(3 | VP_IMMEDIATE)
#define	VP_LOAD_IR	1
#define VP_STORE_AR	(4 | VP_IMMEDIATE)
#define VP_STORE_IR	4
#define VP_VLOAD_AR	(12 | VP_IMMEDIATE)
#define VP_VLOAD_IR	3
#define VP_VSTORE_AR	(13 | VP_IMMEDIATE)
#define VP_VSTORE_IR	7
#define	VP_ADD_CRR	(5 | VP_IMMEDIATE)
#define VP_ADD_RRR	5
#define VP_SUB_CRR	(6 | VP_IMMEDIATE)
#define	VP_SUB_RRR	6
#define VP_SUB_RCR	(7 | VP_IMMEDIATE)
#define	VP_AND_CRR	(8 | VP_IMMEDIATE)
#define	VP_AND_RRR	8
#define	VP_OR_CRR	(9 | VP_IMMEDIATE)
#define	VP_OR_RRR	9
#define	VP_XOR_CRR	(10 | VP_IMMEDIATE)
#define	VP_XOR_RRR	10
#define	VP_LSL_RCR	11
#define VP_LSR_RCR	12
#define	VP_MOVE_RR	13
#define	VP_TEST_R	14
#define VP_DELAY	(11 | VP_IMMEDIATE)
#define VP_CMP_RC	(15 | VP_IMMEDIATE)
#define VP_CMP_CR	(16 | VP_IMMEDIATE)
#define	VP_CMP_RR	15
#define	VP_BR		17
#define VP_BPOS		18
#define VP_BNEG		19
#define	VP_BZERO	20
#define VP_BNPOS	21
#define VP_BNNEG	22
#define VP_BNZERO	23
#define VP_INB_CR	24
#define VP_OUTB_CR	25
#define VP_OUTB_CC	(25 | VP_IMMEDIATE)
#define VP_INW_CR	26
#define VP_OUTW_CR	27
#define VP_OUTW_CC	(27 | VP_IMMEDIATE)
#define VP_CALL		28
#define VP_RETURN	29
#define VP_NOP		31

/* Definitions of masks and shifts for instruction contents.
 * Instructions are 32 bits long.
 */

typedef unsigned long VPInstruction;

#define VP_MASK_FROM_SIZE(size)	(~(~0u << (size)))
#define VP_INSTRUCTION_SIZE	32 /* bits */

#define VP_OPCODE_SIZE		6  /* bits */
#define VP_FIELD_SIZE		5  /* bits */
#define VP_REGISTER_SIZE	4  /* bits => 16 registers */
#define VP_BRANCH_SIZE		26 /* bits */
#define VP_PORT_SIZE		16 /* bits */

#define VP_OPCODE_SHIFT		(VP_INSTRUCTION_SIZE - VP_OPCODE_SIZE)
#define VP_OPCODE_MASK		(VP_MASK_FROM_SIZE(VP_OPCODE_SIZE))
#define VP_IMMEDIATE		(1u << (VP_OPCODE_SIZE - 1))

#define VP_FIELD1_SHIFT		(VP_OPCODE_SHIFT - VP_FIELD_SIZE)
#define VP_FIELD2_SHIFT		(VP_FIELD1_SHIFT - VP_FIELD_SIZE)
#define VP_FIELD3_SHIFT		(VP_FIELD2_SHIFT - VP_FIELD_SIZE)
#define VP_FIELD_MASK		(VP_MASK_FROM_SIZE(VP_FIELD_SIZE))

#define VP_REG_MASK		(VP_MASK_FROM_SIZE(VP_REGISTER_SIZE))
#define VP_BRANCH_MASK		(VP_MASK_FROM_SIZE(VP_BRANCH_SIZE))
#define VP_PORT_MASK		(VP_MASK_FROM_SIZE(VP_PORT_SIZE))

/* The following macros extract appropriate bitfields from an instruction. */

#define VP_OPCODE(i)		(((i) >> VP_OPCODE_SHIFT) & VP_OPCODE_MASK)

#define VP_IMMEDIATE_FOLLOWS_OPCODE(i)	(VP_OPCODE(i) & VP_IMMEDIATE)

/* Extract the full width of each encoded source/dest field */

#define VP_FIELD1(i)		(((i) >> VP_FIELD1_SHIFT) & VP_FIELD_MASK)
#define VP_FIELD2(i)		(((i) >> VP_FIELD2_SHIFT) & VP_FIELD_MASK)
#define VP_FIELD3(i)		(((i) >> VP_FIELD3_SHIFT) & VP_FIELD_MASK)

/* Extract the register field of each encoded source/dest field. */

#define VP_REG1(i)		(VP_FIELD1(i) & VP_REG_MASK)
#define VP_REG2(i)		(VP_FIELD2(i) & VP_REG_MASK)
#define VP_REG3(i)		(VP_FIELD3(i) & VP_REG_MASK)

/* Extract the branch destination field from an instruction */

#define VP_BRANCH_DEST(i)	((i) & VP_BRANCH_MASK)

/* Extract the I/O port field from an instruction. */

#define VP_PORT(i)		((i) & VP_PORT_MASK)

/* For the assembler, a way to set the opcode field. */

static inline VPInstruction
VP_SET_OPCODE(VPInstruction instruction, unsigned int opcode)
{
    instruction &= ~(VP_OPCODE_MASK << VP_OPCODE_SHIFT);
    instruction |= (opcode & VP_OPCODE_MASK) << VP_OPCODE_SHIFT;
    return instruction;
}

/* Set the full width of each encoded field. */

static inline VPInstruction
VP_SET_FIELD(VPInstruction instruction, unsigned int value, int shift)
{
    instruction &= ~(VP_FIELD_MASK << shift);
    instruction |= (value & VP_FIELD_MASK) << shift;
    return instruction;
}

#define VP_SET_FIELD1(i,v)	VP_SET_FIELD((i), (v), VP_FIELD1_SHIFT)
#define VP_SET_FIELD2(i,v)	VP_SET_FIELD((i), (v), VP_FIELD2_SHIFT)
#define VP_SET_FIELD3(i,v)	VP_SET_FIELD((i), (v), VP_FIELD3_SHIFT)

/* Set the register field of each encoded field. */

#define VP_SET_REG1(i,v)	VP_SET_FIELD1((i), (v) & VP_REG_MASK)
#define VP_SET_REG2(i,v)	VP_SET_FIELD2((i), (v) & VP_REG_MASK)
#define VP_SET_REG3(i,v)	VP_SET_FIELD3((i), (v) & VP_REG_MASK)

/* Set the branch destination field of an instruction */

static inline VPInstruction
VP_SET_BRANCH_DEST(VPInstruction instruction, unsigned int value)
{
    instruction &= ~VP_BRANCH_MASK;
    instruction |= value & VP_BRANCH_MASK;
    return instruction;
}

/* Set the I/O port field of an instruction. */

static inline VPInstruction
VP_SET_PORT(VPInstruction instruction, unsigned int value)
{
    instruction &= ~VP_PORT_MASK;
    instruction |= value & VP_PORT_MASK;
    return instruction;
}

#endif /* __VPCODE_H__ */
