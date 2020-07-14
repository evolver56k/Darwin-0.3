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
 * $Id: zip_myzone.c,v 1.1.1.1 1999/04/13 22:26:05 wsanchez Exp $
 */

/* "@(#)zip_myzone.c: 2.0, 1.11; 11/2/92; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	zip_myzone.c
 *
 * Facility:	AppleTalk Zone Information Protocol Library Interface
 *
 * Author:	Gregory Burns, Creation Date: Jun-24-1988
 *
 * History:
 * X01-001	Gregory Burns	24-Jun-1988
 *	 	Initial Creation.
 *
 */

#include <h/sysglue.h>
#include <fcntl.h>
#include <at/appletalk.h>
#include <at/zip.h>
#include <at/ddp.h>
#include <h/lap.h>
#include <h/atlog.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

#ifndef NULL
#define NULL	0
#endif /* NULL */

int
zip_getmyzone (zone)
	at_nvestr_t	*zone;
{
	int		ddp_ctl_fd;
	int		status;
	int		size = 0;
	at_zip_cfg_t	zip_cfg;
	int		iff;

	if ((iff = ddp_primary_interface()) == -1) 
	    return (-1);

	switch (iff) {
	case AT_IFF_ETHERTALK :
		if ((ddp_ctl_fd = ddp_open(NULL)) < 0) {
		    PRINTF("zip_myzone: ddp_open failed\n");
			SET_ERRNO(EIO);
		    return (-1);
		}
    		if ((status = at_send_to_dev(ddp_ctl_fd, ZIP_IOC_GET_CFG,
					     &zip_cfg, &size)) < 0) {
		    PRINTF("zip_myzone: at_send_to_dev failed status=%d\n", status);
			SET_ERRNO(ENXIO);
		    return (-1);
		}

		zone->len = zip_cfg.zonename.len;
		memcpy(zone->str, zip_cfg.zonename.str, zone->len);

		/* Append zero byte */
		zone->str[zone->len] = '\0';

		(void) ddp_close(ddp_ctl_fd);
		break;
	}
	return (0);
}	
