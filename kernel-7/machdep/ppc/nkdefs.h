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

#if TRUE
/* Additional SPR's */
#define	sprMQ		0	/* mulx/divx extension */
#define	sprXER		1	/* */
#define	fromRTCU	4	/* read realtime clock */
#define	fromRTCL	5	/* ditto */
#define	sprLR		8	/* link register */
#define	sprCTR		9	/* counter register */
#define	toRTCU		20	/* write realtime clock (supervisor mode only)*/
#define	toRTCL		21	/* ditto */
#define	fromDEC		22	/* decrementer register */
#define	sprPVR		287	/* processor version register */
#define	sprMMCR0	952	/* performance monitor (604) */
#define	sprPMC1		953	/* performance monitor (604) */
#define	sprPMC2		954	/* performance monitor (604) */
#define	sprSIA		955	/* performance monitor (604) */
#define	sprMMCR1	956	/* performance monitor (Sirocco) */
#define	sprPMC3		957	/* performance monitor (Sirocco) */
#define	sprPMC4		958	/* performance monitor (Sirocco) */
#define	sprSDA		959	/* performance monitor (604) */

#define rcr0 0
#define rcr1 1
#define rcr2 2
#define rcr3 3
#define rcr4 4
#define rcr5 5
#define rcr6 6
#define rcr7 7

#define	lt_b	0
#define	gt_b	1
#define	eq_b	2
#define	so_b	3

/*
**	Register defines for EmulateUnimplimentedInstructions
*/
#define	dKernelDataPtr		1

#define	dInitEmulatorDataPtr	8
#define	dInitZero		0

#define	dContextPtr		6
#define	dContextStatFlags	7
#define	dTmp1			8
#define	dTmp2			9

#define	dSavedSRR0		10
#define	dSavedSRR1		11
#define	dSavedLR			12
#define	dSavedCR			13

#define	dMSR_Disabled		14
#define	dMSR_Enabled		15
#define	dMemCtxFlags		16
#define	dMemCmdInfo		17
#define	dMemUpdateEA		18
#define	dMemAddress		19
#define	dMemDataH		20
#define	dMemDataL		21
#define	dMemByteCount		22

#define	dDataTemp		23
#define	dSavedVBR		24
#define	dMemProcPtr		25
#define	dMemDisp		dDataTemp

#define	dMemSetupInfo		26
#define	dMemInstr		27
#define	dRegIndex		28

/*
**	Register defines for EmulateUnimplimentedInstructions
*/
#define	KernelDataPtr		r1

#define	InitEmulatorDataPtr	r8
#define	InitZero		r0

#define	ContextPtr		r6
#define	ContextStatFlags	r7
#define	Tmp1			r8
#define	Tmp2			r9

#define	SavedSRR0		r10
#define	SavedSRR1		r11
#define	SavedLR			r12
#define	SavedCR			r13

#define	MSR_Disabled		r14
#define	MSR_Enabled		r15
#define	MemCtxFlags		r16
#define	MemCmdInfo		r17
#define	MemUpdateEA		r18
#define	MemAddress		r19
#define	MemDataH		r20
#define	MemDataL		r21
#define	MemByteCount		r22

#define	DataTemp		r23
#define	SavedVBR		r24
#define	MemProcPtr		r25
#define	MemDisp			DataTemp

#define	MemSetupInfo		r26
#define	MemInstr		r27
#define	RegIndex		r28

/*
** Flag Bit Definitions
*/
#define	b_EmulatePowerMQ	8
#define	b_EmulatePowerRTC	9
#define	b_EmulatePowerDEC	10
#define	b_EmulatePowerComplex	11
#define	b_EmulatePowerCLCS	12
#define	b_EmulatePowerMemory	13
#define	b_EmulateInvalidSPR	14
#define	b_EmulatePOWERmaskLSB	b_EmulateInvalidSPR
#define	b_EmulateOptional	16
#define	b_EmulatePrivMFPVR	17
#define	b_EmulatePrivSPRperf	18
#define	b_EmulatePrivSPRperf2	19
#define	b_EmulatePrivSPRperf0	20

/*
** CtxFlags bit numbers
*/
#define	b_CtxFlagSystemContext			8
#define	b_CtxFlagHWPowerCompatible		13
#define	b_CtxFlagHWOptionsalInstr		14
#define	b_CtxFlagHW64Bit			15
#define	b_CtxFlagUnused16			16
#define	b_CtxFlagUnused17			17
#define	b_CtxFlagUnused18			18
#define	b_CtxFlagUnused19			19
#define	b_CtxFlagFPExceptionMode0		20
#define	b_CtxFlagStepTraceEnabled		21
#define	b_CtxFlagBranchTraceEnabled		22
#define	b_CtxFlagFpExceptionMode1		23
#define	b_CtxFlagUnused24			24
#define	b_CtxFlagUnused25			24
#define	b_CtxFlagTracePending			26
#define	b_CtxFlagMemInfoValid			27
#define	b_CtxFlag64BitMode			28
#define	b_CtxFlagEmulatePowerCompatible		29
#define	b_CtxFlagEmulateOptionalInstr		30
#define	b_CtxFlagInstructionContinuation	31

/* Exception Causes stored in bits 0..7 fo CtxFlags	*/
#define	ecNoException			((0<<3)+0)
#define	ecSystemCall			((0<<3)+1)
#define	ecTrapInstr			((0<<3)+2)
#define	ecFloatException		((0<<3)+3)
#define	ecInvalidInstr			((0<<3)+4)
#define	ecPrivilegedInstr		((0<<3)+5)
#define	ecMachineCheck			((0<<3)+7)

#define	ecInstTrace			((1<<3)+0)
#define	ecInstInvalidAddress		((1<<3)+2)
#define	ecInstHardwareFault		((1<<3)+3)
#define	ecPageFalut			((1<<3)+4)
#define	ecInstSupAccessViolation	((1<<3)+6)

#define	ecDataInvalidAddress		((2<<3)+2)
#define	ecDataHardwareFault		((2<<3)+3)
#define	ecDataPageFault			((2<<3)+4)
#define	ecDataWriteViolation		((2<<3)+5)
#define	ecDataSupAccessViolation	((2<<3)+6)
#define	ecDataSupWriteViolation		((2<<3)+7)

/*
**	Registers used for decoding fields of unimplemented instructions
*/
#define	UnimpRSRT		MemCmdInfo
#define	UnimpRS			UnimpRSRT
#define	UnimpRT			UnimpRSRT
#define	UnimpRA			MemUpdateEA
#define	UnimpRB			MemAddress
#define	UnimpMQptr		MemByteCount
#define	UnimpMQtmp		SavedVBR
#define	UnimpXERtmp		MemSetupInfo

#define	dUnimpRSRT		dMemCmdInfo
#define	dUnimpRS		dUnimpRSRT
#define	dUnimpRT		dUnimpRSRT
#define	dUnimpRA		dMemUpdateEA
#define	dUnimpRB		dMemAddress
#define	dUnimpMQptr		dMemByteCount
#define	dUnimpMQtmp		dSavedVBR
#define	dUnimpXERtmp		dMemSetupInfo

#endif
