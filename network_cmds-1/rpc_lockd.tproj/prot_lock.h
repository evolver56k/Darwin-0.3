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
/*      @(#)prot_lock.h 1.13 89/03/10 SMI                              */

/*
 * This file consists of all structure information used by lock manager 
 */

#include <rpc/rpc.h>
#include "nlm_prot.h"
#include "klm_prot.h"
#include "lockf.h"

typedef struct nlm_testres remote_result;
#define lstat stat.stat
#define lholder stat.nlm_testrply_u.holder

#define NLM_LOCK_RECLAIM	16
#define MSG 	0		/* choices of comm to remote svr */
#define RPC 	1		/* choices of comm to remote svr */

#define MAXLEN		((1 << 31) -1)
#define lck 		alock
#define svr		server_name
#define caller		caller_name
#define clnt		clnt_name
#define fh_len		fh.n_len
#define fh_bytes	fh.n_bytes
#define oh_len		oh.n_len
#define oh_bytes	oh.n_bytes
#define cookie_len	cookie.n_len
#define cookie_bytes	cookie.n_bytes

/*
#define granted		nlm_granted
*/
#define denied		nlm_denied
#define nolocks 	nlm_denied_nolocks
#define blocking	nlm_blocked
#define grace		nlm_denied_grace_period
#define deadlck		nlm_deadlck

/*
 * warning:  struct alock consists of klm_lock and nlm_lock,
 * it has to be modified if either structure has been modified!!!
 */
struct alock {
	netobj cookie;

	/* from klm_prot.h */
        char *server_name;
        netobj fh;

	/* from lockf.h */
	struct data_lock lox;

	/* addition from nlm_prot.h */
	char *caller_name;
	netobj oh;
	int svid;
	u_int l_offset;
        u_int l_len;

	/* addition from lock manager */
	char *clnt_name;
	int op;
};


struct reclock {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct alock alock;
	bool_t reclaim;
	int state;

	/* auxiliary structure */
	SVCXPRT *transp; /* transport handle for delayed response due to */ 
			 /* blocking or grace period */
	int rel;
	int w_flag;

	struct reclock *prev; 
	struct reclock *next;
};
typedef struct reclock reclock;

/*
 * Lock manager vp->v_filocks
 */
struct lm_vnode {
        char *server_name;
        netobj fh;
        struct reclock *exclusive;	/* exclusive locks */
	struct reclock *shared;	/* shared locks */
	struct reclock *pending;	/* pending locks */
	struct lm_vnode *prev;
	struct lm_vnode *next;
	struct reclock *reclox;
	int    rel;
};
typedef struct lm_vnode lm_vnode;

struct timer {
	/* timer goes off when exp == curr */
	int exp;
	int curr;
};
typedef struct timer timer;

/*
 * msg passing structure
 */
struct msg_entry {
	reclock *req;
	remote_result *reply;
	timer t;
	int proc; /* procedure name that req is sent to; needed for reply purpose */
	struct msg_entry *prev;
	struct msg_entry *nxt;
};
typedef struct msg_entry msg_entry;

struct priv_struct {
	int pid;
	int *priv_ptr;
};

struct fd_table {
	netobj	fh;
	int	fd;
};
