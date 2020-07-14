/*
 *
 * $Id: ibmpc.c,v 1.1.1.1 1999/04/15 17:45:12 wsanchez Exp $
 *
 * Program:	IBM PC specific routine
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

/*
 * The routines in this file provide support for the IBM-PC and other
 * compatible terminals. It goes directly to the graphics RAM to do
 * screen output. It compiles into nothing if not an IBM-PC driver
 */

#include        <stdio.h>
#include	<conio.h>
#include	<time.h>
#include	"osdep.h"
#if     IBMPC
#define	termdef	1			/* don't define "term" external */
#include	"pico.h"
#include	"estruct.h"
#include        "efunc.h"
#include        "edef.h"

#ifdef	ANSI
    int  ibmmove(int, int);
    int  ibmeeol(void);
    int  ibmputc(int);
    int  ibmoutc(char);
    int  ibmeeop(void);
    int  ibmrev(int);
    void beep(unsigned int, unsigned int);
    int  cutebeep(void);
    int  ibmbeep(void);
    int  ibmopen(void);
    int  ibmclose(void);
#if	COLOR
    int	 ibmfcol(int);
    int	 ibmbcol(int);
#endif	/* COLOR */
#else
    int  ibmmove();
    int  ibmeeol();
    int  ibmputc();
    int  ibmoutc();
    int  ibmeeop();
    int  ibmrev();
    void beep();
    int  cutebeep();
    int  ibmbeep();
    int  ibmopen();
    int  ibmclose();
#if	COLOR
    int	 ibmfcol();
    int	 ibmbcol();
#endif	/* COLOR */
#endif


#define NROW    25              /* Screen size.                 */
#define NCOL    80              /* Edit if you want to.         */
#define	MARGIN	8		/* size of minimim margin and	*/
#define	SCRSIZ	64		/* scroll size for extended lines */
#define	NPAUSE	200		/* # times thru update to pause */
#define BEL     0x07            /* BEL character.               */
#define ESC     0x1B            /* ESC character.               */
#define	SPACE	32		/* space character		*/


int *scptr[NROW];		/* pointer to screen lines	*/
int sline[NCOL];		/* screen line image		*/

unsigned	cattr	= 0x07;	/* gray by default */

static unsigned display_mode;	/*  */

#if	COLOR
int	cfcolor = -1;		/* current forground color */
int	cbcolor = -1;		/* current background color */
int	ctrans[] =		/* ansi to ibm color translation table */
	{0, 4, 2, 6, 1, 5, 3, 7};
#endif

/*
 * Standard terminal interface dispatch table. Most of the fields point into
 * "termio" code.
 */
TERM    term    = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
        ibmopen,
        ibmclose,
        ttgetc,
	ibmputc,
        ttflush,
        ibmmove,
        ibmeeol,
        ibmeeop,
        ibmbeep,
	ibmrev
#if	COLOR
	, ibmfcol,
	ibmbcol
#endif
};


extern union REGS rg;


#if	COLOR
ibmfcol(color)		/* set the current output color */
int color;	/* color to set */
{
    cfcolor = ctrans[color];
}


ibmbcol(color)		/* set the current background color */
int color;	/* color to set */
{
    cbcolor = ctrans[color];
}
#endif	/* COLOR */


/*
 * ibmmove - Use BIOS video services, function 2h to set cursor postion
 */
ibmmove(row, col)
int row, col;
{
    rg.h.ah = 2;		/* set cursor position function code */
    rg.h.bh = 0;		/* set screen page number */
    rg.h.dl = col;
    rg.h.dh = row;
    int86(BIOS_VIDEO, &rg, &rg);
}


/*
 * ibmeeol - erase to the end of the line
 */
ibmeeol()
{
    int col, row, page;

    /* find the current cursor position */
    rg.h.ah = 3;		/* read cursor position function code */
    int86(BIOS_VIDEO, &rg, &rg);
    page = rg.h.bh;
    col = rg.h.dl;		/* record current column */
    row = rg.h.dh;		/* and row */

    rg.h.ah = 0x09;		/* write char to screen with new attrs */
    rg.h.al = ' ';
    rg.h.bl = cattr;
    rg.h.bh = page;
    rg.x.cx = NCOL-col;
    int86(BIOS_VIDEO, &rg, &rg);
}


/*
 * ibmputc - put a character at the current position in the
 *	     current colors
 */
ibmputc(ch)
int ch;
{
    int col, row, page;

    rg.h.ah = 0x03;			/* first, get current position */
    int86(BIOS_VIDEO, &rg, &rg);
    page = rg.h.bh;
    row = rg.h.dh;
    col = rg.h.dl;
    
    if(ch == '\b'){
	if(col > 0)		/* advance the cursor */
	  ibmmove(row, --col);
    }
    else{
	rg.h.ah = 0x09;		/* write char to screen with new attrs */
	rg.h.al = ch;
	rg.h.bl = cattr;		/* inverting if needed */
	rg.h.bh = page;
	rg.x.cx = 1;		/* only once */
	int86(BIOS_VIDEO, &rg, &rg);

	if(col < 80)		/* advance the cursor */
	  ibmmove(row, ++col);
    }
}


/* 
 * ibmoutc - output a single character with the right attributes, but
 *           don't advance the cursor
 */
ibmoutc(c)
char c;
{
    rg.h.ah = 0x09;		/* write char to screen with new attrs */
    rg.h.al = c;
    rg.h.bl = cattr;	/* inverting if needed */
    rg.h.bh = 0;
    rg.x.cx = 1;		/* only once */
    int86(BIOS_VIDEO, &rg, &rg);
}


/*
 * ibmeeop - clear from cursor to end of page
 */
ibmeeop()
{
    int attr;			/* attribute to fill screen with */

    rg.h.ah = 6;		/* scroll page up function code */
    rg.h.al = 0;		/* # lines to scroll (clear it) */
    rg.x.cx = 0;		/* upper left corner of scroll */
    rg.x.dx = (term.t_nrow << 8) | (term.t_ncol - 1);
    attr    = cattr;
    rg.h.bh = attr;
    int86(BIOS_VIDEO, &rg, &rg);

    ibmmove(0, 0);
}


/*
 * ibmrev - change reverse video state
 */
ibmrev(state)
int state;
{
    cattr = (state) ? 0x70 : 0x07;
}


/*
 * getrevstate - return the current reverse state
 */
getrevstate()
{
    return(cattr == 0x70);
}


/* 
 * beep - make the speaker sing!
 */
void
beep(freq, dur)
unsigned freq, dur;
{
    unsigned oport;

    if(!freq)
	return;

    freq = (unsigned)(1193180 / freq);
    /* set up the timer */
    outp(0x43, 0xb6);			/* set timer channel 2 registers */
    outp(0x42, (0xff&freq));		/* low order byte of count */
    outp(0x42, (freq>>8));		/* hi order byte of count */

    /* make the sound */
    oport = inp(0x61);
    outp(0x61, oport | 0x03);
    ssleep((clock_t)((dur < 75) ? 75 : dur));
    outp(0x61, oport);
}


/*
 * cutebeep - make the speeker sing the way we want!
 */
cutebeep()
{
    beep(575, 50);
    ssleep((clock_t)25);
    beep(485, 90);
}


/*
 * ibmbeep - system beep...
 */
ibmbeep()
{
    cutebeep();
}


/*
 * enter_text_mode - get current video mode, saving to be restored 
 *                   later, then explicitly set 80 col text mode.
 *
 *     NOTE: this gets kind of weird.  Both pine and pico call this
 *           during initialization.  To make sure it's only invoked once
 *           it only responds if passed NULL which pico only does if not
 *           called from in pine, and pine does all the time.  make sense?
 *           thought not.
 */
void
enter_text_mode(p)
PICO *p;
{
    static int i = 0;

    if(!p && !i++){
	rg.h.ah = 0x0f;			/* save old mode */
	int86(BIOS_VIDEO, &rg, &rg);
	display_mode = rg.h.al;

	rg.h.ah = 0;			/* then set text mode */
	rg.h.al = 2;
	int86(BIOS_VIDEO, &rg, &rg);		/* video services */
    }
}


/*
 * exit_text_mode - leave text mode by restoring saved original 
 *                  video mode.
 */
void
exit_text_mode(p)
PICO *p;
{
    static int i = 0;

    if(!p && !i++){			/* called many, invoke once! */
	rg.h.ah = 0;			/* just restore old mode */
	rg.h.al = display_mode;
	int86(BIOS_VIDEO, &rg, &rg);
    }
}


/*
 * ibmopen - setup text mode and setup key labels...
 */
ibmopen()
{

    enter_text_mode(Pmaster);

    revexist = TRUE;
    inschar = delchar = 0;
    HelpKeyNames = (gmode&MDFKEY) ? funckeynames : NULL;

    ttopen();
}


ibmclose()
{
#if	COLOR
    ibmfcol(7);
    ibmbcol(0);
#endif

    exit_text_mode(Pmaster);

    ttclose();
}
#endif
