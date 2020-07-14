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
 * $Id: nbp_reg.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

/* "@(#)nbp_reg.c: 2.0, 1.10; 9/27/89; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	nbp_reg.c
 *
 * Facility:	AppleTalk Name Binding Protocol Library Interface
 *
 * Author:	Gregory Burns, Creation Date: Jul-14-1988
 *
 * History:
 * X01-001	Gregory Burns	14-Jul-1988
 *	 	Initial Creation.
 *
 */
#include <h/sysglue.h>
#include <stdio.h>
#include <at/appletalk.h>
#include <at/nbp.h>
#include <h/atlog.h>
#include <h/lap.h>
#include <h/debug.h>
#include <unistd.h>

#include <errno.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

#ifndef NULL
#define	NULL	0
#endif /* NULL */

int
nbp_register (entity, fd, retry)
	at_entity_t	*entity;
	int		fd;
	at_retry_t	*retry;
{
	int		got, cnt=1;
	at_inet_t	addr;
	at_nbptuple_t	tuple;
	static at_entity_t	entity_copy[IF_TOTAL_MAX]; /* only one home zone per
												 port allowed so this is safe */
	int if_id, size,mode;

	if (fd < 0) {
		SET_ERRNO(EBADF);
		return (-1);
	}

	if (rtmp_netinfo(fd, &addr, NULL) < 0)
		return (-1);

	if (nbp_iswild (entity)) {
		FPRINTF(stderr,"nbp iswild failed\n"); 
	    SET_ERRNO(EINVAL);
	    return (-1);
	}
	
	entity_copy[0] = *entity;
#ifdef COMMENTED_OUT	/* removed to allow multi-zone registration */
	/* smash the zone name first */
	entity_copy.zone.len = 1;
	entity_copy.zone.str[0] = '*';
#endif /* COMMENTED_OUT */

	if ((if_id = openCtrlFile(NULL, "nbp_register",2,0)) < 0)
		return(-1);
	size = sizeof(at_nvestr_t);
	
	if (at_send_to_dev(if_id, LAP_IOC_IS_ZONE_LOCAL, &entity_copy[0].zone, &size)) {
		(void)close (if_id);
		return(-1);
	}
	size=0;
	if (at_send_to_dev(if_id, LAP_IOC_GET_MODE, &mode, &size)) {
		(void)close (if_id);
		return(-1);
	}


	if (mode == AT_MODE_MHOME && entity->zone.str[0] == '*')
	{
		size = sizeof(at_nvestr_t);
		for (cnt=0; cnt<IF_TOTAL_MAX; cnt++) {
			if (cnt)
				entity_copy[cnt] = entity_copy[0];
			*(int*)&entity_copy[cnt].zone = cnt;
			if (at_send_to_dev(if_id, LAP_IOC_GET_DEFAULT_ZONE, 
				&entity_copy[cnt].zone, &size))
			{
				FPRINTF(stderr,"error getting Appletalk local zones\n");
				SET_ERRNO(ENXIO);
				(void)close (if_id);
				return(-1);
			}
			/* LD 03/98: multihoming returns an zero sized entity 
			   when the if doesn't exist */
			if (size == 0 || !entity_copy[cnt].zone.len)
				break;
/*			entity_copy[cnt].zone.str[ entity_copy[cnt].zone.len] = '\0';
			printf("registering in zone %s\n", entity_copy[cnt].zone.str);
*/
		}
		if(!cnt) {
			FPRINTF(stderr,"error, no local zones\n");
			(void)close (if_id);
			SET_ERRNO(ENOENT);
			return(-1);
		}

		if ((got = nbp_lookup_multiple(&entity_copy, &tuple, 1, retry,cnt)) < 0)  {
			FPRINTF(stderr,"nbp lookup failed\n");
			(void)close (if_id);
			SET_ERRNO(ENOMSG);
			return (-1);
		}
	}
	else {
		if ((got = nbp_lookup(&entity_copy[0], &tuple, 1, retry)) < 0)  {
			FPRINTF(stderr,"nbp lookup failed\n");
			(void)close (if_id);
			SET_ERRNO(ENOMSG);
			return (-1);
		}
	}
	if (got > 0) {
		SET_ERRNO(EADDRNOTAVAIL);
		return (-1);
	}
	if ((got = nbp_send_multi(NBP_REGISTER, &addr, &entity_copy, NULL, 1, 
		retry,cnt)) < 0) {
		fprintf(stderr,"nbp send failed\n"); 
		return (-1);
	}
	if (got == 0) {
		SET_ERRNO(EADDRNOTAVAIL);
		return (-1);
	}
	return (0);
}	
