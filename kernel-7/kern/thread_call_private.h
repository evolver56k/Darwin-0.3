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
 * Copyright (c) 1993, 1994, 1995 NeXT Computer, Inc.
 *
 * Private declarations for thread-based callouts.
 *
 * HISTORY
 *
 * 8 June 1995 ? at NeXT
 *	Converted from old API naming and
 *	changed the time representation to
 *	tvalspec_t.
 *
 * 7 April 1994 ? at NeXT
 *	Added support for external and delayed
 *	entries.  Cleaned up somewhat.
 * 3 July 1993 ? at NeXT
 *	Created.
 */

#ifndef	_KERN_THREAD_CALL_PRIVATE_H_
#define	_KERN_THREAD_CALL_PRIVATE_H_

#import <kern/queue.h>

typedef struct _thread_call {
    queue_chain_t	q_link;
    thread_call_func_t	func;
    thread_call_spec_t	spec;
    thread_call_spec_t	spec_proto;
    tvalspec_t		deadline;
    enum { IDLE, PENDING, DELAYED }
    			status;
} *_thread_call_t;

#endif	/* _KERN_THREAD_CALL_PRIVATE_H_ */
