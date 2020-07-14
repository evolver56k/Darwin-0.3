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
 * $Id: zip_zlist.c,v 1.1.1.1 1999/04/13 22:26:05 wsanchez Exp $
 */

/* "@(#)zip_zlist.c: 2.0, 1.12; 2/10/93; Copyright 1988-89, Apple Computer, Inc." */

#include <h/sysglue.h>
#include <fcntl.h>
#include <at/appletalk.h>
#include <at/zip.h>

#include <at/atp.h>
#include <at/nbp.h>
#include <at/asp_errno.h>

#include <mach/cthreads.h>

#include "at_proto.h"

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

static at_inet_t	abridge = { 0, 0, 0 };
static int		last = FALSE;

int zip_getzonelist(start, zones)
int		start;
at_nvestr_t	*zones[];
{
	int fd;
	int userdata;
	u_char *puserdata = (u_char *)&userdata;
	at_inet_t dest;
	at_retry_t retry;
	at_resp_t resp;

	if (last) {
		last = FALSE;
		return (0);
	}

	if (start == 1) {
		/* This is the first call, get the bridge node id */
		if (rtmp_netinfo(-1, NULL, &abridge) < 0) {
			SET_ERRNO(ENETUNREACH);
			return(-1);
		}
		if (abridge.node == 0) { /* no router */
			at_nvestr_t	*zone;
			zone = (at_nvestr_t *) zones;
			zone->len = 1;
			zone->str[0] = '*';
			zone->str[1] = '\0';
			return -1;
		}
	} else {
		/* This isn't the 1st call, make sure we use the same ABridge */
		if (abridge.node == 0) {
			SET_ERRNO(EINVAL); /* Never started with start == 1 */
			return(-1);
		}
	}

	fd = atp_open(NULL);
	if (fd < 0)
		return(NULL);

	NET_NET(dest.net, abridge.net);
	dest.node = abridge.node;
	dest.socket = ZIP_SOCKET;
	puserdata[0] = ZIP_GETZONELIST;
	puserdata[1] = 0;
	*(short *)(&puserdata[2]) = start;
	resp.bitmap = 0x01;
	resp.resp[0].iov_base = (u_char *)zones;
	resp.resp[0].iov_len = ATP_DATA_SIZE;
	retry.interval = 2;
	retry.retries = 5;

	if (atp_sendreq(fd, &dest, 0, 0, userdata, 0, 0, 0,
			&resp, &retry, 0) >= 0) {
		/* Connection established okay, just for the sake of our
		* sanity, check the other fields in the packet
		*/
		puserdata = (u_char *)&resp.userdata[0];
		if (puserdata[0] == 1) {
			last = TRUE;
			abridge.node = 0;
			NET_ASSIGN(abridge.net, 0);		
		}
		atp_close(fd);
		return (*(short *)(&puserdata[2]));
	} 
	atp_close(fd);
	return -1;
}
