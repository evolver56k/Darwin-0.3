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
 * Revision 2.7  93/05/10  17:50:22  rvb
 * 	Fix include to use < vs " for new ode shadowing
 * 	[93/05/10  10:30:16  rvb]
 * 
 * Revision 2.6  93/01/14  17:59:47  danner
 * 	Made text on #endif lines into comments.
 * 	[92/12/14            pds]
 * 	Converted file to ANSI C.
 * 	[92/12/08            pds]
 * 
 * Revision 2.5  91/08/28  11:17:39  jsb
 * 	Removed TrapRoutine support.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.4  91/06/25  10:32:55  rpd
 * 	Changed WriteHeader to WriteUserHeader.
 * 	Added WriteServerHeader.
 * 	[91/05/23            rpd]
 * 
 * Revision 2.3  91/02/05  17:56:38  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:55  mrt]
 * 
 * Revision 2.2  90/06/02  15:06:20  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:15:16  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_WRITE_H
#define	_WRITE_H

#include <stdio.h>
#include <statement.h>

extern void WriteUserHeader(FILE *file, const statement_t *stats);
extern void WriteServerHeader(FILE *file, const statement_t *stats);
extern void WriteInternalHeader(FILE *file, const statement_t *stats);
extern void WriteUser(FILE *file, const statement_t *stats);
extern void WriteUserIndividual(const statement_t *stats);
extern void WriteServer(FILE *file, const statement_t *stats);

#endif	/* _WRITE_H */
