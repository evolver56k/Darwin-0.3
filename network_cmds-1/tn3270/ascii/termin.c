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
/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#ifndef lint
static char sccsid[] = "@(#)termin.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/* this takes characters from the keyboard, and produces 3270 keystroke
	codes
 */

#include <stdio.h>
#include <ctype.h>

#include "../general/general.h"
#include "../ctlr/function.h"
#include "../ctlr/externs.h"
#include "../ctlr/declare.h"

#include "../api/astosc.h"
#include "state.h"

#include "../general/globals.h"

#define IsControl(c)	(!isprint(c) || (isspace(c) && ((c) != ' ')))

#define NextState(x)	(x->next)

/* XXX temporary - hard code in the state table */

#define MATCH_ANY 0xff			/* actually, match any character */


static unsigned char
	ourBuffer[100],		/* where we store stuff */
	*ourPHead = ourBuffer,	/* first character in buffer */
	*ourPTail = ourBuffer,	/* where next character goes */
	*TransPointer = 0;	/* For transparent mode data */

static int InControl;
static int WaitingForSynch;

static struct astosc
	*spacePTR = 0;		/* Space is hard to enter */

static state
	*headOfControl = 0;	/* where we enter code state table */

#define FullChar	((ourPTail+5) >= ourBuffer+sizeof ourBuffer)
#define EmptyChar	(ourPTail == ourPHead)


/*
 * init_keyboard()
 *
 * Initialize the keyboard variables.
 */

void
init_keyboard()
{
    ourPHead = ourPTail = ourBuffer;
    InControl = 0;
    WaitingForSynch = 0;
}


/*
 * Initialize the keyboard mapping file.
 */

void
InitMapping()
{
    extern state *InitControl();
    register struct astosc *ptr;

    if (!headOfControl) {
	/* need to initialize */
	headOfControl = InitControl((char *)0, 0, ascii_to_index);
	if (!headOfControl) {		/* should not occur */
	    quit();
	}
	for (ptr = &astosc[0]; ptr <= &astosc[highestof(astosc)]; ptr++) {
	    if (ptr->function == FCN_SPACE) {
		spacePTR = ptr;
	    }
	}
    }
}


/* AddChar - put a function index in our buffer */

static void
AddChar(c)
int	c;
{
    if (!FullChar) {
	*ourPTail++ = c;
    } else {
	RingBell("Typeahead buffer full");
    }
}

/* FlushChar - put everything where it belongs */

static void
FlushChar()
{
    ourPTail = ourBuffer;
    ourPHead = ourBuffer;
}

/*ARGSUSED*/
void
TransInput(onoff, mode)
int	mode;			/* Which KIND of transparent input */
int	onoff;			/* Going in, or coming out */
{
    if (onoff) {
	/* Flush pending input */
	FlushChar();
	TransPointer = ourBuffer;
    } else {
    }
}

int
TerminalIn()
{
    /* send data from us to next link in stream */
    int work = 0;
    register struct astosc *ptr;

    while (!EmptyChar) {			/* send up the link */
	if (*ourPHead == ' ') {
	    ptr = spacePTR;
	} else {
	    ptr = &astosc[*ourPHead];
	}
	if (AcceptKeystroke(ptr->scancode, ptr->shiftstate) == 1) {
	    ourPHead++;
	    work = 1;
	} else {
	    break;
	}
    }

    if (EmptyChar) {
	FlushChar();
    }
	/* return value answers question: "did we do anything useful?" */
    return work;
}

int
DataFromTerminal(buffer, count)
register char	*buffer;		/* the data read in */
register int	count;			/* how many bytes in this buffer */
{
    register state *regControlPointer;
    register char c;
    register int result;
    int origCount;
    extern int bellwinup;
    static state *controlPointer;

    if (TransPointer) {
	int i;

	if ((count+TransPointer) >= (ourBuffer+sizeof ourBuffer)) {
	    i = ourBuffer+sizeof ourBuffer-TransPointer;
	} else {
	    i = count;
	}
	while (i--) {
	    c = (*buffer++)&0x7f;
	    *TransPointer++ = c|0x80;
	    if (c == '\r') {
		SendTransparent((char *)ourBuffer, TransPointer-ourBuffer);
		TransPointer = 0;		/* Done */
		break;
	    }
	}
	return count;
    }

    if (bellwinup) {
	void BellOff();

	BellOff();
    }

    origCount = count;

    while (count) {
	c = *buffer++&0x7f;
	count--;

	if (!InControl && !IsControl(c)) {
	    AddChar(c);			/* add ascii character */
	} else {
	    if (!InControl) {		/* first character of sequence */
		InControl = 1;
		controlPointer = headOfControl;
	    }
	    /* control pointer points to current position in state table */
	    for (regControlPointer = controlPointer; ;
			regControlPointer = NextState(regControlPointer)) {
		if (!regControlPointer) {	/* ran off end */
		    RingBell("Invalid control sequence");
		    regControlPointer = headOfControl;
		    InControl = 0;
		    count = 0;		/* Flush current input */
		    break;
		}
		if ((regControlPointer->match == c) /* hit this character */
			|| (regControlPointer->match == MATCH_ANY)) {
		    result = regControlPointer->result;
		    if (result == STATE_GOTO) {
			regControlPointer = regControlPointer->address;
			break;			/* go to next character */
		    }
		    if (WaitingForSynch) {
			if (astosc[result].function == FCN_SYNCH) {
			    WaitingForSynch = 0;
			} else {
			    void RingBell();

			    RingBell("Need to type synch character");
			}
		    }
		    else if (astosc[result].function == FCN_FLINP) {
			FlushChar();		/* Don't add FLINP */
		    } else {
			if (astosc[result].function == FCN_MASTER_RESET) {
			    FlushChar();
			}
			AddChar(result);		/* add this code */
		    }
		    InControl = 0;	/* out of control now */
		    break;
		}
	    }
	    controlPointer = regControlPointer;		/* save state */
	}
    }
    (void) TerminalIn();			/* try to send data */
    return(origCount-count);
}
