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

/*	@(#)ipc_notify.h	2.0	05/11/90	(c) 1990 NeXT	*/

/* 
 * ipc_notify.h - definition of interface to port death notification server
 *
 * HISTORY
 * 05-11-90	Doug Mitchell at NeXT
 *	Created.
 */ 

#ifndef	_IPC_NOTIFY_
#define _IPC_NOTIFY_

#define PORT_REGISTER_NOTIFY	0x76543		/* register notification request
						 * msg_id */

/*
 * message struct for registering for port death notification with voltask
 * (msg_id = PORT_REGISTER_NOTIFY).
 */
struct port_register_notify {
	msg_header_t	prn_header;
	msg_type_t	prn_p_type;
	kern_port_t	prn_rights_port;	/* port to which requesting 
						 * thread has send rights */
	kern_port_t	prn_notify_port;	/* port to which notification
						 * should be sent when 
						 * prn_rights_port dies */
	msg_type_t	prn_rpr_type;
	int		prn_rp_rep;		/* non-translated 
						 * representation of 
						 * prn_rights_port */
};

typedef struct port_register_notify *port_register_notify_t;

void port_request_notification(kern_port_t reg_port, 
	kern_port_t notify_port);
kern_return_t get_kern_port(task_t task, port_t in_port, kern_port_t *kport);
void pnotify_start();

#endif	/* _IPC_NOTIFY_ */

