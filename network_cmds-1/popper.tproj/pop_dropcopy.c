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
static char SccsId[] = "@(#)@(#)pop_dropcopy.c	2.6  2.6 4/3/91";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <pwd.h>
#ifdef NeXT
#include <stdarg.h>
#include <sys/param.h>
#endif /* NeXT */
#include "popper.h"

#ifndef NeXT
extern int      errno;
extern int      sys_nerr;
extern char    *sys_errlist[];
#endif /* !NeXT */

#ifdef NeXT
/* Return the name of the user's spool file.  Under NEXTSTEP, we check
 * Mail.app's SpoolDir preference in case the user has a mail drop somewhere
 * else than /var/mail.
 */
char *
pop_mailfile(char *buf, const char *fmt, ...)
{
#if BSD < 199506
    extern char *NXGetDefaultValue();
    const char *maildir = NXGetDefaultValue("Mail", "SpoolDir");
    va_list args;

    if (maildir == NULL)
	maildir = POP_MAILDIR;
#else
#define maildir POP_MAILDIR
    va_list args;
#endif

    va_start(args, fmt);
    sprintf(buf, "%s/", maildir);
    vsprintf(buf + strlen(buf), fmt, args);
    va_end(args);

    return buf;
}
#endif /* NeXT */

/* 
 *  dropcopy:   Make a temporary copy of the user's mail drop and 
 *  save a stream pointer for it.
 */

pop_dropcopy(p,pwp)
POP     *   p;
struct passwd	*	pwp;
{
    int                     mfd;                    /*  File descriptor for 
                                                        the user's maildrop */
    int                     dfd;                    /*  File descriptor for 
                                                        the SERVER maildrop */
    FILE		    *tf;		    /*  The temp file */
    char		    template[POP_TMPSIZE];  /*  Temp name holder */
    char                    buffer[BUFSIZ];         /*  Read buffer */
    long                    offset;                 /*  Old/New boundary */
    int                     nchar;                  /*  Bytes written/read */
    struct stat             mybuf;                  /*  For lstat() */
#ifdef NeXT
    char		    lockfile[MAXDROPLEN];   /*  Lock file name */
    int			    lfd;		    /*  Fd for lock file */
#endif /* NeXT */

    /*  Create a temporary maildrop into which to copy the updated maildrop */
#ifdef NeXT
    (void)pop_mailfile(p->temp_drop, POP_DROPFILE, p->user);
#else
    (void)sprintf(p->temp_drop,POP_DROP,p->user);
#endif /* !NeXT */

#ifdef DEBUG
    if(p->debug)
        pop_log(p,POP_DEBUG,"Creating temporary maildrop '%s'",
            p->temp_drop);
#endif DEBUG

    /* Here we work to make sure the user doesn't cause us to remove or
     * write over existing files by limiting how much work we do while
     * running as root.
     */

    /* First create a unique file.  Would prefer mkstemp, but Ultrix...*/
#ifdef NeXT
    (void)pop_mailfile(template,POP_TMPFILE);
#else
    strcpy(template,POP_TMPDROP);
#endif /* !NeXT */
    (void) mktemp(template);
    if ( (tf=fopen(template,"w+")) == NULL ) {	/* failure, bail out	*/
        pop_log(p,POP_PRIORITY,
            "Unable to create temporary temporary maildrop '%s': %s",template,
                (errno < sys_nerr) ? sys_errlist[errno] : "") ;
        return pop_msg(p,POP_FAILURE,
		"System error, can't create temporary file.");
    }

    /* Now give this file to the user	*/
#ifndef NeXT
    (void) chown(template,pwp->pw_uid, pwp->pw_gid);
#endif /* !NeXT */
    (void) chmod(template,0600);

    /* Now link this file to the temporary maildrop.  If this fails it
     * is probably because the temporary maildrop already exists.  If so,
     * this is ok.  We can just go on our way, because by the time we try
     * to write into the file we will be running as the user.
     */
    (void) link(template,p->temp_drop);
    (void) fclose(tf);
    (void) unlink(template);

#ifndef NeXT
    /* Now we run as the user. */
    (void) setuid(pwp->pw_uid);
    (void) setgid(pwp->pw_gid);
#endif /* !NeXT */

#ifdef DEBUG
    if(p->debug)pop_log(p,POP_DEBUG,"uid = %d, gid = %d",getuid(),getgid());
#endif DEBUG

    /* Open for append,  this solves the crash recovery problem */
    if ((dfd = open(p->temp_drop,O_RDWR|O_APPEND|O_CREAT,0600)) == -1){
        pop_log(p,POP_PRIORITY,
            "Unable to open temporary maildrop '%s': %s",p->temp_drop,
                (errno < sys_nerr) ? sys_errlist[errno] : "") ;
        return pop_msg(p,POP_FAILURE,
		"System error, can't open temporary file, do you own it?");
    }

    /*  Lock the temporary maildrop */
    if ( flock (dfd, LOCK_EX|LOCK_NB) == -1 ) 
    switch(errno) {
        case EWOULDBLOCK:
            return pop_msg(p,POP_FAILURE,
                 "Maildrop lock busy!  Is another session active?");
            /* NOTREACHED */
        default:
            return pop_msg(p,POP_FAILURE,"flock: '%s': %s", p->temp_drop,
                (errno < sys_nerr) ? sys_errlist[errno] : "");
            /* NOTREACHED */
        }
    
    /* May have grown or shrunk between open and lock! */
    offset = lseek(dfd,0,L_XTND);

    /*  Open the user's maildrop, If this fails,  no harm in assuming empty */
    if ((mfd = open(p->drop_name,O_RDWR)) > 0) {

        /*  Lock the maildrop */
        if (flock (mfd,LOCK_EX) == -1) {
            (void)close(mfd) ;
            return pop_msg(p,POP_FAILURE, "flock: '%s': %s", p->temp_drop,
                (errno < sys_nerr) ? sys_errlist[errno] : "");
        }

#ifdef NeXT
	/* Try to link the temp drop file into the lock file */
	(void)pop_mailfile(lockfile, POP_LOCKFILE, p->user);
	if (link(p->temp_drop, lockfile) != 0) {
	    /* Oops, lock file may already exist! */
	    (void) close(mfd);
	    /* XXX: close(dfd)? */
	    return pop_msg(p, POP_FAILURE, "Can't create lock file \"%s\".",
			   lockfile);
	}
#endif NeXT

        /*  Copy the actual mail drop into the temporary mail drop */
        while ( (nchar=read(mfd,buffer,BUFSIZ)) > 0 )
            if ( nchar != write(dfd,buffer,nchar) ) {
                nchar = -1 ;
                break ;
            }

        if ( nchar != 0 ) {
            /* Error adding new mail.  Truncate to original size,
               and leave the maildrop as is.  The user will not 
               see the new mail until the error goes away.
               Should let them process the current backlog,  in case
               the error is a quota problem requiring deletions! */
            (void)ftruncate(dfd,(int)offset) ;
        } else {
            /* Mail transferred!  Zero the mail drop NOW,  that we
               do not have to do gymnastics to figure out what's new
               and what is old later */
            (void)ftruncate(mfd,0) ;
        }

        /*  Close the actual mail drop */
        (void)close (mfd);

#ifdef NeXT
	/* Get rid of lock file */
	(void) unlink(lockfile);
#endif /* NeXT */
    }

    /*  Acquire a stream pointer for the temporary maildrop */
    if ( (p->drop = fdopen(dfd,"a+")) == NULL ) {
        (void)close(dfd) ;
        return pop_msg(p,POP_FAILURE,"Cannot assign stream for %s",
            p->temp_drop);
    }

    rewind (p->drop);

    return(POP_SUCCESS);
}
