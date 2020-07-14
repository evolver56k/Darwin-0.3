/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 *  9-Jan-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#import <mach/mach.h>
#import "async.h"
#import <libc.h>
#import <mach/cthreads.h>
#import <kernserv/kern_server_reply_handler.h>
#import "kern_loader_handler.h"

extern kern_server_reply_t kern_server_reply;
extern kern_loader_t kern_loader;

typedef struct {
	msg_header_t		*m;
	kern_server_reply_t	r;
} handler_call_reply_t;

typedef struct {
	msg_header_t	*m;
} handler_call_user_t;

static void handler_call_reply(handler_call_reply_t *h)
{
	kern_server_reply_handler(h->m, &h->r);
	free(h->m);
	free(h);
}

void handler_fork_reply(msg_header_t *msg, void *s)
{
	handler_call_reply_t *h = malloc(sizeof(*h));
	h->m = (msg_header_t *)malloc(msg->msg_size);
	bcopy((char *)msg, (char *)h->m, msg->msg_size);
	h->r = kern_server_reply;
	h->r.arg = s;

	cthread_detach(cthread_fork((cthread_fn_t)handler_call_reply, h));
}

static void handler_call_user(handler_call_user_t *h)
{
	kern_loader_handler(h->m, &kern_loader);
	free(h->m);
	free(h);
}

void handler_fork_user(msg_header_t *msg)
{
	handler_call_user_t *h = malloc(sizeof(*h));
	h->m = (msg_header_t *)malloc(msg->msg_size);
	bcopy((char *)msg, (char *)h->m, msg->msg_size);

	cthread_detach(cthread_fork((cthread_fn_t)handler_call_user, h));
}
