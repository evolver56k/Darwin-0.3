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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#include	"netmsg.h"
#include	<servers/nm_defs.h>

#include "mem.h"

#ifndef NeXT_PDO
#include <mach/vm_param.h>
#endif
#include <mach/mach.h>
#include <mach/cthreads.h>

/*
 * Chain of all the different objects managed by this module.
 */
PRIVATE struct mutex		mem_lock;
EXPORT mem_objrec_ptr_t		mem_chain;


/*
 * mem_init --
 *	Initialise the memory module.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Design:
 *	Initialise the mem_chain list.
 *
 */
EXPORT boolean_t mem_init()
{
	mutex_init(&mem_lock);
	mem_chain = NULL;

	RETURN(TRUE);

}


/*
 * mem_clean --
 *	Clean up the memory allocated to reduce paging activity.
 *
 * Results:
 *	KERN_SUCCESS or KERN_FAILURE
 *
 * Side effects:
 *	May invalidate memory.
 *
 * Design:
 *
 * Note:
 *	Does not do anything yet!
 *
 */
EXPORT int mem_clean()
{

	ERROR((msg, "mem_clean called unexpectedly."));
	RETURN(KERN_SUCCESS);

}



/*
 * mem_initobj --
 *
 * Parameters:
 *	or: pointer to the object record to initialize.
 *	name: name fotr this type of object.
 *	obj_size: size in bytes of one object.
 *	aligned: TRUE if objects must be aligned on a page boundary.
 *	full_num: number of objects in a full bucket.
 *	reuse_num: minimum number of free objects before reusing a bucket.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Fill in the object record. Allocate the first bucket.
 *
 */
EXPORT void mem_initobj(or,name,obj_size,aligned,full_num,reuse_num)
	mem_objrec_ptr_t	or;
	char			*name;
	unsigned int		obj_size;
	boolean_t		aligned;
	int			full_num;
	int			reuse_num;
{
	or->name = name;
	if (aligned) {
		or->obj_size = round_page(obj_size);
	} else {
		or->obj_size = obj_size;
	}
	RET;
}


/*
 * mem_allocobj_proc --
 *
 * Parameters:
 *	or: pointer to the object record.
 *
 * Results:
 *	Pointer to a new object.
 *
 * Side effects:
 *	Allocates memory for a new bucket.
 *	May call panic() if the allocation fails.
 *
 * Note:
 *	The object record must be locked throughout this procedure.
 *
 */
PUBLIC pointer_t mem_allocobj_proc(or)
	mem_objrec_ptr_t	or;
{
	RETURN((pointer_t) malloc(or->obj_size));
}



/*
 * mem_deallocobj_proc --
 *
 * Parameters:
 *	or: pointer to the object record.
 *	ob: pointer to a bucket that may need special action.
 *
 * Side effects:
 *	May place the bucket on the allocation queue, or may
 *	deallocate it.
 *
 * Note:
 *	The object record must be locked throughout this procedure.
 *
 */
PUBLIC void mem_deallocobj_proc(or,ob)
	mem_objrec_ptr_t	or;
	void			*ob;
{
	free(ob);
	RET;
}




/*
 * mem_list --
 *
 *   List all buckets for all classes into the specified buffers.
 *
 * Parameters:
 *
 * Side effects:
 *
 * Note:
 *
 */
PUBLIC void mem_list(class_ptr,nam_ptr,bucket_ptr,class_max,nam_max,bucket_max)
	mem_objrec_ptr_t	class_ptr;
	char			*nam_ptr;
	int			*bucket_ptr;
	int			*class_max;		/* inout */
	int			*nam_max;		/* inout */
	int			*bucket_max;		/* inout */
{
	/* think about returning malloc statistics */
	RET;
}
