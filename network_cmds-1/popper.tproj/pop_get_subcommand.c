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
static char SccsId[] = "@(#)@(#)pop_get_subcommand.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include "popper.h"

/* 
 *  get_subcommand: Extract a POP XTND subcommand from a client input line
 */

static xtnd_table subcommands[] = {
        "xmit",     0,  0,  pop_xmit,
#ifdef NeXT
	"head",	    1,	2,  pop_head,
	"hset",	    0,	MAXPARMCOUNT,  pop_hset,
#endif /* NeXT */
        NULL
};

xtnd_table *pop_get_subcommand(p)
POP     *   p;
{
    xtnd_table      *   s;

    /*  Search for the POP command in the command/state table */
    for (s = subcommands; s->subcommand; s++) {

        if (strcmp(s->subcommand,p->pop_subcommand) == 0) {

            /*  Were too few parameters passed to the subcommand? */
            if ((p->parm_count-1) < s->min_parms)
                return((xtnd_table *)pop_msg(p,POP_FAILURE,
                    "Too few arguments for the %s %s command.",
                        p->pop_command,p->pop_subcommand));

            /*  Were too many parameters passed to the subcommand? */
            if ((p->parm_count-1) > s->max_parms)
                return((xtnd_table *)pop_msg(p,POP_FAILURE,
                    "Too many arguments for the %s %s command.",
                        p->pop_command,p->pop_subcommand));

            /*  Return a pointer to the entry for this subcommand 
                in the XTND command table */
            return (s);
        }
    }
    /*  The client subcommand was not located in the XTND command table */
    return((xtnd_table *)pop_msg(p,POP_FAILURE,
        "Unknown command: \"%s %s\".",p->pop_command,p->pop_subcommand));
}
