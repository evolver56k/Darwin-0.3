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
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_parse.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include "popper.h"

/* 
 *  parse:  Parse a raw input line from a POP client 
 *  into null-delimited tokens
 */

pop_parse(p,buf)
POP         *   p;
char        *   buf;        /*  Pointer to a message containing 
                                the line from the client */
{
    char            *   mp;
    register int        i;
    
    /*  Loop through the POP command array */
    for (mp = buf, i = 0; ; i++) {
    
        /*  Skip leading spaces and tabs in the message */
        while (isspace(*mp))mp++;

        /*  Are we at the end of the message? */
        if (*mp == 0) break;

        /*  Have we already obtained the maximum allowable parameters? */
        if (i >= MAXPARMCOUNT) {
            pop_msg(p,POP_FAILURE,"Too many arguments supplied.");
            return(-1);
        }

        /*  Point to the start of the token */
        p->pop_parm[i] = mp;

        /*  Search for the first space character (end of the token) */
        while (!isspace(*mp) && *mp) mp++;

        /*  Delimit the token with a null */
        if (*mp) *mp++ = 0;
    }

    /*  Were any parameters passed at all? */
    if (i == 0) return (-1);

    /*  Convert the first token (POP command) to lower case */
    pop_lower(p->pop_command);

    /*  Return the number of tokens extracted minus the command itself */
    return (i-1);
    
}
