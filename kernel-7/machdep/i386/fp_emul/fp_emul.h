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

// *************************************************************************
//
//			fp_emul.h
//	================================================================
//               intel corporation proprietary information
//    this software  is  supplied  under  the  terms  of  a  license
//    agreement  or non-disclosure agreement  with intel corporation
//    and  may not be copied nor disclosed except in accordance with
//    the terms of that agreement.                                  
//	================================================================
//
// ****************************************************************************

/* 
 * to turn off counters that track number of traps and number of instructions.
 */
/* #define FP_COUNTERS  */

/*
 * Structure to represent state of stack to the floating point emulation code.
 */

typedef struct fp_emul_work {
	u_char mem_operand_pointer[6];
	short	filler0;
 	u_char instruction_pointer[6]; 
	u_char operation_type;
	u_char reg_num;
	u_char op1_format;
	u_char op1_location;
	u_char op1_use_up;
	
	u_char op2_format;
	u_char op2_location;
	u_char op2_use_up;
	
	u_char result_format;
	u_char result_location;
	
	u_char result2_format;
	u_char result2_location;
	
	u_long dword_frac1[3];
	u_char sign1;
	u_char sign1_ext;
	u_char tag1;
	u_char tag1_ext;
	u_short expon1;
	u_short expon1_ext;	

	u_long dword_frac2[3];
	u_char sign2;
	u_char sign2_ext;
	u_char tag2;
	u_char tag2_ext;
	u_char expon2;
	u_char expon2_ext;
	
	u_long before_error_signals;
	u_long extra_dword_reg[3];
	u_long result_dword_frac[3];
	u_char result_sign;
	u_char result_sign_ext;
	u_char result_tag;
	u_char result_tag_ext;
	u_short result_expon;
	u_short result_expon_ext;
	
	u_long result2_dword_frac[3];
	u_char result2_sign;
	u_char result2_sign_ext;
	u_char result2_tag;
	u_char result2_tag_ext;
	u_short result2_expon;
	u_short result2_expon_ext;

	u_long dword_cop[3];
	u_long dword_dop[3];

	u_char oprnd_siz32;
	u_char addrs_siz32;

	u_long fpstate_ptr;		/* pointer to fp state in pcb */
	u_char	is16bit;
	u_long	inst_counter;		/* counter of loops thru emulator*/
} fp_emul_work_t;

typedef struct fp_emul_state {

	fp_emul_work_t emul_work_area;

	regs_t emul_regs;

	u_long trapno;
	except_frame_t emul_frame;

} fp_emul_state_t ;
