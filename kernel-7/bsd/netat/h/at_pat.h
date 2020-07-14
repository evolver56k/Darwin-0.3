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

/* "@(#)at_pat.h: 2.0, 1.8; 10/4/93; Copyright 1988-89, Apple Computer, Inc." */

/* This is header for the PAT module. This contains a table of pointers that 
 * should get initialized with the BNET stuff and the ethernet driver. The 
 * number of interfaces supported should be communicated. Should include
 * mbuf.h, if.h, socket.h
 *
 * Author: R. C. Venkatraman
 * Date  : 2/29/88 
 */

typedef struct {
	unsigned char dst[6];
	unsigned char src[6];
	unsigned short len;
} enet_header_t;

typedef struct {
	unsigned char dst_sap;
	unsigned char src_sap;
	unsigned char control;
	unsigned char protocol[5];
} llc_header_t;

#define ENET_LLC_SIZE (sizeof(enet_header_t)+sizeof(llc_header_t))
#define SNAP_UI		0x03  /* bits 11000000 reversed!! */
#define SNAP_AT_SAP	0xaa
#define SNAP_PROTO_AT	{0x08, 0x00, 0x07, 0x80, 0x9B}
#define SNAP_PROTO_AARP	{0x00, 0x00, 0x00, 0x80, 0xF3}
#define SNAP_HDR_AT	{SNAP_AT_SAP, SNAP_AT_SAP, SNAP_UI, SNAP_PROTO_AT}
#define SNAP_HDR_AARP	{SNAP_AT_SAP, SNAP_AT_SAP, SNAP_UI, SNAP_PROTO_AARP}

#define LLC_PROTO_EQUAL(a1, a2)                                         \
        ((*((unsigned long *)(a1)) == *((unsigned long *)(a2))) &&      \
	 (a1[4] == a2[4])				                \
	)

/* PAT data structures and control codes */

#define 	MAX_MCASTS			25	/*#multicast addrs tracked per i/f */ 

typedef	struct pat_unit {
	int	state;		/* PAT state				*/
	caddr_t	context;	/* Context pointer for upstream		*/
	int	(*aarp_func)();	/* Function to be called for passing	*/
						/* AARP packets up.			*/
	int	(*addr_check)();/* Function to be called for checking	*/
						/* destination address for an incoming	*/
						/* packet.				*/
	void	*nddp;	/* Pointer to NDD struct for this driver*/
	int	elapno;			/* corresponding entry in elap_specifics */
						/* array for tracking registered mcast
						   addrs, only uniqe portion tracked */
	unsigned char	mcast[MAX_MCASTS];
	void *xddpq;
	void *xarpq;
	char  xflag;
	char  xunit;
	char  xtype;
	char  xpad;
	int   xaddrlen;
	char  xaddr[6];
	char  xname[8];
} pat_unit_t;

#define PAT_REG_CONTEXT		1
#define PAT_REG_ELAPQ		2
#define PAT_REG_AARP_UPSTREAM	3
#define PAT_REG_CHECKADDR	4
#define PAT_REG_MCAST		5
#define PAT_UNREG_MCAST		6
#define PAT_UNREG_ALL_MCAST	7

	/* multicast tracking */
#define MCAST_TRACK_ADD		1
#define MCAST_TRACK_DELETE	2
#define MCAST_TRACK_CHECK	3

/* PAT states */
#define PAT_FREE		0
#define PAT_OFFLINE		1
#define PAT_ONLINE		2
