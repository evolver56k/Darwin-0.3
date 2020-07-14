/*
 * $Id: os_dos.c,v 1.1.1.1 1999/04/15 17:45:13 wsanchez Exp $
 *
 * Program:	Operating system dependent routines - MS DOS
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
 *
 * Notes:
 *      - mouse support added (mss, 921215)
 *
 *  Portions of this code derived from MicroEMACS 3.10:
 *
 *	MSDOS.C:	Operating specific I/O and Spawning functions
 *			under the MS/PCDOS operating system
 *			for MicroEMACS 3.10
 *			(C)opyright 1988 by Daniel M. Lawrence
 *
 */

#include 	<stdio.h>
#include	<errno.h>
#include	<signal.h>
#include	<setjmp.h>
#include	<time.h>
#include	<fcntl.h>
#include	<io.h>
#include	<bios.h>

#include	"osdep.h"
#include	"estruct.h"
#include        "edef.h"
#include        "pico.h"


#ifdef	MOUSE
typedef struct point {
    unsigned	r:8;		/* row value				*/
    unsigned	c:8;		/* column value				*/
} POINT;


typedef struct menuitem {
    unsigned	val;		/* return value				*/
    unsigned long (*action)();	/* action to perform			*/
    POINT	tl;		/* top-left corner of active area	*/
    POINT	br;		/* bottom-right corner of active area	*/
    POINT	lbl;		/* where the label starts		*/
    char	*label;
} MENUITEM;
MENUITEM menuitems[12];		/* key labels and functions */
MENUITEM mfunc;			/* single generic function  */

#define	M_ACTIVE(R, C, X)	(((R) >= (X)->tl.r && (R) <= (X)->br.r) \
				 && ((C) >= (X)->tl.c && (C) < (X)->br.c))
#endif	/* MOUSE */


#ifdef	ANSI
#ifdef	MOUSE
    int      checkmouse(unsigned *);
    int	     register_keys(char *, char *, unsigned);
    void     invert_label(int, MENUITEM *);
#endif
    int enhanced_keybrd(void);
    int dont_interrupt(void);
    int interrupt_ok(void);
    int kbseq(int *);
    int specialkey(unsigned int);
    void do_alarm_signal(void);
    void do_hup_signal(void);
    char *pfnexpand(char *, int);
    int ssleep(long);
    int sleep(int);
#else
#ifdef	MOUSE
    int      checkmouse();
#endif
    int enhanced_keybrd();
    int dont_interrupt();
    int interrupt_ok();
    int kbseq();
    int specialkey();
    void do_alarm_signal();
    void do_hup_signal();
    char *pfnexpand();
    int ssleep();
    int sleep();
#endif


#ifdef TURBOC
/*
 * big stack for turbo C
 */
extern	unsigned	_stklen = 16384;
#endif


/*
 * Useful global def's
 */
char   ptmpfile[128];		/* popen temp file */
static int enhncd = 0;		/* keyboard of enhanced variety? */
union  REGS   rg;
struct SREGS  segreg;

static int mexist = 0;		/* is the mouse driver installed? */
static int nbuttons;		/* number of buttons on the mouse */
static int oldbut;		/* Previous state of mouse buttons */
static unsigned short oldbreak;	/* Original state of break key */
static unsigned mnoop;


/*
 * DISable ctrl-break interruption
 */
dont_interrupt()
{
    /* get original value, to be restored later... */
    rg.h.ah = 0x33;		/* control-break check dos call */
    rg.h.al = 0;		/* get the current state */
    rg.h.dl = 0;		/* pre-set it OFF */
    intdos(&rg, &rg);		/* go for it! */
    oldbreak = rg.h.dl;
    /* kill the ctrl-break interupt */
    rg.h.ah = 0x33;		/* control-break check dos call */
    rg.h.al = 1;		/* set the current state */
    rg.h.dl = 0;		/* set it OFF */
    intdos(&rg, &rg);		/* go for it! */
}


/*
 * re-enable ctrl-break interruption
 */
interrupt_ok()
{
    /* restore the ctrl-break interupt */
    rg.h.ah = 0x33;		/* control-break check dos call */
    rg.h.al = 1;		/* set to new state */
    rg.h.dl = oldbreak;		/* set it to its original value */
    intdos(&rg, &rg);	/* go for it! */
}


/*
 * return true if an enhanced keyboard is present
 */
enhanced_keybrd()
{
    /* and check for extended keyboard */
    rg.h.ah = 0x05;
    rg.x.cx = 0xffff;
    int86(BIOS_KEYBRD, &rg, &rg);
    rg.h.ah = 0x10;
    int86(BIOS_KEYBRD, &rg, &rg);
    return(rg.x.ax == 0xffff);
}


/*
 * This function is called once to set up the terminal device streams.
 */
ttopen()
{
    dont_interrupt();			/* don't allow interrupt */
    enhncd = enhanced_keybrd();		/* check for extra keys */
#if	MOUSE
    init_mouse();
#else	/* !MOUSE */
    mexist = 0;
#endif	/* MOUSE */
    return(1);
}

#ifdef	MOUSE
/* 
 * init_mouse - check for and initialize mouse driver...
 */
init_mouse()
{
    long miaddr;		/* mouse interupt routine address */

    if(mexist)
      return(TRUE);

    /* check if the mouse drive exists first */
    rg.x.ax = 0x3533;		/* look at the interrupt 33 address */
    intdosx(&rg, &rg, &segreg);
    miaddr = (((long)segreg.es) << 16) + (long)rg.x.bx;
    if (miaddr == 0 || *(char *)miaddr == 0xcf) {
	mexist = FALSE;
	return(TRUE);
    }

    /* and then check for the mouse itself */
    rg.x.ax = 0;			/* mouse status flag */
    int86(BIOS_MOUSE, &rg, &rg);	/* check for the mouse interupt */
    mexist = (rg.x.ax != 0);
    nbuttons = rg.x.bx;

    if (mexist == FALSE)
	return(TRUE);

    /* if the mouse exists.. get it in the upper right corner */
    rg.x.ax = 4;			/* set mouse cursor position */
    rg.x.cx = (term.t_ncol/2) << 3;	/* Center of display... */
    rg.x.dx = 1 << 3;			/* Second line down */
    int86(BIOS_MOUSE, &rg, &rg);

    /* and set its attributes */
    rg.x.ax = 10;		/* set text cursor */
    rg.x.bx = 0;		/* software text cursor please */
    rg.x.cx = 0x77ff;	/* screen mask */
    rg.x.dx = 0x7700;	/* cursor mask */
    int86(BIOS_MOUSE, &rg, &rg);
    return(TRUE);
}
#endif


/*
 * This function gets called just before we go back home to the command
 * interpreter.
 */
ttclose()
{
    if(!Pmaster)
      interrupt_ok();

    return(1);
}


/*
 * ttspeed - return tty line speed
 */
ttspeed()
{
    return(0);				/* DOS NO OP */
}


/*
 * Write a character to the display. 
 */
ttputc(c)
{
    return(bdos(6, c, 0));
}


/*
 * Flush terminal buffer. Does real work where the terminal output is buffered
 * up. A no-operation on systems where byte at a time terminal I/O is done.
 */
ttflush()
{
    return(1);
}


/*
 * specialkey - return special key definition
 */
specialkey(kc)
unsigned  kc;
{
    switch(kc){
	case 0x3b00 : return(F1);
	case 0x3c00 : return(F2);
	case 0x3d00 : return(F3);
	case 0x3e00 : return(F4);
	case 0x3f00 : return(F5);
	case 0x4000 : return(F6);
	case 0x4100 : return(F7);
	case 0x4200 : return(F8);
	case 0x4300 : return(F9);
	case 0x4400 : return(F10);
	case 0x8500 : return(F11);
	case 0x8600 : return(F12);
	case 0x4800 : return(K_PAD_UP);
	case 0x5000 : return(K_PAD_DOWN);
	case 0x4b00 : return(K_PAD_LEFT);
	case 0x4d00 : return(K_PAD_RIGHT);
	case 0x4700 : return(K_PAD_HOME);
	case 0x4f00 : return(K_PAD_END);
	case 0x4900 : return(K_PAD_PREVPAGE);
	case 0x5100 : return(K_PAD_NEXTPAGE);
	case 0x5300 : return(K_PAD_DELETE);
	case 0x48e0 : return(K_PAD_UP);			/* grey key version */
	case 0x50e0 : return(K_PAD_DOWN);		/* grey key version */
	case 0x4be0 : return(K_PAD_LEFT);		/* grey key version */
	case 0x4de0 : return(K_PAD_RIGHT);		/* grey key version */
	case 0x47e0 : return(K_PAD_HOME);		/* grey key version */
	case 0x4fe0 : return(K_PAD_END);		/* grey key version */
	case 0x49e0 : return(K_PAD_PREVPAGE);		/* grey key version */
	case 0x51e0 : return(K_PAD_NEXTPAGE);		/* grey key version */
	case 0x53e0 : return(K_PAD_DELETE);		/* grey key version */
	default     : return(NODATA);
    }
}


/*
 * Read a character from the terminal, performing no editing and doing no echo
 * at all. Also mouse events are forced into the input stream here.
 */
ttgetc()
{
    return(_bios_keybrd(enhncd ? _NKEYBRD_READ : _KEYBRD_READ));
}


/*
 * ctrlkey - used to check if the key hit was a control key.
 */
ctrlkey()
{
    return(_bios_keybrd(enhncd ? _NKEYBRD_SHIFTSTATUS : _KEYBRD_SHIFTSTATUS)
            & 0x04);
}


/*
 * Read in a key.
 * Do the standard keyboard preprocessing. Convert the keys to the internal
 * character set.  Resolves escape sequences and returns no-op if global
 * timeout value exceeded.
 */
GetKey()
{
    unsigned ch = 0, lch;
    long timein;

    if(mexist || timeout){
	timein = time(0L);
#ifdef	MOUSE
	if(mexist){
	    rg.x.ax = 1;			/* Show Cursor */
	    int86(BIOS_MOUSE, &rg, &rg); 
	}
#endif
	while(!_bios_keybrd(enhncd ? _NKEYBRD_READY : _KEYBRD_READY)){
#if	MOUSE
	    if(timeout && time(0L) >= timein+timeout){
		if(mexist){
		    rg.x.ax = 2;		/* Hide Cursor */
		    int86(BIOS_MOUSE, &rg, &rg);
		}
		return(NODATA);
	    }

	    if(checkmouse(&ch)){		/* something happen ?? */
		if(mexist){
		    rg.x.ax = 2;		/* Hide Cursor */
		    int86(BIOS_MOUSE, &rg, &rg);
		}
		curwp->w_flag |= WFHARD;
		return(ch);
	    }
#else
	    if(time(0L) >= timein+timeout)
	      return(NODATA);
#endif	/* MOUSE */
        }
#ifdef	MOUSE
	if(mexist){
	    rg.x.ax = 2;			/* Hide Cursor */
	    int86(BIOS_MOUSE, &rg, &rg);
	}
#endif	/* MOUSE */
    }

    ch  = (*term.t_getchar)();
    lch = (ch&0xff);
    return((lch && (lch != 0xe0)) 
            ? (lch < ' ') ? (CTRL|(lch + '@')) 
                          : (lch == ' ' && ctrlkey()) ? (CTRL|'@') : lch
            : specialkey(ch));
}


#if	MOUSE
/* 
 * checkmouse - look for mouse events in key menu and return 
 *              appropriate value.
 */
int
checkmouse(ch)
unsigned *ch;
{
    register int k;		/* current bit/button of mouse */
    int mcol;			/* current mouse column */
    int mrow;			/* current mouse row */
    int sstate;			/* current shift key status */
    int newbut;			/* new state of the mouse buttons */
    int rv = 0;

    if(!mexist)
	return(FALSE);

    /* check to see if any mouse buttons are different */
    rg.x.ax = 3;		/* Get button status and mouse position */
    int86(BIOS_MOUSE, &rg, &rg);
    newbut = rg.x.bx;
    mcol = rg.x.cx >> 3;
    mrow = (rg.x.dx >> 3);

    /* only notice changes */
    if (oldbut == newbut)
	return(FALSE);

    if (mcol < 0)		/* only on screen presses are legit! */
	mcol = 0;
    if (mrow < 0)
	mrow = 0;

    sstate = 0;			/* get the shift key status as well */
    rg.h.ah = 2;
    int86(BIOS_KEYBRD, &rg, &rg);
    sstate = rg.h.al;

    for (k=1; k != (1 << nbuttons); k = k<<1) {
	/* For each button on the mouse */
	if ((oldbut&k) != (newbut&k)) {
	    if(k == 1){
		static int oindex;
		int i = 0;

		if(newbut&k)			/* button down */
		  oindex = -1;

		if(mfunc.action && M_ACTIVE(mrow, mcol, &mfunc)){
		    unsigned long r;

		    if((r = (*mfunc.action)(newbut&k, mrow, mcol))&0xffff){
			*ch = (unsigned)((r>>16)&0xffff);
			rv  = TRUE;
		    }
		}
		else{
		    while(1){	/* see if we understand event */
			if(i >= 12){
			    i = -1;
			    break;
			}

			if(M_ACTIVE(mrow, mcol, &menuitems[i]))
			  break;

			i++;
		    }

		    if(newbut&k){			/* button down */
			oindex = i;			/* remember where */
			if(i != -1)			/* invert label */
			  invert_label(1, &menuitems[i]);
		    }
		    else{				/* button up */
			if(oindex != -1){
			    if(i == oindex){
				*ch = menuitems[i].val;
				rv = 1;
			    }
			}
		    }
		}

		if(!(newbut&k) && oindex != -1)
		  invert_label(0, &menuitems[oindex]);	/* restore label */
	    }

	    oldbut = newbut;
	    return(rv);
	}
    }

    return(FALSE);
}


/*
 * invert_label - highlight the label of the given menu item.
 */
void
invert_label(state, m)
int state;
MENUITEM *m;
{
    int i, j, r, c, p;
    char *lp;
    int old_state = getrevstate();

    if(m->val == mnoop)
      return;

    rg.h.ah = 3;				/* get cursor position */
    int86(BIOS_VIDEO, &rg, &rg);
    p = rg.h.bh;
    c = rg.h.dl;
    r = rg.h.dh;
    rg.x.ax = 2;				/* Hide Cursor */
    int86(BIOS_MOUSE, &rg, &rg);
    (*term.t_move)(m->tl.r, m->tl.c);
    (*term.t_rev)(state);
    for(i = m->tl.r; i <= m->br.r; i++)
      for(j = m->tl.c; j <= m->br.c; j++)
	if(i == m->lbl.r && j == m->lbl.c){	/* show label?? */
	    lp = m->label;
	    while(*lp && j++ < m->br.c)
	      (*term.t_putchar)(*lp++);

	    continue;
	}
	else
	  (*term.t_putchar)(' ');

    (*term.t_rev)(old_state);
    rg.h.ah = 2;
    rg.h.bh = p;
    rg.h.dh = r;
    rg.h.dl = c;
    int86(BIOS_VIDEO, &rg, &rg);		/* restore old position */
    rg.x.ax = 1;				/* Show Cursor */
    int86(BIOS_MOUSE, &rg, &rg);
}


/*
 * register_mfunc - register the given function to get called
 * 		    on mouse events in the given display region
 */
register_mfunc(f, tlr, tlc, brr, brc)
unsigned long (*f)();
int      tlr, tlc, brr, brc;
{
    if(!mexist)
      return(FALSE);

    mfunc.action = f;
    mfunc.tl.r   = tlr;
    mfunc.br.r   = brr;
    mfunc.tl.c   = tlc;
    mfunc.br.c   = brc;
    mfunc.lbl.c  = mfunc.lbl.r = 0;
    mfunc.label  = "";
    return(TRUE);
}


/*
 * clear_mfunc - clear any previously set mouse function
 */
void
clear_mfunc()
{
    mfunc.action = NULL;
}


/*
 * register_key - register the given keystroke to accept mouse events
 */
void
register_key(i, rval, label, row, col, len)
int       i;
unsigned  rval;
char     *label;
int       row, col, len;
{
    if(i > 11)
      return;

    menuitems[i].val   = rval;
    menuitems[i].tl.r  = menuitems[i].br.r = row;
    menuitems[i].tl.c  = col;
    menuitems[i].br.c  = col + len;
    menuitems[i].lbl.r = menuitems[i].tl.r;
    menuitems[i].lbl.c = menuitems[i].tl.c;
    menuitems[i].label = label;
}


/*
 * register_keys - take pico's key help strings and fit them into
 *                 the array the mouse uses
 */
register_keys(k, l, noop)
char     *k, *l;
unsigned  noop;
{
    unsigned  val;
    int       i = 0, ks, ls, n, slop;
    char     *ln, *hk = HelpKeyNames;

    if(!mexist)
      return(FALSE);

    mnoop = noop;
    /* size of key portion */
    ls = term.t_ncol/6;
    ks = (hk && hk[2] == ',') ? 2 : 3;
    for(i = 0;i < 12; i++){
	if(k[i] != '0'){			/* fill in struct */
	    if(!hk || hk[1] == '^')
	      val = (CTRL|(k[i]));
	    else if(hk[1] == 'F' && hk[2] != ',')
	      val = F1 + ((i < 6) ?  (2*i) : ((2*(i-6))+1));
	    else
	      val = k[i];

	    slop = (hk[1] == 'F' && hk[2] != ',' && val > F8) 
			? (val > F10) ? 2 : 1 : 0;
	    ln   = l;
	    for(n = 0;*l != ',' && *l != '\0'; l++)
	      if(n < ls - ks)
		n++;

	    l++;
	    register_key(i, val, ln, 
			 term.t_nrow - ((i < 6) ? 1 : 0),
			 ((i - ((i < 6) ? 0 : 6 )) * ls) + ks + slop,
			 n);
	}
	else
	  register_key(i, noop, NULL, 0, 0, 0);
    }
}


/*
 * pico_mouse - general handler for mouse events in the pico's
 *              text region.
 */
unsigned long
pico_mouse(down, row, col)
int down, row, col;
{
    static   int ldown = 0, lrow = 0, lcol = 0, double_click = 0;
    static   clock_t lastcalled = 0;
    unsigned long rv = FALSE;
    unsigned c;

    if(down){
	if(lrow == row && lcol == col){		/* same event!! */
	    if(clock() < (lastcalled + (clock_t)(CLOCKS_PER_SEC/2)))
	      double_click++;
	}

	lrow       = row;
	lcol       = col;
	lastcalled = clock();
    }
    else if(lrow == row && lcol == col){
	LINE *lp = curwp->w_linep;
	int    i;

	i = lrow - ((Pmaster) ? ComposerTopLine : 2);
	while(i-- && lp != curbp->b_linep)
	  lp = lforw(lp);

        curgoal = col;
	curwp->w_dotp = lp;
	curwp->w_doto = getgoal(lp);
	curwp->w_flag |= WFMOVE;

	if(double_click)
	  setmark(0, 1);

	double_click = 0;
	rv = NODATA;
	rv = (rv<<16)|TRUE;
    }
    return(rv);
}


void
mouseon()
{
    rg.x.ax = 1;			/* Show Cursor */
    int86(BIOS_MOUSE, &rg, &rg); 
}


void
mouseoff()
{
    rg.x.ax = 2;			/* Hide Cursor */
    int86(BIOS_MOUSE, &rg, &rg);
}
#endif	/* MOUSE */


/* kbseq - looks at an escape sequence coming from the keyboard and 
 *         compares it to a trie of known keyboard escape sequences, and
 *         performs the function bound to the escape sequence.
 * 
 *         returns: BADESC, the escaped function, or 0 if not found.
 */
kbseq(c)
int	*c;
{
    return(0);
}


/*
 * alt_editor - fork off an alternate editor for mail message composition
 *
 *  NOTE: Not yet used under DOS
 */
alt_editor(f, n)
{
    return(0);
}


/*
 *  bktoshell - suspend and wait to be woken up
 */
bktoshell()		/* suspend MicroEMACS and wait to wake up */
{
    int i;

    (*term.t_move)(term.t_nrow, 0);
    i = system("command");
    /* redraw */
    if(i == -1)
      emlwrite("Error loading COMMAND.COM");
    else
      refresh(0, 1);
}


/* 
 * rtfrmshell - back from shell, fix modes and return
 */
void
rtfrmshell()
{
}


/*
 * do_alarm_signal - jump back in the stack to where we can handle this
 */
void
do_alarm_signal()
{
}


/*
 * do_hup_signal - jump back in the stack to where we can handle this
 */
void
do_hup_signal()
{
}


unsigned char okinfname[32] = {
      0,    0, 			/* ^@ - ^G, ^H - ^O  */
      0,    0,			/* ^P - ^W, ^X - ^_  */
      0,    0x17,		/* SP - ' ,  ( - /   */
      0xff, 0xe0,		/*  0 - 7 ,  8 - ?   */
      0x7f, 0xff,		/*  @ - G ,  H - O   */
      0xff, 0xe9,		/*  P - W ,  X - _   */
      0x7f, 0xff,		/*  ` - g ,  h - o   */
      0xff, 0xf6,		/*  p - w ,  x - DEL */
      0,    0, 			/*  > DEL   */
      0,    0,			/*  > DEL   */
      0,    0, 			/*  > DEL   */
      0,    0, 			/*  > DEL   */
      0,    0 			/*  > DEL   */
};


/*
 * fallowc - returns TRUE if c is allowable in filenames, FALSE otw
 */
fallowc(c)
int c;
{
    return(okinfname[c>>3] & 0x80>>(c&7));
}


/*
 * fexist - returns TRUE if the file exists, FALSE otherwise
 */
fexist(file, m, l)
char *file, *m;
long *l;
{
    struct stat	sbuf;

    if(l != NULL)
      *l = 0L;

    if(stat(file, &sbuf) < 0){
	if(ENOENT)				/* File not found */
	  return(FIOFNF);
	else
	  return(FIOERR);
    }

    if(l != NULL)
      *l = sbuf.st_size;

    if(sbuf.st_mode & S_IFDIR)
      return(FIODIR);

    if(m[0] == 'r')				/* read access? */
      return((S_IREAD & sbuf.st_mode) ? FIOSUC : FIONRD);
    else if(m[0] == 'w')			/* write access? */
      return((S_IWRITE & sbuf.st_mode) ? FIOSUC : FIONWT);
    else if(m[0] == 'x')			/* execute access? */
      return((S_IEXEC & sbuf.st_mode) ? FIOSUC : FIONEX);
    return(FIOERR);				/* what? */
}


/*
 * isdir - returns true if fn is a readable directory, false otherwise
 *         silent on errors (we'll let someone else notice the problem;)).
 */
isdir(fn, l)
char *fn;
long *l;
{
    struct stat sbuf;

    if(l)
      *l = 0;

    if(stat(fn, &sbuf) < 0)
      return(0);

    if(l)
      *l = sbuf.st_size;

    return(sbuf.st_mode & S_IFDIR);
}


/*
 * gethomedir - returns the users home directory
 *              Note: home is malloc'd for life of pico
 */
char *gethomedir(l)
int *l;
{
    static char *home = NULL;
    static short hlen = 0;

    if(home == NULL){
	sprintf(s, "%c:\\", _getdrive() + 'A' - 1);
	hlen = strlen(s);
	if((home=(char *)malloc(((size_t)hlen + 1) * sizeof(char))) == NULL){
	    emlwrite("Problem allocating space for home dir", NULL);
	    return(0);
	}
	strcpy(home, s);
    }

    if(l)
      *l = hlen;

    return(home);
}


/*
 * homeless - returns true if given file does not reside in the current
 *            user's home directory tree. 
 */
homeless(f)
char *f;
{
    char *home;
    int   len;

    home = gethomedir(&len);
    return(strncmp(home, f, len));
}


/*
 * errstr - return system error string corresponding to given errno
 *          Note: strerror() is not provided on all systems, so it's 
 *          done here once and for all.
 */
char *errstr(err)
int err;
{
    return((err >= 0 && err < sys_nerr) ? sys_errlist[err] : NULL);
}


/*
 * getfnames - return all file names in the given directory in a single 
 *             malloc'd string.  n contains the number of names
 */
char *getfnames(dn, n)
char *dn;
int  *n;
{
    int status;
    long l;
    char *names, *np, *p;
    struct stat sbuf;
    struct find_t dbuf;					/* opened directory */

    *n = 0;

    if(stat(dn, &sbuf) < 0){
	sprintf(s, "\007Dir \"%s\": %s", dn, strerror(errno));
	emlwrite(s, NULL);
	return(NULL);
    } 
    else{
	l = sbuf.st_size;
	if(!(sbuf.st_mode & S_IFDIR)){
	    emlwrite("\007Not a directory: \"%s\"", dn);
	    return(NULL);
	}
    }

    if((names=(char *)malloc(sizeof(char)*3072)) == NULL){
	emlwrite("\007Can't malloc space for file names");
	return(NULL);
    }
    np = names;

    strcpy(s, dn);
    if(dn[strlen(dn)-1] == '\\')
      strcat(s, "*.*");
    else
      strcat(s, "\\*.*");

    if(_dos_findfirst(s, _A_NORMAL|_A_SUBDIR, &dbuf) != 0){
	emlwrite("Can't find first file in \"%s\"", dn);
	free((char *) names);
	return(NULL);
    }

    do{
	(*n)++;
	p = dbuf.name;
	while((*np++ = *p++) != '\0')
	  ;
    }
    while(_dos_findnext(&dbuf) == 0);

    return(names);
}


/*
 * fioperr - given the error number and file name, display error
 */
void
fioperr(e, f)
int  e;
char *f;
{
    switch(e){
      case FIOFNF:				/* File not found */
	emlwrite("\007File \"%s\" not found", f);
	break;
      case FIOEOF:				/* end of file */
	emlwrite("\007End of file \"%s\" reached", f);
	break;
      case FIOLNG:				/* name too long */
	emlwrite("\007File name \"%s\" too long", f);
	break;
      case FIODIR:				/* file is a directory */
	emlwrite("\007File \"%s\" is a directory", f);
	break;
      case FIONWT:
	emlwrite("\007Write permission denied: %s", f);
	break;
      case FIONRD:
	emlwrite("\007Read permission denied: %s", f);
	break;
      case FIONEX:
	emlwrite("\007Execute permission denied: %s", f);
	break;
      default:
	emlwrite("\007File I/O error: %s", f);
    }
}


/*
 * pfnexpand - pico's function to expand the given file name if there is 
 *	       a leading '~'
 */
char *pfnexpand(fn, len)
char *fn;
int  len;
{
    return(fn);
}


/*
 * fixpath - make the given pathname into an absolute path
 */
fixpath(name, len)
char *name;
int  len;
{
    char file[_MAX_PATH];
    int  dr;

    if(!len)
      return(0);
    
    /* return the full path of given file */
    if(isalpha(name[0]) && name[1] == ':'){	/* have drive spec? */
	if(name[2] != '\\'){			/* including path? */
	    dr = toupper(name[0]) - 'A' + 1;
	    if(_getdcwd(dr, file, _MAX_PATH) != NULL){
		strcat(file, "\\");
		strcat(file, &name[2]);		/* add file name */
	    }
	    else
	      return(0);
	}
	else
	  return(1);		/* fully qualified with drive and path! */
    }
    else if(name[0] == '\\') {			/* no drive spec! */
	sprintf(file, "%c:%s", _getdrive()+'A'-1, name);
    }
    else{
	if(Pmaster)				/* home dir relative */
	  strcpy(file, gethomedir(NULL));
	else if(!_getcwd(file, _MAX_PATH))	/* no qualification */
	  return(0);

	if(*name){				/* if name, append it */
	    strcat(file, "\\");
	    strcat(file, name);
	}
    }

    strncpy(name, file, len);			/* copy back to real buffer */
    name[len-1] = '\0';				/* tie off just in case */
    return(1);
}


/*
 * compresspath - given a base path and an additional directory, collapse
 *                ".." and "." elements and return absolute path (appending
 *                base if necessary).  
 *
 *                returns  1 if OK, 
 *                         0 if there's a problem
 *                         new path, by side effect, if things went OK
 */
compresspath(base, path, len)
char *base, *path;
int  len;
{
    register int i;
    int  depth = 0;
    char *p;
    char *stack[32];

#define PUSHD(X)  (stack[depth++] = X)
#define POPD()    ((depth > 0) ? stack[--depth] : "")

    strcpy(s, path);
    fixpath(s, len);

    p = s;
    for(i=0; s[i] != '\0'; i++){		/* pass thru path name */
	if(s[i] == C_FILESEP){
	    if(p != s)
	      PUSHD(p);				/* push dir entry */
	    p = &s[i+1];			/* advance p */
	    s[i] = '\0';			/* cap old p off */
	    continue;
	}

	if(s[i] == '.'){			/* special cases! */
	    if(s[i+1] == '.'			/* parent */
	       && (s[i+2] == C_FILESEP || s[i+2] == '\0')){
		if(!strcmp(POPD(),""))		/* bad news! */
		  return(0);

		i += 2;
		p = (s[i] == '\0') ? "" : &s[i+1];
	    }
	    else if(s[i+1] == C_FILESEP || s[i+1] == '\0'){	/* no op */
		i++;
		p = (s[i] == '\0') ? "" : &s[i+1];
	    }
	}
    }

    if(*p != '\0')
      PUSHD(p);					/* get last element */

    path[0] = '\0';
    for(i = 0; i < depth; i++){
	strcat(path, S_FILESEP);
	strcat(path, stack[i]);
    }

    return(1);					/* everything's ok */
}


/*
 * tmpname - return a temporary file name in the given buffer
 */
void
tmpname(name)
char *name;
{
    sprintf(name, tmpnam(NULL));	/* tmp file name */
}


/*
 * Take a file name, and from it
 * fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system.
 * I suppose that this information could be put in
 * a better place than a line of code.
 */
void
makename(bname, fname)
char    bname[];
char    fname[];
{
        register char   *cp1;
        register char   *cp2;

        cp1 = &fname[0];
        while (*cp1 != 0)
                ++cp1;

        while (cp1!=&fname[0] && cp1[-1]!='\\')
                --cp1;
        cp2 = &bname[0];
        while (cp2!=&bname[NBUFN-1] && *cp1!=0 && *cp1!=';')
                *cp2++ = *cp1++;
        *cp2 = 0;
}


/*
 * copy - copy contents of file 'a' into a file named 'b'.  Return error
 *        if either isn't accessible or is a directory
 */
copy(a, b)
char *a, *b;
{
    int    in, out, n, rv = 0;
    char   *cb;
    struct stat tsb, fsb;

    if(stat(a, &fsb) < 0){		/* get source file info */
	emlwrite("Can't Copy: %s", errstr(errno));
	return(-1);
    }

    if(!(fsb.st_mode&S_IREAD)){		/* can we read it? */
	emlwrite("\007Read permission denied: %s", a);
	return(-1);
    }

    if((fsb.st_mode&S_IFMT) == S_IFDIR){ /* is it a directory? */
	emlwrite("\007Can't copy: %s is a directory", a);
	return(-1);
    }

    if(stat(b, &tsb) < 0){		/* get dest file's mode */
	switch(errno){
	  case ENOENT:
	    break;			/* these are OK */
	  default:
	    emlwrite("\007Can't Copy: %s", errstr(errno));
	    return(-1);
	}
    }
    else{
	if(!(tsb.st_mode&S_IWRITE)){	/* can we write it? */
	    emlwrite("\007Write permission denied: %s", b);
	    return(-1);
	}

	if((tsb.st_mode&S_IFMT) == S_IFDIR){	/* is it directory? */
	    emlwrite("\007Can't copy: %s is a directory", b);
	    return(-1);
	}

	if(fsb.st_dev == tsb.st_dev && fsb.st_ino == tsb.st_ino){
	    emlwrite("\007Identical files.  File not copied", NULL);
	    return(-1);
	}
    }

    if((in = open(a, _O_RDONLY)) < 0){
	emlwrite("Copy Failed: %s", errstr(errno));
	return(-1);
    }

    if((out=creat(b, fsb.st_mode&0xfff)) < 0){
	emlwrite("Can't Copy: %s", errstr(errno));
	close(in);
	return(-1);
    }

    if((cb = (char *)malloc(NLINE*sizeof(char))) == NULL){
	emlwrite("Can't allocate space for copy buffer!", NULL);
	close(in);
	close(out);
	return(-1);
    }

    while(1){				/* do the copy */
	if((n = read(in, cb, NLINE)) < 0){
	    emlwrite("Can't Read Copy: %s", errstr(errno));
	    rv = -1;
	    break;			/* get out now */
	}

	if(n == 0)			/* done! */
	  break;

	if(write(out, cb, n) != n){
	    emlwrite("Can't Write Copy: %s", errstr(errno));
	    rv = -1;
	    break;
	}
    }

    free(cb);
    close(in);
    close(out);
    return(rv);
}


/*
 * Open a file for writing. Return TRUE if all is well, and FALSE on error
 * (cannot create).
 */
ffwopen(fn)
char    *fn;
{
    extern FILE *ffp;

    if ((ffp=fopen(fn, "w")) == NULL) {
        emlwrite("Cannot open file for writing");
        return (FIOERR);
    }

    return (FIOSUC);
}


/*
 * Close a file. Should look at the status in all systems.
 */
ffclose()
{
    extern FILE *ffp;

    if (fclose(ffp) != FALSE) {
        emlwrite("Error closing file");
        return(FIOERR);
    }

    return(FIOSUC);
}


/*
 * P_open - run the given command in a sub-shell returning a file pointer
 *	    from which to read the output
 *
 * note:
 *	For OS's other than unix, you will have to rewrite this function.
 *	Hopefully it'll be easy to exec the command into a temporary file, 
 *	and return a file pointer to that opened file or something.
 */
FILE *P_open(c)
char *c;
{
    sprintf(ptmpfile, tmpnam(NULL));
    sprintf(s, "%s > %s", c, ptmpfile);
    if(system(s) == -1){
	unlink(ptmpfile);
	return(NULL);
    }

    return(fopen(ptmpfile,"r"));
}



/*
 * P_close - close the given descriptor
 *
 */
P_close(fp)
FILE *fp;
{
    fclose(fp);			/* doesn't handle return codes */
    unlink(ptmpfile);
    return(0);;
}


/*
 * worthit - generic sort of test to roughly gage usefulness of using 
 *           optimized scrolling.
 *
 * note:
 *	returns the line on the screen, l, that the dot is currently on
 */
worthit(l)
int *l;
{
    int i;			/* l is current line */
    unsigned below;		/* below is avg # of ch/line under . */

    *l = doton(&i, &below);
    below = (i > 0) ? below/(unsigned)i : 0;

    return(below > 3);
}


/*
 * o_insert - optimize screen insert of char c
 */
o_insert(c)
char c;
{
    return(0);
}


/*
 * o_delete - optimized character deletion
 */
o_delete()
{
    return(0);
}


/*
 * pico_new_mail - just checks mtime and atime of mail file and notifies user 
 *	           if it's possible that they have new mail.
 */
pico_new_mail()
{
    return(0);
}



/*
 * time_to_check - checks the current time against the last time called 
 *                 and returns true if the elapsed time is > timeout
 */
time_to_check()
{
    static time_t lasttime = 0L;

    if(!timeout)
      return(FALSE);

    if(time((long *) 0) - lasttime > (time_t)timeout){
	lasttime = time((long *) 0);
	return(TRUE);
    }
    else
      return(FALSE);
}


/*
 * sstrcasecmp - compare two pointers to strings case independently
 */
sstrcasecmp(s1, s2)
QcompType *s1, *s2;
{
    return(stricmp(*(char **)s1, *(char **)s2));
}


/*
 * sleep the given number of microseconds
 */
ssleep(s)
    clock_t s;
{
    s += clock();
    while(s > clock())
      ;
}


/*
 * sleep the given number of seconds
 */
sleep(t)
    int t;
{
    time_t out = (time_t)t + time((long *) 0);
    while(out > time((long *) 0))
      ;
}
