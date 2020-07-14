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

/* Definitions for ATP protocol and streams module, per 
 * AppleTalk Transaction Protocol documentation from
 * `Inside AppleTalk', July 14, 1986.
 */


#ifndef __ATP__
#define __ATP__

#ifndef UIOSEG_USER
# include <sys/uio.h>
#endif


/* DDP ATP protocol type */

#define ATP_DDP_TYPE		0x03	/* ATP packet type */


/* ATP function codes */

#define ATP_CMD_TREQ		0x01	/* TRequest packet  */
#define ATP_CMD_TRESP		0x02	/* TResponse packet */
#define ATP_CMD_TREL		0x03	/* TRelease packet  */


/* Miscellaneous definitions */

#define	ATP_DEF_RETRIES     8	/* Default for maximum retry count */
#define	ATP_DEF_INTERVAL    2	/* Default for retry interval in seconds */

#define ATP_TRESP_MAX       8	/* Maximum number of Tresp pkts */

#define ATP_HDR_SIZE        8  	/* Size of the ATP header */
#define ATP_DATA_SIZE       578  	/* Maximum size of the ATP data area */

/* Consts for asynch support */
#define	ATP_ASYNCH_REQ	1
#define	ATP_ASYNCH_RESP	2

/* Timer values for XO release timers */
#define	ATP_XO_DEF_REL_TIME	0
#define	ATP_XO_30SEC		0
#define	ATP_XO_1MIN		1
#define	ATP_XO_2MIN		2
#define	ATP_XO_4MIN		3
#define	ATP_XO_8MIN		4

typedef struct {
        unsigned       cmd : 2,
                       xo : 1,
                       eom : 1,
                       sts : 1,
                       xo_relt : 3;
        u_char         bitmap;
	ua_short       tid;
        ua_long        user_bytes;
        u_char         data[ATP_DATA_SIZE];
} at_atp_t;


/* Response buffer structure for atp_sendreq() and atp_sendrsp() */

typedef	struct	at_resp {
	u_char	bitmap;				/* Bitmap of responses */
	u_char	filler[3];			/* Force 68K to RISC alignment */
	struct	iovec resp[ATP_TRESP_MAX];	/* Buffer for response data */
	long	userdata[ATP_TRESP_MAX];	/* Buffer for response user data */
} at_resp_t;

#endif /* __ATP__ */
