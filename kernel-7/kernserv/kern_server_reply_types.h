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
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 24-May-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#ifndef _KERN_SERVER_REPLY_TYPES_
#define _KERN_SERVER_REPLY_TYPES_
#ifdef	KERNEL_PRIVATE
#import <kern/xpr.h>
#endif	/* KERNEL_PRIVATE */
#import <mach/mach_types.h>

typedef port_name_t server_ref_t;
typedef char	panic_msg_t[256];
typedef char	macho_header_name_t[16];

/*
 * Log structure
 */
typedef struct xprbuf log_entry_t;
typedef log_entry_t *log_entry_array_t;
typedef struct {
	log_entry_t	*base;
	log_entry_t	*ptr;
	log_entry_t	*last;
	int		level;		// 0 == no logging
} log_t;

#endif /* _KERN_SERVER_REPLY_TYPES_ */

