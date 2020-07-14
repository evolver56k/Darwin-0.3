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
 **********************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *  
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta, 
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub, 
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young. 
 * 
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 * 
 * Permission to use the CMU portion of this software for 
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 **********************************************************************
 * HISTORY
 * 30-Oct-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */ 

#ifndef	_MACH_ERROR_
#define	_MACH_ERROR_	1

#import <mach/kern_return.h>

#ifdef	__STRICT_BSD__
/*
 *	mach_error_string() returns a string appropriate to the 
 * 	error argument given
 */
extern char *mach_error_string();
/*
 *	mach_erro()r prints an appropriate message on the standard error stream
 */
extern void mach_error();
/*
 *	mach_errormsg() returns a string appropriate to the 
 *	error argument given
 */
extern char *mach_errormsg();
#else
extern char *mach_error_string(kern_return_t error_value);
extern void mach_error(const char *str, kern_return_t error_value);
extern char *mach_errormsg(kern_return_t error_value);
#endif	/* __STRICT_BSD__ */
#endif	/* _MACH_ERROR_ */
