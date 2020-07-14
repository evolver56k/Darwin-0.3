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

/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 */

#ifndef _NETINET_IF_FDDI_H_
#define _NETINET_IF_FDDI_H_

/*
 * Structure of an 100Mb/s FDDI header.
 */
struct	fddi_header {
	u_char	fddi_fc;
	u_char	fddi_dhost[6];
	u_char	fddi_shost[6];
};

#define	FDDIMTU			4470
#define	FDDIMIN			3

#define	FDDIFC_C		0x80	/* 0b10000000 */
#define	FDDIFC_L		0x40	/* 0b01000000 */
#define	FDDIFC_F		0x30	/* 0b00110000 */
#define	FDDIFC_Z		0x0F	/* 0b00001111 */

#define	FDDIFC_LLC_ASYNC	0x50
#define	FDDIFC_LLC_PRIO0	0
#define	FDDIFC_LLC_PRIO1	1
#define	FDDIFC_LLC_PRIO2	2
#define	FDDIFC_LLC_PRIO3	3
#define	FDDIFC_LLC_PRIO4	4
#define	FDDIFC_LLC_PRIO5	5
#define	FDDIFC_LLC_PRIO6	6
#define	FDDIFC_LLC_PRIO7	7
#define FDDIFC_LLC_SYNC         0xd0
#define	FDDIFC_SMT		0x40

#ifdef	_KERNEL
#define	fddibroadcastaddr	etherbroadcastaddr
#define	fddi_ipmulticast_min	ether_ipmulticast_min
#define	fddi_ipmulticast_max	ether_ipmulticast_max
#define	fddi_addmulti		ether_addmulti
#define	fddi_delmulti		ether_delmulti
#define	fddi_sprintf		ether_sprintf

void    fddi_ifattach __P((struct ifnet *));
void    fddi_input __P((struct ifnet *, struct fddi_header *, struct mbuf *));
int     fddi_output __P((struct ifnet *,
           struct mbuf *, struct sockaddr *, struct rtentry *));

#endif

#endif
