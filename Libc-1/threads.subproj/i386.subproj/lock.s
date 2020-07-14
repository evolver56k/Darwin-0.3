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

#include <architecture/i386/asm_help.h>


/* 
 * void
 * spin_lock(p)
 *	int *p;
 *
 * Lock the lock pointed to by p.  Spin (possibly forever) until the next
 * lock is available.
 */
	TEXT

LEAF(_spin_lock, 0)
	movl	4(%esp), %ecx
1:
	movl	(%ecx), %eax
	orl	%eax, %eax
	jnz	1b
	movl	$0xFFFFFFFF, %eax
	xchgl	%eax, (%ecx)
	orl	%eax, %eax
	jnz	1b
END(_spin_lock)
	

/*
 * void
 * spin_unlock(p)
 *	int *p;
 *
 * Unlock the lock pointed to by p.
 */
LEAF(_spin_unlock, 0)
	movl	$0, %eax
	movl	4(%esp), %ecx
	xchgl	%eax, (%ecx)
END(_spin_unlock)



/*
 * int
 * mutex_try_lock(p)
 *	int *p;
 *
 * Try to lock p.  Return a non-zero if successful.
 */

LEAF(_mutex_try_lock, 0)
	movl	$0xFFFFFFFF, %eax
	movl	4(%esp), %ecx
	xchgl	%eax, (%ecx)
	notl	%eax
END(_mutext_try_lock)

/*
 * void
 * mutex_unlock_try(p)
 * 	int *p;
 */

LEAF(_mutex_unlock_try, 0)
	movl	$0, %eax
	movl	4(%esp), %ecx
	xchgl	%eax, (%ecx)
END(_mutex_unlock_try)
