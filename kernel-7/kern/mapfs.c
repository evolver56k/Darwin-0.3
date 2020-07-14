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
 *	File:	mapfs.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1987, Avadis Tevanian, Jr.
 *
 *	Support for mapped file system implementation.
 *
 * HISTORY
 * 2-Jun-1998	Umesh Vaishampayan
 *	Changed error handling to check for all errors.
 *
 * 6-Dec-1997 A.Ramesh at Apple
 * 	Made the chages for Rhapsody; Reanamed mfs to mapfs to avoid confusion
 *	with memory based filesystem.
 *
 * 18-Nov-92 Phillip Dibner at NeXT
 *	Made the i/o throttle global.   This is a hack on top of a hack and 
 *	should be fixed properly, probably in the vm system.
 *
 *  3-Sep-92 Joe Murdock at NeXT
 *	Added an i/o throttle to mfs_io as a cheap work-around for a i/o buffer
 *	resource conflict with usr-space system bottle-necks (nfs servers, etc)
 *
 *  7-Feb-92 Jim Hays
 *	There are still bugs in this code dealing with vmp_pushing wired 
 *	pages. We need to modify the sound drivers locks to be breakable
 *	except during the actual playing. 
 *
 *  3-Aug-90  Doug Mitchell at NeXT
 *	Added primitives for loadable file system support.
 *
 *  7-Mar-90  Brian Pinkerton (bpinker) at NeXT
 *	Changed mfs_trunc to return an indication of change.
 *
 *  9-Mar-88  John Seamons (jks) at NeXT
 *	SUN_VFS: allocate vm_info structures from a zone.
 *
 * 29-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Corrected calls to inode_pager_setup and kmem_alloc.
 *
 * 15-Sep-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 18-Jun-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Make most of this file dependent on MACH_NBC.
 *
 * 30-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#import <mach_nbc.h>

#import <kern/lock.h>
#import <kern/mapfs.h>
#import <kern/sched_prim.h>
#import <kern/assert.h>

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/mount.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <sys/vnode.h>
#import <sys/uio.h>
/* Needed for VOP_DEBLOCKSIZE, ip usage */
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#import <sys/dir.h>

#import <vm/vm_kern.h>
#import <vm/vm_pager.h>
#import <mach/vm_param.h>
#import <mach/machine.h>
#import <vm/vnode_pager.h>
#import <vm/pmap.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>

#define PERFMODS 1

struct zone	*vm_info_zone;

/*
 *	Private variables and macros.
 */

queue_head_t		vm_info_queue;		/* lru list of structures */
decl_simple_lock_data(,	vm_info_lock_data)	/* lock for lru list */
int			vm_info_version = 0;	/* version number */




#define	vm_info_lock()		simple_lock(&vm_info_lock_data)
#define	vm_info_unlock()	simple_unlock(&vm_info_lock_data)

#if	MACH_NBC
lock_data_t		mfsbuf_lock;		/* lock for active_mfsbufs */
lock_data_t		mfs_alloc_lock_data;
boolean_t		mfs_alloc_wanted;
long			mfs_alloc_blocks = 0;

#define mfs_alloc_lock()	lock_write(&mfs_alloc_lock_data)
#define mfs_alloc_unlock()	lock_write_done(&mfs_alloc_lock_data)

vm_map_t	mfs_map;

/*
 *	mfs_map_size is the number of bytes of VM to use for file mapping.
 *	It should be set by machine dependent code (before the call to
 *	mapfs_init) if the default is inappropriate.
 *
 *	mfs_max_window is the largest window size that will be given to
 *	a file mapping.  A default value is computed in mapfs_init based on
 *	mfs_map_size.  This too may be set by machine dependent code
 *	if the default is not appropriate.
 *
 *	mfs_files_max is the maximum number of files that we will
 *	simultaneously leave mapped.  Note th memory for unmapped
 *	files will not necessarily leave the memory cache, but by
 *	unmapping these files the file system can throw away any
 *	file system related info (like vnodes).  Again, this value
 *	can be sent by machine dependent code if the default is not
 *	appropriate.
 */

#ifdef ppc
vm_size_t	mfs_map_size = 64*1024*1024;	/* size in bytes */
#else
vm_size_t	mfs_map_size = 8*1024*1024;	/* size in bytes */
#endif
vm_size_t	mfs_max_window = 0;		/* largest window to use */

#ifdef ppc
int		mfs_files_max = 400;		/* maximum # of files mapped */
#else
int		mfs_files_max = 100;		/* maximum # of files mapped */
#endif
int		mfs_files_mapped = 0;		/* current # mapped */

#define CHUNK_SIZE	(128 * 1024)
#endif	/* MACH_NBC */

#ifdef ppc
#define MFS_MAP_SIZE_MAX	(64 * 1024 * 1024)
#else
#define MFS_MAP_SIZE_MAX	(16 * 1024 * 1024)
#endif

/* The MFS_MAP_SIZE_PER_UNIT is used in remap; as well as in init */
#define	MFS_MAP_SIZE_PER_UNIT	(1024 * 1024)
#define	MFS_MEMORY_UNIT		(1024 * 1024)
#define MFS_FILES_PER_UNIT	12

void vm_info_enqueue __P((struct vm_info *));
void vm_info_dequeue __P((struct vm_info *));
void mapfs_put __P((struct vnode *));
int mapfs_get __P((struct vnode *,vm_offset_t, vm_size_t));
int remap_vnode __P((struct vnode *,vm_offset_t, vm_size_t));
void vmp_put __P((struct vm_info *));
void vmp_get __P((struct vm_info *));
void mapfs_cache_trim __P((void));
void mapfs_memfree __P((struct vm_info *, boolean_t));
int mapfs_map_remove __P((struct vm_info *, vm_offset_t, vm_size_t, boolean_t));
void vno_flush __P((struct vnode *, vm_offset_t, vm_size_t));
void vmp_invalidate __P((struct vm_info *));
int vmp_push __P((struct vm_info *));
int vmp_push_range __P((struct vm_info *,vm_offset_t, vm_size_t));
void vmp_push_all __P((struct vm_info *));
/* Missing from headers so provided the prototypes */
void vm_object_deactivate_pages __P((vm_object_t));
void vm_object_deactivate_pages_first __P((vm_object_t));
void vm_page_deactivate __P((vm_page_t));
void vm_page_activate __P((vm_page_t));
kern_return_t vm_allocate_with_pager __P((vm_map_t, vm_offset_t *, vm_size_t, boolean_t, vm_pager_t,vm_offset_t));

#if PERFMODS
int mapfs_map_cleanup __P((struct vm_info *,vm_offset_t,vm_size_t,boolean_t));
#endif

/*
 *	mapfs_init:
 *
 *	Initialize the mapped FS module.
 */
int
mapfs_init()
{
	int			i;
#if	MACH_NBC
	int			min, max;
#endif	/* MACH_NBC */

	queue_init(&vm_info_queue);
	simple_lock_init(&vm_info_lock_data);
#if	MACH_NBC
	lock_init(&mfs_alloc_lock_data, TRUE);
	mfs_alloc_wanted = FALSE;
	mfs_map = kmem_suballoc(kernel_map, &min, &max, mfs_map_size, TRUE);

	mfs_map_size = (int) ((long long) MFS_MAP_SIZE_PER_UNIT /
				(long long) MFS_MEMORY_UNIT *
				(long long) machine_info.memory_size);

	if (mfs_map_size > MFS_MAP_SIZE_MAX)
		mfs_map_size = MFS_MAP_SIZE_MAX;

#if	notdef
	mfs_files_max = (int)((long long) MFS_FILES_PER_UNIT *
				(long long) machine_info.memory_size /
				(long long) MFS_MEMORY_UNIT);
#endif /* notdef */
	 
	/* Get atleast a Meg and instead of 5% choose 6.25% */
	if (mfs_max_window == 0)
		mfs_max_window = mfs_map_size / 16;
	if (mfs_max_window < MFS_MEMORY_UNIT)
		mfs_max_window = MFS_MEMORY_UNIT;
#endif	/* MACH_NBC */
	i = (vm_size_t) sizeof (struct vm_info);
	vm_info_zone = zinit (i, 10000*i, 8192, FALSE, "vm_info zone");

	return(0);
}

/*
 *	vm_info_init:
 *
 *	Initialize a vm_info structure for a vnode.
 */
int
vm_info_init(vp)
	struct vnode *vp;
{
	register struct vm_info	*vmp;

	vmp = vp->v_vm_info;
	if (vmp == VM_INFO_NULL)
		vmp = (struct vm_info *) zalloc(vm_info_zone);
	vmp->pager = vm_pager_null;
	vmp->map_count = 0;
	vmp->use_count = 0;
	vmp->va = 0;
	vmp->size = 0;
	vmp->offset = 0;
#if PERFMODS
	vmp->dirtysize   = 0;
	vmp->dirtyoffset = 0;
#endif
	vmp->cred = (struct ucred *) NULL;
	vmp->error = 0;

	vmp->queued = FALSE;
	vmp->dirty = FALSE;
	vmp->nfsdirty = FALSE;
	vmp->close_flush = TRUE;	/* for safety, reconsider later */
	vmp->invalidate = FALSE;
	vmp->busy = FALSE;
	vmp->delayed_fsync = FALSE;
	vmp->filesize = FALSE;
	vmp->mapped = FALSE;
	vmp->dying = FALSE;

	vmp->vnode_size = 0;
	vmp->vnode = vp;
	lock_init(&vmp->lock, TRUE);	/* sleep lock */
	vmp->object = VM_OBJECT_NULL;
	vp->v_vm_info = vmp;
	return(0);
}

/*
 * Loadable file system support to avoid exporting struct vm_info.
 */
void vm_info_free(struct vnode *vp)
{
	register struct vm_info *vmp = vp->v_vm_info;

	if (vmp == VM_INFO_NULL)
		return;

	/*
	 * If vmp->dying is set then we have reentered.
	 * Uninterruptible wait for the other thead to finish and return.
	 */
	if (vmp->dying == TRUE) {
		(void)tsleep(vmp, 0, "vminfofree", 0);
		return;
	}

	/* Prevent other threads from racing in */
	vmp->dying = TRUE;

#if MACH_NBC
	mapfs_uncache(vp);		/* could block here */
#endif 
	vp->v_vm_info = VM_INFO_NULL;
	wakeup(vmp);			/* wakeup other threads blocked on vmp */

	zfree(vm_info_zone, (vm_offset_t)vmp);	/* could block here */
}

#if	MACH_NBC  /* [ */
void
vm_info_enqueue(vmp)
	struct vm_info	*vmp;
{
	mfs_assert(!vmp->queued);
	mfs_assert(vmp->mapped);
#if 0
	mfs_assert(vmp->size);
	if ((vmp->size == 0) || !vmp->mapped)
		panic("VMP SIZE IS ZERO\n");

#endif	
	queue_enter(&vm_info_queue, vmp, struct vm_info *, lru_links);
	vmp->queued = TRUE;
	mfs_files_mapped++;
	vm_info_version++;
}

void
vm_info_dequeue(vmp)
	struct vm_info	*vmp;
{
	mfs_assert(vmp->queued);
	queue_remove(&vm_info_queue, vmp, struct vm_info *, lru_links);
	vmp->queued = FALSE;
	mfs_files_mapped--;
	vm_info_version++;
}

/*
 *	map_vnode:
 *
 *	Indicate that the specified vnode should be mapped into VM.
 *	A reference count is maintained for each mapped file.
 */
void
map_vnode(vp,p)
	register struct vnode	*vp;
	register struct proc *p;
{
	register struct vm_info	*vmp;
	vm_pager_t	pager;
	extern lock_data_t	vm_alloc_lock;
	struct vattr vattr;
#if 1
	/* Needed as in some cases the exec, namei returned vp
	 * with no vm_info attached -- XXX (Verify this )
	*/
	if (vp->v_vm_info == (struct vm_info *)0)
		vm_info_init(vp);
#endif
	vmp = vp->v_vm_info;

	if (vmp->map_count++ > 0)
		return;		/* file already mapped */

	if (vmp->mapped)
		return;		/* file was still cached */

	vmp_get(vmp);

	pager = vmp->pager = (vm_pager_t) vnode_pager_setup(vp, FALSE, TRUE);
				/* not a TEXT file, can cache */
	/*
	 *	Lookup what object is actually holding this file's
	 *	pages so we can flush them when necessary.  This
	 *	would be done differently in an out-of-kernel implementation.
	 *
	 *	Note that the lookup keeps a reference to the object which
	 *	we must release elsewhere.
	 */
	lock_write(&vm_alloc_lock);
	vmp->object = vm_object_lookup(pager);
	vm_stat.lookups++;
	if (vmp->object == VM_OBJECT_NULL) {
		vmp->object = vm_object_allocate(0);
		vm_object_enter(vmp->object, pager);
		vm_object_setpager(vmp->object, pager, (vm_offset_t) 0, FALSE);
	}
	else {
		vm_stat.hits++;
	}
	lock_write_done(&vm_alloc_lock);

	vmp->error = 0;

	VOP_GETATTR(vp, &vattr, p->p_ucred ,p);

	vmp->vnode_size = vattr.va_size;	/* must be before setting
						   mapped below to prevent
						   mapfs_fsync from recursive
						   locking */

	vmp->va = 0;
	vmp->size = 0;
	vmp->offset = 0;
	vmp->mapped = TRUE;

	vmp_put(vmp);	/* put will queue on LRU list */
}

int close_flush = 1;

/*
 *	unmap_vnode:
 *
 *	Called when an vnode is closed.
 */
void
unmap_vnode(vp, p)
	register struct vnode	*vp;
	register struct proc	*p;
{
	register struct vm_info		*vmp;
	register struct vm_object	*object;
	int				links;
	register struct pcred *pcred = p->p_cred;
	register struct ucred *cred = pcred->pc_ucred;
	struct vattr	vattr;

	vmp = vp->v_vm_info;
	if (!vmp->mapped)
		return;	/* not a mapped file */
	/* 
	 * If the file, which was prev mapped and closed is opened with 
	 * O_NO_MFS, the map_count will be zero when close
	 * is called. SO, if it is already zero, there is nothing to
	 * be done here. (Otherwise 2269452 and 2269437)
	 */
	if (vmp->map_count == 0)
		return;
	if (--vmp->map_count > 0) {
		return;
	}

	/*
	 *	If there are no links left to the file then release
	 *	the resources held.  If there are links left, then keep
	 *	the file mapped under the assumption that someone else
	 *	will soon map the same file.  However, the pages in
	 *	the object are deactivated to put them near the list
	 *	of pages to be reused by the VM system (this would
	 *	be done differently out of the kernel, of course, then
	 *	again, the primitives for this don't exist out of the
	 *	kernel yet.
	 */

	vmp->map_count++;
		
	VOP_GETATTR(vp, &vattr, cred, p);
	links = vattr.va_nlink;  /* may uncache, see below */
	vmp->map_count--;

	if (links == 0) {
		mapfs_memfree(vmp, FALSE);
	} else {
		/*
		 *	pushing the pages may cause an uncache
		 *	operation (thanks NFS), so gain an extra
		 *	reference to guarantee that the object
		 *	does not go away.  (Note that such an
		 *	uncache actually takes place since we have
		 *	already released the map_count above).
		 */
		object = vmp->object;
		if (close_flush || vmp->close_flush) {
			vmp->map_count++;	/* prevent uncache race */
			vmp_get(vmp);
#if PERFMODS
			if (vmp->dirty)
				(void)vmp_push_range(vmp, vmp->dirtyoffset, vmp->dirtysize);
#else
			(void)vmp_push(vmp); /* Ignore errors! XXX */
#endif
		}
		vm_object_lock(object);
		vm_object_deactivate_pages(object);
		vm_object_unlock(object);
		if (close_flush || vmp->close_flush) {
			vmp_put(vmp);
			vmp->map_count--;
		}
	}
}

/*
 *	remap_vnode:
 *
 *	Remap the specified vnode (due to extension of the file perhaps).
 *	Upon return, it should be possible to access data in the file
 *	starting at the "start" address for "size" bytes.
 */
int
remap_vnode(vp, start, size)
	register struct vnode	*vp;
	vm_offset_t		start;
	register vm_size_t	size;
{
	register struct vm_info	*vmp;
	vm_offset_t		addr, offset;
	kern_return_t		ret;
	int error=0;
	vmp = vp->v_vm_info;
	/*
	 *	Remove old mapping (making its space available).
	 */

	if (vmp->size > 0) {
#if PERFMODS
		if (vmp->dirty)
			(void)vmp_push_range(vmp, vmp->dirtyoffset, vmp->dirtysize);
		error = mapfs_map_remove(vmp, vmp->va, vmp->va + vmp->size, FALSE);
#else
		error = mapfs_map_remove(vmp, vmp->va, vmp->va + vmp->size, TRUE);
#endif /* PERFMODS */
		if (error)
			goto out;
	}

	offset = trunc_page(start);
	size = round_page(start + size) - offset;

	if (size < CHUNK_SIZE)
		size = CHUNK_SIZE;
	do {
		addr = vm_map_min(mfs_map);
		mfs_alloc_lock();
		ret = vm_allocate_with_pager(mfs_map, &addr, size, TRUE,
				vmp->pager, offset);
		/*
		 *	If there was no space, see if we can free up mappings
		 *	on the LRU list.  If not, just wait for someone else
		 *	to free their memory.
		 */
		if (ret == KERN_NO_SPACE) {
			register struct vm_info	*vmp1;

			vm_info_lock();
			vmp1 = VM_INFO_NULL;
			if (!queue_empty(&vm_info_queue)) {
				vmp1 = (struct vm_info *)
						queue_first(&vm_info_queue);
				vm_info_dequeue(vmp1);
			}
			vm_info_unlock();
			/*
			 *	If we found someone, free up its memory.
			 */
			if (vmp1 != VM_INFO_NULL) {
				mfs_alloc_unlock();
				mapfs_memfree(vmp1, TRUE);
				mfs_alloc_lock();
			}
			else {
				mfs_alloc_wanted = TRUE;
				assert_wait(&mfs_map, FALSE);
				mfs_alloc_blocks++;	/* statistic only */
				mfs_alloc_unlock();
				thread_block();
				mfs_alloc_lock();
			}
		}
		else if (ret != KERN_SUCCESS) {
			printf("Unexpected error on file map, ret = %d.\n",
					ret);
			panic("remap_vnode");
		}
		mfs_alloc_unlock();
	} while (ret != KERN_SUCCESS);
	/*
	 *	Fill in variables corresponding to new mapping.
	 */
	vmp->va = addr;
	vmp->size = size;
	vmp->offset = offset;
out:
	return(error);
}

/*
 *	mapfs_trunc:
 *
 *	The specified vnode is truncated to the specified size.
 *	Returns 0 if successful error otherwise.
 */
int
mapfs_trunc(vp, length)
	register struct vnode	*vp;
	register vm_offset_t	length;
{
	register struct vm_info	*vmp;
	register vm_size_t	size, rsize;
	int error = 0;

	vmp = vp->v_vm_info;

	if ((vp->v_type != VREG) || (vmp == (struct vm_info *)0))
		return (0);
	if (!vmp->mapped) {	/* file not mapped, just update size */
		vmp->vnode_size = length;
		return (0);
	}
	vmp_get(vmp);

	vmp->nfsdirty = TRUE;
	/*
	 *	Unmap everything past the new end page.
	 *	Also flush any pages that may be left in the object using
	 *	vno_flush (is this necessary?).
	 *	rsize is the size relative to the mapped offset.
	 */
	NFSTRACE4(NFSTRC_MTR, vp, length, vmp->size, vmp->offset);
	size = round_page(length);
	if (size >= vmp->offset) {
		rsize = size - vmp->offset;
	} else {
		rsize = 0;
	}
	if (rsize < vmp->size) {
		error = mapfs_map_remove(vmp, vmp->va + rsize,
					 vmp->va + vmp->size, FALSE);
		NFSTRACE4(NFSTRC_MTR_MREM, vp, vmp->va, vmp->size, rsize);
		if (error) {
#if DIAGNOSTIC
			kprintf("mapfs_trunc: mapfs_map_remove %d\n", error);
#endif /* DIAGNOSTIC */
			goto out;
		}
		if ((vmp->size = rsize) == 0)		/* mapped size */
			vmp->offset = 0;
	}
	if (vmp->vnode_size > size)
		vno_flush(vp, size, vmp->vnode_size - size);
	vmp->vnode_size = length;	/* file size */
	/*
	 *	If the new length isn't page aligned, zero the extra
	 *	bytes in the last page.
	 */
	if (length != size) {
		vm_size_t	n;

		n = size - length;
		/*
		 * Make sure the bytes to be zeroed are mapped.
		 */
		if ((length < vmp->offset) ||
		   ((length + n - 1) >= (vmp->offset + vmp->size))) {
			NFSTRACE4(NFSTRC_MTR_RMAP, vp, vmp->offset, vmp->size, n);
			error = remap_vnode(vp, length, n);
			if (error) {
#if DIAGNOSTIC
				kprintf("mapfs_trunc: remap_vnode %d\n", error);
#endif /* DIAGNOSTIC */
				goto out;
			}
		}
		NFSTRACE(NFSTRC_MTR_DIRT, vmp->va);
		vmp->nfsdirty = TRUE;
		error = safe_bzero((void *)(vmp->va + length - vmp->offset), n);
		if (error) {
			NFSTRACE4(NFSTRC_MTR_BZER, vp, vmp->va, vmp->offset, n);
#if DIAGNOSTIC
			kprintf("mapfs_trunc: safe_bzero %d\n", error);
			kprintf("mapfs_trunc: va %x vp %x n %x length %x offset %x size %x\n", vmp->va, (unsigned)vp, n, length, vmp->offset, vmp->size);
#endif /* DIAGNOSTIC */
			goto out;
		}

		/*
		 *	Do NOT set dirty flag... the cached memory copy
		 *	is zeroed, but this change doesn't need to be
		 *	flushed to disk (the vnode already has the right
		 *	size.  Besides, if we set this bit, we would need
		 *	to clean it immediately to prevent a later sync
		 *	operation from incorrectly cleaning a cached-only
		 *	copy of this vmp (which causes problems with NFS
		 *	due to the fact that we have changed the mod time
		 *	by truncating and will need to do an mapfs_uncache).
		 *	NFS is a pain.  Note that this means that there
		 *	will be a dirty page left in the vmp.  If this
		 *	turns out to be a problem we'll have to set the dirty
		 *	flag and immediately do a flush.
		 *
		 *	UPDATE: 4/4/13.  We need to really flush this.
		 *	Use the map_count hack to prevent a race with
		 *	uncaching.
		 */
		vmp->dirty = TRUE;
	}

	vmp->map_count++;	/* prevent uncache race */
	error = vmp_push(vmp);
#if DIAGNOSTIC
	if (error)
		kprintf("mapfs_trunc: vmp_push %d\n", error);
#endif /* DIAGNOSTIC */
	vmp->map_count--;

out:
	vmp_put(vmp);
	return (error);
}

/*
 *	mapfs_get:
 *
 *	Get locked access to the specified file.  The start and size describe
 *	the address range that will be accessed in the near future and
 *	serves as a hint of where to map the file if it is not already
 *	mapped.  Upon return, it is guaranteed that there is enough VM
 *	available for remapping operations within that range (each window
 *	no larger than the chunk size).
 */
int
mapfs_get(vp, start, size)
	register struct vnode	*vp;
	vm_offset_t		start;
	register vm_size_t	size;
{
	register struct vm_info	*vmp;
	int error=0;
	vmp = vp->v_vm_info;

	vmp_get(vmp);

	/*
	 *	If the requested size is larger than the size we have
	 *	mapped, be sure we can get enough VM now.  This size
	 *	is bounded by the maximum window size.
	 */

	if (size > mfs_max_window)
		size = mfs_max_window;

	if (size > vmp->size) {
		error = remap_vnode(vp, start, size);
	}
	return(error);
}

/*
 *	mapfs_put:
 *
 *	Indicate that locked access is no longer desired of a file.
 */
void
mapfs_put(vp)
	register struct vnode	*vp;
{
	vmp_put(vp->v_vm_info);
}

/*
 *	vmp_get:
 *
 *	Get exclusive access to the specified vm_info structure.
 *	NeXT: Note mapfs_fsync_invalidate inlines part of this.
 */
void
vmp_get(vmp)
	struct vm_info	*vmp;
{
	/*
	 *	Remove from LRU list (if its there).
	 */
	vm_info_lock();
	if (vmp->queued) {
		vm_info_dequeue(vmp);
	}
	vmp->use_count++;	/* to protect requeueing in vmp_put */
	vm_info_unlock();

	/*
	 *	Lock out others using this file.
	 */
	lock_write(&vmp->lock);
	lock_set_recursive(&vmp->lock);
}

/*
 *	vmp_put:
 *
 *	Release exclusive access gained in vmp_get.
 */
void
vmp_put(vmp)
	register struct vm_info	*vmp;
{
	/*
	 *	Place back on LRU list if noone else using it.
	 */
	vm_info_lock();
	if (--vmp->use_count == 0) {
		vm_info_enqueue(vmp);
	}
	vm_info_unlock();
	/*
	 *	Let others at file.
	 */
	lock_clear_recursive(&vmp->lock);
	lock_write_done(&vmp->lock);

	if (mfs_files_mapped > mfs_files_max)
		mapfs_cache_trim();

	if (vmp->invalidate) {
		vmp->invalidate = FALSE;
		vmp_invalidate(vmp);
	}
}

/*
 *	mapfs_uncache:
 *
 *	Make sure there are no cached mappings for the specified vnode.
 */
void 
mapfs_uncache(vp)
	register struct vnode	*vp;
{
	register struct vm_info	*vmp;

	vmp = vp->v_vm_info;
	/*
	 *	If the file is mapped but there is none actively using
	 *	it then remove its mappings.
	 */
	if (vmp->mapped && vmp->map_count == 0) {
		mapfs_memfree(vmp, FALSE);
	}
}

void
mapfs_memfree(vmp, flush)
	register struct vm_info	*vmp;
	boolean_t		flush;
{
	struct ucred	*cred;
	vm_object_t	object;
	int error = 0;

	vm_info_lock();
	if (vmp->queued) {
		vm_info_dequeue(vmp);
	}
	vm_info_unlock();

	lock_write(&vmp->lock);
	lock_set_recursive(&vmp->lock);

	if (vmp->map_count == 0) {	/* cached only */
		vmp->mapped = FALSE;	/* prevent recursive flushes */
	}

	error =  mapfs_map_remove(vmp, vmp->va, vmp->va + vmp->size, flush);
	if (error)
		panic("mapfs_memfree: mapfs_map_remove failed %d", error); /* XXX */
	vmp->size = 0;
	vmp->va = 0;
	object = VM_OBJECT_NULL;
	if (vmp->map_count == 0) {	/* cached only */
		/*
		 * lookup (in map_vnode) gained a reference, so need to
		 * lose it.
		 */
		object = vmp->object;
		vmp->object = VM_OBJECT_NULL;
		cred = vmp->cred;
		if (cred != NOCRED) {
			vmp->cred = NOCRED;
			crfree(cred);
		}
	}
	lock_clear_recursive(&vmp->lock);
	lock_write_done(&vmp->lock);

	if (object != VM_OBJECT_NULL)
		vm_object_deallocate(object);
}

/*
 *	mapfs_cache_trim:
 *
 *	trim the number of files in the cache to be less than the max
 *	we want.
 */
void
mapfs_cache_trim()
{
	register struct vm_info	*vmp;

	while (TRUE) {
		vm_info_lock();
		if (mfs_files_mapped <= mfs_files_max) {
			vm_info_unlock();
			return;
		}
		/*
		 * grab file at head of lru list.
		 */
		vmp = (struct vm_info *) queue_first(&vm_info_queue);
		vm_info_dequeue(vmp);
		vm_info_unlock();
		/*
		 *	Free up its memory.
		 */
		mapfs_memfree(vmp, TRUE);
	}
}

/*
 *	mapfs_cache_clear:
 *
 *	Clear the mapped file cache.  Note that the map_count is implicitly
 *	locked by the Unix file system code that calls this routine.
 */
int
mapfs_cache_clear()
{
	register struct vm_info	*vmp;
	int			last_version;

	vm_info_lock();
	last_version = vm_info_version;
	vmp = (struct vm_info *) queue_first(&vm_info_queue);
	while (!queue_end(&vm_info_queue, (queue_entry_t) vmp)) {
		if (vmp->map_count == 0) {
			vm_info_unlock();
			mapfs_memfree(vmp, TRUE);
			vm_info_lock();
			/*
			 * mapfs_memfree increments version number, causing
			 * restart below.
			 */
		}
		/*
		 *	If the version didn't change, just keep scanning
		 *	down the queue.  If the version did change, we
		 *	need to restart from the beginning.
		 */
		if (last_version == vm_info_version) {
			vmp = (struct vm_info *) queue_next(&vmp->lru_links);
		}
		else {
			vmp = (struct vm_info *) queue_first(&vm_info_queue);
			last_version = vm_info_version;
		}
	}
	vm_info_unlock();
	return(0);
}

/*
 *	mapfs_map_remove:
 *
 *	Remove specified address range from the mfs map and wake up anyone
 *	waiting for map space.  Be sure pages are flushed back to vnode.
 */
int
mapfs_map_remove(vmp, start, end, flush)
	struct vm_info	*vmp;
	vm_offset_t	start;
	vm_size_t	end;
	boolean_t	flush;
{
	vm_object_t	object;
	int		error = 0;
	/*
	 *	Note:	If we do need to flush, the vmp is already
	 *	locked at this point.
	 */
	if (flush) {
/*		vmp->map_count++;	*//* prevent recursive flushes */
		error = vmp_push(vmp);
/*		vmp->map_count--;*/
		if (error)
			goto out;
	}
		
	/*
	 *	Free the address space.
	 */
	mfs_alloc_lock();
	vm_map_remove(mfs_map, start, end);
	if (mfs_alloc_wanted) {
		mfs_alloc_wanted = FALSE;
		thread_wakeup(&mfs_map);
	}
	mfs_alloc_unlock();
	/*
	 *	Deactivate the pages.
	 */
	object = vmp->object;
	if (object != VM_OBJECT_NULL) {
		vm_object_lock(object);
		vm_object_deactivate_pages_first(object);
		vm_object_unlock(object);
	}

out:
	return(error);
}

#if PERFMODS
/*
 *	mapfs_map_cleanup:
 *
 *	Remove specified address range from the mfs map and wake up anyone
 *	waiting for map space.  Be sure pages are flushed back to vnode.
 */
int
mapfs_map_cleanup(vmp, start, end, flush)
	struct vm_info	*vmp;
	vm_offset_t	start;
	vm_size_t	end;
	boolean_t	flush;
{
	/*
	 *	Free the address space.
	 */
	mfs_alloc_lock();
	vm_map_remove(mfs_map, start, end);
	if (mfs_alloc_wanted) {
		mfs_alloc_wanted = FALSE;
		thread_wakeup(&mfs_map);
	}
	mfs_alloc_unlock();
	return(0);
}
#endif

#ifdef notdef 
vnode_size(vp)
	struct vnode	*vp;
{
	struct vattr		vattr;

	VOP_GETATTR(vp, &vattr, u.u_cred,p);
	return(vattr.va_size);
}
#endif /* notdef */


int active_mfsbufs = 0;		/* global record of buf count in use by mfs */
extern int nbuf;
extern int nmfsbuf;		/* global limit to mfs buffer allocation */

int
mapfs_io(vp, uio, rw, ioflag, cred)
	register struct vnode	*vp;
	register struct uio	*uio;
	enum uio_rw		rw;
	int			ioflag;
	struct ucred		*cred;
{
	register vm_offset_t	va;
	register struct vm_info	*vmp;
	register int		n, diff, bsize;
	int			error=0;
#if PERFMODS
	vm_offset_t		newoffset;
	vm_size_t		newsize;
	vm_size_t		mapfsio_size;
#endif	
	struct ucred		*cr;
	struct proc *p;


	if (uio->uio_resid == 0) {
		return (0);
	}

	if ((int) uio->uio_offset < 0 ||
	    (int) ((int)uio->uio_offset + uio->uio_resid) < 0) {
		return (EINVAL);
	}
	
	mfs_assert(vp->v_type==VREG || vp->v_type==VLNK);

	p = uio->uio_procp;
	if (p && (vp->v_type == VREG) &&
	    uio->uio_offset + uio->uio_resid >
	    p->p_rlimit[RLIMIT_FSIZE].rlim_cur) {
	  psignal(p, SIGXFSZ);
	  return (EFBIG);
	}

	/*
	 * The following code is adapted from code in nfs_bio{read,write}.
	 * The point of having it here is to keep us as synchronized with the
	 * server as we would have been had the nfs file not been mapped. Also
	 * helping in that synchronization goal are the mapfs_memfree calls in
	 * nfs_{get,load}attrcache.
	 */
	if (vp->v_tag == VT_NFS) {
		struct nfsnode *np = VTONFS(vp);
		struct proc	*p = uio->uio_procp;
		struct vattr	vattr;

		if (rw == UIO_WRITE) {
			NFSTRACE4(NFSTRC_MIO_WRT, vp,
				 uio->uio_offset, uio->uio_resid,
				 (ioflag & IO_APPEND		? 0x0010 : 0) |
				 (ioflag & IO_SYNC		? 0x0020 : 0) |
				 (np->n_flag & NMODIFIED	? 0x0001 : 0) |
				 (vp->v_vm_info->nfsdirty	? 0x0002 : 0));
			if (ioflag & (IO_APPEND | IO_SYNC)) {
				if (np->n_flag & NMODIFIED || vp->v_vm_info->nfsdirty) {
					np->n_attrstamp = 0;
					if ((error = nfs_vinvalbuf(vp, V_SAVE,
								   cred, p, 1)))
						return (error);
				}
				if (ioflag & IO_APPEND) {
					np->n_attrstamp = 0;
					if ((error = VOP_GETATTR(vp, &vattr,
								 cred, p)))
						return (error);
				}
			}
		} else { /* UIO_READ we presume */
			NFSTRACE4(NFSTRC_MIO_READ, vp,
				 uio->uio_offset, uio->uio_resid,
				 (np->n_flag & NMODIFIED	? 0x0001 : 0) |
				 (vp->v_vm_info->nfsdirty	? 0x0002 : 0));
			if (np->n_flag & NMODIFIED || vp->v_vm_info->nfsdirty) {
				np->n_attrstamp = 0;
				if ((error = VOP_GETATTR(vp, &vattr, cred, p)))
					return (error);
				np->n_mtime = vattr.va_mtime.tv_sec;
			} else {
				if ((error = VOP_GETATTR(vp, &vattr, cred, p)))
					return (error);
				else if (np->n_mtime != vattr.va_mtime.tv_sec) {
					NFSTRACE(NFSTRC_MIO_RINV, vp);
					if ((error = nfs_vinvalbuf(vp, V_SAVE,
								   cred, p, 1)))
						return (error);
					np->n_mtime = vattr.va_mtime.tv_sec;
				}
			}
		}
	}

	error = mapfs_get(vp, (vm_offset_t)uio->uio_offset, uio->uio_resid);
	if (error)
	        goto out;
	vmp = vp->v_vm_info;

	if ((rw == UIO_WRITE) && (ioflag & IO_APPEND)) {
		uio->uio_offset = vmp->vnode_size;
	}
#if PERFMODS
	bsize = PAGE_SIZE;
#else
	bsize = vp->v_mount->mnt_stat.f_bsize;

#define MAPFS_DEFAULT_BLOCKSIZE		4096
	/* In some cases the f_bsize is not set; then force it to 
	 * default; porbably should consider changing to f_iosize
	 * but not sure whether this will be any accurate either
	 * We need this anyway
	 */
	if (bsize == 0)
		bsize = MAPFS_DEFAULT_BLOCKSIZE;
#endif
	/*
	 *	Set credentials.
	 */
	if (rw == UIO_WRITE || (rw == UIO_READ && vmp->cred == NULL)) {
		cred = crdup(cred);
		cr = vmp->cred;
		if (cr != NOCRED) {
			vmp->cred = NOCRED;
			crfree(cr);
		}
		vmp->cred = cred;
	}

	/* Clear errors before we start */
	vmp->error = 0;

#if PERFMODS
	if (rw == UIO_WRITE) {
	        /*
		 * set up range for this I/O
		 */
	        newoffset = uio->uio_offset;
		newsize   = uio->uio_resid;

	        if (vmp->dirtysize) {
		        /*
			 * if a dirty range already exists, coalesce with the new range, but
			 * don't update the vmp fields yet, because if there was no intersection
			 * between the old range and the range that encompasses the new I/O
			 * we may want to push the old range and not do the coalesce if the new coalesced
			 * size exceeds CHUNK_SIZE
			 */
		        if (newoffset > vmp->dirtyoffset)
			        newoffset = vmp->dirtyoffset;

			if ((uio->uio_offset + uio->uio_resid) > (vmp->dirtyoffset + vmp->dirtysize))
			        newsize = (uio->uio_offset + uio->uio_resid) - newoffset;
			else
			        newsize = (vmp->dirtyoffset + vmp->dirtysize) - newoffset;

			if (newsize > CHUNK_SIZE && ((uio->uio_offset > (vmp->dirtyoffset + vmp->dirtysize)) ||
						     (uio->uio_offset + uio->uio_resid) < vmp->dirtyoffset)) {
			        /*
				 * the new coalasced size exceeded CHUNK_SIZE, and there was no intersection
				 * with the current dirty range, so push the current dirty range....
				 * the new dirty range will be set to the range encompassing this I/O request
				 */
			        vmp_push_range(vmp, vmp->dirtyoffset, vmp->dirtysize);
				newoffset = uio->uio_offset;
				newsize   = uio->uio_resid;
			}
		} 
		/*
		 * now make sure that the proposed dirty range is fully encompassed by the
		 * current vm mapping of the file... if not, we'll clip at either end
		 * if there is no intersection at all with the current mapping, than
		 * we'll set the dirty size to 0.... note that any previous dirty pages would
		 * have been pushed above since they must have fit in the current mapping and
		 * if the new range doesn't intersect with the current mapping, than we couldn't
		 * have coalesced with the old range...  in this case, we'll be going through the
		 * remap path before issuing any I/O... that path will set the dirty range accordingly
		 */
		if (newoffset < vmp->offset) {
		        if ((vmp->offset - newoffset) < newsize)
			        newsize -= vmp->offset - newoffset;
			else
			        newsize = 0;
		        newoffset = vmp->offset;
		}
		if ((newoffset + newsize) > (vmp->offset + vmp->size))
		        newsize = (vmp->offset + vmp->size) - newoffset;

		vmp->dirtyoffset = newoffset;
		vmp->dirtysize   = newsize;
	}
#endif /* PERFMODS */

	do {
		n = MIN((unsigned)bsize, uio->uio_resid);

		if (rw == UIO_READ) {
			/*
			 * only read up to the end of the file
			 */
			if ((diff = (int)(vmp->vnode_size - uio->uio_offset)) <= 0) {
				mapfs_put(vp);
				return (0);
			}
			if (diff < n)
				n = diff;
		} else if (((vm_size_t)uio->uio_offset) + n > vmp->vnode_size)
			vmp->vnode_size = (vm_size_t)uio->uio_offset + n;

		/*
		 *	Check to be sure we have a valid window
		 *	for the mapped file.
		 */
		if (((vm_offset_t)uio->uio_offset < vmp->offset) ||
		   (((vm_offset_t)uio->uio_offset + n) > (vmp->offset + vmp->size))) {

			if ((mapfsio_size = (vmp->size << 1)) > mfs_max_window)
			        mapfsio_size = mfs_max_window;

			error = remap_vnode(vp, (vm_offset_t)uio->uio_offset, mapfsio_size);
			/*
			 * remap_vnode does a push of the dirty pages and then
			 * sets vmp->dirtyoffset and vmp->dirtysize to 0
			 */
			if (error)
				goto out;
			/*
			 * new dirty range encompasses the remaining I/O of this request
			 */
			vmp->dirtyoffset = uio->uio_offset;
			vmp->dirtysize   = uio->uio_resid;

			/*
			 * make sure the new dirty range doesn't extend beyond the end of the map
			 */
			if ((vmp->dirtyoffset + vmp->dirtysize) > (vmp->offset + vmp->size))
			        vmp->dirtysize = (vmp->offset + vmp->size) - vmp->dirtyoffset;
		}
		va = vmp->va + (vm_offset_t)uio->uio_offset - vmp->offset;

		vmp->busy = TRUE;

		if (rw == UIO_WRITE)
			vmp->nfsdirty = TRUE;

		error = uiomove((caddr_t)va, (int)n, uio);

		vmp->busy = FALSE;

		if (error)
			goto out;

		if (vmp->delayed_fsync) {
			vmp->delayed_fsync = FALSE;

                        if (rw == UIO_WRITE)
			    vmp->dirtysize = uio->uio_offset - vmp->dirtyoffset;

			error = vmp_push_range(vmp, vmp->dirtyoffset, vmp->dirtysize);
			if (error)
				goto out;

                        if (rw == UIO_WRITE) {
			    /*
			     * new dirty range encompasses the remaining I/O of this request
			     */
			    vmp->dirtyoffset = uio->uio_offset;
			    vmp->dirtysize   = uio->uio_resid;

			    /*
			     * make sure the new dirty range doesn't extend beyond 
                             * the end of the map
			     */
			    if ((vmp->dirtyoffset + vmp->dirtysize) > (vmp->offset + vmp->size))
			        vmp->dirtysize = (vmp->offset + vmp->size) - vmp->dirtyoffset;
                        }
		} else	if (rw == UIO_WRITE)
			/*
			 *	Set dirty bit each time through loop just in
			 *	case remap above caused it to be cleared.
			 */
			vmp->dirty = TRUE;

		/*
		 *	Check for errors left by the pager.  Report the
		 *	error only once.
		 */
		if (vmp->error) {
			error = vmp->error;
			vmp->error = 0;
			/*
			 * The error might have been a permission
			 * error based on the credential.  We release it
			 * so that the next person who tries a read doesn't
			 * get stuck with it.
			 */
			cr = vmp->cred;
			if (cr != NOCRED) {
				vmp->cred = NOCRED;
				crfree(cr);
			}
		}

		/*
		 * Test to prevent mfs from swamping the buffer cache,
		 * locking out higher-priority transfers, like
		 * pageins, and causing system hangs.
		 */ 
	} while (error == 0 && uio->uio_resid > 0);

#if PERFMODS
	if (error == 0 && rw == UIO_WRITE) {
		/*
		 * Since the window may be as much as 4 Mbytes; write it out
		 * when we reach or exceed CHUNK_SIZE to avoid flooding the
		 * underlying disks with a huge stream of writes all at once
		 */
		if ((ioflag & IO_SYNC) || vmp->dirtysize >= CHUNK_SIZE) {

			error = vmp_push_range(vmp, vmp->dirtyoffset, vmp->dirtysize);

			if (error == 0 && (ioflag & IO_SYNC)) {
			        error = VOP_FSYNC(vp, cred, MNT_WAIT, (struct proc *)0);
				if (error)
					goto out;
			}

			/* This looks like redundant info; but I am keeping this
			 * as this worked at least from one reported case
			 */
			if (vmp->error) {
			        error = vmp->error;
				vmp->error = 0;
			}
		}
	}
#else
	if (
		(error == 0) &&
		(rw == UIO_WRITE) && 
		(ioflag & IO_SYNC)) {

		
		error = vmp_push(vmp);	/* initiate all i/o */
		if (!error) {
			error = VOP_FSYNC(vp, cred, MNT_WAIT, (struct proc *)0);
			if (error)
				goto out;
		}
		/* This looks like redundant info; but I am keeping this
		 * as this worked at least from one reported case
		 */
		if (vmp->error) {
			error = vmp->error;
			vmp->error = 0;
		}
	}
#endif /* PERFMODS */
out:
	mapfs_put(vp);
	return(error);
}

/*
 *	mapfs_sync:
 *
 *	Sync the mfs cache (called by sync()).
 */
int
mapfs_sync()
{
	register struct vm_info	*vmp, *next;
	int			last_version;
	int error = 0;

	vm_info_lock();
	last_version = vm_info_version;
	vmp = (struct vm_info *) queue_first(&vm_info_queue);
	while (!queue_end(&vm_info_queue, (queue_entry_t) vmp)) {
		next = (struct vm_info *) queue_next(&vmp->lru_links);
		if (vmp->dirty) {
			vm_info_unlock();
			vmp_get(vmp);
			error = vmp_push(vmp);
			vmp_put(vmp);
			if (error)
				goto out;
			vm_info_lock();
			/*
			 *	Since we unlocked, the get and put
			 *	operations would increment version by
			 *	two, so add two to our version.
			 *	If anything else happened in the meantime,
			 *	version numbers will not match and we
			 *	will restart.
			 */
			last_version += 2;
		}
		/*
		 *	If the version didn't change, just keep scanning
		 *	down the queue.  If the version did change, we
		 *	need to restart from the beginning.
		 */
		if (last_version == vm_info_version) {
			vmp = next;
		}
		else {
			vmp = (struct vm_info *) queue_first(&vm_info_queue);
			last_version = vm_info_version;
		}
	}
	vm_info_unlock();
out:
	return(error);
}

/*
 *	Sync pages in specified vnode.
 */
int
mapfs_fsync(vp)
	struct vnode	*vp;
{
	struct vm_info	*vmp;
	int error=0;
	vmp = vp->v_vm_info;
	if (vp->v_type == VREG && vmp != VM_INFO_NULL && vmp->mapped) {
		vmp_get(vmp);
		error = vmp_push(vmp);
		vmp_put(vmp);

		return(error);
	}
	return(0);
}


#if 0 /* dead code elimination */
/*
 *	Sync pages in specified vnode, annd invalidate clean.
 *	The vm_info lock protects the vm_info from modification,
 *	or removal. XXX Must protect against sync/invalidate race 
 */
int
mapfs_fsync_invalidate(vp, flag)
	struct vnode	*vp;
{
	struct vm_info	*vmp;

	vmp = vp->v_vm_info;
	if (vp->v_type == VREG && vmp != VM_INFO_NULL && vmp->mapped) {

		/*	Part of vmp_get(vmp), we don't actually
	 	 *	need the write lock if we hold a ref as
	 	 *	we are not changing the vm_info data
 	 	 *
	 	 *	Remove from LRU list (if its there).
	 	 */
		vm_info_lock();
		if (vmp->queued) {
			vm_info_dequeue(vmp);
		}
		vmp->use_count++;	/* to protect requeueing in vmp_put */
		vm_info_unlock();

		if (!(flag & MFS_NOFLUSH))
			vmp_push_all(vmp);

		/* This is not under a lock, nor is it in vm_put XXX */
		/* But it is below */
		if (!(flag & MFS_NOINVALID)){
			vmp->invalidate = FALSE;
			vmp_invalidate(vmp);
		}
		/*
	 	 *	Place back on LRU list if noone else using it.
	 	 */
		vm_info_lock();
		if (--vmp->use_count == 0) {
			vm_info_enqueue(vmp);
		}
		vm_info_unlock();
		return(vmp->error);

	}
	return(0);
}
#endif



/*
 *	Invalidate pages in specified vnode.
 */
int
mapfs_invalidate(vp)
	struct vnode	*vp;
{
	struct vm_info	*vmp;

	vmp = vp->v_vm_info;
	if (vp->v_type == VREG && vmp != VM_INFO_NULL && vmp->mapped) {
		if (vmp->use_count > 0)
			vmp->invalidate = TRUE;
		else {
			vmp_get(vmp);
			vmp_invalidate(vmp);
			vmp_put(vmp);
		}
	}
	return(vmp ? vmp->error : 0);
}

#import <vm/vm_page.h>
#import <vm/vm_object.h>

/*
 *	Search for and flush pages in the specified range.  For now, it is
 *	unnecessary to flush to disk since I do that synchronously.
 */
void vno_flush(vp, start, size)
	struct vnode		*vp;
	register vm_offset_t	start;
	vm_size_t		size;
{
	register vm_offset_t	end;
	register vm_object_t	object;
	register vm_page_t	m;

	object = vp->v_vm_info->object;
	if (object == VM_OBJECT_NULL)
		return;

#if SCRUBVM3
	/* Isn't this the wrong order to aquire the lock */
#endif
	vm_page_lock_queues();
	vm_object_lock(object);	/* mfs code holds reference */
	end = round_page(size + start);	/* must be first */
	start = trunc_page(start);
	while (start < end) {
		m = vm_page_lookup(object, start);
		if (m != VM_PAGE_NULL) {
			if (m->busy) {
#if SCRUBVM3
				/* THIS SHOULD NOT HAPPEN IF ONLY ASYNC
				 * on SWAP */
				/* hint if we miss it its ok */
				if (m->dry_vp){
				    /* object and page queues locked, note
				     * page might not be clean wrt backing
				     * store */	
				    (void) vm_page_completeio(m, TRUE);
				} else {
#endif
					PAGE_ASSERT_WAIT(m, FALSE);
					vm_object_unlock(object);
					vm_page_unlock_queues();
					thread_block();
					vm_page_lock_queues();
					vm_object_lock(object);
					continue;	/* try again */
#if SCRUBVM3
				}
#endif
			}
			vm_page_free(m);
		}
		start += PAGE_SIZE;
	}
	vm_object_unlock(object);
	vm_page_unlock_queues();
}


int mfs_mdirty;
int mfs_mclean;
/*
 *	Search for and free pages in the specified vmp.
 */
void
vmp_invalidate(struct vm_info *vmp)
{
	register vm_object_t	object;
	register vm_page_t	m;

	NFSTRACE(NFSTRC_VMP_INV, vmp);
	object = vmp->object;
	if (object == VM_OBJECT_NULL)
		return;

	vm_page_lock_queues();
	vm_object_lock(object);	/* mfs code holds reference */

	/* Sanity. Different code calls this with and without the vminfo
	 * lock. The locking needs to be fixed for MP. XXX 
	 */
	if (vmp->object != object) {
		vm_object_unlock(object);
		vm_page_unlock_queues();
		return;
	}

retry:
	m = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) m)) {
		vm_page_t next = (vm_page_t) queue_next(&m->listq);

		/* If NFS is paging us in we are not really valid yet. XXX
		 * Re-address this. Without this check we can block forever
		 * waiting on the busy bit that we set. */
		if (m->nfspagereq == TRUE){
			m = next;
			continue;
		}

		if (m->busy) {
#if SCRUBVM3
			/* THIS SHOULD NOT HAPPEN IF ONLY ASYNC
			 * on SWAP */
			/* hint if we miss it its ok */
			if (m->dry_vp){
				/* object and page queues locked, note
				 * page might not be clean wrt backing
				 * store */	
				(void) vm_page_completeio(m, TRUE);
			} else {
#endif
				PAGE_ASSERT_WAIT(m, FALSE);
				vm_object_unlock(object);
				vm_page_unlock_queues();
				thread_block();
				vm_page_lock_queues();
				vm_object_lock(object);
				goto retry;
#if SCRUBVM3
				}
#endif
		}

		/* Kill off the translation as well.
		 * mapfs_map_remove removes them as well, but as
		 * we have seen not everyone calls that.
		 *
		 * If there is a ref to this file and we are being called
		 * and the page is wired we will skip this page. If there 
		 * are no more refs to this file and we are being called 
		 * the wire count should always be zero. In the future we
		 * may want to block on the wired count. XXX joh
		 */
		if (m->wire_count == 0){
			pmap_remove_all(VM_PAGE_TO_PHYS(m));
			/* In the case of mfs only one guy can be in here at a
			 * a time. In the case of mmap they can be dirtying
			 * pages in parallel . So after our sync and invalidate
			 * above we need to check again. If someone has re-
			 * written them again, then they get to keep the page.
			 * NFS does not give any assurances for multiple
			 * writers on different nodes.
			 */
			if ((m->clean == FALSE) ||
			     pmap_is_modified(VM_PAGE_TO_PHYS(m))){
				mfs_mdirty++;
			} else {
				mfs_mclean++;
				vm_page_free(m);
			}
		} 
		m = next;
	}
	vm_object_unlock(object);
	vm_page_unlock_queues();
}


/*
 *	Search for and push (to disk) pages in the specified range.
 *	We need some better interactions with the VM system to simplify
 *	the code. Force tries to push the object regardless of whether 
 *	the MFS thinks it is dirty (mmap could have written it). Some day
 *	vmp_push could support ranges vmp_push(vmp,start,size).
 */

/* Something must be done to handle dirty wired pages. XXX joh */
int
vmp_push(vmp)
	struct vm_info		*vmp;
{
	register vm_offset_t	start;
	vm_size_t		size;
	int error=0;

	if (!vmp->dirty)
		return(0);
	start = vmp->offset;
	size = vmp->size;
	
	/* vmp->dirty is set FALSE in vmp_push_range */
	error = vmp_push_range(vmp, start, size);

	return(error);
}

int
vmp_push_range(vmp, start, size)
	struct vm_info		*vmp;
	register vm_offset_t	start;
	vm_size_t		size;
{
	register vm_offset_t	end;
	register vm_object_t	object;
	register vm_page_t	m;
	struct vattr            vattr;
	int error=0;

	NFSTRACE4(NFSTRC_VPR, vmp->vnode, start, size, vmp->busy);
	if (!vmp->dirty)
		return(0);
	if (vmp->busy) {
	        vmp->delayed_fsync = TRUE;
	        return(0);
	}
	vmp->dirty = FALSE;
        vmp->dirtysize = 0;
        vmp->dirtyoffset = 0;

	object = vmp->object;
	/*  We are trying to catch BSd error; no need to bother
	 * about these errors for now 
	 */
	if (object == VM_OBJECT_NULL)
		return(0);

	vm_page_lock_queues();
	vm_object_lock(object);	/* mfs code holds reference */

	end = round_page(size + start);	/* must be first */
	start = trunc_page(start);
	/* Cleanup error before we start */
	vmp->error = 0;

	while (start < end) {
		m = vm_page_lookup(object, start);
		/* We don't want to deadlock on the page we are bring in */
		if ((m != VM_PAGE_NULL) && (m->nfspagereq == FALSE)){
			if (m->busy) {
#if SCRUBVM3
				/* THIS SHOULD NOT HAPPEN IF ONLY ASYNC
			 	 * on SWAP */
				/* hint if we miss it its ok */
				if (m->dry_vp){
					/* object and page queues locked, note
				 	 * page might not be clean wrt backing
				 	 * store */	
					(void) vm_page_completeio(m, TRUE);
				} else {
#endif
					PAGE_ASSERT_WAIT(m, FALSE);
					vm_object_unlock(object);
					vm_page_unlock_queues();
					thread_block();
					vm_page_lock_queues();
					vm_object_lock(object);
					continue;	/* try again */
#if SCRUBVM3
				}
#endif
			}
			if (!m->active) {
				vm_page_activate(m); /* so deactivate works */
			}
			vm_page_deactivate(m);	/* gets dirty/laundry bit */
			/*
			 *	Prevent pageout from playing with
			 *	this page.  We know it is inactive right
			 *	now (and are holding lots of locks keeping
			 *	it there).
			 */
			queue_remove(&vm_page_queue_inactive, m, vm_page_t,
				     pageq);
			m->inactive = FALSE;
			vm_page_inactive_count--;
			m->busy = TRUE;
			if (m->laundry) {
				pager_return_t	ret;

				pmap_remove_all(VM_PAGE_TO_PHYS(m));
				object->paging_in_progress++;
				vm_object_unlock(object);
				vm_page_unlock_queues();
				/* should call pageout daemon code */
				ret = vnode_pageout(m);
				vm_page_lock_queues();
				vm_object_lock(object);
				object->paging_in_progress--;
				if (ret == PAGER_SUCCESS) {
					/* vnode_pageout marks clean */
#if PERFMODS
				        pmap_clear_reference(VM_PAGE_TO_PHYS(m));
#endif /* PERFMODS */
					m->laundry = FALSE;
				} else {
					/* don't set dirty bit, unrecoverable
					   errors will cause update to go
					   crazy.  User is responsible for
					   retrying the write */
					/* vmp->dirty = TRUE; */
					error = vmp->error;
					vmp->error =0;
				}
				/* if pager failed, activate below */
			}
			vm_page_activate(m);
			m->busy = FALSE;
			PAGE_WAKEUP(m);
		}
		start += PAGE_SIZE;
	}
	vmp->nfsdirty = FALSE;
	vm_object_unlock(object);
	vm_page_unlock_queues();

	/*
	 * On error we have to reset the true file size in the vmp
	 * structure.  The lack of a credential structure pointer
	 * would indicate nothing was changing in the file.
	 */
	if (error && vmp->cred) {
	  vmp->filesize=TRUE;
	  VOP_GETATTR (vmp->vnode, &vattr, vmp->cred, current_proc());
	  vmp->filesize=FALSE;
	  vmp->vnode_size = vattr.va_size;
	}
	NFSTRACE(NFSTRC_VPR_DONE, error);

	return(error);
}


#if 0 /* dead code elimination */
/* Something must be done to handle dirty wired pages. XXX joh */
void
vmp_push_all(vmp)
	struct vm_info		*vmp;
{
	register vm_object_t	object;
	register vm_page_t	m;
	struct vattr            vattr;
	int error=0;

	vmp->dirty = FALSE;

	object = vmp->object;
	if (object == VM_OBJECT_NULL)
		return;

	vm_page_lock_queues();
	vm_object_lock(object);	/* mfs code holds reference */

retry:
	m = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) m)) {
		/* We don't want to deadlock on the page we are bring in */
		if (m->nfspagereq == FALSE){
			if (m->busy) {
#if SCRUBVM3
				/* THIS SHOULD NOT HAPPEN IF ONLY ASYNC
			 	 * on SWAP */
				/* hint if we miss it its ok */
				if (m->dry_vp){
					/* object and page queues locked, note
				 	 * page might not be clean wrt backing
				 	 * store */	
					(void) vm_page_completeio(m, TRUE);
				} else {
#endif
					PAGE_ASSERT_WAIT(m, FALSE);
					vm_object_unlock(object);
					vm_page_unlock_queues();
					thread_block();
					vm_page_lock_queues();
					vm_object_lock(object);
					/* Page may be long gone, XXX Forward 
					 * progress */
					goto retry;	
#if SCRUBVM3
				}
#endif
			}
			if (!m->active) {
				vm_page_activate(m); /* so deactivate works */
			}
			vm_page_deactivate(m);	/* gets dirty/laundry bit */
			/*
			 *	Prevent pageout from playing with
			 *	this page.  We know it is inactive right
			 *	now (and are holding lots of locks keeping
			 *	it there).
			 */
			queue_remove(&vm_page_queue_inactive, m, vm_page_t,
				     pageq);
			m->inactive = FALSE;
			vm_page_inactive_count--;
			m->busy = TRUE;
			if (m->laundry) {
				pager_return_t	ret;

				pmap_remove_all(VM_PAGE_TO_PHYS(m));
				object->paging_in_progress++;
				vm_object_unlock(object);
				vm_page_unlock_queues();
				/* should call pageout daemon code */
				ret = vnode_pageout(m);
				vm_page_lock_queues();
				vm_object_lock(object);
				object->paging_in_progress--;
				if (ret == PAGER_SUCCESS) {
					/* vnode_pageout marks clean */
					m->laundry = FALSE;
				} else {
					/* don't set dirty bit, unrecoverable
					   errors will cause update to go
					   crazy.  User is responsible for
					   retrying the write */
					/* vmp->dirty = TRUE; */
				        error = vmp->error;
					vmp->error=0;
				}
				/* if pager failed, activate below */
			}
			vm_page_activate(m);
			m->busy = FALSE;
			PAGE_WAKEUP(m);
		}
		m = (vm_page_t) queue_next(&m->listq);
	}
	vmp->nfsdirty = FALSE;
	vm_object_unlock(object);
	vm_page_unlock_queues();

	/*
	 * On error we have to reset the true file size in the vmp
	 * structure.  The lack of a credential structure pointer
	 * would indicate nothing was changing in the file.
	 */
	if (error && vmp->cred) {
	  vmp->filesize=TRUE;
	  VOP_GETATTR (vmp->vnode, &vattr, vmp->cred, current_proc());
	  vmp->filesize=FALSE;
	  vmp->vnode_size = vattr.va_size;
	}
}
#endif


vm_size_t vm_get_vnode_size(struct vnode *vp) 
{
	return(vp->v_vm_info->vnode_size);
}

void vm_set_vnode_size(struct vnode *vp, vm_size_t vnode_size) 
{
	vp->v_vm_info->vnode_size = vnode_size;
}

void vm_set_close_flush(struct vnode *vp, boolean_t close_flush)
{
	vp->v_vm_info->close_flush = close_flush ? 1 : 0;
}

void vm_set_error(struct vnode *vp, int error)
{
	vp->v_vm_info->error = error;
}
#endif	/* MACH_NBC  ] */
