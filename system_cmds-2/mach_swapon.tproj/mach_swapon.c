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
 * mach_swapon
 *
 * Copyright (c) 1989 by NeXT, Inc.
 *
 **********************************************************************
 * HISTORY
 *
 * 26-Jun-91  Bradley Taylor (btaylor) at NeXT
 *	Support for compression option
 *
 * 27-Feb-89  Peter King (king) at NeXT
 *	Created.
 *
 **********************************************************************
 */

/*
 * Include files.
 */
#include <c.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bsd/sys/mach_swapon.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "swent.h"

/*
 * Global variables.
 */
char	*program;			/* The name of this program */
bool	verbose = FALSE;		/* Flag whether to be noisy or not */


/*
 * Function prototypes.
 */
int	autoswap(char *swaptab);
int	call_swapon(swapent_t sw);

/*
 * Main program
 */
main(int argc, char *argv[])
{
	char		*cp;		/* generic character pointer */
	struct swapent	swapent;	/* command line swapent */
	bool		automode = FALSE;
	char		*swaptab = NULL;

	/*
	 * Parse arguments.
	 */
	program = *argv++;
	argc--;
	if (argc <= 0) {
		goto usage;
	}
	while (argc != 0) {
		cp = *argv;
		if (*cp == '-') {
			while (*++cp != '\0') {
				switch (*cp) {
				    case 'a':
					automode = TRUE;
					break;

				    case 'v':
					verbose = TRUE;
					break;

				    case 'o':
					if (argc < 3) {
						/*
						 * Gotta at least have
						 * "-o options filename"
						 */
						goto usage;
					}
					argv++;
					argc--;
					swent_parseopts(*argv, &swapent);
					break;

				    case 'f':
					argv++;
					argc--;
					swaptab = *argv;
					break;

				    default:
					goto usage;
					break;
				}
			}
			argv++;
			argc--;
		} else {
			break;
		}
	}
	/*
	 * Make sure we don't have a filename if we are in automode
	 * and vice versa.
	 */
	if (argc != 0) {
		if (automode) {
			goto usage;
		}
		swapent.sw_file = *argv;
	} else {
		if (!automode) {
			goto usage;
		}
	}

	if (automode == TRUE) {
		if (autoswap(swaptab) != 0) {
			exit(1);
		}
	} else {
		if (call_swapon(&swapent) != 0) {
			exit(1);
		}
	}

	exit(0);
usage:
	fprintf(stderr, "usage: %s [-v] [-o options] filename\n", program);
	fprintf(stderr, "       %s [-v] [-f swaptab] -a\n", program);
	exit(1);
}


/*
 * Routine: autoswap
 * Function:
 *	Parse the configuration file and start swapping on each file
 *	listed in it, unless, of course, it has the "noauto" option
 *	set.
 */
int
autoswap(char *swaptab)
{
	swapent_t	sw;

	/*
	 * Set up to read swap entries.
	 */
	if (swent_start(swaptab) != 0) {
		return (-1);
	}

	/*
	 * Call mach_swapon on the file if it is not "noauto" [sic].
	 */
	while ((sw = swent_get()) != NULL) {
		if (sw->sw_noauto == FALSE &&
		    call_swapon(sw) != 0) {
			swent_rele(sw);
			swent_end();
			return (-1);
		}
		swent_rele(sw);
	}

	/*
	 * Close down the swaptab file.
	 */
	swent_end();

	return (0);
}


/*
 * Routine: setup_for_compression
 * Function:
 *	Mount the swapfs filesystem with desired compression options
 */
int
setup_for_compression(swapent_t sw)
{
	static const char FRONT_SUFFIX[] = ".front";
	char *front;
	struct stat st;
	union wait status;
	int pid;

	if (sw->sw_lowat > 0) {
		/*
		 * XXX: handle lowat stuff ourselves, since the swapfile front
		 * does not report the true size of the swapfile.
		 */
		if (stat(sw->sw_file, &st) < 0) {
			perror("stat");
		} else {
			if (st.st_size > sw->sw_lowat) {
				if (truncate(sw->sw_file, sw->sw_lowat) < 0) {
					perror("truncate");
				}
			}
		}
	}

	/*
	 * Mount compressed swapfile
	 */
	front = malloc(strlen(sw->sw_file) + strlen(FRONT_SUFFIX) + 1);
	sprintf(front, "%s%s", sw->sw_file, FRONT_SUFFIX);
	switch (pid = fork()) {
	case 0:
		execl("/usr/etc/mount", "mount", "-t", "swapfs", 
		      sw->sw_file, front, 0);
		_exit(1);
	case -1:
		perror("fork");
		return (0);
	default:
		break;
	}
	
	if (wait4(pid, &status, 0, 0) < 0) {
		perror("wait4");
		return (0);
	}

	if (status.w_retcode != 0) {
		/*
		 * Child process failed: has already printed why
		 */
		return (0);
	}
		
	/*
	 * Success: compressed filename replaces real swapfile name
	 */
	sw->sw_file = front;
	return (1);
}


/*
 * Routine: call_swapon
 * Function:
 *	Call mach_swapon given the information in the swapent "sw".
 */
int
call_swapon(swapent_t sw)
{
	int error;
	
#if !hppa
	if (!sw->sw_nocompress) {
		setup_for_compression(sw);
	}
#endif hppa
	error = mach_swapon(sw->sw_file,
			    sw->sw_prefer ? MS_PREFER : 0,
			    sw->sw_lowat,
			    sw->sw_hiwat);
	if (error) {
		fprintf(stderr, "%s: mach_swapon failed: %s\n",
			program, strerror(error));
		return (-1);
	} else if (verbose) {
		printf("%s: swapping on %s\n", program, sw->sw_file);
	}

	return (0);
}

