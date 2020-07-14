/*
 * $Id: os_dos.h,v 1.1.1.1 1999/04/15 17:45:13 wsanchez Exp $
 *
 * Program:	Operating system dependent routines - Ultrix 4.1
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

#include	<stdlib.h>
#include	<string.h>
#include	<dos.h>
#include	<direct.h>
#include	<search.h>
#undef	CTRL
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>

/* define machine specifics */
#define	IBMPC	1

#ifdef	PCTCP
#define	sleep	_sleep
#endif

/*
 *  IBM PC ROM BIOS Services used
 */
#define	BIOS_VIDEO	0x10
#define	BIOS_KEYBRD	0x16
#define	BIOS_MOUSE	0x33


/*
 * type qsort expects
 */
#define	QSType	void
#define QcompType const void


/*
 * File name separators, char and string
 */
#define	C_FILESEP	'\\'
#define	S_FILESEP	"\\"


/*
 * What and where the tool that checks spelling is located.  If this is
 * undefined, then the spelling checker is not compiled into pico.
 */
#undef	SPELLER


#ifdef	maindef
/*	possible names and paths of help files under different OSs	*/

char *pathname[] = {
	"picorc",
	"pico.hlp",
	"\\usr\\local\\",
	"\\usr\\lib\\",
	""
};

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))

jmp_buf got_hup;

extern struct KBSTREE *kpadseqs = NULL;

#endif
#endif	/* !OSDEP_H */
