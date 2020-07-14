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
 *
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#ifndef __ELAP__
#define __ELAP__

# include <at/nbp.h>

/* ethernet address used by ethertalk - 48 bits
 */

#define  AT_ETHER_HDR_SIZE              14  /* Ethernet header size          */
#define  AT_IF_NAME_LEN                  4  /* interface name length          */

typedef struct etalk_addr {
	u_char 		etalk_addr_octet[6];	
} etalk_addr_t;

typedef	struct {
	struct etalk_addr	dest_addr;
	struct etalk_addr	src_addr;
	u_short			length;
} etalk_header_t;

typedef struct {
	u_int	unknown_mblks;	/* number of unknown streams msgs	*/
	u_int	rcv_bytes;  	/* number of data bytes received	*/
	u_int	rcv_packets;	/* number of packets received		*/
	u_int	xmit_bytes;	/* number of data bytes xmited		*/
	u_int	xmit_packets;	/* number of packets xmited		*/
} at_elap_stats_t;

typedef struct {
    short				network_up;		/* 0=network down, nonzero up.*/
    struct	atalk_addr	node;			/* Our node number.           */
	struct	atalk_addr	initial_addr;
	u_short				flags;			/* misc. port flags, (ELAP_CFG_xxx */
	at_net_al			netStart;		/* network start range */
	at_net_al			netEnd;			/* network ending range */
	at_nvestr_t			zonename;
	char				if_name [AT_IF_NAME_LEN];
} at_elap_cfg_t;

/* router init info */
typedef struct {
	short 	rtable_size;		/* RTMP Routing Table Size */
	short 	ztable_size;		/* ZIP Table Size */
	u_int	flags;				/* used to set at_state.flags */
} router_init_t;

	/* elap_cfg 'flags' defines */
#define ELAP_CFG_ZONELESS	0x01	/* true if we shouldn't set a zone (to avoid
									   generating a zip_getnetinfo when routing) */
#define ELAP_CFG_HOME		0x02	/* designate home port (one allowed) */
#define ELAP_CFG_SET_RANGE	0x04	/* set ifID cable range as supplied */ 
#define ELAP_CFG_SEED		0x08	/* set if it's a seed port */

#define ELAP_CFG_ZONE_MCAST	0x10	/* zone passed to set multicast
									   only from pram zone when no router */
	/* elap ioctl's */ 
#define	ELAP_IOC_MYIOCTL(i)	((i>>8) == AT_MID_ELAP)
#define	ELAP_IOC_GET_CFG	((AT_MID_ELAP<<8) | 1)
#define	ELAP_IOC_GET_STATS	((AT_MID_ELAP<<8) | 2)
#define	ELAP_IOC_SET_CFG	((AT_MID_ELAP<<8) | 3)
#define	ELAP_IOC_SET_ZONE	((AT_MID_ELAP<<8) | 4)
#define	ELAP_IOC_SWITCHZONE	((AT_MID_ELAP<<8) | 5)

#endif /* __ELAP__ */


