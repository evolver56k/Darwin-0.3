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

#ident	"@(#)kern-fp:e80387.h	1.1"

#define LOCORE
#ifdef NEXT
#include "assym.h"
#else /* NEXT */
#include "assym.s"
#endif /* NEXT */

/* the following were aids in RISCizing the emulator */
#define FAST_MOVSL				\
	push	%eax				;\
1:						;\
	movl	0(%esi),%eax			;\
	movl	%eax,0(%edi)			;\
	addl	$4,%esi				;\
	addl	$4,%edi				;\
	decl	%ecx				;\
	testl	%ecx,%ecx			;\
	jg	1b				;\
	pop	%eax

#define FAST_MOVSL_ES				\
1:						;\
	movl	0(%esi),%eax			;\
	movl	%eax,%es:0(%edi)		;\
	addl	$4,%esi				;\
	addl	$4,%edi				;\
	decl	%ecx				;\
	testl	%ecx,%ecx			;\
	jg	1b

#define FAST_MOVSW				\
	push	%eax				;\
1:						;\
	movw	0(%esi),%ax			;\
	movw	%ax,%es:0(%edi)			;\
	addl	$2,%esi				;\
	addl	$2,%edi				;\
	decl	%ecx				;\
	testl	%ecx,%ecx			;\
	jg	1b				;\
	pop	%eax

#define FAST_STOSL				\
1:						;\
	movl	%eax,0(%edi)			;\
	addl	$4,%edi				;\
	decl	%ecx				;\
	testl	%ecx,%ecx			;\
	jg	1b

#define LOOP(arg)				\
	decl	%ecx				;\
	testl	%ecx,%ecx			;\
	jg	arg

/* redefine movw to movl and keep the ability to switch back */
#define MOVL	movw
#define XORL	xorl
#define ALIGN	.align	4
#define FALLSTHRU				\
	jmp	1f				;\
	.align	4				;\
1:

/*     convenience definitions     */
/*  new ones -387     */

#define         word_frac1                      (dword_frac1+2)(%ebp)
#define         word_frac2                      (dword_frac2+2)(%ebp)
#define         result_word_frac                (result_dword_frac+2)(%ebp)
#define         result2_word_frac               (result2_dword_frac+2)(%ebp)

#define		dword_expon1			(expon1)(%ebp)
#define		dword_expon2			(expon2)(%ebp)
#define		dword_result_expon		(result_expon)(%ebp)
#define		dword_result2_expon		(result2_expon)(%ebp)

#define         dop                             (dword_dop+2)(%ebp)
#define         cop                             (dword_cop+2)(%ebp)
#define         extra_word_reg                  (extra_dword_reg+2)(%ebp)
#define		signal_i_error_			(before_error_signals)(%ebp)
#define		signal_z_error_			(before_error_signals+1)(%ebp)
#define		signal_d_error_			(before_error_signals+2)(%ebp)

#define         offset_op1_rec                  $op1_format
#define         offset_op2_rec                  $op2_format
#define         offset_result_rec               $result_format
#define         offset_result2_rec              $result2_format
#define         offset_operand1                 dword_frac1
#define         lsb_frac1                       word_frac1
#define         msb_frac1                       9+word_frac1
#define         offset_operand2                 dword_frac2
#define         lsb_frac2                       word_frac2
#define         msb_frac2                       9+word_frac2
#define         offset_result                   result_dword_frac
#define         lsb_result                      result_word_frac
#define         msb_result                      9+result_word_frac
#define         offset_result2                  result2_dword_frac
#define         lsb_result2                     result2_word_frac
#define         msb_result2                     9+result2_word_frac
#define         offset_cop                      dword_cop
#define         lsb_cop                         cop
#define         msb_cop                         9+lsb_cop
#define         offset_dop                      dword_dop
#define         lsb_dop                         dop
#define         msb_dop                         9+lsb_dop
#define         q                               (mem_operand_pointer)(%ebp)
#define         exp_tmp                         (mem_operand_pointer)(%ebp)
#define         siso                            (mem_operand_pointer+4)(%ebp)
#define		log_loop_ct			(mem_operand_pointer)(%ebp)
#define         bit_ct                          (instruction_pointer)(%ebp)
#define         added_one                       (instruction_pointer+1)(%ebp)
#define         rnd_history                     (instruction_pointer+2)(%ebp)
#define         rnd1_inexact                    (instruction_pointer+2)(%ebp)
#define         rnd2_inexact                    (instruction_pointer+3)(%ebp)

#define		dword_exp_tmp			(before_error_signals)(%ebp)
#define		trans_info			(extra_dword_reg)(%ebp)
#define		op_type				(extra_dword_reg)(%ebp)
#define		sin_sign			(extra_dword_reg+1)(%ebp)
#define		cos_sign			(extra_dword_reg+2)(%ebp)
#define		cofunc_flag			(extra_dword_reg+3)(%ebp)
#define		res_signs			(extra_dword_reg+1)(%ebp)
/* ...define structure of operand info and result info structures...     */

#define         format          0
#define         location        1
#define         use_up          2

/* ...define format codes...     */

#define         single_real     0
#define         double_real     1
#define         int16           2
#define         int32           3
#define         int64           4
#define         extended_fp     5
#define         null            7
#define         bcd             8
#define         op_no_result    9
#define         no_op_result    10
#define         op2_pop         11
#define         op2_no_pop      12
#define         result2         13
#define         op1_top_op2     14

/* ...define operation type codes...     */

#define         save_ptrs       0x80
#define         load_op         0
#define         store_op        1
#define         chsign_op       2
#define         compar_op       3
#define         error_op        4+save_ptrs
#define         add_op          5
#define         sub_op          6
#define         mul_op          7
#define         div_op          8
#define         ldcw_op         9+save_ptrs
#define         save_op         10+save_ptrs
#define         restore_op      11+save_ptrs
#define         null_op         12+save_ptrs
#define         abs_op          13
#define         init_op         14+save_ptrs
#define         fxtrac_op       15
#define         log_op          16
#define         splog_op        17
#define         exp_op          18
#define         tan_op          19
#define         arctan_op       20
#define         sqrt_op         21
#define         remr_op         22
#define         intpt_op        23
#define         scale_op        24
#define         exchange_op     25
#define         free_op         26
#define         ldenv_op        27+save_ptrs
#define         stenv_op        28+save_ptrs
#define         stcw_op         29+save_ptrs
#define         stsw_op         30+save_ptrs
#define         load_1_op       31
#define         load_l2t_op     32
#define         load_l2e_op     33
#define         load_pi_op      34
#define         load_lg2_op     35
#define         load_ln2_op     36
#define         load_0_op       37
#define         decstp_op       38
#define         incstp_op       39
#define         cler_op         40+save_ptrs
#define         test_op         41
#define         exam_op         42
#define         sin_op          43
#define         cos_op          44
#define         sincos_op       45
#define         rem1_op         46
#define         ucom_op         47
#define         compar_pop      48
#define         subr_op         49
#define         divr_op         50
#define         store_pop       51
#define         ucom_pop        52

/* ...definition of register tag codes...     */

#define         valid                    $0
#define         VALID                    0
#define         special                  $1
#define		SPECIAL			 1
#define         inv                      $2
#define         empty                    3
#define         denormd                  $6
#define         infinty                  $0x0a
#define		INFINITY		 0x0a
#define         unsupp                   $0x12 

/* ...infinity_control settings...     */

#define         projective              $0
#define         affine                  $1

/* ...define location codes...     */

#define         memory_opnd             0
#define         stack_top               1
#define         stack_top_minus_1       2
#define         stack_top_plus_1        3
#define         reg                     4

/* ...define use_up codes...     */

#define         free            $0
#define         pop_stack       $1
#define         do_nothing      2

/* ...definition of operand and result records in global reent seg...     */

#define         frac80  2
#define         frac64  4
#define         frac32  8
#define         msb     11
#define         sign    12
#define         tag     14
#define         expon   16

/* ...definition of precision codes...     */

#define         prec24                   $0
#define         prec32                   $1
#define         prec53                   $2
#define         prec64                   $3
#define         prec16                   $4

/* ...define round control codes...     */

#define         rnd_to_even                      $0x0000
#define         rnd_down                         $0x0004
#define         rnd_up                           $0x0008
#define         rnd_to_zero                      $0x000c

/* ...define positive and negative...     */

#define         positive                        $0x0000
#define		POSITIVE			 0x0000
#define         negative                        $0x00ff
#define         NEGATIVE                         0x00ff

/* ...definition of true and false...     */

#define         true                    $0x00ff
#define         false                   $0x0000

#define         wrap_around_constant            $0x6000
#define         no_change                       $0x00f0
#define         exponent_bias                   0x3fff

#define         single_exp_offset               $0x3f80
#define         double_exp_offset               $0x3c00

#define         zero_mask                       0x40
#define         sign_mask                       0x01
#define         a_mask                          0x02
#define         c_mask                          0x04
#define         inexact_mask                    0x20
#define         underflow_mask                  $0x10
#define         overflow_mask                   $0x08
#define         zero_divide_mask                $0x04
#define         invalid_mask                    $0x01
#define         denorm_mask                     $0x02
#define         top_mask                        0x38
#define         precision_mask                  0x03
#define         rnd_control_mask                0x0c
#define         infinity_control_mask           $0x10
#define         high_extended_expon_for_single  $0x407e
#define         low_extended_expon_for_single   $0x3f81
#define         high_extended_expon_for_double  $0x43fe
#define         low_extended_expon_for_double   $0x3c01
#define         high_int16_exponent     $0x400e
#define         max_int16_shift         $17
#define         high_int32_exponent     $0x401e
#define         max_int32_shift         $33
#define         high_int64_exponent     $0x403e
#define         max_int64_shift         $65
/*	the least scale term for which extreme overflow is certain */
#define		least_sf_xtrm_ovfl	$0x0000e03d
/*	the greatest scale term for which extreme underflow is certain */
#define		grtst_sf_xtrm_unfl	$0xffff2002

/*	These values set up for referencing the status registers in u_block */

//       The following structure depicts the Status Register structure
//       for the 80387 emulator.
//
//               +-----------------------------------------------+
//            0  |       sr_masks        |       sr_controls     |
//               +-----------------------------------------------+
//            2  |               sr_reserved1                    |
//               +-----------------------------------------------+
//            4  |       sr_errors       |       sr_flags        |
//               +-----------------------------------------------+
//            6  |               sr_reserved2                    |
//               +-----------------------------------------------+
//            8  |                    sr_tags                    |
//               +-----------------------------------------------+
//           10  |               sr_reserved3                    |
//               +-----------------------------------------------+
//           12  |               sr_instr_offset                 |
//               +                                               +
//           14  |                                               |
//               +-----------------------------------------------+
//           16  |               sr_instr_base                   |
//               +-----------------------------------------------+
//           18  |               sr_reserved4                    |
//               +-----------------------------------------------+
//           20  |               sr_mem_offset                   |
//               +                                               +
//           22  |                                               |
//               +-----------------------------------------------+
//           24  |               sr_mem_base                     |
//               +-----------------------------------------------+
//           26  |               sr_reserved5                    |
//               +-----------------------------------------------+
//           28  |               sr_regstack                     |
//               +-----------------------------------------------+
//           30  |                                               |
//               +-----------------------------------------------+
//                                       .
//
//                                       .
//
//                                       .
//               +-----------------------------------------------+
//           106 |                                               |
//               +-----------------------------------------------+
//
//
//
//
//	The status register memory allocation for the emulator is found at
//	offset u_fpstate into the users u block structure.
//
//
//...define the 80387 status register...
//
#if 0
/* These are now generated automatically - Bob Davies */
#define sr_masks 	0 	
#define sr_controls	1 
#define sr_reserved1 	2 	
#define sr_errors 	4 	
#define sr_flags 	5 	
#define sr_reserved2 	6 	
#define sr_tags 	8 	
#define sr_reserved3 	10 	
#define sr_instr_offset 12
#define sr_instr_base 	16
#define sr_reserved4 	18
#define sr_mem_offset 	20	
#define sr_mem_base 	24	
#define sr_reserved5 	26	
#define sr_regstack 	28	
//

#define	EIP_offset	14*4 + 220  /* EIP that will be loaded on iret */

#endif /* 0 */
