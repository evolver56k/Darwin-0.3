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
/* Title:	lap_init.c
 *
 * Facility:	Generic AppleTalk Link Access Protocol Interface
 *		(ALAP, ELAP, etc...)
 *
 * Author:	Gregory Burns, Creation Date: April-1988
 *
 ******************************************************************************
 *                                                                            *
 *        Copyright (c) 1988, 1998 Apple Computer, Inc.                       *
 *                                                                            *
 *        The information contained herein is subject to change without       *
 *        notice and  should not be  construed as a commitment by Apple       *
 *        Computer, Inc. Apple Computer, Inc. assumes no responsibility       *
 *        for any errors that may appear.                                     *
 *                                                                            *
 *        Confidential and Proprietary to Apple Computer, Inc.                *
 *                                                                            *
 ******************************************************************************
 *
 * $Id: lap_init.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

/* "@(#)lap_init.c: 2.0, 1.19; 2/26/93; Copyright 1988-92, Apple Computer, Inc." */

#include <h/sysglue.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
 /* #include <sys/signal.h> */
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <at/appletalk.h>
#include <h/lap.h>
#include <h/at-config.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/atp.h>
#include <at/nbp.h>
#include <at/at_lap.h>		/* for LAP_xxx defines */
#include <h/atlog.h>
#include <h/routing_tables.h>
#include <at/asp_if.h>		
#include <at/asp_errno.h>

/* #include <locale.h> */

#ifdef _AIX
#include <libat_msg.h>	   /* created by nls at build */
#endif

#include <LibcAT/at_proto.h>
#include <LibcAT/at_paths.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

/*#define DEV_VERSION */							/* development version */

/*#define MSGSTR(num,str)		catgets(catd, MS_LAP_INIT, num,str) */
#define MSGSTR(num,str)		str 

	/* PRAM state information */
#define PRAM_FILE             NVRAM
#define NVRAMSIZE		256

#define ZONENAME_SIZE		32
#define ZONENAME_OFFSET		48
#define ADDRESS_OFFSET		16
#define INTERFACE_OFFSET	1
#define INTERFACE_SIZE		4

	/* multi-port setup defines */
#define MAX_ZONES		50	/* max cfg file zone count */
#define MAX_LINE_LENGTH		240	/* max size cfg file line */
#define INVALID_ZIP_CHARS	"=@*:\377"
#define MAX_NET_NO		0xFEFF	/* max legal net number */
#define NO_HOME_PORT		999	/* indicates no home port
											   selected in home_number */

#define COMMENT_CHAR 		'#'	/* version checking */
#define MIN_MAJOR		0	/* minimum required version for */
#define MIN_MINOR		0	/* advanced features */

	/* printout format defines */
#define ZONE_IFS_PER_LINE	6	/* # of if's to list per line for
				           each zone for showCfg() */
#define Z_MAX_PRINT		15	/* maximum # zones to print */

	/* default message catalog strings */
#define DS_CONF_RANGE "\
Conflict between port %d (%s) and port %d (%s)\n\
they are using the same net range (%d-%d)\n"

#define DS_CONF_SEED_RNG "\
Conflict on port %d (%s): Router %d:%d seeds net %d-%d\n\
and not %d:%d as asked in our configuration\n"

#define DS_CONF_SEED1 "\
Conflict on port %d (%s): Router %d seeds net %d\n\
and not %d as asked in our configuration\n"

#define DS_CONF_SEED_NODE "\
Conflict on port %d (%s): Node %d:%d seeds %d:%d instead of %d:%d\n"

#define DS_NO_ZONES_FOUND "\
No Zones names received for Port %d (%s) on net %d:%d\n"

#define DS_NO_SEED 	"\
No seed information for port %d (%s) was found on the net\n"

#define DS_INVAL_RANGE	"\
Port %d (%s) is using an invalid network range (%d:%d)\n"

#define DS_SEED_STARTUP	"\
Problem, port %d (%s) Router %d:%d seeds in the startup range\n"

#define DS_BAD_VER	"\
Received a bad version (v%d) RTMP packet from node %d:%d\n"

#define DS_RTMP_OVER	"\
RTMP Route Table overflow. Too many routes. Increase RTMP Table size\n"

#define DS_ZIP_OVER	"\
ZIP Zone Table overflow. Too many zones. Increase ZIP Table size\n"

extern int last_getlocalzones;

char *if_types[] = { IF_TYPE_1, IF_TYPE_2, IF_TYPE_3, IF_TYPE_4, IF_TYPE_5}; 
char			homePort[5];			/* name of home port */
unsigned		seed[IF_TYPENO_CNT];	/* I/F bit-mapped seed port ID */

/* prototypes */

static	void	nbp_upshift ( register u_char *str, register intcount);
static	int	nbp_strcmp ( register at_nvestr_t *str1, 
						 register at_nvestr_t *str2);
static int	get_new_zonename(at_nvestr_t *zone_name, int quiet);
static int	set_net_addr(struct atalk_addr *final_address);
static int	get_net_addr(struct atalk_addr *init_address);
static int	set_if_name(char *if_name);
static int	set_zone_name(at_nvestr_t *zone_name);
static int	get_pram_zonename(at_nvestr_t *zone_name);
static void	printOnlineError(int error);

int elap_init (char *hw_interface, struct atalk_options *opt, char **pid_buf);
int routerStartup (struct atalk_options *opt, 
		   at_elap_cfg_t elapcfg[], if_zone_info_t if_zones[], 
		   if_cfg_t *filecfgp, int *homeseed);
int routerShutdown();
int atalkInit(char **);
void atalkUnlink();
int elap_get_cfg (at_elap_cfg_t *);

int move_file(char *, char *);

char version[255];
char errStr[255];
int  stateBuff[0x400];
int  pidBuff[0x400];

static FILE *STDOUT = stdout;

static void reset_multicast(if_id)
     int if_id;
{
	int size = 0;

	at_send_to_dev(if_id, LAP_IOC_DO_DEFER, NULL, &size);
}

int routerStartup(opt, elapcfg, if_zones, filecfgp, homeSeed)
     struct atalk_options *opt;
     at_elap_cfg_t elapcfg[];		
     if_zone_info_t if_zones[]; /* zone info from cfg file */
     if_cfg_t *filecfgp;	/* config info from config file */
     int *homeSeed;
{
	int status;
	int size;
	int if_id=-1;
	int i;
	int error = FALSE;
	kern_err_t	ke;
	static if_cfg_t syscfg;		/* system cfg info */
	at_elap_cfg_t	cfg;
	router_init_t	tsize;
	at_nvestr_t	zone;
	int		mh=FALSE;		/* TRUE = multihome mode */

	if ((if_id = openCtrlFile(&syscfg, "routerStartup",0,0)) < 0)
		return(-1);
	if (checkConfigInfo(&syscfg, filecfgp))
		goto error;	

	/*
	 * Setup the routing & zip table size for the router
	 */

	if (opt->r_table >= RT_MIN && opt->r_table <= RT_MAX) 
		tsize.rtable_size = opt->r_table;
	else
		tsize.rtable_size = RT_DEFAULT;

	if (opt->z_table >= ZT_MIN && opt->z_table <= ZT_MAX) 
		tsize.ztable_size = opt->z_table;
	else 
		tsize.ztable_size = ZT_DEFAULT;
	tsize.flags = AT_ST_ROUTER;
	if (opt->h_option) {
		tsize.flags |= AT_ST_MULTIHOME;
		mh=TRUE;
	}
	size = sizeof(tsize);
	
	status = at_send_to_dev(if_id, LAP_IOC_ROUTER_INIT, &tsize, &size);
	if (status) {
		fprintf(stderr, MSGSTR(M_TABLES, "error clearing tables %d\n"),cthread_errno());
		goto error;	
	}

	/* bring I/F's online */
	for (i=0; i<IF_TOTAL_MAX; i++) {
		status = 0;
		if ( (elapcfg[i].if_name[0] == 'e')
		     || (elapcfg[i].if_name[0] == 'n')
		     || (elapcfg[i].if_name[0] == 't')
		     || (elapcfg[i].if_name[0] == 'f') ) {
		 	size = sizeof(at_elap_cfg_t);
			elapcfg[i].flags |= ELAP_CFG_ZONELESS;
			if (elapcfg[i].netStart)
				elapcfg[i].flags |= ELAP_CFG_SEED;
			status = at_send_to_dev(if_id, LAP_IOC_ONLINE,
						&elapcfg[i], &size);
		}
		if (status) {		/* if any errors, shut down all I/F's
					   that we just brought up */
			fprintf(stderr, MSGSTR(M_EONLINE, 
				"error bringing interface %s online (%d)\n"),
				elapcfg[i].if_name, cthread_errno());
			printOnlineError(status);
			fprintf(stderr, MSGSTR(M_BRING_IFS_DOWN,
					       "bringing all interfaces down\n"));
			error = TRUE;
			break;
		}

	}
	if (status)
		goto error;

	reset_multicast(if_id);

	for (i=0; !mh && i<MAX_ZONES; i++) {
		if (!if_zones[i].zone_name.len)
			break;
/*		printf("adding zone %s\n",if_zones[i].zone_name.str);*/
		size = sizeof(if_zone_info_t);
		if (at_send_to_dev(if_id, LAP_IOC_ADD_ZONE, &if_zones[i], &size) == -1)
			perror(MSGSTR(M_ERR_ZONE, "... ERROR adding zone\n"));

	}

	size = 0;
	memset(&ke,0,sizeof(ke));
	status = at_send_to_dev(if_id, LAP_IOC_ROUTER_START, &ke, &size);
	if (status) {
	    while (cthread_errno() == ENOTREADY) {
		size = 0;
		if ((status=at_send_to_dev(if_id, LAP_IOC_DO_DELAY, &ke, &size)) != -1)
			break;
	    }
	    /* LD 03/98: in router mode, it's ok to be here with ENOTREADY still 
	       set in errno */
	    if (cthread_errno() && (cthread_errno() != ENOTREADY)) {
		switch (cthread_errno()) {
		case ENOMSG:
			/* there was a problem with the startup */
			fprintf(stderr,  MSGSTR(M_NOMSG,"ERROR: NOMSG errno = %d\n"), 
				cthread_errno());
		default:
			fprintf(stderr,  MSGSTR(M_UNKNOWN,
				"ERROR: Unknown errno = %d\n"), cthread_errno());
		}
	    }
	}
	if (size) { 
		char *p = NULL;

		fprintf(stderr, MSGSTR(M_ROUTER,
			"WARNING: Appletalk is not running, " \
			"the following problem occured:\n \n"));
			
		switch (ke.errno) {
			case KE_CONF_RANGE:
				fprintf(stderr, MSGSTR(M_CONF_RANGE, DS_CONF_RANGE),
					ke.port1, ke.name1, ke.port2, ke.name2,
					ke.netr1b, ke.netr1e);
				break;
			case KE_CONF_SEED_RNG:
				fprintf(stderr, MSGSTR(M_CONF_SEED_RNG, DS_CONF_SEED_RNG),
					ke.port1, ke.name1, ke.net, ke.node, ke.netr1b, 
					ke.netr1e, ke.netr2b, ke.netr2e);
				break;
			case KE_CONF_SEED1:
				fprintf(stderr, MSGSTR(M_CONF_SEED1, DS_CONF_SEED1),
					ke.port1, ke.name1, ke.node, ke.netr1e, ke.netr2e);
				break;
			case KE_CONF_SEED_NODE:
				fprintf(stderr, MSGSTR(M_CONF_SEED_NODE, DS_CONF_SEED_NODE),
					ke.port1, ke.name1, ke.net, ke.node, ke.netr1b, 
					ke.netr1e, ke.netr2b, ke.netr2e);
				break;
			case KE_NO_ZONES_FOUND:
				fprintf(stderr, MSGSTR(M_NO_ZONES_FOUND, DS_NO_ZONES_FOUND),
					ke.port1, ke.name1, ke.netr1b, ke.netr1e);
				break;
			case KE_NO_SEED:
				fprintf(stderr, MSGSTR(M_NO_SEED, DS_NO_SEED),
					ke.port1, ke.name1);
				break;
			case KE_INVAL_RANGE:
				fprintf(stderr, MSGSTR(M_INVAL_RANGE, DS_INVAL_RANGE),
					ke.port1, ke.name1, ke.netr1b, ke.netr1e);
				break;
			case KE_SEED_STARTUP:
				fprintf(stderr, MSGSTR(M_SEED_STARTUP, DS_SEED_STARTUP),
					ke.port1, ke.name1, ke.net, ke.node);
				p = MSGSTR(M_SEED_STARTUP, DS_SEED_STARTUP);
				break;
			case KE_BAD_VER:
				fprintf(stderr, MSGSTR(M_BAD_VER, DS_BAD_VER),
					ke.rtmp_id, ke.net, ke.node);
				break;
			case KE_RTMP_OVERFLOW:
				fprintf(stderr, MSGSTR(M_RTMP_OVER, DS_RTMP_OVER));
				break;
			case KE_ZIP_OVERFLOW:
				fprintf(stderr, MSGSTR(M_ZIP_OVER, DS_ZIP_OVER));
				break;
			default:
				fprintf(stderr,MSGSTR(M_UNKNOWN_KE,
					"Unknown kernel error code:%d\n"), ke.errno);
		}
		(void)close(if_id);
		(void)routerShutdown();
		return(-1);
	}
/* for now, don't print anything for success
	else
		fprintf(stderr,  MSGSTR(M_RTR_UP,
			"Bring-up sequence OK: Appletalk is up and running\n"));

	showRoutes();
	showZones();
*/	
	if (!*homeSeed) {
		if(setDefaultZones(opt->h_option ? AT_ST_MULTIHOME :  AT_ST_ROUTER,
			opt->q_option, opt->f_option, if_zones))
			return(-1);
	}

	(void)at_send_to_dev(if_id, ELAP_IOC_GET_CFG, &cfg, &size);
	set_net_addr(&cfg.node);
	zip_getmyzone(&zone);
	set_zone_name(&zone);
	set_if_name(cfg.if_name);	
	reset_multicast(if_id);
	(void) close(if_id);
	return(status);

error:
	if (if_id != -1) {
		(void) close(if_id);
		routerShutdown();
	}
	return(-1);
} /* routerStartup */

int routerShutdown()
{
	int 
	  status,
	  size = 0,
	  if_id;
	static if_cfg_t syscfg;		/* system cfg info */

	if ((if_id = openCtrlFile(&syscfg, "routerShutdown",0,0)) < 0)
		return(-1);
		
	if (status = at_send_to_dev(if_id, LAP_IOC_ROUTER_SHUTDOWN, NULL, &size)) {
		fprintf(stderr,  MSGSTR(M_RTR_SHTDWN,"error shutting AppleTalk down\n"));
		(void) close(if_id);
		return(-1);
	}
	(void) close(if_id);
	return(0);
} /* routerShutdown */

int elap_init (hw_interface, opt, pid_buf)
     char *hw_interface;
     struct atalk_options *opt;
     char **pid_buf;
{
	at_elap_cfg_t elap_cfg;
	int size = 0;
	int status;
	int i;
	int if_id;
	at_nvestr_t	zone_name;
	at_elap_cfg_t cfg;
	struct atalk_addr init_addr;
	int zonesSet=FALSE;		/* TRUE if we sent zones to kernel */

	memset(&elap_cfg,0,sizeof(elap_cfg));    

	if ((if_id = openCtrlFile(NULL, "elap_init",0,0)) < 0) 
		return(-1);

	elap_cfg.flags = 0;
   	size = sizeof(at_elap_cfg_t);
	strcpy (elap_cfg.if_name, hw_interface);
	elap_cfg.flags |= ELAP_CFG_HOME;

	/* Check if we can reuse the same net/node address we have saved */

	if (get_net_addr(&init_addr) == 0) {
		elap_cfg.initial_addr = init_addr;
		status = at_send_to_dev(if_id, ELAP_IOC_SET_CFG, &elap_cfg, &size);
		if (status == -1) {
			(void)close(if_id);
			return (-1);
		}
	}

	/* do the actual startup */
	if (opt->q_option && !get_pram_zonename(&zone_name)) {
	  	elap_cfg.zonename = zone_name;
		size = sizeof(at_elap_cfg_t);
		at_send_to_dev(if_id, ELAP_IOC_SET_ZONE, &elap_cfg, &size);
	}
		
	do {
		elap_cfg.netStart = opt->start;
		elap_cfg.netEnd   = opt->end;
		elap_cfg.flags |= ELAP_CFG_HOME;
		status = at_send_to_dev(if_id, LAP_IOC_ONLINE, 	&elap_cfg, &size);
		if (status == -1) {
			switch (cthread_errno()) {
			case ENODEV :
				/* the zone name supplied from PRAM
				 * is no good, we need to get new 
				 * zone name from user and use it.
				 */
				zonesSet=TRUE;
				zone_name.len = 0;
				if (get_new_zonename(&zone_name,opt->q_option) == -1) {
					(void) close(if_id);
					return (-1);
				}
				elap_cfg.zonename = zone_name;
				
	    			size = sizeof(at_elap_cfg_t);
				if (at_send_to_dev(if_id, ELAP_IOC_SET_ZONE, 
						   &elap_cfg, 
					&size) == -1) {
					(void) close(if_id);
					return (-1);
	    			}
				break;
			default :
				(void) close(if_id);
				return (-1);
			}
		}
		if (!zonesSet) {
			/*  one or no zones, if one, send it down */
			char	buf[ATP_DATA_SIZE+1];
			if (zip_getlocalzones(1, &(buf[0])) != -1 ) {
				size = ((at_nvestr_t*)&buf[0])->len + 1;
				i = at_send_to_dev(if_id, LAP_IOC_SET_LOCAL_ZONES, 
					&buf[0], &size);
				if (i) {
					fprintf(stderr, 
					MSGSTR(M_ERR_SET_LOCAL,"error setting localzones %d\n"),cthread_errno());
					goto error;

				}
			}
		}
	} while (status==-1 && cthread_errno()==ENODEV);

	(void)at_send_to_dev(if_id, ELAP_IOC_GET_CFG, &cfg, &size);
	set_net_addr(&cfg.node);
	zip_getmyzone(&cfg.zonename);

	/* if zone is *, there was no router, so send down pram zone,
	   if there is one, so kernel can register mcast addr for when
	   the router comes back up
	*/	
	if ( cfg.zonename.len == 1 && cfg.zonename.str[0] == '*' ){
		if (get_pram_zonename(&zone_name) == 0 && zone_name.len && 
		    zone_name.len != 1 && zone_name.str[0] != '*' ){
			elap_cfg.flags = ELAP_CFG_ZONE_MCAST;
			elap_cfg.zonename = zone_name;
			size = sizeof(at_elap_cfg_t);
			at_send_to_dev(if_id, ELAP_IOC_SET_ZONE, &elap_cfg, 
				       &size);
		}
	}
	else
		set_zone_name(&cfg.zonename);
	set_if_name(cfg.if_name);	
	reset_multicast(if_id);

	(void) close(if_id);
	return (status);
error:
	(void) close(if_id);
	(void)routerShutdown();
	return (-1);
} /* elap_init */

static int get_new_zonename(zone_name, quiet)
     at_nvestr_t	*zone_name;
     int quiet;		/* if true, don't prompt user for zone */
{
	char	buf[10][ATP_DATA_SIZE+1];
	int	start = 1;
	int	status = 1;
	int size;
	int	i, cnt=0, entryCount=0;
	int	zone_index=0;
	at_nvestr_t *zoneEntries[64], *zonep;
	char	save, format[50];
	char	input_buff[20];
	int		if_id;
	
	zone_name->len = 0;
	status = 1;
	if ((if_id = openCtrlFile(NULL, "get_new_zonename",0,0)) < 0) 
		return(-1);

	while (status && cnt <10 && entryCount < 64 && !last_getlocalzones) {

		status = zip_getlocalzones(start, &(buf[cnt][0]));

		if (status == -1) {
			(void) close(if_id);
			return (-1);
		}
		for (i=0, zonep  = (at_nvestr_t *)(&(buf[cnt][0])); i<status; i++) {
			zonep = (at_nvestr_t *) (((char *)zonep) + zonep->len + 1);
		}
		size = (caddr_t)zonep  - (caddr_t)&buf[cnt][0];



		i = at_send_to_dev(if_id, LAP_IOC_SET_LOCAL_ZONES, 
			&buf[cnt][0], &size);
		if (i) {
			fprintf(stderr, 
				MSGSTR(M_ERR_SET_LOCAL,"error setting localzones %d\n"),cthread_errno());
			(void) close(if_id);
			return(-1);

		}
			/* there's only one zone name on the cable,
			 * take that to be the default and refrain
			 * from prompting the user.
			 * but be sure there is no more packets (ie Cisco AGS)
			 */

		if (start==1 && status==1 && last_getlocalzones) {
			strncpy((char *)zone_name, &(buf[0][0]), sizeof(at_nvestr_t));
			return(0);
		}
			/* we are being called when we do not
			 * want to stop the lap_init and prompt
			 * the user for her zone choice
			 * e.g. when starting up at boot time (rc.net.appletalk) or -z option
			 */

		if (quiet) {
			if (status >= 1) {
				strncpy((char *)zone_name, &(buf[0][0]), sizeof(at_nvestr_t));
				return(0);
			} else {
				/* if (status==0) we didn't get
				 * any zones... So, go back and
				 * try again...
				 */
				/* Previously it would keep trying indefinitely:
				   	status = 1;
				   	continue;
				   For now it will return an error after one try: */
				(void) close(if_id);
				return(-1);
			}
		}

		if (status == 0) {
			/* Previously it would keep trying indefinitely:
			   	fprintf(STDOUT,  MSGSTR(M_RESTART_ZONES,
				   "\n\n Starting over with the zone list...\n"));
			   	fprintf(STDOUT,  MSGSTR(M_SEL_ZN,
				   "{NOTE:You must select a zone name from the list.}\n"));
				status = start = 1;
				continue;
			   For now it will return an error after one try: */
			(void) close(if_id);
			return(-1);
		}

		if (cnt == 0)
			fprintf(STDOUT,  MSGSTR(M_SEL_ZONE,
			"Select a Zone (number) from the list.\n"));

		for (i=0, zonep  = (at_nvestr_t *)(&(buf[cnt][0])); i<status; i++) {
			save = zonep->str[zonep->len];
			zonep->str[zonep->len] = '\0';
			zoneEntries[entryCount++] = zonep;
			sprintf (format, "%%3d: %%-35.%ds",zonep->len);
			printf (format, entryCount, zonep->str);
			if (entryCount % 2 == 0) 
				printf ("\n");
			zonep->str[zonep->len] = save;
			zonep = (at_nvestr_t *) (((char *)zonep) + zonep->len + 1);
		}
		if (entryCount % 2 != 0) 
			printf ("\n");
		
		cnt++;
		start += status;
	}
again:
	printf ( MSGSTR(M_ZONE_NUM, "\nZone Number? "));
	gets(input_buff);
	if (input_buff[0] == '\0')
		goto again;
	if (!isdigit(input_buff[strspn(input_buff, " \t")]))
		goto again;
	zone_index = atoi(input_buff);
	if (zone_index < 1 || zone_index > entryCount) {
		fprintf(STDOUT,  MSGSTR(M_BAD_CHOICE,
			"That's not a valid choice. Try again.\n"));
		goto again;
	}


	zonep = (at_nvestr_t *)zoneEntries[zone_index -1];
	strncpy((char *)zone_name, (char *)zonep, zonep->len + 1);
	return (0);
}

static
int	set_net_addr(final_address)
struct atalk_addr *final_address;
{
/*		printf("set_net_addr :write net number %d, node %d\n",  
		NET_VALUE(final_address->atalk_net), final_address->atalk_node);
		*/

	if (!writexpram(final_address, sizeof(struct atalk_addr), ADDRESS_OFFSET))
		return (-1);

	return (0);
}

static int get_net_addr(init_address)
struct atalk_addr *init_address;
{
	if (!readxpram(init_address, sizeof(struct atalk_addr), ADDRESS_OFFSET)){
/*		printf("get_net_addr: readx net number %d, node %d\n", 
		NET_VALUE(init_address->atalk_net), init_address->atalk_node);
*/
		return(0);
	}
	else
		return (-1);
}

static
int	set_if_name (if_name)
char *if_name;
{
	if (!writexpram(if_name, AT_IF_NAME_LEN, INTERFACE_OFFSET)) 
		return(0);
	else
		return(-1);
}

static
int	get_if_name (if_name)
char *if_name;
{
	if (!readxpram(if_name, AT_IF_NAME_LEN, INTERFACE_OFFSET)) 
		return(0);
	else
		return(-1);
}

static
int	get_zone_name (zone_name)
at_nvestr_t	*zone_name;
{
	if (!readxpram(zone_name, sizeof(at_nvestr_t), ZONENAME_OFFSET))  {
		return(0);
	}
	else
		return(-1);
}

static
int	set_zone_name (zone_name)
at_nvestr_t	*zone_name;
{
	if (zone_name->str[0] == '*'){	/* If we don't have a router  */
		return(0);		/* return w/o writing to PRAM */
	}

/*	printf("set_zone_name: writex PRAM zone %s length=%d\n", 
	       zone_name->str, zone_name->len);
*/
	
	if (!writexpram(zone_name, sizeof(at_nvestr_t), ZONENAME_OFFSET)) {
		return(0);
	}
	else
		return(-1);
}

static
int	get_pram_zonename (zone_name)
at_nvestr_t	*zone_name;
{
	if (!readxpram(zone_name, sizeof(at_nvestr_t), ZONENAME_OFFSET)) {
/*		printf("get_prnam_zonename: readx  PRAM zone %s\n", 
		       zone_name->str);
*/
		return(0);
	}
	else
		return(-1);
}

int	elap_get_cfg (cfg)
     at_elap_cfg_t *cfg;
{
	int			if_id;
	int			size = 0;
	int			status;

	if ((if_id = openCtrlFile(NULL, "elap_get_cfg",0,0)) < 0)
		return(-1);

	status = at_send_to_dev(if_id, ELAP_IOC_GET_CFG, cfg, &size);
	
	if (status == -1)
	    return (-1);

	(void) close(if_id);
	return (0);
}

int lap_get_ifID(if_name, ifIN)
     char *if_name;
     at_if_name_t *ifIN;
{
	int if_id;
	int size = sizeof(at_if_name_t);
	int status;

	if ((if_id = openCtrlFile(NULL, "lap_get_ifID",0,0)) < 0)
		return(-1);

	status = at_send_to_dev(if_id, LAP_IOC_GET_IFID, ifIN, &size);
	(void) close(if_id);

	if (status == -1)
		return(-1);

	return(0);
} /* lap_get_ifID */

int getConfigInfo(elapcfgp, zonep, filecfgp, opt, homeSeed)
     at_elap_cfg_t elapcfgp[];
     if_zone_info_t zonep[];
     if_cfg_t *filecfgp;	/* config info from config file */
     struct atalk_options *opt;
     int *homeSeed;

/* obtains and checks zone and port configuration information from the 
   configuration file. fills in the 2 arrays passed

   returns   0 if no problems were encountered.
			-1 if any error occurred
*/

{
	if (getIFInfo(elapcfgp,filecfgp, opt, homeSeed))
		return(-1);
	if (!elapcfgp->if_name[0]) {
		fprintf(stderr, 
			MSGSTR(M_NO_IF_CFG, 
			"There are no interfaces configured\n"));
		return(-1);
	}
	
 	if (getZoneInfo(zonep, filecfgp, opt) ||
	    checkSeeds(elapcfgp,zonep))
   		return(-1);
#ifdef FOR_TEST_ONLY 
	showCfg(elapcfgp,filecfgp, if_zones);
#endif
	return(0);
} /* getConfigInfo */


int checkConfigInfo(syscfgp, filecfgp)
if_cfg_t *syscfgp, *filecfgp;

/* makes sure that the requested interfaces from the config file
   actually exist and that the system is not already in a running state

   returns  0 if everything is OK
	   -1 if there is any problem
*/
{
	int iftype, ifno;
	int anyOnline=FALSE;	/* TRUE if any installed I/F is not OFFLINE */
	int anyMissing=FALSE;	/* TRUE if any requested I/F is missing */

	/* for each I/F type */
	for (iftype=0; iftype<IF_TYPENO_CNT; iftype++) {
		for (ifno=0; ifno<IF_ANY_MAX; ifno++) {
			if (syscfgp->avail[iftype] & 1<<ifno) {
				/* if I/F exists, make sure it's offline */
				if (syscfgp->state[iftype][ifno] != LAP_OFFLINE) {
					fprintf(stderr,  MSGSTR(M_IF_ONLINE,
						"error, interface %s%d already online\n"),
						if_types[iftype],ifno);	
					anyOnline = TRUE;
					break;
				}
			}
			if (filecfgp->avail[iftype] & 1<<ifno)	{
				/* if cfg'ing I/F, make sure it exits */
				if (!(syscfgp->avail[iftype] & 1<<ifno)) {
					fprintf(stderr,  MSGSTR(M_IF_EXIST,
						"error, interface %s%d does not exist\n"),
						if_types[iftype],ifno);	
					anyMissing = TRUE;

				}
			}
		}
	}
	return( (anyMissing|anyOnline)? -1 : 0);
} /* checkConfigInfo */

int getIFInfo(elapcfgp, filecfgp, opt, homeSeed)
     at_elap_cfg_t elapcfgp[];
     if_cfg_t	*filecfgp;
     struct atalk_options *opt;
     int *homeSeed;		/* set true here if the home port for
				   router mode is a seed port */
/* reads interface configuration information from source file and
   places valid entries into cfgp 

   returns  0 on success
            -1 if any errors were encountered
*/
{
	FILE 	*pFin;			/* input cfg file */
	char	linein[MAX_LINE_LENGTH], 
	  	buf[MAX_LINE_LENGTH], 
	  	buf1[MAX_LINE_LENGTH], 
	  	*pc1,
		errbuf[100];
	int	i,x;
	int	ifno=0;
	int	parmno;
	int	lineno=0;
	int	home = FALSE;
	int	bad = TRUE;
	int	gotNetStart;		/* true if entry had starting network # */
	int	done;
	int	ifType;
	int	error = FALSE;
	int	unitNo;				/* I/F unit no ( the n in etn) */
	int	mh=FALSE;			/* TRUE = multihoming mode */

	if (opt->h_option)
		mh=TRUE;
	if (!(pFin = fopen(opt->f_option, "r"))) {
		fprintf(stderr,  MSGSTR(M_OPEN_CFG,
			"error opening configuration file: %s\n"), opt->f_option);
		return(-1);
	}
	filecfgp->home_number = NO_HOME_PORT;
	
	while (!feof(pFin)) {
	
		if (!fgets(linein, sizeof(linein)-1, pFin)) {
			continue;
		}
		lineno++;
		if (linein[0] == COMMENT_CHAR)  	/* if comment line */
			continue;
		if (linein[0] == ':')			/* if zone entry, skip */
			continue;

		strcpy(buf,linein);

		pc1 = strtok(buf,":");			/* pc1 -> first parm */
		parmno=0;
		bad = FALSE;
		gotNetStart = FALSE;
		done = FALSE;

		if (pc1) do {				/* do loop start for current line */
			if (sscanf(pc1," %s",buf1) != 1) /* skip whitespace */
				break;
			switch(parmno) {
			case 0:				/* I/F name */
				if ((ifType = getIFType(buf1)) != -1) {
					unitNo = IF_NO(buf1);
					if (filecfgp->avail[ifType] & 1<<unitNo) {
						sprintf(errbuf, MSGSTR(M_IF_USED,
							"interface %s already used"), buf1);
						bad = TRUE;
					}
					if (!bad)
						strncpy(elapcfgp[ifno].if_name, buf1, 4);
				}
				else {
					sprintf(errbuf, 
						MSGSTR(M_BAD_IF, 
						       "bad interface %s"), buf1);
					bad = TRUE;
			 	}	
				break;
			case 1:						/* net range or home desig */
			case 2:
				if (sscanf(buf1,"%d",&x) != 1){	
				    if (buf1[0] == '*') {		/* must be home then */
				      	if (home++) {
					    sprintf(errbuf, 
						    MSGSTR(M_MULT_HOME,
							   "multiple home designations (%d) "), parmno);
					    bad = TRUE;
					}
					elapcfgp[ifno].flags = ELAP_CFG_HOME;
					done = TRUE;			/* '*' must be last */
					sprintf(homePort,elapcfgp[ifno].if_name);
				    }
				    else {
				        sprintf(errbuf, MSGSTR(M_BAD_FMT, "invalid format "));
					bad = TRUE;
				    }
				} else	{						/* this is an actual net number */
				    if (x <= 0 || x >= MAX_NET_NO) {
				        sprintf(errbuf, MSGSTR(M_BAD_NET, "invalid net:%d "),x);
					bad = TRUE;
				    }
				    if (parmno == 1) {
					elapcfgp[ifno].netStart = x;
					gotNetStart = TRUE;
				    }
				    else {
				        if (elapcfgp[ifno].netStart > x) {
					    sprintf(errbuf, 
						    MSGSTR(M_END_LT_START,
							   "ending net less than start"));
					    bad = TRUE;
					}
					else	
					    elapcfgp[ifno].netEnd = x;
				    }
				}
				break;
			case 3:
				if (buf1[0] == '*') {		/* only valid entry for parm 3 */
				    if (home++) {
				        sprintf(errbuf, 
						MSGSTR(M_MULT_HOME,
						       "multiple home designations (%d) "), parmno);
					bad = TRUE;
				    }
				    elapcfgp[ifno].flags = ELAP_CFG_HOME;
				    sprintf(homePort,elapcfgp[ifno].if_name);
				    done = TRUE;
				    break;
				}
				/* fall through */
			default:
				fprintf(stderr,  
					MSGSTR(M_EXTRA_IGNORE,
					       "extra input ignored:(%s)\n"));
				done = TRUE;


			} /* end switch */
			if (bad)
				break;
			parmno++;
		} while(!done && pc1 && (pc1  = strtok(NULL,":")));
		if (!bad) {
			if (elapcfgp[ifno].if_name[0]) {		/* if entry used, check it */
			    if (gotNetStart) {
			        if (!elapcfgp[ifno].netEnd)		/* if no end, end = start */
				    elapcfgp[ifno].netEnd = elapcfgp[ifno].netStart;
				
				for (i=0; i<ifno; i++) {		/* check for range conflicts */
				    
				    if ((elapcfgp[ifno].netEnd >= elapcfgp[i].netStart &&
					 elapcfgp[ifno].netEnd <= elapcfgp[i].netEnd)  ||
					(elapcfgp[ifno].netStart >= elapcfgp[i].netStart &&
					 elapcfgp[ifno].netStart <= elapcfgp[i].netEnd) ||
					(elapcfgp[i].netEnd >= elapcfgp[ifno].netStart &&
					 elapcfgp[i].netEnd <= elapcfgp[ifno].netEnd)  ||
					(elapcfgp[i].netStart >= elapcfgp[ifno].netStart &&
					 elapcfgp[i].netStart <= elapcfgp[ifno].netEnd))  {
				        sprintf(errbuf,  
						MSGSTR(M_RANGE_CONFLICT,
						       "%s net range conflict with %s"),
						elapcfgp[ifno].if_name,
						elapcfgp[i].if_name);
					bad = TRUE;
					break;
				    }
				}
			    }
			    if (!bad) {		
			        filecfgp->avail[ifType] |= 1<<IF_NO(elapcfgp[ifno].if_name);
				if (elapcfgp[ifno].flags & ELAP_CFG_HOME) {
				    filecfgp->home_type   = ifType;
				    filecfgp->home_number = IF_NO(elapcfgp[ifno].if_name);
				}
				if (gotNetStart)    /* identify as seed */
				    seed[ifType] |= 1<<unitNo;
				
				if ((elapcfgp[ifno].flags & ELAP_CFG_HOME) &&
				    elapcfgp[ifno].netStart != 0 &&
				    !mh
				    )
				    *homeSeed = TRUE;
				ifno++;		
			    }
			} /* end if if_name */
		} 
		if (bad ) { 			/* reset this entry */
			memset(&elapcfgp[ifno], NULL,sizeof(elapcfgp[ifno]));
			if (opt->c_option)
						/* finish error msg */
				fprintf(stderr,  MSGSTR(M_LINE1,
					"error,  %s\n"),errbuf);
			else
				fprintf(stderr,  MSGSTR(M_LINE,
				"error, line %d. %s\n%s\n"),lineno,errbuf,linein);	
			error = TRUE;
			break;
		}	/* end if bad */
	}
	if (!mh && !home && !error && ifno) {
		if (opt->c_option || opt->e_option) 
			fprintf(stderr, MSGSTR(M_NO_HOME_PT_W, 
				"Warning, no home port specified.\n"\
				"(You must designate one interface as the home port before\n"\
				"starting Appletalk in router mode)\n\n"));
		else {
			error = TRUE;
			fprintf(stderr,  MSGSTR(M_NO_HOME_PT,
				"error, no home port specified\n"));
		}
	}
	if (mh && !home)
		elapcfgp[0].flags = ELAP_CFG_HOME;
	fclose(pFin);
	return(error ? -1 : 0);
} /* getIFInfo */

int getZoneInfo(zonep, filecfgp, opt)
     if_zone_info_t	zonep[];
     if_cfg_t	*filecfgp;
     struct atalk_options *opt;

/* reads  zone information from configuration file and
   places valid entries into zonep 

   returns  0 on success
		   -1 if any errors were encountered
*/
{
	FILE 	*pFin;			/* input cfg file */
	char	linein[MAX_LINE_LENGTH], buf[MAX_LINE_LENGTH], 
			buf1[MAX_LINE_LENGTH], *pc1, *pc2;
	char	tbuf1[NBP_NVE_STR_SIZE+20];
	char	tbuf2[NBP_NVE_STR_SIZE+20];
	char	curzone[NBP_NVE_STR_SIZE+20];
	char	errbuf[100];
	int		i,j;
	int		parmno;
	int		lineno=0, homeLine=0;
	int		home  = FALSE;
	int		bad   = TRUE;
	int		done  = FALSE;
	int		error = FALSE;
	int		zoneno = 0;
	int		ifType;
	int 	len;
	int		isseed;
	int		gotHomePort =  ( filecfgp->home_number != NO_HOME_PORT );
	int		mh=FALSE;		/* TRUE = multihoming mode */

	if (!(pFin = fopen(opt->f_option, "r"))) {
		fprintf(stderr,  MSGSTR(M_OPEN_CFG,
			"error opening configuration file: %s\n"), opt->f_option);
		return(-1);
	}
	if (opt->h_option)
		mh=TRUE;

	while (!feof(pFin)) {
		if (!fgets(linein, sizeof(linein)-1, pFin)) {
			continue;
		}
		lineno++;
		if (linein[0] == COMMENT_CHAR)  /* if comment line */
			continue;
		if (linein[0] != ':')			/* zone entries start with ':' */
			continue;						
		strcpy(buf,linein);

		pc1 = strtok(&buf[1],":");
		parmno=0;
		if (!bad)
			zoneno++;
		bad = FALSE;
		done = FALSE;

		if (pc1) do {					/* do loop start for current line */
			if (sscanf(pc1," %[^:\n]",buf1) != 1)	/* skip whitespace */
				break;
			switch(parmno) {
			case 0:											/* zone name */
				strcpy(curzone,buf1);
				if ((len = strlen(buf1)) > NBP_NVE_STR_SIZE) {		/* chk length */
					sprintf(errbuf,  MSGSTR(M_ZONE_TOO_LONG,
						"zone name too long"));
					bad = TRUE;
					break;
				}											/* chk for bad chars */
				if (pc2 = strpbrk(buf1, INVALID_ZIP_CHARS)) {
					sprintf(errbuf,  MSGSTR(M_IVAL_CHAR,
						"invalid character in zone name (%c)"),*pc2);
					bad = TRUE;
					break;
				}	
				for (i=0; !mh && i<zoneno; i++) {					/* check for dupe zones */
					strcpy(tbuf1, buf1);
					strcpy(tbuf2, zonep[i].zone_name.str);	/* compare same case */
					for (j=0; j<NBP_NVE_STR_SIZE; j++) {
						if (!tbuf1[j]) break;
						tbuf1[j] = toupper(tbuf1[j] );
						tbuf2[j] = toupper(tbuf2[j] );
					}
					if (!strcmp(tbuf1,tbuf2)) {				
						sprintf(errbuf,  MSGSTR(M_DUPE_ZONE,
							"duplicate zone entry"));
						bad = TRUE;
						break;
					}
				}
				strcpy(zonep[zoneno].zone_name.str,buf1);	/* save zone name */
				zonep[zoneno].zone_name.len = len;
				break;
			default:	/* I/F's or home designation */
				if (buf1[0] == '*') {
					if (home++) {
						sprintf(errbuf, MSGSTR(M_DUPE_HOME,
							"home already designated on line %d"),homeLine);
						bad = TRUE;
						break;
					}
					homeLine = lineno;
								/* home zone must exist on home I/F */
					if ( gotHomePort && 
						( !(zonep[zoneno].zone_ifs[ifType] & 
						  1<<filecfgp->home_number))) {
						sprintf(errbuf, MSGSTR(M_HOME_MUST,
							"Zone designated as 'home' must contain\n"\
									   "the home I/F (%s)"),homePort);
						bad = TRUE;
						break;
					}
					done = TRUE;
					zonep[zoneno].zone_home = TRUE;
				}
				else {		/* not home, must be another I/F */
				  		/* check for valid type and make
						   sure it has been defined too */
					isseed = TRUE;					
					if ((ifType = getIFType(buf1)) != -1     &&
					    filecfgp->avail[ifType] & 1<<IF_NO(buf1) &&
					    (mh || (isseed = (seed[ifType] & 1<<IF_NO(buf1)))) 
					   ) {
						zonep[zoneno].zone_ifs[ifType] |= 1<<IF_NO(buf1);
					}
					else {
						if (mh)		/* zone is from previoiusly used I/F
									   just ignore it */
							continue;
						if (isseed)
							sprintf(errbuf,  MSGSTR(M_INVAL_IF,
								"invalid interface (%s)"), buf1);
						else
							sprintf(errbuf, 
								MSGSTR(M_ALL_IFS_ASSOC,
								"all interfaces associated with a "\
							    "zone must be seed type\n"\
							    "interface for this zone is non-seed (%s)"),
								buf1);
						bad = TRUE;
						break;
					}
				}
			} /* end switch */
			parmno++;

			if (bad) { 		/* finish error msg */
				if(opt->c_option) {
					sscanf(pc1,":%[^:\n]",linein);
					fprintf(stderr, MSGSTR(M_EZONE,
						"error, %s\nzone:%s\n"),errbuf,curzone);	
				}
				else
					fprintf(stderr, MSGSTR(M_LINE,
						"error, line %d. %s\n%s\n"),lineno,errbuf,linein);	
				error = TRUE;
				break;
			}
	
		} while(!done && pc1 && (pc1  = strtok(NULL,":")));
	}
			/* if no home zone set and home port is seed type, check to
				see if there is only one zone assigned to that i/f. If
				so, then make it the home zone
			 */
	if ( !mh && gotHomePort && !home && 
		(seed[filecfgp->home_type] & 1<<filecfgp->home_number)) {
		j = -1; 	/* j will be set to home zone */
		for (i=0; i<MAX_ZONES; i++) {
			if (!zonep[i].zone_name.len)
				break;
			if (zonep[i].zone_ifs[filecfgp->home_type] & 
				1<<filecfgp->home_number)
				if (j >=0 )	{
				    home = 0;	/* more than one zone assigned to home
						   port, we can't assume correct one */
				    break;
				}
				else {
				    home = 1;
				    j = i;		/* save 1st zone found */
				}
		}
		if (home) 
			zonep[j].zone_home = TRUE;
		else {
			error = TRUE; 
			fprintf(stderr, MSGSTR(M_NO_HOME_ZN,
				"error, no home zone designated\n")); 
		}
	}
	fclose(pFin);
	return(error? -1:0);
} /* getZoneInfo */

int getIFType(ifname)
     char *ifname;

/* checks  the interface for a valid type, returns the if Type number
   as defined by IF_TYPENO_xx 
*/
{
	int ifType;
	int i;

	for (ifType=0; ifType<IF_TYPENO_CNT; ifType++) {
	 	if (!strncmp(if_types[ifType], ifname, strlen(if_types[ifType]))) {
		  /* a valid type, check unit number */
			if (sscanf(ifname+strlen(if_types[ifType]),"%d",&i) == 1)
				return(ifType);
			else
				return(-1);
		} else
			continue;
	}
	return(-1);
} /* getIFType */

int ifcompare(v1,v2)
void *v1, *v2;
{
	return(strcmp(((at_elap_cfg_t*)v1)->if_name, ((at_elap_cfg_t*)v2)->if_name));
}

void showCfg(cfgp, filecfgp, if_zones)
at_elap_cfg_t	*cfgp;
if_cfg_t	*filecfgp;
if_zone_info_t if_zones[];         /* zone info from cfg file */
{
	int i,j,k, cnt=0, seed=FALSE;
	char range[40];
	qsort((void*)cfgp, IF_TOTAL_MAX, sizeof(*cfgp), ifcompare);
	for (i=0; i<IF_TOTAL_MAX; i++) {
		if (cfgp[i].if_name[0]) {
			if (!cnt++) {
				fprintf(STDOUT, 
					MSGSTR(M_RTR_CFG,"Router mode configuration:\n\n"));
				fprintf(STDOUT,
					MSGSTR(M_RTR_CFG_HDR,"H I/F  Network Range\n"));
				fprintf(STDOUT,"- ---  -------------\n");
			}
			range[0] = '\0';
			if (cfgp[i].netStart || cfgp[i].netEnd) {
				sprintf(range,"%5d - %d", cfgp[i].netStart, cfgp[i].netEnd); 
				seed=TRUE;
			}
			fprintf(STDOUT,"%c %-3s  %s\n", cfgp[i].flags & ELAP_CFG_HOME ? '*' : ' ',
				cfgp[i].if_name,
				range[0] ? range : MSGSTR(M_NONSEED,"(non-seed)"));
		}
	}
	if (!cnt) {
		fprintf(STDOUT, 
			MSGSTR(M_NO_IF_CFG, 
			"There are no interfaces configured\n"));
		return;
	}
	if (seed) {
		fprintf(STDOUT, "\n\n  %-32s  %s\n", 
			MSGSTR(M_ZONE_DEF,"Defined zones"),
			MSGSTR(M_IF_DEF_ZONE,"Interfaces Defining Zone"));
			fprintf(STDOUT,"  %-32s  %s\n",
    					 	   "-------------",
					   "------------------------");
	}

	for (i=0; i<MAX_ZONES && seed; i++)  {
		int ifcnt=0;
		if (!if_zones[i].zone_name.str[0])
			break;
		fprintf(STDOUT, "%c %-32s  ", if_zones[i].zone_home ? '*' : ' ',
			if_zones[i].zone_name.str);
		for (j=0; j<IF_TYPENO_CNT; j++) {
			for (k=0; k<IF_ANY_MAX; k++)
				if (if_zones[i].zone_ifs[j] &1<<k) {
					if (ifcnt && !((ifcnt)%ZONE_IFS_PER_LINE))
						fprintf(STDOUT,"\n%36s","");
					ifcnt++;
					fprintf(STDOUT, "%s%c ",if_types[j],
						   k<=9 ? '0' + k : 'a' + k);
				}
			}
		fprintf(STDOUT, "\n");
	}
	fprintf(STDOUT, 
		MSGSTR(M_HOME_Z_IND,
		"\n* indicates home port and home zone\n"\
  	      "  (if home port is a seed port)\n"));
		
}


int checkSeeds(elapcfgp,zonep)
     at_elap_cfg_t elapcfgp[];
     if_zone_info_t zonep[];
{
	int i,j;
	int	ifType;
	int	unitNo;
	int	ok;
	int	error=FALSE;

	for (i=0; i<IF_TOTAL_MAX; i++) {
		if (!elapcfgp[i].if_name[0])	
			break;
		if (!elapcfgp[i].netStart)		/* if no seed, skip */
			continue;
	 	ifType = getIFType(elapcfgp[i].if_name);
		unitNo = IF_NO(elapcfgp[i].if_name);
		ok = FALSE;
		for (j=0;j<MAX_ZONES; j++) {
			if (!zonep[j].zone_name.str[0])
				break;
			if (zonep[j].zone_ifs[ifType] & 1<<unitNo) {
				ok = TRUE;
				break;
			}
		} /* for each zone */
		if (!ok) {
			fprintf(stderr, MSGSTR(M_SEED_WO_ZONE,
				"error, seed I/F without any zones: %s\n"),elapcfgp[i].if_name);
			error = -1;
		}
	}     /* for I/F type */
	return(error);
} /* checkSeeds */


static void printOnlineError(error)
int error;
{
	switch (error) {
	case EEXIST :
		fprintf(stderr, MSGSTR(M_MULTIPLE_HOME,
			"another home port already designated\n"));
		break;
	case EACCES :
		fprintf(stderr, MSGSTR(M_PERM, "permission denied\n"));
		break;
	case EPERM :
		fprintf(stderr, MSGSTR(M_PORT_UP,
			"port already up, can't designate as home port\n"));
		break;
	case EINVAL :
		fprintf(stderr, MSGSTR(M_INVALID_IF,"invalid interface specified\n"));
		break;
	case EFAULT :
		fprintf(stderr, MSGSTR(M_RANGE_CHG,
			"can't change range in current i/f state\n"));
		break;
	case ECHRNG :
		fprintf(stderr, MSGSTR(M_PKT_REG, "error registering packet type\n"));
		break;
	case EALREADY :
	  	fprintf(stderr, MSGSTR(M_IF_UP,"interface is already running\n"));
	}
} /* printOnlineError */

#define STATS_HEADER1 \
MSGSTR(M_AT_STATS1,\
"\n-------- Appletalk Configuration -----------\n")
#define STATS_HEADER2 \
MSGSTR(M_AT_STATS2,\
"                             Network:\n")
#ifdef DEV_VERSION
#define STATS_HEADER3 \
MSGSTR(M_AT_STATS3,\
" I/F   State            Range       Node      Port Default Zone\n")
#define STATS_HEADER4 \
" ---- ----------------- ----------- --------- ---- -------------------------\n"
#else /* DEV_VERSION */
#define STATS_HEADER3 \
MSGSTR(M_AT_STATS3,\
" I/F  State             Range       Node      Default Zone\n")
#define STATS_HEADER4 \
" ---- ----------------- ----------- --------- -------------------------\n"
#endif /* DEV_VERSION */
int
showRouterStats()
{
	int	status;
	int size;
	int if_id;
	int i, ift, ifn;
	int error = FALSE;
	int	online = 0;
	int mode;
	static if_cfg_t syscfg;		/* system cfg info */
	char zone[64];
	at_if_name_t ifIN;
	char *ifState[8];

	ifState[LAP_OFFLINE]         = MSGSTR(M_OFFLINE,"Offline");
	ifState[LAP_ONLINE]          = MSGSTR(M_ONLINE,"Online");
	ifState[LAP_ONLINE_FOR_ZIP]  = MSGSTR(M_ONLINE_ZIP,"Online for Zip");
	ifState[LAP_HANGING_UP]      = MSGSTR(M_HANGINUP,"Hanging Up");
	ifState[LAP_ONLINE_ZONELESS] = MSGSTR(M_ONLINE_ZLESS,"Online, zoneless");


	if ((if_id = openCtrlFile(&syscfg, "showRouterStats",0, 0)) < 0 )
		return(-1);
	size=0;
	if (at_send_to_dev(if_id, LAP_IOC_GET_MODE, &mode, &size)) {
		(void)close (if_id);
		return(-1);
	}

	for (ift=0; ift<IF_TYPENO_CNT; ift++) {
		if (error)
			break;
		for (ifn=0; ifn<IF_ANY_MAX; ifn++) {
			if (!(syscfg.avail[ift] & 1<<ifn))
				continue;
			size = sizeof(ifIN);
			sprintf(ifIN.if_name,"%s%d",if_types[ift],ifn);
			if (status = at_send_to_dev(if_id, LAP_IOC_GET_IFID, &ifIN, &size)) {
				if (cthread_errno() != EINVAL)
					perror(MSGSTR(M_IFID, "error getting IFID from kernel"));
				continue;
			}
			if (!online++)
				fprintf(STDOUT, "%s%s%s%s",
					STATS_HEADER1,
					STATS_HEADER2,
					STATS_HEADER3,
					STATS_HEADER4);
			i = ifIN.ifID.ifState & 0xFF;
			fprintf(STDOUT, "%c%s%-2d %-18s",
				(ifIN.ifID.ifFlags & AT_IFF_DEFAULT &&
					mode == AT_MODE_ROUTER) ? '*' : ' ',
				if_types[ift],ifn,ifState[i]);
			if (ifIN.ifID.ifState != LAP_OFFLINE) {
				fprintf(STDOUT, "%5d-%-5d %5d:%-3d ", 
					ifIN.ifID.ifThisCableStart,
					ifIN.ifID.ifThisCableEnd,
					NET_VALUE(ifIN.ifID.ifThisNode.atalk_net),
					ifIN.ifID.ifThisNode.atalk_node&0xff);
#ifdef DEV_VERSION
				fprintf(STDOUT, "%2d   ", ifIN.ifID.ifPort);	
#endif /* DEV_VERSION */
			if (ifIN.ifID.ifFlags & AT_IFF_DEFAULT || 
				mode == AT_MODE_MHOME)  {
/*
 * display the zone name for the home port
 */
				strncpy((char *) zone, (char *) ifIN.ifID.ifZoneName.str,
					 ifIN.ifID.ifZoneName.len);
				zone[ifIN.ifID.ifZoneName.len] = '\0';
				fprintf(STDOUT, "%s", zone);
			}

				
			}
			fprintf(STDOUT, "\n"); 
		}

	}
	if (!online) {
		fprintf(STDOUT, MSGSTR(M_NO_IFS_AT,
			"No interfaces currently running Appletalk\n"));
		error = TRUE;
	}
	(void) close(if_id);
	return(error ? -1 : 0);
}


int
showZones()
{
	int status;
	int size;
	int if_id;
	int done = FALSE;
	ZT_entryno zte;
	int did_header=0;

	if ((if_id = openCtrlFile(NULL, "showZones", MIN_MAJOR, MIN_MINOR)) < 0)
		return(-1);

	*(int *)&zte = 1;
	while (1) {
		size = sizeof(ZT_entryno);
		status = at_send_to_dev(if_id, LAP_IOC_GET_ZONE, &zte, &size);
		if (status <0)
			switch (cthread_errno()) {
				case ENOMSG:
					done = TRUE;
					break;
				case 0:
					break;
				default:
					fprintf(stderr, MSGSTR(M_RET_ZONES,
						"showZones: error retrieving zone list\n"));
					goto error;
			}
		if (done)
			break;
		
		if (!did_header++) {
			fprintf(STDOUT, MSGSTR(M_ZONES,"..... Zones ......\n"));
			fprintf(STDOUT, MSGSTR(M_ZONE_HDR,"zno zcnt zone\n"));
		}
		zte.zt.Zone.str[zte.zt.Zone.len] = '\0';
		fprintf(STDOUT, "%3d  %3d %s\n", zte.entryno+1,zte.zt.ZoneCount, zte.zt.Zone.str);
		*(int *)&zte = 0;
	}
	if (*(int *)&zte == 1)
		fprintf(STDOUT, MSGSTR(M_NO_ZONES,"no zones found\n"));
	(void) close(if_id);
	return(0);
error:
	(void) close(if_id);
	return(-1);
}

int
showRoutes()
{
	int status;
	int size;
	int if_id;
	int i,j;
	int zcnt,gap;
	int done = FALSE;
	int did_header = FALSE;
	RT_entry rt;
	char state[10];

	if ((if_id = openCtrlFile(NULL, "showRoutes", MIN_MAJOR, MIN_MINOR)) < 0)
		return(-1);

	*(int *)&rt = 1;
	state[9] = '\0';
	while (1) {
		size = sizeof(RT_entry);
		status = at_send_to_dev(if_id, LAP_IOC_GET_ROUTE, &rt, &size);
		if (status < 0)
			switch (cthread_errno()) {
				case ENOMSG:
					done = TRUE;
					break;
				case 0:
					break;
				default:
					fprintf(STDOUT, MSGSTR(M_RET_ROUTES,
						"showRoutes: error retrieving route table\n"));
					goto error;
			}
		if (done)
			break;
		if (!did_header++)	{
			fprintf(STDOUT, MSGSTR(M_ROUTES,
				"............ Routes ................\n"));
			fprintf(STDOUT, MSGSTR(M_NXT_STATE,
				"                next             state\n"));
			fprintf(STDOUT, MSGSTR(M_RTR_HDR,
				"start-stop    net:node   d  p  PBTZ GSBU zones\n"));
		}
		gap = 0;
		for (i=0; i<8; i++) {
			if (i==4) {
				gap =1; 
				state[4] = ' ';
			}
			state[i+gap] =  (rt.EntryState & 1<<(7-i)) ? '1' : '0';
		}
		fprintf(STDOUT, "%5d-%-5d %5d:%-05d %2d %2d  %s ", rt.NetStart, rt.NetStop,
			rt.NextIRNet, rt.NextIRNode, rt.NetDist, rt.NetPort,state);
		zcnt = 0;
		for (i=0; i<ZT_BYTES; i++) {
			for (j=0; j<8; j++)
				if ((rt.ZoneBitMap[i] <<j) & 0x80) {
					if (zcnt >= Z_MAX_PRINT) { 
						fprintf(STDOUT, MSGSTR(M_MORE,",more..."));
						i = ZT_BYTES;
						break;
					}
					fprintf(STDOUT, zcnt ? ",%d" : " %d",i*8+j+1);	/* 1st zone is 1 not 0 */
					zcnt++;
				}
		}
		fprintf(STDOUT, "\n");
		*(int *)&rt = 0;
	}
	if (*(int *)&rt == 1)
		fprintf(STDOUT, MSGSTR(M_NO_ROUTES,"no routes found\n"));
	(void) close(if_id);
	return(0);
error:
	(void) close(if_id);
	return(-1);
}

int changeRoutingMix(router_mix)
     short router_mix;
{
	int if_id, status, size;

	if ((if_id = openCtrlFile(NULL, "changeMix", MIN_MAJOR, MIN_MINOR)) < 0)
		return(-1);

	size = sizeof(short);
	status = at_send_to_dev(if_id, LAP_IOC_SET_MIX, &router_mix, &size);
	if (status < 0) {
		perror(MSGSTR(M_CHG_MIX,
			      "changeRouterMix: error sending new mix to kernel\n"));
		(void) close(if_id);
		return(-1);
	}
	return(0);
} /* changeRoutingMix */

int
openCtrlFile(cfg, calling_fn, min_major, min_minor)
     if_cfg_t	*cfg;
     char	*calling_fn;	/* name of calling fn (for error msgs) */
     int	min_major,min_minor;	/* minimum kernel versions required
					   or 0,0 if no version checking 
					   (cfg must be non-NULL) */

/* opens control file and if cfg is non-zero, retrieves system config and
   writes it to cfg.
   returns handle if file opened ok
   returns -1 if error occured opening control file

   if cfg is non-zero and there is an error retrieveing the config info, 
   the control file will be closed anr we will return (-1)
*/
{
	int	status;
	int size;
	int if_id;

	if ((if_id = ATsocket(ATPROTO_LAP)) <= 0) {
		fprintf(stderr, MSGSTR(M_CTRL_FILE,
			"%s: error opening ctrl file:%s \n"), calling_fn,
			"atalk");
		fprintf(stderr, MSGSTR(M_NOT_LOADED,
			"perhaps the Appletalk stack is not loaded\n"));
		return(-1);
	}
	if (!cfg)
		return(if_id);
	size = 0;
	
	/* get valid I/F's & status of each */
	if (status = at_send_to_dev(if_id, LAP_IOC_GET_IFS_STAT, cfg, &size)) {
		fprintf(stderr, MSGSTR(M_IFS_STAT,
			"%s:error getting IFS_STAT from kernel\n"), calling_fn);
		close(if_id);
		return(-1);
	}
	if ( cfg->ver_major <= min_major) {
		if (cfg->ver_major < min_major || cfg->ver_minor < min_minor) {
			fprintf(stderr,MSGSTR(M_OLD_KERNEL,
				"kernel version too old (%d.%02d), %d.%02d or later is required\n"),
				cfg->ver_major, cfg->ver_minor, min_major, min_minor);
			close(if_id);
			return(-1);
		}
	}
	return(if_id);
} /* openCtrlFile */

int	readxpram(buf, count, offset)
	char *buf;
	int count;
	int offset;
{
	int fd;

	if ((fd = open(PRAM_FILE, O_RDONLY)) == -1) {
		return(-1);
	}

	if ((int)lseek(fd, (off_t)offset, SEEK_SET) != offset) {
		close(fd);
		return(-1);
	}
/*	printf("readxpram: read %s %d bytes offset %d\n", 
	       PRAM_FILE, count, offset);
*/
	if (read(fd, buf, count) != count) {
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

int	writexpram(buf, count, offset)
	char *buf;
	int count;
	int offset;
{
	int fd;
	char buffer[NVRAMSIZE];

	if ((fd = open(PRAM_FILE, O_RDWR)) == -1) {
		if (cthread_errno() == ENOENT) {
			fd = open(PRAM_FILE, O_RDWR|O_CREAT, 0644);
			if (fd == -1)
				return(-1);
			memset(buffer, 0, sizeof(buffer));
			if (write(fd, buffer, sizeof(buffer)) 
			    != sizeof(buffer)) {
				close(fd);
 				unlink(PRAM_FILE);
				return(-1);
			}
			(void) lseek(fd, (off_t)0, 0);
			
		}
		else
			return(0);
	}

	if ((int)lseek(fd, (off_t)offset, SEEK_SET) != offset) {
		close(fd);
		return(-1);
	}
/*	printf("writexpram: wrote %s %d bytes offset %d\n", PRAM_FILE, 
	       count, offset);
*/
	if (write(fd, buf, count) != count) {
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

int atalkInit(dname)
    char **dname;
{
	int i, rc, len;
	int lap_fd = -1, tp_fd;

	if (dname[0]) {
	   /* open a LAP channel */
	   if ((lap_fd = ATsocket(ATPROTO_LAP)) == -1) {
	      perror(MSGSTR(M_OPEN_LAP, "atalkUp: open lap"));
	      return -1;
	   }

	   for (i=0; dname[i] != NULL; i++) {
		/* add device name for LAP */
		len = strlen(dname[i])+1;
		if ((rc = at_send_to_dev(lap_fd, LAP_IOC_ADD_IFNAME, 
					 dname[i], &len)) == -1) {
		    fprintf(stderr, "%s\n", dname[i]);
		    perror(MSGSTR(M_ADD_IF, "atalkUp: LAP_IOC_ADD_IFNAME"));
		    close(lap_fd);
		    return -1;
		}
	   }

	   if ((tp_fd = ATsocket(ATPROTO_ATP)) == -1) {
	     	perror(MSGSTR(M_OPEN_TP, "atalkUp: open tp"));
		close(lap_fd);
		return -1;
	   } else {
	     	if (at_send_to_dev(tp_fd, AT_ATP_LINK, 0, 0) == -1) {
		  	perror(MSGSTR(M_ILINK_TP, "atalkInit: AT_ATP_LINK"));
			close(tp_fd);
			close(lap_fd);
			return -1;
		}
	   }
	   close(tp_fd);
	   if ((tp_fd = ATsocket(ATPROTO_ADSP)) == -1) {
	     	perror(MSGSTR(M_OPEN_TP, "atalkUp: open tp"));
		atalkUnlink();
		close(lap_fd);
		return -1;
	   } else {
	     	if (at_send_to_dev(tp_fd, AT_ADSP_LINK, 0, 0) == -1) {
			perror(MSGSTR(M_ILINK_TP, "atalkInit: AT_ATP_LINK"));
			close(tp_fd);
			atalkUnlink();
			close(lap_fd);
			return -1;
		}
	   }
	   close(tp_fd);
	}
	return (lap_fd);
} /* atalkInit */

void atalkUnlink()
{
	int tp_fd;

	if ((tp_fd = ATsocket(ATPROTO_ATP)) != -1) {
	  	(void)at_send_to_dev(tp_fd, AT_ATP_UNLINK, 0, 0);
		close(tp_fd);
	}
	if ((tp_fd = ATsocket(ATPROTO_ADSP)) != -1) {
	  	(void)at_send_to_dev(tp_fd, AT_ADSP_UNLINK, 0, 0);
		close(tp_fd);
	}
} /* atalkUunlink */

/*
 * Name: atalkState()
 */
int atalkState(flag, pid_buf)
	char flag;
	char **pid_buf;
{
	int if_id, rc;
	int size = 1;
	int i, j, k, pid;

	*pid_buf = 0;
	*((char *)&stateBuff[0]) = flag;

	if ((if_id = openCtrlFile(NULL, "atalkState",0,5)) < 0)
		return -1;
	rc = at_send_to_dev(if_id, LAP_IOC_CHECK_STATE, stateBuff, &size);
	close(if_id);
	size = size/sizeof(int);
	if (size >= 1) {
		stateBuff[size] = 0;
		for (i=0, j=0; i < size; i++) {
			pid = stateBuff[i];
			for (k=0; k < j; k++) {
				if (pid == pidBuff[k]) {
					pid = 0;
					break;
				}
			}
			if (pid)
				pidBuff[j++] = pid;
		}
		pidBuff[j] = 0;
		*pid_buf = (char *)pidBuff;
		SET_ERRNO(EBUSY);
	}
	return rc;
}

static	int	nbp_strcmp (str1, str2)
register at_nvestr_t	*str1, *str2;
{
	u_char	        *c1,*c2;
	register int	i;
	at_nvestr_t cp1,cp2;

	 /* returns 0 if two strings are equal (modulo case), -1 otherwise 
	 */
	
	if (str1->len == 0 || str2->len == 0) {
		return (-1);
	}	
	cp1=*str1;
	cp2=*str2;
	nbp_upshift(cp1.str,cp1.len);
	nbp_upshift(cp2.str,cp2.len);
	c1 = cp1.str;
	c2 = cp2.str;
	for (i= 0; i < cp1.len ; i++) { 

		if (*c1++ != *c2++) 
			return (-1);
	}

	return (0);
}

static	void	nbp_upshift (str, count)
register u_char	*str;
register int	count;
{
	register int	i, j;
	register u_char	ch;
	static	unsigned char	lower_case[] =
		{0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0xbe,
		 0xbf, 0xcf, 0x9b, 0x8b, 0x88, 0};
	static	unsigned char	upper_case[] = 
		{0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xae,
		 0xaf, 0xce, 0xcd, 0xcc, 0xcb, 0};

	for (j=0 ; j<count ; j++) {
		ch = str[j];
		if (ch >= 'a' && ch <= 'z')
			str[j] = ch + 'A' - 'a';
		else if (ch & 0x80)
			for (i=0; lower_case[i]; i++)
				if (ch == lower_case[i])
					str[j] = upper_case[i];
	}
}

int move_file(src, dst)
char *src,*dst;

/* moves file accross file systems (where rename won't work)
*/
{
	FILE *fin, *fout;
	static char buf[8192];
	int i;

	fin = fopen(src, "r");
	fout = fopen(dst, "w");

	if (!fin || !fout)
		return(-1);
	while (i=fread(buf,1,sizeof(buf),fin)) 
		fwrite(buf,1,i,fout);
	fclose(fin);
	fclose(fout);
	unlink(src);
	return(0);
}

setDefaultZones(mode, quiet, pCfg, if_zones)
     int mode;		/* stack mode */
     int quiet;		/* quiet mode, (don't prompt for zones) */
     char *pCfg;       	/* configuration file */
     if_zone_info_t if_zones[];         /* zone info from cfg file */
/* get zone selection from user for all MH ports or just home port
   if router mode and send them down to kernel. Update config file or
   nvram (for router) accordingly
*/

{
	if_name_t names[IF_TOTAL_MAX], *pn;
	static if_zone_t zones[MAX_ZONES], *pz;
	int 	if_id, status, zcnt,zno,size, x,j,k,ifno;
	int		defZones[IF_TOTAL_MAX];
	char str[35], buf[20], linebuf[50];
	int		validZones;			/* zone count for current I/F */
	int		gotZone, ifType;
	char  	tmpTmp[L_tmpnam];	
	char  	tmpCfg[L_tmpnam];	
	FILE	*fCfg=NULL, *fCfg1=NULL,*fTmp;
	at_nvestr_t nvramzone;
	nvramzone.len=0;
	if ( mode == AT_ST_ROUTER) 
		get_zone_name(&nvramzone);

	if ( mode == AT_ST_MULTIHOME) {
		fCfg = fopen(pCfg, "r");
		if (fCfg) {
			tmpnam(tmpTmp);
			tmpnam(tmpCfg);
			if (!(fTmp = fopen(tmpTmp, "w+"))){
				fclose(fCfg);
				fCfg=NULL;
			}
			else {
				if(!(fCfg1 = fopen(tmpCfg, "w"))) {
					fclose(fCfg);
					fclose(fTmp);
					fCfg=NULL;
					fTmp=NULL;
				}
			}
		}
	}
	size = sizeof(if_zone_t);
	if ((if_id = openCtrlFile(NULL, "setDefaultZones",2,0)) < 0)
			return(-1);
	for (zno=zcnt=0; zcnt< MAX_ZONES; zno++) {
		zones[zcnt].ifzn.zone = zno;
		status = at_send_to_dev(if_id, LAP_IOC_GET_LOCAL_ZONE, 
			&zones[zcnt], &size);
		if (status) {
			fprintf(stderr, MSGSTR(M_RET_ZONES1, "error retrieving zones\n"));
			close(if_id);
			return(-1);
		}
		if (zones[zcnt].ifzn.ifnve.len == 0)
			break;
				/* if router mode & zone not for home i/f, don't count it */
		if ( mode == AT_ST_ROUTER  && !zones[zcnt].usage[0])
			continue;
		zones[zcnt].ifzn.ifnve.str[zones[zcnt].ifzn.ifnve.len] = '\0';
#ifdef COMMENTED_OUT 
		printf("i/f usage for %s:\n%d %d %d %d\n",
			zones[zcnt].ifzn.ifnve.str,
			zones[zcnt].usage[0],
			zones[zcnt].usage[1],
			zones[zcnt].usage[2],
			zones[zcnt].usage[3]);
#endif /* COMMENTED_OUT */
	 zcnt++;
	}
	size = 0;
	status = at_send_to_dev(if_id, LAP_IOC_GET_IF_NAMES,
		names, &size);
	if (status) {
		fprintf(stderr, MSGSTR(M_RET_IF,"error retrieving I/F names\n"));
		close(if_id);
		return(-1);
	}
	linebuf[0] = '\0';
	for(ifno=0, pn=names; pn[0][0]; ifno++, pn++) {
		for (j=0,k=0,pz=zones; k<=zcnt; k++,pz++ ) {
			if ( pz->usage[ifno]) {
				if (!quiet) {
					strncpy(str,pz->ifzn.ifnve.str,pz->ifzn.ifnve.len);
					str[pz->ifzn.ifnve.len] = '\0';
					if (j>=1)
						fprintf(STDOUT,"%s",linebuf);
					sprintf(linebuf,"%s%2d: %s\n",j==0 ? "\n" : "",j+1,str);
				}
				j++;
			}
		}
		validZones=j;
		if ( !validZones ) {
			fprintf(stderr, 
				"No zones remaining for interface %s\n", pn);
			SET_ERRNO(ENOENT);
			return(-1);
		}
			
		if(!quiet || !ifno) { /* print zone only if more than one choice */
			if (validZones > 1)
				fprintf(STDOUT,"%s",linebuf);
			fprintf(STDOUT,"\n");
		}
		x=1;
		if (validZones > 1 && !quiet) {	/* if more than one zone & not quiet */
			do {
				fprintf(STDOUT,MSGSTR(M_SEL_ZN1,
					"select default zone for interface %s:"),pn);
				gets(buf);
				if (!buf[0])
					continue;
				if ((x = atoi(buf)) < 1 || x > j)
					continue;
				break;
			} while(1);
		}
		gotZone=FALSE;
						/* auto-select zone from cfg file */
		if (quiet && validZones > 1 &&
			(ifType = getIFType(pn)) != -1) {
			int i;
			for (i=0; !gotZone && i<zcnt; i++) {
				if ( mode == AT_ST_ROUTER) {
					if (!nbp_strcmp(&zones[i].ifzn.ifnve,&nvramzone)) {
			   			gotZone=TRUE;
						x = i+1;
			   			break;
					}
				}
				if(if_zones[i].zone_ifs[ifType] & (1<<IF_NO(pn[0]))) {
					for (k=0,pz=zones; k<=zcnt; k++,pz++ ) {
						if ( pz->usage[ifno] ) {
							if (!nbp_strcmp(&pz->ifzn.ifnve, 
								&if_zones[i].zone_name)) {
						   		gotZone=TRUE;
						   		break;
							}
						}
					}
				}
			}
		}
					

		if (!gotZone || (gotZone && mode == AT_ST_ROUTER))
			for (j=1,k=0,pz=zones; k<=zcnt; k++,pz++ ) {
				if ( pz->usage[ifno]) 
					if (j++ == x)
						break;
			}
		strncpy(str,pz->ifzn.ifnve.str,pz->ifzn.ifnve.len);
		str[pz->ifzn.ifnve.len] = '\0';
		if (quiet || validZones == 1) {
			fprintf(STDOUT,MSGSTR(M_DEF_SET_TO,
				"default zone for %s set to \"%s\"\n"),pn,str);
		}
		if (fTmp) 
			fprintf(fTmp,":%s:%s\n",str,pn);

		defZones[ifno] = pz->index;
		memset(pz->usage,0,sizeof(pz->usage));	/* prevent dupe default zones */
		if ( mode == AT_ST_ROUTER)
			break;

	}

	if (fCfg1) {		/* update config file with new selections */
		char buf[128];
		rewind(fTmp);
		while(!feof(fCfg)) {
			if(fgets(buf,sizeof(buf),fCfg)) 
				if (buf[0] != ':')
					fputs(buf,fCfg1);
		}
		fclose(fCfg);
		while(!feof(fTmp)) {
			if(fgets(buf,sizeof(buf),fTmp)) 
				fputs(buf,fCfg1);
		}

		fclose(fCfg1);
		fclose(fTmp);
		unlink(tmpTmp);
		unlink(pCfg);
		move_file(tmpCfg, pCfg);
	}
	size = sizeof(defZones);
	status = at_send_to_dev(if_id, LAP_IOC_SET_DEFAULT_ZONES,
		defZones, &size);
	if (status) {
		fprintf(stderr, MSGSTR(M_ERR_SET_Z,"error setting default zones\n"));
		close(if_id);
		return(-1);
	}
	close(if_id);
	return(0);
} /* setDefaultZones */

void aurpd(fd)
	int fd;
{
	ATgetmsg(fd, 0, 0, 0);
}
