#if	!defined(lint) && !defined(DOS)
static char rcsid_osh[] = "$Id: os_bsd.h,v 1.1.1.1 1999/04/15 17:45:13 wsanchez Exp $";
#endif
/*
 * Program:	Operating system dependent routines - 4.3 bsd
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 * Copyright 1991-1993  University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee to the University of
 * Washington is hereby granted, provided that the above copyright notice
 * appears in all copies and that both the above copyright notice and this
 * permission notice appear in supporting documentation, and that the name
 * of the University of Washington not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Pine and Pico are trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior
 * written permission of the University of Washington.
 *
 */

#ifndef	OSDEP_H
#define	OSDEP_H

#ifdef	dyn
#include	<strings.h>
#else
#include	<string.h>
#endif
#undef	CTRL
#include	<signal.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/ioctl.h>		/* to get at the typeahead */
#include	<sys/stat.h>

/* Machine/OS definition			*/
#define V7      1                       /* V7 UN*X or Coherent or BSD4.2*/
#define JOB_CONTROL	1               /* OS has job control */

#ifdef	dyn
#define	strchr	index			/* Dynix doesn't know about strchr */
#define	strrchr	rindex
#endif	/* dyn */

extern struct KBSTREE *kpadseqs;
extern int kbseq();

#ifdef	termdef
#if	ANSI
#define NROW    25                      /* Screen size.                 */
#define NCOL    80                      /* Edit if you want to.         */
#endif
#else
#if	TERMCAP
extern struct KBSTREE *kpadseqs;
#endif	/* TERMCAP */
#endif

#ifdef	maindef
/*	possible names and paths of help files under different OSs	*/

char *pathname[] = {
	".picorc",
	"pico.hlp",
	"/usr/local/",
	"/usr/lib/",
	""
};

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))

jmp_buf	got_hup;		/* stack environment to handle SIGHUP */
#endif

extern int errno;


#endif	/* OSDEP_H */
