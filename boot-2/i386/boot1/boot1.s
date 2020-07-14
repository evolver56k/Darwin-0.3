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
; boot1.s - boot1 written for nasm assembler, since gas only
; generates 32 bit code and this must run in real mode.
; To compile as hard disk boot1:
;	nasm -DBOOTDEV=HDISK boot1.s -o boot1
; To compile as floppy boot1f:
;	nasm -DBOOTDEV=FLOPPY boot1.s -o boot1f

;***********************************************************************
;	This is the code for the NeXT boot1 bootsector.
;***********************************************************************

	SEGMENT .text

	SDEBUG EQU 0

BOOTSEG		EQU	00h
BOOTOFF		EQU	1000h
BUFSZ		EQU	2000h	; 8K disk transfer buffer

; FDISK partition table in sector 0
PARTSTART	EQU	1beh	; starting address of partition table
NUMPART		EQU	4	; number of partitions in partition table
PARTSZ		EQU	16	; each partition table entry is 16 bytes
BOOT_IND	EQU	0	; offset of boot indicator in partition table
BEG_HEAD	EQU	1	; offset of beginning head
BEG_SEC		EQU 	2	; offset of beginning sector
BEG_CYL		EQU	3	; offset of beginning cylinder
NAME_OFFSET	EQU	4	; offset of partition name
PARTSEC		EQU	8	; offset of partition sector specifier
NEXTNAME	EQU	0A7h	; value of boot_ind, means bootable partition

LOADSZ		EQU	88	; maxiumum possible size of unix boot- 44k

FLOPPY		EQU	0
HDISK		EQU	80h

; NeXT disk label
DISKLABEL	EQU	15	; sector num of 2nd disk label, 1st is trashed by bootsec
DL_DISKTAB	EQU	44

; We support disk label version 3 "3Vld" in our little endian world
DL_V3		EQU 	33566c64h

; NeXT disktab
DT_SECSIZE	EQU	48
DT_BOOT0_BLKNO	EQU	80

; This code is a replacement for boot1.  It is loaded at 0x0:0x7c00

start:
	mov	ax,BOOTSEG
	cli			; interrupts off
	mov	ss,ax		; set up stack seg
	mov	sp,0fff0h
	sti			; reenable interrupts

	xor	ax,ax
	mov	es,ax
	mov	ds,ax
	mov	si,7C00h
	cld			; so pointers will get updated
	mov	di,0E000h	; relocate boot program to 0xE000
	mov	cx,100h		; copy 256x2 bytes
	repnz	movsw		; move it
	jmp	0000:0E000h + (a1 - start)	; jump to a1 in relocated place

a1:
	mov	ax,0E00h
	mov	ds,ax
	mov	ax,BOOTSEG
	mov	es,ax

	; load the boot loader (boot2) into BOOTSEG:BUFSZ
	call	loadboot

	; ljmp to the second stage boot loader (boot2).
	; After ljmp, cs is BOOTSEG and boot1 (BUFSZ bytes) will be used
	; as an internal buffer "intbuf".

	xor	edx,edx		; bootdev = 0 for hard disk
%IF	BOOTDEV = FLOPPY
	inc	edx		; bootdev = 1 for floppy disk
%ENDIF

	;boot2 immediately follows disk buffer;  4K + BUFSZ
	jmp	BOOTSEG:(BOOTOFF + BUFSZ)	
				; jump to boot2 in loaded location



loadboot:
	mov	si, intro
	call	message		; display intro message

	; load second stage boot from fixed disk
	; get boot drive parameters
	; Note: I believe that the bootsector read may not be necessary;
	; at least some blk0 bootsectors leave a pointer to the active
	; partition entry in si (assuming there was another blk0 bootsec)

	call	getinfo
	
%IF	BOOTDEV = HDISK
	
	; read sector 0 into BOOTSEG:0 by using BIOS call (INT 13H 02H)
	; this gets info on the disk's actual partition table
	; However, in the case of multiple partitions, this may not
	; be the same as the sector with the code here.

	mov	di,5		; try 5 times to read bootsector

retry_disk:
		xor	bx, bx		; buffer is BOOTSEG:0
		mov	ax,201h
		mov	cx,bx
		mov	dx,bx
		mov	bx,BOOTOFF	; actually, it's 0:BOOTOFF
		inc	cx		; cyl 0, sector 1
		mov	dl,BOOTDEV	; target 0, head 0

		push	di
		int	13h		; read the bootsector
		pop	di

		jnb	read_success1

		; woops, bios failed to read sector
		xor	ax,ax
		int	13h		; reset disk
		dec	di
		jne	retry_disk

		jmp	read_error	; disk failed


read_success1:		; find the NeXT partition
	mov	bx,PARTSTART
	mov	cx,NUMPART

again:
	mov	al, [es:(bx+BOOTOFF)+NAME_OFFSET]
					; get partition name
	cmp	al, NEXTNAME		; is it NeXT partition?
	jne	cont			; nope, keep looking

foundNextPart:				; found it, get label location

	mov	eax, [es:(bx+BOOTOFF)+PARTSEC]
					; offset to NeXT partition
	add	eax, byte DISKLABEL		; add offset to the label
	jmp	short getLabl			; fetch that label

cont:
	add	bx, byte PARTSZ
	loop	again			; if more part table entries,
					; keep looking

	; fall through, didn't find NeXT disk partition entry

no_fdisk:
%ENDIF
	; Read NeXT disk label
	mov	eax, DISKLABEL		; Get block number of label
getLabl:
	mov	bx,BOOTOFF		; read into load area
	mov	cx,1
	call	readSectors

	; we used to think about testing the disk label version here...

	mov	bx,BOOTOFF		; point to beginning of label

	; Use values from label to read entire boot program
	; Get block number of boot
	; Get dl_secsize and dl_boot0_blkno[0]
	mov	edx, [es:(bx + DL_DISKTAB+DT_SECSIZE)]
	bswap	edx			; edx -> sector size

	mov	eax, [es:(bx + DL_DISKTAB+DT_BOOT0_BLKNO)]
	bswap	eax			; eax -> block #

	; Compute dl_secsize * dt_boot0_blkno[0] / 512
	shr	edx, 9			; divide dl_secsize by 512
	mul	edx			; multiply boot block loc
					; by dl_secsize/512
					; eax has secno
	mov	bx, (BUFSZ + BOOTOFF)	; read boot2 into BOOTSEG:BUFSZ
	mov	edi, LOADSZ		; read this many sectors
nexsec:
		push	eax		; push sector #
		push	bx		; push buffer address
		mov	ecx, edi	; max number of sectors to read
		
		call	readSectors
		pop	bx
		pop	eax
		add	eax, ecx	; add number of sectors read
		sub	di, cx
		shl	cx, 9		; multiply by 512 bytes per sector
		add	bx, cx

	cmp di, byte 0
	jne	nexsec

	ret

spt:	DW	0			; sectors;track (one-based)
spc:	DW	0			; tracks;cylinder (zero-based)
nsec:	DW	0			; number of sectors to read

readSectors:	; eax has starting block #, bx has offset from BOOTSEG
		; cx has maximum number of sectors to read
		; Trashes ax, bx, cx, dx

	; BIOS call "INT 0x13 Function 0x2" to read sectors
	;	ah = 0x2	al = number of sectors
	;	ch = cylinder	cl = sector
	;	dh = head	dl = drive (0x80=hard disk, 0=floppy disk)
	;	es:bx = segment:offset of buffer

%IF	BOOTDEV = FLOPPY
		push	eax
		mov	al,'.'
		call	putchr
		pop	eax
%ENDIF

	push	bx			; save offset
	mov [WORD nsec], cx

	mov	ebx, eax		; bx -> block #
	xor	ecx, ecx
	mov	cx, [WORD spc]		; cx -> sectors/cyl

	xor	edx,edx			; prepare for division
	div	ecx			; eax = cyl, edx=remainder
	push	ax			; save cylinder #, sec/spc

	mov	eax, edx
	mov	cx,  [WORD spt]		; ecx -> sectors/track
	xor	edx,edx			; prepare for division
	div	ecx			; eax = head
	push	ax			; save head, (secspc)/spt

	mov	eax, ebx		; reload block #
	xor	edx, edx		; prepare for division
	div	ecx			; edx has sector #

	sub	cx, dx 			;cx now has number of sectors to read
	cmp	cx, [WORD nsec]
	jge	last			; use the minimum of bx and cx
	mov	[WORD nsec], cx
last:

	mov	cx, dx			; cl -> sector
	inc	cl			; starts @ 1
	pop	ax			; get head
	mov	dh, al			; dh -> head

	pop	ax			; get cyl
	mov	ch, al			; ch -> cyl
	mov	dl, BOOTDEV		; floppy disk

	xor	al,al
	shr	ax,2
	or	cl,al			; retain pesky big cyl bits

;	mov	al, 1			; get # of sectors
;pop ax ; number of sectors to read
mov ax, [WORD nsec]
	pop	bx			; get buffer
	mov	ah, 2			; bios read function
	
	int	13h

	jb	read_error
mov cx, [WORD nsec] ; return number of sectors read
	ret


getinfo:	; get some drive parameters
	mov	dl, BOOTDEV		; boot drive is drive C
	mov	ah, 8h
	push	es
	int	13h
	pop	es

	mov	al, dh			; max head #
	inc	al			; al -> tracks/cyl
	and	cx, byte 3fh			; cl -> secs/track
	mul	cl			; ax -> secs/cyl
	mov	[WORD spc], ax
	mov	[WORD spt], cx

	ret


message:				; write the error message in ds:esi
					; to console
	push	es
	mov	ax,ds
	mov	es,ax

	mov	bx, 1			; bh=0, bl=1 (blue)
	cld

nextb:
	lodsb				; load a byte into al
	cmp	al, 0
	je	done
	mov	ah, 0eh			; bios int 10, function 0xe
	int	10h			; bios display a byte in tty mode
	jmp	short nextb
done:	pop	es
	ret

putchr:
	push	bx
	mov	bx, 1			; bh=0, bl=1 (blue)
	mov	ah, 0eh			; bios int 10, function 0xe
	int	10h			; bios display a byte in tty mode
	pop	bx
	ret

%IF	SDEBUG
hexout:				; print ebx as hex number
	push	cx
	push	ax
	mov	cx,8			;output 8 nibbles

htop:
		rol	ebx,4
		mov	al,bl
		and	al,0fh
		cmp	al,0ah
		jb	o_digit
		add	al,'A'-10
		jmp	o_print
o_digit:	add	al,'0'
o_print:	call	putchr
		dec	cx
		jne	htop
;	mov	al,10
;	call	putchr
;	mov	al,13
;	call	putchr
	mov	al,' '
	call	putchr
	pop	ax
	pop	cx
	ret
%ENDIF


read_error:
	mov	si, eread
boot_exit:				; boot_exit: write error message and halt
	call	message			; display error message
halt:	jmp	short halt


intro:	db	10,VERS,10,13,0
eread:	db	'Read error',10,13,0

; the last 2 bytes in the sector contain the signature
d1:
	a2 equ 510 - (d1 - start)
  times a2 db 0
	dw 0AA55h
	ENDS
	END
