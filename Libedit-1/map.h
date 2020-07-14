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
 *	@(#)map.h	8.1 (Berkeley) 6/4/93
 */

/*
 * el.map.h:	Editor maps
 */
#ifndef _h_el_map
#define _h_el_map

typedef struct el_bindings_t {	/* for the "bind" shell command */
    const char   *name;		/* function name for bind command */
    int     func;		/* function numeric value */
    const char   *description;	/* description of function */
} el_bindings_t;


typedef struct el_map_t {
    el_action_t   *alt;		/* The current alternate key map	*/
    el_action_t   *key;		/* The current normal key map		*/
    el_action_t   *current;	/* The keymap we are using		*/
    el_action_t   *emacs;	/* The default emacs key map		*/
    el_action_t   *vic;		/* The vi command mode key map		*/
    el_action_t   *vii;		/* The vi insert mode key map		*/
    int		   type;	/* Emacs or vi				*/
    el_bindings_t *help;	/* The help for the editor functions	*/
    el_func_t     *func;	/* List of available functions		*/
    int  	   nfunc;	/* The number of functions/help items	*/
} el_map_t;

#define MAP_EMACS	0
#define MAP_VI		1

protected int	map_bind		__P((EditLine *, int, char **));
protected int	map_init		__P((EditLine *));
protected void	map_end			__P((EditLine *));
protected void	map_init_vi		__P((EditLine *));
protected void	map_init_emacs		__P((EditLine *));
protected int	map_set_editor		__P((EditLine *, char *));
protected int	map_addfunc		__P((EditLine *, const char *, 
					     const char *, el_func_t));

#endif /* _h_el_map */
