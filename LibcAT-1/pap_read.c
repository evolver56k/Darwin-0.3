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
 *      Copyright (c) 1998 Apple Computer, Inc. 
 *
 *      The information contained herein is subject to change without
 *      notice and  should not be  construed as a commitment by Apple
 *      Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *      for any errors that may appear.
 *
 *      Confidential and Proprietary to Apple Computer, Inc.
 *
 * $Id: pap_read.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

#include <h/sysglue.h>
#include <fcntl.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/pap.h>
#include <at/atp.h>
#include <at/nbp.h>
#include <at/at_pap.h>

#include <at/asp_errno.h>

#include <mach/cthreads.h>

#include "at_proto.h"

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

extern struct pap_state *paps[];
extern int papm[];

int
pap_read(int fd, u_char *data, int len)
{
	register int i;
	register struct pap_state *papp;
	int userdata;
	u_char	*puserdata = (u_char *)&userdata;
	at_resp_t resp;
	at_retry_t retry;
	at_inet_t dest;

	if (fd < 0 || data == 0)
	    return -1;
	if (len <= 0)
		return(0);
	papp = paps[papm[fd]];
	if (papp == NULL) {
		SET_ERRNO(EINVAL);
		return -1;
	}

	if (papp->pap_eof)
		return 0;

	NET_NET(dest.net,papp->pap_to_net);
	dest.node = papp->pap_to_node;
	dest.socket = papp->pap_to_socket;
	puserdata[0] = papp->pap_connID;
	puserdata[1] = AT_PAP_TYPE_SEND_DATA;
	papp->pap_send_count++;
	if (papp->pap_send_count == 0)
	    papp->pap_send_count = 1;
	*(u_short *)&puserdata[2] = papp->pap_send_count;
	retry.interval = 2;
	retry.retries = 5;

	for (i = 0; i < 8; i++) {
		if (len > AT_PAP_DATA_SIZE) {
			resp.resp[i].iov_base = data;
			resp.resp[i].iov_len = AT_PAP_DATA_SIZE;
			len -= AT_PAP_DATA_SIZE;
			data += AT_PAP_DATA_SIZE;
		} else {
			resp.resp[i].iov_base = data;
			resp.resp[i].iov_len = len;
			break;
		}
	}
	resp.bitmap = (1 << (i + 1)) - 1;

	if (atp_sendreq(fd, &dest, 0, 0, userdata, 1, 0, 0, &resp, &retry, 0) < 0)
		return -1;

	for (len=0, i=0; i < ATP_TRESP_MAX; i++) {
		if (resp.bitmap & (1 << i))
			len += resp.resp[i].iov_len;
	}

	return len;
}


