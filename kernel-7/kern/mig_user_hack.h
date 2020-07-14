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
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_KERN_MIG_USER_HACK_H_
#define _KERN_MIG_USER_HACK_H_

/*
 * This file is meant to be imported with
 *	uimport <kern/mig_user_hack.h>;
 * by those interfaces for which the kernel (but not builtin tasks)
 * is the client.  See memory_object.defs and memory_object_default.defs.
 * It has the hackery necessary to make Mig-generated user-side stubs
 * usable by the kernel.
 *
 * The uimport places a
 *	#include <kern/mig_user_hack.h>
 * in the generated header file for the interface, so any file which
 * includes the interface header file will get these definitions,
 * not just the user stub file like we really want.
 */

#define msg_send	msg_send_from_kernel

#endif	/* _KERN_MIG_USER_HACK_H_ */
