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
 *	Copyright (C) 1988, 1989,  NeXT, Inc.
 *
 *	File:	kern/mach_loader.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Mach object file loader (kernel version, for now).
 *
 * 21-Jul-88  Avadis Tevanian, Jr. (avie) at NeXT
 *	Started.
 */
#import <mach_nbc.h>
#import <sys/param.h>
#import <sys/vnode.h>
#import <sys/uio.h>
#import <sys/namei.h>
#import <sys/proc.h>
#import <sys/stat.h>
#import <sys/malloc.h>
#import <sys/mount.h>
#import <sys/fcntl.h>

#include <ufs/ufs/lockf.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>

#import <mach/mach_types.h>

#import <kern/mach_loader.h>
#import <kern/mapfs.h>

#import <mach-o/fat.h>
#import <mach-o/loader.h>

#import <bsd/machine/cpu.h>

#import <vm/vm_kern.h>
#import <vm/vm_pager.h>
#import <vm/vnode_pager.h>

/*
 * Prototypes of static functions.
 */
static
load_return_t
parse_machfile(
	struct vnode		*vp,
	vm_map_t		map,
	struct mach_header	*header,
	unsigned long		file_offset,
	unsigned long		macho_size,
	int			depth,
	unsigned long		*lib_version,
	load_result_t		*result
),
load_segment(
	struct segment_command	*scp,
	vm_pager_t		pager,
	unsigned long		pager_offset,
	unsigned long		macho_size,
	unsigned long		end_of_file,
	vm_map_t		map,
	load_result_t		*result
),
load_unixthread(
	struct thread_command	*tcp,
	load_result_t		*result
),
load_thread(
	struct thread_command	*tcp,
	load_result_t		*result
),
load_threadstate(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size
),
load_threadstack(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size,
	vm_offset_t	*user_stack
),
load_threadentry(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size,
	vm_offset_t	*entry_point
),
load_fvmlib(
	struct fvmlib_command	*lcp,
	vm_map_t		map,
	int			depth
),
load_idfvmlib(
	struct fvmlib_command	*lcp,
	unsigned long		*version
),
load_dylinker(
	struct dylinker_command	*lcp,
	vm_map_t		map,
	int			depth,
	load_result_t		*result
),
get_macho_vnode(
	char			*path,
	struct mach_header	*mach_header,
	unsigned long		*file_offset,
	unsigned long		*macho_size,
	struct vnode		**vpp
);

load_return_t
load_machfile(
	struct vnode		*vp,
	struct mach_header	*header,
	unsigned long		file_offset,
	unsigned long		macho_size,
	load_result_t		*result
)
{
	pmap_t			pmap;
	vm_map_t		map;
	vm_map_t		old_map;
	load_result_t		myresult;
	kern_return_t		kret;
	load_return_t		lret;

	old_map = current_task()->map;
	pmap = old_map->pmap;
	pmap_reference(pmap);
	map = vm_map_create(pmap,
			old_map->min_offset,
			old_map->max_offset,
			old_map->hdr.entries_pageable);

	if (!result)
		result = &myresult;

	*result = (load_result_t) { 0 };

	lret = parse_machfile(vp, map, header, file_offset, macho_size,
			     0, (unsigned long *)0, result);

	if (lret != LOAD_SUCCESS) {
		vm_map_deallocate(map);	/* will lose pmap reference too */
		return(lret);
	}
	/*
	 *	Commit to new map, destroy old map.  (Destruction
	 *	of old map cleans up pmap data structures too).
	 */
	current_task()->map = map;
	vm_map_deallocate(old_map);

	return(LOAD_SUCCESS);
}

static
load_return_t
parse_machfile(
	struct vnode		*vp,
	vm_map_t		map,
	struct mach_header	*header,
	unsigned long		file_offset,
	unsigned long		macho_size,
	int			depth,
	unsigned long		*lib_version,
	load_result_t		*result
)
{
	struct machine_slot	*ms;
	int			ncmds;
	struct load_command	*lcp, *next;
	struct dylinker_command	*dlp = 0;
	vm_pager_t		pager;
	load_return_t		ret;
	vm_offset_t		addr;
	vm_size_t		size;
	int			offset;
	int			pass;

	/*
	 *	Break infinite recursion
	 */
	if (depth > 6)
		return(LOAD_FAILURE);
	depth++;

	/*
	 *	Check to see if right machine type.
	 */
	ms = &machine_slot[cpu_number()];
	if ((header->cputype != ms->cpu_type) ||
	    !check_cpu_subtype(header->cpusubtype))
		return(LOAD_BADARCH);
		
	switch (header->filetype) {
	
	case MH_OBJECT:
	case MH_EXECUTE:
	case MH_PRELOAD:
		if (depth != 1)
			return (LOAD_FAILURE);
		break;
		
	case MH_FVMLIB:
	case MH_DYLIB:
		if (depth == 1)
			return (LOAD_FAILURE);
		break;

	case MH_DYLINKER:
		if (depth != 2)
			return (LOAD_FAILURE);
		break;
		
	default:
		return (LOAD_FAILURE);
	}

	/*
	 *	Get the pager for the file.
	 */
	pager = (vm_pager_t) vnode_pager_setup(vp, FALSE, TRUE);

	/*
	 *	Map portion that must be accessible directly into
	 *	kernel's map.
	 */
	if ((sizeof (struct mach_header) + header->sizeofcmds) > macho_size)
		return(LOAD_BADMACHO);

	/*
	 *	Round size of Mach-O commands up to page boundry.
	 */
	size = round_page(sizeof (struct mach_header) + header->sizeofcmds);
	if (size <= 0)
		return(LOAD_BADMACHO);

	/*
	 * Map the load commands into kernel memory.
	 */
	addr = 0;
	ret = vm_allocate_with_pager(kernel_map, &addr, size, TRUE, pager,
				     file_offset);
	if (ret != KERN_SUCCESS) {
		return(LOAD_NOSPACE);
	}

	/*
	 *	Scan through the commands, processing each one as necessary.
	 */
	for (pass = 1; pass <= 2; pass++) {
		offset = sizeof(struct mach_header);
		ncmds = header->ncmds;
		while (ncmds--) {
			/*
			 *	Get a pointer to the command.
			 */
			lcp = (struct load_command *)(addr + offset);
			offset += lcp->cmdsize;

			/*
			 *	Check for valid lcp pointer by checking
			 *	next offset.
			 */
			if (offset > header->sizeofcmds
					+ sizeof(struct mach_header)) {
				vm_map_remove(kernel_map, addr, addr + size);
				return(LOAD_BADMACHO);
			}

			/*
			 *	Check for valid command.
			 */
			switch(lcp->cmd) {
			case LC_SEGMENT:
				if (pass != 1)
					break;
				ret = load_segment(
					       (struct segment_command *) lcp,
						   pager, file_offset,
						   macho_size,
						   vp->v_vm_info->vnode_size,
						   map,
						   result);
				break;
			case LC_THREAD:
				if (pass != 2)
					break;
				ret = load_thread((struct thread_command *)lcp,
						  result);
				break;
			case LC_UNIXTHREAD:
				if (pass != 2)
					break;
				ret = load_unixthread(
						 (struct thread_command *) lcp,
						 result);
				break;
			case LC_LOADFVMLIB:
				if (pass != 1)
					break;
				ret = load_fvmlib((struct fvmlib_command *)lcp,
						  map, depth);
				break;
			case LC_IDFVMLIB:
				if (pass != 1)
					break;
				if (lib_version) {
					ret = load_idfvmlib(
						(struct fvmlib_command *)lcp,
						lib_version);
				}
				break;
			case LC_LOAD_DYLINKER:
				if (pass != 2)
					break;
				if (depth == 1 || dlp == 0)
					dlp = (struct dylinker_command *)lcp;
				else
					ret = LOAD_FAILURE;
				break;
			default:
				ret = KERN_SUCCESS;/* ignore other stuff */
			}
			if (ret != LOAD_SUCCESS)
				break;
		}
		if (ret != LOAD_SUCCESS)
			break;
	}
	if (ret == LOAD_SUCCESS && dlp != 0)
		ret = load_dylinker(dlp, map, depth, result);

	vm_map_remove(kernel_map, addr, addr + size);
	if ((ret == LOAD_SUCCESS) && (depth == 1) &&
				(result->thread_count == 0))
		ret = LOAD_FAILURE;
	return(ret);
}

static
load_return_t load_segment(
	struct segment_command	*scp,
	vm_pager_t		pager,
	unsigned long		pager_offset,
	unsigned long		macho_size,
	unsigned long		end_of_file,
	vm_map_t		map,
	load_result_t		*result
)
{
	int			vmsize, copyoffset, copysize;
	kern_return_t		ret;
	vm_offset_t		dest_addr, src_addr;
	vm_map_t		temp_map;

	/*
	 * Make sure what we get from the file is really ours (as specified
	 * by macho_size).
	 */
	if (scp->fileoff + scp->filesize > macho_size)
		return (LOAD_BADMACHO);

	/*
	 *	Round segment size to page size and check validity.
	 */
	vmsize = round_page(scp->vmsize);
	if (vmsize < 0)
		return(LOAD_BADMACHO);

	if (vmsize == 0)
		return(KERN_SUCCESS);

	/*
	 *	Truncate address to page boundry and check validity
	 *	by allocating the space in the map.
	 */
	dest_addr = trunc_page(scp->vmaddr);
	ret = vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t)0,
			  &dest_addr, vmsize, FALSE);

	if (ret != KERN_SUCCESS)
		return(LOAD_NOSPACE);

	/*
	 *	Map file into a temporary map.
	 */
	copysize = round_page(scp->filesize);
	copyoffset = scp->fileoff + pager_offset;

	if (copysize < 0)
		return(LOAD_BADMACHO);

	if (copysize > 0) {
		temp_map = vm_map_create(pmap_create(copysize),
				 VM_MIN_ADDRESS, VM_MIN_ADDRESS + copysize, 
				 TRUE);
		src_addr = VM_MIN_ADDRESS;
		ret = vm_allocate_with_pager(temp_map, &src_addr, copysize,
					     FALSE, pager, copyoffset);
		if (ret != KERN_SUCCESS) {
			vm_map_deallocate(temp_map);
			return(LOAD_NOSPACE);
		}

		/*
		 *	If last page is not complete, we need to zero fill
		 *	it.
		 *
		 *	OPTIMIZATION:
		 *	If we have a pointer to the vnode we are paging
		 *	from, we can optimize the zero-fill if we are at
		 *	the end-of-file because the vnode pager semantics
		 *	assure that bytes after the end-of-file will be zero.
		 *
		 *	FIXME:
		 *	Figure out if we can optimize away zeroing the end
		 *	of a Mach-O within a fat file.
		 *	
		 */
		if (copysize != scp->filesize
		    && (end_of_file == 0
			|| copyoffset + scp->filesize != end_of_file)) {
			vm_offset_t	tmp_addr;
			int		trunc_addr;

			trunc_addr = trunc_page(scp->filesize);
			/*
			 *	Allocate some space accessible to the kernel.
			 */
			tmp_addr = 0;
			ret = vm_map_find(kernel_map, VM_OBJECT_NULL,
					  (vm_offset_t)0, &tmp_addr,
					  PAGE_SIZE, TRUE);
			if (ret != KERN_SUCCESS) {
				vm_map_deallocate(temp_map);
				return(LOAD_NOSPACE);
			}
			/*
			 *	Copy last page into kernel.
			 */
			ret = vm_map_copy(kernel_map, temp_map,
				tmp_addr, PAGE_SIZE, trunc_addr,
				FALSE, FALSE);
			if (ret != KERN_SUCCESS) {
				vm_deallocate(kernel_map, tmp_addr, PAGE_SIZE);
				vm_map_deallocate(temp_map);
				return(LOAD_FAILURE);
			}
			/*
			 *	Zero appropriate bytes in copy-on-write copy.
			 */
			bzero(tmp_addr + (scp->filesize - trunc_addr),
				copysize - scp->filesize);
			/*
			 *	Copy new data back to user task map.
			 */
			ret = vm_map_copy(map, kernel_map,
				dest_addr + trunc_addr, PAGE_SIZE, tmp_addr,
				FALSE, FALSE);
			vm_deallocate(kernel_map, tmp_addr, PAGE_SIZE);
			if (ret != KERN_SUCCESS) {
				vm_map_deallocate(temp_map);
				return(LOAD_FAILURE);
			}

			/*
			 * Adjust copysize for correct copy below.
			 */
			copysize = trunc_addr;
		}

		/*
		 *	Copy the data into map.
		 */
		ret = vm_map_copy(map, temp_map, dest_addr, copysize, src_addr,
				FALSE, FALSE);
		vm_map_deallocate(temp_map);
		if (ret != KERN_SUCCESS)
			return(LOAD_FAILURE);
	}

#if	0
	/*
	 *	Prepage data XXX do this only on a flag.
	 */

	/*
	 *	Touch each page by referencing it.
	 */
	addr = dest_addr;
	while (addr < dest_addr + copysize) {
		(void) vm_fault(map, addr, VM_PROT_READ, FALSE, 0);
		addr += PAGE_SIZE;
	}
#endif	0

	/*
	 *	Set protection values. (Note: ignore errors!)
	 */

	if (scp->maxprot != VM_PROT_DEFAULT) {
		(void) vm_map_protect(map,
				      dest_addr,
				      dest_addr + vmsize,
				      scp->maxprot,
				      TRUE);
	}
	if (scp->initprot != VM_PROT_DEFAULT) {
		(void) vm_map_protect(map,
				      dest_addr,
				      dest_addr + vmsize,
				      scp->initprot,
				      FALSE);
	}
	if ( (scp->fileoff == 0) && (scp->filesize != 0) )
		result->mach_header = dest_addr;
	return(LOAD_SUCCESS);
}

static
load_return_t
load_unixthread(
	struct thread_command	*tcp,
	load_result_t		*result
)
{
	thread_t	thread = current_thread();
	load_return_t	ret;
	
	if (result->thread_count != 0)
		return (LOAD_FAILURE);
	
	ret = load_threadstack(thread,
		       (unsigned long *)(((vm_offset_t)tcp) + 
		       		sizeof(struct thread_command)),
		       tcp->cmdsize - sizeof(struct thread_command),
		       &result->user_stack);
	if (ret != LOAD_SUCCESS)
		return(ret);

	ret = load_threadentry(thread,
		       (unsigned long *)(((vm_offset_t)tcp) + 
		       		sizeof(struct thread_command)),
		       tcp->cmdsize - sizeof(struct thread_command),
		       &result->entry_point);
	if (ret != LOAD_SUCCESS)
		return(ret);

	ret = load_threadstate(thread,
		       (unsigned long *)(((vm_offset_t)tcp) + 
		       		sizeof(struct thread_command)),
		       tcp->cmdsize - sizeof(struct thread_command));
	if (ret != LOAD_SUCCESS)
		return (ret);

	result->unixproc = TRUE;
	result->thread_count++;

	return(LOAD_SUCCESS);
}

static
load_return_t
load_thread(
	struct thread_command	*tcp,
	load_result_t		*result
)
{
	thread_t	thread;
	kern_return_t	kret;
	load_return_t	lret;

	if (result->thread_count == 0)
		thread = current_thread();
	else {
		kret = thread_create(current_task(), &thread);
		if (kret != KERN_SUCCESS)
			return(LOAD_RESOURCE);
		thread_deallocate(thread);
	}

	lret = load_threadstate(thread,
		       (unsigned long *)(((vm_offset_t)tcp) + 
		       		sizeof(struct thread_command)),
		       tcp->cmdsize - sizeof(struct thread_command));
	if (lret != LOAD_SUCCESS)
		return (lret);

	if (result->thread_count == 0) {
		lret = load_threadstack(current_thread(),
				(unsigned long *)(((vm_offset_t)tcp) + 
					sizeof(struct thread_command)),
				tcp->cmdsize - sizeof(struct thread_command),
				&result->user_stack);
		if (lret != LOAD_SUCCESS)
			return(lret);

		lret = load_threadentry(current_thread(),
				(unsigned long *)(((vm_offset_t)tcp) + 
					sizeof(struct thread_command)),
				tcp->cmdsize - sizeof(struct thread_command),
				&result->entry_point);
		if (lret != LOAD_SUCCESS)
			return(lret);
	}
	/*
	 *	Resume thread now, note that this means that the thread
	 *	commands should appear after all the load commands to
	 *	be sure they don't reference anything not yet mapped.
	 */
	else
		thread_resume(thread);
		
	result->thread_count++;

	return(LOAD_SUCCESS);
}

static
load_return_t
load_threadstate(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size
)
{
	kern_return_t	ret;
	unsigned long	size;
	int		flavor;

	/*
	 *	Set the thread state.
	 */

	while (total_size > 0) {
		flavor = *ts++;
		size = *ts++;
		total_size -= (size+2)*sizeof(unsigned long);
		if (total_size < 0)
			return(LOAD_BADMACHO);
		ret = thread_setstatus(thread, flavor, ts, size);
		if (ret != KERN_SUCCESS)
			return(LOAD_FAILURE);
		ts += size;	/* ts is a (unsigned long *) */
	}
	return(LOAD_SUCCESS);
}

static
load_return_t
load_threadstack(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size,
	vm_offset_t	*user_stack
)
{
	kern_return_t	ret;
	unsigned long	size;
	int		flavor;

	/*
	 *	Set the thread state.
	 */
	*user_stack = 0;
	while (total_size > 0) {
		flavor = *ts++;
		size = *ts++;
		total_size -= (size+2)*sizeof(unsigned long);
		if (total_size < 0)
			return(LOAD_BADMACHO);
		ret = thread_userstack(thread, flavor, ts, size, user_stack);
		if (ret != KERN_SUCCESS)
			return(LOAD_FAILURE);
		ts += size;	/* ts is a (unsigned long *) */
	}
	return(LOAD_SUCCESS);
}

static
load_return_t
load_threadentry(
	thread_t	thread,
	unsigned long	*ts,
	unsigned long	total_size,
	vm_offset_t	*entry_point
)
{
	kern_return_t	ret;
	unsigned long	size;
	int		flavor;

	/*
	 *	Set the thread state.
	 */
	*entry_point = 0;
	while (total_size > 0) {
		flavor = *ts++;
		size = *ts++;
		total_size -= (size+2)*sizeof(unsigned long);
		if (total_size < 0)
			return(LOAD_BADMACHO);
		ret = thread_entrypoint(thread, flavor, ts, size, entry_point);
		if (ret != KERN_SUCCESS)
			return(LOAD_FAILURE);
		ts += size;	/* ts is a (unsigned long *) */
	}
	return(LOAD_SUCCESS);
}

static
load_return_t
load_fvmlib(
	struct fvmlib_command	*lcp,
	vm_map_t		map,
	int			depth
)
{
	char			*name;
	char			*p;
	struct vnode		*vp;
	struct mach_header	header;
	unsigned long		file_offset;
	unsigned long		macho_size;
	unsigned long		lib_version;
	load_result_t		myresult;
	kern_return_t		ret;

	name = (char *)lcp + lcp->fvmlib.name.offset;
	/*
	 *	Check for a proper null terminated string.
	 */
	p = name;
	do {
		if (p >= (char *)lcp + lcp->cmdsize)
			return(LOAD_BADMACHO);
	} while (*p++);

	ret = get_macho_vnode(name, &header, &file_offset, &macho_size, &vp);
	if (ret)
		return (ret);
		
	myresult = (load_result_t) { 0 };

	/*
	 *	Load the Mach-O.
	 */
	ret = parse_machfile(vp, map, &header,
				file_offset, macho_size,
				depth, &lib_version, &myresult);

	if ((ret == LOAD_SUCCESS) &&
	    (lib_version < lcp->fvmlib.minor_version))
		ret = LOAD_SHLIB;

	vrele(vp);
	return(ret);
}

static
load_return_t
load_idfvmlib(
	struct fvmlib_command	*lcp,
	unsigned long		*version
)
{
	*version = lcp->fvmlib.minor_version;
	return(LOAD_SUCCESS);
}

#if hppa
#include <machdep/hppa/thread.h>
extern int catch_user_alignment_traps;
#endif /* hppa */

static
load_return_t
load_dylinker(
	struct dylinker_command	*lcp,
	vm_map_t		map,
	int			depth,
	load_result_t		*result
)
{
	char			*name;
	char			*p;
	struct vnode		*vp;
	struct mach_header	header;
	unsigned long		file_offset;
	unsigned long		macho_size;
	vm_map_t		copy_map;
	load_result_t		myresult;
	kern_return_t		ret;

	name = (char *)lcp + lcp->name.offset;
	/*
	 *	Check for a proper null terminated string.
	 */
	p = name;
	do {
		if (p >= (char *)lcp + lcp->cmdsize)
			return(LOAD_BADMACHO);
	} while (*p++);

	ret = get_macho_vnode(name, &header, &file_offset, &macho_size, &vp);
	if (ret)
		return (ret);
		
	copy_map = vm_map_create(pmap_create(macho_size),
			map->min_offset, map->max_offset, TRUE);
			
	myresult = (load_result_t) { 0 };

	/*
	 *	Load the Mach-O.
	 */
	ret = parse_machfile(vp, copy_map, &header,
				file_offset, macho_size,
				depth, 0, &myresult);
	if (ret)
		goto out;

	if (copy_map->hdr.nentries > 0) {
		vm_offset_t	dyl_start, map_addr;
		vm_size_t	dyl_length;
		
		dyl_start = vm_map_first_entry(copy_map)->vme_start;
		dyl_length = vm_map_last_entry(copy_map)->vme_end - dyl_start;
		
		map_addr = dyl_start;
		if (vm_map_find(map, VM_OBJECT_NULL, 0,
				    &map_addr, dyl_length, FALSE) &&
			vm_map_find(map, VM_OBJECT_NULL, 0,
					&map_addr, dyl_length, TRUE))
			ret = LOAD_NOSPACE;
		else if (vm_map_copy(map, copy_map,
					map_addr, dyl_length, dyl_start,
					FALSE, FALSE))
			ret = LOAD_NOSPACE;
		if (map_addr != dyl_start)
			myresult.entry_point += (map_addr - dyl_start);
	}
	else
		ret = LOAD_FAILURE;

	if (ret == LOAD_SUCCESS) {		
		result->dynlinker = TRUE;
		result->entry_point = myresult.entry_point;
#if hppa
		if (!catch_user_alignment_traps)
			current_thread()->pcb->pcb_flags |= PCB_ALIGNMENT_FAULT_NO_CATCH;
#endif
	}

out:
	vm_map_deallocate(copy_map);
	
	vrele(vp);
	return (ret);
}

static
load_return_t
get_macho_vnode(
	char			*path,
	struct mach_header	*mach_header,
	unsigned long		*file_offset,
	unsigned long		*macho_size,
	struct vnode		**vpp
)
{
	struct vnode		*vp;
	struct vattr attr, *atp;
	struct nameidata nid, *ndp;
	struct proc *p = current_proc();		/* XXXX */
	boolean_t		is_fat;
	struct fat_arch		fat_arch;
	int			error;
	int resid;
	union {
		struct mach_header	mach_header;
		struct fat_header	fat_header;
		char	pad[512];
	} header;
	error = KERN_SUCCESS;
	
	ndp = &nid;
	atp = &attr;
	
	/* init the namei data to point the file user's program name */
	NDINIT(ndp, LOOKUP, FOLLOW | LOCKLEAF | SAVENAME, UIO_SYSSPACE, path, p);

	if (error = namei(ndp))
		return(error);
	
	vp = ndp->ni_vp;
	
	/* check for regular file */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}

	/* get attributes */
	if (error = VOP_GETATTR(vp, &attr, p->p_ucred, p))
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}

	if ((vp->v_mount->mnt_flag & MNT_NOSUID) || (p->p_flag & P_TRACED))
		atp->va_mode &= ~(VSUID | VSGID);

		/* check access.  for root we have to see if any exec bit on */
	if (error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p))
		goto bad1;
	if ((atp->va_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
		error = EACCES;
		goto bad1;
	}

	/* try to open it */
	if (error = VOP_OPEN(vp, FREAD, p->p_ucred, p))
		goto bad1;
#if MACH_NBC
	VOP_UNLOCK(vp, 0, p);
#endif /* MACH_NBC */
	if(error = vn_rdwr(UIO_READ, vp, (caddr_t)&header, sizeof(header), 0,
	    UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred, &resid, p))
		goto bad2;


/* XXXX WMG - we should check for a short read of the header here */
	
	if (header.mach_header.magic == MH_MAGIC)
	    is_fat = FALSE;
	else if (header.fat_header.magic == FAT_MAGIC ||
		 header.fat_header.magic == FAT_CIGAM)
	    is_fat = TRUE;
	else {
	    error = LOAD_BADMACHO;
	    goto bad2;
	}

	if (is_fat) {
		/*
		 * Look up our architecture in the fat file.
		 */
		error = fatfile_getarch(vp, (vm_offset_t)(&header.fat_header), &fat_arch);
		if (error != LOAD_SUCCESS) {
			goto bad2;
		}
		/*
		 *	Read the Mach-O header out of it
		 */
		error = vn_rdwr(UIO_READ, vp, &header.mach_header,
				sizeof(header.mach_header), fat_arch.offset,
				UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred, &resid, p);
		if (error) {
			error = LOAD_FAILURE;
			goto bad2;
		}

		/*
		 *	Is this really a Mach-O?
		 */
		if (header.mach_header.magic != MH_MAGIC) {
			error = LOAD_BADMACHO;
			goto bad2;
		}
		
		*mach_header = header.mach_header;
		*file_offset = fat_arch.offset;
		*macho_size = fat_arch.size;
		*vpp = vp;
		// leaks otherwise - A.R
		FREE_ZONE(ndp->ni_cnd.cn_pnbuf, ndp->ni_cnd.cn_pnlen, M_NAMEI);
		
 		// i_lock exclusive panics, otherwise during pageins
#if !MACH_NBC
		VOP_UNLOCK(vp, 0, p);
#endif /* !MACH_NBC */
		return (error);
	} else {
	
		*mach_header = header.mach_header;
		*file_offset = 0;
		if (vp->v_vm_info) {
			vp->v_vm_info->vnode_size = attr.va_size;
		}
		*macho_size =  attr.va_size;
		*vpp = vp;
		// leaks otherwise - A.R
		FREE_ZONE(ndp->ni_cnd.cn_pnbuf, ndp->ni_cnd.cn_pnlen, M_NAMEI);

		// i_lock exclusive panics, otherwise during pageins
#if !MACH_NBC
        VOP_UNLOCK(vp, 0, p);
#endif /* !MACH_NBC */
		return (error);
	}

bad2:
	/*
	 * unlock and close the vnode, restore the old one, free the
	 * pathname buf, and punt.
	 */
#if !MACH_NBC
	VOP_UNLOCK(vp, 0, p);
#endif /* !MACH_NBC */
	vn_close(vp, FREAD, p->p_ucred, p);
	FREE_ZONE(ndp->ni_cnd.cn_pnbuf, ndp->ni_cnd.cn_pnlen, M_NAMEI);
	return (error);
bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	FREE_ZONE(ndp->ni_cnd.cn_pnbuf, ndp->ni_cnd.cn_pnlen, M_NAMEI);
	vput(vp);
	return(error);
}
