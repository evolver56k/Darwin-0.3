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

#ifndef __PAP__
#define __PAP__


#define  AT_PAP_DATA_SIZE	      512    /* Maximum PAP data size */
#define  AT_PAP_STATUS_SIZE	      255    /* Maximum PAP status length */
#define  PAP_TIMEOUT		      120

/* PAP packet types */

#define  AT_PAP_TYPE_OPEN_CONN        0x01   /* Open-Connection packet */
#define  AT_PAP_TYPE_OPEN_CONN_REPLY  0x02   /* Open-Connection-Reply packet */
#define  AT_PAP_TYPE_SEND_DATA        0x03   /* Send-Data packet */
#define  AT_PAP_TYPE_DATA             0x04   /* Data packet */
#define  AT_PAP_TYPE_TICKLE           0x05   /* Tickle packet */
#define  AT_PAP_TYPE_CLOSE_CONN       0x06   /* Close-Connection packet */
#define  AT_PAP_TYPE_CLOSE_CONN_REPLY 0x07   /* Close-Connection-Reply pkt */
#define  AT_PAP_TYPE_SEND_STATUS      0x08   /* Send-Status packet */
#define  AT_PAP_TYPE_SEND_STS_REPLY   0x09   /* Send-Status-Reply packet */
#define  AT_PAP_TYPE_READ_LW	      0x0A   /* Read LaserWriter Message */


/* PAP packet structure */

typedef struct {
        u_char     at_pap_connection_id;
        u_char	   at_pap_type;
        u_char     at_pap_sequence_number[2];
        at_socket  at_pap_responding_socket;
        u_char     at_pap_flow_quantum;
        u_char     at_pap_wait_time_or_result[2];
        u_char     at_pap_buffer[AT_PAP_DATA_SIZE];
} at_pap;


/* ioctl definitions */

#define	AT_PAP_SETHDR		(('~'<<8)|0)
#define	AT_PAP_READ		(('~'<<8)|1)
#define	AT_PAP_WRITE		(('~'<<8)|2)
#define	AT_PAP_WRITE_EOF	(('~'<<8)|3)
#define	AT_PAP_WRITE_FLUSH	(('~'<<8)|4)
#define	AT_PAP_READ_IGNORE	(('~'<<8)|5)
#define	AT_PAPD_SET_STATUS	(('~'<<8)|40)
#define	AT_PAPD_GET_NEXT_JOB	(('~'<<8)|41)

extern	char	at_pap_status[];
extern  char   *pap_status ();

#endif /* __PAP__ */
