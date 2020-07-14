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
 * forkexec.c
 *
 * forkexec()
 * - fork and exec the binary, returning the status and the output
 *   from the binary
 *
 * - creates a shared pipe using the usual UNIX methods
 *
 * Modification History:
 * 
 * Fri Aug  1 11:09:22 PDT 1997		Dieter Siegmund (dieter@apple.com)
 * - created
 */
#import <stdio.h>
#import <errno.h>
#import <sys/types.h>
#import <sys/wait.h>
#import <sys/uio.h>
#import <unistd.h>
#import <stdlib.h>
#import <stdarg.h>

static int verbose = 1;

static void
report_error(char * msg)
{
    if (verbose) {
	perror(msg);
    }
}

#define PIPEFULL	(4 * 1024)

#include "forkexec.h"

unsigned char *
forkexec(char * progname, char * argv[], int * status_p, int * n_bytes)
{
    int fdp[2];
    int pid;
    int i;
    char * buf;
    int where;

    if (pipe(fdp) == -1) {
	report_error("forkexec(): fork failed");
	return (NULL);
    }

    pid = fork();
    switch (pid) {
      case -1:
	break;

      case 0: /* child */
	/* make stdout/stderr point to the write pipe */
	close(1); 	/* close stdout */
	dup(fdp[1]);	/* stdout becomes write pipe */
	close(2); 	/* close stderr */
	dup(fdp[1]);	/* stderr becomes write pipe */
	close(fdp[0]);	/* close read end of pipe */
	close(0); 	/* close stdin */
	execv(progname, argv);
	printf("forkexec(): could not exec '%s'\n", progname);
	exit(-1);

      default: /* parent */
	close(fdp[1]); /* we don't use the write end */
	buf = malloc(PIPEFULL); /* a pipe buffer full */
	if (!buf) {
	    fprintf(stderr, "forkexec(): malloc failed\n");
	    break;
	}
	where = 0;
	while (where < (PIPEFULL - 1)
	       && (i = read(fdp[0], buf + where, PIPEFULL - 1 - where))) {
	    where += i;
	}
	buf[where] = 0;
	waitpid(pid, status_p, 0);
	*n_bytes = where;
	return (buf);
    }
    return (NULL);
}

#ifdef TESTING_FORKEXEC
main()
{
    char * nargv[] = { "/usr/sbin/pdisk", "/dev/rsd0h", "-dis", 0 };
    char * results;
    int status;
    int n_bytes;

    results = forkexec(nargv[0], nargv, &status, &n_bytes);
    if (WIFEXITED(status)) {
	printf("status was 0x%x, 0x%x\n", status, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
	printf("signalled, status was 0x%x\n", status);
    }
    printf("\nparent got '%d' bytes '%s'", n_bytes, results);
    exit (0);

}
#endif TESTING_FORKEXEC
