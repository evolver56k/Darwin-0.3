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

#ifndef	_MEM_
#define	_MEM_

#include <stdlib.h>
#include <mach/mach.h>
#include <mach/cthreads.h>
#include "debug.h"
#include "ls_defs.h"

#define	MEM_DEBUG_LO	debug.mem
#define	MEM_DEBUG_HI	(debug.mem & 0x2)


/*
 * Record for object allocation chain.
 */
typedef struct mem_objrec {
	char		*name;		/* name of zone */
	unsigned int	obj_size;	/* size of one object */
} mem_objrec_t, *mem_objrec_ptr_t;

/*
 * Macros for object allocation.
 */

extern mem_objrec_t	MEM_NNREC;

#define MEM_ALLOCOBJ(ret,type,ot) { ret = (type) calloc(1, ot.obj_size); }

#define	MEM_DEALLOCOBJ(ptr,ot) free((void *)(ptr))

#define	MEM_ALLOC(ptr,type,_size,aligned) {					\
	if ((int)_size <= 0) {							\
		ERROR((msg,"MEM_ALLOC: illegal size: %d", (int)_size));		\
		panic("MEM_ALLOC with negative size");				\
	}									\
										\
	ptr = (type) calloc(1, _size);					\
	if (ptr == (type) NULL) {						\
		ERROR((msg,"MEM_ALLOC.calloc returned NULL, size=%d",	\
			(int)_size));						\
		panic("MEM_ALLOC.calloc");				\
	}								\
	INCSTAT(mem_allocs);							\
}


#define	MEM_DEALLOC(ptr,_size) {							\
	if (_size < 0) {							\
		ERROR((msg,"MEM_ALLOC: illegal size: %d, ptr=0x%x",	\
							(int)_size, (int)ptr));	\
		panic("MEM_DEALLOC with negative size");		\
	}								\
	if (_size > 0) {							\
		free((char *)ptr);					\
	}								\
	INCSTAT(mem_deallocs);							\
}


/*
 * To use the object allocator for objects of type foo_t:
 *
 * Declarations:
 *
 *	extern mem_objrec_t	MEM_FOO;
 *
 * Initialization:
 *
 *	PUBLIC mem_objrec_t	MEM_FOO;
 *	mem_initobj(&MEM_FOO,"foo",sizeof(foo_t),
 *			aligned,full_num,reuse_num);
 *
 */


/*
 * External procedures.
 */
extern boolean_t mem_init();
/*
*/

extern int mem_clean();
/*
*/

extern void mem_initobj();
/*
mem_objrec_ptr_t	or;
char			*name;
unsigned int		obj_size;
boolean_t		aligned;
int			full_num;
int			reuse_num;
*/

extern pointer_t mem_allocobj_proc();
/*
mem_objrec_ptr_t	or;
*/

extern void mem_deallocobj_proc();
/*
mem_objrec_ptr_t	or;
mem_objbucket_ptr_t	ob;
*/

#endif	_MEM_
