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
 * @OSF_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: asm.h,v $
 * Revision 1.1.1.2  1999/08/04 21:16:30  wsanchez
 * Impoort of boot-66
 *
 * Revision 1.3  1999/08/04 21:12:44  wsanchez
 * Update APSL
 *
 * Revision 1.2  1999/03/25 05:49:00  wsanchez
 * Add APL.
 * Remove unused gzip code.
 * Remove unused Adobe fonts.
 *
 * Revision 1.1.1.1.66.2  1999/03/16 16:09:24  wsanchez
 * Substitute License
 *
 * Revision 1.1.1.1.66.1  1999/03/16 07:33:45  wsanchez
 * Add APL
 *
 * Revision 1.1.1.1  1998/01/21 21:58:08  wsanchez
 * Import of secondary_loader (~tamura2)
 *
 * Revision 1.1.11.1  1996/12/09  16:53:12  stephen
 * 	nmklinux_1.0b3_shared into pmk1.1
 * 	[1996/12/09  10:58:09  stephen]
 *
 * Revision 1.1.9.1  1996/10/14  18:36:43  stephen
 * 	New defines for new PPC types (603 mainly)
 * 	FM_REDZONE is no longer needed as new compiler
 * 	follows ELF specs correctly
 * 	[1996/10/14  18:16:17  stephen]
 * 
 * Revision 1.1.7.1  1996/04/11  09:07:15  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/10  17:02:38  emcmanus]
 * 
 * Revision 1.1.5.2  1996/01/12  16:35:23  stephen
 * 	Removed defn of MACH_DEBUG
 * 	[1996/01/12  16:33:28  stephen]
 * 
 * Revision 1.1.5.1  1995/11/23  17:37:39  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  16:47:15  stephen]
 * 
 * Revision 1.1.2.4  1995/11/16  21:56:03  stephen
 * 	Introduced FM_REDZONE, increased FM_SIZE to follow gcc (not following spec)
 * 	[1995/11/16  21:29:30  stephen]
 * 
 * Revision 1.1.2.3  1995/10/18  08:21:43  stephen
 * 	Tidied up
 * 	[1995/10/17  17:53:58  stephen]
 * 
 * Revision 1.1.2.2  1995/10/10  15:08:43  stephen
 * 	return from apple
 * 	[1995/10/10  14:35:05  stephen]
 * 
 * 	a few more definitions
 * 
 * Revision 1.1.3.2  95/09/05  17:48:35  stephen
 * 	Added #ifdef MACH_KERNEL around include of mach_debug.h
 * 
 * Revision 1.1.2.1  1995/08/25  06:32:57  stephen
 * 	Added BREAKPOINT_TRAP macro defining instruction to enter debugger
 * 	[1995/08/25  06:08:45  stephen]
 * 
 * 	Initial checkin of files for PowerPC port
 * 	[1995/08/23  15:06:15  stephen]
 * 
 * $EndLog$
 */

#ifndef	_PPC_ASM_H_
#define	_PPC_ASM_H_

#define ARG0 r3
#define ARG1 r4
#define ARG2 r5
#define ARG3 r6
#define ARG4 r7
#define ARG5 r8
#define ARG6 r9
#define ARG7 r10

#define tmp0	r0	/* Temporary GPR remapping (603e specific) */
#define tmp1	r1
#define tmp2	r2
#define tmp3	r3

/* SPR registers as taken from 603e user guide */

#define dmiss	976		/* ea that missed */
#define dcmp	977		/* compare value for the va that missed */
#define hash1	978		/* pointer to first hash pteg */
#define	hash2	979		/* pointer to second hash pteg */
#define imiss	980		/* ea that missed */
#define icmp	981		/* compare value for the va that missed */
#define rpa	982		/* required physical address register */

#define iabr	1010		/* instruction address breakpoint register */
#define dabr	1013		/* data address breakpoint register */

#define CR0 cr0
#define CR1 cr1
#define CR2 cr2
#define CR3 cr3
#define CR4 cr4
#define CR5 cr5
#define CR6 cr6
#define CR7 cr7
/* Tags are placed before Immediately Following Code (IFC) for the debugger
 * to be able to deduce where to find various registers when backtracing
 * 
 * We only define the values as we use them, see SVR4 ABI PowerPc Supplement
 * for more details (defined in ELF spec).
 */

#define TAG_NO_FRAME_USED 0x00000000

#define COPYIN_ARG0_OFFSET FM_ARG0

#define MACH_KDB 0

#define BREAKPOINT_TRAP twge	r2,r2

/* There is another definition of ALIGNMENT for .c sources */
#ifndef __LANGUAGE_ASSEMBLY
#define ALIGNMENT 4
#endif /* __LANGUAGE_ASSEMBLY */

#ifndef FALIGNMENT
#define FALIGNMENT 4 /* Align functions on words for now. Cachelines is better */
#endif

#define LB(x,n) n
#if	__STDC__
#ifndef __NO_UNDERSCORES__
#define	LCL(x)	L ## x
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#else
#define	LCL(x)	.L ## x
#define EXT(x) x
#define LEXT(x) x ## :
#endif
#define LBc(x,n) n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f
#else /* __STDC__ */
#ifndef __NO_UNDERSCORES__
#define LCL(x) L/**/x
#define EXT(x) _/**/x
#define LEXT(x) _/**/x/**/:
#else /* __NO_UNDERSCORES__ */
#define	LCL(x)	.L/**/x
#define EXT(x) x
#define LEXT(x) x/**/:
#endif /* __NO_UNDERSCORES__ */
#define LBc(x,n) n/**/:
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f
#endif /* __STDC__ */

#define String	.asciz
#define Value	.word
#define Times(a,b) (a*b)
#define Divide(a,b) (a/b)

#define data16	.byte 0x66
#define addr16	.byte 0x67

#define MCOUNT

#ifdef __ELF__
#define ELF_FUNC(x)	.type x,@function
#define ELF_DATA(x)	.type x,@object
#define ELF_SIZE(x,s)	.size x,s
#else
#define ELF_FUNC(x)
#define ELF_DATA(x)
#define ELF_SIZE(x,s)
#endif

#define	Entry(x,tag)	.globl EXT(x) @ ELF_FUNC(EXT(x)) @ .long tag @ .align FALIGNMENT @ LEXT(x)
#define	ENTRY(x,tag)	Entry(x,tag) MCOUNT
#define	ENTRY2(x,y,tag)	.globl EXT(x) @ .globl EXT(y) @ \
			ELF_FUNC(EXT(x)) @ ELF_FUNC(EXT(y)) @ \
			.align FALIGNMENT @ LEXT(x) @ LEXT(y) \
			MCOUNT
#if __STDC__
#define	ASENTRY(x) 	.globl x @ .align FALIGNMENT @ x ## : ELF_FUNC(x) MCOUNT
#else
#define	ASENTRY(x) 	.globl x @ .align FALIGNMENT @ x: ELF_FUNC(x) MCOUNT
#endif /* __STDC__ */

#define	DATA(x)		.globl EXT(x) @ ELF_DATA(EXT(x)) @ .align ALIGNMENT @ LEXT(x)

#define End(x)		ELF_SIZE(x,.-x)
#define END(x)		End(EXT(x))
#define ENDDATA(x)	END(x)
#define Enddata(x)	End(x)

/* These defines are here for .c files that wish to reference global symbols
 * within __asm__ statements. 
 */
#ifdef __LANGUAGE_ASSEMBLY
#ifndef __NO_UNDERSCORES__
#define CC_SYM_PREFIX "_"
#else
#define CC_SYM_PREFIX ""
#endif /* __NO_UNDERSCORES__ */
#endif /* __LANGUAGE_ASSEMBLY */

#endif /* _PPC_ASM_H_ */
