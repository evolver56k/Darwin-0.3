/* 
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * Copyright (c) 1980, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 *
 *	@(#)proc.h	8.1 (Berkeley) 5/31/93
 */

/*
 * Structure for each process the shell knows about:
 *	allocated and filled by pcreate.
 *	flushed by pflush; freeing always happens at top level
 *	    so the interrupt level has less to worry about.
 *	processes are related to "friends" when in a pipeline;
 *	    p_friends links makes a circular list of such jobs
 */
struct process {
    struct process *p_next;	/* next in global "proclist" */
    struct process *p_friends;	/* next in job list (or self) */
    struct directory *p_cwd;	/* cwd of the job (only in head) */
    short unsigned p_flags;	/* various job status flags */
    char    p_reason;		/* reason for entering this state */
    int     p_index;		/* shorthand job index */
    int     p_pid;
    int     p_jobid;		/* pid of job leader */
    /* if a job is stopped/background p_jobid gives its pgrp */
    struct timeval p_btime;	/* begin time */
    struct timeval p_etime;	/* end time */
    struct rusage p_rusage;
    Char   *p_command;		/* first PMAXLEN chars of command */
};

/* flag values for p_flags */
#define	PRUNNING	(1<<0)	/* running */
#define	PSTOPPED	(1<<1)	/* stopped */
#define	PNEXITED	(1<<2)	/* normally exited */
#define	PAEXITED	(1<<3)	/* abnormally exited */
#define	PSIGNALED	(1<<4)	/* terminated by a signal != SIGINT */

#define	PALLSTATES	(PRUNNING|PSTOPPED|PNEXITED|PAEXITED|PSIGNALED|PINTERRUPTED)
#define	PNOTIFY		(1<<5)	/* notify async when done */
#define	PTIME		(1<<6)	/* job times should be printed */
#define	PAWAITED	(1<<7)	/* top level is waiting for it */
#define	PFOREGND	(1<<8)	/* started in shells pgrp */
#define	PDUMPED		(1<<9)	/* process dumped core */
#define	PERR		(1<<10)	/* diagnostic output also piped out */
#define	PPOU		(1<<11)	/* piped output */
#define	PREPORTED	(1<<12)	/* status has been reported */
#define	PINTERRUPTED	(1<<13)	/* job stopped via interrupt signal */
#define	PPTIME		(1<<14)	/* time individual process */
#define	PNEEDNOTE	(1<<15)	/* notify as soon as practical */

#define	PMAXLEN		80

/* defines for arguments to pprint */
#define	NUMBER		01
#define	NAME		02
#define	REASON		04
#define	AMPERSAND	010
#define	FANCY		020
#define	SHELLDIR	040	/* print shell's dir if not the same */
#define	JOBDIR		0100	/* print job's dir if not the same */
#define	AREASON		0200

extern struct process proclist;	/* list head of all processes */
extern bool    pnoprocesses;	/* pchild found nothing to wait for */

extern struct process *pholdjob;/* one level stack of current jobs */

extern struct process *pcurrjob;/* current job */
extern struct process *pcurrent;/* current job in table */
extern struct process *pprevious;/* previous job in table */

extern int    pmaxindex;	/* current maximum job index */
