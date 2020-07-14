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
/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	libsa/atob.c
 *
 *	Ascii to Binary converter.  Accepts all C numeric formats.
 *
 * HISTORY
 * 13-Sep-91  Mike DeMoney (mike@next.com) and Peter King (king@next.com)
 *	Created from libmike.
 */


/*
 * Header files.
 */
#import "ctype.h"

static unsigned int	digit(char c);

/*
 * atob -- convert ascii to binary.  Accepts all C numeric formats.
 * 	   Returns pointer to next character after last character used.
 */
char *
atob(
	const char		*cp,
	int			*iptr
) {
	int		minus = 0;
	int	 	value = 0;
	unsigned int	base = 10;
	unsigned int	d;

	*iptr = 0;
	if (!cp) {
		return(0);
	}
 
	while (isspace(*cp)) {
		cp++;
	}

	while (*cp == '-') {
		cp++;
		minus = !minus;
	}

	/*
	 * Determine base by looking at first 2 characters
	 */
	if (*cp == '0') {
		switch (*++cp) {
		case 'X':
		case 'x':
			base = 16;
			cp++;
			break;

		case 'B':	/* a frill: allow binary base */
		case 'b':
			base = 2;
			cp++;
			break;
		
		default:
			base = 8;
			break;
		}
	}

	while ((d = digit(*cp)) < base) {
		value *= base;
		value += d;
		cp++;
	}

	if (minus) {
		value = -value;
	}

	*iptr = value;
	return((char *)cp);
}

/*
 * digit -- convert the ascii representation of a digit to its
 * binary representation
 */
static unsigned int
digit(
	char		c
) {
	unsigned int	d;

	if (isdigit(c)) {
		d = c - '0';
	} else if (isalpha(c)) {
		if (isupper(c)) {
			c = tolower(c);
		}
		d = c - 'a' + 10;
	} else {
		d = 999999; /* larger than any base to break callers loop */
	}

	return d;
}

