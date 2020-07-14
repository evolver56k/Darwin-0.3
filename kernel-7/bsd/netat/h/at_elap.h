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

/* "@(#)at_elap.h: 2.0, 1.6; 7/14/89; Copyright 1988-89, Apple Computer, Inc." */

/* This is a header file for EtherTalk LAP layer.
 * 
 * Author: R. C. Venkatraman
 * Date  : 2/29/88 (WOW! What a day to LEAP forward)
 */

#define NOOFIFS		4		/* no of ethernet cards we can support
                                         */
#ifndef FLUSHALL
#define FLUSHALL	1		/* parm for streams flushq */
#endif

typedef struct {
	int		pat_id;
	char		interface_no;	/* interface number -- 
					 			 * 0 for "ethertalk0", 
					 			 * 1 for "ethertalk1" etc.
					 			 */
	at_elap_cfg_t	cfg;		/* elap config struct; defined in 
					 			 * <at/elap.h>
					 			 */
	at_elap_stats_t	stats;
	at_if_t		elap_if;
	u_short		flags;			/* port specific flags (see MPORT_FL_xxx ) */
	struct etalk_addr 	ZoneMcastAddr;	/* zone multicast addr
											   (for MH only) */
	struct etalk_addr 	cable_multicast_addr;	/* AppleTalk broadcast addr */
	int		(*wait_p)();
	gref_t		*wait_q;
	gbuf_t		*wait_m;
} elap_specifics_t;

/* ELAP internal control codes (for elap_control() ) */
#define ELAP_CABLE_BROADCAST_FOR_ZONE	0
#define ELAP_REG_ZONE_MCAST		1
#define ELAP_UNREG_ZONE_MCAST		2
#define ELAP_RESET_INITNODE		3
#define	ELAP_DESIRED_ZONE		4

#define AT_ADDR				0
#define ET_ADDR				1
#define AT_ADDR_NO_LOOP		2		/* disables packets from looping back */

