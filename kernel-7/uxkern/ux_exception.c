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
 *********************************************************************
 *
 *  1-Oct-87  David Black (dlb) at Carnegie-Mellon University
 *	Created
 *
 **********************************************************************
 */

#import <sys/param.h>

#import <mach/boolean.h>
#import <mach/exception.h>
#import <mach/kern_return.h>
#import <mach/message.h>
#import <mach/port.h>
#import <kern/task.h>
#import <kern/thread.h>
#import <sys/proc.h>
#import <sys/ux_exception.h>
#import <mach/mig_errors.h>
#import <kern/kalloc.h>

#import <mach/mach_user_internal.h>

#import <kern/sched_prim.h>

/*
 *	Unix exception handler.
 */

static void	ux_exception();

decl_simple_lock_data(static,	ux_handler_init_lock)
port_t				ux_exception_port;
static port_t			ux_handler_self;

static
void
ux_handler(void)
{
    task_t		self = current_task();
    port_name_t		exc_port_name;
    port_set_name_t	exc_set_name;


    self->kernel_vm_space = TRUE;
    ux_handler_self = task_self();

    simple_lock(&ux_handler_init_lock);

    /*
     *	Allocate a port set that we will receive on.
     */
    if (port_set_allocate(ux_handler_self, &exc_set_name) != KERN_SUCCESS)
	    panic("ux_handler: port_set_allocate failed");

    /*
     *	Allocate an exception port and use object_copyin to
     *	translate it to the global name.  Put it into the set.
     */
    if (port_allocate(ux_handler_self, &exc_port_name) != KERN_SUCCESS)
	panic("ux_handler: port_allocate failed");
    if (port_set_add(ux_handler_self,
    			exc_set_name, exc_port_name) != KERN_SUCCESS)
	panic("ux_handler: port_set_add failed");
    if (!object_copyin(self, exc_port_name,
			MSG_TYPE_PORT, FALSE,
			(void *) &ux_exception_port))
	panic("ux_handler: object_copyin(ux_exception_port) failed");

    /*
     *	Release kernel to continue.
     */
    thread_wakeup(&ux_exception_port);
    simple_unlock(&ux_handler_init_lock);

    /* Message handling loop. */

    for (;;) {
	struct rep_msg {
	    msg_header_t	h;
	    int			d[4];
	} rep_msg;
	struct exc_msg {
	    msg_header_t	h;
	    int			d[16];	/* max is actually 10 */
	} exc_msg;
	port_t			reply_port;
	msg_return_t		result;

	exc_msg.h.msg_local_port = exc_set_name;
	exc_msg.h.msg_size = sizeof (exc_msg);

	result = msg_receive(&exc_msg.h, MSG_OPTION_NONE, 0);

	if (result == RCV_SUCCESS) {
	    reply_port = exc_msg.h.msg_remote_port;

	    if (exc_server(&exc_msg.h, &rep_msg.h))
		(void) msg_send(&rep_msg.h, MSG_OPTION_NONE, 0);

	    if (reply_port != PORT_NULL)
		(void) port_deallocate(ux_handler_self, reply_port);
	}
	else if (result == RCV_TOO_LARGE)
		/* ignore oversized messages */;
	else
		panic("exception_handler");
    }
}

void
ux_handler_init(void)
{
    task_t	handler_task;

    simple_lock_init(&ux_handler_init_lock);
    ux_exception_port = PORT_NULL;
    handler_task = kernel_task_create(kernel_task, 0);
    (void) kernel_thread(handler_task, ux_handler, (void *)0);
    simple_lock(&ux_handler_init_lock);
    if (ux_exception_port == PORT_NULL) 
	    thread_sleep(&ux_exception_port,
		    simple_lock_addr(ux_handler_init_lock), FALSE);
    else
	    simple_unlock(&ux_handler_init_lock);
}

kern_return_t
catch_exception_raise(
    port_t		exception_port,
    port_t		thread_name,
    port_t		task_name,
    int			exception,
    int			code,
    int			subcode
)
{
    task_t		self = current_task();
    thread_t		thread;
    port_t		thread_port;
    kern_return_t	result = KERN_SUCCESS;
    int			signal = 0;
    u_long		ucode = 0;

    /*
     *	Convert local thread name to global port.
     */

    if (object_copyin(self, thread_name,
		       MSG_TYPE_PORT, FALSE,
		       (void *) &thread_port)) {

	thread = convert_port_to_thread(thread_port);
	port_release(thread_port);

	/*
	 *	Catch bogus ports
	 */
	if (thread != THREAD_NULL) {
    
	    /*
	     *	Convert exception to unix signal and code.
	     */
	    ux_exception(exception, code, subcode, &signal, &ucode);

	    /*
	     *	Send signal.
	     */
	    if (signal != 0)
		threadsignal(thread, signal, ucode);

	    thread_deallocate(thread);
	}
	else
	    result = KERN_INVALID_ARGUMENT;
    }
    else
    	result = KERN_INVALID_ARGUMENT;

    /*
     *	Delete our send rights to the task and thread ports.
     */
    (void) port_deallocate(ux_handler_self, task_name);
    (void) port_deallocate(ux_handler_self, thread_name);

    return (result);
}


boolean_t	machine_exception();

/*
 *	ux_exception translates a mach exception, code and subcode to
 *	a signal and u.u_code.  Calls machine_exception (machine dependent)
 *	to attempt translation first.
 */

static
void ux_exception(
    int			exception,
    int			code,
    int			subcode,
    int			*ux_signal,
    int			*ux_code
)
{
    /*
     *	Try machine-dependent translation first.
     */
    if (machine_exception(exception, code, subcode, ux_signal, ux_code))
	return;
	
    switch(exception) {

    case EXC_BAD_ACCESS:
	if (code == KERN_INVALID_ADDRESS)
	    *ux_signal = SIGSEGV;
	else
	    *ux_signal = SIGBUS;
	break;

	case EXC_BAD_INSTRUCTION:
	    *ux_signal = SIGILL;
	    break;

	case EXC_ARITHMETIC:
	    *ux_signal = SIGFPE;
	    break;

	case EXC_EMULATION:
	    *ux_signal = SIGEMT;
	    break;

	case EXC_SOFTWARE:
	    switch (code) {

	    case EXC_UNIX_BAD_SYSCALL:
		*ux_signal = SIGSYS;
		break;
	    case EXC_UNIX_BAD_PIPE:
		*ux_signal = SIGPIPE;
		break;
	    case EXC_UNIX_ABORT:
		*ux_signal = SIGABRT;
		break;
	    }
	    break;

	case EXC_BREAKPOINT:
	    *ux_signal = SIGTRAP;
	    break;
    }
}
