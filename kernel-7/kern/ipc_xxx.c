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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * Random stuff needed for new IPC under NEXTSTEP.
 *
 * HISTORY
 *
 * 5 May 1994 ? at NeXT
 *	Created.
 */
 
#import <mach/features.h>

#import <mach/mach_types.h>
#import <mach/notify.h>

static inline
port_name_t
do_object_copyout(
    task_t		task,
    ipc_port_t		port
)
{
	port_name_t	name;

	if (port != IP_NULL) {
		port = ipc_port_copy_send(port);
		if (port != IP_DEAD)
			ipc_object_copyout(
				task->itk_space, port,
					MACH_MSG_TYPE_PORT_SEND, TRUE, &name);
		else
			name = PORT_NULL;
	}
	else
		name = PORT_NULL;

	return name;
}

/*
 * Why are the semantics of hostXXXself() and
 * device_master_self() so inconsistent?
 */

port_name_t host_priv_self()
{
	register task_t self = current_thread()->task;
	register ipc_port_t port;
	port_name_t name;

	if (!is_suser()) {
		name = PORT_NULL;
	}
	else {
		port = realhost.host_priv_self;
		name = do_object_copyout(self, port);
	}
	return name;
}

#if	DRIVERKIT

port_name_t device_master_self()
{
	register task_t self = current_thread()->task;
	register ipc_port_t port;
	port_name_t name;

	if (!is_suser()) {
		port = realhost.host_self;
	}
	else {
		port = realhost.host_priv_self;
	}
	name = do_object_copyout(self, port);
	return name;
}

#endif	DRIVERKIT

/*
 * This should be implemented using
 * the name service (or object service).
 */

ipc_port_t lookupd_port, lookupd_port_priv;

static
ipc_port_t
_lookupd_port_choose(void)
{
    ipc_port_t port;

    if (lookupd_port_priv != IP_NULL && is_suser1())
	port = lookupd_port_priv;
    else
	port = lookupd_port;

    return port;
}

port_name_t
_lookupd_port(name)
	port_name_t name;
{
	register task_t self = current_thread()->task;
	ipc_port_t port;

	if (name != PORT_NULL) {
		if (!is_suser())	
			name = PORT_NULL;
		else {
			if (ipc_object_copyin(self->itk_space,
						name, MACH_MSG_TYPE_MAKE_SEND,
						&port) == KERN_SUCCESS) {
				if (lookupd_port != IP_NULL)
					ipc_port_release_send(lookupd_port);
				lookupd_port = port;
			}
			else
				name = PORT_NULL;
		}
	}	
	else {
		port = _lookupd_port_choose();
		name = do_object_copyout(self, port);
	}
	return name;
}

port_name_t
_lookupd_port1(
	port_name_t	name)
{
	register task_t self = current_thread()->task;
	ipc_port_t port;

	if (!is_suser())
		return (PORT_NULL);

	if (name != PORT_NULL) {
		if (ipc_object_copyin(self->itk_space,
				      name, MACH_MSG_TYPE_MAKE_SEND,
				      		&port) == KERN_SUCCESS) {
			if (lookupd_port_priv != IP_NULL)
				ipc_port_release_send(lookupd_port_priv);
			lookupd_port_priv = port;
		}
		else
			name = PORT_NULL;
	}	
	else {
		port = lookupd_port;
		name = do_object_copyout(self, port);
	}
	return name;
}

/*
 * This should definitely be handled
 * by the object service.
 */

ipc_port_t ev_port_list[2] = { IP_NULL, IP_NULL };

port_name_t
_event_port_by_tag( num )
	int num;
{
	register task_t mytask = current_thread()->task;
	register ipc_port_t port;
	port_name_t name;

	if (num == 0 && !is_suser()) /* Security check on privileged port */
		return PORT_NULL;
	if ( num < 0 || num > (sizeof ev_port_list/sizeof ev_port_list[0]) )
		return PORT_NULL;

	port = ev_port_list[num];

	name = do_object_copyout(mytask, port);
	return name;
}

boolean_t
object_copyin(
	task_t			task,
	port_name_t		name,
	mach_msg_type_name_t	rights,
	boolean_t		dealloc,
	ipc_port_t		*port
)
{
	if (ipc_object_copyin_compat(
				    task->itk_space,
				    name, rights, dealloc,
				    port) != KERN_SUCCESS)
		return FALSE;
	else
		return TRUE;		
}

void
object_copyout(
	task_t			task,
	ipc_port_t		port,
	mach_msg_type_name_t	rights,
	port_name_t		*namep
)
{
	if (port == IP_NULL)
		panic("object_copyout");

	if (rights == MSG_TYPE_PORT_ALL)
		rights = MACH_MSG_TYPE_PORT_RECEIVE;
	else
		rights = MACH_MSG_TYPE_PORT_SEND;

	if (port == IP_NULL ||
		ipc_object_copyout_compat(
					task->itk_space,
					port, rights,
					namep) != KERN_SUCCESS)
		*namep = PORT_NULL;
}

void
port_reference(
	ipc_port_t		port
)
{
	if (port != IP_NULL)
		(void) ipc_port_copy_send(port);	/* XXX */
}

void
port_release(
    	ipc_port_t		port
)
{
	if (port != IP_NULL)
		ipc_port_release_send(port);		/* XXX */
}

void
send_notification(task, msg_id, name)
	register task_t task;		/* Who we're notifying */
	int msg_id;			/* What event occurred */
	port_name_t name;		/* What port is involved */
{
	ipc_port_t	tnotify;
	
	if (msg_id != NOTIFY_MSG_ACCEPTED)
		return;
	
	if (task_get_special_port(task,
				TASK_NOTIFY_PORT, &tnotify) != KERN_SUCCESS)
		return;
		
	ipc_notify_msg_accepted_compat(tnotify, name);
}
