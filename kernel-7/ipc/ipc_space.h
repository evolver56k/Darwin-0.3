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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	ipc/ipc_space.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for IPC spaces of capabilities.
 */

#ifndef	_IPC_IPC_SPACE_H_
#define _IPC_IPC_SPACE_H_

#import <mach/features.h>

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <kern/macro_help.h>
#include <kern/lock.h>
#include <kern/zalloc.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_splay.h>
#include <ipc/ipc_types.h>

/*
 *	Every task has a space of IPC capabilities.
 *	IPC operations like send and receive use this space.
 *	IPC kernel calls manipulate the space of the target task.
 *
 *	Every space has a non-NULL is_table with is_table_size entries.
 *	A space may have a NULL is_tree.  is_tree_small records the
 *	number of entries in the tree that, if the table were to grow
 *	to the next larger size, would move from the tree to the table.
 *
 *	is_growing marks when the table is in the process of growing.
 *	When the table is growing, it can't be freed or grown by another
 *	thread, because of krealloc/kmem_realloc's requirements.
 */

typedef unsigned int ipc_space_refs_t;

struct ipc_space {
	decl_simple_lock_data(,is_ref_lock_data)
	ipc_space_refs_t is_references;

	decl_simple_lock_data(,is_lock_data)
	boolean_t is_active;		/* is the space alive? */
	boolean_t is_growing;		/* is the space growing? */
	ipc_entry_t is_table;		/* an array of entries */
	ipc_entry_num_t is_table_size;	/* current size of table */
	struct ipc_table_size *is_table_next; /* info for larger table */
	struct ipc_splay_tree is_tree;	/* a splay tree of entries */
	ipc_entry_num_t is_tree_total;	/* number of entries in the tree */
	ipc_entry_num_t is_tree_small;	/* # of small entries in the tree */
	ipc_entry_num_t is_tree_hash;	/* # of hashed entries in the tree */

#if	MACH_IPC_COMPAT
	struct ipc_port *is_notify;	/* notification port */
#endif	/* MACH_IPC_COMPAT */
};

#define	IS_NULL			((ipc_space_t) 0)

extern zone_t ipc_space_zone;

#define is_alloc()		((ipc_space_t) zalloc(ipc_space_zone))
#define	is_free(is)		zfree(ipc_space_zone, (vm_offset_t) (is))

extern ipc_space_t ipc_space_kernel;
extern ipc_space_t ipc_space_reply;

#define	is_ref_lock_init(is)	simple_lock_init(&(is)->is_ref_lock_data)

#define	ipc_space_reference_macro(is)					\
MACRO_BEGIN								\
	simple_lock(&(is)->is_ref_lock_data);				\
	assert((is)->is_references > 0);				\
	(is)->is_references++;						\
	simple_unlock(&(is)->is_ref_lock_data);				\
MACRO_END

#define	ipc_space_release_macro(is)					\
MACRO_BEGIN								\
	ipc_space_refs_t _refs;						\
									\
	simple_lock(&(is)->is_ref_lock_data);				\
	assert((is)->is_references > 0);				\
	_refs = --(is)->is_references;					\
	simple_unlock(&(is)->is_ref_lock_data);				\
									\
	if (_refs == 0)							\
		is_free(is);						\
MACRO_END

#define	is_lock_init(is)	simple_lock_init(&(is)->is_lock_data)

#define	is_read_lock(is)	simple_lock(&(is)->is_lock_data)
#define is_read_unlock(is)	simple_unlock(&(is)->is_lock_data)

#define	is_write_lock(is)	simple_lock(&(is)->is_lock_data)
#define	is_write_lock_try(is)	simple_lock_try(&(is)->is_lock_data)
#define is_write_unlock(is)	simple_unlock(&(is)->is_lock_data)

#define	is_write_to_read_lock(is)

/* Take a reference on a space */
extern void ipc_space_reference(
	ipc_space_t	space);

/* Realase a reference on a space */
extern void ipc_space_release(
	ipc_space_t	space);

#define	is_reference(is)	ipc_space_reference(is)
#define	is_release(is)		ipc_space_release(is)

/* Create  new IPC space */
extern kern_return_t ipc_space_create(
	ipc_table_size_t	initial,
	ipc_space_t		*spacep);

/* Create a special IPC space */
extern kern_return_t ipc_space_create_special(
	ipc_space_t	*spacep);

/* Mark a space as dead and cleans up the entries*/
extern void ipc_space_destroy(
	ipc_space_t	space);

#if	MACH_IPC_COMPAT

/*
 *	Routine:	ipc_space_make_notify
 *	Purpose:
 *		Given a space, return a send right for a notification.
 *		May return IP_NULL/IP_DEAD.
 *	Conditions:
 *		The space is locked (read or write) and active.
 *
 *	ipc_port_t
 *	ipc_space_make_notify(space)
 *		ipc_space_t space;
 */

#define	ipc_space_make_notify(space)	\
		ipc_port_copy_send(space->is_notify)

#endif	/* MACH_IPC_COMPAT */

#endif	/* _IPC_IPC_SPACE_H_ */
