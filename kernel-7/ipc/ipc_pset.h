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
 *	File:	ipc/ipc_pset.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for port sets.
 */

#ifndef	_IPC_IPC_PSET_H_
#define _IPC_IPC_PSET_H_

#include <mach/port.h>
#include <mach/kern_return.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_mqueue.h>

typedef struct ipc_pset {
	struct ipc_object ips_object;

	mach_port_t ips_local_name;
	struct ipc_mqueue ips_messages;
} *ipc_pset_t;

#define	ips_references		ips_object.io_references

#define	IPS_NULL		((ipc_pset_t) IO_NULL)

#define	ips_active(pset)	io_active(&(pset)->ips_object)
#define	ips_lock(pset)		io_lock(&(pset)->ips_object)
#define	ips_lock_try(pset)	io_lock_try(&(pset)->ips_object)
#define	ips_unlock(pset)	io_unlock(&(pset)->ips_object)
#define	ips_check_unlock(pset)	io_check_unlock(&(pset)->ips_object)
#define	ips_reference(pset)	io_reference(&(pset)->ips_object)
#define	ips_release(pset)	io_release(&(pset)->ips_object)

/* Allocate a port set */
extern kern_return_t ipc_pset_alloc(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_pset_t	*psetp);

/* Allocate a port set, with a specific name */
extern kern_return_t ipc_pset_alloc_name(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_pset_t	*psetp);

/* Remove a port from a port set */
extern void ipc_pset_remove(
	ipc_pset_t	pset,
	ipc_port_t	port);

/* Remove a port from its current port set to the specified port set */
extern kern_return_t ipc_pset_move(
	ipc_space_t	space,
	ipc_port_t	port,
	ipc_pset_t	nset);

/* Destroy a port_set */
extern void ipc_pset_destroy(
	ipc_pset_t	pset);

#define	ipc_pset_reference(pset)	\
		ipc_object_reference(&(pset)->ips_object)

#define	ipc_pset_release(pset)		\
		ipc_object_release(&(pset)->ips_object)

#endif	/* _IPC_IPC_PSET_H_ */
