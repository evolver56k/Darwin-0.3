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
/* -*-mode:Assembler; tab-width: 4 -*- */
DEBUGLEVEL	equ	0
#
#	File:		CompilerSupport.s
#
#	Contains:	C compiler support functions for calling through a fn pointer
#				and for structure copies.
#
#	Version:	System 8
#
#
#	Copyright:	© 1994-1996 by Apple Computer, Inc. - All rights reserved.
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
#		(TS)	Thomas Saulpaugh
#		(TS)	Tom Saulpaugh
#		(JLR)	Jeff Robbin
#
#	Change History (most recent first):
#
#		 <4>	 8/22/96	JLR		Add pointer glue for metrowerks.
#		 <3>	 11/1/95	rob		Replaced the real value of fpxnvcvi to avoid including
#									ProcessorArchitecture.a.
#		 <2>	10/24/95	rob		“include	'ProcessorArchitecture.a'” is not needed.
#		 <1>	10/24/95	rob		First checked in.
#		 <3>	10/23/95	TS		Add 603/604 Support.
#		 <2>	 9/21/95	JLR		Try to make _ptrgl12 work with PPCLink and MrC.
#		 <1>	 3/22/95	JLR		First checked in.
#		 <6>	 3/12/95	WSK		Add _ptrgl12.
#		 <5>	 11/2/94	WSK		Nuke DeclareFn uitrunc.
#		 <4>	10/14/94	WSK		Add uitrunc.
#		 <3>	 10/6/94	WSK		Nuke ovbcopy and memmove.
#		 <2>	 10/6/94	WSK		Well, let's try again with PPCAsm 1.1a2c1 and the symbolics
#									crap.
#		 <1>	 10/3/94	WSK		first checked in
#


	include	'CompilerSupportMacros.s'

	ImportData	gCIPointer
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Call CallCI from C passing the following parameters:
;	int CallCI (void *ciArgs)
;
    EntryNoStack	CallCI

		lwz		r4, .gCIPointer(rtoc)	; Get client interface callback pointer
		lwz		r4, 0(r4)
		mtctr	r4
		bctr							; Call Open Firwmare Client Interface; return directly back to caller
	ExitNoStack		CallCI

	EntryNoStack	CIbreakpoint
		dc.l	0x0FE01234
	ExitNoStack		CIbreakpoint

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#
#	To support the MrC (Symantec) and MetroWerks builds,
#	the following pointer glue must be exported by
#	KernelUtilities.o
#	Note: The "environment variable", r11, that is passed
#		in the _ptrgl routine is not passed to this routine.
#		That parameter is used by Pascal's calling conventions, not
#		not by C's variable number of arguments style.
#
	EntryNoStackGlue	_ptrgl12

		lwz		r0,0(r12)  		# Load address of target routine.
		stw		r2,20(r1)		# Save current TOC address.
		mtctr	r0				# Move target address to Count Reg
		lwz		r2,4(r12)		# Load address of target's TOC.
		bctr					# Branch to target.

	ExitNoStack			_ptrgl12

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#
#	Metrowerks pointer glue for fun.  From MWCRuntime.lib.
#
	EntryNoStackGlue	__ptr_glue

		lwz      r0,0(r12)
		stw      RTOC,20(SP)
		mtctr    r0
		lwz      RTOC,4(r12)
		bctr

	ExitNoStack			__ptr_glue


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Invalidate I-cache for range R3..R3+R4-1
; (This is likely to be the whole i-cache.  Easy come, easy go...)
	EntryNoStack	DataToCode
		li		r0,-32			; Get cache line size mask
		add 	r7,r4,r3		; Calculate ending address value
		addi	r7,r7,32		; Last byte is one less but round up to NEXT cache line multiple
		and 	r7,r7,r0		; Round down to a cache line boundary
		and 	r6,r3,r0		; Get start address rounded down similarly

@loop:	icbi	0,r6			; Invalidate this instruction cache line
		addi	r6,r6,32		; Increment address to next cache block boundary
		cmplw	r6,r7			; Done yet?
		blt 	@loop			; do until all done
		isync
	ExitNoStack		DataToCode
	


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int LoaderSetjmp (LoaderJumpBuffer *bufferP)			// Just like UNIX setjmp

LoaderJumpBuffer	RECORD		0
saveLR				ds.l		1
saveCR				ds.l		1
saveSP				ds.l		1
saveRTOC			ds.l		1
saveGPRs			ds.l		32 - 13			; Space for R13..R31
saveFP14			ds.l		2
saveFP15			ds.l		2
saveFP16			ds.l		2
saveFP17			ds.l		2
saveFP18			ds.l		2
saveFP19			ds.l		2
saveFP20			ds.l		2
saveFP21			ds.l		2
saveFP22			ds.l		2
saveFP23			ds.l		2
saveFP24			ds.l		2
saveFP25			ds.l		2
saveFP26			ds.l		2
saveFP27			ds.l		2
saveFP28			ds.l		2
saveFP29			ds.l		2
saveFP30			ds.l		2
saveFP31			ds.l		2
saveFPSCR			ds.l		1
sizeof				equ			$
					ENDR

	EntryNoStack	LoaderSetjmp
	WITH			LoaderJumpBuffer
		mflr     r5
		mfcr     r6
		stw      r5,saveLR(r3)
		stw      r6,saveCR(r3)
		stw      SP,saveSP(r3)
		stw      RTOC,saveRTOC(r3)
		stmw     r13,saveGPRs(r3)
		mffs     fp0
		stfd     fp14,saveFP14(r3)
		stfd     fp15,saveFP15(r3)
		stfd     fp16,saveFP16(r3)
		stfd     fp17,saveFP17(r3)
		stfd     fp18,saveFP18(r3)
		stfd     fp19,saveFP19(r3)
		stfd     fp20,saveFP20(r3)
		stfd     fp21,saveFP21(r3)
		stfd     fp22,saveFP22(r3)
		stfd     fp23,saveFP23(r3)
		stfd     fp24,saveFP24(r3)
		stfd     fp25,saveFP25(r3)
		stfd     fp26,saveFP26(r3)
		stfd     fp27,saveFP27(r3)
		stfd     fp28,saveFP28(r3)
		stfd     fp29,saveFP29(r3)
		stfd     fp30,saveFP30(r3)
		stfd     fp31,saveFP31(r3)
		stfd     fp0,saveFPSCR(r3)
		li       r3,0
	ENDWITH
	ExitNoStack		LoaderSetjmp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int LoaderLongjmp (LoaderJumpBuffer *bufferP)			// Just like UNIX longjmp

	EntryNoStack	LoaderLongjmp
	WITH			LoaderJumpBuffer
		lwz      r5,saveLR(r3)
		lwz      r6,saveCR(r3)
		mtlr     r5
		mtcrf    0xFF,r6
		lwz      SP,saveSP(r3)
		lwz      RTOC,saveRTOC(r3)
		lmw      r13,saveGPRs(r3)
		lfd      fp14,saveFP14(r3)
		lfd      fp15,saveFP15(r3)
		lfd      fp16,saveFP16(r3)
		lfd      fp17,saveFP17(r3)
		lfd      fp18,saveFP18(r3)
		lfd      fp19,saveFP19(r3)
		lfd      fp20,saveFP20(r3)
		lfd      fp21,saveFP21(r3)
		lfd      fp22,saveFP22(r3)
		lfd      fp23,saveFP23(r3)
		lfd      fp24,saveFP24(r3)
		lfd      fp25,saveFP25(r3)
		lfd      fp26,saveFP26(r3)
		lfd      fp27,saveFP27(r3)
		lfd      fp28,saveFP28(r3)
		lfd      fp29,saveFP29(r3)
		lfd      fp30,saveFP30(r3)
		lfd      fp0,saveFPSCR(r3)
		lfd      fp31,saveFP31(r3)
		cmpwi    r4,0
		mr       r3,r4
		mtfsf    0xFF,fp0
		bnelr
		li       r3,1
	ENDWITH
	ExitNoStack		LoaderLongjmp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#
# @(#)31	1.7  R2/lib/c/misc/ptrgl.s, bos 6/15/90 17:55:16
#
#  COMPONENT_NAME: (LIBCMISC) lib/c/misc 
#
#  FUNCTIONS: ptrgl
#
#  ORIGINS: 27
#
#  IBM CONFIDENTIAL -- (IBM Confidential Restricted when
#  combined with the aggregated modules for this product)
#  
#  (C) COPYRIGHT International Business Machines Corp. 1985, 1989
#  All Rights Reserved
#
#  US Government Users Restricted Rights - Use, duplication or
#  disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
#
#-----------------------------------------------------------------------#

	EntryNoStackGlue	_ptrgl

		lwz		r0,0(r11)  		# Load address of target routine.
		stw		r2,20(r1)		# Save current TOC address.
		mtctr	r0				# Move target address to Count Reg
		lwz		r2,4(r11)		# Load address of target's TOC.
		lwz		r11,8(r11)		# Get environment pointer.
		bctr					# Branch to target.

	ExitNoStack			_ptrgl


	ifndef	LOTSA_ROOM
#	R3	Address of target string
#	R4	Address of source string
#	R5	Length of source string

	EntryNoStack	MoveBytes
		addic.	r5, r5, -1		; Decrement byte count
		lbzx	r0, r4, r5		; Fetch a source byte
		stbx	r0, r3, r5		; Store it
		bgt		.MoveBytes		; Loop for remaining bytes
	ExitNoStack		MoveBytes

	else
# @(#)28	1.23	R2/inc/sys/asdef.s, bos, bos320 6/24/91 10:56:29
#
# COMPONENT_NAME: (CMDAS) Assembler and Macroprocessor 
#
# FUNCTIONS: 
#
# ORIGINS: 3, 27
#
# (C) COPYRIGHT International Business Machines Corp. 1985, 1990
# All Rights Reserved
# Licensed Materials - Property of IBM
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
#
# XCOFF version
#
#	FUNCTIONS: MoveBytes (formerly _moveeq)
#
#	NAME: MoveBytes
#
#	FUNCTION: Equal length character string move
#
#	EXECUTION ENVIRONMENT:
#	Standard register usage and linkage convention.
#	Registers used r0,r3-r12
#	Condition registers used: 0,1
#	No stack requirements.
#
#	NOTES:
#	The source string represented by R5 and R6 is moved to the target string
#	area represented by R3 and R6.
#
#	The strings may be on any address boundary and may be of any length from 0
#	through (2**31)-1 inclusive.  The strings may overlap.  The move is performed
#	in a nondestructive manner (backwards if necessary).
#
#	The addresses are treated as unsigned quantities,
#	i.e., max is 2**32-1.
#
#	The lengths are treated as unsigned quantities,
#	i.e., max is 2**31-1.
#
#	Use of this logic assumes string lengths <= 2**31-1 - Signed arithmetic
#
#	RETURN VALUE DESCRIPTION: Target string modified.
#
#
# The PL.8 entry description is ENTRY(INTEGER VALUE, CHAR(*)) NOSI.
#
#	Calling sequence: MoveBytes
#	R3	Address of target string
#	R4	Address of source string
#	R5	Length of source string
#

	EntryNoStack	MoveBytes

		srwi.	r0,r5,5			# Number of 32-byte chunks to move (L/32),
								# CR0 = short/long move switch.
		bgt		0,movelong		# Branch if > 32 bytes to move.
#
# Short move (0 to 32 bytes).
#
		mtxer	r5				# Set move length (presuming short).
		lswx	r5,r0,r4		# Get source string (kills R5 - R12).
		stswx	r5,r0,r3		# Store it.
		blr						# Return.
#
# Here we check if we have overlapping strings.
# overlap if: source < target && target < source + length
#		r4	<	r3	&&	r3	<	r4	+	r5
#
movelong:
		cmplw	cr1,r4,r3		# CR1 = forward/backward switch.
		bge		cr1,nobackward	# skip if r4 >= r3
		addc	r10,r4,r5		# r10 = r4 + r5 (source + length)
		cmplw	cr1,r3,r10		# Test if long fwd move might destruct
nobackward:
		mtctr	r0				# CTR = num chunks to move.
		li		r0,32			#
		mtxer	r0				# XER = 32 (move length per iteration).
		rlwinm.	r0,r5,0,0x1f	# R0 = remainder length.
		stw		r3,24(r1)		# save target address for return
		subc	r3,r3,r4		# R3 = targ addr - source addr.
		blt		cr1,backward	# B If A(source) < A(target) logically.

forward:
		lswx	r5,r0,r4		# Get 32 bytes of source.
		stswx	r5,r3,r4		# Store it.
		addic	r4,r4,32		# Increment source address.
		bdnz	forward			# Decr count, Br if chunk(s) left to do.

		mtxer	r0				# XER = remainder length.
		lswx	r5,r0,r4		# Get the remainder string.
		stswx	r5,r3,r4		# Store it.
		lwz		r3,24(r1)		# restore target start address for return
		blr						# Return.

backward:
		addc	r4,r4,r5		# R4 = source addr + len

loopb:
		subic	r4,r4,32		# Decrement source address.
		lswx	r5,r0,r4		# Get 32 bytes of source.
		stswx	r5,r3,r4		# Store it.
		bdnz	loopb			# Decr count, Br if chunk(s) left to do.

		subc	r4,r4,r0		# Decrement source address.
		mtxer	r0				# XER = remainder length.
		lswx	r5,r0,r4		# Get the remainder string.
		stswx	r5,r3,r4		# Store it.
		lwz		r3,24(1)		# restore target start address for return

	ExitNoStack		MoveBytes
	endif
