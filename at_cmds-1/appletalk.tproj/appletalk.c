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
#include <at/atp.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/nbp.h>  
#include <at/asp_errno.h>
#include <h/llap.h> 
#include <h/lap.h>  
#include <h/debug.h>
#include <LibcAT/at_proto.h>
#include <LibcAT/at_paths.h>

#include <varargs.h>

/*#define MSGSTR(num,str)		catgets(catd, MS_SBIN, num,str)*/
#define MSGSTR(num,str)         str

#define ZONENAME_SIZE		32
#define ZONENAME_OFFSET		48
#define ADDRESS_OFFSET		16
#define INTERFACE_OFFSET	1
#define INTERFACE_SIZE		4
#define SPCONFIG_OFFSET		0  	/* not used */
#define MAX_ZONES		50	/* max cfg file zone count */

/* version checking */
#define MIN_MAJOR       0                   /* minimum required
											   version for */
#define MIN_MINOR       8                   /* advanced features */

#define   ATOL(str)       strtol(str, (char **) NULL, 0)

static char *usage = 
"  Startup in single port mode:\n\
	-u bring up in single-interface mode\n\
    modifiers:\n\
	-b specify the ethernet interface to use\n\
	-q don't ask for zones (non-interactive mode)\n\
  Startup for multiple ports:\n\
	-r bring up Appletalk in routing mode\n\
	-x bring up Appletalk in multihoming mode\n\
    modifiers:\n\
	-f <router config file>\n\
	-c check config file only\n\
	-e check & display configuration only\n\
	-q don't ask for zones (non-interactive mode)\n\
	-v <value> maximum routing table entries\n\
	-w <value> maximum zip table entries\n\
  Other commands:\n\
	-a check state\n\
	-d shut down AppleTalk\n\
	-n print network number and node id\n\
	-p print saved PRAM AppleTalk information\n\
	-s show statistics & error counts\n\
  Other routing commands:\n\
	-j print router stats\n\
	-m <value> maximum routing (pack/sec)\n\
	-t show routing table\n\
	-z show zone list\n\
" ;

/* These two flags apply to streams modules only, and are 
   not supported in Rhapsody:
	-g disable ATP protocol
	-h disable ADSP protocol
*/
extern char	*optarg;

static char	*at_interface = NULL;
static char	*et_interface = AT_DEF_ET_INTERFACE;
static char	*progname = NULL;
static int	header_already_printed = 0;
static int	router = 0;
static at_elap_cfg_t elapcfg[IF_TOTAL_MAX];		
static if_zone_info_t if_zones[MAX_ZONES]; 	/* zone info from cfg file */
static if_cfg_t filecfg;

#ifdef NOT_SUPPORTED
static int	atp_on = 1;
static int	adsp_on = 1;
#endif NOT_SUPPORTED

/* XXX extern char *lap_default (); */

int     routerShutdown(),
	routerStartup(struct atalk_options *opt, at_elap_cfg_t elapcfg[],
		      if_zone_info_t if_zones[], if_cfg_t *filecfgp, int *homeseed),
	elap_init(char *, struct atalk_options *, char **),
	showRouterStats(), 
  	showRoutes(),
  	showZones(),
	setDefaultZones(int, int),
  	changeRoutingMix(short),
	atalkState(char, char **);
     
int do_init(struct atalk_options *opt);
	  
static void	print_nodeid(void);
static void	print_header(char *);
static void	print_pram_info(void);
static void	print_statistics(int);
static void	lookup_if(int);
static void	do_shutdown();
static void	down_error(int *);
static int	register_this_node(void);

static FILE *STDOUT = stdout;

#define TRUE 1
#define FALSE 0

int
main(argc, argv)
int	argc;
char	*argv[];
{
	int *pid_buf;
	int	opterr;
	int	ch;

	int	no_option = 1,
	 	d_option = 0,
	   	r_option = 0,
		j_option = 0,
		v_option = 0,
	  	w_option = 0,
	  	z_option = 0,
		t_option = 0,
	  	m_option = 0,
		u_option = 0,
		b_option = 0,
		n_option = 0,
		s_option = 0,
		p_option = 0,
		a_option = 0;

	long router_mix = 0;

	struct atalk_options opt;
	int	retval = 0;		/* return value (for -t opt) */

 	opt.f_option = NULL;
	opt.h_option = FALSE;
	opt.q_option = FALSE;		/* default, ask for zones on startup */

	memset(&opt,'\0',sizeof(opt));

	/* find out what this program is called and other miscellaneous stuff */
	progname = argv[0];

	/* check whether at least one argument was provided */
	opterr = argc < 2;

	/* process the arguments */
	while (!opterr && (ch = getopt(argc, argv, "ab:cdef:jm:npqrstuv:w:xz")) != EOF) {
		no_option = 0;
		switch (ch) {
#ifdef NOT_SUPPORTED
			/* option '-g': ATP off */
		case 'g':
		  	atp_on = 0;
			break;

			/* option '-h': ADSP off */
		case 'h':
		  	adsp_on = 0;
			break;
#endif NOT_SUPPORTED

			/* option '-j': print router stats */
		case 'j':
			j_option++; 
			break;

			/* option '-n': print current & initial at_interface address */
		case 'n':
			n_option++; 
			break;

			/* option '-u': start the network up */
		case 'u':
			u_option++; 
			break;
			/* option '-q': run quiet, don't ask for zones */
		case 'q':
			opt.q_option = TRUE;
			break;

		case 'b':
			b_option++;
			et_interface = optarg;
			break;

			/* option '-d': stop AppleTalk stack */
		case 'd':
			d_option++; 
			break;

			/* option '-s': print statistics and error counts */
		case 's':
			s_option++; 
			break;
		case 'p':
			p_option++;
			break;

		case 'a':
			a_option++;
			break;

		/* router command only */
			/* option '-r': start the network up - router mode*/
		case 'r':
			r_option++; 
			break;

			/* option '-x': start the network up - multihoming*/
		case 'x':
			opt.h_option = TRUE; 
			break;

			/* option '-e': display config only */
		case 'e':
			opt.e_option = TRUE;
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

		case 'v': 
			 v_option++;
			 opt.r_table = ATOL(optarg) & 0xFFFF;
			 break;

			/* option '-w': zip table size*/
		case 'w': 
			 w_option++;
			 opt.z_table = ATOL(optarg) & 0xFFFF;
			 break;

			 /* option '-t' display routing table */
		case 't': 
			 t_option++;
			 break;

			 /* option '-z' display zone table */
		case 'z': 
			 z_option++;
			 break;

		default:
			opterr++; 
			break;
		}
	}/* of while */

	if (a_option) {
		atalkState(0, (char **)&pid_buf);
		if (pid_buf) {
			fprintf(STDOUT, MSGSTR(M_PROCS,
				"The following processes are using AppleTalk services:\n"));
			while (*pid_buf != 0) {
				fprintf(STDOUT, "  %d\n", *pid_buf);
					pid_buf++;
			}
			exit(EBUSY);
		}
		else
		  fprintf(STDOUT, "No processes are currently using AppleTalk services.\n");
		exit(0);
	}

	if (u_option || b_option || d_option || r_option || opt.h_option) {
	  if (getuid () != 0) {
	    fprintf (stderr, MSGSTR(M_MUST_BE_SU,
			"%s: Permission denied; must be super-user.\n"), progname);
	    exit(1);
	  }
	}

	if ((u_option + r_option + opt.h_option) > 1) {
	    fprintf(stderr, "%s: Only one of [-u -r -x] may be used  at the same time.\n",
		    progname);
	    opterr++;
	}

	if (u_option || r_option || opt.h_option) {
	  if (d_option) {
	    fprintf(stderr, "%s: -%c and -d options are incompatible\n",
		    progname, (r_option)? 'r': (opt.h_option)? 'x' : 'u');
	    opterr++;
	  }
	  if (u_option && (opt.c_option || opt.e_option || opt.f_option || v_option || w_option)) {
	    fprintf (stderr, "%s: -c, -e, -f, -v and -w can only be used with -r or -x option\n",
		     progname);
	    opterr++;
	  }
	}
	else {
#ifdef NOT_SUPPORTED
	    if (!atp_on || !adsp_on || opt.q_option) {
		fprintf (stderr, 
			 "%s: -g, -h, and -q can only be used with -u, -r, or -x option\n",
			 progname);
#else
	    if (opt.q_option) {
		fprintf (stderr, "%s: -q can only be used with -u, -r, or -x option\n",
			 progname);
#endif
		opterr++;
	    }
	    if (opt.c_option || opt.e_option || opt.f_option || v_option || w_option) {
		fprintf (stderr, "%s: -c, -e, -f, -v and -w can only be used with -r or -x option\n",
			 progname);
		opterr++;
	    }
	}

	if (b_option && !u_option) {
		fprintf (stderr, "%s: -b can only be used with -u option\n",
			 progname);
		opterr++;
	}

	if (opterr || no_option) {
		printf(usage, progname);
		exit(1);
	}

	lookup_if(b_option);
	if ((strncmp(at_interface, "ethertalk", strlen("ethertalk")) != 0) && 
	    (b_option)) {
		/* the at_interface is not ethertalk, and the user is
		 * trying to set net# or zone name OR is trying to say which
		 * hardware at_interface to use ... not allowed!
		 */
		fprintf(stderr, MSGSTR(M_B_OPT,
			"%s: option -b applicable only to ethertalk\n"), progname);
		exit(1);
	}
	
	if (u_option || r_option || opt.h_option) {
	    if (checkATStack() == RUNNING) {
	        fprintf(stderr,"The AppleTalk stack is already running.\n");
	    }
	    else {
		if (r_option || opt.h_option) {
			router = TRUE;
			if (!opt.f_option) /* alternate cfg file specified? */
			   if (opt.h_option)
			      opt.f_option = MH_CFG_FILE;
			   else
			      opt.f_option = AT_CFG_FILE;

			/* does cfg file exist? */
			if (access(opt.f_option,0)) {
				fprintf(stderr,
					MSGSTR(M_CFG_FILE,
					       "Error, configuration file %s not found\n"),
					opt.f_option);
				exit(1);
			}
		}
	        if (do_init(&opt) == -1) {
		    exit(1);
		}
	    }
	}
	if (checkATStack() == RUNNING) {
	    /* do the printing after it's been started, but before it's stopped */
	    if (n_option)
		print_nodeid();
	    if (s_option)
		print_statistics(p_option);
	    else
	       if (p_option)
		  print_pram_info();
	    if (t_option) {
		showRoutes();
	    }
	    if (z_option) {
		showZones();
	    }
	    if (j_option) {
		showRouterStats();
	    }
	    if (m_option) {
		changeRoutingMix((short)router_mix);
	    }

	    if (d_option)
	    	do_shutdown();
	    }
	else { /* not RUNNING */
	    if (p_option)
		print_pram_info();
	    else
		fprintf(stderr, "The AppleTalk stack is not running.\n");
	}
	exit (retval);
} /* main */

#define ET	('e'<<8|'t')
#define TR	('t'<<8|'r')
#define FI	('f'<<8|'i')
#define AT	('a'<<8|'t')

static void
print_header(if_name)
     char *if_name;
{
    if (!header_already_printed) {
	switch (*(short*)if_name) {
	  case ET:
	    printf(MSGSTR(M_IF_ET, "\tAppleTalk interface.............. EtherTalk:"));
	    break;
	  case TR:
	    printf(MSGSTR(M_IF_TR, "\tAppleTalk interface.............. TokenTalk:"));
	    break;
	  case FI:
	    printf(MSGSTR(M_IF_FD, "\tAppleTalk interface.............. FddiTalk:"));
	    break;
	  case AT:
	    printf(MSGSTR(M_IF_AT, "\tAppleTalk interface.............. AtmTalk:"));
	    break;
	  default:
	    printf(MSGSTR(M_IF,"\tAppleTalk interface.............. "));
	    break;
	}
	printf("%s\n", if_name);
	header_already_printed++;
    }

    return;
}


static void
print_nodeid ()
{
	at_ddp_cfg_t	ddp_cfg;

	if (ddp_config(&ddp_cfg) != 0) {
		fprintf(stderr, MSGSTR(M_NODE_ID_NG,
			"%s: Can't get the node id (%s) \n"), 
			progname, sys_errlist [errno]);
		exit(1);
	}

	printf(MSGSTR(M_NET_NUM,
		"\tNetwork Number .................. %u (0x%x)\n"), 
		NET_VALUE(ddp_cfg.node_addr.net), NET_VALUE(ddp_cfg.node_addr.net));
	printf(MSGSTR(M_NODE_ID,
		"\tNode ID ......................... %u (0x%x)\n"),
		ddp_cfg.node_addr.node, ddp_cfg.node_addr.node);

	return;
}


static void
print_pram_info ()
{
	at_nvestr_t zonename;
	struct atalk_addr netnumber;
	char zone[64];
	int config_err = 0;
	char if_name[AT_IF_NAME_LEN];

	if (readxpram(if_name, AT_IF_NAME_LEN, INTERFACE_OFFSET)) {
		fprintf(stderr, MSGSTR(M_PRAM_ZONE, "PRAM interface info not found (%s) \n"),
			 sys_errlist[errno]);
		config_err++;
	}
	if (!config_err)
		print_header (if_name);

	if (readxpram(&zonename, sizeof(at_nvestr_t), ZONENAME_OFFSET)) {
		fprintf(stderr, MSGSTR(M_PRAM_ZONE, "PRAM Zone info not found (%s) \n"),
			 sys_errlist[errno]);
		config_err++;
	}

	if (readxpram(&netnumber, sizeof(struct atalk_addr), ADDRESS_OFFSET)) {
		fprintf(stderr, MSGSTR(M_PRAM_ADDR, "PRAM Address info not found (%s) \n"),
			sys_errlist[errno]);
		config_err++;
	}

/*	printf(MSGSTR(M_PRM_DEF,"\tPRAM default device ............. N/A\n")); */

	strncpy(zone, zonename.str, zonename.len);
	zone[zonename.len] = '\0';

	if (zonename.len == 0)
		strcpy(zone, "*");

	printf(MSGSTR(M_PRM_ZONE, "\tPRAM default zonename ........... %s\n"),
		zone);
	printf(MSGSTR(M_PRM_NET, "\tPRAM netnumber .................. %u (%#x)\n"),
		NET_VALUE(netnumber.atalk_net), NET_VALUE(netnumber.atalk_net));
	printf(MSGSTR(M_PRM_NODE, "\tPRAM node id .................... %u (%#x)\n"),
	 	netnumber.atalk_node, netnumber.atalk_node);

	if (config_err)
		exit(-1);

	return;
}


static void
print_statistics (p_option)
int p_option;
{
	at_ddp_cfg_t	ddp_cfg;
	at_ddp_stats_t	ddp_stats;
	static if_cfg_t syscfg;		/* system cfg info */
	at_nvestr_t	zone;
	char zonename[64];
	at_elap_cfg_t	elap_cfg;
	int if_id;
	
	if ((if_id = openCtrlFile(&syscfg, "print_statistics",MIN_MAJOR, MIN_MINOR)) < 0 )
		return;
	(void) close(if_id);

	if (ddp_config(&ddp_cfg) != 0) {
		fprintf(stderr, MSGSTR(M_NODE_ID_NG, 
			"%s: Can't get the node id (%s)\n"),
			progname, sys_errlist [errno]);
		exit(1);
	}
	if (elap_get_cfg(&elap_cfg)) {
		perror(MSGSTR(M_CFG, "error getting configuration from kernel"));
		exit(1);
	}
	/* 
	   Note that at one time "syscfg.ver_major, syscfg.ver_minor"
	   contained the version number in the format "%d.%02d",
	   however it's currently a constant in the kernel, and the
	   appletalk application doesn't print it.
	*/

	print_header (elap_cfg.if_name);

	if (ddp_statistics(&ddp_stats) != 0) {
		fprintf(stderr, MSGSTR(M_NET_STAT_NG,
			"%s: Can't get the network statistics (%s)\n"),
			progname, sys_errlist [errno]);
		exit(1);
	}

	if (zip_getmyzone(&zone) != 0) {
		fprintf(stderr, MSGSTR(M_NET_STAT_NG,
			"%s: Can't get the network statistics (%s)\n"),
			progname, sys_errlist [errno]);
		exit(1);
	}

	printf(MSGSTR(M_NET_NUM,
		"\tNetwork Number .................. %u (0x%x)\n"), 
		NET_VALUE(ddp_cfg.node_addr.net), 
			NET_VALUE(ddp_cfg.node_addr.net));
	printf(MSGSTR(M_NODE_ID, "\tNode ID ......................... %u (0x%x)\n"),
		ddp_cfg.node_addr.node, ddp_cfg.node_addr.node);
	strncpy(zonename, zone.str, zone.len);
	zonename[zone.len] = '\0';
	printf(MSGSTR(M_CURZONE, "\tCurrent Zone .................... %s\n"),
		zonename);
	if (p_option) {
		printf("\n");
		print_pram_info();
	}

	printf("\n");

	printf(MSGSTR(M_BRDG_NO, 
		"\tBridge number ................... %u (0x%x)\n"),
		ddp_cfg.router_addr.node,
		ddp_cfg.router_addr.node);
	printf(MSGSTR(M_BRDG_NET, 
		"\tBridge net ...................... %u (0x%x)\n"),
		NET_VALUE(ddp_cfg.router_addr.net),
		NET_VALUE(ddp_cfg.router_addr.net));
	printf(MSGSTR(M_PKT_XMIT, "\tPackets Transmitted ............. %u\n"),
		ddp_stats.xmit_packets);
	printf(MSGSTR(M_BYTES_XMIT, "\tBytes Transmitted ............... %u\n"),
		ddp_stats.xmit_bytes);
	printf(MSGSTR(M_BRC, "\tBest Router Cache used (pkts) ... %u\n"),
		ddp_stats.xmit_BRT_used);
	printf(MSGSTR(M_PKT_REC, "\tPackets Received ................ %u\n"),
		ddp_stats.rcv_packets);
	printf(MSGSTR(M_BYTES_REC, "\tBytes Received .................. %u\n"),
		ddp_stats.rcv_bytes);
	printf(MSGSTR(M_PKT_REG, "\tPackets for unregistered socket . %u\n"),
		ddp_stats.rcv_unreg_socket);
	printf(MSGSTR(M_PKT_RANGE, "\tPackets for out of range socket . %u\n"),
		ddp_stats.rcv_bad_socket);
	printf(MSGSTR(M_LENGTH, "\tLength errors ................... %u\n"),
		ddp_stats.rcv_bad_length);
	printf(MSGSTR(M_CHKSUM, "\tChecksum errors ................. %u\n"),
		ddp_stats.rcv_bad_checksum);
	printf(MSGSTR(M_PKT_DROP, "\tPackets dropped (no buffers) .... %u\n"),
		ddp_stats.rcv_dropped_nobuf + 
		ddp_stats.xmit_dropped_nobuf);

	return;
}

static int sigusr1_flag;
static void sigusr1_handler(int sig) { sigusr1_flag = 0; }
static int sigterm_flag;
static void sigterm_handler(int sig) { sigterm_flag = 1; }

int do_init(opt)
     struct atalk_options *opt;
{
	pid_t pid, ppid;
	int *pid_buf;
	int lap_fd, status;
	int i, k, x;
	int homeseed = FALSE;
	char *devicename[IF_TOTAL_MAX+1];
	char *p, ifname[128];

	/* get the defice information */
	if (router) {
	  	/* read & validate config file */
	    	if (getConfigInfo(elapcfg, if_zones, &filecfg, opt, &homeseed)) {
			return(-1);
		}
		if (opt->e_option) {
			showCfg(elapcfg, &filecfg, if_zones);
			return(0);
		}
		if (opt->c_option) {
		  	return(0);	/* if just checking cfg, we passed */
		}

		for (k=0, i=0; i<IF_TOTAL_MAX; i++) {
		  if ( (elapcfg[i].if_name[0] == 'e')
		       || (elapcfg[i].if_name[0] == 'n')
		       || (elapcfg[i].if_name[0] == 't')
		       || (elapcfg[i].if_name[0] == 'f') ) {
		    devicename[k++] = elapcfg[i].if_name;
		  }
		  
		}
		if (k == 0)
		  devicename[k++] = et_interface;
		devicename[k] = NULL;
	}
	else {
	  	devicename[0] = et_interface;
		devicename[1] = NULL;
	}

	kernelDbg(DBG_SET_FROM_FILE,0);
	
	/* make sure all of the interfaces are up */
	for (i=0; devicename[i] != NULL; i++) {
	        /* make sure that the interface name is valid */
		for (p = devicename[i]; *p != '\0'; p++) {
			if (isdigit(*p))
				break;
		}
		if (*p == '\0' || !isdigit(*p)) {
			fprintf(stderr,	"%s: %s: bad device name\n", progname, devicename[i]);
			return(-1);
		}

		/* do an ifconfig on the interface name */
		sprintf(ifname, "%s %s up", IFCONFIG_CMD, devicename[i]);
		if (0 != (system(ifname))) {
			fprintf(stderr, "%s: '%s' failed\n", progname, ifname);
			return(-1);
		}
	   }

	/* get the parent process id */
	ppid = getpid();

	/* set up to catch SIGINT */
	sigusr1_flag = 1;
	signal(SIGUSR1, sigusr1_handler);
	sigterm_flag = 0;
	signal(SIGTERM, sigterm_handler);
							
        /* spawn a child process to build up the stack */
	if ( !(pid = fork())) {
	  /* child process */
	  
	  /*
	   * The following code is for the child process.
	   * We're going to build up the AppleTalk stack and then
	   * suspend the process to hold up the stack until requested
	   * to terminate.
	   */
	  setpgid(0, 0);
		  
	  if (-1 == (lap_fd = atalkInit(devicename))) {
	     fprintf(stderr, "Failed to start the AppleTalk stack.\n");

	     /* force the parent to exit immediately */
	     kill(ppid, SIGTERM);
	  } else {
	     kill(ppid, SIGUSR1);

	     /* wait to be shutdown */
	     read(lap_fd, &x, 1);
	     if (sigterm_flag) {
	     	(void)routerShutdown();
	     }
	     atalkUnlink();
	     if (close(lap_fd))
	        perror(MSGSTR(M_CLOSE_LAP, "do_init: close lap_fd"));

	  }
	  exit(0);
	} else {
	   /* parent process */
	   if (sigusr1_flag)
	      pause();
	   if (sigusr1_flag)
	      exit(1);

	   status = (router)? routerStartup(opt, elapcfg, if_zones, &filecfg, &homeseed)
			    : elap_init(et_interface, opt, pid_buf);
	   if (status < 0) {
		switch (errno) {
		case EEXIST :
			fprintf(stderr, MSGSTR(M_PORT_USED,
				"%s: another home port already designated\n"),
				progname);
			break;
		case EACCES :
		        fprintf(stderr, MSGSTR(M_PERM, 
				"%s: permission denied\n"), progname);
			break;
		case EPERM :
			fprintf(stderr, MSGSTR(M_PORT_UP, 
				"%s: port already up, can't designate as home port\n"),
				progname);
			break;
		case EINVAL :
			fprintf(stderr, MSGSTR(M_BAD_IF,
				"%s: invalid interface specified\n"), progname);
			break;
		case EFAULT :
			fprintf(stderr, MSGSTR(M_IF_STATE,
				"%s: can't change range in current i/f state\n"),
				progname);
			break;
		case ECHRNG :
			fprintf(stderr, MSGSTR(M_PKT_REG_ERR,
				"%s: error registering packet type\n"), progname);
			break;
		case EALREADY :
			fprintf(stderr, MSGSTR(M_IF_RUNNING,
				"%s: interface %s is already running\n"),progname, et_interface);
			break;
		default :
			fprintf(stderr, "%s: %s (%d)\n", progname, sys_errlist [errno], errno);
			break;
		}
		return(-1);
	  } else {
		if(register_this_node() != 0) 
			printf(MSGSTR(M_NODE_REG, "node registration failed\n"));
/* For now, don't print anything on success.
		else
			printf(MSGSTR(M_AT_UP, "AppleTalk now online\n"));
*/
	  }
	return(0);
	}
} /* do_init */

static void
do_shutdown ()
{
	int *pid_buf;

	if (atalkState(0, (char **)&pid_buf) == 0) {
	  	if (pid_buf) {
			down_error(pid_buf);
			exit(1);
		}
	}
	if (routerShutdown()) {
		if (errno == EACCES)
		      fprintf(stderr, 
			      MSGSTR(M_PERM,"%s: permission denied\n"),
			      progname);
		else
			fprintf(stderr, 
				"%s: error: %s\n",
				progname, sys_errlist[errno]);
		exit(1);
	}
} /* do_shutdown */

static void
down_error(pid_buf)
	int *pid_buf;
{
	/* pid_buf contains ids of processes still holding on to resources */
	fprintf(stderr,
		MSGSTR(M_PROC_ERR,"Cannot bring down AppleTalk.\n"\
		"You must first terminate the following processes:\n"));
	while (*pid_buf != 0) {
		fprintf(stderr, "PID=%d\n", *pid_buf);
		pid_buf++;
	}
}

static int
register_this_node()
{
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
	nbp_make_entity(&name, u_name.nodename, u_name.sysname, "*");

	if (_nbp_send_(NBP_REGISTER, &addr, &name, NULL, 1, NULL) <= 0) {
		printf(MSGSTR(M_NBP_SND, "reg_this_node:_nbp_send failed\n"));
		return (-1);
	}
	return (0);
} /* register_this_node */

static void
lookup_if(bflag)
     int bflag;
{
	if (!bflag) 
		et_interface = AT_DEF_ET_INTERFACE;
	at_interface = "ethertalk0";
} /* lookup_if */
