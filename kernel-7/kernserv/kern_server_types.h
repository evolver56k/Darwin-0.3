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
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 8-Jun-92  Matthew Self (mself) at NeXT
 *	Added mach_header field to kern_server_var_t for Objective-C.
 *
 * 12-Jul-90  Gregg Kellogg (gk) at NeXT
 *	Changed msgs to msg, as only a single message is retained.
 *	Added KS_VERSION(=2), which is updated whenever a protocol change
 *	is made.
 *
 * 51-May-90  Gregg Kellogg (gk) at NeXT
 *	Added pad filed at end of kern_server struct so that already compiled
 *	servers won't be impacted when the kern_server grows.
 *
 * 04-Apr-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#ifndef	_KERN_SERVER_
#define	_KERN_SERVER_
#if	_KERNEL
#import <mach/mach_user_internal.h>
#endif	/* _KERNEL */
#import <mach/mach_interface.h>
#import <mach/message.h>
#import <sys/types.h>
#import <kern/queue.h>
#import <kernserv/kern_server_reply_types.h>

/*
 * Messages to be sent to the Mach bootstrap port.  The bootstrap port
 * is inherited from the parent task and is used for finding information
 * about the external and internal environment.
 *
 * Generic things that need to be done to get a kernel server started:
 *	Wire down memory regions
 *	Associate ports with procedures (to handle messages)
 */

/*
 * This must be small enough to fit (along with a message header) in 8K.
 */
#define	KERN_SERVER_LOG_SIZE	500

#define KERN_SERVER_ERROR	100		// bad message
#define KERN_SERVER_NOTLOGGING	101		// server not logging data
#define KERN_SERVER_UNCREC_PORT	102		// port not recognized
#define KERN_SERVER_BAD_VERSION	103		// server version not supported

#define KERN_SERVER_NPORTPROC	50

typedef boolean_t (*port_map_proc_t) (
	msg_header_t *in_msg,
	void *arg);

typedef boolean_t (*port_map_serv_t) (
	msg_header_t *in_msg,
	msg_header_t *out_msg);

typedef boolean_t (*port_death_proc_t)(port_name_t port);
typedef boolean_t (*port_notify_proc_t)(port_name_t port, u_int notify_type);

typedef int (*call_proc_t)(int arg);

typedef struct port_proc_map {
	port_name_t	port;		// port -> proc mapping
	port_map_proc_t	proc;		// proc to call
	void *		uarg;
	enum { PP_handler, PP_server }
			type;		// proc interface type
} port_proc_map_t;

typedef struct {
	port_t		reply_port;
	port_name_t	req_port;
	queue_chain_t	link;
} ks_notify_t;

#if	_KERNEL
/*
 * Supported compatibility level.
 */
#define	KS_COMPAT	2
#define	KS_VERSION	2

/*
 * Server instance structure
 */
typedef struct kern_server_var {
	simple_lock_data_t	slock;		// access synchronization
	port_name_t		local_port;	// local_port of cur message
	port_name_t		task_port;	// my task port
	thread_t		server_thread;	// server thread.
	port_name_t		bootstrap_port;	// my bootstrap port
	port_name_t		boot_listener_port; // port we listen on
	port_name_t		log_port;	// port to send log info to
	port_name_t		notify_port;	// notification port.
	port_set_name_t		port_set;	// port set we receive on
	log_t			log;		// log structure
	queue_head_t		msg_callout_q;	// queue of messages to send
	queue_head_t		msg_callout_fq;	// free queue
	msg_header_t		*msg;		// msg we receive into
	int			msg_size;	// size of msg.
	struct msg_send_entry {
		void		(*func)();
		void *		arg;
		queue_chain_t	link;
	} msg_send_array[20];
	port_proc_map_t		port_proc[KERN_SERVER_NPORTPROC];
	port_name_t		last_unrec_port;// last bad kern_serv_disp()
	port_name_t		last_rec_port;	// last good kern_serv_disp()
	int			last_rec_index;	// index of last good k_s_d()
	port_death_proc_t	pd_proc;	// port death procedure
	port_notify_proc_t	pn_proc;	// port notify procedure
	queue_head_t		notify_q;	// queue of notify/req pairs.
	int			version;	// version of server.
	port_t			kernel_port;	// kernel task port for server
	struct mach_header	*mach_header;	// mach_header of server module
} kern_server_var_t;

typedef kern_server_var_t *kern_server_t;

/*
 * Structure access functions.
 */
port_t kern_serv_local_port(kern_server_t *ksp);
port_t kern_serv_bootstrap_port(kern_server_t *ksp);
port_t kern_serv_notify_port(kern_server_t *ksp);
port_set_name_t kern_serv_port_set(kern_server_t *ksp);

/*
 * Function prototypes.
 */
void kern_server_main(void);
void kern_serv_port_gone(kern_server_t *ksp, port_name_t port);

/*
 * Log functions.
 */
void kern_serv_log (			// log a message
	kern_server_t	*ksp,		// kern_server instance vars
	int		log_level,	// level to log at
	char		*msg,		// what to log (followed by args)
	int		arg1,
	int		arg2,
	int		arg3,
	int		arg4,
	int		arg5);

/*
 * Alias for msg_send to ensure that messages are sent at ipl0 from the proper
 * task.
 */
kern_return_t kern_serv_callout (
	kern_server_t	*ksp,
	void		(*proc)(void *arg),
	void		*arg);

/*
 * Kernel version of kern_serv_notify, doesn't contact kern_loader, uses
 * internal port_request_notification facility.
 */
kern_return_t kern_serv_notify (
	kern_server_t	*ksp,
	port_t		reply_port,
	port_t		req_port);

/*
 * Kernel reference for kern_serv interface routines.
 */
kern_return_t kern_serv_instance_loc (
	void		*arg,		// (kern_server_t *ksp)
	vm_address_t	instance_loc);
kern_return_t kern_serv_boot_port (	// how to talk to loader
	void		*arg,		// (kern_server_t *ksp)
	port_t		boot_port);
kern_return_t kern_serv_version (
	void		*arg,		// (kern_server_t *ksp)
	int		version);	// server version
kern_return_t kern_serv_load_objc (
	void		*arg,		// (kern_server_t *ksp)
	vm_address_t	header);	// header addr
kern_return_t kern_serv_wire_range (	// wire the specified range or memory
	void		*arg,		// (kern_server_t *ksp)
	vm_address_t	addr,
	vm_size_t	size);
kern_return_t kern_serv_unwire_range (	// unwire the specified range or memory
	void		*arg,		// (kern_server_t *ksp)
	vm_address_t	addr,
	vm_size_t	size);
kern_return_t kern_serv_port_proc (	// map a message on port to proc/arg
	void		*arg,		// (kern_server_t *ksp)
	port_all_t	port,		// port to map (all rights passed)
	port_map_proc_t	proc,		// proc to call
	int		uarg);		// replace local_port with uarg
kern_return_t kern_serv_port_death_proc ( // specify port death handler
	void			*arg,	// (kern_server_t *ksp)
	port_death_proc_t	proc);	// proc to call
kern_return_t kern_serv_call_proc (	// call procedure with argument
	void		*arg,		// (kern_server_t *ksp)
	call_proc_t	proc,		// proc to call
	int		uarg);		// arg to supply
kern_return_t kern_serv_shutdown (
	void		*arg);		// (kern_server_t *ksp)
kern_return_t kern_serv_log_level (
	void		*arg,		// (kern_server_t *ksp)
	int		log_level);
kern_return_t kern_serv_get_log (
	void		*arg,		// (kern_server_t *ksp)
	port_t		reply_port);	// port to send log information to
kern_return_t kern_serv_port_serv (	// map a message on port to proc/arg
	void		*arg,		// (kern_server_t *ksp)
	port_all_t	port,		// port to map (all rights passed)
	port_map_proc_t	proc,		// proc to call
	int		uarg);		// replace local_port with uarg
port_t kern_serv_kernel_task_port(void);	// kernel port (for vm ops).

#endif	/* _KERNEL */
#endif	/* _KERN_SERVER_ */
