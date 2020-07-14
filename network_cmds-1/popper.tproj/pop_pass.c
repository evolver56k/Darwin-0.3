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
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_pass.c	2.3  2.3 4/2/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <pwd.h>
#include "popper.h"

/* 
 *  pass:   Obtain the user password from a POP client
 */

int pop_pass (p)
POP     *   p;
{
    register struct passwd  *   pw;
    char *crypt();

#ifdef NeXT
    /* Check with restricted users list */
    if (p->restricted_users) {
	const char **pp;

	for (pp = p->restricted_users; *pp != NULL; pp++) {
	    if (strcmp(*pp, p->user) == 0)
		break;
	}
	if (*pp == NULL) {
	    /* Not in list; deny access */
	    return (pop_msg(p, POP_FAILURE,
		"Access denied for \"%s\".", p->user));
	}
    }
#endif /* NeXT */

    /*  Look for the user in the password file */
    if ((pw = getpwnam(p->user)) == NULL)
        return (pop_msg(p,POP_FAILURE,
            "Password supplied for \"%s\" is incorrect.",p->user));

    /*  We don't accept connections from users with null passwords */
    if (pw->pw_passwd == NULL)
        return (pop_msg(p,POP_FAILURE,
            "Password supplied for \"%s\" is incorrect.",p->user));

    /*  Compare the supplied password with the password file entry */
    if (strcmp (crypt (p->pop_parm[1], pw->pw_passwd), pw->pw_passwd) != 0)
        return (pop_msg(p,POP_FAILURE,
            "Password supplied for \"%s\" is incorrect.",p->user));

#ifdef NeXT
    /* Become the user right away -- we may not have root privileges to
     * the mail spool if we're using NFS and besides, pop_mailfile() needs
     * the right uid to find the proper default spool dir.
     */
    (void) setuid(pw->pw_uid);
    (void) setgid(pw->pw_gid);

    /*  Build the name of the user's maildrop */
    (void)pop_mailfile(p->drop_name,"%s",p->user);
#else
    /*  Build the name of the user's maildrop */
    (void)sprintf(p->drop_name,"%s/%s",POP_MAILDIR,p->user);
#endif /* !NeXT */

    /*  Make a temporary copy of the user's maildrop */
    /*    and set the group and user id */
    if (pop_dropcopy(p,pw) != POP_SUCCESS) return (POP_FAILURE);

    /*  Get information about the maildrop */
    if (pop_dropinfo(p) != POP_SUCCESS) return(POP_FAILURE);

    /*  Initialize the last-message-accessed number */
    p->last_msg = 0;

    /*  Authorization completed successfully */
    return (pop_msg (p,POP_SUCCESS,
        "%s has %d message(s) (%d octets).",
            p->user,p->msg_count,p->drop_size));
}
