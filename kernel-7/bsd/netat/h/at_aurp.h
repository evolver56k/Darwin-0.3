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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	Change Log:
 *	  Created April 8, 1996 by Tuyen Nguyen
 */

#ifndef _at_aurp_h
#define _at_aurp_h

#define AURPCODE_REG                   0
#define AURPCODE_RTMPPKT               1
#define AURPCODE_DATAPKT               2
#define AURPCODE_AURPPROTO             3
#define AURPCODE_DEBUGINFO             10
#define AURPCODE_RTINFO                11
#define AURPCODE_RTUPDATE              12

#define AURPSTATE_Unconnected          0
#define AURPSTATE_Connected            1
#define AURPSTATE_WaitingForOpenRsp    2
#define AURPSTATE_WaitingForRIRsp      3
#define AURPSTATE_WaitingForTickleAck  4
#define AURPSTATE_WaitingForRIAck1     5
#define AURPSTATE_WaitingForRIAck2     6
#define AURPSTATE_WaitingForRIAck3     7

#define AURPCMD_RIReq                  1
#define AURPCMD_RIRsp                  2
#define AURPCMD_RIAck                  3
#define AURPCMD_RIUpd                  4
#define AURPCMD_RDReq                  5
#define AURPCMD_ZReq                   6
#define AURPCMD_ZRsp                   7
#define AURPCMD_OpenReq                8
#define AURPCMD_OpenRsp                9
#define AURPCMD_Tickle                 14
#define AURPCMD_TickleAck              15

#define AURPSUBCODE_ZoneInfo1          1
#define AURPSUBCODE_ZoneInfo2          2
#define AURPSUBCODE_GetZoneNets        3
#define AURPSUBCODE_GetDomainZoneList  4

#define AURPEV_Null                    0
#define AURPEV_NetAdded                1
#define AURPEV_NetDeleted              2
#define AURPEV_NetRouteChange          3
#define AURPEV_NetDistChange           4
#define AURPEV_NetZoneChange           5

#define AURP_Version                   1
#define AURP_ProbeRetryInterval        300
#define AURP_MaxTickleRetry            4
#define AURP_TickleRetryInterval       30
#define AURP_MaxRetry                  10
#define AURP_RetryInterval             3
#define AURP_UpdateRate                1
#define AURP_UDType                    0
#define AURP_UDNode                    1
#define AURP_UDSize                    2
#define AURP_FirstSeqNum               1
#define AURP_LastSeqNum                65535
#define AURP_MaxPktSize                1400
#define AURP_MaxNetAccess              64
#define AURP_NetHiden                  0x01

#define AURPERR_NormalConnectionClose  -1
#define AURPERR_RoutingLoopDetected    -2
#define AURPERR_ConnectionOutOfSync    -3
#define AURPERR_OptionNegotiationError -4
#define AURPERR_InvalidVersionNumber   -5
#define AURPERR_InsufficientResources  -6
#define AURPERR_AuthenticationError    -7

#define AURPFLG_NA    0x4000
#define AURPFLG_ND    0x2000
#define AURPFLG_NDC   0x1000
#define AURPFLG_ZC    0x0800
#define AURPFLG_RMA   0x4000
#define AURPFLG_HCRA  0x2000
#define AURPFLG_SZI   0x4000
#define AURPFLG_LAST  0x8000

/*
 * AURP state block
 */
typedef struct {
	unsigned char  get_zi;          /* get zone info flag */
	unsigned char  rem_node;        /* node id of a tunnel peer */
	unsigned char  tickle_retry;    /* tickle retry count */
	unsigned char  rcv_retry;       /* data receiver retry count */
	unsigned char  snd_state;       /* data sender state */
	unsigned char  rcv_state;       /* data receiver state */
	unsigned char  filler[2];
	unsigned short rcv_update_rate;
	unsigned short snd_next_entry;  /* next entry in RT */
	unsigned short rcv_env;
	unsigned short snd_sui;
	unsigned short rcv_connection_id;   /* data receiver connection id */
	unsigned short snd_connection_id;   /* data sender connection id */
	unsigned short rcv_sequence_number; /* data receiver sequence number */
	unsigned short snd_sequence_number; /* data sender sequence number */
	void   *rcv_tmo;
	void   *snd_tmo;
	gbuf_t *rsp_m;
	gbuf_t *upd_m;
} aurp_state_t;

/*
 * AURP protocol header
 */
typedef struct {
	unsigned short connection_id;
	unsigned short sequence_number;
	unsigned short command_code;
	unsigned short flags;
} aurp_hdr_t;

/*
 *
 */
typedef struct {
	void *RT_table;
	void *ZT_table;
	short RT_maxentry;
	short ZT_maxentry;
	void *rt_lock;
	void *rt_insert;
	void *rt_delete;
	void *rt_lookup;
	void *zt_add_zname;
	void *zt_set_zmap;
	void *zt_get_zindex;
	void *zt_remove_zones;
} aurp_rtinfo_t;

/**********/

#ifdef _AURP
extern short RT_maxentry;
extern short ZT_maxentry;
extern RT_entry *RT_table;
extern ZT_entry *ZT_table;
extern void *RT_lock;
extern RT_entry *(*RT_insert)();
extern RT_entry *(*RT_delete)();
extern RT_entry *(*RT_lookup)();
extern void (*ZT_set_zmap)();
extern int  (*ZT_add_zname)();
extern int  (*ZT_get_zindex)();
extern void (*ZT_remove_zones)();

extern atlock_t aurpgen_lock;
extern gref_t *aurp_gref;
extern unsigned char opt_proto;
extern unsigned char dst_addr_cnt;
extern unsigned char net_access_cnt;
extern unsigned char net_export;
extern unsigned short rcv_connection_id;
extern int net_port;
extern void *update_tmo;
extern aurp_state_t aurp_state[];
extern unsigned short net_access[];
#endif

#ifdef _KERNSYS
struct myq
{	struct mbuf *q_head;
	struct mbuf *q_tail;
	int q_cnt;
};

#ifdef _AIX
#define LOCK_DECL(x)	Simple_lock x
#else
#define LOCK_DECL(x)	atlock_t x
#endif
/*
 * Quandry: if we use a single socket, we have to rebind on each call.
 * If we use separate sockets per tunnel endpoint, we have to examine
 *  each one on wakeup.  What to do; what to do?
 */
struct aurp_global_t
{	int src_addr;		/* What's our IP address? */
	int udp_port;		/* Local UDP port */
	unsigned short net_access[AURP_MAXNETACCESS];
	long dst_addr[256];	/* Tunnel 'other ends', passed in from user */
	int pid;		/* Who are we? */
	struct socket *tunnel;	/* IP socket for all IP endpoints */
	int event;		/* Sleep queue anchor */
	int event_anchor;	/* Sleep queue anchor */
#ifdef _AIX
	LOCK_DECL(glock);	/* aurp_global lock */
#else
	atlock_t glock;
#endif
	struct uio auio;	/* Dummy uio struct for soreceive() */
	/* Statistics */
	unsigned int toosmall;	/* size less than domain header, from UDP */
	unsigned int no_mbufs;	/* gbuf_to_mbuf failed */
	unsigned int no_gbufs;	/* mbuf_to_gbuf failed */
	unsigned int shutdown;	/* shutdown flag */
	unsigned int running;	/* running flag */
};

#define AE_ATALK	0x01	/* A/talk input event */
#define AE_UDPIP	0x02	/* UDP/IP input event */
#define AE_SHUTDOWN	0x04	/* Shutdown AURP process */
 
extern struct mbuf *at_gbuf_to_mbuf(gbuf_t *);
extern gbuf_t *at_mbuf_to_gbuf(struct mbuf *, int);
#endif

/* AURP header for IP tunneling */
typedef struct domain
{	char  dst_length;
	char  dst_authority;
	short dst_distinguisher;
	long  dst_address;
	char  src_length;
	char  src_authority;
	short src_distinguisher;
	long  src_address;
	short version;
	short reserved;
	short type;
} domain_t;

/* AURP/domain header constants */
#define AUD_Version	0x1
#define AUD_Atalk	0x2
#define AUD_AURP	0x3

/* IP domain identifier constants */
#define IP_LENGTH		7
#define IP_AUTHORITY		1
#define IP_DISTINGUISHER	0
/* Need this because the )(*&^%$#@ compiler rounds up the size */
#define IP_DOMAINSIZE		22

/****### LD 9/26/97*/
extern struct aurp_global_t aurp_global;
#endif

