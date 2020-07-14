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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_PS_DEFS_
#define	_PS_DEFS_

#define PS_DEBUG		1

#include <mach/port.h>

#include "mem.h"
#include "disp_hdr.h"
#include "key_defs.h"
#include <servers/nm_defs.h>
#include "port_defs.h"

/*
 * Structure used to send and reply to port search requests.
 */
typedef struct {
    disp_hdr_t		psd_disp_hdr;
    unsigned long	psd_status;
    np_uid_t		psd_puid;
    netaddr_t		psd_owner;
    netaddr_t		psd_receiver;
} ps_data_t, *ps_data_ptr_t;

/*
 * Status values.
 */
#define PS_OWNER_MOVED		1
#define PS_RECEIVER_MOVED	2
#define PS_PORT_DEAD		4
#define PS_PORT_UNKNOWN		8
#define PS_PORT_HERE		16


/*
 * Structure used to send and reply to authentication requests.
 */
typedef struct {
    disp_hdr_t		psa_disp_hdr;
    np_uid_t		psa_puid;
    secure_info_t	psa_token;
    long		psa_random;
} ps_auth_t, *ps_auth_ptr_t;


/*
 * Structure used to remember about port searches in progress.
 */
typedef struct {
    int		pse_state;
    netaddr_t	pse_destination;
    port_t	pse_lport;
    int		(*pse_retry)();
} ps_event_t, *ps_event_ptr_t;

/*
 * Possible event states.
 */
#define PS_OWNER_QUERIED	1
#define PS_RECEIVER_QUERIED	2
#define PS_DONE_BROADCAST	4
#define PS_AUTHENTICATION	8


/*
 * extern definitions internal to the Port Search module.
 */
extern ps_cleanup();
/*
ps_event_ptr_t	event_ptr;
int		reason;
*/


extern ps_handle_auth_reply();
/*
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
*/

extern ps_handle_auth_request();
/*
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
*/


extern ps_retry();
/*
ps_event_ptr_t	event_ptr;
*/

extern void ps_send_query();
/*
ps_event_ptr_t	event_ptr;
port_rec_ptr_t	port_rec_ptr;
*/

extern void ps_authenticate_port();
/*
ps_event_ptr_t	event_ptr;
port_rec_ptr_t	port_rec_ptr;
*/

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_PSEVENT;


#endif	_PS_DEFS_
