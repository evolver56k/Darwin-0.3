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

#ifndef	_MACHDEP_PPC_FRAME_
#define	_MACHDEP_PPC_FRAME_

#ifndef	__ASSEMBLER__
/*
 * Frame Marker
 */
#ifndef	LOCORE
/*
**	These are defined int the PowerPC Runtime Conventions Doc ch 4
*/
struct linkage_area {
	int	saved_sp;	/* Back chain */
	int	saved_cr;	/* Link Save word */
	int	saved_lr;	/* Local variable space */
	int	saved_rsv0;	/* FPSCR save area */
	int	saved_rsv1;	/* CR save area */
	int	saved_r2;	/* General register save area */
};
typedef struct linkage_area linkage_area_t;
typedef linkage_area_t *linkage_areaPtr_t;

struct reg_param_area {
	int	saved_p0;
	int	saved_p1;
	int	saved_p2;
	int	saved_p3;
	int	saved_p4;
	int	saved_p5;
	int	saved_p6;
	int	saved_p7;
};
typedef struct reg_param_area reg_param_area_t;
typedef reg_param_area_t *reg_param_areaPtr_t;


struct stack_param_area {
	int	saved_p8;
	int	saved_p9;
	int	saved_p10;
	int	saved_p11;
};
typedef struct stack_param_area stack_param_area_t;
typedef stack_param_area_t *stack_param_areaPtr_t;

struct frame_area {
	struct linkage_area	la;
	struct reg_param_area	rpa;
	struct stack_param_area	spa;
};
typedef struct frame_area frame_area_t;
typedef frame_area_t *frame_areaPtr_t;

struct param_area {
	struct reg_param_area	rpa;
	struct stack_param_area	spa;
};
typedef struct param_area param_area_t;
typedef param_area_t *param_areaPtr_t;

struct stack_frame {
	struct linkage_area	la;
	struct param_area	pa;
};
typedef struct stack_area stack_area_t;
typedef stack_area_t *stack_areaPtr_t;

/*
** some helpful defines
*/
#define	NARGS		(sizeof(struct param_area)/ sizeof(int))
#define	MAXREGARGS	(sizeof(struct reg_param_area)/ sizeof(int))




/*
 * redzone is the area under the stack pointer which must be preserved
 * when taking a trap, interrupt etc. This is no longer needed as gcc
 * (2.7.2 and above) now follows ELF spec correctly and never loads/stores
 * below the frame pointer
 */
struct redzone {
					/* save register area */
	int	save_reg13;
	int	save_reg14;
	int	save_reg15;
	int	save_reg16;
	int	save_reg17;
	int	save_reg18;
	int	save_reg19;
	int	save_reg20;
	int	save_reg21;
	int	save_reg22;
	int	save_reg23;
	int	save_reg24;
	int	save_reg25;
	int	save_reg26;
	int	save_reg27;
	int	save_reg28;
	int	save_reg29;
	int	save_reg30;
	int	save_reg31;

	double	save_fpu14;
	double	save_fpu15;
	double	save_fpu16;
	double	save_fpu17;
	double	save_fpu18;
	double	save_fpu19;
	double	save_fpu20;
	double	save_fpu21;
	double	save_fpu22;
	double	save_fpu23;
	double	save_fpu24;
	double	save_fpu25;
	double	save_fpu26;
	double	save_fpu27;
	double	save_fpu28;
	double	save_fpu29;
	double	save_fpu30;
	double	save_fpu31;

	int	round;
};
#endif

#endif	/* __ASSEMBLER__ */

/*
 * Useful Constants
 */
#define	NUMARGREGS	8		/* number of arguments in registers */

/*
 * Assembly Language Offsets for Argument or Stack Pointer
 *	to Status Save and Function Return Areas
 *	(should use genassym to get these offsets)
 */

#endif	/* _MACHDEP_PPC_FRAME_ */
