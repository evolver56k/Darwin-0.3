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
 * Revision 1.1.1.1  1997/10/14 18:36:30  wsanchez
 * Import of bootstrap_cmds-5
 *
 * Revision 2.5  93/05/10  17:49:01  rvb
 * 	Fix include to use < vs " for new ode shadowing
 * 	[93/05/10  10:27:23  rvb]
 * 
 * 	386BSD lex does not do line numbers ... yet
 * 	[93/05/05  09:24:01  rvb]
 * 
 * Revision 2.4  93/01/14  17:57:48  danner
 * 	Converted file to ANSI C.
 * 	[92/12/08            pds]
 * 
 * Revision 2.3  91/02/05  17:54:11  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:53:53  mrt]
 * 
 * Revision 2.2  90/06/02  15:04:25  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:10:17  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <stdio.h>
#include <stdarg.h>
#include <global.h>
#include <error.h>

#ifndef	__386BSD__
extern int yylineno;
extern char *yyinname;
#endif

static const char *program;
int errors = 0;

extern int vfprintf(FILE *file, const char *fmt, va_list);

volatile void
fatal(const char *format, ...)
{
    va_list pvar;
    va_start(pvar, format);
    fprintf(stderr, "%s: fatal: ", program);
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    exit(1);
}

__private_extern__
void
warn(const char *format, ...)
{
    va_list pvar;
    va_start(pvar, format);
    if (!BeQuiet && (errors == 0))
    {
#ifndef	__386BSD__
	fprintf(stderr, "\"%s\", line %d: warning: ", yyinname, yylineno-1);
#endif
	(void) vfprintf(stderr, format, pvar);
	fprintf(stderr, "\n");
    }
    va_end(pvar);
}

void
error(const char *format, ...)
{
    va_list pvar;
    va_start(pvar, format);
#ifndef	__386BSD__
    fprintf(stderr, "\"%s\", line %d: ", yyinname, yylineno-1);
#endif
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    errors++;
}

const char *
unix_error_string(int error_num)
{
    static char buffer[256];
    const char *error_mess;

    if ((0 <= error_num) && (error_num < sys_nerr))
	error_mess = sys_errlist[error_num];
    else
	error_mess = "strange errno";

    sprintf(buffer, "%s (%d)", error_mess, error_num);
    return buffer;
}

void
set_program_name(const char *name)
{
    program = name;
}
