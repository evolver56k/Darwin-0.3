/*	$NetBSD: xx.c,v 1.4 1997/11/21 08:38:02 lukem Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)xx.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: xx.c,v 1.4 1997/11/21 08:38:02 lukem Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include <string.h>
#define EXTERN
#include "xx.h"
#undef  EXTERN
#include "defs.h"
#include "tt.h"

int
xxinit()
{
	if (ttinit() < 0)
		return -1;
	xxbufsize = tt.tt_nrow * tt.tt_ncol * 2;
	/* ccinit may choose to change xxbufsize */
	if (tt.tt_ntoken > 0 && ccinit() < 0)
		return -1;
	xxbuf = malloc((unsigned) xxbufsize * sizeof *xxbuf);
	if (xxbuf == 0) {
		wwerrno = WWE_NOMEM;
		return -1;
	}
	xxbufp = xxbuf;
	xxbufe = xxbuf + xxbufsize;
	return 0;
}

void
xxstart()
{
	(*tt.tt_start)();
	if (tt.tt_ntoken > 0)
		ccstart();
	xxreset1();			/* might be a restart */
}

void
xxreset()
{
	if (tt.tt_ntoken > 0)
		ccreset();
	xxreset1();
	(*tt.tt_reset)();
}

void
xxreset1()
{
	struct xx *xp, *xq;

	for (xp = xx_head; xp != 0; xp = xq) {
		xq = xp->link;
		xxfree(xp);
	}
	xx_tail = xx_head = 0;
	xxbufp = xxbuf;
}

void
xxend()
{
	if (tt.tt_scroll_top != 0 || tt.tt_scroll_bot != tt.tt_nrow - 1)
		/* tt.tt_setscroll is known to be defined */
		(*tt.tt_setscroll)(0, tt.tt_nrow - 1);
	if (tt.tt_modes)
		(*tt.tt_setmodes)(0);
	if (tt.tt_scroll_down)
		(*tt.tt_scroll_down)(1);
	(*tt.tt_move)(tt.tt_nrow - 1, 0);
	if (tt.tt_ntoken > 0)
		ccend();
	(*tt.tt_end)();
	ttflush();
}

struct xx *
xxalloc()
{
	struct xx *xp;

	if (xxbufp > xxbufe)
		abort();
	if ((xp = xx_freelist) == 0)
		/* XXX can't deal with failure */
		xp = (struct xx *) malloc((unsigned) sizeof *xp);
	else
		xx_freelist = xp->link;
	if (xx_head == 0)
		xx_head = xp;
	else
		xx_tail->link = xp;
	xx_tail = xp;
	xp->link = 0;
	return xp;
}

void
xxfree(xp)
	struct xx *xp;
{
	xp->link = xx_freelist;
	xx_freelist = xp;
}

void
xxmove(row, col)
	int row, col;
{
	struct xx *xp = xx_tail;

	if (xp == 0 || xp->cmd != xc_move) {
		xp = xxalloc();
		xp->cmd = xc_move;
	}
	xp->arg0 = row;
	xp->arg1 = col;
}

void
xxscroll(dir, top, bot)
	int dir, top, bot;
{
	struct xx *xp = xx_tail;

	if (xp != 0 && xp->cmd == xc_scroll &&
	    xp->arg1 == top && xp->arg2 == bot &&
	    ((xp->arg0 < 0 && dir < 0) || (xp->arg0 > 0 && dir > 0))) {
		xp->arg0 += dir;
		return;
	}
	xp = xxalloc();
	xp->cmd = xc_scroll;
	xp->arg0 = dir;
	xp->arg1 = top;
	xp->arg2 = bot;
}

void
xxinschar(row, col, c, m)
	int row, col, c, m;
{
	struct xx *xp;

	xp = xxalloc();
	xp->cmd = xc_inschar;
	xp->arg0 = row;
	xp->arg1 = col;
	xp->arg2 = c;
	xp->arg3 = m;
}

void
xxinsspace(row, col)
	int row, col;
{
	struct xx *xp = xx_tail;

	if (xp != 0 && xp->cmd == xc_insspace && xp->arg0 == row &&
	    col >= xp->arg1 && col <= xp->arg1 + xp->arg2) {
		xp->arg2++;
		return;
	}
	xp = xxalloc();
	xp->cmd = xc_insspace;
	xp->arg0 = row;
	xp->arg1 = col;
	xp->arg2 = 1;
}

void
xxdelchar(row, col)
	int row, col;
{
	struct xx *xp = xx_tail;

	if (xp != 0 && xp->cmd == xc_delchar &&
	    xp->arg0 == row && xp->arg1 == col) {
		xp->arg2++;
		return;
	}
	xp = xxalloc();
	xp->cmd = xc_delchar;
	xp->arg0 = row;
	xp->arg1 = col;
	xp->arg2 = 1;
}

void
xxclear()
{
	struct xx *xp;

	xxreset1();
	xp = xxalloc();
	xp->cmd = xc_clear;
}

void
xxclreos(row, col)
	int row, col;
{
	struct xx *xp = xxalloc();

	xp->cmd = xc_clreos;
	xp->arg0 = row;
	xp->arg1 = col;
}

void
xxclreol(row, col)
	int row, col;
{
	struct xx *xp = xxalloc();

	xp->cmd = xc_clreol;
	xp->arg0 = row;
	xp->arg1 = col;
}

void
xxwrite(row, col, p, n, m)
	int row, col;
	char *p;
	int n, m;
{
	struct xx *xp;

	if (xxbufp + n + 1 > xxbufe)
		xxflush(0);
	xp = xxalloc();
	xp->cmd = xc_write;
	xp->arg0 = row;
	xp->arg1 = col;
	xp->arg2 = n;
	xp->arg3 = m;
	xp->buf = xxbufp;
	memmove(xxbufp, p, n);
	xxbufp += n;
	*xxbufp++ = char_sep;
}
