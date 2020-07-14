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
/*	@(#)kl_com.m	2.0	27/07/90	(c) 1990 NeXT	*/

/* 
 * kl_com.m -- common kern_loader communication module.
 *
 * HISTORY
 * 27-Jul-90	Doug Mitchell at NeXT
 *	Cloned from DOS source.
 */

#import <mach/mach_traps.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import <bsd/libc.h>
#import <ansi/stdio.h>
#import <bsd/strings.h>
#import <mach/mach_error.h>
#import <mach/mig_errors.h>
#import <mach/cthreads.h>
#import <servers/bootstrap.h>
#import <kernserv/kern_loader_error.h>
#import <kernserv/kern_loader_types.h>
#import <kernserv/kern_loader.h>
#import <kernserv/kern_loader_reply_handler.h>
#import <bsd/sys/param.h>
#import <mach/notify.h>
#import <mach/message.h>
#import "kl_com.h"
#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>

static int kl_init();
static int kl_com_log(port_name_t port);
static kern_return_t print_string (
	void		*arg,
	printf_data_t	string,
	u_int		stringCnt,
	int		level);
static kern_return_t ping (
	void		*arg,
	int		id);
static void kl_com_error(char *err_str, kern_return_t rtn);

port_name_t kl_port;
port_name_t kernel_task, reply_port;
boolean_t kl_init_flag = FALSE;
static id ping_lock;

/*
 * Set up ports, etc. for kern_loader communication.
 */
static int 
kl_init() {
	kern_return_t r;
	boolean_t isactive;

	if(kl_init_flag)
		return(0);
	r = kern_loader_look_up(&kl_port);
	if (r != KERN_SUCCESS) {
		kl_com_error("can't find kernel loader", r);
		return(1);
	}

	r = task_by_unix_pid(task_self(), 0, &kernel_task);
	if (r != KERN_SUCCESS) {
		kl_com_error("cannot get kernel task port\n", r);
		return(1);
	}

	r = port_allocate(task_self(), &reply_port);
	if (r != KERN_SUCCESS) {
		kl_com_error("can't allocate reply port", r);
		return(1);
	}

	r = bootstrap_status(bootstrap_port, KERN_LOADER_NAME, &isactive);
	if (r != KERN_SUCCESS) {
		kl_com_error("can't find kernel loader status", r);
		return(1);
	}
	/*
	 * Create a thread to listen on reply_port
	 * and print out anything that comes back.
	 */
	cthread_init();
	cthread_set_name(cthread_self(), "command thread");
	cthread_detach(cthread_fork((cthread_fn_t)kl_com_log,
		(any_t)reply_port));

	if (!isactive)
		printf("kernel loader inactive, pausing\n");

	r = kern_loader_status_port(kl_port, reply_port);
	if(r) {
		kl_com_error("can't get status back\n", r);
		return(1);
	}
	ping_lock = [[NXConditionLock alloc] initWith:FALSE];
	kl_init_flag = TRUE;
	return(0);
}


kern_loader_reply_t kern_loader_reply = {
	0,
	0,
	print_string,
	ping
};

/*
 * wait for reply messages.
 */
static int 
kl_com_log(port_name_t port)
{
	char msg_buf[kern_loader_replyMaxRequestSize];
	msg_header_t *msg = (msg_header_t *)msg_buf;
	port_name_t notify_port;
	port_set_name_t port_set;
	kern_return_t r;

	r = port_allocate(task_self(), &notify_port);
	if (r != KERN_SUCCESS) {
		kl_com_error("allocating notify port", r);
		return(1);
	}

	r = task_set_notify_port(task_self(), notify_port);
	if (r != KERN_SUCCESS) {
		kl_com_error("cannot set notify port", r);
		return(1);
	}

	r = port_set_allocate(task_self(), &port_set);
	if (r != KERN_SUCCESS) {
		kl_com_error("allocating port set", r);
		return(1);
	}

	r = port_set_add(task_self(), port_set, notify_port);
	if (r != KERN_SUCCESS) {
		kl_com_error("adding notify port to port set", r);
		return(1);
	}

	r = port_set_add(task_self(), port_set, port);
	if (r != KERN_SUCCESS) {
		kl_com_error("adding listener port to port set", r);
		return(1);
	}
	while(1) {
		msg->msg_size = kern_loader_replyMaxRequestSize;
		msg->msg_local_port = port_set;
		r = msg_receive(msg, MSG_OPTION_NONE, 0);
		if (r != KERN_SUCCESS) {
			kl_com_error("log_thread receive", r);
			return(1);
		}
		if (msg->msg_local_port == notify_port) {
			notification_t *n = (notification_t *)msg;
			
			if(msg->msg_id == NOTIFY_PORT_DELETED) {
			    if(n->notify_port == kl_port) {
				printf("kdb: kern_loader port "
					"deleted\n");
				continue;
			    }
			    else {
				printf("kdb: port death detected"
					"(port %d)\n", n->notify_port);
				continue;
			    }
			}
			else {
				printf("kdb: weird notification "
					"(msg_id %d)\n", msg->msg_id);
				continue;
			}
		} 
		else 
			kern_loader_reply_handler(msg, &kern_loader_reply);
	}
	return(0);
}

static kern_return_t 
print_string (
	void		*arg,
	printf_data_t	string,
	u_int		stringCnt,
	int		level)
{
#if	DEBUG
	if (stringCnt == 0 || !string)
		return KERN_SUCCESS;
	fputs(string, stdout);
	fflush(stdout);
#endif	DEBUG
	return KERN_SUCCESS;
}

static kern_return_t 
ping (
	void		*arg,
	int		id)
{
	[ping_lock lockWhen:FALSE];
	[ping_lock unlockWith:TRUE];
	return KERN_SUCCESS;
}

/*
 * Exported functions.
 */
 
/*
 * Add specified server ("kl_util -a"). If server already exists, this
 * is a nop.
 *
 * path_name is the location of the relocatable.
 * server_name is kern_loader's notion of the server's identity.
 */
int 
kl_com_add(char *path_name, char *server_name) {
	int rtn;
	kern_return_t r;
	
	if(rtn = kl_init())
		return(rtn);
	switch(kl_com_get_state(server_name)) {
	    case KSS_LOADED:
	    case KSS_ALLOCATED:
	    	return(0);		/* this function is a nop */
	   default:
	   	break;
	}
	r = kern_loader_add_server(kl_port, kernel_task, path_name);
	if((r != KERN_SUCCESS) && 
	   (r != KERN_LOADER_SERVER_EXISTS) &&
	   /* bogus - covers kern_loader bug #7190 */
	   (r != KERN_LOADER_SERVER_WONT_LOAD)) {
		kl_com_error("kern_loader_add_server", r);
		rtn = 1;
	}
	return(rtn);
}

/*
 * Delete specified server.
 */
int 
kl_com_delete(char *server_name) {
	int rtn;
	kern_return_t r;

	if(rtn = kl_init())
		return(rtn);
	r = kern_loader_delete_server(kl_port, kernel_task, server_name);
	if(r) {
		kl_com_error("kern_loader_delete_server", r);
		rtn = 1;
	}
	return(rtn);
}

/*
 * Load the specified server into kernel memory if it isn't already loaded.
 */
int 
kl_com_load(char *server_name) {
	int rtn;
	kern_return_t r;

	if(rtn = kl_init())
		return(rtn);
	switch(kl_com_get_state(server_name)) {
	    case KSS_LOADED:
	    	return(0);		/* this function is a nop */
	   default:
	   	break;
	}
	r = kern_loader_load_server(kl_port, server_name);
	if(r) {
		kl_com_error("kern_loader_load_server", r);
		rtn = 1;
	}
	return(rtn);
}

/*
 * Unload specified server from kernel memory. The server is not deleted
 * from kern_loader's list of servers.
 */
int 
kl_com_unload(char *server_name) {
	int rtn = 0;
	kern_return_t r;

	if(rtn = kl_init())
		return(rtn);
	r = kern_loader_unload_server(kl_port, kernel_task, server_name);
	if(r) {
		kl_com_error("kern_loader_unload_server", r);
		rtn = 1;
	}
	return(rtn);
}

klc_server_state 
kl_com_get_state(char *server_name)
{
	server_state_t state;
	/*
	 * remainder are unused...
	 */
	vm_address_t load_address;
	vm_size_t load_size;
	server_reloc_t reloc, loadable;
	port_name_t *ports;
	port_name_string_array_t port_names;
	u_int cnt;
	boolean_t *advertised;
	kern_return_t rtn;
	
	if(rtn = kl_init())
		return(rtn);
	rtn = kern_loader_server_info(kl_port, PORT_NULL, server_name,
		&state, &load_address, &load_size, reloc, loadable,
		&ports, &cnt, &port_names, &cnt, &advertised, &cnt);
	if(rtn != KERN_SUCCESS) {
		return(KSS_UNKNOWN);
	}
	switch(state) {
	    case Allocating:
	    case Allocated:
	    	return(KSS_ALLOCATED);
	    case Loading:
	    case Loaded:
	    	return(KSS_LOADED);
	    case Zombie:
	    case Unloading:
	    case Deallocated:
	    default:
	    	return(KSS_UNKNOWN);
	}

}

void 
kl_com_error(char *err_str, kern_return_t rtn)
{
	IOLog("%s: error %d\n", err_str, rtn);
}

void 
kl_com_wait()
{
	kern_loader_ping(kl_port, reply_port, 0);
	
	
	/*
	 * ping() is called when this is done...
	 */
	[ping_lock lockWhen:TRUE];
	[ping_lock unlockWith:FALSE];
}

