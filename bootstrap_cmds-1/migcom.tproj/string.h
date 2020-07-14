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
/**************************************************************
 * ABSTRACT:
 *   Header file for string utility programs (string.c)
 *
 *
 *	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/string.h,v 1.1.1.1 1999/04/15 18:30:37 wsanchez Exp $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed strNULL to be the null string instead of the null string
 *	pointer.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 *****************************************************************/

#ifndef	_MIG_STRING_H
#define	_MIG_STRING_H

#include <strings.h>

typedef char *string_t;
typedef string_t identifier_t;

extern char	charNULL;
#define	strNULL		&charNULL

extern string_t strmake(/* char *string */);
extern string_t strconcat(/* string_t left, right */);
extern void strfree(/* string_t string */);

#define	streql(a, b)	(strcmp((a), (b)) == 0)

extern char *strbool(/* boolean_t bool */);
extern char *strstring(/* string_t string */);

#endif	_MIG_STRING_H
