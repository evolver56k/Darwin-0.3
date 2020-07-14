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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Machine dependent thread module.
 *
 * HISTORY
 *
 * 6 April 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <kern/mach_param.h>

#import <architecture/i386/table.h>

#import <machdep/i386/fp_exported.h>
#import <machdep/i386/configure.h>

#import <machdep/i386/ldt.h>
#import <machdep/i386/table_inline.h>
#import <machdep/i386/desc_inline.h>
#import <machdep/i386/sel_inline.h>
#import <machdep/i386/cpu_inline.h>

/* U**X crap!! */
#import <sys/param.h>
#import <sys/proc.h>

#import <fp_emul.h>
#import <pc_support.h>

zone_t		pcb_zone;

vm_offset_t	stack_pointers[NCPUS];
boolean_t	empty_stacks[NCPUS];

/*
 * The base of the kernel stack
 * is different when a thread is
 * in v86 mode.
 */
#define KERNEL_STACK_BASE(state) \
    ((unsigned int) &(state)->frame.v_es)
    
#define KERNEL_V86_STACK_BASE(state) \
    ((unsigned int) &(&(state)->frame)[1])
    
void
stack_attach(
    thread_t		thread,
    vm_offset_t		stack,
    void		(*continuation)(void)
)
{
    tss_t		*tss;
    extern void		_stack_attach(void);
    
    thread->kernel_stack = stack;

    tss = thread->pcb->tss;
    tss->esp = tss->ebp = stack + KERNEL_STACK_SIZE;
    tss->eip = (unsigned int)_stack_attach;
    tss->ebx = (unsigned int)continuation;
}


vm_offset_t
stack_detach(
    thread_t		thread
)
{
    vm_offset_t		stack;
        
    stack = thread->kernel_stack;
    thread->kernel_stack = 0;
    
    return (stack);
}

void
stack_handoff(
    thread_t		old,
    thread_t		new
)
{
    struct pcb		*new_pcb = new->pcb,
    			*old_pcb = old->pcb;
    vm_offset_t		stack = stack_detach(old);
    
    stack_attach(new, stack, 0);

    /*
     * Change software state.
     */
    if (new->task != old->task) {
    	int		mycpu = cpu_number();

    	PMAP_DEACTIVATE(vm_map_pmap(old->task->map), old, mycpu);
	PMAP_ACTIVATE(vm_map_pmap(new->task->map), new, mycpu);
    }

    current_thread() = new;

    /*
     * Change hardware state.
     */
    if (new_pcb->tss->cr3 != old_pcb->tss->cr3)
	set_cr3(new_pcb->tss->cr3);

    if (new_pcb->ldt != old_pcb->ldt
	    || new_pcb->ldt_size != old_pcb->ldt_size) {
	map_ldt(sel_to_gdt_entry(LDT_SEL),
		    (vm_offset_t) new_pcb->ldt,
			    (vm_size_t) new_pcb->ldt_size);
	lldt();		
    }

    map_tss(sel_to_gdt_entry(TSS_SEL),
    		(vm_offset_t) new_pcb->tss
		    + KERNEL_LINEAR_BASE,
			(vm_size_t) new_pcb->tss_size);
    ltr(); setts();
}

extern
thread_t
_switch_tss(
	tss_t			*old_tss,
	tss_t			*new_tss,
	thread_t		old
);

thread_t
switch_context(
    thread_t		old,
    void		(*continuation)(void),
    thread_t		new
)
{
    struct pcb		*new_pcb = new->pcb,
    			*old_pcb = old->pcb;

    /*
     * Change software state.
     */
    if (new->task != old->task) {
    	int		mycpu = cpu_number();

    	PMAP_DEACTIVATE(vm_map_pmap(old->task->map), old, mycpu);
	PMAP_ACTIVATE(vm_map_pmap(new->task->map), new, mycpu);
    }

    current_thread() = new;
    current_stack() = new->kernel_stack;
    current_stack_pointer() = new->kernel_stack + KERNEL_STACK_SIZE;

    /*
     * Change hardware state.
     */
    if (new_pcb->tss->cr3 != old_pcb->tss->cr3)
	set_cr3(new_pcb->tss->cr3);

    if (new_pcb->ldt != old_pcb->ldt
	    || new_pcb->ldt_size != old_pcb->ldt_size) {
	map_ldt(sel_to_gdt_entry(LDT_SEL),
		    (vm_offset_t) new_pcb->ldt,
			    (vm_size_t) new_pcb->ldt_size);
	lldt();		
    }

    map_tss(sel_to_gdt_entry(TSS_SEL),
    		(vm_offset_t) new_pcb->tss
		    + KERNEL_LINEAR_BASE,
			(vm_size_t) new_pcb->tss_size);
    ltr(); setts();
    
    if (old->swap_func = continuation)
    	return _switch_tss(0, new_pcb->tss, old);
    else
    	return _switch_tss(old_pcb->tss, new_pcb->tss, old);
}

void
call_continuation(
    void		(*continuation)(void)
)
{
    _call_with_stack(continuation, current_stack_pointer());
    /* NOTREACHED */
}

void
pcb_module_init(void)
{
    pcb_zone = zinit(
		    sizeof (struct pcb),
		    THREAD_MAX * sizeof (struct pcb),
		    THREAD_CHUNK * sizeof (struct pcb),
		    FALSE, "pcb");
}

/*
 * Initialize a pcb for a
 * new thread.
 */
void pcb_init(
    thread_t		thread
)
{
    struct pcb			*pcb;
    tss_t			*tss;

    /*
     * Allocate a new pcb.
     */	
    thread->pcb = pcb = (void *)zalloc(pcb_zone);
    *pcb = (struct pcb) { 0 };

    /*
     * Setup with internal TSS.
     */
    pcb->tss = tss = &pcb->tss_store.internal;
    pcb->tss_size = TSS_SIZE(0);

    /*
     * Context from pmap.
     */
    tss->cr3 = vm_map_pmap(thread->task->map)->cr3;
    
    /*
     * Setup with default LDT.
     */
    pcb->ldt = (vm_offset_t)ldt + KERNEL_LINEAR_BASE;
    pcb->ldt_size = LDTSZ * sizeof (ldt_entry_t);
    
    tss->ldt = LDT_SEL;

    /*
     * Kernel state.
     */
    tss->ss0 = KDS_SEL;
    tss->eflags = EFL_IF;
    tss->ss = KDS_SEL;
    tss->cs = KCS_SEL;
    tss->ds = KDS_SEL;
    tss->es = KDS_SEL;
    tss->fs = LDATA_SEL;
    tss->gs = NULL_SEL;
    tss->io_bmap = TSS_SIZE(0);
}

void
pcb_common_init(
    task_t		task
)
{
    pcb_common_t	*common = (void *)kalloc(sizeof (*common));
    
    lock_init(&common->lock, TRUE);
    common->ldt = (vm_offset_t)ldt + KERNEL_LINEAR_BASE;
    common->ldt_size = LDTSZ * sizeof (ldt_entry_t);
    common->io_bmap.length = common->io_bmap.base = 0;
    task->pcb_common = common;
}

/*
 * Set the thread's io space to the region
 * defined by base and length.  The TSS is
 * expanded if necessary.  Thread must either
 * be the current thread, or suspended.
 * N.B. On exit, the thread is guaranteed to
 * have an external TSS.
 */
static void
thread_set_io_bmap(
    thread_t		thread,
    vm_offset_t		base,
    vm_size_t		length
)
{
    struct pcb		*pcb = thread->pcb;
    
    if (pcb->tss_size < TSS_SIZE(length + 1)) {
	struct tss_alloc	new_tss_alloc, old_tss_alloc;
	tss_t			*new_tss, *old_tss;

	/*
	 * TSS needs to be enlarged.
	 */
	new_tss_alloc.length = TSS_SIZE(length + 1);
	new_tss_alloc.base = (vm_offset_t)kalloc(new_tss_alloc.length);
		
	new_tss = (tss_t *)new_tss_alloc.base;
	old_tss = pcb->tss;

	/*
	 * Copy old TSS to new one.
	 */
	*new_tss = *old_tss;

	if (pcb->extern_tss) {
	    /*
	     * Deal with old external tss.
	     */	
	    old_tss_alloc = pcb->tss_store.external;
    
	    /*
	     * Switch to new TSS.
	     */	
	    pcb->tss_store.external = new_tss_alloc;
	    pcb->tss = new_tss;
	    pcb->tss_size = new_tss_alloc.length;
	    pcb->extern_tss = TRUE;
		
	    /*
	     * Free old TSS.
	     */
	    kfree((void *)old_tss_alloc.base, old_tss_alloc.length);
	}
	else {
	    /*
	     * Switch to new TSS.
	     */	
	    pcb->tss_store.external = new_tss_alloc;
	    pcb->tss = new_tss;
	    pcb->tss_size = new_tss_alloc.length;
	    pcb->extern_tss = TRUE;
	}

	/*
	 * If changing the current
	 * thread, remap its tss now.
	 */	
	if (thread == current_thread()) {
	    map_tss(sel_to_gdt_entry(TSS_SEL),
			(vm_offset_t) thread->pcb->tss
			    + KERNEL_LINEAR_BASE,
				(vm_size_t) thread->pcb->tss_size);
	    ltr();
	}
    }
    
    memcpy(
	(void *)pcb->tss + pcb->tss->io_bmap,
		(void *)base, length);
}

static void
task_update_io_bmap(
    task_t		task
)
{
    queue_head_t	*list;
    thread_t		thread, prev_thread;
    pcb_common_t	*common = task->pcb_common;
    
    list = &task->thread_list;
    prev_thread = THREAD_NULL;
    task_lock(task);
    thread = (thread_t)queue_first(list);
    while (!queue_end(list, (queue_entry_t)thread)) {
	thread_reference(thread);
	task_unlock(task);
	if (prev_thread != THREAD_NULL)
	    thread_deallocate(prev_thread);
	thread_set_io_bmap(thread,
			    common->io_bmap.base,
			    common->io_bmap.length);
	prev_thread = thread;
	task_lock(task);
	thread = (thread_t)queue_next(&thread->thread_list);
    }
    task_unlock(task);
    if (prev_thread != THREAD_NULL)
    	thread_deallocate(prev_thread);
}

kern_return_t
task_map_io_ports(
    task_t		task,
    unsigned int	port,
    unsigned int	length,
    boolean_t		unmap
)
{
    pcb_common_t	*common = task->pcb_common;
    vm_offset_t		new_io_bmap_base;
    vm_size_t		new_io_bmap_length;
    int			i;

    /*
     * Bad port range.
     */	
    if ((port + length) > 65536)
    	return (KERN_INVALID_ADDRESS);
	
    if (task->kernel_vm_space)
    	return (KERN_SUCCESS);

    /*
     * If task is being terminated,
     * don't bother.
     */ 
    if (task_hold(task) != KERN_SUCCESS)
    	return (KERN_INVALID_ARGUMENT);
    
    new_io_bmap_length = roundup(port + length, NBBY) / NBBY;
    
    lock_write(&common->lock);
    
    if (common->io_bmap.length < new_io_bmap_length) {
	new_io_bmap_base = (vm_offset_t)kalloc(new_io_bmap_length);
	
	/*
	 * Invalidate all of new IO bitmap.
	 */
	memset(
	    (void *)new_io_bmap_base,
		    0xff, new_io_bmap_length);
	
	/*
	 * Copy old IO bitmap.
	 */
	memcpy(
	    (void *)new_io_bmap_base,
	    	(void *)common->io_bmap.base, common->io_bmap.length);
		
	kfree((void *)common->io_bmap.base, common->io_bmap.length);
	
	common->io_bmap.base = new_io_bmap_base;
	common->io_bmap.length = new_io_bmap_length;
    }
    
    for (i = port; i < (port + length); i++)
    	if (unmap)
	    setbit(common->io_bmap.base, i);
	else
	    clrbit(common->io_bmap.base, i);
	    
    (void) task_dowait(task, TRUE);
    
    task_update_io_bmap(task);
    
    (void) task_release(task);
    
    lock_done(&common->lock);
    
    return (KERN_SUCCESS);
}

static void
thread_set_ldt(
    thread_t		thread,
    vm_offset_t		address,
    vm_size_t		size
)
{
    thread->pcb->ldt = address;
    thread->pcb->ldt_size = size;

    /*
     * If changing the current
     * thread, remap its LDT now.
     */	
    if (thread == current_thread()) {
	map_ldt(sel_to_gdt_entry(LDT_SEL),
		    (vm_offset_t) thread->pcb->ldt,
			    (vm_size_t) thread->pcb->ldt_size);
	lldt();
    }
}

static void
task_update_ldt(
    task_t		task
)
{
    queue_head_t	*list;
    thread_t		thread, prev_thread;
    pcb_common_t	*common = task->pcb_common;
    
    list = &task->thread_list;
    prev_thread = THREAD_NULL;
    task_lock(task);
    thread = (thread_t)queue_first(list);
    while (!queue_end(list, (queue_entry_t)thread)) {
	thread_reference(thread);
	task_unlock(task);
	if (prev_thread != THREAD_NULL)
	    thread_deallocate(prev_thread);
	thread_set_ldt(thread, common->ldt, common->ldt_size);
	prev_thread = thread;
	task_lock(task);
	thread = (thread_t)queue_next(&thread->thread_list);
    }
    task_unlock(task);
    if (prev_thread != THREAD_NULL)
    	thread_deallocate(prev_thread);
}

kern_return_t
task_locate_ldt(
    task_t		task,
    vm_offset_t		address,
    vm_size_t		size
)
{
    pcb_common_t	*common = task->pcb_common;
    
    if (vm_map_min(task->map) > address ||
	    vm_map_max(task->map) <= (address + size))
	return (KERN_INVALID_ADDRESS);

    /*
     * If task is being terminated,
     * don't bother.
     */ 
    if (task_hold(task) != KERN_SUCCESS)
    	return (KERN_INVALID_ARGUMENT);
    
    lock_write(&common->lock);
    
    common->ldt = address;
    common->ldt_size = size;
    
    (void) task_dowait(task, TRUE);
    
    task_update_ldt(task);
    
    (void) task_release(task);
    
    lock_done(&common->lock);
    
    return (KERN_SUCCESS);
}

kern_return_t
task_default_ldt(
    task_t		task
)
{
    pcb_common_t	*common = task->pcb_common;

    /*
     * If task is being terminated,
     * don't bother.
     */ 
    if (task_hold(task) != KERN_SUCCESS)
    	return (KERN_INVALID_ARGUMENT);
    
    lock_write(&common->lock);
    
    common->ldt = (vm_offset_t)ldt + KERNEL_LINEAR_BASE;
    common->ldt_size = LDTSZ * sizeof (ldt_entry_t);
    
    (void) task_dowait(task, TRUE);
    
    task_update_ldt(task);
    
    (void) task_release(task);
    
    lock_done(&common->lock);
    
    return (KERN_SUCCESS);
}

thread_saved_state_t *
thread_user_state(
    thread_t		thread
)
{
    thread_save_area_t	*save_area = thread->pcb->save_area;

    if (save_area == 0) {
    	thread_saved_state_t	*saved_state;

    	thread->pcb->save_area =
		save_area = (void *)kalloc(sizeof (*save_area));

	saved_state = &save_area->sa_u;
	*saved_state = (thread_saved_state_t) { 0 };
	saved_state->frame.eflags = EFL_IF;
	saved_state->frame.cs = UCODE_SEL;
	saved_state->frame.ss = UDATA_SEL;
	saved_state->regs.ds = UDATA_SEL;
	saved_state->regs.es = UDATA_SEL;
	saved_state->regs.fs = NULL_SEL;
	saved_state->regs.gs = NULL_SEL;
	
	return (saved_state);
    }
    else
	return (&save_area->sa_u);
}

/*
 * Entry point for new user
 * threads.
 */
void
thread_bootstrap_return(void)
{
    thread_t		thread = current_thread();
    pcb_common_t	*common = thread->task->pcb_common;
    struct pcb		*pcb = thread->pcb;

    lock_read(&common->lock);
    
    if (common->io_bmap.length > 0)
    	thread_set_io_bmap(thread,
			    common->io_bmap.base,
			    common->io_bmap.length);
    
    if (common->ldt != pcb->ldt)
	thread_set_ldt(thread, common->ldt, common->ldt_size);
	
    lock_done(&common->lock);
    
    thread_exception_return();
    /* NOTREACHED */
}

void
thread_exception_return(void)
{
    thread_t			thread = current_thread();
    thread_saved_state_t	*saved_state = USER_REGS(thread);

    check_for_ast(saved_state);		// *may* not return

    thread->pcb->tss->esp0 =
	    ((saved_state->frame.eflags & EFL_VM) ?
					KERNEL_V86_STACK_BASE(saved_state) :
					KERNEL_STACK_BASE(saved_state));

    _return_with_state(saved_state);
    /* NOTREACHED */
}

void __volatile__
thread_syscall_return(
    kern_return_t	result
)
{
    thread_t			thread = current_thread();
    thread_saved_state_t	*saved_state = USER_REGS(thread);
    
    saved_state->regs.eax = result;

    check_for_ast(saved_state);		// *may* not return

    thread->pcb->tss->esp0 =
	    ((saved_state->frame.eflags & EFL_VM) ?
					KERNEL_V86_STACK_BASE(saved_state) :
					KERNEL_STACK_BASE(saved_state));

    _return_with_state(saved_state);
    /* NOTREACHED */
}

void
thread_set_syscall_return(
    thread_t		thread,
    kern_return_t	result
)
{
    thread_saved_state_t	*saved_state = USER_REGS(thread);
    
    saved_state->regs.eax = result;
}

/*
 * Called from locore to
 * startup the first thread. (never returns)
 */
void
start_initial_context(
    thread_t		thread
)
{
    struct pcb		*pcb = thread->pcb;

    /*
     * Initialize the common,
     * static LDT.
     */
    ldt_init();

    /*
     * Change software state.
     */
   PMAP_ACTIVATE(vm_map_pmap(thread->task->map), thread, 0);

   current_thread() = thread;
   current_stack() = thread->kernel_stack;
   current_stack_pointer() = thread->kernel_stack + KERNEL_STACK_SIZE;

    /*
     * Change hardware state.
     */
    set_cr3(pcb->tss->cr3);

    map_ldt(sel_to_gdt_entry(LDT_SEL),
		(vm_offset_t) pcb->ldt,
			(vm_size_t) pcb->ldt_size);
    lldt();

    map_tss(sel_to_gdt_entry(TSS_SEL),
    		(vm_offset_t) pcb->tss
		    + KERNEL_LINEAR_BASE,
			(vm_size_t) pcb->tss_size);
    ltr(); setts();
			
    _switch_tss(0, pcb->tss, 0);
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

    case i386_THREAD_STATE:
	return (set_thread_state(thread, tstate, count));
    
    case i386_THREAD_FPSTATE:
 	return (set_thread_fpstate(thread, tstate, count));

    default:
	return (KERN_INVALID_ARGUMENT);
    }
}

static void set_thread_v86_state(
		thread_t			thread,
		i386_thread_state_t		*state);

kern_return_t
set_thread_state(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		count
)
{
    thread_saved_state_t	*saved_state;
    i386_thread_state_t		*state;
    
    if (count < i386_THREAD_STATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_state_t *)tstate;
    
    if ((state->eflags & EFL_VM) != 0)
	set_thread_v86_state(thread, state);
    else {
	if (!thread->task->kernel_privilege) {
	    /*
	     * Validate segment selector values.
	     */
	    if (
		    !valid_user_code_selector(state->cs) ||
		    !valid_user_data_selector(state->ds) ||
		    !valid_user_data_selector(state->es) ||
		    !valid_user_data_selector(state->fs) ||
		    !valid_user_data_selector(state->gs) ||
		    !valid_user_stack_selector(state->ss)
		)
		return (KERN_INVALID_ARGUMENT);
	}
	else {
	    tss_t	*tss = thread->pcb->tss;
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
		
	    tss->eax = state->eax;
	    tss->ebx = state->ebx;
	    tss->ecx = state->ecx;
	    tss->edx = state->edx;
	    tss->edi = state->edi;
	    tss->esi = state->esi;
		
	    thread_start(thread, state->eip);
	    
	    return (KERN_SUCCESS);
	}

	saved_state = USER_REGS(thread);
	    
	saved_state->regs.eax = state->eax;
	saved_state->regs.ebx = state->ebx;
	saved_state->regs.ecx = state->ecx;
	saved_state->regs.edx = state->edx;
	saved_state->regs.edi = state->edi;
	saved_state->regs.esi = state->esi;
	saved_state->regs.ebp = state->ebp;
	saved_state->frame.esp = state->esp;
	saved_state->frame.ss = selector_to_sel(state->ss);
	saved_state->frame.eflags = state->eflags;
	saved_state->frame.eflags &= ~( EFL_VM | EFL_NT | EFL_IOPL | EFL_CLR );
	saved_state->frame.eflags |=  ( EFL_IF | EFL_SET );
	saved_state->frame.eip = state->eip;
	saved_state->frame.cs = selector_to_sel(state->cs);
	saved_state->regs.ds = selector_to_sel(state->ds);
	saved_state->regs.es = selector_to_sel(state->es);
	saved_state->regs.fs = selector_to_sel(state->fs);
	saved_state->regs.gs = selector_to_sel(state->gs);
    }

    return (KERN_SUCCESS);
}

static
void
set_thread_v86_state(
    thread_t			thread,
    i386_thread_state_t		*state
)
{
    thread_saved_state_t	*saved_state = USER_REGS(thread);

    saved_state->regs.eax = state->eax;
    saved_state->regs.ebx = state->ebx;
    saved_state->regs.ecx = state->ecx;
    saved_state->regs.edx = state->edx;
    saved_state->regs.edi = state->edi;
    saved_state->regs.esi = state->esi;
    saved_state->regs.ebp = state->ebp;
    saved_state->frame.esp = state->esp;
    saved_state->frame.ss = selector_to_sel(state->ss);
    saved_state->frame.eflags = state->eflags;
    saved_state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    saved_state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    saved_state->frame.eip = state->eip;
    saved_state->frame.cs = selector_to_sel(state->cs);
    saved_state->regs.ds = NULL_SEL;
    saved_state->regs.es = NULL_SEL;
    saved_state->regs.fs = NULL_SEL;
    saved_state->regs.gs = NULL_SEL;
    saved_state->frame.v_ds = state->ds;
    saved_state->frame.v_es = state->es;
    saved_state->frame.v_fs = state->fs;
    saved_state->frame.v_gs = state->gs;
}

kern_return_t
set_thread_fpstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		count
)
{
    fp_state_t			*saved_state;
    i386_thread_fpstate_t	*state;
    
    if (count < i386_THREAD_FPSTATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_fpstate_t *)tstate;
    saved_state = &thread->pcb->fpstate;
    
    fp_terminate(thread);
	
    saved_state->environ = state->environ;
    saved_state->stack = state->stack;
    
    thread->pcb->fpvalid = TRUE;

    return (KERN_SUCCESS);
}

kern_return_t
thread_set_cthread_self(
    int				self
)
{
   current_thread()->pcb->cthread_self = (unsigned int)self;
   
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

    case i386_THREAD_STATE:
	return (get_thread_state(thread, tstate, count));
	
    case i386_THREAD_FPSTATE:
	return (get_thread_fpstate(thread, tstate, count));
	
    case i386_THREAD_EXCEPTSTATE:
	return (get_thread_exceptstate(thread, tstate, count));
	
    case i386_THREAD_CTHREADSTATE:
	return (get_thread_cthreadstate(thread, tstate, count));

    case THREAD_STATE_FLAVOR_LIST:
	return (get_thread_state_flavor_list(tstate, count));

    default:
	return (KERN_INVALID_ARGUMENT);
    }
}

kern_return_t
get_thread_state(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
    thread_saved_state_t	*saved_state;
    i386_thread_state_t		*state;
    
    if (*count < i386_THREAD_STATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_state_t *)tstate;
    saved_state = USER_REGS(thread);

    state->eax = saved_state->regs.eax;
    state->ebx = saved_state->regs.ebx;
    state->ecx = saved_state->regs.ecx;
    state->edx = saved_state->regs.edx;
    state->edi = saved_state->regs.edi;
    state->esi = saved_state->regs.esi;
    state->ebp = saved_state->regs.ebp;
    state->esp = saved_state->frame.esp;
    state->ss = sel_to_selector(saved_state->frame.ss);
    state->eflags = saved_state->frame.eflags;
    state->eip = saved_state->frame.eip;
    state->cs = sel_to_selector(saved_state->frame.cs);
    if ((saved_state->frame.eflags & EFL_VM) == 0) {
	state->ds = sel_to_selector(saved_state->regs.ds);
	state->es = sel_to_selector(saved_state->regs.es);
	state->fs = sel_to_selector(saved_state->regs.fs);
	state->gs = sel_to_selector(saved_state->regs.gs);
    }
    else {
	state->ds = saved_state->frame.v_ds;
	state->es = saved_state->frame.v_es;
	state->fs = saved_state->frame.v_fs;
	state->gs = saved_state->frame.v_gs;
    }
	
    *count = i386_THREAD_STATE_COUNT;
    
    return (KERN_SUCCESS);
}

kern_return_t
get_thread_fpstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
    fp_state_t			*saved_state;
    i386_thread_fpstate_t	*state;
    
    if (*count < i386_THREAD_FPSTATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_fpstate_t *)tstate;
    saved_state = &thread->pcb->fpstate;
    
    fp_synch(thread);
    
    state->environ = saved_state->environ;
    state->stack = saved_state->stack;
    
    *count = i386_THREAD_FPSTATE_COUNT;
    
    return (KERN_SUCCESS);
}

kern_return_t
get_thread_exceptstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
    thread_saved_state_t	*saved_state;
    i386_thread_exceptstate_t	*state;
    
    if (*count < i386_THREAD_EXCEPTSTATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_exceptstate_t *)tstate;
    saved_state = USER_REGS(thread);
    
    state->trapno = saved_state->trapno;
    state->err = saved_state->frame.err;
    
    *count = i386_THREAD_EXCEPTSTATE_COUNT;
    
    return (KERN_SUCCESS);
}

kern_return_t
get_thread_cthreadstate(
    thread_t			thread,
    thread_state_t		tstate,
    unsigned int		*count
)
{
    i386_thread_cthreadstate_t	*state;
    
    if (*count < i386_THREAD_CTHREADSTATE_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (i386_thread_cthreadstate_t *)tstate;
    state->self = thread->pcb->cthread_self;
    
    *count = i386_THREAD_CTHREADSTATE_COUNT;
    
    return (KERN_SUCCESS);
}

kern_return_t
thread_get_cthread_self(
    void
)
{
    return ((kern_return_t)current_thread()->pcb->cthread_self);
}

kern_return_t
get_thread_state_flavor_list(
    thread_state_t		tstate,
    unsigned int		*count
)
{
    struct thread_state_flavor	*state;
#define i386_THREAD_STATE_FLAVOR_COUNT	4

#define i386_THREAD_STATE_FLAVOR_LIST_COUNT			\
    ( i386_THREAD_STATE_FLAVOR_COUNT *				\
	( sizeof (struct thread_state_flavor) / sizeof (int) ) )
    
    if (*count < i386_THREAD_STATE_FLAVOR_COUNT)
	return (KERN_INVALID_ARGUMENT);
	
    state = (struct thread_state_flavor *)tstate;

    state->flavor = i386_THREAD_STATE;
    state->count = i386_THREAD_STATE_COUNT;
    
    (++state)->flavor = i386_THREAD_FPSTATE;
    state->count = i386_THREAD_FPSTATE_COUNT;
    
    (++state)->flavor = i386_THREAD_EXCEPTSTATE;
    state->count = i386_THREAD_EXCEPTSTATE_COUNT;
    
    (++state)->flavor = i386_THREAD_CTHREADSTATE;
    state->count = i386_THREAD_CTHREADSTATE_COUNT;
    
    *count = i386_THREAD_STATE_FLAVOR_LIST_COUNT;
    
    return (KERN_SUCCESS);
}

/*
 * thread_userstack:
 *
 * Return the user stack pointer from the machine dependent thread state info.
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
    i386_thread_state_t	*state;

    /*
     * Set a default.
     */
    if (*user_stack == 0)
	*user_stack = VM_MAX_ADDRESS;
		
    switch (flavor) {

    case i386_THREAD_STATE:
	if (count < i386_THREAD_STATE_COUNT)
	    return (KERN_INVALID_ARGUMENT);

	state = (i386_thread_state_t *) tstate;

	/*
	 * If a valid user stack is specified, use it.
	 */
	*user_stack = state->esp ? state->esp: VM_MAX_ADDRESS;
	break;
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
    i386_thread_state_t	*state;

    /*
     * Set a default.
     */
    if (*entry_point == 0)
	*entry_point = VM_MIN_ADDRESS;
		
    switch (flavor) {

    case i386_THREAD_STATE:
	if (count < i386_THREAD_STATE_COUNT)
	    return (KERN_INVALID_ARGUMENT);

	state = (i386_thread_state_t *) tstate;

	/*
	 * If a valid entry point is specified, use it.
	 */
	*entry_point = state->eip ? state->eip: VM_MIN_ADDRESS;
	break;
    }

    return (KERN_SUCCESS);
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
    struct thread_saved_state	*parent_state, *child_state;

    parent_state = USER_REGS(parent);
    child_state = thread_user_state(child);

    *child_state = *parent_state;

    child_state->regs.eax = child->task->proc->p_pid;

    child_state->regs.edx = 1;
    child_state->frame.eflags &= ~EFL_CF;
}

/*
 * Release resources on
 * thread termination.
 */
pcb_terminate(
    thread_t		thread
)
{
    struct pcb		*pcb = thread->pcb;

    /*
     * Give up the fpu if
     * necessary.
     */
    fp_terminate(thread);

#if	PC_SUPPORT    
    if (pcb->PCpriv)
    	PCdestroy(thread);
#endif

    /*
     * Free save area.
     */
    if (pcb->save_area)
    	kfree((void *)pcb->save_area,
		sizeof (*pcb->save_area));

    /*
     * Free external tss.
     */
    if (pcb->extern_tss)
	kfree(
	    (void *)pcb->tss_store.external.base,
	    		pcb->tss_store.external.length);
			
    thread->pcb = 0;
    zfree(pcb_zone, pcb);
}

/*
 * Synchronize pcb with
 * hardware state.
 */
pcb_synch(
    thread_t		thread
)
{
    /*
     * Write out our fp state
     * if necessary.
     */
    fp_synch(thread);
}

void
pcb_common_terminate(
    task_t		task
)
{
    pcb_common_t	*common = task->pcb_common;
    
    kfree((void *)common, sizeof (*common));
}
