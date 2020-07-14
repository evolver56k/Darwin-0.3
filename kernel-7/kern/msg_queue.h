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

#ifndef	_KERN_MSG_QUEUE_H_
#define _KERN_MSG_QUEUE_H_

#import <kern/queue.h>
#import <kern/lock.h>
#import <kernserv/macro_help.h>

typedef struct {
	queue_head_t messages;
	decl_simple_lock_data(,lock)
	queue_head_t blocked_threads;
} msg_queue_t;

#define msg_queue_lock(mq)	simple_lock(&(mq)->lock)
#define msg_queue_unlock(mq)	simple_unlock(&(mq)->lock)

#define msg_queue_init(mq)			\
MACRO_BEGIN					\
	simple_lock_init(&(mq)->lock);		\
	queue_init(&(mq)->messages);		\
	queue_init(&(mq)->blocked_threads);	\
MACRO_END

#endif	/* _KERN_MSG_QUEUE_H_ */
