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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#include <stdio.h>

#if defined(hpux)
#include <dce/cma.h>
#include <sys/ioctl.h>
#endif

#if defined(__svr4__) || defined(hpux)
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#endif

#if defined(WIN32)
#include <winnt-pdo.h>
#include <windows.h>
#endif

#ifndef NeXT_PDO
#include <libc.h>
#endif

#ifndef WIN32
#include <sys/signal.h>
#endif WIN32

#include <mach/cthreads.h>
#include <mach/mach.h>
#include <mach/message.h>

#include "debug.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include "nm_init.h"

extern void reinit_network();

#define USAGE	"Usage: nmserver [-f config_file]"


int	debug_flag = 0;
int	local_flag = 0;
int	secure_flag = 0;
int	nevernet_flag = 0;

#ifndef NeXT_PDO
char	*config_file = NULL;
#endif

static void procArgs(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++) {

	/* Add a debugging flag that will avoid detaching from the parent
	 * process .
	 */
	if (strcmp(argv[i], "-d") == 0) {
	    debug_flag = 1;
	}

	/* Add a flag that will refuse all remote name lookup requests.
	 */
	else if (strcmp(argv[i], "-secure") == 0) {
	    secure_flag = 1;
 	}

	/* Add a flag that will refuse requests based on a config file.
	 */
	else if (strcmp(argv[i], "-local") == 0) {
	    local_flag = 1;
	}

	/* Add a flag that will refuse all remote access.
	 */
	else if (strcmp(argv[i], "-nevernet") == 0) {
	    nevernet_flag = 1;
 	}

#ifndef NeXT_PDO
	/* Read the config file. */
	else if ((strcmp(argv[i], "-f") == 0) && ((i + 1) < argc)) {
	    i ++;
	    config_file = argv[i];
	}

	/* Make it possible to start nmserver up without knowing about any
	 * networks (so that it later can be reinitialized using SIGUSR2)
	 */
	else if (strcmp(argv[i], "-nonet") == 0)
	    param.conf_network = FALSE;
#endif

	else {
	    fprintf(stderr, "%s\n", USAGE);
	    (void)fflush(stderr);
	    _exit(-1);
	}
    }
}

#ifndef WIN32

static void parent_terminate()
{
    /* The nmserver is dead -- long live the nmserver! */
    exit(0);
}

/*
 *  Fork a copy of this process -- the child continues the normal flow
 *  of control while the parent sits tight and waits for the child to
 *  either terminate (due to some erroneous condition) or signal it to
 *  release control with a clean exit.  This will enable us to let the
 *  child process initialize nmserver properly before allowing the
 *  parent context (/etc/rc) to continue.	     --Lennart, 930308
 */
static void parent_detach()
{
    extern int errno;
#if defined(__svr4__) || defined(hpux)
    int status;
#else
    union wait status;
#endif
    int pid, fd;

    /* Set up a signal handler to catch SIGHUP as this will be the
     * signal to the parent to release control and terminate.
     */
    signal(SIGHUP, parent_terminate);

    switch ((pid = fork())) {
      case -1:
	/* Error */
	ERROR((msg, "detach_parent: cannot fork -- errno = %d", errno));
	panic("detach_parent: cannot fork");

      case 0:
	/* Child -- close all open file descriptors, detach ourselves
	 * from the parent's controlling tty and all that jazz.
	 */

        /* Child doesn't catch SIGHUP */
        signal(SIGHUP, SIG_IGN);
	//for (fd=getdtablesize(); fd>=0; fd--)
	for (fd=2; fd>=0; fd--)
	    (void) close(fd);

	(void) open("/", 0);
	(void) dup2(0, 1);
	(void) dup2(0, 2);

#if defined(__svr4__) || defined(hpux)
	setpgrp();
#else
	(void) setpgrp(0, getpid());
#endif

#ifndef hpux
	fd = open("/dev/tty", 2);
	if (fd >= 0) {
	    (void) ioctl(fd, (u_long) TIOCNOTTY, NULL);
	    (void) close(fd);
	}
#endif
	
	return;

      default:
        /* Wait for the child to send me a SIGHUP.  If the child
         * exits prematurely, return that exit code instead.
         */
	while (wait(&status) != pid && !WIFSTOPPED(status));
#if defined(__svr4__) || defined(hpux)
	if (WIFEXITED(status))
	    exit(WEXITSTATUS(status));
	else
	    exit(-1);
#else
	exit(WIFEXITED(status) ? status.w_retcode : -1);
#endif
    }
}

static void parent_release()
{
    /* Tell parent to die -- the child is ready to roll! */
    kill(getppid(), SIGHUP);
}

#endif !WIN32


#if !defined(WIN32)

#define is_there_anyone_out_there()

#else /* WIN32 */

extern void is_there_anyone_out_there(void);	// win_daemon.c
extern void other_win32_main (int argc, char *argv[]);
#define main other_win32_main

#endif /* !WIN32 */

/*
 * main
 */
void
main(argc, argv)
int	argc;
char	**argv;
{
    long	clock;

#ifndef WIN32
    signal(SIGHUP, SIG_IGN);
#endif

    is_there_anyone_out_there();

#ifndef NeXT_PDO
    if (!access("/cores/.doNMSCores", F_OK)) {
	    /*
	     * Make sure that we can drop a core file in case of error
	     */
	     struct rlimit rlim;

	    getrlimit(RLIMIT_CORE, &rlim);
	    rlim.rlim_cur = rlim.rlim_max;
	    setrlimit(RLIMIT_CORE, &rlim);
    }
#endif

    procArgs(argc, argv);

#ifndef WIN32
    /* Detach from the parent process unless we're running in debug mode
     */
    if (!debug_flag) {
	parent_detach();
    }

    /*
     * Initialize syslog if appropriate.
     */
    if (param.syslog) {
#ifdef LOG_REMOTEAUTH
	openlog("netmsgserver", LOG_PID | LOG_NOWAIT, LOG_REMOTEAUTH);
#else // LOG_REMOTEAUTH
	openlog("netmsgserver", LOG_PID | LOG_NOWAIT, LOG_USER);
#endif // LOG_REMOTEAUTH
    }

    /*
     * Put out a timestamp when starting
     */
    clock = time(0);
    fprintf(stdout, "Started %s at %24.24s \n", argv[0], ctime(&clock));
    if (param.syslog)
	syslog(LOG_INFO, "Started %s at %24.24s ", argv[0], ctime(&clock));
#endif !WIN32

    (void)fflush(stdout);


    if (nm_init()) {
	ERROR((msg,"Network Server initialised."));

#ifndef NeXT_PDO
	/* Continue up above -- we're ready to receive requests & USR2's */
	signal(SIGUSR2, reinit_network);
#endif

#ifndef WIN32
	if (!debug_flag)
	    parent_release();
#endif
    } else {
	panic("Network Server initialisation failed.");
    }

    cthread_exit(0);
}

#ifdef WIN32

MS_BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;

/*
 * ServiceStart for windowsNT the main is in win_daemon.c
 */
void
ServiceStart(int argc, char *argv[])
{
    procArgs(argc, argv);

    // report the status to the service control manager.
    (void) ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 30000);

    if (nm_init()) {
	ERROR((msg,"Network Server initialised."));
    } else {
	panic("Network Server initialisation failed.");
    }

    // report the status to the service control manager.
    (void) ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000);

    hServerStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hServerStopEvent)
	exit(1);

    // report the status to the service control manager.
    (void) ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0);

    WaitForSingleObject(hServerStopEvent, INFINITE);
}

VOID ServiceStop()
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
}

#endif /* WIN32 */
