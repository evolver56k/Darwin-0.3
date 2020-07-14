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
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)init_main.c	8.16 (Berkeley) 5/14/95
 */

/* 
 *
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * 02-Jul-97  Umesh Vaishampayan at Apple
 *	Cleanup.
 *
 * 06-Jan-93  Mac Gillon at NeXT
 *  	POSIX support
 *
 * 20-Apr-90  Doug Mitchell at NeXT
 *	Started up reaper_thread() before mounting root.
 *
 * 19-Mar-90  Gregg Kellogg (gk) at NeXT
 *	NeXT doesn't use schedcpu.
 *	Move kallocinit() to vm/vm_init.c
 *
 * 14-Feb-90  Gregg Kellogg (gk) at NeXT
 *	Changes for new scheduler:
 *		Initialize scheduler
 *		use newproc() to start ux_handler.
 *		Remove process lock initialization.
 *		Remove obsolete service_timers() kickoff
 *		do callout_lock initialization in kern_synch.c/rqinit.
 *
 * 17-Jan-90  Morris Meyer (mmeyer) at NeXT
 *	NFS 4.0 Changes: Minimal cleanup.  Removed ihinit.
 *
 * 07-Nov-88  Avadis Tevanian (avie) at NeXT
 *	Removed code to use transparent addresses since it only works
 *	when we have fully populated buffers (which is a bad assumption).
 *
 * 13-Aug-88  Avadis Tevanian (avie) at NeXT
 *	Removed dependencies on proc table.
 *
 *  4-May-88  David Black (dlb) at Carnegie-Mellon University
 *	MACH_TIME_NEW is now standard.
 *
 * 21-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	Set kernel_only for kernel task.
 *
 * 10-Apr-88  John Seamons (jks) at NeXT
 *	NeXT: Improve TLB performance for buffers by using the
 *	transparently translated virtual address for single page sized bufs.
 *
 * 07-Apr-88  John Seamons (jks) at NeXT
 *	NeXT: reduced the allocation size of various zones to save space.
 *
 *  3-Apr-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Force the vm_map for the inode_ and device_ pager tasks to
 *	be the kernel map.
 *
 * 25-Jan-88  Richard Sanzi (sanzi) at Carnegie-Mellon University
 *	Moved float_init() call to configure() in autoconf.c
 *
 * 21-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Neither task_create nor thread_create return the data port
 *	any longer.
 *
 * 29-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Removed code to shuffle initial processes for idle threads;
 *	MACH doesn't need to make extra processes for them.
 *	Delinted.
 *
 * 12-Dec-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added device_pager startup.  Moved setting of ipc_kernel and
 *	kernel_only flags here.
 *
 *  9-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Follow thread_terminate with thread_halt_self for new thread
 *	termination logic; extra reference no longer necessary.
 *
 *  9-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Grab extra reference to first thread before terminating it.
 *
 *  4-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Name changes for exc interface.  set ipc_kernel in first thread
 *	for paranoia purposes.
 *
 * 19-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Eliminated MACH conditionals, purged history.
 *
 *  5-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	start up network service thread.
 *
 *  9-Sep-87  Peter King (king) at NeXT
 *	SUN_VFS:  Add a call to vfs_init() in setup_main().
 *
 * 17-Aug-87  Peter King (king) at NeXT
 *	SUN_VFS:  Support credentials record in u.
 *	      Convert to Sun quota system.
 *	      Add call to dnlc_init().  Remove nchinit() call.
 *	      Call swapconf() in binit().
 */
 
#import <mach_xp.h>
#import <quota.h>
#import <cpus.h>
#import <cputypes.h>
#import <mach_old_vm_copy.h>


#import <sys/param.h>
#import <sys/filedesc.h>
#import <sys/kernel.h>
#import <sys/mount.h>
#import <sys/proc.h>
#import <sys/systm.h>
#import <sys/vnode.h>
#import <sys/conf.h>
#import <sys/buf.h>
#import <sys/clist.h>
#import <sys/user.h>

#import <ufs/ufs/quota.h>

#import <machine/cpu.h>

#import <sys/malloc.h>
#import <sys/dkstat.h>
#import <machine/reg.h>
#import <machine/spl.h>

#import <kern/thread.h>
#import <kern/task.h>
#import <mach/machine.h>
#import <kern/timer.h>
#import <sys/version.h>
#import <machdep/machine/pmap.h>
#import <mach/vm_param.h>
#import <vm/vm_page.h>
#import <vm/vm_map.h>
#import <vm/vm_kern.h>
#import <vm/vm_object.h>
#import <mach/boolean.h>
#import <kern/sched_prim.h>
#import <kern/zalloc.h>
#if	MACH_XP
#import <vm/vnode_pager.h>
#import <vm/device_pager.h>
#endif	/* MACH_XP */

#import <mach/task_special_ports.h>
#import <sys/ux_exception.h>
#import <sys/reboot.h>
#import "kernobjc.h"
#import "driverkit.h"

#if	NeXT
#import <kern/power.h>
#import <kern/parallel.h>
#endif	/* NeXT */

char    copyright[] =
"Copyright (c) 1982, 1986, 1989, 1991, 1993\n\tThe Regents of the University of California.  All rights reserved.\n\n";

extern void	ux_handler();

/* Components of the first process -- never freed. */
struct	proc proc0;
struct	session session0;
struct	pgrp pgrp0;
struct	pcred cred0;
struct	filedesc filedesc0;
struct	plimit limit0;
struct	pstats pstats0;
struct	sigacts sigacts0;
struct	proc *kernproc, *initproc;


long cp_time[CPUSTATES];
long dk_seek[DK_NDRIVE];
long dk_time[DK_NDRIVE];
long dk_wds[DK_NDRIVE];
long dk_wpms[DK_NDRIVE];
long dk_xfer[DK_NDRIVE];
long dk_bps[DK_NDRIVE];

int dk_busy;
int dk_ndrive;

long tk_cancc;
long tk_nin;
long tk_nout;
long tk_rawcc;

#if	NeXT
thread_t	pageoutThread;
/* Global variables to make pstat happy. We do swapping differently */
int nswdev, nswap;
int nswapmap;
void *swapmap;
struct swdevt swdevt[1];
#endif	/* NeXT */

dev_t	rootdev;		/* device of the root */
dev_t	dumpdev;		/* device to take dumps on */
long	dumplo;			/* offset into dumpdev */
extern int	show_space;
long	hostid;
char	hostname[MAXHOSTNAMELEN];
int	hostnamelen;
char	domainname[MAXDOMNAMELEN];
int	domainnamelen;

struct	timeval boottime;		/* GRODY!  This has to go... */
struct	timeval time;

#ifdef  KMEMSTATS
struct	kmemstats kmemstats[M_LAST];
#endif

int	lbolt;				/* awoken once a second */
struct	vnode *rootvp;

vm_map_t	kernel_pageable_map;
vm_map_t	mb_map;

int	cmask = CMASK;
/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 *	     - process 2 to page out
 */

/*
 *	Sets the name for the given task.
 */
void task_name(s, p)
	char		*s;
	struct proc *p;
{
	int		length = strlen(s);

	bcopy(s, p->p_comm,
		length >= sizeof(p->p_comm) ? sizeof(p->p_comm) :
			length + 1);
}

#if	KERNOBJC
static int kernDefaultClassHandler(const char *className)
{
	return 0;
}
#endif	/* KERNOBJC */

/* To allow these values to be patched, they're globals here */
#import <machine/vmparam.h>
struct rlimit vm_initial_limit_stack = { DFLSSIZ, MAXSSIZ };
struct rlimit vm_initial_limit_data = { DFLDSIZ, MAXDSIZ };
struct rlimit vm_initial_limit_core = { DFLCSIZ, MAXCSIZ };

extern thread_t first_thread;

#define SPL_DEBUG	0
#if	SPL_DEBUG
#define	dprintf(x)	printf x
#else	SPL_DEBUG
#define dprintf(x)
#endif	/* SPL_DEBUG */

void
main()
{
	register struct proc *p;
	extern struct ucred *rootcred;
	register int i;
	int s;
	thread_t	th;
	extern void	idle_thread(), init_task(), vm_pageout();
	extern void	reaper_thread(), swapin_thread();
	extern void	netisr_thread(), sched_thread();
#if	NeXT
	extern int	power_callout(void *, void *);
#endif	/* NeXT */

#if PRELOAD
	extern void	prepagein_thread();
#endif
#if	NCPUS > 1
	extern void	 action_thread();
#endif	/* NCPUS > 1 */
#if	MACH_XP
	thread_t	inode_th;
	thread_t	device_th;
#endif	/* MACH_XP */
	void		lightning_bolt(void *, void *);
	extern thread_t	cloneproc();

	extern int (*mountroot) __P((void));

	dprintf(("main entry: curspl=0x%x\n", curspl()));

	printf(copyright);

	kmeminit();

	/*
	 * Initialize process and pgrp structures.
	 */
	procinit();

	kernproc = &proc0;

	p = kernproc;
	kernel_task->proc = kernproc;
	p->p_pid = 0;
	
	/*
	 * Create process 0.
	 */
	LIST_INSERT_HEAD(&allproc, p, p_list);
	p->p_pgrp = &pgrp0;
	LIST_INSERT_HEAD(PGRPHASH(0), &pgrp0, pg_hash);
	LIST_INIT(&pgrp0.pg_members);
	LIST_INSERT_HEAD(&pgrp0.pg_members, p, p_pglist);

	pgrp0.pg_session = &session0;
	session0.s_count = 1;
	session0.s_leader = p;

	p->task = kernel_task;
	/*
	 *	Now in thread context, switch to thread timer.
	 */
	s = splhigh();
	dprintf(("main splhigh: curspl=0x%x\n", curspl()));
	timer_switch(&current_thread()->system_timer);
	splx(s);
	dprintf(("main splx: curspl=0x%x\n", curspl()));
	
	thread_call_init();
	
	p->p_stat = SRUN;
	p->p_flag = P_INMEM|P_SYSTEM;
	p->p_nice = NZERO;
	p->p_pptr = p;
	simple_lock_init(&p->siglock);
	p->sigwait = FALSE;
	p->sigwait_thread = THREAD_NULL;
	p->exit_thread = THREAD_NULL;

	/* Create credentials. */
	lockinit(&cred0.pc_lock, PLOCK, "proc0 cred", 0, 0);
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	p->p_ucred = crget();
	p->p_ucred->cr_ngroups = 1;	/* group 0 */

	/* Create the file descriptor table. */
	filedesc0.fd_refcnt = 1+1; /* +1 so shutdown will not _FREE_ZONE */
	p->p_fd = &filedesc0;
	filedesc0.fd_cmask = cmask;

	/* Create the limits structures. */
	p->p_limit = &limit0;
	for (i = 0; i < sizeof(p->p_rlimit)/sizeof(p->p_rlimit[0]); i++)
		limit0.pl_rlimit[i].rlim_cur = 
			limit0.pl_rlimit[i].rlim_max = RLIM_INFINITY;
	limit0.pl_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE;
	limit0.pl_rlimit[RLIMIT_NPROC].rlim_cur = MAXUPRC;
	limit0.pl_rlimit[RLIMIT_STACK] = vm_initial_limit_stack;
	limit0.pl_rlimit[RLIMIT_DATA] = vm_initial_limit_data;
	limit0.pl_rlimit[RLIMIT_CORE] = vm_initial_limit_core;
	limit0.p_refcnt = 1;

	p->p_stats = &pstats0;
	p->p_sigacts = &sigacts0;

	/*
	 * Charge root for one process.
	 */
	(void)chgproccnt(0, 1);
	
	/*
	 *	Allocate a kernel submap for pageable memory
	 *	for temporary copying (table(), execve()).
	 */
	{
	    vm_offset_t	min, max;

	    kernel_pageable_map = kmem_suballoc(kernel_map,
						&min, &max,
						512*1024,
						TRUE);
#if	MACH_OLD_VM_COPY
#else	MACH_OLD_VM_COPY
	    kernel_pageable_map->wait_for_space = TRUE;
#endif	/* MACH_OLD_VM_COPY */
	}

	hardclock_init();

	mapfs_init();

	/* Initialize the file systems. */
	vfsinit();

	/* Initialize mbuf's. */
	mbinit();

	/* Initialize syslog */
	log_init();

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splimp();
	ifinit();
	socketinit();
	domaininit();
	splx(s);

	/*
	 *	Create kernel idle cpu processes.  This must be done
 	 *	before a context switch can occur (and hence I/O can
	 *	happen in the binit() call).
	 */
	p->p_fd->fd_cdir = NULL;
	p->p_fd->fd_rdir = NULL;

	for (i = 0; i < NCPUS; i++) {
		if (machine_slot[i].is_cpu == FALSE)
			continue;
		(void) thread_create(kernel_task, &th);
		thread_bind(th, cpu_to_processor(i));
		thread_start(th, idle_thread);
		thread_doswapin(th);
		(void) thread_resume(th);
	}

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* kick off timeout driven events by calling first time */
	lightning_bolt(0, 0);

	/*
	 * Start up netisr thread now in case we are doing an nfs_mountroot.
	 */
	(void) kernel_thread(kernel_task, reaper_thread, (void *)0);
	(void) kernel_thread(kernel_task, swapin_thread, (void *)0);
	(void) kernel_thread(kernel_task, sched_thread, (void *)0);
#if	NCPUS > 1
	(void) kernel_thread(kernel_task, action_thread, (void *)0);
#endif	/* NCPUS > 1 */
	(void) kernel_thread(kernel_task, netisr_thread, (void *)0);

#if	KERNOBJC
	_objcInit();
	objc_setClassHandler(kernDefaultClassHandler);
#endif	/* KERNOBJC */

#ifdef  DRIVERKIT
	/* Spin cursor in a fashion vaguely similar to 68k boot ROM.
	 * Eventually this will be the same on all architectures.
	 */
	kmEnableAnimation();
#if	defined(ppc)
	/* this is temporary - until driver kit works */
#warning this is temporary too
	bsd_autoconf();
#endif

	autoconf();

#elif	/* DRIVERKIT */
#warning this is temporary
	/* this is temporary - until driver kit works */
	bsd_autoconf();
#endif /* DRIVERKIT */

	/*
	 * We attach the loopback interface *way* down here to ensure
	 * it happens after autoconf(), otherwise it becomes the
	 * "primary" interface.
	 */
#import <loop.h>
#if NLOOP > 0
	loopattach();			/* XXX */
#endif

	/* Mount the root file system. */
	while( TRUE) {
	    int err;

            setconf();
            if (0 == (err = vfs_mountroot()))
		break;
            printf("cannot mount root, errno = %d\n", err);
	    boothowto |= RB_ASKNAME;
	}

	mountlist.cqh_first->mnt_flag |= MNT_ROOTFS;

	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	if (VFS_ROOT(mountlist.cqh_first, &rootvnode))
		panic("cannot find root vnode");
	filedesc0.fd_cdir = rootvnode;
	VREF(rootvnode);
	VOP_UNLOCK(rootvnode, 0, p);
	
	/*
	 * Now can look at time, having had a chance to verify the time
	 * from the file system.  Reset p->p_rtime as it may have been
	 * munched in mi_switch() after the time got set.
	 */
	p->p_stats->p_start = boottime = time;
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;

	/* Initialize signal state for process 0. */
	siginit(p);

	/*
	 * make init process
	 */

	th = cloneproc(kernproc);
	th->task->kernel_privilege = FALSE;	/* XXX cleaner way to do this?
							*/
#if	NeXT
	initproc = pfind(1);			/* now that it is set */
#endif	/* NeXT */
	/*
	 *	After calling start_init,
	 *	machine-dependent code must
	 *	set up stack as though a system
	 *	call trap occurred, then call
	 *	load_init_program.
	 */

	/*
	 *	Start up unix exception server
	 */
	ux_handler_init();
	port_reference(ux_exception_port);
	(void) task_set_exception_port(th->task, ux_exception_port);

	thread_start(th, init_task);
	(void) thread_resume(th);

	/*
	 *	Kernel daemon threads that don't need their own tasks
	 */

	/*
	 *	Initialize power management
	 */
	power_init();

	pageoutThread = kernel_thread(kernel_task, vm_pageout, (void *)0);
	
	/*
	 * 	vol driver and notification server startup
	 */
#if	NeXT
	vol_start_thread();
	pnotify_start();
#endif	/* NeXT */

#if	NeXT && NCPUS > 1
	/*
	 * The CMU VAX code does this from the start_init() code,
	 * but that seems like an "unusual" place to do it.  (Like
	 * I didn't look there right away :-).  Doing this here isn't
	 * much better, but it is one of the places I looked.  This
	 * can't be done until the idle threads are created for each
	 * cpu (see about 50 lines above).
	 */
	start_other_cpus();
#endif	/* NeXT && NCPUS > 1 */
	
	task_name("kernel idle", p);
	(void) thread_terminate(current_thread());
	thread_halt_self();
	/*NOTREACHED*/
}

void
init_task()
{
	struct proc *p = current_proc();

	task_name("init", p);

	current_thread()->_uthread->uu_ar0 = (void *)USER_REGS(current_thread());
    
	load_init_program(p);
	
	thread_exception_return();
	/*NOTREACHED*/
}

void
lightning_bolt(
    thread_call_spec_t	argument,
    thread_call_t	callout
)
{			
	thread_wakeup(&lbolt);

	if (!callout)
		callout = thread_call_allocate(lightning_bolt, 0);
	
	thread_call_enter_delayed(callout,
 		deadline_from_interval((tvalspec_t) { 1, 0 } ));
}

