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
/*
 * HISTORY
 *  3-Aug-90  Doug Mitchell at NeXT
 *	Added prototypes for exported functions.
 *
 * 18-Feb-90  Gregg Kellogg (gk) at NeXT
 *	Merged in with Mach 2.5 stuff.
 *
 *  9-Mar-88  John Seamons (jks) at NeXT
 *	Allocate vm_info structures from a zone.
 *
 * 11-Jun-87  William Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed pager_id to pager.
 *
 * 30-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */
/*
 *	File:	mapfs.h
 *	Author:	Avadis Tevanian, Jr.
 *	Copyright (C) 1987, Avadis Tevanian, Jr.
 *
 *	Header file for mapped file system support.
 *
 */ 

#ifndef	_KERN_MAPFS_H_
#define	_KERN_MAPFS_H_

#ifdef KERNEL_BUILD
#import <mach_nbc.h>
#endif  /* KERNEL_BUILD */
#import <vm/vm_pager.h>
#import <kern/lock.h>
#import <kern/queue.h>
#import <kern/zalloc.h>
#import <vm/vm_object.h>
#import <sys/types.h>
#import <sys/vnode.h>

/*
 *	Associated with each mapped file is information about its
 *	corresponding VM window.  This information is kept in the following
 *	vm_info structure.
 */
struct vm_info {
	vm_pager_t	pager;		/* [external] pager */
	short		map_count;	/* number of times mapped */
	short		use_count;	/* number of times in use */
	vm_offset_t	va;		/* mapped virtual address */
	vm_size_t	size;		/* mapped size */
	vm_offset_t	offset;		/* offset into file at va */
	vm_size_t	vnode_size;	/* vnode size (not reflected in ip) */
	struct vnode    *vnode;         /* vnode backpointer */
	lock_data_t	lock;		/* lock for changing window */
	vm_object_t	object;		/* object [for KERNEL flushing] */
	queue_chain_t	lru_links;	/* lru queue links */
	struct ucred	*cred;		/* vnode credentials */
	int		error;		/* holds error codes */
	u_int	queued:1,	/* on lru queue? */
			dirty:1,	/* range needs flushing? */
			close_flush:1,	/* flush on close */
			invalidate:1,	/* is mapping invalid? */
			busy:1,			/* are we busy conducting an uiomove */
			delayed_fsync:1,/* need to do a push after the uiomove completes */
			filesize:1,		/* want size as reflected in ip */
			mapped:1,		/* mapped into KERNEL VM? */
			dying:1,		/* vm_info being freed */
			nfsdirty:1;		/* locally modified */
	vm_size_t	dirtysize;	/* unwritten data so far */
	vm_offset_t	dirtyoffset;	/* unwritten data so far */
};

#define VM_INFO_NULL	((struct vm_info *) 0)
#if NeXT
#define MFS_NOFLUSH 1
#define MFS_NOINVALID 2
#endif

extern struct zone	*vm_info_zone;

/*
 * exported primitives for loadable file systems.
 */
int vm_info_init __P((struct vnode *));
void vm_info_free  __P((struct vnode *));
vm_size_t vm_get_vnode_size __P((struct vnode *));
void vm_set_vnode_size __P((struct vnode *, vm_size_t));
void vm_set_close_flush __P((struct vnode *, boolean_t));
void mapfs_uncache __P((struct vnode *));
int mapfs_trunc __P((struct vnode *, vm_offset_t));
int mapfs_io __P((struct vnode *, struct uio *, enum uio_rw, int, struct ucred *));
void vm_set_error __P((struct vnode *, int));
void map_vnode __P((struct vnode *, struct proc *));
void unmap_vnode  __P((struct vnode *, struct proc *));
void blkflush  __P((struct vnode *, daddr_t, vm_size_t));

#ifdef KERNEL_BUILD
#define	mfs_assert(e) assert(e)
#endif /* KERNEL_BUILD */

/* defines for common checks that a file system has to perform */

#define ISMAPFSFILE(vp)	\
	(((vp)->v_type == VREG) &&	\
	((vp)->v_vm_info) && ((vp)->v_vm_info->mapped))

#define NOTMAPFSFILE(vp)	\
	(((vp)->v_type == VREG) &&	\
	(((vp)->v_vm_info) && !((vp)->v_vm_info->mapped)))

#endif	/* _KERN_MAPFS_H_ */

