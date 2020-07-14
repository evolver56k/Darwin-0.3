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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

/* Miscellaneous definitions for AppleTalk used by all protocol 
 * modules.
 */

#ifndef __APPLETALK__
#define __APPLETALK__

# include <sys/types.h>


/* New fundemental types: non-aligned variations of u_short and u_long */
typedef u_char ua_short[2];		/* Unaligned short */
typedef u_char ua_long[4];		/* Unaligned long */


/* Two at_net typedefs; the first is aligned the other isn't */
typedef u_short at_net_al;		/* Aligned AppleTalk network number */
typedef ua_short at_net_unal;		/* Unaligned AppleTalk network number */

/* Miscellaneous types */

typedef at_net_unal at_net;		/* Default: Unaligned AppleTalk network number */
typedef u_char	at_node;		/* AppleTalk node number */
typedef u_char  at_socket;		/* AppleTalk socket number */

/* AppleTalk Internet Address */

typedef struct at_inet {
    at_net	net;			/* Network Address */
    at_node	node;			/* Node number */
    at_socket	socket;			/* Socket number */
} at_inet_t;


/* AppleTalk protocol retry and timeout */

typedef struct at_retry {
    short	interval;		/* Retry interval in seconds */
    short	retries;		/* Maximum number of retries */
    u_char      backoff;                /* Retry backoff, must be 1 through 4 */
} at_retry_t;


struct atalk_addr {
	u_char	atalk_unused;
	at_net	atalk_net;
	at_node	atalk_node;
};

#define ATPROTO_NONE  0
#define ATPROTO_DDP   1
#define ATPROTO_LAP   2
#define ATPROTO_ATP   3
#define ATPROTO_ASP   4
#define ATPROTO_AURP  5
#define ATPROTO_ADSP  6
#define ATPROTO_LOOP  7  /* not used */

#endif /*  #ifndef __APPLETALK__ */
