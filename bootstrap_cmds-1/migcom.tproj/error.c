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

#include <stdio.h>
#include <varargs.h>
#include "global.h"
#include "error.h"

extern int yylineno;
extern char *yyinname;

static char *program;
int errors = 0;

/*ARGSUSED*/
/*VARARGS1*/
void
fatal(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    fprintf(stderr, "%s: fatal: ", program);
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    exit(1);
}

/*ARGSUSED*/
/*VARARGS1*/
__private_extern__
void
warn(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    if (!BeQuiet && (errors == 0))
    {
	fprintf(stderr, "\"%s\", line %d: warning: ", yyinname, yylineno-1);
	(void) vfprintf(stderr, format, pvar);
	fprintf(stderr, "\n");
    }
    va_end(pvar);
}

/*ARGSUSED*/
/*VARARGS1*/
void
error(format, va_alist)
    char *format;
    va_dcl
{
    va_list pvar;
    va_start(pvar);
    fprintf(stderr, "\"%s\", line %d: ", yyinname, yylineno-1);
    (void) vfprintf(stderr, format, pvar);
    fprintf(stderr, "\n");
    va_end(pvar);
    errors++;
}

char *
unix_error_string(errno)
    int errno;
{
    extern const char * const sys_errlist[]; 
    extern int sys_nerr;
    static char buffer[256];
    char *error_mess;

    if ((0 <= errno) && (errno < sys_nerr))
	error_mess = sys_errlist[errno];
    else
	error_mess = "strange errno";

    sprintf(buffer, "%s (%d)", error_mess, errno);
    return buffer;
}

void
set_program_name(name)
    char *name;
{
    program = name;
}
