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
/*
 *	Copyright (c) 1988, 1989, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 * $Id: nbp_send.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

/* "@(#)nbp_send.c: 2.0, 1.12; 9/28/89; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	nbp_send.c
 *
 * Facility:	AppleTalk Name Binding Protocol Library Interface
 *
 * Author:	Gregory Burns, Creation Date: Jun-24-1988
 *
 * History:
 * X01-001	Gregory Burns	24-Jun-1988
 *	 	Initial Creation.
 *
 */

#include <stdio.h>
#include <h/sysglue.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/nbp.h>
#include <h/if_cnt.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/select.h>

#include <sys/select.h>

#include <unistd.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

#define	ZONEOK	TRUE
#define	NOZONE	FALSE
#define	METAOK	TRUE
#define	NOMETA	FALSE

#ifndef FD_SET
#define NFDBITS		(sizeof(int)*8)
#define FD_SET(n, p)	(p)->fds_bits[(n)/NFDBITS] |= (1<<((n)%NFDBITS))
#define FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1<<((n)%NFDBITS))
#define FD_ZERO(p)	(void) memset((char *)(p), 0, sizeof(*(p)))
#endif  /* FD_SET */

static int	nbpId = 0;

static int doNbpReply(at_nbp_t *nbpIn, u_char *reply, u_char **replyPtr,
				int got, int max);

static int nbp_len(at_nbptuple_t *tuple);
static void nbp_pack_tuple(at_entity_t *entity, at_nbptuple_t *tuple);
static void nbp_unpack_tuple(at_nbptuple_t *tuple, at_entity_t *entity);
static void time_diff(struct timeval *time1, struct timeval *time2,
			struct timeval *diff);


int
_nbp_send_ (func, addr, name, reply, max, retry)
	u_char		func;
	at_inet_t	*addr;
	at_entity_t	*name;
	u_char		*reply;
	int		max;
	at_retry_t	*retry;
{

	at_ddp_t	ddpOut, ddpIn;
	at_nbp_t	*nbpOut = (at_nbp_t *) ddpOut.data;
	at_nbp_t	*nbpIn = (at_nbp_t *) ddpIn.data;
	at_retry_t	retrybuf;
	int		fd;
	int 		olderrno;
	int		n, got = 0;	/* Number of tuples received */
	fd_set		selmask;
	struct timeval	timeout;
	char		expectedControl;
	u_char		*replyPtr;
	at_inet_t	aBridge;
	struct	timeval	start_time,stop_time,time_spent,time_remaining;
	struct timezone	tzp;

	int		result;
	
	SET_ERRNO(0);

	if (max <= 0) {
	    SET_ERRNO(EINVAL);
	    return (-1);
	}
	if ((fd = ddp_open(NULL)) < 0)
		return (-1);
	if (rtmp_netinfo(fd, &nbpOut->tuple[0].enu_addr, &aBridge) < 0) {
		FPRINTF(stderr,"nbp_send: rtmp_netinfo failed\n");
		goto out;
	}
	
	if (!retry) {
		retrybuf.retries = NBP_RETRY_COUNT;
		retrybuf.interval = NBP_RETRY_INTERVAL;
		retrybuf.backoff = 1;
	} else {
	        if ((retry->retries <= 0) || (retry->interval <= 0)) {
		  SET_ERRNO(EINVAL);
		  goto out;
		}
		retrybuf.retries = retry->retries; 
		retrybuf.interval = retry->interval;
		retrybuf.backoff = retry->backoff;
	}
	retry = &retrybuf;
	
	/* Validate addr if called with one */
	if (addr) {
		if (_validate_at_addr(addr) == -1) {
			goto out;
		}
	}

	/* Set up common DDP header fields */
	ddpOut.type = NBP_DDP_TYPE;
	ddpOut.dst_socket = NBP_SOCKET;
	UAS_ASSIGN(ddpOut.checksum, 0);
	
	/* Set up common NBP header fields */
	nbpOut->control = func;
	nbpOut->tuple_count = 1;
	nbpOut->at_nbp_id = ++nbpId;
	nbpOut->tuple[0].enu_enum = 0;

	switch (func) {
		case NBP_LKUP:
			if (!_nbp_validate_entity_(name, METAOK, ZONEOK)) {
				SET_ERRNO(EINVAL);
				goto out;
			}
			if (aBridge.node) {
				ddpOut.dst_node = aBridge.node;
				NET_NET(ddpOut.dst_net, aBridge.net);
				nbpOut->control = NBP_BRRQ;
			} else {
				if (name->zone.str[0] != '*' ) {
					SET_ERRNO(ENETUNREACH);
					return -1;
				}
				ddpOut.dst_node = 0xff;
				NET_ASSIGN(ddpOut.dst_net, 0);
			}
			expectedControl = NBP_LKUP_REPLY;
			SET_ERRNO(0);
			break;

		case NBP_CONFIRM:
			if (!_nbp_validate_entity_(name, NOMETA, ZONEOK)) {
				SET_ERRNO(EINVAL);
				goto out;
			}
			nbpOut->control = NBP_LKUP;
			NET_NET(ddpOut.dst_net, addr->net);
			ddpOut.dst_node = addr->node;
			expectedControl = NBP_LKUP_REPLY;
			SET_ERRNO(0);
			break;

		case NBP_REGISTER:
		case NBP_DELETE:
			if (!_nbp_validate_entity_(name, NOMETA, ZONEOK)) {
				SET_ERRNO(EINVAL);
				goto out;
			}
			NET_NET(ddpOut.dst_net, nbpOut->tuple[0].enu_addr.net);
			ddpOut.dst_node = nbpOut->tuple[0].enu_addr.node;
			nbpOut->tuple[0].enu_addr.socket = addr->socket;
			nbpOut->tuple_count = 0;
			expectedControl = NBP_STATUS_REPLY;
			SET_ERRNO(0);
			break;

		default:
			SET_ERRNO(EINVAL);
			goto out;

	}

	/* Check for attempt to lookup non-local zone when no bridge
	 * exists.  Inside AppleTalk, pp VI-5.
	 */
	if (aBridge.node == 0 && !(name->zone.len == 1 && name->zone.str[0] == '*')) {
		SET_ERRNO(ENETUNREACH);
		goto out;
	}

	(void) nbp_pack_tuple (name, &nbpOut->tuple[0]);

	ddpOut.length = DDP_X_HDR_SIZE + 2 + nbp_len(&nbpOut->tuple[0]);

	replyPtr = reply;

	while (got < max) {
		if ((result = write(fd, &ddpOut, ddpOut.length)) < 0) 
		{
			goto out;
		}
		time_remaining.tv_sec = retry->interval;
		time_remaining.tv_usec = 0;
		timeout = time_remaining;
		gettimeofday(&start_time, &tzp);
poll:
		FD_ZERO(&selmask);
		FD_SET(fd, &selmask);
		switch (select(fd+1, &selmask, 0, 0, &timeout)) {
			default:
				if ((result =read(fd, &ddpIn, DDP_DATAGRAM_SIZE)) < 0) 
				{
					goto out;
				}
				/* If we re-poll, return immediately */
				timeout.tv_sec = timeout.tv_usec = 0;
				/* Match NBP-ID and must be reply to request func */
				if (nbpIn->at_nbp_id != nbpOut->at_nbp_id) 
					goto poll;
				if (nbpIn->control != expectedControl) 
					goto poll;
				
				if (expectedControl != NBP_STATUS_REPLY) {
					n = doNbpReply(nbpIn, reply, 
						&replyPtr, got, max);
					got += n;
					SET_ERRNO(0);
					if (got >= max)
						break;
					goto poll;
				} else {
					got = 1;
					if (nbpIn->tuple_count != 0) 
						SET_ERRNO(EADDRNOTAVAIL);
					SET_ERRNO(0);
					goto out;
				}
				SET_ERRNO(0);
				break;
			case -1:
				/* An error occurred */
				if (cthread_errno() != EINTR) 
				    goto out;
				
				/* fall through */
				SET_ERRNO(0);
			case 0:
				/* Nothing found, we timed out */
				gettimeofday (&stop_time, &tzp);
				time_diff(&stop_time, &start_time, 
					&time_spent);
				time_diff(&time_remaining, &time_spent, 
					&timeout);
				time_remaining = timeout;
				if (timeout.tv_sec > 0 || 
					timeout.tv_usec > 0) {
					gettimeofday(&start_time, &tzp);
					goto poll;
				}
				SET_ERRNO(0);
				break;
		}
		/* Are we finished yet ? */
		if (retry->retries-- == 0) {
			SET_ERRNO(0);
			break;
		}
	}

out:
	olderrno = cthread_errno();
	(void) ddp_close(fd);
	SET_ERRNO(olderrno);
	
	if (cthread_errno()) 
	  return (-1);
	
	else {
	  if ((got > 0) && (func == NBP_CONFIRM))
	    addr->socket = nbpIn->tuple[0].enu_addr.socket;
	  return (got);
	}

}

int
nbp_send_multi (func, addr, name, reply, max, retry,cnt)
	u_char		func;
	at_inet_t	*addr;
	at_entity_t	*name;
	u_char		*reply;
	int			max;
	int 		cnt;		/* number of entities to lookup */
	at_retry_t	*retry;
{

	at_ddp_t	*ddpOut, ddpIn;
	at_nbp_t	*nbpOut[IF_TOTAL_MAX];
	at_nbp_t	*nbpIn;
	at_retry_t	retrybuf;
	int		fd;
	int 		olderrno;
	int		n, got = 0;	/* Number of tuples received */
	fd_set		selmask;
	struct timeval	timeout;
	char		expectedControl;
	u_char		*replyPtr;
	at_inet_t	aBridge;
	struct	timeval	start_time,stop_time,time_spent,time_remaining;
	struct timezone	tzp;
	int			i;

	if (!(ddpOut = (at_ddp_t*)malloc(sizeof(at_ddp_t) * cnt)))
		return(-1);
	for (i=0;i<cnt; i++)
		nbpOut[i] = (at_nbp_t *) ddpOut[i].data;
	nbpIn = (at_nbp_t *) ddpIn.data;
	
	SET_ERRNO(0);

	if (max <= 0) {
	    SET_ERRNO(EINVAL);
	    return (-1);
	}
	if ((fd = ddp_open(NULL)) < 0)
		return (-1);
	if (rtmp_netinfo(fd, &nbpOut[0]->tuple[0].enu_addr, &aBridge) < 0) {
		FPRINTF(stderr,"nbp_send: rtmp_netinfo failed\n");
		goto out;
	}
	
	if (!retry) {
		retrybuf.retries = NBP_RETRY_COUNT;
		retrybuf.interval = NBP_RETRY_INTERVAL;
		retrybuf.backoff = 1;
	} else {
	        if ((retry->retries <= 0) || (retry->interval <= 0)) {
		  SET_ERRNO(EINVAL);
		  goto out;
		}
		retrybuf.retries = retry->retries; 
		retrybuf.interval = retry->interval;
		retrybuf.backoff = retry->backoff;
	}
	retry = &retrybuf;
	
	/* Validate addr if called with one */
	if (addr) {
		if (_validate_at_addr(addr) == -1) 
			goto out;
		
	}

	if (aBridge.node == 0 && cnt == 1 &&
		!(name->zone.len == 1 && name->zone.str[0] == '*')) {
		/* if single port mode and specifying a zone and 
		   ther is no router out there, fail */
		SET_ERRNO(ENETUNREACH);
		goto out;
	}
	
	/* Set up common DDP header fields */
	ddpOut[0].type = NBP_DDP_TYPE;
	ddpOut[0].dst_socket = NBP_SOCKET;
	UAS_ASSIGN(ddpOut[0].checksum, 0);
	
	nbpId++;
	for (i=0; i<cnt; i++) {
		/* Set up common NBP header fields */
		nbpOut[i]->control = func;
		nbpOut[i]->tuple_count = 1;
		nbpOut[i]->at_nbp_id = nbpId;
		nbpOut[i]->tuple[0].enu_enum = 0;
		if (i) 
			ddpOut[i] = ddpOut[0];
		switch (func) {
			case NBP_LKUP:
				if (!_nbp_validate_entity_(&name[i], METAOK, ZONEOK)) {
					SET_ERRNO(EINVAL);
					goto out;
				}
				ddpOut[i].dst_node = aBridge.node;
				NET_NET(ddpOut[i].dst_net, aBridge.net);
				nbpOut[i]->control = NBP_BRRQ;
				expectedControl = NBP_LKUP_REPLY;
				break;
		case NBP_REGISTER:
		case NBP_DELETE:
			if (!_nbp_validate_entity_(&name[i], NOMETA, ZONEOK)) {
				SET_ERRNO(EINVAL);
				goto out;
			}
			NET_NET(ddpOut[i].dst_net, nbpOut[i]->tuple[0].enu_addr.net);
			ddpOut[i].dst_node = nbpOut[i]->tuple[0].enu_addr.node;
			nbpOut[i]->tuple[0].enu_addr.socket = addr->socket;
			nbpOut[i]->tuple_count = 0;
			expectedControl = NBP_STATUS_REPLY;
			break;
	
			default:
				SET_ERRNO(EINVAL);
				goto out;
	
		}
	
		(void) nbp_pack_tuple (&name[i], &nbpOut[i]->tuple[0]);
		ddpOut[i].length = DDP_X_HDR_SIZE + 2 + nbp_len(&nbpOut[i]->tuple[0]);
	}

	replyPtr = reply;

	while (got < max) {
		for (i=0; i<cnt; i++)
			if (write(fd, &ddpOut[i], ddpOut[i].length) < 0) 
				goto out;
			
		time_remaining.tv_sec = retry->interval;
		time_remaining.tv_usec = 0;
		timeout = time_remaining;
		gettimeofday(&start_time, &tzp);
poll:
		FD_ZERO(&selmask);
		FD_SET(fd, &selmask);
		switch (select(fd+1, &selmask, 0, 0, &timeout)) {
			default:
				if (read(fd, &ddpIn, DDP_DATAGRAM_SIZE) < 0)  
					goto out;
				
				/* If we re-poll, return immediately */
				timeout.tv_sec = timeout.tv_usec = 0;
				/* Match NBP-ID and must be reply to request func */
				if (nbpIn->at_nbp_id != nbpOut[0]->at_nbp_id)
					goto poll;
				if (nbpIn->control != expectedControl)
					goto poll;
				if (expectedControl != NBP_STATUS_REPLY) {
					n = doNbpReply(nbpIn, reply, 
						&replyPtr, got, max);
					got += n;
					if (got >= max)
						break;
					goto poll;
				} else {
					got = 1;
					if (nbpIn->tuple_count != 0)
						SET_ERRNO(EADDRNOTAVAIL);
					goto out;
				}
				break;
			case -1:
				/* An error occurred */
				if (cthread_errno() != EINTR)
				    goto out;
				/* fall through */
				SET_ERRNO(0);
			case 0:
				/* Nothing found, we timed out */
				gettimeofday (&stop_time, &tzp);
				time_diff(&stop_time, &start_time, 
					&time_spent);
				time_diff(&time_remaining, &time_spent, 
					&timeout);
				time_remaining = timeout;
				if (timeout.tv_sec > 0 || 
					timeout.tv_usec > 0) {
					gettimeofday(&start_time, &tzp);
					goto poll;
				}
				break;
		}
		/* Are we finished yet ? */
		if (retry->retries-- == 0)
			break;
	}

out:
	olderrno = cthread_errno();
	(void) ddp_close(fd);
	SET_ERRNO(olderrno);
	
	if (cthread_errno())
	  return (-1);
	else {
	  if ((got > 0) && (func == NBP_CONFIRM))
	    addr->socket = nbpIn->tuple[0].enu_addr.socket;
	  return (got);
	}

}


static int
doNbpReply(nbpIn, reply, replyPtr, got, max)
	at_nbp_t	*nbpIn;
	u_char		*reply, **replyPtr;
	int		got, max;
{
	int 		i, wegot = 0;
	at_nbptuple_t	*tuple, *tupleIn, *tupleNext;

	tupleIn = &nbpIn->tuple[0];
	tupleNext = * (at_nbptuple_t **) replyPtr;

	for (i = 0; i < (int) nbpIn->tuple_count; i++) {
		for (tuple = (at_nbptuple_t *) reply; tuple < tupleNext; tuple++) {
			if (tuple->enu_enum == tupleIn->enu_enum &&
			    NET_EQUAL(tuple->enu_addr.net, tupleIn->enu_addr.net) &&
			    tuple->enu_addr.node == tupleIn->enu_addr.node &&
			    tuple->enu_addr.socket == tupleIn->enu_addr.socket){
				goto skip;
			}
		}
		if (got + wegot >= max)
			break;
		wegot++;

		tupleNext->enu_addr = tupleIn->enu_addr;
		tupleNext->enu_enum = tupleIn->enu_enum;
		(void) nbp_unpack_tuple(tupleIn, &tupleNext->enu_entity);
		tupleNext++;
skip:
		tupleIn = (at_nbptuple_t *) (((u_char *) tupleIn) + nbp_len(tupleIn));
	}
	*replyPtr = (u_char *) tupleNext;
	return (wegot);
}


static int
nbp_len(tuple)
	at_nbptuple_t	*tuple;
{
	register u_char	*p;
	int		len;

	len = sizeof(at_inet_t) + 1;
	p = tuple->enu_name;
	len += *p + 1;
	p += *p + 1;
	len += *p + 1;
	p += *p + 1;
	len += *p + 1;
	return (len);
}


static void
nbp_pack_tuple(entity, tuple)
	at_entity_t	*entity;
	at_nbptuple_t	*tuple;
{
	register u_char	*p;

	p = tuple->enu_name;
	memcpy (p, &entity->object, entity->object.len + 1);
	p += entity->object.len + 1;
	memcpy (p, &entity->type, entity->type.len + 1);
	p += entity->type.len + 1;
	memcpy (p, &entity->zone, entity->zone.len + 1);
}


static void
nbp_unpack_tuple(tuple, entity)
	at_nbptuple_t	*tuple;
	at_entity_t	*entity;
{
	register u_char	*p;

	p = tuple->enu_name;
	memcpy (&entity->object, p, *p + 1);
	p += *p + 1;
	memcpy (&entity->type, p, *p + 1);
	p += *p + 1;
	memcpy (&entity->zone, p, *p + 1);
}

/* returns (time1 - time2) in diff.
 * diff may be the same as either time1 or time2.
 * Will return 0's in diff if the time difference is negative.
 */
static	void	time_diff (time1, time2, diff)
struct	timeval	*time1, *time2, *diff;
{
	int	carry = 0;

	if (time1->tv_usec >= time2->tv_usec) {
		diff->tv_usec = time1->tv_usec - time2->tv_usec;
	} else {
		diff->tv_usec = time1->tv_usec+1000000-time2->tv_usec;
		carry = 1;
	}
	
	if (time1->tv_sec < (time2->tv_sec + carry))
		diff->tv_sec = diff->tv_usec = 0;
	else
		diff->tv_sec = time1->tv_sec - (time2->tv_sec + carry);
	return;
}
