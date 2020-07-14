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
 *	Copyright (c) 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* @(#)lap.h: 2.0, 1.4; 2/16/90; Copyright 1988-89, Apple Computer, Inc. */

/* Definitions for generic access to AppleTalk link level protocols.
 */

#ifndef __LAP__
#define __LAP__
#include <h/if_cnt.h>
#include <at/nbp.h>




	/* struture containing general information regarding the state of
	 * the Appletalk networking 
	 */
typedef struct at_state {
	unsigned int	flags;		/* various init flags (INIT_FL_XXX */
	int			pats_online;	/* count of pat's that are online */
	int			ifs_online;		/* count of all i/fs that are online */
	int			home_port;		/* the ddp logical port, or -1 if not set */
} at_state_t;

	/*  at_state_t 'flags' defines */
#define AT_ST_PAT_INIT		0x0001	/* pat init done */
#define AT_ST_DDP_INIT 		0x0002	/* ddp initialized */
#define AT_ST_ELAP_INIT		0x0004	/* elap initialized */
#define AT_ST_MULTIHOME		0x0080	/* set if  multihome mode */
#define AT_ST_ROUTER		0x0100	/* set if we are a router */
#define AT_ST_IF_CHANGED	0x0200	/* set when state of any I/F
									   changes (for SNMP) */
#define AT_ST_RT_CHANGED	0x0400  /* route table changed (for SNMP)*/
#define AT_ST_ZT_CHANGED 	0x0800  /* zone table changed (for SNMP) */
#define AT_ST_NBP_CHANGED   0x1000  /* if nbp table changed (for SNMP)*/


	/* mode defines */
#define AT_MODE_SPORT			1		/* single port */
#define AT_MODE_MHOME			2		/* multi-homing */
#define AT_MODE_ROUTER			3		/* router */

	/* GET_ZONES defines */
#define GET_ALL_ZONES 			0
#define GET_LOCAL_ZONES_ONLY 	1

struct atalk_options {		/* for appletalk command */
	int     router;			/* if true, tells lap_init we're a router */
	int		t_option;		/* for development, test cfg w/o stack loaded */
	int     start;          /* cable start */
	int     end;            /* cable end */
	char	*f_option;		/* optional alternate router.cfg file */
	short	c_option;		/* if true, just check config file */
	short	e_option;		/* display configuration only */
	short	r_table;		/* optional RTMP table size param */
	short	z_table;		/* optional ZIP table size param */
	short	h_option;		/* multihoming mode of router */
	short   q_option;		/* run quiet, don't ask for zones */
};

	/* I/F status structure, returned from kernel to indicate status of stack
	   the each element of the avail and stat arrays is assigned to one I/F type
	   as defined above. For each type there is one bit assigned to each physical
	   I/F with the lsb equal to if0 (e.g. et0) through the msb equal to if32 .
	   If a bit is set in the avail element, then that I/F exists in the system. 
	   the state element contains the at_if_t.ifState value
	   To indicate the home port there are 2 elements, home_type (same as above
	   defines) and the home_number (the n in etn).
	 */
typedef struct if_cfg {
	unsigned 	avail[IF_TYPENO_CNT];
	char    	state[IF_TYPENO_CNT][IF_ANY_MAX];
	unsigned	max[IF_TYPENO_CNT];
	unsigned	home_type;		/* type of home interface */
	unsigned	home_number;
	unsigned	ver_major;		/* major & minor version no's (m.n) */
	unsigned	ver_minor;
	unsigned	ver_date;		/* version date (YYMMDD) */
	char		comp_date[50];	/* compile date & time string */
} if_cfg_t;

typedef struct if_zone_info {
	at_nvestr_t	zone_name;						/* the zone name & len */
	unsigned	zone_ifs[IF_TYPENO_CNT];		/* bitmapped I/F usage for zone */
	unsigned	zone_home;						/* TRUE for home zone */
} if_zone_info_t;

typedef union if_zone_nve {
	at_nvestr_t		ifnve;
	int				zone;
} if_zone_nve_t;

		/* this struct used to obtain local zones for specific
		   ifID's from the kernel  and to set default zones for
		   specific ifID numbers */
typedef struct if_zone {
	if_zone_nve_t	ifzn;
	char			usage[IF_TOTAL_MAX];	/* I/F usage (1 set if
									           I/F in this zone */
	int				index;					/* zone index in ZT_table */
} if_zone_t;

		/* struct for multihoming config info read from file */
typedef struct mh_cfg {
	char		ifName[IF_NAME_LEN];/* interface name (e.g.et0) */
	at_nvestr_t	zone;				/* default zone */
} mh_cfg_t;

#define	IFID_HOME			0 		/* home port in ifID_table */



#define	ATALK_VALUE(a)		((*(u_long *) &(a))&0x00ffffff)

#define	ATALK_EQUAL(a, b)	(ATALK_VALUE(a) == ATALK_VALUE(b))

#define ATALK_ASSIGN(a, net, node, unused ) \
		a.atalk_unused = unused; a.atalk_node = node; NET_ASSIGN(a.atalk_net, net)

#define NO_IFS	4

#define AT_IFF_IFMASK		0xffff	/* lower 16 bits define interface type*/
#define	AT_IFF_LOCALTALK	0x1	/* This is a LocalTalk interface */
#define	AT_IFF_ETHERTALK	0x2	/* This is an EtherTalk interface */
#define AT_IFF_DEFAULT		0x40000
#define AT_IFF_AURP		0x20000

/* Generic LAP ioctl's.  Each LAP may implement other ioctl's specific to
 * its functionality.
 */
#define	LAP_IOC_MYIOCTL(i)	  	((i>>8) == AT_MID_LAP)
#define	LAP_IOC_ONLINE		  	((AT_MID_LAP<<8) | 1)
#define	LAP_IOC_OFFLINE		  	((AT_MID_LAP<<8) | 2)
#define	LAP_IOC_GET_IFS_STAT  	((AT_MID_LAP<<8) | 3)
#define	LAP_IOC_ADD_ZONE  	  	((AT_MID_LAP<<8) | 4)
#define	LAP_IOC_ROUTER_START 	((AT_MID_LAP<<8) | 5)
#define	LAP_IOC_ROUTER_SHUTDOWN ((AT_MID_LAP<<8) | 6)
#define	LAP_IOC_ROUTER_INIT     ((AT_MID_LAP<<8) | 7)
#define	LAP_IOC_GET_IFID	    ((AT_MID_LAP<<8) | 8)
#define	LAP_IOC_ADD_ROUTE	   	((AT_MID_LAP<<8) | 9)
#define	LAP_IOC_GET_DBG			((AT_MID_LAP<<8) | 10)
#define	LAP_IOC_SET_DBG			((AT_MID_LAP<<8) | 11)
#define	LAP_IOC_GET_ZONE		((AT_MID_LAP<<8) | 12)
#define	LAP_IOC_GET_ROUTE		((AT_MID_LAP<<8) | 13)
#define	LAP_IOC_ADD_IFNAME		((AT_MID_LAP<<8) | 14)
#define	LAP_IOC_DO_DEFER		((AT_MID_LAP<<8) | 15)
#define	LAP_IOC_DO_DELAY		((AT_MID_LAP<<8) | 16)
#define	LAP_IOC_SHUT_DOWN		((AT_MID_LAP<<8) | 17)
#define	LAP_IOC_CHECK_STATE		((AT_MID_LAP<<8) | 18)
#define	LAP_IOC_DEL_IFNAME		((AT_MID_LAP<<8) | 19)
#define	LAP_IOC_SET_MIX			((AT_MID_LAP<<8) | 20)
#define LAP_IOC_SNMP_GET_CFG    ((AT_MID_LAP<<8) | 21)
#define LAP_IOC_SNMP_GET_AARP   ((AT_MID_LAP<<8) | 22)
#define LAP_IOC_SNMP_GET_RTMP	((AT_MID_LAP<<8) | 23)
#define LAP_IOC_SNMP_GET_ZIP	((AT_MID_LAP<<8) | 24)
#define LAP_IOC_SNMP_GET_DDP	((AT_MID_LAP<<8) | 25)
#define LAP_IOC_SNMP_GET_NBP	((AT_MID_LAP<<8) | 26)
#define LAP_IOC_SNMP_GET_PORTS	((AT_MID_LAP<<8) | 27)
#define LAP_IOC_SET_LOCAL_ZONES	((AT_MID_LAP<<8) | 28)
#define LAP_IOC_GET_LOCAL_ZONE	((AT_MID_LAP<<8) | 29)
#define LAP_IOC_IS_ZONE_LOCAL	((AT_MID_LAP<<8) | 30)
#define LAP_IOC_GET_MODE		((AT_MID_LAP<<8) | 31)
#define LAP_IOC_GET_IF_NAMES    ((AT_MID_LAP<<8) | 32)
#define LAP_IOC_GET_DEFAULT_ZONE ((AT_MID_LAP<<8) | 33)
#define LAP_IOC_SET_DEFAULT_ZONES ((AT_MID_LAP<<8) | 34)


#endif /* __LAP__ */

