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

/*	HISTORY

	1997/05/16	Rene Vega -- cleanup sync/isync usage.
*/

#import <machdep/ppc/exception.h>
#import <mach/mach_types.h>
#import <kern/mach_param.h>
#import <kern/thread.h>
#import <kern/kernel_stack.h>
#import <machdep/ppc/thread.h>
#import <bsd/ppc/psl.h>
#import <sys/time.h>
#import <bsd/ppc/vmparam.h>
#import <kernserv/ppc/spl.h>
#import <sys/param.h>
#import <sys/proc.h>
#import <kern/parallel.h>
#include <machdep/ppc/proc_reg.h>
#include <ppc/trap.h>
#if	KDEBUG
#include <kern/kdebug.h>
#include <mach_counters.h>
#include <kern/counters.h>
#import <machdep/ppc/frame.h>
#endif
#import <machdep/ppc/asm.h>
#import <assym.h>

pcb_t active_pcbs[NCPUS];	/* PCB belonging to the active thread */

#if DEBUG
int	fpu_trap_count = 0;
int	fpu_switch_count = 0;
#endif

extern struct per_proc_info per_proc_info[];

#define current_pcb() active_pcbs[cpu_number()]

zone_t          pcb_zone;

/*
 * Initialize pcb allocation zone.
 */
void
pcb_module_init(void)
{
	int i;
    pcb_zone = zinit(
                    sizeof (struct pcb),
                    THREAD_MAX * sizeof (struct pcb),
                    THREAD_CHUNK * sizeof (struct pcb),
                    FALSE, "pcb");

	for (i=0; i < NCPUS; i++) {
		active_pcbs[i] = 0;
	}
}

/*
 * Allocated and initialize a pcb for a new thread.
 */
void pcb_init(thread_t  thread)
{
        struct pcb      *pcb = (void *)zalloc(pcb_zone);
	pmap_t pmap = thread->task->map->pmap;


        thread->pcb = pcb;

	/* all fields default to zero */
        bzero((caddr_t)pcb, sizeof (struct pcb));

	/*
	 * User threads will pull their context from the pcb when first
	 * returning to user mode, so fill in all the necessary values.
	 * Kernel threads are initialized from the save state structure 
	 * at the base of the kernel stack (see stack_attach()).
	 */

	pcb->ss.srr1 = MSR_EXPORT_MASK_SET;

	pcb->sr0     = SEG_REG_PROT | (pmap->space<<4);
	pcb->ss.sr_copyin    = SEG_REG_PROT | SR_COPYIN + (pmap->space<<4);
}

/*
 * Release machine dependent resources on
 * thread termination.
 */
pcb_terminate(
    thread_t    thread
)
{
	struct pcb	*pcb = thread->pcb;


	if (per_proc_info[cpu_number()].fpu_pcb == pcb)
	{
		per_proc_info[cpu_number()].fpu_pcb = (pcb_t)0;
	}

        thread->pcb = 0;
        zfree(pcb_zone, pcb);
}

#define KF_SIZE		(FM_SIZE+ARG_SIZE+FM_REDZONE)

/*
 * stack_attach: Attach a kernel stack to a thread.
 */
void stack_attach(thread, stack, continuation)
        register thread_t       thread;
        vm_offset_t             stack;
        void                    (*continuation)();
{
	struct ppc_kernel_state *kss;
        struct pcb      *pcb = thread->pcb;

#if KDEBUG
	if (continuation) {
		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_STACK_ATTACH),
			thread, thread->priority,
			thread->sched_pri, continuation, 
			0);
	}
#endif
	thread->kernel_stack = stack;
	kss = STACK_IKS(stack);

	/*
	 * Build a kernel state area + arg frame on the stack for the initial
	 * switch into the thread. We also store a zero into the kernel
	 * stack pointer so the trap code knows there is already a frame
	 * on the kernel stack.
	 */

	kss->lr = (unsigned int) continuation;
	kss->r1 = (vm_offset_t) ((int)kss - KF_SIZE);

	*((int*)kss->r1) = 0;       /* Zero the frame backpointer */

        pcb->ksp = 0;
}

vm_offset_t
stack_detach(
    thread_t            thread
)
{
    vm_offset_t         stack;

    stack = thread->kernel_stack;
    thread->kernel_stack = 0;

    return (stack);
}

/*
 * stack_handoff: Move the current threads kernel stack to the new thread.
 */
void
stack_handoff(
    thread_t		old,
    thread_t		new
)
{
    vm_offset_t		stack = stack_detach(old);

#if KDEBUG
   KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_STACK_HANDOFF),
	old, new, 
	old->priority, old->sched_pri, 
	new->sched_pri);
#endif

    stack_attach(new, stack, 0);

    /*
     * Change software state.
     */
    if (new->task != old->task) {
      	int		mycpu = cpu_number();

    	PMAP_DEACTIVATE(vm_map_pmap(old->task->map), old, mycpu);
	PMAP_ACTIVATE(vm_map_pmap(new->task->map), new, mycpu);
	pmap_switch(new->task->map->pmap);
    }

    // current_thread() = new; // can't use this because it may be inlined!
    cpu_data[cpu_number()].active_thread = new;
    current_pcb() = new->pcb;
    cpu_data[cpu_number()].flags = new->pcb->flags;

#if NCPUS > 1
	/* There is no free lunch!
	 * save the floating point state for the old thread
	 * if the fpu has been used since the last context switch
	 */
	fp_state_save(old);
	
#endif /* NCPUS > 1 */

	new->pcb->ksp = 0;

}

void
call_continuation(
    void            (*continuation)(void)
)
{
	struct ppc_kernel_state *kss;
    extern Call_continuation();
	int tkss;


#if KDEBUG
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALL_CONT),
			current_thread(), current_thread()->priority, 
			current_thread()->sched_pri, continuation, 
			0);
#endif

	kss = STACK_IKS(current_thread()->kernel_stack);

	*((int*)((int)kss - KF_SIZE)) = 0;       /* Zero the frame backpointer */
	tkss = (int)kss - KF_SIZE;

    Call_continuation(continuation, tkss);
}


/*
 * switch_context: Switch from one thread to another.
 */
thread_t
switch_context(
    thread_t		old,
    void		(*continuation)(void),
    thread_t		new
)
{
    thread_t ret_thread;
    extern thread_t Switch_context();
#if KDEBUG
    struct linkage_area *frame_ptr;
    uint i;
    uint lr[4];
#endif

    if (new->task != old->task) {
	int		mycpu = cpu_number();

    	PMAP_DEACTIVATE(vm_map_pmap(old->task->map), old, mycpu);
	PMAP_ACTIVATE(vm_map_pmap(new->task->map), new, mycpu);

	pmap_switch(new->task->map->pmap);
    }
	
    // current_thread() = new; // can't use this because it may be inlined!
    cpu_data[cpu_number()].active_thread = new;
    //current_stack() = new->kernel_stack; // assembly does this
    current_pcb()= new->pcb;
    cpu_data[cpu_number()].flags = new->pcb->flags;

#if NCPUS > 1
	/* There is no free lunch!
	 * Save the floating point state for the old thread
	 * if the fpu has been used since the last context switch.
	 * Otherwise we need to broadcast an interrupt to the
	 * old fpu to get the state.
	 */
	fp_state_save(old);

#else
	//disable_fpu();
#endif 

#if KDEBUG
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_SCHED) | DBG_FUNC_END,
			old, new, 
			old->priority, old->sched_pri, 
			new->sched_pri);

	frame_ptr = STACK_IKS(new->kernel_stack)->r1;
	lr[0] = lr[1] = lr[2] = lr[3] = 0;
	lr[0] = STACK_IKS(new->kernel_stack)->lr;
	for (i=1;i<4;i++) {
	  if (frame_ptr == NULL) break;
	  lr[i] = frame_ptr->saved_lr;
	  frame_ptr = frame_ptr->saved_sp;
	}
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_SCHED) | DBG_FUNC_START,
			new, lr[0], lr[1], lr[2], lr[3]); 

	lr[0] = lr[1] = lr[2] = lr[3] = 0;
	for (i=0;i<4;i++) {
	  if (frame_ptr == NULL) break;
	  lr[i] = frame_ptr->saved_lr;
	  frame_ptr = frame_ptr->saved_sp;
	}
	if (lr[0]) {
		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_SCHED),
			0, lr[0], lr[1], lr[2], lr[3]);
	};
#endif
	ret_thread = Switch_context(old, continuation, new);

	return(ret_thread);
}

void
start_initial_context(
    thread_t            thread
)
{
    struct pcb          *pcb = thread->pcb;

    /*
     * Change software state.
     */

   PMAP_ACTIVATE(vm_map_pmap(thread->task->map), thread, 0);

// current_thread() = thread; // can't use this because it may be inlined!
   cpu_data[cpu_number()].active_thread = thread;
   current_stack() = thread->kernel_stack;
   current_pcb() = thread->pcb;
   cpu_data[cpu_number()].flags = thread->pcb->flags;

    /*
     * Change hardware state
     */
    load_context(thread);
    /*NOTREACHED*/
}

/*
 * Duplicate parent state in child
 * for U**X fork.
 */
thread_dup(
    thread_t		parent,
    thread_t		child
)
{
	struct ppc_saved_state	*parent_state, *child_state;
	struct ppc_float_state	*parent_float_state, *child_float_state;
	struct ppc_exception_state	*parent_exception_state,
					*child_exception_state;

	/* Save the FPU state */
	if (per_proc_info[cpu_number()].fpu_pcb == parent->pcb) {
		fp_state_save(parent);
	}

	parent_state = &parent->pcb->ss;
	child_state = &child->pcb->ss;

	/* rely on compiler structure assignment */
	*child_state = *parent_state;

	parent_float_state = &parent->pcb->fs;
	child_float_state = &child->pcb->fs;

	/* rely on compiler structure assignment */
	*child_float_state = *parent_float_state;
#if 0
	parent_exception_state = &parent->pcb->es;
	child_exception_state = &child->pcb->es;

	/* rely on compiler structure assignment */
	*child_exception_state = *parent_exception_state;
#endif
	child_state->r3 = child->task->proc->p_pid;
	child_state->r4 = 1;

	child_state->sr_copyin = child->pcb->sr0 + SR_COPYIN;
}

/*
 * Set thread integer state
 */
kern_return_t
set_thread_state(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		count
)
{
	struct ppc_saved_state	*saved_state;
	struct ppc_thread_state	*state;
    
	if (count < PPC_THREAD_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);
	
	if (thread->task->kernel_privilege) {
		/*
		* State for kernel threads
		* can only be set before thread
		* is first started.
		*
		* XXX This hack is due to the
		* fact that the &^%$#@! kernel loader
		* uses the thread_set_state() call to
		* start a thread in kernel mode.
		*/
		if (thread->swap_func != thread_bootstrap_return)
			return (KERN_INVALID_ARGUMENT);
	}

	state = (struct ppc_thread_state *)tstate;
	saved_state = &thread->pcb->ss;

	/*
	* structure assignment - depends on 
	* ppc_thread_state being a prefix of ppc_saved_state !
	*/

	*((struct ppc_thread_state *)saved_state) = *state;

	saved_state->sr_copyin = thread->pcb->sr0 + SR_COPYIN;

	saved_state->srr1 |= MSR_EXPORT_MASK_SET;

	if (thread->task->kernel_privilege) {
		saved_state->srr1 &= ~ MASK(MSR_PR);
		thread_start(thread, state->srr0);
	}

	return (KERN_SUCCESS);
}

/*
 * Set thread floating point state
 */
kern_return_t
set_thread_fpstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		count
)
{
	struct ppc_float_state	*state;
    
	if (count < PPC_FLOAT_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);

	fpu_save();
	fpu_disable();

	state = (struct ppc_float_state *)tstate;

	/* structure assignment */
	thread->pcb->fs = *state;
    
	return (KERN_SUCCESS);
}

/*
 * Set thread exception point state
 */
kern_return_t
set_thread_exstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		count
)
{
	struct ppc_exception_state	*state;
    
	if (count < PPC_EXCEPTION_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);

	state = (struct ppc_exception_state *)tstate;

	/* structure assignment */
	thread->pcb->es = *state;
    
	return (KERN_SUCCESS);
}

/*
 * Set externally visible thread
 * state.
 */
kern_return_t
thread_setstatus(
    thread_t			thread,
    int				flavor,
    thread_state_t		tstate,
    unsigned int		count
)
{
    switch (flavor) {

    case PPC_THREAD_STATE:
	return (set_thread_state(thread, tstate, count));
    
    case PPC_FLOAT_STATE:
 	return (set_thread_fpstate(thread, tstate, count));

    case PPC_EXCEPTION_STATE:
 	return (set_thread_exstate(thread, tstate, count));

    default:
	return (KERN_INVALID_ARGUMENT);
    }
}

/*
 * Get thread state flavor list
 */
kern_return_t
get_thread_state_flavor_list(
    thread_state_t		tstate,
    unsigned int		*count
)
{
	struct thread_state_flavor *state;

	if (*count < PPC_THREAD_STATE_FLAVOR_LIST_COUNT)
		return (KERN_INVALID_ARGUMENT);
	
	state = (struct thread_state_flavor *)tstate;

	state->flavor = PPC_THREAD_STATE;
	state->count = PPC_THREAD_STATE_COUNT;
    
	(++state)->flavor = PPC_FLOAT_STATE;
	state->count = PPC_FLOAT_STATE_COUNT;
    
	(++state)->flavor = PPC_EXCEPTION_STATE;
	state->count = PPC_EXCEPTION_STATE_COUNT;
    
	*count = PPC_THREAD_STATE_FLAVOR_LIST_COUNT;
    
	return (KERN_SUCCESS);
}

/*
 * Get thread integer state
 */
kern_return_t
get_thread_state(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
	struct ppc_thread_state	*saved_state;
	struct ppc_thread_state	*state;
    
	if (*count < PPC_THREAD_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);
	
	state = (struct ppc_thread_state *)tstate;
	saved_state = (struct ppc_thread_state *) &thread->pcb->ss;

	/*
	 * structure assignment - depends on 
	 * ppc_thread_state being a prefix of ppc_saved_state !
	 */
	*state = *((struct ppc_thread_state *)saved_state);

	*count = PPC_THREAD_STATE_COUNT;
    
	return (KERN_SUCCESS);
}

/*
 * Get thread floating point state
 */
kern_return_t
get_thread_fpstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
	struct ppc_float_state	*state;
    
	if (*count < PPC_FLOAT_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);

	fpu_save();
	fpu_disable();

	state = (struct ppc_float_state *)tstate;

	/* structure assignment */
	*state = thread->pcb->fs;

	*count = PPC_FLOAT_STATE_COUNT;
    
	return (KERN_SUCCESS);
}

/*
 * Get thread exception point state
 */
kern_return_t
get_thread_exstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
	struct ppc_exception_state	*state;
    
	if (*count < PPC_EXCEPTION_STATE_COUNT)
		return (KERN_INVALID_ARGUMENT);

	state = (struct ppc_exception_state *)tstate;

	/* structure assignment */
	*state = thread->pcb->es;

	*count = PPC_EXCEPTION_STATE_COUNT;
    
	return (KERN_SUCCESS);
}

/*
 * Return externally visible
 * thread status.
 */
kern_return_t
thread_getstatus(
    thread_t			thread,
    int				flavor,
    thread_state_t		tstate,
    unsigned int		*count
)
{
    switch (flavor) {

	case THREAD_STATE_FLAVOR_LIST:
		return (get_thread_state_flavor_list(tstate, count));

	case PPC_THREAD_STATE:
		return (get_thread_state(thread, tstate, count));
	
	case PPC_FLOAT_STATE:
		return (get_thread_fpstate(thread, tstate, count));
	
	case PPC_EXCEPTION_STATE:
		return (get_thread_exstate(thread, tstate, count));
	
	default:
		return (KERN_INVALID_ARGUMENT);
    }
}

/*
 * thread_userstack:
 *
 * Return the user stack pointer from the machine 
 * dependent thread state info.
 */
kern_return_t
thread_userstack(
    thread_t		thread,
    int			flavor,
    thread_state_t	tstate,
    unsigned int	count,
    vm_offset_t		*user_stack
)
{
	struct ppc_thread_state	*state;

	/*
	 * Set a default.
	 */
	if (*user_stack == 0)
		*user_stack = USRSTACK;
		
	switch (flavor) {
	case PPC_THREAD_STATE:
		if (count < PPC_THREAD_STATE_COUNT)
			return (KERN_INVALID_ARGUMENT);

		state = (struct ppc_thread_state *) tstate;

		/*
		 * If a valid user stack is specified, use it.
		 */
		*user_stack = state->r1 ? state->r1: USRSTACK;
		break;
	default :
		return (KERN_INVALID_ARGUMENT);
	}

	return (KERN_SUCCESS);
}

kern_return_t
thread_entrypoint(
    thread_t		thread,
    int			flavor,
    thread_state_t	tstate,
    unsigned int	count,
    vm_offset_t		*entry_point
)
{
    struct ppc_thread_state	*state;

    /*
     * Set a default.
     */
    if (*entry_point == 0)
	*entry_point = VM_MIN_ADDRESS;
		
    switch (flavor) {

    case PPC_THREAD_STATE:
	if (count < PPC_THREAD_STATE_COUNT)
	    return (KERN_INVALID_ARGUMENT);

	state = (struct ppc_thread_state *) tstate;

	/*
	 * If a valid entry point is specified, use it.
	 */
	*entry_point = state->srr0 ? state->srr0: VM_MIN_ADDRESS;
	break;
    default:
	return (KERN_INVALID_ARGUMENT);
    }

    return (KERN_SUCCESS);
}


void __volatile__
thread_syscall_return(
	kern_return_t	retval)
{
	thread_t	thread = current_thread();
	struct ppc_saved_state *ssp = &thread->pcb->ss;

	ssp->r3 = retval;

	thread_exception_return();
	/* NOTREACHED */
}

void
thread_set_syscall_return(
	thread_t	thread,
	kern_return_t	retval)
{
	struct ppc_saved_state *ssp = &thread->pcb->ss;

	ssp->r3 = retval;
}

void
pmap_switch(pmap_t map)
{
	unsigned int i;

	if (map->space == PPC_SID_KERNEL)
		return;

	/* sr value has Ks=1, Ku=1, and vsid composed of space+seg num */
	i = SEG_REG_PROT | (map->space << 4);

	isync();		/* context sync before */
/*	mtsr(0x0, i + 0x0); SR0 is part of the kernel address space */
/*	mtsr(0x1, i + 0x1); SR1 is part of the kernel address space */
/*	mtsr(0x2, i + 0x2); SR2 is part of the kernel address space */
/*	mtsr(0x3, i + 0x3); SR3 is part of the kernel address space */
	mtsr(0x4, i + 0x4);
	mtsr(0x5, i + 0x5);
	mtsr(0x6, i + 0x6);
	mtsr(0x7, i + 0x7);
	mtsr(0x8, i + 0x8);
	mtsr(0x9, i + 0x9);
	mtsr(0xa, i + 0xa);
	mtsr(0xb, i + 0xb);
	mtsr(0xc, i + 0xc);
	mtsr(0xd, i + 0xd);
	mtsr(0xe, i + 0xe);
	mtsr(0xf, i + 0xf);
	isync();		/* context sync after */
}

/*
 * task_map_io_ports() is required by the driverkit routines.
 */
kern_return_t
task_map_io_ports(
    task_t		task,
    unsigned int	port,
    unsigned int	length,
    boolean_t		unmap
)
{
	return (KERN_SUCCESS);
}


#if MACH_ASSERT
void
dump_pcb(pcb_t pcb)
{
	printf("pcb @ %8.8x:\n", pcb);
	printf("ksp       = 0x%08x\n\n",pcb->ksp);
#if DEBUG
	regDump(&pcb->ss);
#endif /* DEBUG */
}

void
dump_thread(thread_t th)
{
	printf(" thread @ 0x%x:\n", th);
}
#endif
