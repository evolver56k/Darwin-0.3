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
;
;	File:		InterpretPrefix.s
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
;		Other Contact:		Stanford Au
;
;		Technology:			MacOS
;
;	Writers:
;
;		(ABM)	Alan Mimms
;
;	Change History (most recent first):
;
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
		bl	nextCode
args:	; Symbol for where %LR points during nextCode's execution.  Must immediately follow BL above.
interpretArg:	dc.l	interpret-args		; command "interpret"
		dc.l	1			; N-args
		dc.l	6			; N-returns
forthArg:	dc.l	forth-args		; ( -- entry-addr loaded-size new-sp disk-ih partition#|0 )
		dc.l	0			; catch-result
partNumArg:	dc.l	0			; partition#
diskIHandleArg:	dc.l	0			; disk-ih
newSPArg:	dc.l	0			; new-stack-pointer
loadSizeArg:	dc.l	0			; loaded-size
entryAddrArg:	dc.l	0			; entry-address

nextCode:	mflr	r30			; Get pointer to args
		mr	r3,r30			; Save parameter for O/F client interface callback
		lwz	r0,interpretArg-args(r30)	; Get offset to interpret string
		add	r0,r0,r30		; Offset to absolute address
		stw	r0,interpretArg-args(r30)	; Save back absolute address
	
		lwz	r0,forthArg-args(r30)	; Get offset to forth to interpret
		add	r0,r0,r30		; Offset to absolute address
		stw	r0,forthArg-args(r30)	; Save back absolute address
	
		mtctr	r5			; Get our client interface callback pointer
		bctrl				; Call O/F's client interface to run forth
	
; When we return, returned values are in args structure.
; Set up runtime environment loaded code requires and enter the next loader phase.
		lwz	r4,partNumArg-args(r30)
		cmpwi	r4,0			; Did we fail?
		beqlr				; Return to O/F outer layer if we failed
		lwz	r3,diskIHandleArg-args(r30)
		lwz	r1,newSPArg-args(r30)
		lwz	r6,entryAddrArg-args(r30)
		lwz	r7,loadSizeArg-args(30)
		
	; Invalidate I-cache for range entryAddr..entryAddr+loadSize-1
	; (This is likely to be the whole i-cache.  Easy come, easy go...)
		li	r0,-32			; Get cache line size mask
		add 	r17,r7,r6		; Calculate ending address value
		addi	r17,r17,32		; Last byte is one less but round up to NEXT cache line multiple
		and 	r17,r17,r0		; Round down to a cache line boundary
		and 	r16,r6,r0		; Get start address rounded down similarly

syncCacheLoop:
		icbi	0,r16			; Invalidate this instruction cache line
		addi	r16,r16,32		; Increment address to next cache block boundary
		cmplw	r16,r17			; Done yet?
		blt 	syncCacheLoop		; do until all done
		isync

;	dc.l	0x0FE01234	; DEBUGGING

		mtctr	r6			; Get tertiary loader's entry point
		bctrl				; Off to the tertiary loader! (only to return on dire error)
		dc.l	0x0FE01275		; Just in case we get back do an Open Firmware debugger breakpoint

interpret:	dc.b	'interpret',0
		align	2

forth:		; Here the build process appends the NUL-terminated string containing the
		; Open Firmware forth code to interpret

		end
