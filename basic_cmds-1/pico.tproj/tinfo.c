#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id: tinfo.c,v 1.1.1.1 1999/04/15 17:45:14 wsanchez Exp $";
#endif
/*
 * Program:	Display routines
 *
 *
 * Donn Cave
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: donn@cac.washington.edu
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
 *      tinfo - substitute for tcap, on systems that have terminfo.
 */

#define	termdef	1			/* don't define "term" external */

#include	<stdio.h>
#include        <signal.h>
#include	"osdep.h"
#include	"estruct.h"
#include        "edef.h"
#include        "pico.h"

extern char *tigetstr ();

#define NROW    24
#define NCOL    80
#define	MARGIN	8
#define	SCRSIZ	64
#define BEL     0x07
#define ESC     0x1B

extern int      ttopen();
extern int      ttgetc();
extern int      ttputc();
extern int      ttflush();
extern int      ttclose();

static int      tinfomove();
static int      tinfoeeol();
static int      tinfoeeop();
static int      tinfobeep();
static int	tinforev();
static int      tinfoopen();
static int      tinfoclose();

extern int      tput();
extern char     *tgoto();

static int      kpinsert();
static int      putpad();

static char *UP, PC, *CM, *CE, *CL, *SO, *SE;
/* 
 * PICO extentions 
 */
static char *DL,			/* delete line */
	*AL,			/* insert line */
	*CS,			/* define a scrolling region, vt100 */
	*IC,			/* insert character, preferable to : */
	*IM,			/* set insert mode and, */
	*EI,			/* end insert mode */
	*DC,			/* delete character */
	*DM,			/* set delete mode and, */
	*ED,			/* end delete mode */
	*SF,			/* scroll text up */
	*SR,			/* scroll text down */
	*TI,			/* string to start termcap */
        *TE;			/* string to end termcap */

static char *KU, *KD, *KL, *KR;
static char *KPPU, *KPPD, *KPHOME, *KPEND;

struct KBSTREE *kpadseqs = NULL;

TERM term = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
        tinfoopen,
        tinfoclose,
        ttgetc,
        ttputc,
        ttflush,
        tinfomove,
        tinfoeeol,
        tinfoeeop,
        tinfobeep,
        tinforev
};


static tinfoopen()
{
    char  *t;
    char  *getenv();

    ttgetwinsz();

    /*
     * determine the terminal's communication speed and decide
     * if we need to do optimization ...
     */
    optimize = ttisslow();

    if (Pmaster) {
	/*
	 *		setupterm() automatically retrieves the value
	 *		of the TERM variable.
	 */
	int err;
	setupterm (0, 1, &err);
	if (err != 1) return FALSE;
    }
    else {
	/*
	 *		setupterm() issues a message and exits, if the
	 *		terminfo data base is gone or the term type is
	 *		unknown, if arg2 is 0.
	 */
	setupterm (0, 1, 0);
    }

    t = tigetstr("pad");
    if(t)
      PC = *t;

    CL = tigetstr("clear");
    CM = tigetstr("cup");
    CE = tigetstr("el");
    UP = tigetstr("cuu1");
    SE = tigetstr("rmso");
    SO = tigetstr("smso");
    DL = tigetstr("dl1");
    AL = tigetstr("il1");
    CS = tigetstr("csr");
    IC = tigetstr("ich1");
    IM = tigetstr("smir");
    EI = tigetstr("rmir");
    DC = tigetstr("dch1");
    DM = tigetstr("smdc");
    ED = tigetstr("rmdc");
    SF = tigetstr("ind");
    SR = tigetstr("ri");
    TI = tigetstr("smcup");
    TE = tigetstr("rmcup");

    eolexist = (CE != NULL);	/* will we be able to use clear to EOL? */
    revexist = (SO != NULL);	/* will be able to use reverse video */
    if(DC == NULL && (DM == NULL || ED == NULL))
      delchar = FALSE;
    if(IC == NULL && (IM == NULL || EI == NULL))
      inschar = FALSE;
    if((CS==NULL || SF==NULL || SR==NULL) && (DL==NULL || AL==NULL))
      scrollexist = FALSE;

    if(CL == NULL || CM == NULL || UP == NULL){
	if(Pmaster == NULL){
	    puts("Incomplete terminfo entry\n");
	    exit(1);
	}
    }
    else{
	KPPU   = tigetstr("kpp");
	KPPD   = tigetstr("knp");
	KPHOME = tigetstr("khome");
	KU = tigetstr("kcuu1");
	KD = tigetstr("kcud1");
	KL = tigetstr("kcub1");
	KR = tigetstr("kcuf1");
	if(KU != NULL && (KL != NULL && (KR != NULL && KD != NULL))){
	    kpinsert(KU,K_PAD_UP);
	    kpinsert(KD,K_PAD_DOWN);
	    kpinsert(KL,K_PAD_LEFT);
	    kpinsert(KR,K_PAD_RIGHT);

	    if(KPPU != NULL)
	      kpinsert(KPPU,K_PAD_PREVPAGE);
	    if(KPPD != NULL)
	      kpinsert(KPPD,K_PAD_NEXTPAGE);
	    if(KPHOME != NULL)
	      kpinsert(KPHOME,K_PAD_HOME);
	}
    }

    /*
     * add default keypad sequences to the trie...
     */
    if(gmode&MDFKEY){
	/*
	 * Initialize UW-modified NCSA telnet to use its functionkeys
	 */
	if(Pmaster == NULL){
	    puts("\033[99h");
	}

	/*
	 * this is sort of a hack [no kidding], but it allows us to use
	 * the function keys on pc's running telnet
	 */

	/* 
	 * UW-NDC/UCS vt10[01] application mode.
	 */
	kpinsert("OP",F1);
	kpinsert("OQ",F2);
	kpinsert("OR",F3);
	kpinsert("OS",F4);
	kpinsert("Op",F5);
	kpinsert("Oq",F6);
	kpinsert("Or",F7);
	kpinsert("Os",F8);
	kpinsert("Ot",F9);
	kpinsert("Ou",F10);
	kpinsert("Ov",F11);
	kpinsert("Ow",F12);

	/*
	 * special keypad functions
	 */
	kpinsert("[4J",K_PAD_PREVPAGE);
	kpinsert("[3J",K_PAD_NEXTPAGE);
	kpinsert("[2J",K_PAD_HOME);
	kpinsert("[N",K_PAD_END);

	/* 
	 * ANSI mode.
	 */
	kpinsert("[=a",F1);
	kpinsert("[=b",F2);
	kpinsert("[=c",F3);
	kpinsert("[=d",F4);
	kpinsert("[=e",F5);
	kpinsert("[=f",F6);
	kpinsert("[=g",F7);
	kpinsert("[=h",F8);
	kpinsert("[=i",F9);
	kpinsert("[=j",F10);
	kpinsert("[=k",F11);
	kpinsert("[=l",F12);

	HelpKeyNames = funckeynames;

    }
    else{
	HelpKeyNames = NULL;
    }

    /*
     * DEC vt100, ANSI and cursor key mode.
     */
    kpinsert("OA",K_PAD_UP);
    kpinsert("OB",K_PAD_DOWN);
    kpinsert("OD",K_PAD_LEFT);
    kpinsert("OC",K_PAD_RIGHT);

    /*
     * DEC vt100, ANSI and cursor key mode reset.
     */
    kpinsert("[A",K_PAD_UP);
    kpinsert("[B",K_PAD_DOWN);
    kpinsert("[D",K_PAD_LEFT);
    kpinsert("[C",K_PAD_RIGHT);

    /*
     * DEC vt52 mode.
     */
    kpinsert("A",K_PAD_UP);
    kpinsert("B",K_PAD_DOWN);
    kpinsert("D",K_PAD_LEFT);
    kpinsert("C",K_PAD_RIGHT);

    /*
     * Sun Console sequences.
     */
    kpinsert("[215z",K_PAD_UP);
    kpinsert("[221z",K_PAD_DOWN);
    kpinsert("[217z",K_PAD_LEFT);
    kpinsert("[219z",K_PAD_RIGHT);

    ttopen();

    if(TI && !Pmaster){
	putpad(TI);			/* any init terminfo requires */
	if(CS)
	  putpad(tgoto(CS, term.t_nrow, 0));
    }
}


static tinfoclose()
{
    if(!Pmaster){
	if(gmode&MDFKEY)
	  puts("\033[99l");		/* reset UW-NCSA telnet keys */

	if(TE)				/* any clean up terminfo requires */
	  putpad(TE);
    }

    ttclose();
}


#define	newnode()	(struct KBSTREE *)malloc(sizeof(struct KBSTREE))
/*
 * kbinsert - insert a keystroke escape sequence into the global search
 *	      structure.
 */
static kpinsert(kstr, kval)
char	*kstr;
int	kval;
{
    register	char	*buf;
    register	struct KBSTREE *temp;
    register	struct KBSTREE *trail;

    if(kstr == NULL)
      return;

    temp = trail = kpadseqs;
    if(kstr[0] == '\033')
      buf = kstr+1;			/* can the ^[ character */ 
    else
      buf = kstr;

    for(;;) {
	if(temp == NULL){
	    temp = newnode();
	    temp->value = *buf;
	    temp->func = 0;
	    temp->left = NULL;
	    temp->down = NULL;
	    if(kpadseqs == NULL)
	      kpadseqs = temp;
	    else
	      trail->down = temp;
	}
	else{				/* first entry */
	    while((temp != NULL) && (temp->value != *buf)){
		trail = temp;
		temp = temp->left;
	    }
	    if(temp == NULL){   /* add new val */
		temp = newnode();
		temp->value = *buf;
		temp->func = 0;
		temp->left = NULL;
		temp->down = NULL;
		trail->left = temp;
	    }
	}

	if (*(++buf) == '\0'){
	    break;
	}
	else{
	    trail = temp;
	    temp = temp->down;
	}
    }

    if(temp != NULL)
      temp->func = kval;
}


/*
 * tinfoinsert - insert a character at the current character position.
 *               IC takes precedence.
 */
tinfoinsert(ch)
register char	ch;
{
    if(IC != NULL){
	putpad(IC);
	ttputc(ch);
    }
    else{
	putpad(IM);
	ttputc(ch);
	putpad(EI);
    }
}


/*
 * tinfodelete - delete a character at the current character position.
 */
tinfodelete()
{
    if(DM == NULL && ED == NULL)
      putpad(DC);
    else{
	putpad(DM);
	putpad(DC);
	putpad(ED);
    }
}


/*
 * o_scrolldown() - open a line at the given row position.
 *               use either region scrolling or deleteline/insertline
 *               to open a new line.
 */
o_scrolldown(row, n)
register int row;
register int n;
{
    register int i;

    if(CS != NULL){
	putpad(tgoto(CS, term.t_nrow - 3, row));
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad( (SR != NULL && *SR != '\0') ? SR : "\n" );
	putpad(tgoto(CS, term.t_nrow, 0));
	tinfomove(row, 0);
    }
    else{
	/*
	 * this code causes a jiggly motion of the keymenu when scrolling
	 */
	for(i = 0; i < n; i++){
	    tinfomove(term.t_nrow - 3, 0);
	    putpad(DL);
	    tinfomove(row, 0);
	    putpad(AL);
	}
#ifdef	NOWIGGLYLINES
	/*
	 * this code causes a sweeping motion up and down the display
	 */
	tinfomove(term.t_nrow - 2 - n, 0);
	for(i = 0; i < n; i++)
	  putpad(DL);
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad(AL);
#endif
    }
}


/*
 * o_scrollup() - open a line at the given row position.
 *               use either region scrolling or deleteline/insertline
 *               to open a new line.
 */
o_scrollup(row, n)
register int row;
register int n;
{
    register int i;

    if(CS != NULL){
	putpad(tgoto(CS, term.t_nrow - 3, row));
	/* setting scrolling region moves cursor to home */
	tinfomove(term.t_nrow-3, 0);
	for(i = 0;i < n; i++)
	  putpad((SF == NULL || SF[0] == '\0') ? "\n" : SF);
	putpad(tgoto(CS, term.t_nrow, 0));
	tinfomove(2, 0);
    }
    else{
	for(i = 0; i < n; i++){
	    tinfomove(row, 0);
	    putpad(DL);
	    tinfomove(term.t_nrow - 3, 0);
	    putpad(AL);
	}
#ifdef  NOWIGGLYLINES
	/* see note above */
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad(DL);
	tinfomove(term.t_nrow - 2 - n, 0);
	for(i = 0;i < n; i++)
	  putpad(AL);
#endif
    }
}



/*
 * o_insert - use terminfo to optimized character insert
 *            returns: true if it optimized output, false otherwise
 */
o_insert(c)
char c;
{
    if(inschar){
	tinfoinsert(c);
	return(1);			/* no problems! */
    }

    return(0);				/* can't do it. */
}


/*
 * o_delete - use terminfo to optimized character insert
 *            returns true if it optimized output, false otherwise
 */
o_delete()
{
    if(delchar){
	tinfodelete();
	return(1);			/* deleted, no problem! */
    }

    return(0);				/* no dice. */
}


static tinfomove(row, col)
register int row, col;
{
    putpad(tgoto(CM, col, row));
}


static tinfoeeol()
{
    putpad(CE);
}


static tinfoeeop()
{
        putpad(CL);
}


static tinforev(state)		/* change reverse video status */
int state;	                /* FALSE = normal video, TRUE = rev video */
{
    static int cstate = FALSE;

    if(state == cstate)		/* no op if already set! */
      return(0);

    if(cstate = state){		/* remember last setting */
	if (SO != NULL)
	  putpad(SO);
    } else {
	if (SE != NULL)
	  putpad(SE);
    }
}


static tinfobeep()
{
    ttputc(BEL);
}


static putpad(str)
char    *str;
{
    tputs(str, 1, ttputc);
}


static putnpad(str, n)
char    *str;
{
    tputs(str, n, ttputc);
}
