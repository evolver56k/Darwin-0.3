/*
 * Copyright 1995-1999 Apple Computer, Inc. All rights reserved.
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
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christos Zoulas of Cornell University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)el.h	8.1 (Berkeley) 6/4/93
 */

/*
 * el.h: Internal structures.
 */
#ifndef _h_el
#define _h_el
/*
 * Local defaults
 */
#define KSHVI
#define VIDEFAULT
#define ANCHOR

#include <stdio.h>
#include <sys/types.h>

#define EL_BUFSIZ	1024		/* Maximum line size		*/

#define HANDLE_SIGNALS	1

typedef int bool_t;			/* True or not			*/

typedef unsigned char el_action_t;	/* Index to command array	*/

typedef struct coord_t {		/* Position on the screen	*/
    int h, v;
} coord_t;

typedef struct el_line_t {
    char *buffer, 			/* Input line 			*/
	 *cursor, 			/* Cursor position 		*/
	 *lastchar,			/* Last character 		*/
	 *limit;			/* Max position			*/
} el_line_t;

/*
 * Editor state
 */
typedef struct el_state_t {
    int 	inputmode;		/* What mode are we in? 	*/
    int 	doingarg;		/* Are we getting an argument?	*/
    int	        argument;		/* Numeric argument 		*/
    int		metanext;		/* Is the next char a meta char */
    el_action_t lastcmd;		/* Previous command		*/
} el_state_t;

/*
 * Until we come up with something better...
 */
#define el_malloc(a)	malloc(a)
#define el_realloc(a,b)	realloc(a, b)
#define el_free(a)	free(a)

#include "tty.h"
#include "prompt.h"
#include "key.h"
#include "term.h"
#include "refresh.h"
#include "chared.h"
#include "common.h"
#include "search.h"
#include "hist.h"
#include "map.h"
#include "parse.h"
#include "sig.h"
#include "help.h"

struct editline {
    char	 *el_prog;	/* the program name 			*/
    FILE         *el_outfile;	/* Stdio stuff				*/
    FILE         *el_errfile;	/* Stdio stuff				*/
    int           el_infd;	/* Input file descriptor		*/
    int		  el_flags;	/* Various flags.			*/
    coord_t       el_cursor;	/* Cursor location			*/
    char        **el_display, 	/* Real screen image = what is there	*/
	        **el_vdisplay;	/* Virtual screen image = what we see	*/

    el_line_t     el_line;	/* The current line information		*/
    el_state_t	  el_state;	/* Current editor state			*/
    el_term_t     el_term;	/* Terminal dependent stuff		*/
    el_tty_t	  el_tty;	/* Tty dependent stuff			*/
    el_refresh_t  el_refresh;	/* Refresh stuff			*/
    el_prompt_t   el_prompt;	/* Prompt stuff				*/
    el_chared_t	  el_chared;	/* Characted editor stuff		*/
    el_map_t	  el_map;	/* Key mapping stuff			*/
    el_key_t	  el_key;	/* Key binding stuff			*/
    el_history_t  el_history;	/* History stuff			*/
    el_search_t	  el_search;	/* Search stuff				*/
    el_signal_t	  el_signal;	/* Signal handling stuff		*/
};

#endif /* _h_el */
