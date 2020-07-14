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
 *	File:	ipc/ipc_hash.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Declarations of entry hash table operations.
 */

#ifndef	_IPC_IPC_HASH_H_
#define _IPC_IPC_HASH_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach_debug/hash_info.h>

/*
 * Exported interfaces
 */

/* Lookup (space, obj) in the appropriate reverse hash table */
extern boolean_t ipc_hash_lookup(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_t		*namep,
	ipc_entry_t		*entryp);

/* Insert an entry into the appropriate reverse hash table */
extern void ipc_hash_insert(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_t		name,
	ipc_entry_t		entry);

/* Delete an entry from the appropriate reverse hash table */
extern void ipc_hash_delete(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_t		name,
	ipc_entry_t		entry);

/*
 *	For use by functions that know what they're doing:
 *	the global primitives, for splay tree entries,
 *	and the local primitives, for table entries.
 */

/* Delete an entry from the global reverse hash table */
extern void ipc_hash_global_delete(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_t		name,
	ipc_tree_entry_t	entry);

/* Lookup (space, obj) in local hash table */
extern boolean_t ipc_hash_local_lookup(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_t		*namep,
	ipc_entry_t		*entryp);

/* Inserts an entry into the local reverse hash table */
extern void ipc_hash_local_insert(
	ipc_space_t		space,
	ipc_object_t		obj,
	mach_port_index_t	index,
	ipc_entry_t		entry);

/* Initialize the reverse hash table implementation */
extern void ipc_hash_init(void);

#include <mach_ipc_debug.h>

#if	MACH_IPC_DEBUG

extern unsigned int ipc_hash_info(
	hash_info_bucket_t	*info,
	unsigned int		 count);

#endif	/* MACH_IPC_DEBUG */

#endif	/* _IPC_IPC_HASH_H_ */
