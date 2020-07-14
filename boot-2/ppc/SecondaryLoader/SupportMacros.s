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
#
#	File:		Support.s
#
#	Contains:	PowerPC assembly language support macros for Mac OS 8 Secondary Loader
#
#	Version:	Mac OS 8
#
#	Written by:	Jeffrey Robbin
#
#	Copyright:	© 1994-1997 by Apple Computer, Inc., all rights reserved.
#
#	File Ownership:
#
#		DRI:				Alan Mimms
#
#		Other Contact:		Pradeep Kathail
#
#		Technology:			Kernel Loading/Booting
#
#	Writers:
#
#		(rob)	Rob ‘lunatic’ Moore
#		(TS)	Tom Saulpaugh
#		(JLR)	Jeff Robbin
#		(ABM)	Alan Mimms
#
#	Change History (most recent first):
#
#		 <0+>	12/11/96	ABM		Converted CompilerSupportMacros.s for use in standalone Secondary Loader code.
#									This basically consisted of removing everything requiring TOCs and data fixups.
#		 <2>	11/27/95	rob		Removed the need for Exceptions.a.
#		 <1>	10/24/95	rob		First checked in.
#		<18>	10/23/95	TS		Add 603/604 Support.
#		<17>	 9/21/95	JLR		Add EntryNoStackGlue and DeclareEntryGlue.
#		<16>	 6/27/95	WSK		Add some more equates for condition register bit sub-fields.
#		<15>	  4/7/95	JLR		Change alignment of StartStackFrame.
#		<14>	 3/23/95	RW		Include Exceptions.a for DEBUGLEVEL
#		<13>	 2/12/95	WSK		d5 CodeBertization.
#		<12>	  2/7/95	WSK		Add DEBUGLEVEL check to TracebackTable macro.
#		<11>	  2/3/95	JLR		Remove equ equates that are in normal headers.
#		<10>	11/22/94	WSK		Add equ for memFullErr.
#		 <9>	 11/6/94	WSK		Nuke unused third long word from T vectors.
#		 <8>	 11/2/94	WSK		Nuke definitions for cr0..7 and un since they are now predefined
#									as of PPCAsm 1.1b1c2. Nuke DeclareFn in favor of '-sym on' build
#									option.
#		 <7>	 10/6/94	WSK		Well, let's try again with PPCAsm 1.1a2c1 and the symbolics
#									crap.
#		 <6>	 10/3/94	WSK		The FUNCTION directive seems to work with PPCAsm 1.1.1a1e4.  Of
#									course, according to dumpxcoff it isn't generating any
#									symbolics, but what the heck.
#		 <5>	 10/3/94	WSK		Well, now the assembler sometimes complains because the ENDF
#									doesn't have a matching BEGINF. This does not bode well.  For
#									now, forget about PPCAsm symbolics.
#		 <4>	 9/29/94	WSK		Add ENDF directive to end of ExitNoStack.  Shouldn't be
#									necessary, but PPCAsm whines without it.
#		 <3>	 9/26/94	WSK		Add traceback table macro and better support for multiple entry
#									points.
#		 <2>	 9/22/94	WSK		Change CallPanic to call Panic instead of panic. Got that?
#		 <1>	 9/20/94	JLR		first checked in
#
#

#********************************************************************************************
# 						                                                                    *
#   Macros for Assembl-ease                                           						*
# 						                                                                    *
#********************************************************************************************

#============================================================================================
# SaveGPR
#============================================================================================


	MACRO
	SaveGPR31
	stw	r31, savedRegs(sp)
	ENDM
	
	MACRO
	SaveGPR30
	stw	r30, savedRegs(sp)
	stw	r31, savedRegs+4(sp)
	ENDM
	
	MACRO
	SaveGPR29
	stw r29, savedRegs(sp)
	stw	r30, savedRegs+4(sp)
	stw	r31, savedRegs+8(sp)
	ENDM
	
	MACRO
	SaveGPR28
	stw r28, savedRegs(sp)
	stw	r29, savedRegs+4(sp)
	stw	r30, savedRegs+8(sp)
	stw	r31, savedRegs+12(sp)
	ENDM
	
	MACRO
	SaveGPR27
	stw r27, savedRegs(sp)
	stw	r28, savedRegs+4(sp)
	stw	r29, savedRegs+8(sp)
	stw	r30, savedRegs+12(sp)
	stw	r31, savedRegs+16(sp)
	ENDM
	
	MACRO
	SaveGPR26
	stw r26, savedRegs(sp)
	stw	r27, savedRegs+4(sp)
	stw	r28, savedRegs+8(sp)
	stw	r29, savedRegs+12(sp)
	stw	r30, savedRegs+16(sp)
	stw	r31, savedRegs+20(sp)	
	ENDM
	
	MACRO
	SaveGPR25
	stw r25, savedRegs(sp)
	stw	r26, savedRegs+4(sp)
	stw	r27, savedRegs+8(sp)
	stw	r28, savedRegs+12(sp)
	stw	r29, savedRegs+16(sp)
	stw	r30, savedRegs+20(sp)
	stw	r31, savedRegs+24(sp)
	ENDM
	
	MACRO
	SaveGPR24
	stw r24, savedRegs(sp)
	stw	r25, savedRegs+4(sp)
	stw	r26, savedRegs+8(sp)
	stw	r27, savedRegs+12(sp)
	stw	r28, savedRegs+16(sp)
	stw	r29, savedRegs+20(sp)
	stw	r30, savedRegs+24(sp)
	stw	r31, savedRegs+28(sp)
	ENDM
	
	MACRO
	SaveGPR23
	stw r23, savedRegs(sp)
	stw	r24, savedRegs+4(sp)
	stw	r25, savedRegs+8(sp)
	stw	r26, savedRegs+12(sp)
	stw	r27, savedRegs+16(sp)
	stw	r28, savedRegs+20(sp)
	stw	r29, savedRegs+24(sp)
	stw	r30, savedRegs+28(sp)
	stw	r31, savedRegs+32(sp)
	ENDM
	
	MACRO
	SaveGPR22
	stw r22, savedRegs(sp)
	stw	r23, savedRegs+4(sp)
	stw	r24, savedRegs+8(sp)
	stw	r25, savedRegs+12(sp)
	stw	r26, savedRegs+16(sp)
	stw	r27, savedRegs+20(sp)
	stw	r28, savedRegs+24(sp)
	stw	r29, savedRegs+28(sp)
	stw	r30, savedRegs+32(sp)
	stw	r31, savedRegs+36(sp)
	ENDM
	
	MACRO
	SaveGPR21
	stw r21, savedRegs(sp)
	stw	r22, savedRegs+4(sp)
	stw	r23, savedRegs+8(sp)
	stw	r24, savedRegs+12(sp)
	stw	r25, savedRegs+16(sp)
	stw	r26, savedRegs+20(sp)
	stw	r27, savedRegs+24(sp)
	stw	r28, savedRegs+28(sp)
	stw	r29, savedRegs+32(sp)
	stw	r30, savedRegs+36(sp)
	stw	r31, savedRegs+40(sp)
	ENDM
	
	MACRO
	SaveGPR20
	stw r20, savedRegs(sp)
	stw	r21, savedRegs+4(sp)
	stw	r22, savedRegs+8(sp)
	stw	r23, savedRegs+12(sp)
	stw	r24, savedRegs+16(sp)
	stw	r25, savedRegs+20(sp)
	stw	r26, savedRegs+24(sp)
	stw	r27, savedRegs+28(sp)
	stw	r28, savedRegs+32(sp)
	stw	r29, savedRegs+36(sp)
	stw	r30, savedRegs+40(sp)
	stw	r31, savedRegs+44(sp)
	ENDM
	
	MACRO
	SaveGPR19
	stw r19, savedRegs(sp)
	stw	r20, savedRegs+4(sp)
	stw	r21, savedRegs+8(sp)
	stw	r22, savedRegs+12(sp)
	stw	r23, savedRegs+16(sp)
	stw	r24, savedRegs+20(sp)
	stw	r25, savedRegs+24(sp)
	stw	r26, savedRegs+28(sp)
	stw	r27, savedRegs+32(sp)
	stw	r28, savedRegs+36(sp)
	stw	r29, savedRegs+40(sp)
	stw	r30, savedRegs+44(sp)
	stw	r31, savedRegs+48(sp)
	ENDM
	
	MACRO
	SaveGPR18
	stw r18, savedRegs(sp)
	stw	r19, savedRegs+4(sp)
	stw	r20, savedRegs+8(sp)
	stw	r21, savedRegs+12(sp)
	stw	r22, savedRegs+16(sp)
	stw	r23, savedRegs+20(sp)
	stw	r24, savedRegs+24(sp)
	stw	r25, savedRegs+28(sp)
	stw	r26, savedRegs+32(sp)
	stw	r27, savedRegs+36(sp)
	stw	r28, savedRegs+40(sp)
	stw	r29, savedRegs+44(sp)
	stw	r30, savedRegs+48(sp)
	stw	r31, savedRegs+52(sp)
	ENDM
	
	MACRO
	SaveGPR17
	stw r17, savedRegs(sp)
	stw	r18, savedRegs+4(sp)
	stw	r19, savedRegs+8(sp)
	stw	r20, savedRegs+12(sp)
	stw	r21, savedRegs+16(sp)
	stw	r22, savedRegs+20(sp)
	stw	r23, savedRegs+24(sp)
	stw	r24, savedRegs+28(sp)
	stw	r25, savedRegs+32(sp)
	stw	r26, savedRegs+36(sp)
	stw	r27, savedRegs+40(sp)
	stw	r28, savedRegs+44(sp)
	stw	r29, savedRegs+48(sp)
	stw	r30, savedRegs+52(sp)
	stw	r31, savedRegs+56(sp)
	ENDM
	
	MACRO
	SaveGPR16
	stw r16, savedRegs(sp)
	stw	r17, savedRegs+4(sp)
	stw	r18, savedRegs+8(sp)
	stw	r19, savedRegs+12(sp)
	stw	r20, savedRegs+16(sp)
	stw	r21, savedRegs+20(sp)
	stw	r22, savedRegs+24(sp)
	stw	r23, savedRegs+28(sp)
	stw	r24, savedRegs+32(sp)
	stw	r25, savedRegs+36(sp)
	stw	r26, savedRegs+40(sp)
	stw	r27, savedRegs+44(sp)
	stw	r28, savedRegs+48(sp)
	stw	r29, savedRegs+52(sp)
	stw	r30, savedRegs+56(sp)
	stw	r31, savedRegs+60(sp)
	ENDM
	
	MACRO
	SaveGPR15
	stw r15, savedRegs(sp)
	stw	r16, savedRegs+4(sp)
	stw	r17, savedRegs+8(sp)
	stw	r18, savedRegs+12(sp)
	stw	r19, savedRegs+16(sp)
	stw	r20, savedRegs+20(sp)
	stw	r21, savedRegs+24(sp)
	stw	r22, savedRegs+28(sp)
	stw	r23, savedRegs+32(sp)
	stw	r24, savedRegs+36(sp)
	stw	r25, savedRegs+40(sp)
	stw	r26, savedRegs+44(sp)
	stw	r27, savedRegs+48(sp)
	stw	r28, savedRegs+52(sp)
	stw	r29, savedRegs+56(sp)
	stw	r30, savedRegs+60(sp)
	stw	r31, savedRegs+64(sp)
	ENDM
	
	MACRO
	SaveGPR14
	stw r14, savedRegs(sp)
	stw	r15, savedRegs+4(sp)
	stw	r16, savedRegs+8(sp)
	stw	r17, savedRegs+12(sp)
	stw	r18, savedRegs+16(sp)
	stw	r19, savedRegs+20(sp)
	stw	r20, savedRegs+24(sp)
	stw	r21, savedRegs+28(sp)
	stw	r22, savedRegs+32(sp)
	stw	r23, savedRegs+36(sp)
	stw	r24, savedRegs+40(sp)
	stw	r25, savedRegs+44(sp)
	stw	r26, savedRegs+48(sp)
	stw	r27, savedRegs+52(sp)
	stw	r28, savedRegs+56(sp)
	stw	r29, savedRegs+60(sp)
	stw	r30, savedRegs+64(sp)
	stw	r31, savedRegs+68(sp)
	ENDM
	
	MACRO
	SaveGPR13
	stw r13, savedRegs(sp)
	stw	r14, savedRegs+4(sp)
	stw	r15, savedRegs+8(sp)
	stw	r16, savedRegs+12(sp)
	stw	r17, savedRegs+16(sp)
	stw	r18, savedRegs+20(sp)
	stw	r19, savedRegs+24(sp)
	stw	r20, savedRegs+28(sp)
	stw	r21, savedRegs+32(sp)
	stw	r22, savedRegs+36(sp)
	stw	r23, savedRegs+40(sp)
	stw	r24, savedRegs+44(sp)
	stw	r25, savedRegs+48(sp)
	stw	r26, savedRegs+52(sp)
	stw	r27, savedRegs+56(sp)
	stw	r28, savedRegs+60(sp)
	stw	r29, savedRegs+64(sp)
	stw	r30, savedRegs+68(sp)
	stw	r31, savedRegs+72(sp)
	ENDM
	
	MACRO
	SaveGPR &p1
	
	IF &p1 == 31
	SaveGPR31
	ENDIF
	
	IF &p1 == 30
	SaveGPR30
	ENDIF
	
	IF &p1 == 29
	SaveGPR29
	ENDIF
	
	IF &p1 == 28
	SaveGPR28
	ENDIF
	
	IF &p1 == 27
	SaveGPR27
	ENDIF
	
	IF &p1 == 26
	SaveGPR26
	ENDIF
	
	IF &p1 == 25
	SaveGPR25
	ENDIF
	
	IF &p1 == 24
	SaveGPR24
	ENDIF
	
	IF &p1 == 23
	SaveGPR23
	ENDIF
	
	IF &p1 == 22
	SaveGPR22
	ENDIF
	
	IF &p1 == 21
	SaveGPR21
	ENDIF
	
	IF &p1 == 20
	SaveGPR20
	ENDIF
	
	IF &p1 == 19
	SaveGPR19
	ENDIF
	
	IF &p1 == 18
	SaveGPR18
	ENDIF
	
	IF &p1 == 17
	SaveGPR17
	ENDIF
	
	IF &p1 == 16
	SaveGPR16
	ENDIF
	
	IF &p1 == 15
	SaveGPR15
	ENDIF
	
	IF &p1 == 14
	SaveGPR14
	ENDIF
	
	IF &p1 == 13
	SaveGPR13
	ENDIF
	
	ENDM


#============================================================================================
# RestoreGPR
#============================================================================================

	MACRO
	RestoreGPR31
	lwz	r31, savedRegs(sp)
	ENDM

	MACRO
	RestoreGPR30
	lwz	r30, savedRegs(sp)
	lwz	r31, savedRegs+4(sp)
	ENDM

	MACRO
	RestoreGPR29
	lwz r29, savedRegs(sp)
	lwz	r30, savedRegs+4(sp)
	lwz	r31, savedRegs+8(sp)
	ENDM

	MACRO
	RestoreGPR28
	lwz r28, savedRegs(sp)
	lwz	r29, savedRegs+4(sp)
	lwz	r30, savedRegs+8(sp)
	lwz	r31, savedRegs+12(sp)
	ENDM

	MACRO
	RestoreGPR27
	lwz r27, savedRegs(sp)
	lwz	r28, savedRegs+4(sp)
	lwz	r29, savedRegs+8(sp)
	lwz	r30, savedRegs+12(sp)
	lwz	r31, savedRegs+16(sp)
	ENDM

	MACRO
	RestoreGPR26
	lwz r26, savedRegs(sp)
	lwz	r27, savedRegs+4(sp)
	lwz	r28, savedRegs+8(sp)
	lwz	r29, savedRegs+12(sp)
	lwz	r30, savedRegs+16(sp)
	lwz	r31, savedRegs+20(sp)
	ENDM

	MACRO
	RestoreGPR25
	lwz r25, savedRegs(sp)
	lwz	r26, savedRegs+4(sp)
	lwz	r27, savedRegs+8(sp)
	lwz	r28, savedRegs+12(sp)
	lwz	r29, savedRegs+16(sp)
	lwz	r30, savedRegs+20(sp)
	lwz	r31, savedRegs+24(sp)
	ENDM

	MACRO
	RestoreGPR24
	lwz r24, savedRegs(sp)
	lwz	r25, savedRegs+4(sp)
	lwz	r26, savedRegs+8(sp)
	lwz	r27, savedRegs+12(sp)
	lwz	r28, savedRegs+16(sp)
	lwz	r29, savedRegs+20(sp)
	lwz	r30, savedRegs+24(sp)
	lwz	r31, savedRegs+28(sp)
	ENDM

	MACRO
	RestoreGPR23
	lwz r23, savedRegs(sp)
	lwz	r24, savedRegs+4(sp)
	lwz	r25, savedRegs+8(sp)
	lwz	r26, savedRegs+12(sp)
	lwz	r27, savedRegs+16(sp)
	lwz	r28, savedRegs+20(sp)
	lwz	r29, savedRegs+24(sp)
	lwz	r30, savedRegs+28(sp)
	lwz	r31, savedRegs+32(sp)
	ENDM

	MACRO
	RestoreGPR22
	lwz r22, savedRegs(sp)
	lwz	r23, savedRegs+4(sp)
	lwz	r24, savedRegs+8(sp)
	lwz	r25, savedRegs+12(sp)
	lwz	r26, savedRegs+16(sp)
	lwz	r27, savedRegs+20(sp)
	lwz	r28, savedRegs+24(sp)
	lwz	r29, savedRegs+28(sp)
	lwz	r30, savedRegs+32(sp)
	lwz	r31, savedRegs+36(sp)
	ENDM

	MACRO
	RestoreGPR21
	lwz r21, savedRegs(sp)
	lwz	r22, savedRegs+4(sp)
	lwz	r23, savedRegs+8(sp)
	lwz	r24, savedRegs+12(sp)
	lwz	r25, savedRegs+16(sp)
	lwz	r26, savedRegs+20(sp)
	lwz	r27, savedRegs+24(sp)
	lwz	r28, savedRegs+28(sp)
	lwz	r29, savedRegs+32(sp)
	lwz	r30, savedRegs+36(sp)
	lwz	r31, savedRegs+40(sp)
	ENDM

	MACRO
	RestoreGPR20
	lwz r20, savedRegs(sp)
	lwz	r21, savedRegs+4(sp)
	lwz	r22, savedRegs+8(sp)
	lwz	r23, savedRegs+12(sp)
	lwz	r24, savedRegs+16(sp)
	lwz	r25, savedRegs+20(sp)
	lwz	r26, savedRegs+24(sp)
	lwz	r27, savedRegs+28(sp)
	lwz	r28, savedRegs+32(sp)
	lwz	r29, savedRegs+36(sp)
	lwz	r30, savedRegs+40(sp)
	lwz	r31, savedRegs+44(sp)
	ENDM

	MACRO
	RestoreGPR19
	lwz r19, savedRegs(sp)
	lwz	r20, savedRegs+4(sp)
	lwz	r21, savedRegs+8(sp)
	lwz	r22, savedRegs+12(sp)
	lwz	r23, savedRegs+16(sp)
	lwz	r24, savedRegs+20(sp)
	lwz	r25, savedRegs+24(sp)
	lwz	r26, savedRegs+28(sp)
	lwz	r27, savedRegs+32(sp)
	lwz	r28, savedRegs+36(sp)
	lwz	r29, savedRegs+40(sp)
	lwz	r30, savedRegs+44(sp)
	lwz	r31, savedRegs+48(sp)
	ENDM

	MACRO
	RestoreGPR18
	lwz r18, savedRegs(sp)
	lwz	r19, savedRegs+4(sp)
	lwz	r20, savedRegs+8(sp)
	lwz	r21, savedRegs+12(sp)
	lwz	r22, savedRegs+16(sp)
	lwz	r23, savedRegs+20(sp)
	lwz	r24, savedRegs+24(sp)
	lwz	r25, savedRegs+28(sp)
	lwz	r26, savedRegs+32(sp)
	lwz	r27, savedRegs+36(sp)
	lwz	r28, savedRegs+40(sp)
	lwz	r29, savedRegs+44(sp)
	lwz	r30, savedRegs+48(sp)
	lwz	r31, savedRegs+52(sp)
	ENDM

	MACRO
	RestoreGPR17
	lwz r17, savedRegs(sp)
	lwz	r18, savedRegs+4(sp)
	lwz	r19, savedRegs+8(sp)
	lwz	r20, savedRegs+12(sp)
	lwz	r21, savedRegs+16(sp)
	lwz	r22, savedRegs+20(sp)
	lwz	r23, savedRegs+24(sp)
	lwz	r24, savedRegs+28(sp)
	lwz	r25, savedRegs+32(sp)
	lwz	r26, savedRegs+36(sp)
	lwz	r27, savedRegs+40(sp)
	lwz	r28, savedRegs+44(sp)
	lwz	r29, savedRegs+48(sp)
	lwz	r30, savedRegs+52(sp)
	lwz	r31, savedRegs+56(sp)
	ENDM

	MACRO
	RestoreGPR16
	lwz r16, savedRegs(sp)
	lwz	r17, savedRegs+4(sp)
	lwz	r18, savedRegs+8(sp)
	lwz	r19, savedRegs+12(sp)
	lwz	r20, savedRegs+16(sp)
	lwz	r21, savedRegs+20(sp)
	lwz	r22, savedRegs+24(sp)
	lwz	r23, savedRegs+28(sp)
	lwz	r24, savedRegs+32(sp)
	lwz	r25, savedRegs+36(sp)
	lwz	r26, savedRegs+40(sp)
	lwz	r27, savedRegs+44(sp)
	lwz	r28, savedRegs+48(sp)
	lwz	r29, savedRegs+52(sp)
	lwz	r30, savedRegs+56(sp)
	lwz	r31, savedRegs+60(sp)
	ENDM

	MACRO
	RestoreGPR15
	lwz r15, savedRegs(sp)
	lwz	r16, savedRegs+4(sp)
	lwz	r17, savedRegs+8(sp)
	lwz	r18, savedRegs+12(sp)
	lwz	r19, savedRegs+16(sp)
	lwz	r20, savedRegs+20(sp)
	lwz	r21, savedRegs+24(sp)
	lwz	r22, savedRegs+28(sp)
	lwz	r23, savedRegs+32(sp)
	lwz	r24, savedRegs+36(sp)
	lwz	r25, savedRegs+40(sp)
	lwz	r26, savedRegs+44(sp)
	lwz	r27, savedRegs+48(sp)
	lwz	r28, savedRegs+52(sp)
	lwz	r29, savedRegs+56(sp)
	lwz	r30, savedRegs+60(sp)
	lwz	r31, savedRegs+64(sp)
	ENDM

	MACRO
	RestoreGPR14
	lwz r14, savedRegs(sp)
	lwz	r15, savedRegs+4(sp)
	lwz	r16, savedRegs+8(sp)
	lwz	r17, savedRegs+12(sp)
	lwz	r18, savedRegs+16(sp)
	lwz	r19, savedRegs+20(sp)
	lwz	r20, savedRegs+24(sp)
	lwz	r21, savedRegs+28(sp)
	lwz	r22, savedRegs+32(sp)
	lwz	r23, savedRegs+36(sp)
	lwz	r24, savedRegs+40(sp)
	lwz	r25, savedRegs+44(sp)
	lwz	r26, savedRegs+48(sp)
	lwz	r27, savedRegs+52(sp)
	lwz	r28, savedRegs+56(sp)
	lwz	r29, savedRegs+60(sp)
	lwz	r30, savedRegs+64(sp)
	lwz	r31, savedRegs+68(sp)
	ENDM

	MACRO
	RestoreGPR13
	lwz r13, savedRegs(sp)
	lwz	r14, savedRegs+4(sp)
	lwz	r15, savedRegs+8(sp)
	lwz	r16, savedRegs+12(sp)
	lwz	r17, savedRegs+16(sp)
	lwz	r18, savedRegs+20(sp)
	lwz	r19, savedRegs+24(sp)
	lwz	r20, savedRegs+28(sp)
	lwz	r21, savedRegs+32(sp)
	lwz	r22, savedRegs+36(sp)
	lwz	r23, savedRegs+40(sp)
	lwz	r24, savedRegs+44(sp)
	lwz	r25, savedRegs+48(sp)
	lwz	r26, savedRegs+52(sp)
	lwz	r27, savedRegs+56(sp)
	lwz	r28, savedRegs+60(sp)
	lwz	r29, savedRegs+64(sp)
	lwz	r30, savedRegs+68(sp)
	lwz	r31, savedRegs+72(sp)
	ENDM


	MACRO
	RestoreGPR &p1
	
	IF &p1 == 31
	RestoreGPR31
	ENDIF
	
	IF &p1 == 30
	RestoreGPR30
	ENDIF
	
	IF &p1 == 29
	RestoreGPR29
	ENDIF
	
	IF &p1 == 28
	RestoreGPR28
	ENDIF
	
	IF &p1 == 27
	RestoreGPR27
	ENDIF
	
	IF &p1 == 26
	RestoreGPR26
	ENDIF
	
	IF &p1 == 25
	RestoreGPR25
	ENDIF
	
	IF &p1 == 24
	RestoreGPR24
	ENDIF
	
	IF &p1 == 23
	RestoreGPR23
	ENDIF
	
	IF &p1 == 22
	RestoreGPR22
	ENDIF
	
	IF &p1 == 21
	RestoreGPR21
	ENDIF
	
	IF &p1 == 20
	RestoreGPR20
	ENDIF
	
	IF &p1 == 19
	RestoreGPR19
	ENDIF
	
	IF &p1 == 18
	RestoreGPR18
	ENDIF
	
	IF &p1 == 17
	RestoreGPR17
	ENDIF
	
	IF &p1 == 16
	RestoreGPR16
	ENDIF
	
	IF &p1 == 15
	RestoreGPR15
	ENDIF
	
	IF &p1 == 14
	RestoreGPR14
	ENDIF
	
	IF &p1 == 13
	RestoreGPR13
	ENDIF
	
	ENDM


#============================================================================================
# StartStackFrame Name
#============================================================================================

	MACRO
	StartStackFrame	&p1

				dsect	Frame&p1
				
backLink		set		$
				dc.l	0
savedCR			set		$
				dc.l	0
savedLR			set		$
				dc.l	0
				dc.l	0		; Reserved 1
				dc.l	0		; Reserved 2
savedTOC		set		$
				dc.l	0
	
	ENDM


#============================================================================================
# EndStackFrame Name [ , StartingRegisterToSave ]
#============================================================================================

	MACRO
	EndStackFrame	&p1, &p2=32

					align			3
savedRegs			set		$
					ds.b	(4*(32-&p2))
					align			4
	
FrameSize&p1		EQU		$-backLink
	
callerBackLink		set		$
					dc.l	0
callerSavedCR		set		$
					dc.l	0
callerSavedLR		set		$
					dc.l	0
					dc.l	0		; Reserved 1
					dc.l	0		; Reserved 2
callerSavedTOC		set		$
					dc.l	0
	
inParams			set		$
					ds.b	32

StartingRegister&p1	EQU		&p2

	ENDM


#============================================================================================
# Entry Name
#============================================================================================

	MACRO
	Entry	&p1

	csect	[PR]
.&p1:
	stwu	sp, -FrameSize&p1(sp)
	SaveGPR(StartingRegister&p1)
	mflr	r0									# Get the Link Register
	stw		r0, callerSavedLR(sp)				# Save the Link Register
	mfcr	r0
	stw		r0, callerSavedCR(sp)
	
	ENDM


#============================================================================================
# Exit Name
#============================================================================================
	
	MACRO
	Exit	&p1

	lwz		r0, callerSavedCR(sp)
	mtcr	r0
	lwz		r0, callerSavedLR(sp)
	mtlr	r0
	RestoreGPR(StartingRegister&p1)
	addi	sp, sp, FrameSize&p1
	blr
	
	TracebackTable &p1
	
	ENDM



#============================================================================================
# EntryPoint Name
#============================================================================================

	MACRO
	EntryPoint	&p1

	csect [PR]
.&p1:

	ENDM


#============================================================================================
# EntryNoStack Name
#============================================================================================

	MACRO
	EntryNoStack	&p1

	csect	[PR]
.&p1:

	ENDM


#============================================================================================
# ExitNoStack
#============================================================================================

	MACRO
	ExitNoStack	&p1
	
	blr
	
	TracebackTable &p1
	
	ENDM


#============================================================================================
# TracebackTable Name
#============================================================================================

	MACRO
	TracebackTable	&p1
	
	IF DEBUGLEVEL != 0

TTB_&p1:
	dc.l	0				# marks beginning of traceback table
	dc.b	0x00			# version 0
	dc.b	0x0C			# language (C = 0, Asm = 12)
	dc.b	0x20			# flags[0] (0x20 = has offset field)
	dc.b	0x40			# flags[1] (0x40 = has name field)
	dc.b	0x00			# flags[2] (number of saved floats, etc.)
	dc.b	0x00			# flags[3] (number of gprs saved, etc.)
	dc.b	0x00			# flags[4] (number of fixed point params)
	dc.b	0x00			# flags[5] (floating point param info)
	dc.l	TTB_&p1-.&p1	# offset from fn begin to traceback table
	dc.w	STREND_&p1-STRBEG_&p1
STRBEG_&p1:
	dc.w	'&p1'
	ALIGN	2
STREND_&p1:

	ENDIF
	
	ENDM
