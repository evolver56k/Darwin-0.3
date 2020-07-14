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

/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	bsd/kern/kern_core.c
 *
 *	This file contains machine independent code for performing core dumps.
 *
 * HISTORY
 * 16-Feb-91  Mike DeMoney (mike@next.com)
 *	Massaged into MI form from m68k/core.c.
 */
#import <cputypes.h>
#import <mach_host.h>

#import <mach/vm_param.h>
#import <mach/thread_status.h>

#import <machine/spl.h>
#import <machine/cpu.h>
#import <machine/reg.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>

#import <mach-o/loader.h>

#import <kern/sched.h>
#import <kern/thread.h>
#import <kern/parallel.h>
#import <kern/sched_prim.h>
#import <kern/task.h>
#import <vm/vm_kern.h>

/*
 * Create a core image on the file "core".
 */
#define	MAX_TSTATE_FLAVORS	10
int
coredump(p)
	register struct proc *p;
{
	register struct vnode	*vp;
	register struct pcred *pcred = p->p_cred;
	register struct ucred *cred = pcred->pc_ucred;
	struct nameidata nd;
	struct vattr	vattr;
	vm_map_t	map;
	int		thread_count, segment_count;
	int		command_size, header_size, tstate_size;
	int		hoffset, foffset, vmoffset;
	vm_offset_t	header;
	struct machine_slot	*ms;
	struct mach_header	*mh;
	struct segment_command	*sc;
	struct thread_command	*tc;
	vm_size_t	size;
	vm_prot_t	prot;
	vm_prot_t	maxprot;
	vm_inherit_t	inherit;
	boolean_t	is_shared;
	port_t		name;
	vm_offset_t	offset;
	int		error, error1;
	task_t		task;
	thread_t	thread;
	char		core_name[MAXCOMLEN+6];
	struct thread_state_flavor flavors[MAX_TSTATE_FLAVORS];
	vm_size_t	nflavors;
	int		i;


	if (pcred->p_svuid != pcred->p_ruid || pcred->p_svgid != pcred->p_rgid)
		return (EFAULT);

	task = current_task();
	map = task->map;
	if (map->size >=  p->p_rlimit[RLIMIT_CORE].rlim_cur)
		return (EFAULT);
	(void) task_halt(task);	/* stop this task, except for current thread */
	/*
	 *	Make sure all registers, etc. are in pcb so they get
	 *	into core file.
	 */
	pcb_synch(current_thread());
	sprintf(core_name, "/cores/core.%d", p->p_pid);
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, core_name, p);
	if(error = vn_open(&nd, O_CREAT | FWRITE, S_IRUSR ))
		return (error);
	vp = nd.ni_vp;
	
	/* Don't dump to non-regular files or files with links. */
	if (vp->v_type != VREG ||
	    VOP_GETATTR(vp, &vattr, cred, p) || vattr.va_nlink != 1) {
		error = EFAULT;
		goto out;
	}

	VATTR_NULL(&vattr);
	vattr.va_size = 0;
	VOP_LEASE(vp, p, cred, LEASE_WRITE);
	VOP_SETATTR(vp, &vattr, cred, p);
	p->p_acflag |= ACORE;

	/*
	 *	If the task is modified while dumping the file
	 *	(e.g., changes in threads or VM, the resulting
	 *	file will not necessarily be correct.
	 */

	thread_count = task->thread_count;
	segment_count = map->hdr.nentries;	/* XXX */
	/*
	 * nflavors here is really the number of ints in flavors
	 * to meet the thread_getstatus() calling convention
	 */
	nflavors = sizeof(flavors)/sizeof(int);
	if (thread_getstatus(current_thread(), THREAD_STATE_FLAVOR_LIST,
				(thread_state_t)(flavors),
				 &nflavors) != KERN_SUCCESS)
	    panic("core flavor list");
	/* now convert to number of flavors */
	nflavors /= sizeof(struct thread_state_flavor)/sizeof(int);

	tstate_size = 0;
	for (i = 0; i < nflavors; i++)
		tstate_size += sizeof(struct thread_state_flavor) +
		  (flavors[i].count * sizeof(int));

	command_size = segment_count*sizeof(struct segment_command) +
	  thread_count*sizeof(struct thread_command) +
	  tstate_size*thread_count;

	header_size = command_size + sizeof(struct mach_header);

	(void) kmem_alloc_wired(kernel_map,
				    (vm_offset_t *)&header,
				    (vm_size_t)header_size);

	/*
	 *	Set up Mach-O header.
	 */
	mh = (struct mach_header *) header;
	ms = &machine_slot[cpu_number()];
	mh->magic = MH_MAGIC;
	mh->cputype = ms->cpu_type;
	mh->cpusubtype = ms->cpu_subtype;
	mh->filetype = MH_CORE;
	mh->ncmds = segment_count + thread_count;
	mh->sizeofcmds = command_size;

	hoffset = sizeof(struct mach_header);	/* offset into header */
	foffset = round_page(header_size);	/* offset into file */
	vmoffset = VM_MIN_ADDRESS;		/* offset into VM */
	/* We use to check for an error, here, now we try and get 
	 * as much as we can
	 */
	while (segment_count > 0){
		/*
		 *	Get region information for next region.
		 */
		if (vm_region(map, &vmoffset, &size, &prot, &maxprot,
			  &inherit, &is_shared, &name, &offset)
					== KERN_NO_SPACE)
			break;

		/*
		 *	Fill in segment command structure.
		 */
		sc = (struct segment_command *) (header + hoffset);
		sc->cmd = LC_SEGMENT;
		sc->cmdsize = sizeof(struct segment_command);
		/* segment name is zerod by kmem_alloc */
		sc->vmaddr = vmoffset;
		sc->vmsize = size;
		sc->fileoff = foffset;
		sc->filesize = size;
		sc->maxprot = maxprot;
		sc->initprot = prot;
		sc->nsects = 0;

		/*
		 *	Write segment out.  Try as hard as possible to
		 *	get read access to the data.
		 */
		if ((prot & VM_PROT_READ) == 0) {
			vm_protect(map, vmoffset, size, FALSE,
				   prot|VM_PROT_READ);
		}
		/*
		 *	Only actually perform write if we can read.
		 *	Note: if we can't read, then we end up with
		 *	a hole in the file.
		 */
		if ((maxprot & VM_PROT_READ) == VM_PROT_READ) {
			error = vn_rdwr(UIO_WRITE, vp, (caddr_t)vmoffset, size, foffset,
				UIO_USERSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) 0, p);
		}

		hoffset += sizeof(struct segment_command);
		foffset += size;
		vmoffset += size;
		segment_count--;
	}
	task_lock(task);
	thread = (thread_t) queue_first(&task->thread_list);
	while (thread_count > 0) {
		/*
		 *	Fill in thread command structure.
		 */
		tc = (struct thread_command *) (header + hoffset);
		tc->cmd = LC_THREAD;
		tc->cmdsize = sizeof(struct thread_command)
				+ tstate_size;
		hoffset += sizeof(struct thread_command);
		/*
		 * Follow with a struct thread_state_flavor and
		 * the appropriate thread state struct for each
		 * thread state flavor.
		 */
		for (i = 0; i < nflavors; i++) {
			*(struct thread_state_flavor *)(header+hoffset) =
			  flavors[i];
			hoffset += sizeof(struct thread_state_flavor);
			thread_getstatus(thread, flavors[i].flavor,
					(thread_state_t *)(header+hoffset),
					&flavors[i].count);
			hoffset += flavors[i].count*sizeof(int);
		}
		thread = (thread_t) queue_next(&thread->thread_list);
		thread_count--;
	}
	task_unlock(task);

	/*
	 *	Write out the Mach header at the beginning of the
	 *	file.
	 */
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)header, header_size, (off_t)0,
			UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) 0, p);
	kmem_free(kernel_map, header, header_size);
out:
	VOP_UNLOCK(vp, 0, p);
	error1 = vn_close(vp, FWRITE, cred, p);
	if (error == 0)
		error = error1;
	return(error);
}
