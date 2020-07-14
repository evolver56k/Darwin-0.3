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
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 *	File:	kern/server_loop.c
 *
 *	A common server loop for builtin pagers.
 */

/*
 *	Must define symbols for:
 *		SERVER_NAME		String name of this module
 *		TERMINATE_FUNCTION	How to handle port death
 *		SERVER_LOOP		Routine name for the loop
 *		SERVER_DISPATCH		MiG function(s) to handle message
 *
 *	May optionally define:
 *		RECEIVE_OPTION		Receive option (default NONE)
 *		RECEIVE_TIMEOUT		Receive timeout (default 0)
 *		TIMEOUT_FUNCTION	Timeout handler (default null)
 *
 *	Must redefine symbols for pager_server functions.
 */

#ifndef	RECEIVE_OPTION
#define RECEIVE_OPTION	MSG_OPTION_NONE
#endif	RECEIVE_OPTION

#ifndef	RECEIVE_TIMEOUT
#define RECEIVE_TIMEOUT	0
#endif	RECEIVE_TIMEOUT

#ifndef	TIMEOUT_FUNCTION
#define TIMEOUT_FUNCTION
#endif	TIMEOUT_FUNCTION

#import <mach/boolean.h>
#import <mach/port.h>
#import <mach/message.h>
#import <mach/notify.h>

void		SERVER_LOOP(rcv_set, do_notify)
	port_t		rcv_set;
	boolean_t	do_notify;
{
	msg_return_t	r;
	port_t		my_notify;
	port_t		my_self;
	vm_offset_t	messages;
	register
	msg_header_t	*in_msg;
	msg_header_t	*out_msg;

	/*
	 *	Find out who we are.
	 */

	my_self = task_self();

	if (do_notify) {
		my_notify = task_notify();

		if (port_set_add(my_self, rcv_set, my_notify)
						!= KERN_SUCCESS) {
			printf("%s: can't enable notify port", SERVER_NAME);
			panic(SERVER_NAME);
		}
	}

	/*
	 *	Allocate our message buffers.
	 *	[The buffers must reside in kernel space, since other
	 *	message buffers (in the mach_user_external module) are.]
	 */

	if ((messages = kmem_alloc(kernel_map, 2 * MSG_SIZE_MAX)) == 0) {
		printf("%s: can't allocate message buffers", SERVER_NAME);
		panic(SERVER_NAME);
	}
	in_msg = (msg_header_t *) messages;
	out_msg = (msg_header_t *) (messages + MSG_SIZE_MAX);

	/*
	 *	Service loop... receive messages and process them.
	 */

	for (;;) {
		in_msg->msg_local_port = rcv_set;
		in_msg->msg_size = MSG_SIZE_MAX;
		if ((r = msg_receive(in_msg, RECEIVE_OPTION, RECEIVE_TIMEOUT)) != RCV_SUCCESS) {
			if (r == RCV_TIMED_OUT) {
				TIMEOUT_FUNCTION ;
			} else {
				printf("%s: receive failed, 0x%x.\n", SERVER_NAME, r);
			}
			continue;
		}
		if (do_notify && (in_msg->msg_local_port == my_notify)) {
			notification_t	*n = (notification_t *) in_msg;
			switch(in_msg->msg_id) {
				case NOTIFY_PORT_DELETED:
					TERMINATE_FUNCTION(n->notify_port);
					break;
				default:
					printf("%s: wierd notification (%d)\n", SERVER_NAME, in_msg->msg_id);
					printf("port = 0x%x.\n", n->notify_port);
					break;
			}
			continue;
		}
		if (SERVER_DISPATCH(in_msg, out_msg) &&
		    (out_msg->msg_remote_port != PORT_NULL))
			msg_send(out_msg, MSG_OPTION_NONE, 0);
	}
}

