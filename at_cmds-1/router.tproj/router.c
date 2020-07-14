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
 *	Copyright (c) 1988-91 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)router.c: 1.0, 1.25; 5/1/942; Copyright 1988-89, Apple Computer, Inc.";
#endif  /* lint */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/signal.h>

#include <h/at-config.h>
#include <h/localglue.h> 
#include <at/appletalk.h>
#include <h/debug.h>
#include <h/llap.h>
#include <h/lap.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/nbp.h>
#include <LibcAT/at_paths.h>
#include <at/atp.h>
#include <at/asp_errno.h>
#include <LibcAT/at_proto.h>

/*#define MSGSTR(num,str)		catgets(catd, MS_SBIN, num,str)*/
#define MSGSTR(num,str)			str

#define TRUE	1

#define ZONENAME_SIZE		32
#define ZONENAME_OFFSET		48
#define ADDRESS_OFFSET		16
#define SPCONFIG_OFFSET		0

/* board ids of ethernet cards */
#define	ATOL(str)	strtol(str, (char **) NULL, 0)

static char	*usage = "\
usage: %s  -d|u [options] \n\
	-a check state\n\
	-c check config file only\n\
	-d shutdown router\n\
	-e check & display configuration only\n\
	-f router config file\n\
	-g disable ATP protocol\n\
	-h disable ADSP protocol\n\
	-m <value> maximum routing (pack/sec)\n\
	-r show routing table\n\
	-s show statistics\n\
	-u bring up router\n\
	-v <value> maximum routing table entries\n\
	-w <value> maximum zip table entries\n\
	-q run quiet, don't ask for zones\n\
	-x run in multihoming mode\n\
	-z show zone list\n";

extern int	optmaybe;
extern char	*optarg;
extern char version[];				/* maintained in lap_init.c */
static char	*at_interface = NULL;
static char	*et_interface = AT_DEF_ET_INTERFACE;
static char	*progname = NULL;
static int	atp_on = 1;
static int	adsp_on = 1;

static FILE *STDOUT = stdout;

int do_init (struct atalk_options *);
int register_this_node(short);
void do_shutdown(char *);
void down_error(int *);

int     lap_shutdown(char *, struct atalk_options *, char **),
	lap_init (char *, struct atalk_options *, int, int, char **),
	showRouterStats(), 
  	showRoutes(),
  	showZones(),
	setDefaultZones(int, int),
  	changeRoutingMix(short),
	atalkState(char, char **);
     

int quiet=0;		/* temp debug */
int
main(argc, argv)
int	argc;
char	*argv[];
{
	int *pid_buf;
	int	opterr;
	int	ch;
	int	d_option = 0;
	int	u_option = 0;
	int	s_option = 0;
	int	no_option = 1;
	int	z_option = 0;
	int	r_option = 0;
	int	m_option = 0;
	int	a_option = 0;
	int	v_option = 0;
	int	w_option = 0;
/*	int	c_option = 0;  not used */
	int	b_option = 0;
	long router_mix = 0;
	static struct atalk_options opt;
/*	char	*range;  not used */

	if (getenv("_SMIT_VERBOSE_FLAG") == NULL)  {
		STDOUT = stdout;
	}
	else
		STDOUT = stderr; 	/* in smit, route all output to stderr */
	opterr = argc < 2;
/*
	printf("%s\n",version);
*/

	/* process the arguments */
	while (!opterr && (ch = getopt(argc, argv, "f:ab:ceqduszrtghxm:v:w:")) != EOF) {
		no_option = 0;
		switch (ch) {


			/* option '-g': ATP off */
		case 'g':
			atp_on = 0; 
			break;

			/* option '-h': ADSP off */
		case 'h':
			adsp_on = 0; 
			break;

			/* option '-u': start the network up */
		case 'u':
			u_option++; 
			break;

			/* option '-d': shut the network down */
		case 'd':
			d_option++; 
			break;

			/* option '-s': print statistics and error counts */
		case 's':
			s_option++; 
			break;

			/* option '-z': show zone table */
		case 'z':
			z_option++; 
			break;

			/* option '-r': show routing table */			
		case 'r':
			r_option++; 
			break;

			/* option '-e': display config only */
		case 'e':
			opt.e_option =1;
			break;

			/* option '-f': specify router config file */ 
		case 'f':
			opt.f_option = optarg;
			break;

			/* option '-c': test config file only */
		case 'c':
			opt.c_option = TRUE;
			break;

			/* option '-m': mix of routing vs home stack selection*/
		case 'm': 
			 m_option++;
			 router_mix = ATOL(optarg) & 0xFFFF;
			 break;

		case 'a': 
			 a_option =TRUE;
			 break;

			/* option '-v': routing table size*/
		case 'v': 
			 v_option++;
			 opt.r_table = ATOL(optarg) & 0xFFFF;
			 break;

			/* option '-w': zip table size*/
		case 'w': 
			 w_option++;
			 opt.z_table = ATOL(optarg) & 0xFFFF;
			 break;

			/* option '-x': multihoming mode of router
			   (-z 'cause -m & -h were taken) */
		case 'x': 
			opt.h_option++;
			break;
			/* run quiet, don't ask for zones */
		case 'q':
			opt.q_option++;
			break;
	
		case 'b':
			b_option++;
			quiet = atoi(optarg);
			break;
		default:
			opterr++; 
			break;
		}
	}/* of while */

	if (b_option) {
		setDefaultZones(opt.h_option ? AT_ST_MULTIHOME :
			AT_ST_ROUTER, quiet);
		exit(0);
	}
	if (a_option) {
		atalkState(0, (char **)&pid_buf);
		if (pid_buf) {
			fprintf(STDOUT, MSGSTR(M_PROCS,
				"The following processes are using AppleTalk Services:\n"));
			while (*pid_buf != 0) {
				fprintf(STDOUT, "  %d\n", *pid_buf);
					pid_buf++;
			}
			exit(EBUSY);
		}
		exit(0);
	}

	if (u_option || d_option || z_option ) {
/*	  if (getuid () != 0) { */	/* temp */
	  if (0) {
	    fprintf (stderr, MSGSTR(M_PERM,
			"%s: Permission denied; must be super-user.\n"), progname);
	    opterr++;
	  }
	}

	if (u_option && d_option) {
		fprintf(stderr, MSGSTR(M_U_D_OPT,
			"%s: -u and -d options are incompatible\n"), progname);
		opterr++;
	}



	if (opterr || no_option) {
		exit(1);
	}


	if (s_option) {
		showRouterStats();
	}

	if (r_option) {
		showRoutes();
	}

	if (z_option) {
		showZones();
	}

	if (m_option) {
		changeRoutingMix((short)router_mix);
	}
	
	if (u_option || opt.c_option || opt.e_option) {
		if ( !(opt.c_option || opt.e_option) )
			kernelDbg(DBG_SET_FROM_FILE,0);
		if (do_init(&opt) == -1) {
			if (errno == ENOENT)
	  			do_shutdown(et_interface);
			exit (1);
      	}
		if ( !(opt.c_option || opt.e_option) )
			showRouterStats();
	}
	else if (d_option)
	  do_shutdown(et_interface);
	exit (0);
}


int do_init (opt)
struct atalk_options *opt;
{
	int *pid_buf;
	int	status;
/*	int	interactive = 1;  not used */
/*	char *devname[33];  not used */

	opt->router = TRUE;			/* set router flag */
	status = lap_init(et_interface, opt, atp_on,adsp_on, (char **)&pid_buf);
	if (opt->c_option || opt->e_option)
		return(status);
	if (status < 0) {
		switch (errno) {

		case 0:				/* error msg was already displayed, do nothing*/
			break;
		case EBUSY :
			down_error(pid_buf);
			break;
		case EEXIST :
			fprintf(stderr, MSGSTR(M_PORT_USED,
				"%s: another home port already designated\n"), progname);
			break;
		case EACCES :
			fprintf(stderr, MSGSTR(M_PERM, 
				"%s: permission denied\n"), progname);
			break;
		case EPERM :
			fprintf(stderr, MSGSTR(M_PORT_UP, 
				"%s: port already up, can't designate as home port\n"), progname);
			break;
		case EINVAL :
			fprintf(stderr, MSGSTR(M_BAD_IF,
				"%s: invalid interface specified\n"), progname);
			break;
		case EFAULT :
			fprintf(stderr, MSGSTR(M_IF_STATE,
				"%s: can't change range in current i/f state\n"), progname);
			break;
		case ECHRNG :
			fprintf(stderr, MSGSTR(M_PKT_REG_ERR,
				"%s: error registering packet type\n"), progname);
			break;
		case EALREADY :
			{
			fprintf(stderr, MSGSTR(M_IF_RUNNING,
				"%s: interface %s is already running\n"),progname, et_interface);
			}
			break;
		case ENOENT:
			break;
		default :
			fprintf(stderr, "%s: %s (%d)\n", progname, sys_errlist [errno], errno);
			break;
		}
		return (-1);
	}

	/*### LD 11/30/94 Add entity registration on the net */
	register_this_node( opt->h_option);
	return (0);
}

void do_shutdown (hw_interface)
	char *hw_interface;
{
	int *pid_buf;

	if (checkATStack() != RUNNING) {
		fprintf(stderr, 
			MSGSTR(M_NOT_LOADED,
			       "The AppleTalk stack is not running.\n"));
	}
	else {
	  	if (lap_shutdown(hw_interface, 0, (char **)&pid_buf) < 0) {
		  	if (errno == EBUSY)
			      down_error(pid_buf);
			else if (errno == EACCES)
			      fprintf(stderr, 
				      MSGSTR(M_PERM,"%s: permission denied\n"),
				      progname);
			else
			      fprintf(stderr, 
				      "%s: error: %s\n",
				      progname, sys_errlist[errno]);
			exit(1);
		}
	}
} /* do_shutdown */

void
down_error(pid_buf)
	int *pid_buf;
{
	/* pid_buf contains ids of processes still holding on to resources */
	fprintf(stderr,
		MSGSTR(M_PROC_ERR,"Cannot bring down AppleTalk.\n"\
		"You must first terminate the following processes:\n"));
	while (*pid_buf != 0) {
		fprintf(stderr, "  %d\n", *pid_buf);
		pid_buf++;
	}
}

int	register_this_node(mhome)
short mhome;		/* if true, we're in multihoming mode */
{
	char		hostname[128];
	char		verstring[64];
	struct utsname	u_name;
	at_inet_t	addr;
	at_entity_t	name;

	if (uname(&u_name) < 0)
		return (-1);
	if (rtmp_netinfo(-1, &addr, NULL) != 0) {
		printf(MSGSTR(M_NETINFO, "reg_this_node: rtmp_netinfo failed\n"));
		return(-1);
	}
	addr.socket = DDP_SOCKET_1st_DYNAMIC;
	strcpy(hostname, u_name.sysname);
	if (mhome)
		sprintf(verstring, " PPC-H v%d.%d", AT_VERSION_MAJOR, AT_VERSION_MINOR);
	else
		sprintf(verstring, " PPC-R v%d.%d", AT_VERSION_MAJOR, AT_VERSION_MINOR);
	strcat(hostname, verstring);
	nbp_make_entity(&name, u_name.nodename, hostname, "*");
	if (_nbp_send_(NBP_REGISTER, &addr, &name, NULL, 1, NULL) <= 0) {
		printf(MSGSTR(M_NBP_SND, "reg_this_node:_nbp_send failed\n"));
		return (-1);
	}

	return (0);
}
