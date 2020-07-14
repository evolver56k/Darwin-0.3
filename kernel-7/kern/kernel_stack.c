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
 * Copyright (c) 1990 NeXT, Inc.
 *
 *  History:
 *
 *	18-Jul-90: Brian Pinkerton at NeXT
 *		created
 */
 
/*
 *  Kernel stack module: handle the allocation, swapping (unwiring) and
 *  freeing of thread kernel stacks.  Kernel stacks are allocated from whole
 *  pages. They may only cross page boundaries if they are bigger than a page.
 *  The constant KERNEL_STACK_SIZE (kernel_stack.h) defines the actual size of
 *  kernel stacks.
 *
 *  The address of a kernel stack given to a client differs from the actual
 *  address of the stack.  Internally, we prepend a struct _kernelStack to
 *  every stack we return to the user.  This structure allows us to queue the
 *  stacks, and maintain information about the status of the stack.
 *
 *  Access to the stack structures is protected by a single lock.  We need
 *  a sleep lock because certain functions we call can block (for example,
 *  vm_map_pageable).  Ultimately, this module should use finer granularity
 *  locks -- one on the chunk of stacks, one on an individual stack, and an
 *  infrequently used one on the entire system.
 *
 *  Exported routines:
 *
 *  void initKernelStacks() 		  initializes this module
 *  vm_offset_t allocStack() 		  allocate a new kernel stack
 *  void freeStack(vm_offset_t stack) 	  free a previously alloc'ed kernel stack
 *  void swapOutStack(vm_offset_t stack)  try to swap out a stack
 *  void swapInStack(vm_offset_t stack)   swap in a stack
 *
 *  Internal routines:
 *
 *  vm_offset_t newStack():		  allocate new stacks from new memory
 *  void enterFreeList(vm_offset_t stack) put a stack on the free list
 *  void checkFreeList(vm_offset_t stack) try to free (or swap) an entire page
 *  int canSwap(vm_offset_t stack)	  return TRUE if we can swap this stack
 *  void doSwapout(vm_offset_t stack)	  really swap out this stack
 */

#import <mach_debug.h>

#import <kern/queue.h>
#import <kern/thread.h>
#import <kern/kernel_stack.h>
#import <kern/sched_prim.h>
#import <mach/vm_param.h>
#import <vm/vm_map.h>
#import <vm/vm_kern.h>

#import <kern/assert.h>

/*
 *  Internal prototypes
 */
vm_offset_t newStack();
static __inline__ vm_offset_t _allocStack(boolean_t canblock);
static void enterFreeList(vm_offset_t stack);
static void checkFreeList(vm_offset_t stack);
int canSwap(vm_offset_t stack);
void doSwapout(vm_offset_t stack);

/*
 *  Data structures
 */
static queue_head_t	stack_queue;
lock_data_t		stack_queue_lock;

static int		kernelStackBlock;	/* actual size of stack */
static int		stacksPerPage;		/* can be <= 1 */
static int		stack_free_count = 0;	/* number actually free */
static int		stack_free_target = 8;	/* number we want free */
static boolean_t	need_stack_wakeup = FALSE;/* if true, notify that
						   * stacks are available */

struct stackStats {
	int	allocatedChunks;    /* space alloc'ed from the kernel map */
	int	allocatedStacks;    /* total number of allocated stacks */
	int	freeStacks;	    /* number of stacks on the free list */
	int	swappableStacks;    /* number of stacks marked swappable */
	int	swappedChunks;	    /* number of pages swapped out */
} stackStats;


/*
 *  kstack_init: initialize the kernel stack data structures
 */
void
initKernelStacks()
{	
	queue_init(&stack_queue);
	lock_init(&stack_queue_lock, TRUE);
	
	kernelStackBlock = KERNEL_STACK_SIZE + sizeof(struct _kernelStack);
	stacksPerPage = (page_size + kernelStackBlock - 1) / kernelStackBlock;
}


/*
 *  enterFreeList: enter a stack on the free list, marking it free in the process
 */
static void
enterFreeList(vm_offset_t stack)
{
	/*
	 *  Put the guy on the free list
	 */
	((kernelStack) stack)->status = STACK_FREE;
	queue_enter(&stack_queue, (kernelStack) stack, kernelStack, freeList);
	stack_free_count ++;
	stackStats.freeStacks++;
}


/*
 *  checkFreeList: try to free up a chunk of memory of all stacks in that chunk
 *  are free.
 */
static void
checkFreeList(vm_offset_t stack)
{
	int i, freeAll;
	vm_offset_t page = trunc_page(stack);
	vm_offset_t thisStack;
	
	/*
	 *  Determine if we should free a page by checking the status of each
	 *  stack on a page.
	 */
	thisStack = page;
	freeAll = TRUE;
	for (i = 0; i < stacksPerPage; i++) {
		if ( ((kernelStack) thisStack)->status != STACK_FREE )
			freeAll = FALSE;
			
		thisStack += kernelStackBlock;
	}
	
	if (!freeAll) {
		if (canSwap(stack))
			doSwapout(stack);
		return;
	}

	/*
	 *  We should free the page, so go through and remove all the stacks
	 *  on this page from the free list, then free the page.
	 */
	thisStack = page;
	for (i = 0; i < stacksPerPage; i++) {
		queue_remove(&stack_queue, (kernelStack) thisStack, kernelStack, freeList);
		stack_free_count--;
		stackStats.freeStacks--;

#if	MACH_DEBUG
		stack_finalize(thisStack + sizeof (struct _kernelStack));
#endif	/* MACH_DEBUG */
			
		thisStack += kernelStackBlock;
	}

	kmem_free(kernel_map, stack, kernelStackBlock);
	stackStats.allocatedChunks--;
}


/*
 *  freeStack: free up a kernel stack
 *
 *  We put the stack on the free list, then check all items on that page to see if
 *  they can be freed.  If so, we remove all the stacks on the page from the free
 *  list and free the page.
 *  
 *  A further optimization would be to try to swap the page if only free stacks and
 *  swapped stacks remained on the page.
 */
void
freeStack(vm_offset_t stack)
{	
	stackStats.allocatedStacks--;
	stack -= sizeof(struct _kernelStack);

	assert(((kernelStack) stack)->status == STACK_IN_USE);
		
	lock_write(&stack_queue_lock);
	enterFreeList(stack);
	lock_done(&stack_queue_lock);
	
	/*
	 *  Try to keep some stacks free so not everyone goes through the pain of
	 *  allocation.
	 */
	if (need_stack_wakeup) {
		need_stack_wakeup = FALSE;
		thread_wakeup(&stack_queue);
	}
	
	if (stack_free_count <= stack_free_target)
		return;
		
	checkFreeList(stack);
}


/*
 *  newStack: allocate a new kernel stack
 *
 *  Here, we just allocate a new page and break it up into its constituent stacks.
 *  One stack (the first in the chunk) is returned as the new stack, and the
 *  remaining ones are marked as free and put on the free list.
 */
vm_offset_t
newStack()
{
	vm_offset_t newPage, stack;
	int i;
	
	if (kmem_alloc_wired(kernel_map,
				&newPage, kernelStackBlock) != KERN_SUCCESS)
		return 0;

	stackStats.allocatedChunks++;
		
	((kernelStack) newPage)->status = STACK_IN_USE;
	
	stackStats.allocatedStacks++;

#if	MACH_DEBUG
	stack_init(newPage + sizeof (struct _kernelStack));
#endif	/* MACH_DEBUG */

	if (stacksPerPage <= 1)
		return newPage + sizeof(struct _kernelStack);
	
	/*
	 *  Return the first guy on the page as our stack, and create
	 *  free stacks out of the rest of the slots on the page.
	 */
	lock_write(&stack_queue_lock);
	stack = newPage + kernelStackBlock;
	for (i = 1; i < stacksPerPage; i++) {
#if	MACH_DEBUG
		stack_init(stack + sizeof (struct _kernelStack));
#endif	/* MACH_DEBUG */
		enterFreeList(stack);
		stack += kernelStackBlock;
	}
	lock_done(&stack_queue_lock);
	
	return newPage + sizeof(struct _kernelStack);
}


/*
 *  allocStack: allocate and return a kernel stack (was stack_alloc)
 *
 *  Try to grab a free stack off the list of free stacks.  If that fails, get
 *  a new stack.  If that fails (unlikely), fall asleep and wait for someone to
 *  free a stack.
 *
 *  Return the address of the new stack.
 */
static __inline__
vm_offset_t
_allocStack(
	boolean_t	canblock
)
{
	register vm_offset_t	stack;
	register boolean_t	msg_printed = FALSE;
	register kern_return_t	result = THREAD_AWAKENED;

	do {
	    lock_write(&stack_queue_lock);
	    if (stack_free_count != 0) {
		stack = (vm_offset_t) dequeue_head(&stack_queue);
		((kernelStack) stack)->status = STACK_IN_USE;
		stack += sizeof(struct _kernelStack);
		stack_free_count--;
		stackStats.freeStacks--;
		stackStats.allocatedStacks++;
	    } else {
		stack = (vm_offset_t)0;
	    }
	    lock_done(&stack_queue_lock);
	    
	    if (!canblock)
	    	return (stack);

	    /*
	     *	If no stacks on queue, allocate one.  If that fails,
	     *	pause and wait for a stack to be freed.
	     */
	    if (stack == (vm_offset_t)0)
		stack = newStack();

	    if (stack == (vm_offset_t)0) {
		if (!msg_printed ) {
		    msg_printed = TRUE;
		    uprintf("MACH: Out of kernel stacks, pausing...");
		    if (!need_stack_wakeup)
			printf("stack_alloc: Kernel stacks exhausted\n");
		}
		else if (result != THREAD_AWAKENED) {
		    /*
		     *	Somebody wants us; return a bogus stack.
		     */
		    return((vm_offset_t)0);
		}

		/*
		 *	Now wait for stack, but first make sure one
		 *	hasn't appeared in the interim.
		 */
		lock_write(&stack_queue_lock);
		if(stack_free_count != 0) {
		    lock_done(&stack_queue_lock);
		    result = THREAD_AWAKENED;
		    continue;
		}
		assert_wait(&stack_queue, FALSE);
		need_stack_wakeup = TRUE;
		lock_done(&stack_queue_lock);
		thread_block();
		result = current_thread()->wait_result;
	    } else {
		if (msg_printed)
		    uprintf("continuing\n");		/* got a stack now */
		}
	} while (stack == (vm_offset_t)0);
	
	return(stack);
}

vm_offset_t
allocStack()
{
	return (_allocStack(TRUE));
}


/*
 *  canSwap: see if we can swap the entire chunk that this stack lives on
 *
 *  Return TRUE if we can, FALSE otherwise.
 */
int
canSwap(vm_offset_t stack)
{
	int i;
	vm_offset_t thisStack;

	/*
	 *  Determine if we should swap a page by checking the status of each
	 *  stack on a page.
	 */
	thisStack = trunc_page(stack);
	for (i = 0; i < stacksPerPage; i++) {
		if ( ((kernelStack) thisStack)->status == STACK_IN_USE )
			return FALSE;
			
		thisStack += kernelStackBlock;
	}
	
	return TRUE;
}


/*
 *  doSwapout: really swap out a stack.
 *
 *  The stack_queue_lock must be held across this call.
 */
void
doSwapout(vm_offset_t stack)
{
	int i, swapAll;
	vm_offset_t page = trunc_page(stack);
	vm_offset_t thisStack;

	/*
	 *  Make sure we remove all free stacks on this page from the free list.
	 */
	thisStack = page;
	swapAll = TRUE;
	for (i = 0; i < stacksPerPage; i++) {
		
		assert( ((kernelStack) thisStack)->status != STACK_IN_USE );
		
		if ( ((kernelStack) thisStack)->status == STACK_FREE ) {
			queue_remove(&stack_queue, (kernelStack) thisStack, kernelStack, freeList);
			stack_free_count--;
			stackStats.freeStacks--;
		}

		thisStack += kernelStackBlock;
	}
	
	/*
	 *  Hack... we need a way to designate that the page is really
	 *  unwired so that when we bring it back in, we can notice that
	 *  it had been unwired.
	 */
	((kernelStack) page)->freeList.next = (struct queue_entry *) 0xfeedface;
	(void) vm_map_pageable(kernel_map, page,
			       round_page(page + kernelStackBlock), TRUE);
	stackStats.swappedChunks++;
}


/*
 *  swapoutStack: try to swap out a stack
 *
 *  We swap out stacks by unwiring their memory, then allowing the pagout daemon
 *  to page out the unused stack.  If a kernel stack spans whole pages, we can just
 *  unwire its memory right away.  However, if it occupies a fraction of a page,
 *  then we must also be able to swap any other stacks on that page. 
 */
void
swapoutStack(vm_offset_t stack)
{
	int i, swapAll;
	vm_offset_t page = trunc_page(stack);
	vm_offset_t thisStack;

	stack -= sizeof(struct _kernelStack);
	stackStats.swappableStacks++;
	
	lock_write(&stack_queue_lock);
	/*
	 *  Mark this stack swappable
	 */
	((kernelStack) stack)->status = STACK_SWAPPED;
	
	/*
	 *  Bug out now if we can't swap the stack
	 */
	if (!canSwap(stack)) {
		lock_done(&stack_queue_lock);
		return;
	}
	
	doSwapout(stack);
	lock_done(&stack_queue_lock);
}


/*
 *  swapinStack: swap in a stack
 *
 *  We swap in stacks by wiring their memory.  We can just wire its memory right
 *  away.  If there are other stacks in that memory, no problem, they just end up
 *  resident too.
 */
void
swapinStack(vm_offset_t stack)
{
	int i, swapAll;
	vm_offset_t page = trunc_page(stack);
	vm_offset_t thisStack;

	stack -= sizeof(struct _kernelStack);
	stackStats.swappableStacks--;
	
	(void) vm_map_pageable(kernel_map, page,
			       round_page(page + kernelStackBlock), FALSE);
	
	lock_write(&stack_queue_lock);
	/*
	 *  Mark our particular stack in use.
	 */
	((kernelStack) stack)->status = STACK_IN_USE;
	
	/*
	 *  Check the magic hack to see if we've already put this stuff on the free
	 *  list.
	 */
	if (((kernelStack) page)->freeList.next != (struct queue_entry *) 0xfeedface) {
		lock_done(&stack_queue_lock);
		return;
	}
		
	((kernelStack) page)->freeList.next = (struct queue_entry *) 0;
	stackStats.swappedChunks--;
		
	/*
	 *  Scan through the memory we just brought in and put free stacks on
	 *  the free list.
	 */
	thisStack = page;
	swapAll = TRUE;
	for (i = 0; i < stacksPerPage; i++) {
		if ( ((kernelStack) thisStack)->status == STACK_FREE ) {
			enterFreeList(thisStack);
		}
			
		thisStack += kernelStackBlock;
	}	
	lock_done(&stack_queue_lock);
}

boolean_t stack_alloc_try(
	thread_t	thread,
	void		(*resume)(thread_t))
{
	register vm_offset_t stack = _allocStack(FALSE);
	
	if (!stack)
		stack = thread->stack_privilege;
	if (stack) {
		stack_attach(thread, stack, resume);
		return TRUE;
	}

	return FALSE;
}

void stack_alloc(
	thread_t	thread,
	void		(*resume)(thread_t))
{
	vm_offset_t stack;
	
	stack = allocStack();
	if (!stack)
		panic("stack_alloc");

	stack_attach(thread, stack, resume);
}

void stack_free(
	thread_t thread)
{
	register vm_offset_t stack;

	stack = stack_detach(thread);

	if (stack != thread->stack_privilege)
		freeStack(stack);
}

void stack_collect(void)
{
}

#if	MACH_DEBUG

void stack_statistics(
	natural_t	*totalp,
	vm_size_t	*maxusagep)
{
	extern boolean_t stack_check_usage;

	lock_read(&stack_queue_lock);
	if (stack_check_usage) {
		vm_offset_t stack;
		
		for (stack = (vm_offset_t)queue_first(&stack_queue);
			!queue_end(&stack_queue, (queue_entry_t)stack);
				stack = (vm_offset_t)
					queue_next((queue_entry_t)stack)) {
			vm_size_t usage =
				stack_usage(stack +
					sizeof (struct _kernelStack));
					
			if (usage > *maxusagep)
				*maxusagep = usage;
		}
	}
	
	*totalp = stack_free_count;
	lock_done(&stack_queue_lock);
}

#endif	/* MACH_DEBUG */
