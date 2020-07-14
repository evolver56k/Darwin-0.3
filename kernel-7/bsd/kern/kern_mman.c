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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.10 (Berkeley) 2/19/95
 */

/*
 * Mapped file (mmap) interface to VM
 */

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/filedesc.h>
#import <sys/proc.h>
#import <sys/resourcevar.h>
#import <sys/buf.h>
#import <sys/vnode.h>
#import <sys/acct.h>
#import <sys/wait.h>
#import <sys/file.h>
#import <sys/vadvise.h>
#import <sys/trace.h>
#import <sys/mman.h>
#import <sys/conf.h>
#import <mach/kern_return.h>
#import <kern/task.h>
#import <mach/vm_param.h>
#import <vm/vm_map.h>
#import <vm/vm_pager.h>
#import <vm/vnode_pager.h>
#import <kern/mapfs.h>
#import <miscfs/specfs/specdev.h>

struct sbrk_args {
		int	incr;
};

/* ARGSUSED */
int
sbrk(p, uap, retval)
	struct proc *p;
	struct sbrk_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	kprintf("sbrk(): not supported!\n");
	return (EOPNOTSUPP);
}

struct sstk_args {
	int	incr;
} *uap;

/* ARGSUSED */
int
sstk(p, uap, retval)
	struct proc *p;
	struct sstk_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	kprintf("sstk(): not supported!\n");
	return (EOPNOTSUPP);
}

#if COMPAT_43
/* ARGSUSED */
int
ogetpagesize(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{

	*retval = PAGE_SIZE;
	return (0);
}
#endif /* COMPAT_43 */

struct osmmap_args {
		caddr_t	addr;
		int	len;
		int	prot;
		int	share;
		int	fd;
		long	pos;
};

osmmap(curp, uap, retval)
	struct proc *curp;
	register struct osmmap_args *uap;
	register_t *retval;
{
struct mmap_args {
		caddr_t addr;
		size_t len;
		int prot;
		int flags;
		int fd;
#ifdef DOUBLE_ALIGN_PARAMS
		long pad;
#endif
		off_t pos;
} newargs;

	if ((uap->share ==  MAP_SHARED )|| (uap->share ==  MAP_PRIVATE )) {
		newargs.addr = uap->addr;
		newargs.len = (size_t)uap->len;
		newargs.prot = uap->prot;
		newargs.flags = uap->share;
		newargs.fd = uap->fd;
		newargs.pos = (off_t)uap->pos;
		return(mmap(curp,&newargs, retval));
	} else
		return(EINVAL);	
}

struct mmap_args {
		caddr_t addr;
		size_t len;
		int prot;
		int flags;
		int fd;
#ifdef DOUBLE_ALIGN_PARAMS
		long pad;
#endif
		off_t pos;
};

mmap(p, uap, retval)
	struct proc *p;
	struct mmap_args *uap;
	register_t *retval;
{
	/*
	 *	Map in special device (must be SHARED) or file
	 */
	struct file *fp;
	register struct vnode *vp;
	int		flags;
	int		prot;
	int		err=0;
	vm_map_t	user_map;
	kern_return_t	result;
	vm_offset_t	user_addr;
	vm_size_t	user_size;
	vm_offset_t	file_pos;
	boolean_t	find_space;
	extern vm_object_t	vm_object_special();

	user_addr = (vm_offset_t)uap->addr;
	user_size = (vm_size_t) uap->len;

	prot = (uap->prot & VM_PROT_ALL);
	flags = uap->flags;

	/*
	 * The vm code does not have prototypes & compileser doesn't do the'
	 * the right thing when you cast 64bit value and pass it in function 
	 * call. So here it is.
	 */
	file_pos = (vm_offset_t)uap->pos;

	/* Anonymous mapping not supported, for now. */
	if (((flags & MAP_FIXED) && !page_aligned(user_addr)) ||
				(user_size < 0) || (flags & MAP_ANON)) {
		return(EINVAL);
	}

	err = fdgetf(p, uap->fd, &fp);
	if (err)
		return(err);

	if (fp->f_type != DTYPE_VNODE)
		return(EBADF);
	vp = (struct vnode *)fp->f_data;

	/*
	 *	We bend a little - round the start and end addresses
	 *	to the nearest page boundary.
	 */
	user_addr = trunc_page(user_addr);
	user_size = round_page(user_size);

	/*
	 *	File can be COPIED at an arbitrary offset.
	 *	File can only be SHARED if the offset is at a
	 *	page boundary.
	 */

	if ((flags & MAP_SHARED) && ((vm_offset_t)file_pos & page_mask)) {
		return(EINVAL);
	}

	/*
	 *	File must be writable if memory will be.
	 */
	if ((prot & PROT_WRITE) && (fp->f_flag&FWRITE) == 0) {
		if ((flags & MAP_PRIVATE) == 0) {
		/*  #2211802 */
		/* If private map, its okay to have write access 
		   as allocated space is both read and write
		 */
			return(EACCES);
		}
	}
	if ((prot & PROT_READ) && (fp->f_flag&FREAD) == 0) {
		return(EACCES);
	}
	/*
	 * Bug# 2203998 ->  zero sized length is a valid arg
	 *	in 4.4; for ex, copy of zero sized file
	 *	will call mmap with length 0
	 */
	if (user_size == 0) 
		return(0);

	/*
	 *	memory must exist and be writable (even if we're
	 *	just reading)
	 */

	user_map = current_task()->map;

#if 0
	if (!vm_map_check_protection(user_map, user_addr,
				(vm_offset_t)(user_addr + user_size),
				VM_PROT_READ|VM_PROT_WRITE)) {
		return(EINVAL);
	}
#endif
	if (flags & MAP_FIXED) {
		find_space = FALSE;
	} else {
		find_space = TRUE;
	}

	if (vp->v_type == VCHR || vp->v_type == VSTR) {
		return(EOPNOTSUPP);
	}
	else {
		/*
		 *	Map in a file.  May be PRIVATE (copy-on-write)
		 *	or SHARED (changes go back to file)
		 */
		vm_pager_t	pager;
		vm_map_t	copy_map;
		vm_offset_t	off;
		struct vm_info	*vmp;

		/*
		 *	Only allow regular files for the moment.
		 */
		if (vp->v_type != VREG) {
			return(EBADF);
		}
		
		pager = vnode_pager_setup(vp, FALSE, TRUE);
		
		/*
		 *  Set credentials:
		 *	FIXME: if we're writing the file we need a way to
		 *      ensure that someone doesn't replace our R/W creds
		 * 	with ones that only work for read.
		 */
		vmp = vp->v_vm_info;
		if (vmp->cred == NULL) {			
			crhold(p->p_ucred);
			vmp->cred = p->p_ucred;
		}
		/* Find space if not MAP_FIXED; otherwise validate */
		/* This also fixes 2215605 */
		result = vm_map_find(user_map, NULL, (vm_offset_t) 0,
					&user_addr, user_size, find_space);
		if (result != KERN_SUCCESS) {
			goto out;
		}
		if (flags & MAP_SHARED) {
			/*
			 *	Map it directly, allowing modifications
			 *	to go out to the inode.
			 */
			(void) vm_deallocate(user_map, user_addr, user_size);
			result = vm_allocate_with_pager(user_map,
					&user_addr, user_size, find_space,
					pager,
					(vm_offset_t)file_pos);
			if (result != KERN_SUCCESS) {
				goto out;
			}
		}
		else {
#if 0
			/* Moved to cover both shared and private cases */
			result = vm_map_find(user_map, NULL, (vm_offset_t) 0,
					&user_addr, user_size, find_space);
			if (result != KERN_SUCCESS) {
				goto out;
			}
#endif /* 0 */
			/*
			 *	Copy-on-write of file.  Map into private
			 *	map, then copy into our address space.
			 */
			copy_map = vm_map_create(pmap_create(user_size),
					0, user_size, TRUE);
			off = 0;
			result = vm_allocate_with_pager(copy_map,
					&off, user_size, find_space,
					pager,
					(vm_offset_t)file_pos);
			if (result != KERN_SUCCESS) {
				vm_map_deallocate(copy_map);
				goto out;
			}
			result = vm_map_copy(user_map, copy_map,
					user_addr, user_size,
					0, FALSE, FALSE);
			if (result != KERN_SUCCESS) {
				vm_map_deallocate(copy_map);
				goto out;
			}
			vm_map_deallocate(copy_map);
		}
	}

	/*
	 *	Our memory defaults to read-write.  If it shouldn't
	 *	be readable, protect it.
	 */
	if ((prot & PROT_WRITE) == 0) {
		result = vm_protect(user_map, user_addr, user_size,
					FALSE, VM_PROT_READ);
		if (result != KERN_SUCCESS) {
			(void) vm_deallocate(user_map, user_addr, user_size);
			goto out;
		}
	}

	/*
	 *	Shared memory is also shared with children
	 */
	/*
	 *	HACK HACK HACK
	 *	Since this memory CAN'T be made copy-on-write, and since
	 *	its users fork, we must change its inheritance to SHARED.
	 *	HACK HACK HACK
	 */
	if (flags & MAP_SHARED) {
		result = vm_inherit(user_map, user_addr, user_size,
				VM_INHERIT_SHARE);
		if (result != KERN_SUCCESS) {
			(void) vm_deallocate(user_map, user_addr, user_size);
			goto out;
		}
	}

	*fdflags(p, uap->fd) |= UF_MAPPED;

	if (result == KERN_SUCCESS)
		*retval = (register_t)user_addr;
	/* FALL THROUGH */
out: 
	switch (result) {
	case KERN_SUCCESS:
		return (0);
	case KERN_INVALID_ADDRESS:
	case KERN_NO_SPACE:
		return (ENOMEM);
	case KERN_PROTECTION_FAILURE:
		return (EACCES);
	default:
		return (EINVAL);
	}
}

struct msync_args {
		caddr_t addr;
		int len;
};
int
msync(p, uap, retval)
	struct proc *p;
	struct msync_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}


mremap()
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct munmap_args {
		caddr_t	addr;
		int	len;
};
munmap(p, uap, retval)
	struct proc *p;
	struct munmap_args *uap;
	register_t *retval;

{
	vm_offset_t	user_addr;
	vm_size_t	user_size;
	kern_return_t	result;

	user_addr = (vm_offset_t) uap->addr;
	user_size = (vm_size_t) uap->len;

	/*
	 * user size is allocated to page boundary and size from
	 * the user land is not necessarily in multiple of page size
	 */
	user_size = round_page(user_size);
	if ((user_addr & page_mask) ||
	    (user_size & page_mask)) {
		return(EINVAL);
	}
	result = vm_deallocate(current_task()->map, user_addr, user_size);
	if (result != KERN_SUCCESS) {
		return(EINVAL);
	}
	return(0);
}

void
munmapfd(p, fd)
	struct proc *p;
	int fd;
{
	/*
	 * XXX should vm_deallocate any regions mapped to this file
	 */
	*fdflags(p, fd) &= ~UF_MAPPED;
}

struct mprotect_args {
		caddr_t addr;
		int len;
		int prot;
};
int
mprotect(p, uap, retval)
	struct proc *p;
	struct mprotect_args *uap;
	register_t *retval;
{
	vm_size_t size;
	register vm_prot_t prot;
	caddr_t addr;
	int len;
	vm_map_t	user_map;

	addr = uap->addr;
	len = uap->len;
	prot = (vm_prot_t)(uap->prot & VM_PROT_ALL);

	if (((unsigned long)addr & page_mask) || len < 0)
		return(EINVAL);
	size = (vm_size_t)len;

	user_map = current_task()->map;
	switch (vm_map_protect(user_map, addr, addr+size, prot,
	    FALSE)) {
	case KERN_SUCCESS:
		return (0);
	case KERN_PROTECTION_FAILURE:
		return (EACCES);
	}
	return (EINVAL);
}

struct madvise_args {
		caddr_t addr;
		int len;
		int behav;
};
/* ARGSUSED */
int
madvise(p, uap, retval)
	struct proc *p;
	struct madvise_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct mincore_args {
		caddr_t addr;
		int len;
		char * vec;
};
/* ARGSUSED */
int
mincore(p, uap, retval)
	struct proc *p;
	struct mincore_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct mlock_args {
		caddr_t addr;
		size_t len;
};
int
mlock(p, uap, retval)
	struct proc *p;
	struct mlock_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

struct munlock_args {
		caddr_t addr;
		size_t len;
};
int
munlock(p, uap, retval)
	struct proc *p;
	struct munlock_args *uap;
	register_t *retval;
{
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

int
map_fd(fd, offset, va, findspace, size)
	int		fd;
	vm_offset_t	offset;
	vm_offset_t	*va;
	boolean_t	findspace;
	vm_size_t	size;
{
	struct file	*fp;
	register struct vnode *vp;
	vm_map_t	user_map;
	kern_return_t	result;
	vm_offset_t	user_addr;
	vm_size_t	user_size;
	vm_pager_t	pager;
	vm_map_t	copy_map;
	vm_offset_t	off;

	user_map = current_task()->map;

	/*
	 *	Find the inode; verify that it's a regular file.
	 */
	if (getvnode(current_proc(), fd, &fp))
		return (KERN_INVALID_ARGUMENT);

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VREG)
		return(KERN_INVALID_ARGUMENT);

	user_size = round_page(size);

	if (findspace) {
		/*
		 *	Allocate dummy memory.
		 */
		result = vm_allocate(user_map, &user_addr, size, TRUE);
		if (result != KERN_SUCCESS)
			return(result);
		if (copyout(&user_addr, va, sizeof(vm_offset_t))) {
			(void) vm_deallocate(user_map, user_addr, size);
			return(KERN_INVALID_ADDRESS);
		}
	}
	else {
		/*
		 *	Get user's address, and verify that it's
		 *	page-aligned and writable.
		 */

		if (copyin(va, &user_addr, sizeof(vm_offset_t)))
			return(KERN_INVALID_ADDRESS);
		if ((trunc_page(user_addr) != user_addr))
			return(KERN_INVALID_ARGUMENT);
		if (!vm_map_check_protection(user_map, user_addr,
				(vm_offset_t)(user_addr + user_size),
				VM_PROT_READ|VM_PROT_WRITE))
			return(KERN_INVALID_ARGUMENT);
	}

	/*
	 * Allow user to map in a zero length file.
	 */
	if (size == 0)
		return KERN_SUCCESS;

	/*
	 *	Map in the file.
	 */

	pager = vnode_pager_setup(vp, FALSE, TRUE);

	/*
	 *	Map into private map, then copy into our address space.
	 */
	copy_map = vm_map_create(pmap_create(user_size), 0, user_size, TRUE);
	off = 0;
	result = vm_allocate_with_pager(copy_map, &off, user_size, FALSE,
					pager,
					offset);
	if (result == KERN_SUCCESS)
		result = vm_map_copy(user_map, copy_map, user_addr, user_size,
					0, FALSE, FALSE);

	if ((result != KERN_SUCCESS) && findspace)
		(void) vm_deallocate(user_map, user_addr, user_size);

	vm_map_deallocate(copy_map);

	/*
	 * Set credentials.
	 */
	if (vp->v_vm_info->cred == NULL) {
		crhold(current_proc()->p_ucred);
		vp->v_vm_info->cred = current_proc()->p_ucred;
	}

	return(result);
}


/* BEGIN DEFUNCT */
struct obreak_args {
	char *nsiz;
};
obreak(p, uap, retval)
	struct proc *p;
	struct obreak_args *uap;
	register_t *retval;
{
	/* Not implemented, obsolete */
	return (ENOMEM);
}

int	both;

ovadvise()
{

#ifdef lint
	both = 0;
#endif
}
/* END DEFUNCT */
