##
# Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
# 
# Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
# Reserved.  This file contains Original Code and/or Modifications of
# Original Code as defined in and that are subject to the Apple Public
# Source License Version 1.1 (the "License").  You may not use this file
# except in compliance with the License.  Please obtain a copy of the
# License at http://www.apple.com/publicsource and read it before using
# this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
# License for the specific language governing rights and limitations
# under the License.
# 
# @APPLE_LICENSE_HEADER_END@
##

#
#	.gdbinit for kernel debugging
#
#	Created and Maintained by :
#		Umesh Vaishampayan  (umeshv@apple.com)
#
#   Contributions from:
#		Conrad Minshall	(conrad@apple.com)
#		Eryk Vershen, Tom Mason
#

# Set prettyprinting of structures
set print pretty
# Set printing of unions interior to structures
set print union

# Unlimited page size.
set height 0

#
# Macros to print stacktrace on PPC
# Typical usage :
#		kstack <thread-addr>
#		tstack
#		tstack
#		...
#
# For the active_thread(), following would be slightly faster
#		ststack
#		tstack
#		tstack
#		....
#
define kstack
    set $s=(thread_t)$arg0
    if (($s->kernel_stack) == 0)
	echo "no kernel stack\n"
    else
	if $arg0 == cpu_data->active_thread
	    ststack
	else
	    set $f=((struct ppc_kernel_state *)(($s->kernel_stack)+0x3f94))->r1
	    p/x $f
	    x/i ((struct ppc_kernel_state *)(($s->kernel_stack)+0x3f94))->lr
	end
    end
end
document kstack
	Start a stack backtrace for the thread passed in as first argument
	must be called before calling 'tstack'
	(for PPC only)
end

define ststack
        set $f=(long*)*$r1
	p/x $f
	x/i *($f+2)-4
end
document ststack
	Start a stack backtrace for the active_thread()
	must be called before calling 'tstack'
	(for PPC only)
end

define tstack
    if (*(long*)$f) != 0
        set $f=*(long*)$f
        p/x $f
        x/i *($f+8)-4
    else
        echo end of stack\n
    end
end
document tstack
	Print top of the stack and pop a frame
	must be called after 'kstack' or 'ststack'
	(for PPC only)
end

#
# Other stack related macros
#

define tsaved
	p/x *(struct ppc_saved_state *)($f+0x90)
end
document tsaved
	Print the saved state for the current frame.
	must be used after 'tstack' or 'kstack' or ststack'.
end

#
# Macros to print addresses as various structures
#
define pcb
    p *(pcb_t)$arg0
end
document pcb
	Print first argument as a pcb_t.
end

define pth
    p *(thread_t)$arg0
end
document pth
	Print first argument as a thread_t.
end

define pswapf
	p *((thread_t)$arg0)->swap_func
end
document pswapf
	Print the swap_func of a thread
end

define pta
    p *(task_t)$arg0
end
document pta
	Print first argument as a task_t.
end

define dmapent
    echo "  map_entry"
    p $arg0
    echo "    start "
    p ((vm_map_entry_t)$arg0)->links.start
    echo "    end "
    p ((vm_map_entry_t)$arg0)->links.end
    if ( $arg0->is_a_map != 0)
        echo "    object is a map"\n
        echo "    share"
	p $arg0->object.share_map
        echo "    offset"
	p $arg0->offset
    end
    if ( $arg0->is_shared != 0)
        echo "    region is shared"\n
    end
    if ( $arg0->is_sub_map != 0)
        echo "    object is a sub map"\n
    end
    if ( $arg0->copy_on_write != 0)
        echo "    data is copy on write"\n
    end
    if ( $arg0->in_transition != 0)
        echo "    Entry is being changed"\n
    end
    if ( $arg0->needs_wakeup != 0)
        echo "    Waiters on in_transition"\n
    end
    if ( $arg0->needs_copy != 0)
        echo "    object needs to be copied"\n
    end
    echo "    protection "
    p ((vm_map_entry_t)$arg0)->protection
    echo "    max protection "
    p ((vm_map_entry_t)$arg0)->max_protection
    echo "    inheritance "
    p ((vm_map_entry_t)$arg0)->inheritance
    echo "    wired count "
    p ((vm_map_entry_t)$arg0)->wired_count
    echo "    user_wired_count "
    p ((vm_map_entry_t)$arg0)->user_wired_count
end
document dmapent
	Pretty print the first argument as a vm_map_entry_t.
end

define dmap
    set $vmmapptr=$arg0
    set $iter=0
    if (((vm_map_t)$vmmapptr)->is_main_map != 0)
	echo "    this is a main map"\n
    end
    echo "    start "
    p ((vm_map_t)$vmmapptr)->hdr.links.start
    echo "    end "
    p ((vm_map_t)$vmmapptr)->hdr.links.end
    if ((vm_map_t)$vmmapptr->entries_pageable != 0)
        echo "    map entries are pageable"\n
    else 
        echo "    map entries are NOT pageable"\n
    end
    echo "    size "
    p ((vm_map_t)$vmmapptr)->size
    echo "    pmap "
    p ((vm_map_t)$vmmapptr)->pmap
    set $mapentptr=$vmmapptr->hdr.links.next
    echo "    number of map entries "
    p ((vm_map_t)$vmmapptr)->hdr.nentries
    while ($iter < ((vm_map_t)$arg0)->hdr.nentries)
	echo \n
	echo "  entry "
	p $iter
	dmapent $mapentptr
	set $mapentptr=((vm_map_entry_t)$mapentptr)->links.next
	set $iter=$iter+1
    end
end
document dmap
	Pretty print the first argument as a vm_map_t.
end

#
# List all the tasks.
# Print interesting information in the following order :
#	Name of the task    [task->proc->p_comm]
#	The task 			[task]
#	map					[task->map]
#
define tasks
    set $numtasks=default_pset->task_count
    set $iter=0
    set $taskptr=default_pset->tasks.next
    while ($iter < $numtasks)
	if ((((struct task *)($taskptr))->proc) != 0)
	    p (char *)(((struct task *)($taskptr))->proc->p_comm)
		p $taskptr
		p ((struct task *)($taskptr))->map
	end
	set $taskptr=((struct task *)($taskptr))->pset_tasks.next
	set $iter=$iter+1
    end
end
document tasks
	Print all the tasks and maps associated with each one of them.
end

#
# List all the threads.
# Print interesting information in the following order :
#	Name of the task    [thread->task->proc->p_comm]
#	The thread 			[thread]
#	
define threads
    set $numthreads=default_pset->thread_count
    set $iter=0
    set $threadptr=default_pset->threads.next
    while ($iter < $numthreads)
        if ((((struct thread *)($threadptr))->task->proc) != 0) 
		p (char *)(((struct thread *)($threadptr))->task->proc->p_comm)
		p $threadptr
	    end
	set $threadptr=((struct thread *)($threadptr))->pset_threads.next
	set $iter=$iter+1
    end
end
document threads
	Print all the threads and name of the task associated with each one of them.
end

#
# List all the threads with a nonzero value of kernel_stack.
# Print interesting information in the following order :
#	Name of the task    [thread->task->proc->p_comm]
#	The thread 			[thread]
#
define kstackthreads
    set $numthreads=default_pset->thread_count
    set $iter=0
    set $threadptr=default_pset->threads.next
    while ($iter < $numthreads)
	if ((((struct thread *)($threadptr))->kernel_stack) != 0)
	    if ((((struct thread *)($threadptr))->task->proc) != 0)
		p (char *)(((struct thread *)($threadptr))->task->proc->p_comm)
		p $threadptr
	    end
	end
	set $threadptr=((struct thread *)($threadptr))->pset_threads.next
	set $iter=$iter+1
    end
end
document kstackthreads
	Print all the threads with nonzero kernel_stack value,
	and name of the task associated with each one of them.
end

#
# List all the threads with a nonzero value of wait_event.
# Print interesting information in the following order :
#	Name of the task    [thread->task->proc->p_comm]
#	The thread 			[thread]
#   The wait event		[thread->wait_event]
#	The wait message    [thread->wait_mesg]
#
define waitthreads
    set $numthreads=default_pset->thread_count
    set $iter=0
    set $threadptr=default_pset->threads.next
    while ($iter < $numthreads)
	if ((((struct thread *)($threadptr))->wait_event) != 0)
	    p (char *)(((struct thread *)($threadptr))->task->proc->p_comm)
	    p $threadptr
		p ((struct thread *)($threadptr))->wait_event
		p ((struct thread *)($threadptr))->wait_mesg
	end
	set $threadptr=((struct thread *)($threadptr))->pset_threads.next
	set $iter=$iter+1
    end
end
document waitthreads
	Print all the threads with nonzero wait_event value,
	and name of the task associated with each one of them.
	Also print wait_event and wait_mesg.
end

define setRegFromSaved
	set $s_pc=(long)$pc
	set $s_lr=(long)$lr
	set $s_r1=(long)$r1
	set $s_r13=(long)$r13
	set $s_r14=(long)$r14
	set $s_r15=(long)$r15
	set $s_r16=(long)$r16
	set $s_r17=(long)$r17
	set $s_r18=(long)$r18
	set $s_r19=(long)$r19
	set $s_r20=(long)$r20
	set $s_r21=(long)$r21
	set $s_r22=(long)$r22
	set $s_r23=(long)$r23
	set $s_r24=(long)$r24
	set $s_r25=(long)$r25
	set $s_r26=(long)$r26
	set $s_r27=(long)$r27
	set $s_r28=(long)$r28
	set $s_r29=(long)$r29
	set $s_r30=(long)$r30
	set $s_r31=(long)$r31
	x/i $s_pc
end

define printReg
	printf "pc\t0x%x\n", $s_pc
	printf "lr\t0x%x\n", $s_lr
	printf "r1\t0x%x\n", $s_r1
	printf "r13\t0x%x\n", $s_r13
	printf "r14\t0x%x\n", $s_r14
	printf "r15\t0x%x\n", $s_r15
	printf "r16\t0x%x\n", $s_r16
	printf "r17\t0x%x\n", $s_r17
	printf "r18\t0x%x\n", $s_r18
	printf "r19\t0x%x\n", $s_r19
	printf "r20\t0x%x\n", $s_r20
	printf "r21\t0x%x\n", $s_r21
	printf "r22\t0x%x\n", $s_r22
	printf "r23\t0x%x\n", $s_r23
	printf "r24\t0x%x\n", $s_r24
	printf "r25\t0x%x\n", $s_r25
	printf "r26\t0x%x\n", $s_r26
	printf "r27\t0x%x\n", $s_r27
	printf "r28\t0x%x\n", $s_r28
	printf "r29\t0x%x\n", $s_r29
	printf "r30\t0x%x\n", $s_r30
	printf "r31\t0x%x\n", $s_r31
end

define popReg
	if (*(long*)$s_r1) == 0
		echo end of stack\n
	else
	set $s_r1=*(long*)$s_r1
	set $s_lr=*(long*)($s_r1+8)
	set $s_t1=$s_r1
	if $arg0 != 0
		if $arg0 <= 31
			set $s_t1=$s_t1-4
			set $s_r31=*(long*)$s_t1
		end
		if $arg0 <= 30
			set $s_t1=$s_t1-4
			set $s_r30=*(long*)$s_t1
		end
		if $arg0 <= 29
			set $s_t1=$s_t1-4
			set $s_r29=*(long*)$s_t1
		end
		if $arg0 <= 28
			set $s_t1=$s_t1-4
			set $s_r28=*(long*)$s_t1
		end
		if $arg0 <= 27
			set $s_t1=$s_t1-4
			set $s_r27=*(long*)$s_t1
		end
		if $arg0 <= 26
			set $s_t1=$s_t1-4
			set $s_r26=*(long*)$s_t1
		end
		if $arg0 <= 25
			set $s_t1=$s_t1-4
			set $s_r25=*(long*)$s_t1
		end
		if $arg0 <= 24
			set $s_t1=$s_t1-4
			set $s_r24=*(long*)$s_t1
		end
		if $arg0 <= 23
			set $s_t1=$s_t1-4
			set $s_r23=*(long*)$s_t1
		end
		if $arg0 <= 22
			set $s_t1=$s_t1-4
			set $s_r22=*(long*)$s_t1
		end
		if $arg0 <= 21
			set $s_t1=$s_t1-4
			set $s_r21=*(long*)$s_t1
		end
		if $arg0 <= 20
			set $s_t1=$s_t1-4
			set $s_r20=*(long*)$s_t1
		end
		if $arg0 <= 19
			set $s_t1=$s_t1-4
			set $s_r19=*(long*)$s_t1
		end
		if $arg0 <= 18
			set $s_t1=$s_t1-4
			set $s_r18=*(long*)$s_t1
		end
		if $arg0 <= 17
			set $s_t1=$s_t1-4
			set $s_r17=*(long*)$s_t1
		end
		if $arg0 <= 16
			set $s_t1=$s_t1-4
			set $s_r16=*(long*)$s_t1
		end
	end
	set $s_pc=$s_lr-4
	x/i $s_pc
	end
end

define wherePC
	x/i $s_pc
end

define popSaved
	set $s_t1=(struct ppc_saved_state *)($s_r1+0x90)
	set $s_pc=(long)$s_t1->srr0
	set $s_lr=(long)$s_t1->lr
	set $s_r1=(long)$s_t1->r1
	set $s_r13=(long)$s_t1->r13
	set $s_r14=(long)$s_t1->r14
	set $s_r15=(long)$s_t1->r15
	set $s_r16=(long)$s_t1->r16
	set $s_r17=(long)$s_t1->r17
	set $s_r18=(long)$s_t1->r18
	set $s_r19=(long)$s_t1->r19
	set $s_r20=(long)$s_t1->r20
	set $s_r21=(long)$s_t1->r21
	set $s_r22=(long)$s_t1->r22
	set $s_r23=(long)$s_t1->r23
	set $s_r24=(long)$s_t1->r24
	set $s_r25=(long)$s_t1->r25
	set $s_r26=(long)$s_t1->r26
	set $s_r27=(long)$s_t1->r27
	set $s_r28=(long)$s_t1->r28
	set $s_r29=(long)$s_t1->r29
	set $s_r30=(long)$s_t1->r30
	set $s_r31=(long)$s_t1->r31
	x/i $s_pc
end

define setRegFromThread
	set $s_t1=(thread_t)$arg0
	set $s_t1=(struct ppc_kernel_state *)(($s_t1->kernel_stack)+0x3f94)
	set $s_pc=(long)$s_t1->lr
	set $s_lr=(long)$s_t1->lr
	set $s_r1=(long)$s_t1->r1
	set $s_r13=(long)$s_t1->r13[0]
	set $s_r14=(long)$s_t1->r13[1]
	set $s_r15=(long)$s_t1->r13[2]
	set $s_r16=(long)$s_t1->r13[3]
	set $s_r17=(long)$s_t1->r13[4]
	set $s_r18=(long)$s_t1->r13[5]
	set $s_r19=(long)$s_t1->r13[6]
	set $s_r20=(long)$s_t1->r13[7]
	set $s_r21=(long)$s_t1->r13[8]
	set $s_r22=(long)$s_t1->r13[9]
	set $s_r23=(long)$s_t1->r13[10]
	set $s_r24=(long)$s_t1->r13[11]
	set $s_r25=(long)$s_t1->r13[12]
	set $s_r26=(long)$s_t1->r13[13]
	set $s_r27=(long)$s_t1->r13[14]
	set $s_r28=(long)$s_t1->r13[15]
	set $s_r29=(long)$s_t1->r13[16]
	set $s_r30=(long)$s_t1->r13[17]
	set $s_r31=(long)$s_t1->r13[18]
	x/i $s_pc
end

define popLR
	set $s_pc=$s_lr-4
	x/i $s_pc
end

#
# Difficult to ducument macros
#
define sustack
	set $u=(long*)((((int)$arg0) & 0x0FFFFFFF) + 0x60000000)
	p/x $u
	p/x *($u+2)-4
end

define ustack
	if (*(long*)$u) != 0
		set $u=(long*)((((int)(*(long*)$u)) & 0x0FFFFFFF) + 0x60000000)
		p/x $u
		if (((int)(*($u+2)-4)) & 0xF0000000) == 0
			x/i ((((int)(*($u+2)-4)) & 0x0FFFFFFF) + 0x20000000)
		else
			p/x *($u+2)-4
		end
	else
		echo end of user stack\n
	end
end

#
# Obscure macros for printing zones
#

define dcz
    p *(zone_t)$arg0
end
document dcz
	Print first argument as a zone_t.
end

define dzleak
    if (((zone_t)$arg0)->zone_name != 0)
	p (char *)(((zone_t)($arg0))->zone_name)
    end
    echo "    num elem used "
    p/x $arg0->count
    echo "    curr zone size "
    p/x $arg0->cur_size
    echo "    max zone size "
    p/x $arg0->max_size
    if ($arg0->free_elements !=0)
        echo "    next free element"
        p/x $arg0->free_elements
    end
    if ( $arg0->doing_alloc != 0)
        echo "    doing allocation"
        p/x $arg0->doing_alloc
    end
    echo "    allocation size"
    p/x $arg0->alloc_size
    if ( $arg0->pageable != 0)
        echo "    zone is pageable\n"
    end
    if ( $arg0->sleepable != 0)
        echo "    zone is sleepable\n"
    end
    if ( $arg0->exhaustible != 0)
        echo "    zone is exhaustible\n"
    end
    if ( $arg0->expandable != 0)
        echo "    zone is expandable\n"
    end
    set $iter=0
    while ($iter < $arg0->num_alloc_callers)
	p/x *(struct caller_s *)$arg0->alloc_callers[$iter]
	set $iter=$iter+1
    end
    echo "    number of alloc callers "
    p/x $arg0->num_alloc_callers
    set $iter=0
    while ($iter < $arg0->num_free_callers)
	p/x *(struct caller_s *)$arg0->free_callers[$iter]
	set $iter=$iter+1
    end
    echo "    number of free callers "
    p/x $arg0->num_free_callers
end

define dz
    if (((zone_t)$arg0)->zone_name != 0)
	p (char *)(((zone_t)($arg0))->zone_name)
    end
    echo "    num elem used "
    p/x $arg0->count
    echo "    curr zone size "
    p/x $arg0->cur_size
    echo "    max zone size "
    p/x $arg0->max_size
    if ($arg0->free_elements !=0)
        echo "    next free element"
        p/x $arg0->free_elements
    end
    if ( $arg0->doing_alloc != 0)
        echo "    doing allocation"
        p/x $arg0->doing_alloc
    end
    echo "    allocation size"
    p/x $arg0->alloc_size
    if ( $arg0->pageable != 0)
        echo "    zone is pageable\n"
    end
    if ( $arg0->sleepable != 0)
        echo "    zone is sleepable\n"
    end
    if ( $arg0->exhaustible != 0)
        echo "    zone is exhaustible\n"
    end
    if ( $arg0->expandable != 0)
        echo "    zone is expandable\n"
    end
end

define daz
    set $numtasks=default_pset->task_count
    set $iter=0
    set $zoneptr=zone_zone
    while ($zoneptr != 0)
	echo \n
	dz $zoneptr
	set $zoneptr=((zone_t)($zoneptr))->next_zone
	set $iter=$iter+1
    end
    echo number of zones = 
    p $iter
end

define dleakaz
    set $numtasks=default_pset->task_count
    set $iter=0
    set $zoneptr=zone_zone
    while ($zoneptr != 0)
    echo \n
    dzleak $zoneptr
    set $zoneptr=((zone_t)($zoneptr))->next_zone
    set $iter=$iter+1
    end
    echo number of zones =
    p $iter
end

#
# prints info about a TAILQ: how many are in the list and total KB
#
define tqcount
    echo $arg0\ $arg2\ length\ is
    set $tqptr=($arg0).tqh_first
    set $iter=0
    set $size=0
    while ($tqptr)
        set $size=$size + ((struct $arg1 *)$tqptr)->$arg3
        set $tqptr=((struct $arg1 *)$tqptr)->$arg2.tqe_next
        set $iter=$iter + 1
    end
    printf " %d kbytes is 0x%x\n",$iter,$size/1024
end
document tqcount
	prints info about a TAILQ: how many are in the list and total KB
end

#
# prints info about the buffer cache queues
#
define bufinfo
    printf "nbuf is %d\n",nbuf
    printf "niobuf is %d\n",niobuf
    printf "bufpages is %d\n",bufpages
    printf "LOCKED "
    tqcount bufqueues[0] buf b_freelist b_bufsize
    printf "LRU "
    tqcount bufqueues[1] buf b_freelist b_bufsize
    printf "AGE "
    tqcount bufqueues[2] buf b_freelist b_bufsize
    printf "EMPTY "
    tqcount bufqueues[3] buf b_freelist b_bufsize
    tqcount iobufqueue buf b_freelist b_bufsize
end
document bufinfo
	prints info about the buffer cache queues
end
