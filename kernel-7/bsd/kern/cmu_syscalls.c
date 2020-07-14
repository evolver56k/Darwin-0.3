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
 *
 * HISTORY
 *
 * 27-Apr-97  A.Ramesh (aramesh) at Apple
 *  Added limited set for Rhapsody
 * 
 */

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/dir.h>
#import <sys/buf.h>
#import <sys/vnode.h>
#import <sys/file.h>
#import <sys/proc.h>
#import <sys/conf.h>
#import <sys/ioctl.h>
#import <sys/tty.h>
//#import <sys/vfs.h>
//#import <ufs/mount.h>
#import <sys/kernel.h>
#import <sys/table.h>

#import <mach/mach_types.h>

#import <vm/vm_user.h>
#import <vm/vm_map.h>
#import <vm/vm_kern.h>
#import <mach/vm_param.h>
#import <machine/vmparam.h>	/* only way to find the user stack (argblock) */

#import <sys/vmmeter.h>
#import <sys/socket.h>
#import <net/if.h>

#import <sys/version.h>

struct	vmmeter cnt;

/*
 *  table - get/set element(s) from system table
 *
 *  This call is intended as a general purpose mechanism for retrieving or
 *  updating individual or sequential elements of various system tables and
 *  data structures.
 *
 *  One potential future use might be to make most of the standard system
 *  tables available via this mechanism so as to permit non-privileged programs
 *  to access these common SYSTAT types of data.
 *
 *  Parameters:
 *
 *  id		= an identifer indicating the table in question
 *  index	= an index into this table specifying the starting
 *		  position at which to begin the data copy
 *  addr	= address in user space to receive/supply the data
 *  nel		= number of table elements to retrieve/update
 *  lel		= expected size of a single element of the table.  If this
 *		  is smaller than the actual size, extra data will be
 *		  truncated from the end.  If it is larger, holes will be
 *		  left between elements copied to/from the user address space.
 *
 *		  The intent of separately distinguishing these final two
 *		  arguments is to insulate user programs as much as possible
 *		  from the common change in the size of system data structures
 *		  when a new field is added.  This works so long as new fields
 *		  are added only to the end, none are removed, and all fields
 *		  remain a fixed size.
 *
 *  Returns:
 *
 *  val1	= number of elements retrieved/updated (this may be fewer than
 *		  requested if more elements are requested than exist in
 *		  the table from the specified index).
 *
 *  Note:
 *
 *  A call with lel == 0 and nel == MAXSHORT can be used to determine the
 *  length of a table (in elements) before actually requesting any of the
 *  data.
 */

#define	MAXLEL	(sizeof(long))	/* maximum element length (for set) */

struct table_args {
		int id;
		int index;
		caddr_t addr;
		int nel;	/* >0 ==> get, <0 ==> set */
		u_int lel;
};

table(p, uap, retval)
	struct proc *p;
	register struct table_args *uap;
	int *retval;
{
	caddr_t data;
	unsigned size;
	int error = 0;
	int set;
	vm_offset_t	arg_addr;
	vm_size_t	arg_size;
	int		*ip;
	struct proc	*pp;
	vm_offset_t	copy_start, copy_end;
	vm_map_t	proc_map;
	vm_offset_t	dealloc_start;	/* area to remove from kernel map */
	vm_offset_t	dealloc_end;
	struct tbl_procinfo	tp;
	struct tbl_cpuinfo	tc;
	struct tbl_loadavg	tl;
	int val1=0;
	extern long avenrun[3], mach_factor[3];

	/*
	 *  Verify that any set request is appropriate.
	 */
	set = 0;
	if (uap->nel < 0) {
		/*
		 * Give the machine dependent code a crack at this first
		 */
		switch (machine_table_setokay(uap->id)) {
		case TBL_MACHDEP_OKAY:
			goto okay;
		case TBL_MACHDEP_BAD:
			*retval = val1;
			return(EINVAL);
		case TBL_MACHDEP_NONE:
		default:
			break;
		}
		switch (uap->id) {
		default:
			*retval = val1;
			return(EINVAL);
		}
	    okay:
		set++;
		uap->nel = -(uap->nel);
	}

	val1 = 0;

	/*
	 *  Main loop for each element.
	 */

	while (uap->nel > 0) {

		dealloc_start = (vm_offset_t) 0;
		dealloc_end = (vm_offset_t) 0;

		/*
		 * Give machine dependent code a chance
		 */
		switch (machine_table(uap->id, uap->index, uap->addr,
				      uap->nel, uap->lel, set))
		{
		case TBL_MACHDEP_OKAY:
			uap->addr += uap->lel;
			uap->nel -= 1;
			uap->index += 1;
			val1 += 1;
			continue;
		case TBL_MACHDEP_NONE:
			break;
		case TBL_MACHDEP_BAD:
		default:
			goto bad;
		}

		switch (uap->id) {
		case TBL_ARGUMENTS:
			/*
			 *	Returns the top N bytes of the user stack, with
			 *	everything below the first argument character
			 *	zeroed for security reasons.
			 *	Odd data structure is for compatibility.
			 */
			/*
			 *	Lookup process by pid
			 */
			p = pfind(uap->index);
			if (p == (struct proc *)0) {
				/*
				 *	No such process
				 */
				*retval = val1;
				return(ESRCH);
			 }
			/*
			 *	Get map for process
			 */
			proc_map = ((task_t)p->task)->map;

			/*
			 *	Copy the top N bytes of the stack.
			 *	On all machines we have so far, the stack grows
			 *	downwards.
			 *
			 *	If the user expects no more than N bytes of
			 *	argument list, use that as a guess for the
			 *	size.
			 */
			if ((arg_size = uap->lel) == 0) {
				error = EINVAL;
				goto bad;
			}

			if (!p->user_stack)
				goto bad;	/* kernel only proc */
#if	STACK_GROWTH_UP
			arg_addr = p->user_stack;
#else	STACK_GROWTH_UP
			arg_addr = p->user_stack - arg_size;
#endif	/* STACK_GROWTH_UP */

			/*
			 *	Before we can block (any VM code), make another
			 *	reference to the map to keep it alive.
			 */
			vm_map_reference(proc_map);

			copy_start = kmem_alloc_wait(kernel_pageable_map,
						round_page(arg_size));

			copy_end = round_page(copy_start + arg_size);

			if (vm_map_copy(kernel_pageable_map, proc_map, copy_start,
			    round_page(arg_size), trunc_page(arg_addr),
			    FALSE, FALSE) != KERN_SUCCESS) {
				kmem_free_wakeup(kernel_pageable_map, copy_start,
					round_page(arg_size));
				vm_map_deallocate(proc_map);
				goto bad;
			}

			/*
			 *	Now that we've done the copy, we can release
			 *	the process' map.
			 */
			vm_map_deallocate(proc_map);
			
#if	STACK_GROWTH_UP
			data = (caddr_t)copy_start;
			ip = (int *) ((*(int *)copy_start) - arg_addr + data);
			/*
			 * sanity check ip since it comes from user-accessible
			 * stack area
			 */
			if (((vm_offset_t)ip > copy_end) ||
					((vm_offset_t)ip < copy_start))
				ip = (int *)copy_end;
			/*
			 * relocate so that end of string area is at end
			 * of buffer.
			 */
			size = (unsigned) ((int)ip - (int)copy_start);
			data = (caddr_t)(copy_end - size);
			bcopy(copy_start, data, size);
			/*
			 * now find beginning of string area so we can
			 * clear out data user should not see
			 */
			ip = (int *)copy_end;	// start at new end
			ip -= 2; /*skip trailing 0 word and assume at least one
				  argument.  The last word of argN may be just
				  the trailing 0, in which case we'd stop
				  there */
			while (*--ip)
				if (ip == (int *)data)
					break;			
			bzero(copy_start,
				(unsigned) ((int)ip - (int)copy_start));
			/*
			 * now prepare data/size for the copy's out of the
			 * switch.  We copy the last arg_size bytes from
			 * our data.
			 */
			size = arg_size;
			data = (caddr_t)(copy_end - size);
#else	STACK_GROWTH_UP
#if	( defined(vax) || defined(romp) )
			data = (caddr_t) (copy_end-arg_size-SIGCODE_SIZE);
			ip = (int *) (copy_end - SIGCODE_SIZE);
#else	( defined(vax) || defined(romp) )		
			data = (caddr_t) (copy_end - arg_size);
			ip = (int *) copy_end;		
#endif	/* ( defined(vax) || defined(romp) )		 */
			size = arg_size;

			/*
			 *	Now look down the stack for the bottom of the
			 *	argument list.  Since this call is otherwise
			 *	unprotected, we can't let the nosy user see
			 *	anything else on the stack.
			 *
			 *	The arguments are pushed on the stack by
			 *	execve() as:
			 *
			 *		.long	0
			 *		arg 0	(null-terminated)
			 *		arg 1
			 *		...
			 *		arg N
			 *		.long	0
			 *
			 */

			ip -= 2; /*skip trailing 0 word and assume at least one
				  argument.  The last word of argN may be just
				  the trailing 0, in which case we'd stop
				  there */
			while (*--ip)
				if (ip == (int *)data)
					break;			
			bzero(data, (unsigned) ((int)ip - (int)data));
#endif	/* STACK_GROWTH_UP */

			dealloc_start = copy_start;
			dealloc_end = copy_end;
			break;

		case TBL_PROCINFO:
		    {
			register struct proc	*p;

			/*
			 *	Index is entry number in proc table.
			 */
			/* transition, take negative numbers as pid for now */
			if (uap->index < 0)
				uap->index = -uap->index;	/* compatibility */
			p = pfind(uap->index);
			if (p == (struct proc *)0) {
				/*
				 *	No such process
				 */
				return(ESRCH);
			}
			if (p->p_stat == 0) {
			    bzero((caddr_t)&tp, sizeof(tp));
			    tp.pi_status = PI_EMPTY;
			}
			else {
			    tp.pi_uid	= p->p_ucred->cr_uid;
			    tp.pi_pid	= p->p_pid;

				if (p->p_pptr)
			   	 tp.pi_ppid	= p->p_pptr->p_pid;
				else
			   	 tp.pi_ppid	= 0;
				if (p->p_pgrp)
			    	  tp.pi_pgrp	= p->p_pgrp->pg_id;
				else
			    	  tp.pi_pgrp	= 0;
			    tp.pi_flag	= p->p_flag;

			    if (p->task == TASK_NULL) {
				tp.pi_status = PI_ZOMBIE;
			    }
			    else {
if (p->p_pgrp && p->p_pgrp->pg_session && p->p_pgrp->pg_session->s_ttyp )
	tp.pi_ttyd = p->p_pgrp->pg_session->s_ttyp->t_dev;
else
	tp.pi_ttyd = -1;
				bcopy(&p->p_comm[0], tp.pi_comm,
				      MAXCOMLEN);
				tp.pi_comm[MAXCOMLEN] = '\0';

				if (p->p_flag & P_WEXIT)
				    tp.pi_status = PI_EXITING;
				else
				    tp.pi_status = PI_ACTIVE;
			    }
			}

			data = (caddr_t)&tp;
			size = sizeof(tp);
			break;
		    }

		case TBL_CPUINFO:
			if (uap->index != 0 || uap->nel != 1)
				goto bad;
			tc.ci_swtch = cnt.v_swtch;
			tc.ci_intr = cnt.v_intr;
			tc.ci_syscall = cnt.v_syscall;
			tc.ci_traps = cnt.v_trap;
			tc.ci_hz = hz;
			tc.ci_phz = 0;
			bcopy(cp_time, tc.ci_cptime, sizeof(cp_time));
			data = (caddr_t)&tc;
			size = sizeof (tc);
			break;
		case TBL_LOADAVG:
			if (uap->index != 0 || uap->nel != 1)
				goto bad;
			bcopy((caddr_t)&avenrun[0], (caddr_t)&tl.tl_avenrun[0],
					sizeof(tl.tl_avenrun));
			tl.tl_lscale = LSCALE;
			data = (caddr_t)&tl;
			size = sizeof (tl);
			break;
		case TBL_MACHFACTOR:
			if (uap->index != 0 || uap->nel != 1)
				goto bad;
			bcopy((caddr_t)&mach_factor[0],
					(caddr_t)&tl.tl_avenrun[0],
					sizeof(tl.tl_avenrun));
			tl.tl_lscale = LSCALE;
			data = (caddr_t)&tl;
			size = sizeof (tl);
			break;
		default:
		bad:
			/*
			 *	Return error only if all indices
			 *	are invalid.
			 */
			if (val1 == 0)
				error = EINVAL;
			*retval = val1;
			return(error);
		}
		/*
		 * This code should be generalized if/when other tables
		 * are added to handle single element copies where the
		 * actual and expected sizes differ or the table entries
		 * are not contiguous in kernel memory (as with TTYLOC)
		 * and also efficiently copy multiple element
		 * tables when contiguous and the sizes match.
		 */
		size = MIN(size, uap->lel);
		if (size) {
			if (set) {
				char buff[MAXLEL];

			        error = copyin(uap->addr, buff, size);
				if (error == 0)
					bcopy(buff, data, size);
			}
			else {
				error = copyout(data, uap->addr, size);
			}
		}
		if (dealloc_start != (vm_offset_t) 0) {
			kmem_free_wakeup(kernel_pageable_map, dealloc_start,
				dealloc_end - dealloc_start);
		}
		if (error) {
			*retval = val1;
			return(error);
		}
		uap->addr += uap->lel;
		uap->nel -= 1;
		uap->index += 1;
		val1 += 1;
	}
	*retval = val1;
	return(0);
}

