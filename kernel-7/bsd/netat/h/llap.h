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

/* @(#)llap.h: 2.0, 1.5; 7/10/91; Copyright 1988-89, Apple Computer, Inc. */

#ifndef __LLAP__
#define __LLAP__

# include <h/lap.h>

/* LLAP packet definitions */

#define  LLAP_HDR_SIZE                 3	/* LAP header size.	*/
#define  LLAP_DATA_SIZE              600	/* LAP data size.	*/
#define  LLAP_SIZE                   603	/* LAP frame size.	*/


/* LLAP header */

typedef struct {
        at_node  destination;			/* Destination address. */
        at_node  source;  			/* Source address.      */
        u_char   type;  			/* Frame type.          */
} at_llap_hdr_t;


/* LLAP packet */
  
typedef struct {
        at_node  destination;			/* Destination address. */
        at_node  source;  			/* Source address.      */
        u_char   type;  			/* Frame type.          */
        char     data[LLAP_DATA_SIZE];		/* data area.           */
} at_llap_t;


/* LLAP ioctl and driver definitions */

#define	LLAP_IOC_MYIOCTL(i)	((i>>8) == AT_MID_LLAP)
#define	LLAP_IOC_GET_CFG	((AT_MID_LLAP<<8) | 1)
#define	LLAP_IOC_GET_STATS	((AT_MID_LLAP<<8) | 2)
#define	LLAP_IOC_SET_CFG	((AT_MID_LLAP<<8) | 3)

typedef struct {
	u_long  unknown_irupts;		/* number of unexpected interrupts, recovery */
	u_long  unknown_mblks;		/* number of unknown stream messages */
	u_long  ioc_unregistered;	/* number of AT_SYNCs on sockets that closed */
					/* before it filtered through q for response */
	u_long  timeouts;	        /* number of state timeouts occured */
	u_long  rcv_bytes;	  	/* number of productive data bytes received */
	u_long  rcv_packets;		/* number of productive packets received */
	u_long  type_unregistered; 	/* number of packets thrown away because */
					/* no one is no one listenning for it */
	u_long  overrun_errors;    	/* number of overruns received */
	u_long  abort_errors;      	/* number of aborts received */
	u_long  crc_errors;		/* number of crc errors received */
	u_long  too_long_errors;   	/* number of packets which are too big */
	u_long  too_short_errors;  	/* number of packets which are too small */
	u_long  missing_sync_irupt;	/* number of missing sync interrupts while */
					/* while waiting for access to a busy net */
	u_long  xmit_bytes;		/* number of productive data bytes xmited */
	u_long  xmit_packets;		/* number of productive packets xmited */
	u_long  collisions;		/* number of collisions on xmit */
	u_long  defers;			/* number of defers on xmit */
	u_long  underrun_errors;   	/* number of underruns transmitted */
} at_llap_stats_t;

typedef struct {
        short    network_up;  	/* 0=network down, nonzero up.  */
	short	filler;	
        int      node;  	/* Our node number.             */
        int      initial_node;  /* Our initial node address.    */
        int      rts_attempts;  /* RTS attempts on write.       */
} at_llap_cfg_t;

/* Miscellaneous definitions */

#define  LLAP_TYPE_DDP              0x01  /* DDP short header packet.      */
#define  LLAP_TYPE_DDP_X            0x02  /* DDP extended header packet.   */

#endif /* __LLAP__ */
