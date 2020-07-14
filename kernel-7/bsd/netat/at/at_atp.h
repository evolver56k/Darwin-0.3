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

/* Definitions for ATP protocol and streams module, per 
 * AppleTalk Transaction Protocol documentation from
 * `Inside AppleTalk', July 14, 1986.
 */

#ifndef UIOSEG_USER
# include <sys/uio.h>
#endif

#define ATP_ATP_HDR(c) ((at_atp_t *)(&((at_ddp_t *)(c))->data[0]))

#define TOTAL_ATP_HDR_SIZE    (ATP_HDR_SIZE+DDP_X_HDR_SIZE)
#define ATP_CLEAR_CONTROL(c)  (*(char *)(c) = 0)

/* ATP ioctl interface */

/* Structure for the atp_set_default call */

#define	ATP_INFINITE_RETRIES	0xffffffff	/* means retry forever
						 * in the def_retries field
					 	 */

struct atp_set_default {
	u_int	def_retries;		/* number of retries for a request */
	u_int	def_rate;		/* retry rate (in seconds/100) NB: the
					 * system may not be able to resolve
					 * delays of 100th of a second but will
					 * instead make a 'best effort'
					 */
	struct atpBDS *def_bdsp; /*  BDS structure associated with this req */
	u_int	def_BDSlen;	/* size of BDS structure */
};


/* Return header from requests */

struct atp_result {
	u_short		count;		/* the number of packets */
	u_short		hdr;		/* offset to header in buffer */
	u_short 	offset[8];	/* offset to the Nth packet in the buffer */
	u_short		len[8];		/* length of the Nth packet */
};

struct atpBDS {
	ua_short	bdsBuffSz;
	ua_long		bdsBuffAddr;
	ua_short	bdsDataSz;
	unsigned char	bdsUserData[4];
};


typedef struct {
        u_short        at_atpreq_type;
        at_net         at_atpreq_to_net;
        at_node        at_atpreq_to_node;
        at_socket      at_atpreq_to_socket;
        u_char         at_atpreq_treq_user_bytes[4];
        u_char         *at_atpreq_treq_data;
        u_short        at_atpreq_treq_length;
        u_char         at_atpreq_treq_bitmap;
        u_char         at_atpreq_xo;
        u_char         at_atpreq_xo_relt;
        u_short        at_atpreq_retry_timeout;
        u_short        at_atpreq_maximum_retries;
        u_char         at_atpreq_tresp_user_bytes[ATP_TRESP_MAX][4];
        u_char         *at_atpreq_tresp_data[ATP_TRESP_MAX];
        u_short        at_atpreq_tresp_lengths[ATP_TRESP_MAX];
        u_long         at_atpreq_debug[4];
        u_short        at_atpreq_tid;
        u_char         at_atpreq_tresp_bitmap;
        u_char         at_atpreq_tresp_eom_seqno;
        u_char         at_atpreq_got_trel;
} at_atpreq;


/* The ATP module ioctl commands */

#define AT_ATP_CANCEL_REQUEST		(('|'<<8)|1)
#define AT_ATP_ISSUE_REQUEST		(('|'<<8)|2)
#define AT_ATP_ISSUE_REQUEST_DEF	(('|'<<8)|3)
#define AT_ATP_ISSUE_REQUEST_DEF_NOTE	(('|'<<8)|4)
#define AT_ATP_ISSUE_REQUEST_NOTE	(('|'<<8)|5)
#define AT_ATP_GET_POLL			(('|'<<8)|6)
#define AT_ATP_RELEASE_RESPONSE	(('|'<<8)|7)
#define AT_ATP_REQUEST_COMPLETE	(('|'<<8)|8)
#define AT_ATP_SEND_FULL_RESPONSE	(('|'<<8)|9)
#define AT_ATP_BIND_REQ			(('|'<<8)|10)
#define AT_ATP_GET_CHANID		(('|'<<8)|11)
#define AT_ATP_PEEK				(('|'<<8)|12)

/* These macros don't really depend here, but since they're used only by the
 * old ATP and old PAP, they're put here.  Unisoft PAP includes this file.
 */
#define	R16(x)		UAS_VALUE(x)
#define	W16(x,v)	UAS_ASSIGN(x, v)
#define	C16(x,v)	UAS_UAS(x, v)


/*
 * these are the dispatch codes for
 * the new atp_control system call
 */
#define ATP_SENDREQUEST  0
#define ATP_GETRESPONSE  1
#define ATP_SENDRESPONSE 2
#define ATP_GETREQUEST   3
