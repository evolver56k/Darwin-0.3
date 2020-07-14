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
 * Copyright (c) 1992, 1993
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


/*
 * $Source: /cvs/Darwin/CoreOS/Libraries/NeXT/libtelnet/read_password.c,v $
 * $Author: wsanchez $
 *
 * Copyright 1985, 1986, 1987, 1988 by the Massachusetts Institute
 * of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * This routine prints the supplied string to standard
 * output as a prompt, and reads a password string without
 * echoing.
 */

#if	defined(RSA_ENCPWD) || defined(KRB4_ENCPWD)

#include <stdio.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <setjmp.h>

static jmp_buf env;

/*** Routines ****************************************************** */
/*
 * This version just returns the string, doesn't map to key.
 *
 * Returns 0 on success, non-zero on failure.
 */

int
local_des_read_pw_string(s,max,prompt,verify)
    char *s;
    int	max;
    char *prompt;
    int	verify;
{
    int ok = 0;
    char *ptr;
    
    jmp_buf old_env;
    struct sgttyb tty_state;
    char key_string[BUFSIZ];

    if (max > BUFSIZ) {
	return -1;
    }

    /* XXX assume jmp_buf is typedef'ed to an array */
    bcopy((char *)old_env, (char *)env, sizeof(env));
    if (setjmp(env))
	goto lose;

    /* save terminal state*/
    if (ioctl(0,TIOCGETP,(char *)&tty_state) == -1) 
	return -1;
/*
    push_signals();
*/
    /* Turn off echo */
    tty_state.sg_flags &= ~ECHO;
    if (ioctl(0,TIOCSETP,(char *)&tty_state) == -1)
	return -1;
    while (!ok) {
	(void) printf(prompt);
	(void) fflush(stdout);
	while (!fgets(s, max, stdin));

	if ((ptr = index(s, '\n')))
	    *ptr = '\0';
	if (verify) {
	    printf("\nVerifying, please re-enter %s",prompt);
	    (void) fflush(stdout);
	    if (!fgets(key_string, sizeof(key_string), stdin)) {
		clearerr(stdin);
		continue;
	    }
            if ((ptr = index(key_string, '\n')))
	    *ptr = '\0';
	    if (strcmp(s,key_string)) {
		printf("\n\07\07Mismatch - try again\n");
		(void) fflush(stdout);
		continue;
	    }
	}
	ok = 1;
    }

lose:
    if (!ok)
	bzero(s, max);
    printf("\n");
    /* turn echo back on */
    tty_state.sg_flags |= ECHO;
    if (ioctl(0,TIOCSETP,(char *)&tty_state))
	ok = 0;
/*
    pop_signals();
*/
    bcopy((char *)env, (char *)old_env, sizeof(env));
    if (verify)
	bzero(key_string, sizeof (key_string));
    s[max-1] = 0;		/* force termination */
    return !ok;			/* return nonzero if not okay */
}
#endif	/* defined(RSA_ENCPWD) || defined(KRB4_ENCPWD) */
