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
 * $Id: rtmp_info.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

/* "@(#)rtmp_info.c: 2.0, 1.9; 7/17/89; Copyright 1988-89, Apple Computer, Inc." */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <h/at-config.h>
#include <h/lap.h>
#include <h/atlog.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

int
rtmp_netinfo(fd, node, router)
int		fd;
at_inet_t	*node;
at_inet_t	*router;
{
	int len, dyn_fd = -1;
	at_ddp_cfg_t cfg;
	int status;

	if (!node && !router) {
		SET_ERRNO(EINVAL);
		return(-1);
	}

	if (fd == -1)
		if ((dyn_fd = ddp_open(NULL)) < 0)
			return (-1);

	len = sizeof(cfg);
	status = at_send_to_dev((dyn_fd != -1 ? dyn_fd : fd),
		DDP_IOC_GET_CFG, &cfg, &len);

	if (dyn_fd != -1) {
		ddp_close(dyn_fd);
	}
	if (status < 0)
		return(-1);

	if (node) {
		NET_NET(node->net, cfg.node_addr.net);
		node->node = cfg.node_addr.node;
		node->socket = (fd == -1) ? 0 : cfg.node_addr.socket;
	}

	if (router) {
		router->node = cfg.router_addr.node;
		router->socket = 0;
		if (cfg.flags & AT_IFF_ETHERTALK)
			NET_NET(router->net, cfg.router_addr.net);
		else
			NET_ASSIGN(router->net, 0);
	}
	return(0);
}

