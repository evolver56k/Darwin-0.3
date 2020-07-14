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
 * $Id: valid_addr.c,v 1.1.1.1 1999/04/13 22:26:05 wsanchez Exp $
 */

/* "@(#)valid_addr.c: 2.0, 1.3; 9/13/89; Copyright 1988-89, Apple Computer, Inc." */

#include <h/sysglue.h>
#include <at/appletalk.h>

#include <mach/cthreads.h>

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

int
_validate_at_addr(addr)
at_inet_t	*addr;
{
	/* make sure the net, node and socket numbers are in legal range :
	 *
	 * Net#		0		Local Net
	 *		1 - 0xfffe	Legal net nos
	 *		0xffff		Reserved by Apple for future use.
	 * Node#	0		Illegal
	 *		1 - 0x7f	Legal (user node id's)
	 *		0x80 - 0xfe	Legal (server node id's; 0xfe illegal in
	 *				Phase II nodes)
	 *		0xff		Broadcast
	 * Socket#	0		Illegal
	 *		1 - 0xfe	Legal
	 *		0xff		Illegal
	 */

	if (NET_VALUE(addr->net) == 0xffff || addr->node == 0 || addr->socket == 0 || 
		addr->socket == 0xff) {
		SET_ERRNO(EINVAL);
		return (-1);
	} else
		return (0);
}
