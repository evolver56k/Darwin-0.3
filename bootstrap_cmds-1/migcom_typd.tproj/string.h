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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * Revision 2.4  93/01/14  17:59:06  danner
 * 	Made text on #endif lines into comments.
 * 	[92/12/14            pds]
 * 	Converted file to ANSI C.
 * 	Added constant string and identifier types.
 * 	[92/12/08            pds]
 * 
 * Revision 2.3  91/02/05  17:55:57  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:03  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:49  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:13:56  rpd]
 * 
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
 */

#ifndef	_STRING_H
#define	_STRING_H

#include <strings.h>

typedef char *string_t;
typedef const char *const_string_t;
typedef const_string_t identifier_t;

#define	strNULL		((string_t) 0)

extern string_t strmake(const char *string);
extern string_t strconcat(const_string_t left, const_string_t right);
extern void strfree(string_t string);

#define	streql(a, b)	(strcmp((a), (b)) == 0)

extern const char *strbool(boolean_t bool);
extern const char *strstring(const_string_t string);

#endif	/* _STRING_H */
