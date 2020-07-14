/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*
 * Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved.
 *
 * miniMon.c -- Machine-independent mini-monitor.
 *
 * HISTORY
 * 04-Oct-1993		Curtis Galloway at NeXT
 *	Created.
 *
 */

#import <kern/miniMon.h>
#import <kern/miniMonPrivate.h>
#import <sys/param.h>
#import <sys/systm.h>
#import <sys/reboot.h>
#include <stdarg.h>
#import <mach/machine/simple_lock.h>

extern int boothowto;

static boolean_t miniMonContinue(char *);
static boolean_t miniMonHelp(char *);
static boolean_t miniMonResetKdp(char *);
static boolean_t miniMonTryGdb(char *);
static boolean_t miniMonForceGdb(char *);
void *miniMonState;

miniMonCommand_t miniMonCommands[] = {
    {"continue", miniMonContinue, "Continue execution"},
    {"reboot", miniMonReboot, "Reboot the computer, no sync"},
    {"halt", miniMonHalt, "Sync and halt the computer"},
    {"gdb", miniMonTryGdb, "Break to remote debugger"},
    {"forcegdb", miniMonForceGdb, "Break to remote debugger"},
    {"reset", miniMonResetKdp, "Reset debugger state"},
    {"help", miniMonHelp, 0},
    {"?", miniMonHelp, 0},
    {0,0}
};

 
static match_t
commandcmp(char *command, char *line)
{
    while (*command) {
	if (*line == ' ' || *line == '\t' ||
	    *line == '\n' || *line == '\0')
		return FOUND;
	if (*command++ != *line++)
	    return NOT_FOUND;
    }
    return FOUND;
}



static boolean_t
miniMonParseLine(char *line)
{
    struct miniMonCommand *command, *found_command = NULL;
    
    for (command = miniMonCommands; command->name; command++) {
	if (commandcmp(command->name, line) == FOUND) {
	    if (found_command != NULL) {
		safe_prf("Ambiguous command - type '?' for help\n");
		return TRUE;
	    } else {
		found_command = command;
	    }
	}
    }
    for (command = miniMonMDCommands; command->name; command++) {
	if (commandcmp(command->name, line) == FOUND) {
	    if (found_command != NULL) {
		safe_prf("Ambiguous command - type '?' for help\n");
		return TRUE;
	    } else {
		found_command = command;
	    }
	}
    }
    if (found_command != NULL) {
	return (*found_command->function)(line);
    } else {
	safe_prf("Invalid command - type '?' for help\n");
	return TRUE;
    }
}

static void
miniMonGetLine(char *line, int len)
{
    register int c;
    char *anchor = line;

    len--;	/* for '\0' */
    for (;;) {
	c = miniMonGetchar();
	switch (c) {
	case '\r':
	    miniMonPutchar('\n');
	    /* fall into ... */
	case '\n':
	    *line++ = '\0';
	    return;
	case '\b':	/* bs */
	    miniMonPutchar(' ');
	    if (line != anchor) {
		miniMonPutchar('\b');
		line--;
		len++;
	    }
	    continue;
	case '\025':	/* ^u */
	    line = anchor;
	    miniMonPutchar('\n');
	    continue;
	default:
	    if (len) {
		*line++ = c;
		len--;
	    } else {
		miniMonPutchar('\b');
		miniMonPutchar(' ');
		miniMonPutchar('\b');
	    }
	}
    }
}

simple_lock_t _kernDebuggerLock;

void
miniMonInit(void)
{
    /*
     * Initialize kernel debugger lock.
     */
    _kernDebuggerLock = simple_lock_alloc();
    simple_lock_init(_kernDebuggerLock);
}


static char miniMonLine[128];

void
miniMonLoop(char *prompt, int panic, void *state)
{
    int c;
    
    miniMonState = state;
    if (panic) {
	safe_prf("System Panic:\n");
	safe_prf("%s\n",panicstr);
#if defined(ppc)
    {
		unsigned int *fp;
        unsigned int register sp;
		unsigned int depth=15;
        __asm__ volatile("mr %0,r1" : "=r" (sp));

        safe_prf("stack backtrace -  ");
        fp = *((unsigned int *)sp);
        while (fp && (--depth)) {
            safe_prf("0x%08x ", fp[2]);
            fp = (unsigned int *)*fp;
        }
        safe_prf("\n");
    }
#endif /* ppc */

	if ((boothowto & RB_DEBUG) == 0) {
	    /* Reboot automatically */
	    safe_prf("(Type 'm' for monitor,\n");
	    safe_prf("or wait for automatic reboot)");
	    /* Wait for 30 seconds */
	    for (c=30000; c>0; c--) {
		if (miniMonTryGetchar() == 'm') {
		    safe_prf("\n");
		    break;
		}
		us_spin(1000);
	    }
	    if (c == 0) {
		safe_prf("\nRebooting...");
		miniMonReboot("");
	    }
	} else {
	    safe_prf("(Type 'r' to reboot or 'm' for monitor)");
	    for (;;) {
		c = miniMonTryGetchar();
		if (c == 'r') {
		    safe_prf("\nRebooting...");
		    miniMonReboot("");
		} else if (c == 'm') {
		    safe_prf("\n");
		    break;
		}
	    }
	}
    }
    safe_prf("Mini-monitor\n");
    for(;;) {
	safe_prf("%s> ",prompt);
	miniMonGetLine(miniMonLine, sizeof(miniMonLine));
	if (miniMonParseLine(miniMonLine) == FALSE)
	    break;
    }
}

static boolean_t
miniMonContinue(char *line)
{
    return FALSE;
}

static boolean_t
miniMonHelp(char *line)
{
    struct miniMonCommand *command;
    safe_prf("Mini-monitor commands:\n");
    safe_prf("?,help - Print this message\n");
    for (command = miniMonCommands; command->name; command++) {
	if (command->help)
	    safe_prf("%s - %s\n",command->name,command->help);
    }
    for (command = miniMonMDCommands; command->name; command++) {
	if (command->help)
	    safe_prf("%s - %s\n",command->name,command->help);
    }
    return TRUE;
}

/* keep messages from getting written to message log */
void safe_prf(const char *format, ...)
{
    va_list ap;
    static char buf[512];
    char *p = buf;
	
    va_start(ap, format);
#define	TOSTR	0x8
    prf(format, ap, TOSTR, (struct tty *)&p);
    va_end(ap);
    *p++ = '\0';

    p = buf;	
    while (*p)
	miniMonPutchar(*p++);
}

static boolean_t
miniMonResetKdp(char *line)
{
    extern void kdp_reset(void);
    
    kdp_reset();
    return TRUE;
}

static boolean_t
miniMonTryGdb(char *line)
{
    extern simple_lock_t _kernDebuggerLock;
    boolean_t ret;

    if (simple_lock_try(_kernDebuggerLock)) {
	ret = miniMonGdb(line);
	simple_unlock(_kernDebuggerLock);
    } else {
	safe_prf("Couldn't acquire debugger lock:\n");
	safe_prf("exit from monitor and try again.\n");
	ret = TRUE;
    }
    return ret;
}

static boolean_t
miniMonForceGdb(char *line)
{
    extern simple_lock_t _kernDebuggerLock;
    boolean_t ret;

    simple_lock_try(_kernDebuggerLock);
	ret = miniMonGdb(line);
	simple_unlock(_kernDebuggerLock);

    return ret;
}
