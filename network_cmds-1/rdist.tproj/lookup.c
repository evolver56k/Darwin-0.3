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
/* 
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 *	@(#)lookup.c	8.1 (Berkeley) 6/9/93
 */


#include "defs.h"

	/* symbol types */
#define VAR	1
#define CONST	2

struct syment {
	int	s_type;
	char	*s_name;
	struct	namelist *s_value;
	struct	syment *s_next;
};

static struct syment *hashtab[HASHSIZE];

/*
 * Define a variable from a command line argument.
 */
void
define(name)
	char *name;
{
	register char *cp, *s;
	register struct namelist *nl;
	struct namelist *value;

	if (debug)
		printf("define(%s)\n", name);

	cp = index(name, '=');
	if (cp == NULL)
		value = NULL;
	else if (cp[1] == '\0') {
		*cp = '\0';
		value = NULL;
	} else if (cp[1] != '(') {
		*cp++ = '\0';
		value = makenl(cp);
	} else {
		nl = NULL;
		*cp++ = '\0';
		do
			cp++;
		while (*cp == ' ' || *cp == '\t');
		for (s = cp; ; s++) {
			switch (*s) {
			case ')':
				*s = '\0';
			case '\0':
				break;
			case ' ':
			case '\t':
				*s++ = '\0';
				while (*s == ' ' || *s == '\t')
					s++;
				if (*s == ')')
					*s = '\0';
				break;
			default:
				continue;
			}
			if (nl == NULL)
				value = nl = makenl(cp);
			else {
				nl->n_next = makenl(cp);
				nl = nl->n_next;
			}
			if (*s == '\0')
				break;
			cp = s;
		}
	}
	(void) lookup(name, REPLACE, value);
}

/*
 * Lookup name in the table and return a pointer to it.
 * LOOKUP - just do lookup, return NULL if not found.
 * INSERT - insert name with value, error if already defined.
 * REPLACE - insert or replace name with value.
 */

struct namelist *
lookup(name, action, value)
	char *name;
	int action;
	struct namelist *value;
{
	register unsigned n;
	register char *cp;
	register struct syment *s;
	char buf[256];

	if (debug)
		printf("lookup(%s, %d, %x)\n", name, action, value);

	n = 0;
	for (cp = name; *cp; )
		n += *cp++;
	n %= HASHSIZE;

	for (s = hashtab[n]; s != NULL; s = s->s_next) {
		if (strcmp(name, s->s_name))
			continue;
		if (action != LOOKUP) {
			if (action != INSERT || s->s_type != CONST) {
				(void)sprintf(buf, "%s redefined", name);
				yyerror(buf);
			}
		}
		return(s->s_value);
	}

	if (action == LOOKUP) {
		(void)sprintf(buf, "%s undefined", name);
		yyerror(buf);
		return(NULL);
	}

	s = ALLOC(syment);
	if (s == NULL)
		fatal("ran out of memory\n");
	s->s_next = hashtab[n];
	hashtab[n] = s;
	s->s_type = action == INSERT ? VAR : CONST;
	s->s_name = name;
	s->s_value = value;
	return(value);
}
