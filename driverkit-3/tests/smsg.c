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
/*	@(#)lfs.c	2.0	26/06/90	(c) 1990 NeXT	*/

/* 
 * smsg.c -- Send simple message to arbitrary server.
 *
 * HISTORY
 * 23-Jun-91	Doug Mitchell at NeXT
 *	Clones from lfs.c.
 */

#import <sys/types.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <ansi/stdlib.h>
#import <bsd/libc.h>
#import <mach/port.h>
#import <servers/netname.h>

extern void usage(char **argv);

int main(int argc, char **argv)
{
	kern_return_t krtn;
	int arg;
	char *hostname = "";
	port_t fs_port, local_port;
	msg_header_t msg;
	
	if(argc < 3) 
		usage(argv);
	for(arg=3; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    default:
		    	usage(argv);
		}
	}
	krtn = port_allocate(task_self(), &local_port);
	if(krtn) {
		mach_error("port_allocate", krtn);
		exit(1);
	}
	krtn = netname_look_up(name_server_port, hostname, argv[1], &fs_port);
	if(krtn) {
	    	mach_error("netname_look_up", krtn);
		exit(1);
	}
	
	/*
	 * Cook up a basic message.
	 */
	msg.msg_id = atoi(argv[2]);
	msg.msg_size = sizeof(msg_header_t);
	msg.msg_local_port = local_port;
	msg.msg_remote_port = fs_port;
	krtn = msg_rpc(&msg,
		MSG_OPTION_NONE,	
		msg.msg_size,		
		0,			/* send_timeout */
		0);			/* rcv_timeout */
	if(krtn) {
		mach_error("msg_rpc", krtn);
		exit(1);
	}
	printf("...OK\n");
	exit(0);
}

void usage(char **argv)
{
	printf("usage: %s server_name msg_id [h=hostname]\n", argv[0]);
	exit(1);
}
