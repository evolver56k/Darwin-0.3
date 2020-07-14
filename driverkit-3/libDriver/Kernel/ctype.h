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
/*	ctype.h	4.2	85/09/04	*/

/* Copyright (c) 1988 NeXT, Inc. - 9/13/88 CCH */

#ifndef _CTYPE_H
#define _CTYPE_H

#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define _P	020
#define _C	040
#define _X	0100
#define	_B	0200

extern const char _ctype_[];

#define isalnum(c)	((int)((_ctype_+1)[c]&(_U|_L|_N)))
#define	isalpha(c)	((int)((_ctype_+1)[c]&(_U|_L)))
#define iscntrl(c)	((int)((_ctype_+1)[c]&_C))
#define	isdigit(c)	((int)((_ctype_+1)[c]&_N))
#define isgraph(c)	((int)((_ctype_+1)[c]&(_P|_U|_L|_N)))
#define	islower(c)	((int)((_ctype_+1)[c]&_L))
#define isprint(c)	((int)((_ctype_+1)[c]&(_P|_U|_L|_N|_B)))
#define ispunct(c)	((int)((_ctype_+1)[c]&_P))
#define	isspace(c)	((int)((_ctype_+1)[c]&_S))
#define	isupper(c)	((int)((_ctype_+1)[c]&_U))
#define	isxdigit(c)	((int)((_ctype_+1)[c]&(_N|_X)))

#define _tolower(c)	((int)((c)-'A'+'a'))
#define _toupper(c)	((int)((c)-'a'+'A'))

#define tolower(c)	({int _c=(c); isupper(_c) ? _tolower(_c) : _c;})
#define toupper(c)	({int _c=(c); islower(_c) ? _toupper(_c) : _c;})
#define isascii(c)	((unsigned)(c)<=0177)
#define toascii(c)	((int)((c)&0177))

#endif /* _CTYPE_H */
