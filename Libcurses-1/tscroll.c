/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "curses.h"

#define	MAXRETURNSIZE	64

/*
 * Routine to perform scrolling.  Derived from tgoto.c in tercamp(3)
 * library.  Cap is a string containing printf type escapes to allow
 * scrolling.  The following escapes are defined for substituting n:
 *
 *	%d	as in printf
 *	%2	like %2d
 *	%3	like %3d
 *	%.	gives %c hacking special case characters
 *	%+x	like %c but adding x first
 *
 *	The codes below affect the state but don't use up a value.
 *
 *	%>xy	if value > x add y
 *	%i	increments n
 *	%%	gives %
 *	%B	BCD (2 decimal digits encoded in one byte)
 *	%D	Delta Data (backwards bcd)
 *
 * all other characters are ``self-inserting''.
 */
char *
__tscroll(cap, n1, n2)
	const char *cap;
	int n1, n2;
{
	static char result[MAXRETURNSIZE];
	int c, n;
	char *dp;

	if (cap == NULL)
		goto err;
	for (n = n1, dp = result; (c = *cap++) != '\0';) {
		if (c != '%') {
			*dp++ = c;
			continue;
		}
		switch (c = *cap++) {
		case 'n':
			n ^= 0140;
			continue;
		case 'd':
			if (n < 10)
				goto one;
			if (n < 100)
				goto two;
			/* FALLTHROUGH */
		case '3':
			*dp++ = (n / 100) | '0';
			n %= 100;
			/* FALLTHROUGH */
		case '2':
two:			*dp++ = n / 10 | '0';
one:			*dp++ = n % 10 | '0';
			n = n2;
			continue;
		case '>':
			if (n > *cap++)
				n += *cap++;
			else
				cap++;
			continue;
		case '+':
			n += *cap++;
			/* FALLTHROUGH */
		case '.':
			*dp++ = n;
			continue;
		case 'i':
			n++;
			continue;
		case '%':
			*dp++ = c;
			continue;
		case 'B':
			n = (n / 10 << 4) + n % 10;
			continue;
		case 'D':
			n = n - 2 * (n % 16);
			continue;
		/*
		 * XXX
		 * System V terminfo files have lots of extra gunk.
		 * The only one we've seen in scrolling strings is
		 * %pN, and it seems to work okay if we ignore it.
		 */
		case 'p':
			++cap;
			continue;
		default:
			goto err;
		}
	}
	*dp = '\0';
	return (result);

err:	return("curses: __tscroll failed");
}
