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
/*****************************************************
 * ABSTRACT:
 *   String utility programs to allocate and copy
 *   or concatenate, free and return constant strings.
 *
 *	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/string.c,v 1.1.1.1 1999/04/15 18:30:37 wsanchez Exp $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Declare and initialize charNULL here for strNull def in string.h
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 ***************************************************/

#define	EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <sys/types.h>
#include "error.h"
#include "alloc.h"
#include "string.h"

char	charNULL = 0;

string_t
strmake(string)
    char *string;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(string) + 1));
    if (saved == strNULL)
	fatal("strmake('%s'): %s", string, unix_error_string(errno));
    return strcpy(saved, string);
}

string_t
strconcat(left, right)
    string_t left, right;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(left) + strlen(right) + 1));
    if (saved == strNULL)
	fatal("strconcat('%s', '%s'): %s",
	      left, right, unix_error_string(errno));
    return strcat(strcpy(saved, left), right);
}

void
strfree(string)
    string_t string;
{
    free(string);
}

char *
strbool(bool)
    boolean_t bool;
{
    if (bool)
	return "TRUE";
    else
	return "FALSE";
}

char *
strstring(string)
    string_t string;
{
    if (string == strNULL)
	return "NULL";
    else
	return string;
}
