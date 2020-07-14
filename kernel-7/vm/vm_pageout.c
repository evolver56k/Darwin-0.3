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
 *	File:	vm/vm_pageout.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	The proverbial page-out daemon.
 */

#import <vm/vm_page.h>
#import <vm/pmap.h>
#import <vm/vm_object.h>
#import <vm/vm_pageout.h>
#import <mach/vm_statistics.h>
#import <mach/vm_param.h>
#import <kern/thread.h>
#import <machine/spl.h>

simple_lock_data_t	vm_pages_needed_lock;

int	vm_pages_needed;		/* Event on which pageout daemon sleeps */
int	vm_pageout_free_min = 0;	/* Stop pageout to wait for pagers at this free level */

int	vm_page_free_min_sanity = 256*1024;

#if	MACH_VM_DEBUG
long	vm_page_total_pages = 0;
long	local_total_pages;
#endif	/* MACH_VM_DEBUG */

/*
 *	vm_pageout_scan does the dirty work for the pageout daemon.
 */
boolean_t
vm_pageout_scan()
{
	register vm_page_t	m;
	register int		page_shortage;
	register int		s;
	register int		pages_freed = 0;
	register int            pages_cleaned = 0;
	boolean_t		free_pages;
	boolean_t		did_work = FALSE;
#if	MACH_VM_DEBUG
	int			dbg_intoa_pages = 0;
	int			dbg_pages_needed = 0;
	int			dbg_pages_cleaned = 0;
#endif	/* MACH_VM_DEBUG */


	/*
	 *	Only continue when we want more pages to be "free"
	 */
		s = splimp();
		simple_lock(&vm_page_queue_free_lock);

		free_pages = FALSE;
		if (vm_page_free_count <= vm_page_free_min) {
			free_pages = TRUE;
			/*
		 	 *	See whether the physical mapping system
		 	 *	knows of any pages which are not being used.
		 	 */
		 
			simple_unlock(&vm_page_queue_free_lock);
			splx(s);

			/*
		 	 *	And be sure the pmap system is updated so
		 	 *	we can scan the inactive queue.
		 	 */

			pmap_update();
		}
		else {
			simple_unlock(&vm_page_queue_free_lock);
			splx(s);
		}

	/*
	 *	Acquire the resident page system lock,
	 *	as we may be changing what's resident quite a bit.
	 */
	vm_page_lock_queues();

	/*
	 *	Start scanning the inactive queue for pages we can free.
	 *	We keep scanning until we have enough free pages or
	 *	we have scanned through the entire queue.  If we
	 *	encounter dirty pages, we start cleaning them.
	 */

	pages_freed = 0;
	m = (vm_page_t) queue_first(&vm_page_queue_inactive);
	while (free_pages &&
	    !queue_end(&vm_page_queue_inactive, (queue_entry_t) m)) {
		vm_page_t	next;

			s = splimp();
			simple_lock(&vm_page_queue_free_lock);
			if ((vm_page_free_count + pages_cleaned) >= vm_page_free_target) {
				simple_unlock(&vm_page_queue_free_lock);
				splx(s);
				break;
			}
			simple_unlock(&vm_page_queue_free_lock);
			splx(s);

#if 0
		if (pmap_is_referenced(VM_PAGE_TO_PHYS(m)) &&
		    (vm_page_inactive_target < vm_page_inactive_count))
#else
		if (pmap_is_referenced(VM_PAGE_TO_PHYS(m)))
#endif
		{
			next = (vm_page_t) queue_next(&m->pageq);
			vm_page_activate(m);
			vm_stat.reactivations++; 
			m = next;
#if	MACH_VM_DEBUG
			dbg_intoa_pages++;
#endif	/* MACH_VM_DEBUG */
			continue;
		}

		if (m->clean) {
				register vm_object_t	object;

				next = (vm_page_t) queue_next(&m->pageq);
				object = m->object;
				if (!vm_object_lock_try(object)) {
					/*
				 	 *	Can't lock object -
				 	 *	skip page.
				 	 */
					m = next;
					continue;
				}
				did_work = TRUE;
				m->busy = TRUE;
				vm_page_unlock_queues();

				pmap_remove_all(VM_PAGE_TO_PHYS(m));
				vm_page_lock_queues();
				m->busy = FALSE;
				PAGE_WAKEUP(m);
			
				next = (vm_page_t) queue_next(&m->pageq);
				vm_page_addfree(m);
				pages_freed++;
#if	MACH_VM_DEBUG
			--dbg_pages_needed;
#endif	/* MACH_VM_DEBUG */
				vm_object_unlock(object);
				m = next;
		}
		else {
			/*
			 *	If a page is dirty, then it is either
			 *	being washed (but not yet cleaned)
			 *	or it is still in the laundry.  If it is
			 *	still in the laundry, then we start the
			 *	cleaning operation.
			 */

#if	0 && MACH_VM_DEBUG
	kprintf("     : pulled dirty page off inactive list\n");
#endif	/* MACH_VM_DEBUG */
			if (m->laundry) {
				/*
				 *	Clean the page and remove it from the
				 *	laundry.
				 *
				 *	We set the busy bit to cause
				 *	potential page faults on this page to
				 *	block.
				 *
				 *	And we set pageout-in-progress to keep
				 *	the object from disappearing during
				 *	pageout.  This guarantees that the
				 *	page won't move from the inactive
				 *	queue.  (However, any other page on
				 *	the inactive queue may move!)
				 */

				register vm_object_t	object;
				register vm_pager_t	pager;
				boolean_t	pageout_succeeded;

#if	0 && MACH_VM_DEBUG
	kprintf("    cleaning dirty page off inactive list\n");
#endif	/* MACH_VM_DEBUG */
				object = m->object;
				if (!vm_object_lock_try(object)) {
					/*
					 *	Skip page if we can't lock
					 *	its object
					 */
					m = (vm_page_t) queue_next(&m->pageq);
					continue;
				}

				m->busy = TRUE;
				vm_stat.pageouts++;

				/*
				 *	Try to collapse the object before
				 *	making a pager for it.  We must
				 *	unlock the page queues first.
				 */
				vm_page_unlock_queues();

				did_work = TRUE;
				/*
				 * Moved this call from inside the queue lock
				 * to prevent the following scenario:
				 * remove_all -> pmap_collapse ->
				 * kmem_free -> vm_page_lock_queues
				 */
				pmap_remove_all(VM_PAGE_TO_PHYS(m));

				vm_object_collapse(object);

				object->paging_in_progress++;

				vm_object_unlock(object);


				/*
				 *	Do a wakeup here in case the following
				 *	operations block.
				 */
				thread_wakeup(&vm_page_free_count);

				/*
				 *	If there is no pager for the page,
				 *	use the default pager.  If there's
				 *	no place to put the page at the
				 *	moment, leave it in the laundry and
				 *	hope that there will be paging space
				 *	later.
				 */

				if ((pager = object->pager) == vm_pager_null) {
					pager = (vm_pager_t)vm_pager_allocate(
							object->size);
					if (pager != vm_pager_null) {
						vm_object_setpager(object,
							pager, 0, FALSE);
					}
				}

				pageout_succeeded = FALSE;
				if (pager != vm_pager_null) {
				    if (vm_pager_put(pager, m) == PAGER_SUCCESS) {
					pageout_succeeded = TRUE;
#if	MACH_VM_DEBUG
					dbg_pages_cleaned++;
#endif	/* MACH_VM_DEBUG */
					pages_cleaned++;
				    }
				}

				vm_object_lock(object);
				vm_page_lock_queues();

				/*
				 *	If page couldn't be paged out, then
				 *	reactivate the page so it doesn't
				 *	clog the inactive list.  (We will try
				 *	paging out it again later).
				 */
				next = (vm_page_t) queue_next(&m->pageq);
				if (pageout_succeeded)
					m->laundry = FALSE;
				else
					vm_page_activate(m);

/* Why are we doing this. Was it not cleared when the mappings were removed
 * Is this here for a different arch */
				pmap_clear_reference(VM_PAGE_TO_PHYS(m));
					m->busy = FALSE;
					PAGE_WAKEUP(m);

					object->paging_in_progress--;
					thread_wakeup(object);
				vm_object_unlock(object);
				m = next;
			}
			else
				m = (vm_page_t) queue_next(&m->pageq);
		}
	}

	/*
	 *	Compute the page shortage.  If we are still very low on memory
	 *	be sure that we will move a minimal amount of pages from active
	 *	to inactive.
	 */

	page_shortage = vm_page_inactive_target - vm_page_inactive_count;
	page_shortage -= vm_page_free_count;

//	if ((page_shortage <= 0) && (pages_freed == 0))
//		page_shortage = 1;

	while (page_shortage > 0) {
		/*
		 *	Move some more pages from active to inactive.
		 */

		if (queue_empty(&vm_page_queue_active)) {
			break;
		}
		did_work = TRUE;
		m = (vm_page_t) queue_first(&vm_page_queue_active);
		vm_page_deactivate(m);
		page_shortage--;
	}

	vm_page_unlock_queues();

	if (!pages_cleaned && !pages_freed && did_work) {
		/*
		 * We did not issue any IO in the loop and did not free any
		 * pages. Yield to someone before proceeding so that we do
		 * not go compute bound, which will prevent the threads trying
		 * to do a biodone() from running. If that does not happen,
		 * no forward progress would be made.
		 */
		thread_will_wait(current_thread());
		thread_set_timeout(2);	/* 2 ticks */
		thread_block();
	}

	return did_work;
}


/*
 *	vm_pageout is the high level pageout daemon.
 */

void vm_pageout()
{
	thread_t	self = current_thread();
	boolean_t	vm_pageout_scan_did_work = TRUE;

	stack_privilege(self);
	self->sched_pri = self->priority = MAXPRI_USER;
	self->policy = POLICY_FIXEDPRI;
	self->sched_data = min_quantum;
	self->vm_privilege = TRUE;

	(void) spl0();

	/*
	 *	Initialize some paging parameters.
	 */

	if (vm_page_free_min == 0) {
//		vm_page_free_min = vm_page_free_count / 20;
		/*
		** This is 2.00% of available free pages
		*/
		vm_page_free_min = vm_page_free_count / 50;
		if (vm_page_free_min < 3)
			vm_page_free_min = 3;

		if (vm_page_free_min*PAGE_SIZE > vm_page_free_min_sanity)
			vm_page_free_min = vm_page_free_min_sanity/PAGE_SIZE;
	}

	if (vm_page_free_reserved == 0) {
		/*
		** This is 0.50% of available free pages
		*/
		if ((vm_page_free_reserved = vm_page_free_min / 4) < 3)
			vm_page_free_reserved = 3;
	}
	if (vm_pageout_free_min == 0) {
		if ((vm_pageout_free_min = vm_page_free_reserved / 2) > 10)
			vm_pageout_free_min = 10;
	}

	if (vm_page_free_target == 0)
//		vm_page_free_target = (vm_page_free_min * 4) / 3;
		/*
		** This is 8.00% of available free pages
		*/
		vm_page_free_target = vm_page_free_min * 4;

	if (vm_page_inactive_target == 0)
		vm_page_inactive_target = vm_page_free_count / 3;

	if (vm_page_free_target <= vm_page_free_min)
		vm_page_free_target = vm_page_free_min + 1;

	if (vm_page_inactive_target <= vm_page_free_target)
		vm_page_inactive_target = vm_page_free_target + 1;

#if	MACH_VM_DEBUG
	kprintf("vm_pageout: free_count=0x%X\n",
	    vm_page_free_count);
	kprintf("vm_pageout: free_min=0x%X, free_reserved=0x%X\n",
	    vm_page_free_min, vm_page_free_reserved);
	kprintf("vm_pageout: free_target=0x%X, inactive_target=0x%X\n",
	    vm_page_free_target, vm_page_inactive_target);
#endif

	/*
	 *	The pageout daemon is never done, so loop
	 *	forever.
	 */

	simple_lock(&vm_pages_needed_lock);
	while (TRUE) {
		if (!vm_pageout_scan_did_work)
			thread_sleep(&vm_pages_needed,
				     &vm_pages_needed_lock, FALSE);
		else
		if ((vm_page_free_count > vm_page_free_min) &&
		    ((vm_page_free_count >= vm_page_free_target) ||
		     (vm_page_inactive_count > vm_page_inactive_target)))
			thread_sleep(&vm_pages_needed,
				     &vm_pages_needed_lock, FALSE);
		else{
			simple_unlock(&vm_pages_needed_lock);
		}
		vm_pageout_scan_did_work = vm_pageout_scan();
		simple_lock(&vm_pages_needed_lock);
		thread_wakeup(&vm_page_free_count);
	}
}
