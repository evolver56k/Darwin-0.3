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

/* "@(#)atp_inc.h: 2.0, 1.16; 10/4/93; Copyright 1988-89, Apple Computer, Inc." */

#include "sysglue.h"
#include "debug.h"
#include "at-config.h"

/*
 *	Stuff for accessing protocol headers 
 */
#define AT_DDP_HDR(m) ((at_ddp_t *)(gbuf_rptr(m)))
#define AT_ATP_HDR(m) ((at_atp_t *)(&((at_ddp_t *)(gbuf_rptr(m)))->data[0]))

/*
 *	Masks for accessing/manipulating the bitmap field in atp headers
 */

#ifdef ATP_DECLARE
unsigned char atp_mask [] = {
	0x01, 0x02, 0x04, 0x08, 
	0x10, 0x20, 0x40, 0x80, 
};

unsigned char atp_lomask [] = {
	0x00, 0x01, 0x03, 0x07, 
	0x0f, 0x1f, 0x3f, 0x7f, 
	0xff
};
#else
extern unsigned char atp_mask [];
extern unsigned char atp_lomask [];
#endif /* ATP_DECLARE */

/*
 *	doubly linked queue types and primitives
 */

#define ATP_Q_ENTER(hdr, object, entry) {					\
		if ((hdr).head) {						\
			(hdr).head->entry.prev = (object);			\
			(object)->entry.next = (hdr).head;			\
		} else {							\
			(hdr).tail = (object);					\
			(object)->entry.next = NULL;				\
		}								\
		(object)->entry.prev = NULL;					\
		(hdr).head = (object);						\
	}

#define ATP_Q_APPEND(hdr, object, entry) {					\
		if ((hdr).head) {						\
			(hdr).tail->entry.next = (object);			\
			(object)->entry.prev = (hdr).tail;			\
		} else {							\
			(hdr).head = (object);					\
			(object)->entry.prev = NULL;				\
		}								\
		(object)->entry.next = NULL;					\
		(hdr).tail = (object);						\
	}

#define ATP_Q_REMOVE(hdr, object, entry) {					\
		if ((object)->entry.prev) {					\
			(object)->entry.prev->entry.next = (object)->entry.next;\
		} else {							\
			(hdr).head = (object)->entry.next;			\
		}								\
		if ((object)->entry.next) {					\
			(object)->entry.next->entry.prev = (object)->entry.prev;\
		} else {							\
			(hdr).tail = (object)->entry.prev;			\
		}								\
	}

struct atp_rcb_qhead {
	struct atp_rcb 	*head;
	struct atp_rcb 	*tail;
};

struct atp_rcb_q {
	struct atp_rcb *prev;
	struct atp_rcb *next;
};

struct atp_trans_qhead {
	struct atp_trans *head;
	struct atp_trans *tail;
};

struct atp_trans_q {
	struct atp_trans *prev;
	struct atp_trans *next;
};

/*
 *	Locally saved remote node address
 */

struct atp_socket {
	u_short		net;
	at_node		node;
	at_socket	socket;
};

/*
 *	transaction control block (local context at requester end)
 */

struct atp_trans {
	struct atp_trans_q	tr_list;		/* trans list */
	struct atp_state	*tr_queue;		/* state data structure */
	gbuf_t			*tr_xmt;		/* message being sent */
	gbuf_t			*tr_rcv[8];		/* message being rcvd */
	unsigned int		tr_retry;		/* # retries left */
	unsigned int		tr_timeout;		/* timer interval */
	char			tr_state;		/* current state */
	char			tr_rsp_wait;		/* waiting for transaction response */
	char 			filler[2];
	unsigned char		tr_xo;			/* execute once transaction */
	unsigned char		tr_bitmap;		/* requested bitmask */
	unsigned short		tr_tid;			/* transaction id */
	struct atp_socket	tr_socket;		/* the remote socket id */
	struct atp_trans_q	tr_snd_wait;		/* list of transactions waiting
							   for space to send a msg */
	at_socket		tr_local_socket;
	unsigned char	tr_local_node;
	unsigned char	tr_local_net[2];
	gbuf_t                  *tr_bdsp;               /* bds structure pointer */
	unsigned int		tr_tmo_delta;
	void 				(*tr_tmo_func)();
	struct atp_trans	*tr_tmo_next;
	struct atp_trans	*tr_tmo_prev;
	atlock_t tr_lock;
	atevent_t tr_event;
};

#define	TRANS_TIMEOUT		0	/* waiting for a reply */
#define	TRANS_REQUEST		1	/* waiting to send a request */
#define	TRANS_RELEASE		2	/* waiting to send a release */
#define	TRANS_DONE		3	/* done - waiting for poll to complete */
#define	TRANS_FAILED		4	/* done - waiting for poll to report failure */

/*
 *	reply control block (local context at repling end)
 */

struct atp_rcb {
	struct atp_rcb_q	rc_list;		/* rcb list */
	struct atp_rcb_q        rc_tlist;
	struct atp_state	*rc_queue;		/* state data structure */
	gbuf_t			*rc_xmt;		/* replys being sent */
	gbuf_t			*rc_ioctl;		/* waiting ioctl */
	char			rc_snd[8];		/* replys actually to be sent */
	int                     rc_pktcnt;              /* no of pkts in this trans */
	short			rc_state;		/* current state */
	unsigned char		rc_xo;			/* execute once transaction */
	unsigned char		rc_local_node;
	unsigned char		rc_local_net[2];
	short			rc_rep_waiting;		/* in the reply wait list */
	int			rc_timestamp;		/* reply timer */
	unsigned char		rc_bitmap;		/* replied bitmask */
	unsigned char		rc_not_sent_bitmap;	/* replied bitmask */
	unsigned short		rc_tid;			/* transaction id */
	struct atp_socket	rc_socket;		/* the remote socket id */
};

#define RCB_UNQUEUED		0 	/* newly allocated, not q'd */
#define RCB_RESPONDING		2	/* waiting all of response from process*/
#define RCB_RESPONSE_FULL	3	/* got all of response */
#define RCB_RELEASED		4	/* got our release */
#define RCB_PENDING		5	/* a no wait rcb is full */
#define RCB_NOTIFIED		6
#define RCB_SENDING		7	/* we're currently xmitting this trans */

/*
 *	socket state (per module data structure)
 */

struct atp_state {
	gref_t		*atp_gref;	/* must be the first entry */
	int			atp_pid; /* process id, must be the second entry */
	gbuf_t 			*atp_msgq; /* data msg, must be the third entry */
	unsigned char	dflag; /* structure flag, must be the fourth entry */
	unsigned char	filler;
	short	atp_minor_no;
	short	atp_socket_no;
	short	atp_flags;	        /* general flags */
	struct atp_trans_qhead	atp_trans_wait;		/* pending transaction list */
	struct atp_state	*atp_trans_waiting;	/* list of atps waiting for a
							   free transaction */
	unsigned int		atp_retry;		/* retry count */
	unsigned int		atp_timeout;		/* retry timeout */
	struct atp_state	*atp_rcb_waiting;
	struct atp_rcb_qhead	atp_rcb;		/* active rcbs */
	struct atp_rcb_qhead	atp_attached;		/* rcb's waiting to be read */
	atlock_t atp_lock;
	atevent_t atp_event;
	atlock_t atp_delay_lock;
	atevent_t atp_delay_event;
};


/*
 *     atp_state flag definitions
 */
#define ATP_CLOSING  0x08        /* atp stream in process of closing */


/*
 *	tcb/rcb/state allocation queues
 */

/*
 * Size defines; must be outside following #ifdef to permit
 *  debugging code to reference independent of ATP_DECLARE
 */
#define	NATP_RCB	512	/* the number of ATP RCBs at once */
#define NATP_STATE	192	/* the number of ATP sockets open at once */
				/* note: I made NATP_STATE == NSOCKETS */

#ifdef ATP_DECLARE
struct atp_trans *atp_trans_free_list = NULL;	/* free transactions */
struct atp_rcb *atp_rcb_free_list = NULL;	/* free rcbs */
static struct atp_state *atp_free_list = NULL;		/* free atp states */
static struct atp_rcb atp_rcb_data[NATP_RCB];
static struct atp_state atp_state_data[NATP_STATE];

#else
extern struct atp_trans *atp_trans_free_list;		/* free transactions */
extern struct atp_rcb *atp_rcb_free_list;		/* free rcbs */
extern struct atp_state *atp_free_list;			/* free atp states */
extern struct atp_rcb atp_rcb_data[];
extern struct atp_state atp_state_data[];

extern void atp_req_timeout();
extern void atp_rcb_timer();
extern void atp_x_done();
extern struct atp_rcb *atp_rcb_alloc();
extern struct atp_trans *atp_trans_alloc();
#endif /* ATP_DECLARE */

/* prototypes */
void atp_send_req(gref_t *, gbuf_t *);
void atp_drop_req(gref_t *, gbuf_t *);
void atp_send_rsp(gref_t *, gbuf_t *, int);
void atp_wput(gref_t *, gbuf_t *);
void atp_rput(gref_t *, gbuf_t *);
void atp_retry_req(gbuf_t *);
void atp_stop(gbuf_t *, int);
void atp_cancel_req(gref_t *, unsigned short);
int atp_open(gref_t *, int);
int atp_bind(gref_t *, unsigned int, unsigned char *);
int atp_close(gref_t *, int);
gbuf_t *atp_build_release(struct atp_trans *);
void atp_req_timeout(struct atp_trans *);
void atp_free(struct atp_trans *);
void atp_x_done(struct atp_trans *);
void atp_send(struct atp_trans *);
void atp_rsp_ind(struct atp_trans *, gbuf_t *);
void atp_trans_free(struct atp_trans *);
void atp_reply(struct atp_rcb *);
void atp_rcb_free(struct atp_rcb *);
void atp_send_replies(struct atp_state *, struct atp_rcb *);
void atp_dequeue_atp(struct atp_state *);
int atp_iocack(struct atp_state *, gbuf_t *);
void atp_req_ind(struct atp_state *, gbuf_t *);
int atp_iocnak(struct atp_state *, gbuf_t *, int);
void atp_trp_timer(void *, int);
void atp_timout(void (*func)(), struct atp_trans *, int);
void atp_untimout(void (*func)(), struct atp_trans *);
int atp_tid(struct atp_state *);
