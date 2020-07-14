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
 *	File:	ipc/ipc_table.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for tables, used for IPC capabilities (ipc_entry_t)
 *	and dead-name requests (ipc_port_request_t).
 */

#ifndef	_IPC_IPC_TABLE_H_
#define	_IPC_IPC_TABLE_H_

#include <mach/boolean.h>
#include <mach/vm_param.h>

/*
 *	The is_table_next field of an ipc_space_t points to
 *	an ipc_table_size structure.  These structures must
 *	be elements of an array, ipc_table_entries.
 *
 *	The array must end with two elements with the same its_size value.
 *	Except for the terminating element, the its_size values must
 *	be strictly increasing.  The largest (last) its_size value
 *	must be less than or equal to MACH_PORT_INDEX(MACH_PORT_DEAD).
 *	This ensures that
 *		1) MACH_PORT_INDEX(MACH_PORT_DEAD) isn't a valid index
 *		in the table, so ipc_entry_get won't allocate it.
 *		2) MACH_PORT_MAKE(index+1, 0) and MAKE_PORT_MAKE(size, 0)
 *		won't ever overflow.
 *
 *
 *	The ipr_size field of the first element in a table of
 *	dead-name requests (ipc_port_request_t) points to the
 *	ipc_table_size structure.  The structures must be elements
 *	of ipc_table_dnrequests.  ipc_table_dnrequests must end
 *	with an element with zero its_size, and except for this last
 *	element, the its_size values must be strictly increasing.
 *
 *	The is_table_next field points to the ipc_table_size structure
 *	for the next larger size of table, not the one currently in use.
 *	The ipr_size field points to the currently used ipc_table_size.
 */

typedef unsigned int ipc_table_index_t;	/* index into tables */
typedef unsigned int ipc_table_elems_t;	/* size of tables */

typedef struct ipc_table_size {
	ipc_table_elems_t its_size;	/* number of elements in table */
} *ipc_table_size_t;

#define	ITS_NULL	((ipc_table_size_t) 0)
#define ITS_SIZE_NONE	-1

extern ipc_table_size_t ipc_table_entries;
extern ipc_table_size_t ipc_table_dnrequests;

/* Initialize IPC capabilities table storage */
extern void ipc_table_init(void);

/*
 *	Note that ipc_table_alloc, ipc_table_realloc, and ipc_table_free
 *	all potentially use the VM system.  Hence simple locks can't
 *	be held across them.
 *
 *	We can't use a copying realloc, because the realloc happens
 *	with the data unlocked.  ipc_table_realloc remaps the data,
 *	so it is OK.
 */

/* Allocate a table */
extern vm_offset_t ipc_table_alloc(
	vm_size_t	size);

/* Reallocate a big table */
extern vm_offset_t ipc_table_realloc(
	vm_size_t	old_size,
	vm_offset_t	old_table,
	vm_size_t	new_size);

/* Free a table */
extern void ipc_table_free(
	vm_size_t	size,
	vm_offset_t	table);

#define	it_entries_alloc(its)						\
	((ipc_entry_t)							\
	 ipc_table_alloc((its)->its_size * sizeof(struct ipc_entry)))

#define it_entries_reallocable(its)					\
	(((its)->its_size * sizeof(struct ipc_entry)) >= PAGE_SIZE)

#define	it_entries_realloc(its, table, nits)				\
	((ipc_entry_t)							\
	 ipc_table_realloc((its)->its_size * sizeof(struct ipc_entry),	\
			   (vm_offset_t)(table),			\
			   (nits)->its_size * sizeof(struct ipc_entry)))

#define	it_entries_free(its, table)					\
	ipc_table_free((its)->its_size * sizeof(struct ipc_entry),	\
		       (vm_offset_t)(table))

#define	it_dnrequests_alloc(its)					\
	((ipc_port_request_t)						\
	 ipc_table_alloc((its)->its_size *				\
			 sizeof(struct ipc_port_request)))

#define	it_dnrequests_free(its, table)					\
	ipc_table_free((its)->its_size *				\
		       sizeof(struct ipc_port_request),			\
		       (vm_offset_t)(table))

#endif	/* _IPC_IPC_TABLE_H_ */
