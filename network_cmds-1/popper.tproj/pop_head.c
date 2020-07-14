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
static char SccsId[] = "@(#)@(#)pop_head.c	2.1  2.1 10/31/94";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include "popper.h"

#ifdef NeXT
#define strdup		(char *) NXCopyStringBuffer
#endif /* NeXT */

int
pop_good_header(p, line)
POP		*   p;
const char	*   line;
{
    const char	    **   hs;

    for (hs = p->header_set; *hs != NULL; hs++) {
	int head_len = strlen(*hs);
	if (strncasecmp(*hs, line, head_len) == 0 && line[head_len] == ':')
	    return 1;
    }

    return 0;
}

/* 
 *  head:   List selected headers for the specified range of messages
 */

pop_head(p)
POP     *   p;
{
    int			    msg_num_first;
    int			    msg_num_last;
    int			    mi;
    char                    buffer[MAXMSGLINELEN];

    /* The first parameter [1] is "HEAD", the other min and max msg num */

    /*  Convert the first parameter into an integer */
    msg_num_first = atoi(p->pop_parm[2]);

    /*  Convert the first parameter into an integer */
    msg_num_last = (p->parm_count > 2) ? atoi(p->pop_parm[3]) : msg_num_first;

    /*  Are the parameters reasonable? */
    if (msg_num_first < 1 || msg_num_first > msg_num_last ||
	msg_num_last > p->msg_count) 
        return pop_msg(p,POP_FAILURE,"Message interval %d..%d is out of range"
		       " (should be within 1..%d)",
		       msg_num_first, msg_num_last, p->msg_count);

    /*  Go through each message in the interval */
    for (mi = msg_num_first; mi <= msg_num_last; mi++) {
	MsgInfoList         *   mp = &p->mlp[mi-1];
	int			last_was_good = 1;

	/* Indicate which message we're at */
	pop_msg(p, POP_SUCCESS, "%d %d octets", mi, mp->length);
	
	/*  Position to the start of the message */
	(void)fseek(p->drop,mp->offset,0);

	/*  Skip the first line (the headmail "From" line) */
	(void)fgets (buffer,MAXMSGLINELEN,p->drop);

	/*  Send the headers */
	while (fgets(buffer,MAXMSGLINELEN,p->drop) && buffer[0] != '\n') {
	    if (p->header_set == NULL ||
		(isspace(buffer[0]) && last_was_good) ||
		pop_good_header(p, buffer)) {
		pop_sendline(p,buffer);
		last_was_good = 1;
	    } else {
		last_was_good = 0;
	    }
	}
    }

    /*  "." signals the end of a multi-line transmission */
    (void)fputs(".\r\n",p->output);
    (void)fflush(p->output);

    return(POP_SUCCESS);
}

/* 
 *  hset:   Select headers to send in future HEAD commands
 */

pop_hset(p)
POP     *   p;
{
    char **hs;

    /* Get rid of any old header set */
    if (p->header_set) {
	for (hs = p->header_set; *hs != NULL; hs++)
	    free(hs);
	free(p->header_set);
	p->header_set = NULL;
    }

    /* The first parameter [1] is "HSET", the rest headers to be remembered */
    if (p->parm_count > 1) {
	int i;

	hs = p->header_set =
	    (char **) malloc((p->parm_count - 1 + 1) * sizeof(char *));

	for (i = 2; i <= p->parm_count; i++) {
	    *hs++ = strdup(p->pop_parm[i]);
	}
	*hs = NULL;
	return pop_msg(p, POP_SUCCESS,
		       "XTND HEAD command will report %d headers",
		       p->parm_count - 1);
    } else {
	return pop_msg(p, POP_SUCCESS,
		       "XTND HEAD command will report all headers");
    }
}
