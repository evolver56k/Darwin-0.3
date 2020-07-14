/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*
 *	Copyright (c) 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* "@(#)nbp.h: 2.0, 1.7; 7/26/89; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	nbp.h
 *
 * Facility:	Include file for NBP kernel module.
 *
 * Author:	Kumar Vora, Creation Date: May-1-1989
 *
 * History:
 * X01-001	Kumar Vora	May-1-1989
 *	 	Initial Creation.
 */

/* Struct for name registry */
typedef struct _nve_ {
	struct	_nve_		*fwd;
	struct	_nve_		*bwd;
	gbuf_t			*tag;		/*pointer to the parent gbuf_t*/
	at_nvestr_t		zone;
	u_int			zone_hash;
	at_nvestr_t		object;
	u_int			object_hash;
	at_nvestr_t		type;
	u_int			type_hash;
	at_inet_t		address;
	u_char			enumerator;
	u_char			ddptype;
	int				pid;
} nve_entry_t;

#define	NBP_WILD_OBJECT	0x01
#define	NBP_WILD_TYPE	0x02
#define	NBP_WILD_MASK	0x03

typedef	struct	nbp_req	{
	int		(*func)();
	gbuf_t		*response;	/* the response datagram	*/
	int		space_unused;	/* Space available in the resp	*/
					/* packet.			*/
	gbuf_t		*request;	/* The request datagram		*/
					/* Saved for return address	*/
	nve_entry_t	nve;
	u_char		flags;		/* Flags to indicate whether or	*/
					/* not the request tuple has	*/
					/* wildcards in it		*/
} nbp_req_t;


#define DEFAULT_ZONE(nve) (nve->len == 1 && nve->str[0] == '*')
