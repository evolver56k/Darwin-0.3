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
/*	$OpenBSD: etherent.c,v 1.5 1996/09/16 02:33:04 tholo Exp $	*/

/*
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef lint
static char rcsid[] =
    "@(#) Header: etherent.c,v 1.18 95/10/07 03:08:12 leres Exp (LBL)";
#endif

#include <sys/types.h>

#include <ctype.h>
#include <memory.h>
#include <pcap.h>
#include <pcap-namedb.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

#include "pcap-int.h"

static __inline int xdtoi(int);
static __inline int skip_space(FILE *);
static __inline int skip_line(FILE *);

/* Hex digit to integer. */
static __inline int
xdtoi(c)
	register int c;
{
	if (isdigit(c))
		return c - '0';
	else if (islower(c))
		return c - 'a' + 10;
	else
		return c - 'A' + 10;
}

static __inline int
skip_space(f)
	FILE *f;
{
	int c;

	do {
		c = getc(f);
	} while (isspace(c) && c != '\n');

	return c;
}

static __inline int
skip_line(f)
	FILE *f;
{
	int c;

	do
		c = getc(f);
	while (c != '\n' && c != EOF);

	return c;
}

struct pcap_etherent *
pcap_next_etherent(FILE *fp)
{
	register int c, d, i;
	char *bp;
	static struct pcap_etherent e;

	memset((char *)&e, 0, sizeof(e));
	do {
		/* Find addr */
		c = skip_space(fp);
		if (c == '\n')
			continue;

		/* If this is a comment, or first thing on line
		   cannot be etehrnet address, skip the line. */
		if (!isxdigit(c)) {
			c = skip_line(fp);
			continue;
		}

		/* must be the start of an address */
		for (i = 0; i < 6; i += 1) {
			d = xdtoi(c);
			c = getc(fp);
			if (isxdigit(c)) {
				d <<= 4;
				d |= xdtoi(c);
				c = getc(fp);
			}
			e.addr[i] = d;
			if (c != ':')
				break;
			c = getc(fp);
		}
		if (c == EOF)
			break;

		/* Must be whitespace */
		if (!isspace(c)) {
			c = skip_line(fp);
			continue;
		}
		c = skip_space(fp);

		/* hit end of line... */
		if (c == '\n')
			continue;

		if (c == '#') {
			c = skip_line(fp);
			continue;
		}

		/* pick up name */
		bp = e.name;
		/* Use 'd' to prevent buffer overflow. */
		d = sizeof(e.name) - 1;
		do {
			*bp++ = c;
			c = getc(fp);
		} while (!isspace(c) && c != EOF && --d > 0);
		*bp = '\0';

		/* Eat trailing junk */
		if (c != '\n')
			(void)skip_line(fp);

		return &e;

	} while (c != EOF);

	return (NULL);
}
