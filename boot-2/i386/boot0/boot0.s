; Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
;
; @APPLE_LICENSE_HEADER_START@
; 
; Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
; Reserved.  This file contains Original Code and/or Modifications of
; Original Code as defined in and that are subject to the Apple Public
; Source License Version 1.1 (the "License").  You may not use this file
; except in compliance with the License.  Please obtain a copy of the
; License at http://www.apple.com/publicsource and read it before using
; this file.
; 
; The Original Code and all software distributed under the License are
; distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
; EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
; INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
; License for the specific language governing rights and limitations
; under the License.
; 
; @APPLE_LICENSE_HEADER_END@
;
; This boot code (in intel format) finds and boots the
; active partition of a dos disk
; to assemble: 
;     nasm boot0.s -o boot0

	SEGMENT .text

start:	cli			; interrupts off
	xor	ax,ax
	mov	ss,ax		; set up stack seg
	mov	sp,7c00h	; stack grows up from code load area
	mov	si,sp		; set up source pointer

	push	ax
	pop	es		; es -> 0
	push	ax
	pop	ds		; ds -> 0
	sti			; reenable interrupts

	cld			; so pointers will get updated
	mov	di,600h		; relocate boot program to 0x600
	mov	cx,100h		; copy 256x2 bytes
	repnz	movsw		; move it
	jmp	0000:600h + (a1 - start)	; jump to a1 in relocated place

a1:
	mov 	si,600h + (intro - start)	; "NeXT boot"
	call 	msg

;------- retrieve and clean up CMOS day of week ------------
	mov	al,6
	out	70h,al		; select CMOS address 6
	nop			; delay
	in	al,71h
	mov	bl,al		; save old val in bl
	mov	dl,al		; save old val in dl
	and	bl,7h		; old day
	mov	al,6
	out	70h,al		; select CMOS address 6
	nop			; delay
	mov	al,bl
	out	71h,al		;restore day of week

	;cmos value now in dl

;------- Look for keyboard input ---------------------------
	call	getTime
	add	ax,4		; 3 seconds
	mov	bx,ax
l1:
	mov	ah,1
	int	16h		; key available?
	jne	gotkey		; got one!

	call	getTime
	cmp	ax,bx
	jle	l1

;-----------------------------------------------------------
findCMOS:
	mov	al,dl
	and	al,10h		; reboot NeXT?
	je	fc1		; no
	mov	al,'r'
	jmp	short tryletter	; pretend found keystroke

fc1:	mov	al,dl
	and	al,20h		; reboot dos?
	je	findactive
	mov	al,'d'
	jmp	short tryletter	; pretend found keystroke

findactive:
	mov 	si,7beh		; 0600+1be = relocated partition table
	mov	bl,4		; 4 entries

check_active:
	cmp 	BYTE [si],80h	; is it active?
	je 	found_active	; yes

	add	si,byte 10h		; next partition table entry
	dec 	bl
	jne 	check_active

	int	18h		; let bios handle boot failure

;------- found keyboard input ---------------------------
gotkey:	xor	ah,ah
	int	16h		;get it
	xor	ah,ah
	cmp	al,' '
	; je	findactive	; on space, just look for active
	je	findCMOS	; on space, boot without waiting
	cmp	al,13
	; je	findactive	; on ret, just look for active
	je	findCMOS	; on ret, boot without waiting

	cmp	al,'1'		; look for 1-4
	jb	nonesuch
	cmp	al,'4'
	jnbe	tryletter
	sub	al,'1'
	mov	cl,10h
	mul	cl		; al->offset to specified partition
	mov 	si,7beh		; 0600+1be = relocated partition table
	add	si,ax		; si->specified name
	cmp	BYTE [si+4],0	; check name
	je 	nonesuch
	jmp	short found_active	; part has name, load it

tryletter:
	mov	cl,0a7h		; preload NeXT name
	cmp	al,'r'		; did user ask for next partition?
	je	t2		; yep
	mov	cl,6		; preload DOS large partition name
	cmp	al,'d'		; did user ask for dos partition?
	jne	nonesuch	; no

t2:	mov 	si,7beh		; 0600+1be = relocated partition table
	mov	bl,4		; 4 entries

t1:
	cmp 	BYTE [si+4],cl	; is it requested partition?
	je 	found_active	; yes

	add	si,byte 10h		; next partition table entry
	dec 	bl
	jne 	t1

	mov	al,cl
	cmp	al,6		; dos requested, didn't find big dos
	jne	ns0
	mov	cl,1		;look for small dos
	jmp	short t2

nonesuch:
	mov 	si,600h + (nopart - start)	; "No such partition"
	call 	msg
	jmp	a1
;-----------------------------------------------------------

msg:	lodsb
	cmp	al,0		; eos?
	je	msgDone 
	push 	si
	mov 	bx,7		; colors
	mov 	ah,0eh
	int	10h		; bios output char as teletype
	pop	si
	jmp 	short msg
msgDone: ret

msgHang:
	call msg
	int	18h		; let bios handle boot problem
hang:	jmp 	short hang		; output string done, just hang around

found_active: mov	dx,[si]	; save params from part table to load partition bootsec
	mov	dl,80h		;in case it wasn't really active
	mov	cx,[si+2]

	mov	bp,si		; bp points to active partition entry

	mov	di,5		; try 5 times
bios_retry:
	mov 	bx,7c00h	; load into expected boot buffer
	mov 	ax,0201h	; bios read 1 sector, have preloaded
				; partition values into dx & cx (cool!)
	push	di
	int 	13h
	pop 	di
	jnb 	bios_read_fine

	xor	ax,ax
	int 	13h		; reset boot drive
	dec 	di
	jne	bios_retry

	mov 	si,600h + (ero - start)		; "Error loading OS"
	jmp 	short msgHang

ns0:
	cmp	al,1		; dos requested, didn't find small dos
	jne	nonesuch
	mov	cl,4		;look for small dos 16 bit fat
	jmp	short t2

bios_read_fine:
	mov 	si,600h + (mis - start)		; "Missing OS"
	mov 	di,7dfeh	; look for signature on active partition's bootsec
	cmp	WORD [di],0aa55h
	jne 	msgHang
	mov 	si,bp		; si points to active partition so bootsec needn't be reread
	jmp 	0000:7c00h	; jump to loaded boot code

getTime:
	push	dx
	mov	ah,0
	int	1ah		; get system timer
	and	cx,byte 0fh
	mov	ax,cx
	and	dx,byte 0fff0h
	or	ax,dx
	pop	dx
	ror	ax,1
	ror	ax,1
	ror	ax,1
	ror	ax,1
	ret

intro:	db	'Type r for Rhapsody, d for DOS or Windows, 1-4 for partition #:',10,13,0
ero:	db 	'Cant load OS',10,13,0
mis:	db 	'Missing OS',10,13,0
nopart:	db	'No such partition',10,13,0
	;... followed eventually by partition table and sig

d1z:
	a2z equ 444 - (d1z - start)
	times a2z db 0
	db 01          ; boot0 version
	db 0a7h	       ; Next BOOT0 id
d1:
	a2 equ 510 - (d1 - start)
	times a2 db 0
	dw 0AA55h
	ENDS
	END
