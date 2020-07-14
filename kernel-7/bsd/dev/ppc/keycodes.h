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
 * 04 April 97	Simon Douglas
 *      Created based on previous versions.
 *
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

// The following table maps scancodes from the ADB keyboard into ascii
// characters. Each two entries in the table represent a single character.
// The first value is used whn the shift key is NOT pressed. The second
// value is used wheh the shift key IS preseed. This consumes the first
// _N_KEYCODES * 2 entries in the table. After that, there are another
// _N_KEYCODES * 2 entries that represent the ascii to use for the given
// keycode when the control key is pressed.

#define	_N_KEYCODES	0x80
u_short keycodeToAscii[_N_KEYCODES * 4] = {
	// The following entries represent the non-control keys and their
	// shift values (in that order). Each pair of entries represents 1 key.

	'a', 'A',	's', 'S',	'd', 'D',	'f', 'F',  // 00-03
	'h', 'H',	'g', 'G',	'z', 'Z',	'x', 'X',  // 04-07
	'c', 'C',	'v', 'V',	inv, inv,	'b', 'B',  // 08-0B
	'q', 'Q',	'w', 'W',	'e', 'E',	'r', 'R',  // 0C-0F
	'y', 'Y',	't', 'T',	'1', '!',	'2', '@',  // 10-13
	'3', '#',	'4', '$',	'6', '^',	'5', '%',  // 14-17
	'=', '+',	'9', '(',	'7', '&',	'-', '_',  // 18-1B
	'8', '*',	'0', ')',	']', '}', 	'o', 'O',  // 1C-1F
	'u', 'U',	'[', '{',	'i', 'I',	'p', 'P',  // 20-23
	ret, ret,	'l', 'L',	'j', 'J',	'\'', '"', // 24-27
	'k', 'K',	';', ':',	'\\', '|',	',', '<',  // 28-2B
	'/', '?',	'n', 'N',	'm', 'M',	'.', '>',  // 2C-2F
	tab, tab,	' ', ' ',	'`', '~',	del, bs,   // 30-33
	inv, inv,	esc, esc,	inv, inv,	inv, inv,  // 34-37
	inv, inv,	inv, inv,	inv, inv,	left, left, // 38-3B
	right, right,	down, down,	up, up,		inv, inv,  // 3C-3F
	inv, inv,	'.', '.',	inv, inv,	'*', '*',  // 40-43
	inv, inv,	'+', '+',	inv, inv,	inv, inv,  // 44-47
	inv, inv,	inv, inv,	inv, inv,	'/', '/',  // 48-4B
	ret, ret,	inv, inv,	'-', '-',	inv, inv,  // 4C-4F
	inv, inv,	'=', '=',	'0', '0',	'1', '1',  // 50-53
	'2', '2',	'3', '3',	'4', '4',	'5', '5',  // 54-57
	'6', '6',	'7', '7',	inv, inv,	'8', '8',  // 58-5B
	'9', '9',	inv, inv,	inv, inv,	inv, inv,  // 5C-5F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 60-63
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 64-67
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 68-6B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 6C-6F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 70-73
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 74-77
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 78-7B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 7C-7F

	// The following entries represent the control and control-shift
	// keys (in that order). Each pair of entries represents 1 key.

	soh, soh,	dc3, dc3,	eot, eot,	ack, ack,  // 00-03
	bs, bs, 	bel, bel,	sub, sub,	can, can,  // 04-07
	etx, etx,	syn, syn,	inv, inv,	stx, stx,  // 08-0B
	dc1, dc1,	etb, etb,	enq, enq,	dc2, dc2,  // 0C-0F
	em, em, 	dc4, dc4,	'1', '!',	nul, nul,  // 10-13
	'3', '#',	'4', '$',	rs, rs, 	'5', '%',  // 14-17
	'=', '+',	'9', '(',	'7', '&',	us, us,    // 18-1B
	'8', '*',	'0', ')',	fs, fs,  	si, si,    // 1C-1F
	nak, nak,	esc, esc,	ht, ht, 	dle, dle,  // 20-23
	ret, ret,	np, np, 	nl, nl, 	'\'', '"', // 24-27
	vt, vt, 	';', ':',	fs, fs, 	',', '<',  // 28-2B
	'/', '?',	so, so, 	cr, cr, 	'.', '>',  // 2C-2F
	tab, tab,	nul, nul,	'`', '~',	quiet, quiet,  // 30-33(del)
	inv, inv,	esc, esc,	inv, inv,	inv, inv,  // 34-37
	inv, inv,	inv, inv,	inv, inv,	left, left, // 38-3B
	right, right,	down, down,	up, up,		inv, inv,  // 3C-3F
	inv, inv,	'.', '.',	inv, inv,	'*', '*',  // 40-43
	inv, inv,	'+', '+',	inv, inv,	inv, inv,  // 44-47
	inv, inv,	inv, inv,	inv, inv,	'/', '/',  // 48-4B
	ret, ret,	inv, inv,	'-', '-',	inv, inv,  // 4C-4F
	inv, inv,	'=', '=',	'0', '0',	'1', '1',  // 50-53
	'2', '2',	'3', '3',	'4', '4',	'5', '5',  // 54-57
	'6', '6',	'7', '7',	inv, inv,	'8', '8',  // 58-5B
	'9', '9',	inv, inv,	inv, inv,	inv, inv,  // 5C-5F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 60-63
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 64-67
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 68-6B
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 6C-6F
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 70-73
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 74-77
	inv, inv,	inv, inv,	inv, inv,	inv, inv,  // 78-7B
	inv, inv,	inv, inv,	inv, inv,	inv, inv   // 7C-7F
};

#endif	/* _KEYCODES_ */

#endif	/* KERNEL_PRIVATE */
