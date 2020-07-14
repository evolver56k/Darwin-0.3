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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   


#ifndef __NBP__
#define __NBP__

/* NBP DDP socket type */
#define  NBP_DDP_TYPE		0x02  	/* NBP packet */

/* NBP DDP socket number */
#define  NBP_SOCKET		0x02  	/* NIS socket number */


/* NBP packet types */

#define NBP_BRRQ			0x01  	/* Broadcast request */
#define NBP_LKUP    		0x02  	/* Lookup */
#define NBP_LKUP_REPLY		0x03  	/* Lookup reply */
#define NBP_FWDRQ			0x04	/* Forward Request (router only) */
#define NBP_REGISTER    	0x07	/* Register a name */
#define NBP_DELETE      	0x08	/* Delete a name */
#define NBP_CONFIRM   		0x09	/* Confirm, not sent on wire */
#define NBP_STATUS_REPLY	0x0a	/* Status on register/delete */
#define	NBP_CLOSE_NOTE		0x0b	/* Close notification from DDP */


/* Protocol defaults */

#define NBP_RETRY_COUNT		8	/* Maximum repeats */
#define NBP_RETRY_INTERVAL	1	/* Retry timeout */


/* Special (partial) wildcard character */
#define	NBP_SPL_WILDCARD	0xC5
#define	NBP_ORD_WILDCARD	'='


#define NBP_NVE_STR_SIZE	32	/* Maximum NBP tuple string size */
typedef struct at_nvestr {
	u_char		len;
	u_char		str[NBP_NVE_STR_SIZE];
} at_nvestr_t;


/* Entity Name */

typedef struct at_entity {
	at_nvestr_t	object;
	at_nvestr_t	type;
	at_nvestr_t	zone;
} at_entity_t;


/* Packet definitions */

#define NBP_TUPLE_SIZE	((3*NBP_NVE_STR_SIZE)+3) /* 3 for field lengths + 3*32 for three names */
#define NBP_TUPLE_MAX	15	/* Maximum number of tuples in one DDP packet */
#define	NBP_HDR_SIZE	2

typedef struct at_nbptuple {
	at_inet_t	enu_addr;
	union {
	  struct {
            u_char	enumerator;
	    at_entity_t entity;
	  } en_se;
	  struct {
            u_char	enumerator;
            u_char	name[NBP_TUPLE_SIZE];
	  } en_sn;
	} en_u;
} at_nbptuple_t;

/* Some macros for easier access to the tuple union */
#define	enu_enum	en_u.en_sn.enumerator
#define	enu_name	en_u.en_sn.name
#define	enu_entity	en_u.en_se.entity


typedef struct at_nbp {
        unsigned      	control : 4,
        	      	tuple_count : 4;
/* fix for bug 2228529: 
        u_char      	id;*/
	u_char		at_nbp_id;
	at_nbptuple_t	tuple[NBP_TUPLE_MAX];
} at_nbp_t;

	/* misc */
#define IF_NAME_LEN		5

#endif /* __NBP__ */

