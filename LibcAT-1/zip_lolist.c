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
 * $Id: zip_lolist.c,v 1.1.1.1 1999/04/13 22:26:05 wsanchez Exp $
 */

/* "@(#)zip_lolist.c: 2.0, 1.12; 2/10/93; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	zip_locallist.c
 *
 * Facility:	AppleTalk Zone Information Protocol Library Interface
 *
 * History:
 *
 */

#include <stdio.h>
#include <h/sysglue.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>
#include <at/appletalk.h>
#include <at/atp.h>
#include <at/nbp.h>
#include <at/zip.h>
#include <h/lap.h>
#include <h/atlog.h>

#include <at/atp.h>
#include <at/asp_errno.h>

#include <mach/cthreads.h>

#include "at_proto.h"

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

#ifndef	NULL
#define	NULL	0
#endif	/* NULL */

#define 	TRUE  1
#define 	FALSE 0

#define TOTAL_ALLOWED ATP_DATA_SIZE-sizeof(at_nvestr_t)

static at_inet_t	abridge = { 0, 0, 0 };
int		last_getlocalzones = FALSE;

int
zip_getlocalzones (start, zones)
	int		start;
	at_nvestr_t	*zones[];
{
	int if_id;
	int status, size=0,mode;
	at_nvestr_t	*pz;
	int total=0, i;
	if_zone_t	ifz;

	if (start <1) {
		SET_ERRNO(EINVAL);
		return(-1);
	}
	if ((if_id = openCtrlFile(NULL, "zip_getlocalzones",2,0)) < 0)
			return(-1);
	status = at_send_to_dev(if_id, LAP_IOC_GET_MODE, &mode, &size);
	if (status) {
		/*fprintf(stderr, MSGSTR(M_TABLES, "error clearing tables %d\n"),cthread_errno()); */
		FPRINTF(stderr,"error getting Appletalk mode\n");
		(void)close (if_id);
		SET_ERRNO(ENXIO);
		return(-1);
	}
	switch (mode) {
	case AT_MODE_SPORT:
		(void)close (if_id);
		return(getSpLocalZones(start,zones));
		break;
	case AT_MODE_MHOME:
	case AT_MODE_ROUTER:
		pz = zones;
		size = sizeof(if_zone_t);
		ifz.ifzn.zone = i = start-1;
		total=0;
		for ( ; total < TOTAL_ALLOWED; ifz.ifzn.zone = ++i) {
			if (at_send_to_dev(if_id, LAP_IOC_GET_LOCAL_ZONE,&ifz, &size))
			{
				FPRINTF(stderr,"error getting Appletalk local zones\n");
				SET_ERRNO(ENOENT);
				(void)close (if_id);
				return(-1);
			}
			if (!ifz.ifzn.ifnve.len) {
				if (i == (start-1)) /* if none retrieved */
					i=0;
				break;			/* no more left */
			}
			*pz =ifz.ifzn.ifnve; 
			total += (pz->len + 1);
			pz = (at_nvestr_t*)((caddr_t)pz + pz->len +1);
		}
		(void)close (if_id);
		return(i);
		break;
	}
	return(-1);
}



int
getSpLocalZones(start,zones)
	int		start;
	at_nvestr_t	*zones[];
{
	int		fd;
	int		localtalk = 0;
	int		iff;
	int userdata;
	u_char *puserdata = (u_char *)&userdata;
	at_inet_t dest;
	at_retry_t retry;
	at_resp_t resp;

	if (last_getlocalzones) {
		last_getlocalzones = FALSE;
		return (0);
	}

	if ((iff = ddp_primary_interface()) == -1)
		return (-1);
	if (iff == AT_IFF_LOCALTALK)
		localtalk++;

	if (start == 1) {
		/* This is the first call, get the bridge node id */
		if (rtmp_netinfo(-1, NULL, &abridge) < 0) {
			SET_ERRNO(ENETUNREACH);
			return(-1);
		}
		if ((abridge.node == 0) && localtalk) { /* no router */
			at_nvestr_t	*zone;
			zone = (at_nvestr_t *) zones;
			zone->len = 1;
			zone->str[0] = '*';
			zone->str[1] = '\0';
			return -1;
		}
	} else {
		/* can't have multiple zones on LocalTalk... */
		if (localtalk) {
			SET_ERRNO(EINVAL);
			return (-1);
		}
		/* This isn't the first call, make sure we use the same ABridge */
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
	puserdata[0] = localtalk ? ZIP_GETMYZONE : ZIP_GETLOCALZONES;
	puserdata[1] = 0;
	*(short *)(&puserdata[2]) = localtalk ? 0 : start;
	resp.bitmap = 0x01;
	resp.resp[0].iov_base = (u_char *)zones;
	resp.resp[0].iov_len = ATP_DATA_SIZE;
	retry.interval = 2;
	retry.retries = 5;

	if (atp_sendreq(fd, &dest, 0, 0, userdata, 0, 0, 0,
			&resp, &retry, 0) >= 0) {
		puserdata = (u_char *)&resp.userdata[0];
		if (puserdata[0] == 1) {
			last_getlocalzones = TRUE;
			abridge.node = 0;
			NET_ASSIGN(abridge.net, 0);		
		}
		atp_close(fd);
		return (localtalk? 0 : 
			*(short *)(&puserdata[2]));
	} 
	atp_close(fd);
	return -1;
}
