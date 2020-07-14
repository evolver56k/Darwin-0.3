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
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#ifndef __DDP__
#define __DDP__

/* Header and data sizes */

#define  DDP_HDR_SIZE                 5  /* DDP (short) header size */
#define  DDP_X_HDR_SIZE              13  /* DDP extended header size */
#define  DDP_DATA_SIZE              586  /* Maximum DataGram data size */
#define  DDP_DATAGRAM_SIZE          599  /* Maximum DataGram size */


/* DDP socket definitions */

#define  DDP_SOCKET_1st_RESERVED      1  /* First in reserved range */
#define  DDP_SOCKET_1st_EXPERIMENTAL 64  /* First in experimental range */
#define  DDP_SOCKET_1st_DYNAMIC     128  /* First in dynamic range */
#define  DDP_SOCKET_LAST            253  /* Last socket in any range */


/* DDP extended header packet format */

typedef struct {
        unsigned   unused:2,
		   hopcount:4,
		   length:10;  		/* Datagram length */
        ua_short   checksum;    	/* Checksum */
        at_net     dst_net;  		/* Destination network number */
        at_net     src_net;  		/* Source network number */
        at_node    dst_node;  		/* Destination node ID */
        at_node    src_node;  		/* Source node ID */
        at_socket  dst_socket; 		/* Destination socket number */
        at_socket  src_socket; 		/* Source socket number */
        u_char	   type;  		/* Protocol type */
        char       data[DDP_DATA_SIZE];
} at_ddp_t;


#define	DDPLEN_ASSIGN(ddp, len)		ddp->length = len
#define	DDPLEN_VALUE(ddp)		ddp->length


/* DDP module statistics and configuration */

typedef struct {
	/* General */

	/* Receive stats */
	u_int	rcv_bytes;
	u_int	rcv_packets;
	u_int	rcv_bad_length;
	u_int	rcv_unreg_socket;
	u_int	rcv_bad_socket;
	u_int	rcv_bad_checksum;
	u_int	rcv_dropped_nobuf;

	/* Transmit stats */
	u_int	xmit_bytes;
	u_int	xmit_packets;
	u_int	xmit_BRT_used;
	u_int	xmit_bad_length;
	u_int	xmit_bad_addr;
	u_int	xmit_dropped_nobuf;
} at_ddp_stats_t;

typedef struct {
        u_short  	network_up;
	u_short		filler;
	int		flags;		/* to indicate what type
					 * of network interface the
					 * default net is (ethertalk/
					 * localtalk?)
					 */
	at_inet_t	node_addr;
	at_inet_t	router_addr;
	int		netlo;
	int		nethi;
} at_ddp_cfg_t;

union at_ddpopt {
    struct {
        unsigned char checksum; /* do checksum */
        unsigned char type;     /* ddp type */
    } ct;
};

union at_adspopt {
    struct {
    u_short sendBlocking;	/* quantum for data packets */
    u_char sendTimer;		/* send timer in 10-tick intervals */
    u_char rtmtTimer;		/* retransmit timer in 10-tick intervals */
    u_char badSeqMax;		/* threshold for sending retransmit advice */
    u_char useCheckSum;		/* use ddp packet checksum */
    u_short filler;
    int newPID;				/* ### Temp for backward compatibility 02/11/94 */
    } c;
};

#endif /* __DDP__ */
