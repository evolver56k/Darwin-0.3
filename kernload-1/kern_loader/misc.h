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
 * 27-Oct-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#import <mach/mach.h>
#import <mach-o/loader.h>

/*
 * Return a pointer to the data associated with the given segment/section
 * names.  This works for MH_EXECUTABLE and MH_OBJECT file types.
 */
const void *getSectDataFromHeader (
	const server_t	*server,
	const char	*segname,
	const char	*sectname,
	int		*size);
const char *getMachoString (
	const server_t	*server,
	const char	*segname,
	const char	*sectname);

const char *getMachoData (
	const server_t	*server,
	const char	*segname,
	const char	*sectname);

/*
 * Given a name look up the task_port associated with it (if any).
 */
kern_return_t task_by_name(const char *name, task_t *result);

/*
 * Save this task/name combination.
 */
boolean_t save_task_name(const char *name, task_t task);

/*
 * If the task_port goes away, forget this association.
 */
boolean_t delete_task_name(task_t task);

/*
 * Given a filename and symbolname lookup the value of the symbol.
 */
kern_return_t sym_value (
	const char	*filename,
	const char	*symname,
	vm_address_t	*result);

/*
 * Save this task/name combination.
 */
boolean_t save_symvalue (
	const char	*filename,
	const char	*symname,
	vm_address_t	symvalue);

/*
 * Obsolete symbols in the given file.
 */
boolean_t delete_symfile(const char *filename);

void hash_init(void);

/*
 * Error stuff.
 */
const char *kern_serv_error_string(kern_return_t r);

void kern_serv_error(const char *s, kern_return_t r);
