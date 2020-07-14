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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * keycodes.h
 *
 * HISTORY
 * 21 Oct 92	Joe Pasqua
 *      Created based on previous versions.
 */

#ifdef KERNEL_PRIVATE

#ifndef	_KEYCODES_
#define	_KEYCODES_

#define	nul	0x000
#define	soh	0x001
#define	stx	0x002
#define	etx	0x003
#define	eot	0x004
#define	enq	0x005
#define	ack	0x006
#define	bel	0x007
#define	bs	0x008
#define	ht	0x009
#define	nl	0x00a
#define	vt	0x00b
#define	np	0x00c
#define	cr	0x00d
#define	so	0x00e
#define	si	0x00f
#define	dle	0x010
#define	dc1	0x011
#define	dc2	0x012
#define	dc3	0x013
#define	dc4	0x014
#define	nak	0x015
#define	syn	0x016
#define	etb	0x017
#define	can	0x018
#define	em	0x019
#define	sub	0x01a
#define	esc	0x01b
#define	fs	0x01c
#define	gs	0x01d
#define	rs	0x01e
#define	us	0x01f
#define	del	0x07f
#define	inv	0x100		/* invalid key code */
#define	dim	0x101
#define	quiet	0x102
#define	left	0x103
#define	down	0x104
#define	right	0x105
#define	up	0x106
#define	bright	0x107
#define	loud	0x108
#define	mouse_l	0x109
#define	ret	'\r'
#define	tab	'\t'

// The following table maps scancodes from the PC keyboard into ascii
// characters. Each two entries in the table represent a single character.
// The first value is used whn the shift key is NOT pressed. The second
// value is used wheh the shift key IS preseed. This consumes the first
// _N_KEYCODES * 2 entries in the table. After that, there are another
// _N_KEYCODES * 2 entries that represent the ascii to use for the given
// keycode when the control key is pressed.

#define	_N_KEYCODES	0x6E
u_short ascii[_N_KEYCODES * 4] = {
	// The following entries represent the non-control keys and their
	// shift values (in that order). Each pair of entries represents 1 key.
	inv, inv,	esc, esc,	'1', '!',	'2', '@',  // 00-03
	'3', '#',	'4', '$',	'5', '%',	'6', '^',  // 04-07
	'7', '&',	'8', '*',	'9', '(',	'0', ')',  // 08-0B
	'-', '_',	'=', '+',	del, bs,	tab, tab,  // 0C-0F
	'q', 'Q',	'w', 'W',	'e', 'E',	'r', 'R',  // 10-13
	't', 'T',	'y', 'Y',	'u', 'U',	'i', 'I',  // 14-17
	'o', 'O',	'p', 'P',	'[', '{',	']', '}',  // 18-1B
	ret, ret,	inv, inv,	'a', 'A', 	's', 'S',  // 1C-1F
	'd', 'D',	'f', 'F',	'g', 'G',	'h', 'H',  // 20-23
	'j', 'J',	'k', 'K',	'l', 'L',	';', ':',  // 24-27
	'\'', '"',	'`', '~',	inv, inv,	'\\', '|', // 28-2B
	'z', 'Z',	'x', 'X',	'c', 'C',	'v', 'V',  // 2C-2F
	'b', 'B',	'n', 'N',	'm', 'M',	',', '<',  // 30-33
	'.', '>',	'/', '?',	inv, inv,	'*', '*',  // 34-37
	inv, inv,	' ', ' ',	inv, inv,	inv, inv,  // 38-3B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 3C-3F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 40-43
	inv, inv,	'`', '~',	inv, inv,	'7', '7',  // 44-47
	'8', '8',	'9', '9',	'-', '-',	'4', '4',  // 48-4B
	'5', '5',	'6', '6',	'+', '+',	'1', '1',  // 4C-4F
	'2', '2',	'3', '3',	'0', '0',	'.', '.',  // 50-53
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 54-57
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 58-5B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 5C-5F
	inv, inv,	inv, inv,	ret, ret,	'/', '/',  // 60-63
	up, up,		down, down,	left, left,	right, right, // 64-67
	loud, loud,	quiet, quiet,	bright, bright,	dim, dim,  // 68-6B
	inv, inv,	inv, inv,				   // 6C-6D
	
	// The following entries represent the control and control-shift
	// keys (in that order). Each pair of entries represents 1 key.
	inv, inv,	esc, esc,	'1', '!',	nul, nul,  // 00-03
	'3', '#',	'4', '$',	'5', '%',	rs, rs,    // 04-07
	'7', '&',	'8', '*',	'9', '(',	'0', ')',  // 08-0B
	us, us, 	'=', '+',	inv, inv,	tab, tab,  // 0C-0F
	dc1, dc1,	etb, etb,	enq, enq,	dc2, dc2,  // 10-13
	dc4, dc4,	em, em, 	nak, nak,	ht, ht,    // 14-17
	si, si, 	dle, dle,	esc, esc,	fs, fs,    // 18-1B
	ret, ret,	inv, inv,	soh, soh, 	dc3, dc3,  // 1C-1F
	eot, eot,	ack, ack,	bel, bel,	bs, bs,    // 20-23
	nl, nl, 	vt, vt, 	np, np, 	';', ':',  // 24-27
	'\'', '"',	'`', '~',	inv, inv,	fs, fs,    // 28-2B
	sub, sub,	can, can,	etx, etx,	syn, syn,  // 2C-2F
	stx, stx,	so, so, 	cr, cr, 	',', '<',  // 30-33
	'.', '>',	'/', '?',	inv, inv,	'*', '*',  // 34-37
	inv, inv,	nul, nul,	inv, inv,	inv, inv,  // 38-3B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 3C-3F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 40-43
	inv, inv,	'`', '~',	inv, inv,	'7', '7',  // 44-47
	'8', '8',	'9', '9',	'-', '-',	'4', '4',  // 48-4B
	'5', '5',	'6', '6',	'+', '+',	'1', '1',  // 4C-4F
	'2', '2',	'3', '3',	'0', '0',	'.', '.',  // 50-53
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 54-57
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 58-5B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 5C-5F
	inv, inv,	inv, inv,	ret, ret,	'/', fs,   // 60-63
	up, up,		down, down,	left, left,	right, right, // 64-67
	loud, loud,	quiet, quiet,	bright, bright,	dim, dim,  // 68-6B
	inv, inv,	inv, inv				   // 6C-6D
};

#endif	/* _KEYCODES_ */

#endif	/* KERNEL_PRIVATE */
