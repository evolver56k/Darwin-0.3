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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/* 
 **********************************************************************
 * HISTORY
 * 25-Apr-88  Karl Hauth (hauth) at Carnegie-Mellon University
 *	Created.
 *
 *
 **********************************************************************
 */ 

#ifndef	_MACH_EXCEPTION_
#define	_MACH_EXCEPTION_	1

#import <mach/kern_return.h>

char		*mach_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
	int	exception
				);


void		mach_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
	const char	*str,
	int		exception
				);


char		*mach_NeXT_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
	int	exception,
	int	code,
	int	subcode
				);


void		mach_NeXT_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
	const char	*str,
	int	exception,
	int		code,
	int			subcode
				);

#endif	/* _MACH_EXCEPTION_ */
