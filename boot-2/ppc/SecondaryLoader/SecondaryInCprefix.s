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
;
;	File:		SecondaryInC.prefix.s
;
;	Contains:	
;
;	Copyright:	© 1996 by Apple Computer, Inc., all rights reserved.
;
;	Version:	Maxwell
;
;	File Ownership:
;
;		DRI:				Alan Mimms
;
;		Other Contact:		Pradeep Kathail
;
;		Technology:			MacOS
;
;	Writers:
;
;		(ABM)	Alan Mimms
;
;	Change History (most recent first):
;
;		 <0+>	12/12/96	ABM		Converted from InterpretPrefix.s to be a prefix for the new
;									C based 8k byte Secondary Loader.
;		 <5>	11/27/96	ABM		Fix i-cache invalidate loop controls.
;		 <4>	11/25/96	ABM		Invalidate i-cache PROPERLY. Also notice if 2ary loader fails.
;		 <3>	11/13/96	ABM		Needs to be i-cache coherent for code that is loaded by FORTH.
;		 <2>	 4/22/96	ABM		Ditch bogus boot-unit# scheme in favor of patching to make the
;									"right way" work.
;		 <2>	 4/22/96	ABM		Ditch bogus boot-unit# scheme in favor of patching to make the
;									"right way" work.
;		 <1>	 4/16/96	ABM		First checked in.
;		<4+>	 4/15/96	ABM		#1339239: Still not fixed: It's not SAFE to return from a client
;									program, I think.  Use broken system folder icon
;									stuff in Secondary Loader instead.
;		 <4>	 4/15/96	ABM		#1340785: Add alignment directive at end since assembler seems
;									unsure of the size of a NUL byte
;		 <3>	  4/9/96	ABM		#1339239: Return to caller if secondary loader returns ZERO for
;									tertiary loader's %srr0 value.
;		 <2>	  3/7/96	ABM		Return to using mac-parts style load of secondary loader.  Add
;									more parameters passed from secondary loader to tertiary loader
;									it finds and loads.
;		 <1>	  3/5/96	ABM		This is the prefix I would use to get interpreted FORTH to
;							execute as loaded by mac-parts.
;
				STRING	ASIS

	; Open Firmware calls us with R5 set to the Client Interface callback function pointer.
	; This function takes a parameter vector pointer in R3 and returns in integer in R3
	; that is ZERO on success and nonzero otherwise.  Even though the PowerPC binding for
	; Open Firmware claims we're given a valid stack in R1, on Apple's machines
	; containing Open Firmware versions less than 3.0 we get garbage (0xDEADBEEF to be precise)
	; in R1 so we must allocate our own.  The entire purpose of this hunk of kludgery is that
	; allocation of a stack and setup of R1 so our secondary loader can run properly.

StartOfPrefix:
				bl	nextCode
args:			; Symbol for where %LR points during nextCode's execution.  Must immediately follow BL above.

interpretArg:	dc.l	0				; command "interpret"
nArgs:			dc.l	1				; N-args
nReturns:		dc.l	2				; N-returns
forthArg:		dc.l	0				; ( -- SP-value )
catchResult:	dc.l	0				; catch-result
stackBase:		dc.l	0				; SP-value

interpret:		dc.b	'interpret', 0

; Allocate an entirely zero-filled stack and return stack pointer value at bottom of red zone
				; ( -- SP-value )
forth:			dc.b	'hex 8000 dup alloc-mem 2dup swap '	; ( 32k stack stack 32k )
				dc.b	'erase '							; ( 32k stack )
				dc.b	'+ E0 -', 0							; ( SP )

				ALIGN	2
nextCode:		mflr	r30							; Get pointer to args in a nonvolatile register
				la		r3,interpret-args(r30)		; Get address of interpret string
				stw		r3,interpretArg-args(r30)	; Save back absolute address
				la		r3,forth-args(r30)			; Get address of forth string to interpret
				stw		r3,forthArg-args(r30)		; Save back absolute address
				mr		r3, r30						; Point at our parameters array for "interpret" CI callback
				mtctr	r5							; Get our client interface callback pointer
				bctrl								; Call O/F's client interface to run forth

	; When we return, returned value is in args structure.
				lwz		r1,stackBase-args(r30)		; Set up stack pointer

	; We fall into the immediately-appended SelfPEFLoader which in turn immediately precedes
	; the Secondary Loader PEF container.  We have to do this carefully since our build
	; mechanism PADS to doubleword boundaries the pieces it appends together.  I have been
	; repeatedly and brutally "thwarted" (there are other words for it) by the tools, so I am
	; using a brute-force mechanism to pad with branches that do basically nothing.  Some
	; assemblers are too "smart" for me...
	;
	; And on top of THAT this %^$^%@#^@^#^&*% assembler has no REPEAT directive!
PaddingRequired		equ		16 - (($-StartOfPrefix) % 16)

		if	PaddingRequired > 0
				b		SelfPEFLoaderGoesHere
		endif

		if	PaddingRequired > 4
				b		SelfPEFLoaderGoesHere
		endif

		if	PaddingRequired > 8
				b		SelfPEFLoaderGoesHere
		endif

SelfPEFLoaderGoesHere:
				end
