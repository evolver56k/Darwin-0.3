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
 * Copyright (c) 1992,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: mig_strncpy.c,v $
 * Revision 1.1.1.1  1999/04/14 23:19:15  wsanchez
 * Import of Libc-78-8
 *
 * Revision 1.1.1.1.38.3  1999/03/16 15:47:14  wsanchez
 * Substitute License
 *
 * Revision 2.7  93/05/10  21:33:31  rvb
 * 	No size_t's.
 * 	[93/05/06  09:21:41  af]
 * 
 * Revision 2.6  93/05/10  17:51:00  rvb
 * 	Avoid size_t
 * 	[93/05/04  18:00:57  rvb]
 * 
 * Revision 2.5  93/01/14  18:03:46  danner
 * 	Fixed include of mig_support.h.
 * 	[92/12/14            pds]
 * 	Converted file to ANSI C.
 * 	[92/12/11            pds]
 * 
 * Revision 2.4  91/07/31  18:29:31  dbg
 * 	Changed to return the length of the string, including
 * 	the trailing 0.
 * 	[91/07/25            dbg]
 * 
 * Revision 2.3  91/05/14  17:53:46  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/02/14  14:18:02  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:44:34  mrt]
 * 
 * Revision 2.1  89/08/03  17:06:47  rwd
 * Created.
 * 
 * Revision 2.1  89/05/09  22:06:06  mrt
 * Created.
 * 
 */
/*
 * mig_strncpy.c - by Joshua Block
 *
 * mig_strncpy -- Bounded string copy.  Does what the library routine strncpy
 * OUGHT to do:  Copies the (null terminated) string in src into dest, a 
 * buffer of length len.  Assures that the copy is still null terminated
 * and doesn't overflow the buffer, truncating the copy if necessary.
 *
 * Parameters:
 * 
 *     dest - Pointer to destination buffer.
 * 
 *     src - Pointer to source string.
 * 
 *     len - Length of destination buffer.
 *
 * Result:
 *	length of string copied, INCLUDING the trailing 0.
 */

#include <mach/mig_support.h>

vm_size_t mig_strncpy(register char *dest, register const char *src,
		   register vm_size_t len)
{
    register vm_size_t i;

    if (len == 0)
	return 0;

    for (i=1; i<len; i++)
	if (! (*dest++ = *src++))
	    return i;

    *dest = '\0';
    return i;
}
