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
 *	Copyright (c) 1997, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 * $Id: atalk_status.c,v 1.1.1.1 1999/04/13 22:26:03 wsanchez Exp $
 */


/* @(#)atalk_status.c:  Copyright 1997, Apple Computer, Inc. */
/*
  11-21-97 Vida Amani new
*/

#include <h/sysglue.h>
#include <fcntl.h>
#include <errno.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/atp.h>
#include <at/nbp.h>
#include <at/asp_errno.h>
#include "at_proto.h"

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

extern int atalkState();

/* if OTHERERROR is returned then errno may be checked for the reason */
int
checkATStack()
{
	int		*pid_buf;
	int		status=0;
	int		rc=0;

	/*SET_ERRNO(0);*/
	rc = atalkState (0, (char **)&pid_buf);	
	/*printf ("returned from atalkState rc= %d\n", rc);
	printf ("returned from atalkState errno= %d\n", cthread_errno());*/
 
	if (rc == 0)
		status= RUNNING;
	else {
		switch (cthread_errno()) {
			case ENETDOWN:
				status=NOTLOADED;
				break;

			case ENOTREADY:
				status=LOADED;
				break;

			default:
				status=OTHERERROR;
		}	
	}
	
	return status;		
}

