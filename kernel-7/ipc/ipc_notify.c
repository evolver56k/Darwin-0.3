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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	ipc/ipc_notify.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Notification-sending functions.
 */

#import <mach/features.h>

#include <mach/port.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <kern/assert.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_port.h>

#include <ipc/ipc_machdep.h>

/*
 * Forward declarations
 */
void ipc_notify_init_port_deleted(
	mach_port_deleted_notification_t	*n);

void ipc_notify_init_port_destroyed(
	mach_port_destroyed_notification_t	*n);

void ipc_notify_init_no_senders(
	mach_no_senders_notification_t		*n);

void ipc_notify_init_send_once(
	mach_send_once_notification_t		*n);

void ipc_notify_init_dead_name(
	mach_dead_name_notification_t		*n);

mach_port_deleted_notification_t	ipc_notify_port_deleted_template;
mach_port_destroyed_notification_t	ipc_notify_port_destroyed_template;
mach_no_senders_notification_t		ipc_notify_no_senders_template;
mach_send_once_notification_t		ipc_notify_send_once_template;
mach_dead_name_notification_t		ipc_notify_dead_name_template;

#if	MACH_IPC_COMPAT
notification_t			ipc_notify_port_deleted_compat_template;
notification_t			ipc_notify_msg_accepted_compat_template;
notification_t			ipc_notify_port_destroyed_compat_template;

/*
 *	When notification messages are received via the old
 *	msg_receive trap, the msg_type field should contain
 *	MSG_TYPE_EMERGENCY.  We arrange for this by putting
 *	MSG_TYPE_EMERGENCY into msgh_reserved, which
 *	ipc_kmsg_copyout_compat copies to msg_type.
 */

#define NOTIFY_MSGH_RESERVED	MSG_TYPE_EMERGENCY
#endif	/* MACH_IPC_COMPAT */

/*
 *	Routine:	ipc_notify_init_port_deleted
 *	Purpose:
 *		Initialize a template for port-deleted notifications.
 */

void
ipc_notify_init_port_deleted(
	mach_port_deleted_notification_t	*n)
{
	mach_msg_header_t *m = &n->not_header;

	m->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0);
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_PORT_DELETED;
	m->msgh_size = ((int)sizeof *n) - sizeof(mach_msg_format_0_trailer_t);

	n->NDR = NDR_record;
	n->not_port = MACH_PORT_NULL;
}

#if	MACH_IPC_COMPAT
void
ipc_notify_init_port_deleted_compat(
	notification_t		*n)
{
	mach_msg_header_t *m = (mach_msg_header_t *)&n->notify_header;
	mach_msg_type_t *t = (mach_msg_type_t *)&n->notify_type;

	m->msgh_bits = MACH_MSGH_BITS_OLD_FORMAT |
		MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0);
	m->msgh_size = sizeof *n;
	m->msgh_reserved = NOTIFY_MSGH_RESERVED;
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_PORT_DELETED;

	t->msgt_name = MACH_MSG_TYPE_PORT_NAME;
	t->msgt_size = PORT_T_SIZE_IN_BITS;
	t->msgt_number = 1;
	t->msgt_inline = TRUE;
	t->msgt_longform = FALSE;
	t->msgt_deallocate = FALSE;
	t->msgt_unused = 0;

	n->notify_port = PORT_NULL;
}
#endif	/* MACH_IPC_COMPAT */

/*
 *	Routine:	ipc_notify_init_msg_accepted
 *	Purpose:
 *		Initialize a template for msg-accepted notifications.
 */

#if	MACH_IPC_COMPAT
void
ipc_notify_init_msg_accepted_compat(
	notification_t		*n)
{
	mach_msg_header_t *m = (mach_msg_header_t *)&n->notify_header;
	mach_msg_type_t *t = (mach_msg_type_t *)&n->notify_type;

	m->msgh_bits = MACH_MSGH_BITS_OLD_FORMAT |
		MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0);
	m->msgh_size = sizeof *n;
	m->msgh_reserved = NOTIFY_MSGH_RESERVED;
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_MSG_ACCEPTED;

	t->msgt_name = MACH_MSG_TYPE_PORT_NAME;
	t->msgt_size = PORT_T_SIZE_IN_BITS;
	t->msgt_number = 1;
	t->msgt_inline = TRUE;
	t->msgt_longform = FALSE;
	t->msgt_deallocate = FALSE;
	t->msgt_unused = 0;

	n->notify_port = PORT_NULL;
}
#endif	/* MACH_IPC_COMPAT */

/*
 *	Routine:	ipc_notify_init_port_destroyed
 *	Purpose:
 *		Initialize a template for port-destroyed notifications.
 */

void
ipc_notify_init_port_destroyed(
	mach_port_destroyed_notification_t	*n)
{
	mach_msg_header_t *m = &n->not_header;

	m->msgh_bits = MACH_MSGH_BITS_COMPLEX |
		MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0);
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_PORT_DESTROYED;
	m->msgh_size = ((int)sizeof *n) - sizeof(mach_msg_format_0_trailer_t);

	n->not_body.msgh_descriptor_count = 1;
	n->not_port.disposition = MACH_MSG_TYPE_PORT_RECEIVE;
	n->not_port.name = MACH_PORT_NULL;
	n->not_port.type = MACH_MSG_PORT_DESCRIPTOR;
}

#if	MACH_IPC_COMPAT
void
ipc_notify_init_port_destroyed_compat(
	notification_t		 *n)
{
	mach_msg_header_t *m = (mach_msg_header_t *)&n->notify_header;
	mach_msg_type_t *t = (mach_msg_type_t *)&n->notify_type;

	m->msgh_bits = MACH_MSGH_BITS_OLD_FORMAT | MACH_MSGH_BITS_COMPLEX |
		MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0);
	m->msgh_size = sizeof *n;
	m->msgh_reserved = NOTIFY_MSGH_RESERVED;
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_PORT_DESTROYED;

	t->msgt_name = MACH_MSG_TYPE_PORT_RECEIVE;
	t->msgt_size = PORT_T_SIZE_IN_BITS;
	t->msgt_number = 1;
	t->msgt_inline = TRUE;
	t->msgt_longform = FALSE;
	t->msgt_deallocate = FALSE;
	t->msgt_unused = 0;

	n->notify_port = PORT_NULL;
}
#endif	/* MACH_IPC_COMPAT */

/*
 *	Routine:	ipc_notify_init_no_senders
 *	Purpose:
 *		Initialize a template for no-senders notifications.
 */

void
ipc_notify_init_no_senders(
	mach_no_senders_notification_t	*n)
{
	mach_msg_header_t *m = &n->not_header;

	m->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0);
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_NO_SENDERS;
	m->msgh_size = ((int)sizeof *n) - sizeof(mach_msg_format_0_trailer_t);

	n->NDR = NDR_record;
	n->not_count = 0;
}

/*
 *	Routine:	ipc_notify_init_send_once
 *	Purpose:
 *		Initialize a template for send-once notifications.
 */

void
ipc_notify_init_send_once(
	mach_send_once_notification_t	*n)
{
	mach_msg_header_t *m = &n->not_header;

	m->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0);
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_SEND_ONCE;
	m->msgh_size = ((int)sizeof *n) - sizeof(mach_msg_format_0_trailer_t);
}

/*
 *	Routine:	ipc_notify_init_dead_name
 *	Purpose:
 *		Initialize a template for dead-name notifications.
 */

void
ipc_notify_init_dead_name(
	mach_dead_name_notification_t	*n)
{
	mach_msg_header_t *m = &n->not_header;

	m->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0);
	m->msgh_local_port = MACH_PORT_NULL;
	m->msgh_remote_port = MACH_PORT_NULL;
	m->msgh_id = MACH_NOTIFY_DEAD_NAME;
	m->msgh_size = ((int)sizeof *n) - sizeof(mach_msg_format_0_trailer_t);

	n->NDR = NDR_record;
	n->not_port = MACH_PORT_NULL;
}

/*
 *	Routine:	ipc_notify_init
 *	Purpose:
 *		Initialize the notification subsystem.
 */

void
ipc_notify_init(void)
{
	ipc_notify_init_port_deleted(&ipc_notify_port_deleted_template);
	ipc_notify_init_port_destroyed(&ipc_notify_port_destroyed_template);
	ipc_notify_init_no_senders(&ipc_notify_no_senders_template);
	ipc_notify_init_send_once(&ipc_notify_send_once_template);
	ipc_notify_init_dead_name(&ipc_notify_dead_name_template);
#if	MACH_IPC_COMPAT
	ipc_notify_init_msg_accepted_compat(
				&ipc_notify_msg_accepted_compat_template);
	ipc_notify_init_port_deleted_compat(
				&ipc_notify_port_deleted_compat_template);
	ipc_notify_init_port_destroyed_compat(
				&ipc_notify_port_destroyed_compat_template);
#endif	/* MACH_IPC_COMPAT */
}

/*
 *	Routine:	ipc_notify_port_deleted
 *	Purpose:
 *		Send a port-deleted notification.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/soright for port.
 */

void
ipc_notify_port_deleted(
	ipc_port_t	port,
	mach_port_t	name)
{
	ipc_kmsg_t kmsg;
	mach_port_deleted_notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped port-deleted (0x%08x, 0x%x)\n", port, name);
		ipc_port_release_sonce(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (mach_port_deleted_notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_port_deleted_template;

	n->not_header.msgh_remote_port = (mach_port_t) port;
	n->not_port = name;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_port_destroyed
 *	Purpose:
 *		Send a port-destroyed notification.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/soright for port.
 *		Consumes a ref for right, which should be a receive right
 *		prepped for placement into a message.  (In-transit,
 *		or in-limbo if a circularity was detected.)
 */

void
ipc_notify_port_destroyed(
	ipc_port_t	port,
	ipc_port_t	right)
{
	ipc_kmsg_t kmsg;
	mach_port_destroyed_notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped port-destroyed (0x%08x, 0x%08x)\n",
		       port, right);
		ipc_port_release_sonce(port);
		ipc_port_release_receive(right);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (mach_port_destroyed_notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_port_destroyed_template;

	n->not_header.msgh_remote_port = (mach_port_t) port;
	n->not_port.name = (mach_port_t)right;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_no_senders
 *	Purpose:
 *		Send a no-senders notification.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/soright for port.
 */

void
ipc_notify_no_senders(
	ipc_port_t		port,
	mach_port_mscount_t	mscount)
{
	ipc_kmsg_t kmsg;
	mach_no_senders_notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped no-senders (0x%08x, %u)\n", port, mscount);
		ipc_port_release_sonce(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (mach_no_senders_notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_no_senders_template;

	n->not_header.msgh_remote_port = (mach_port_t) port;
	n->not_count = mscount;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_send_once
 *	Purpose:
 *		Send a send-once notification.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/soright for port.
 */

void
ipc_notify_send_once(
	ipc_port_t	port)
{
	ipc_kmsg_t kmsg;
	mach_send_once_notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped send-once (0x%08x)\n", port);
		ipc_port_release_sonce(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (mach_send_once_notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_send_once_template;

        n->not_header.msgh_remote_port = (mach_port_t) port;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_dead_name
 *	Purpose:
 *		Send a dead-name notification.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/soright for port.
 */

void
ipc_notify_dead_name(
	ipc_port_t	port,
	mach_port_t	name)
{
	ipc_kmsg_t kmsg;
	mach_dead_name_notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped dead-name (0x%08x, 0x%x)\n", port, name);
		ipc_port_release_sonce(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (mach_dead_name_notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_dead_name_template;

	n->not_header.msgh_remote_port = (mach_port_t) port;
	n->not_port = name;

	ipc_mqueue_send_always(kmsg);
}

#if	MACH_IPC_COMPAT

/*
 *	Routine:	ipc_notify_port_deleted_compat
 *	Purpose:
 *		Send a port-deleted notification.
 *		Sends it to a send right instead of a send-once right.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/sright for port.
 */

void
ipc_notify_port_deleted_compat(
	ipc_port_t	port,
	mach_port_t	name)
{
	ipc_kmsg_t kmsg;
	notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped port-deleted-compat (0x%08x, 0x%x)\n",
		       port, name);
		ipc_port_release_send(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_port_deleted_compat_template;

	kmsg->ikm_header.msgh_remote_port = (mach_port_t) port;

	n->notify_port = (port_t)name;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_msg_accepted_compat
 *	Purpose:
 *		Send a msg-accepted notification.
 *		Sends it to a send right instead of a send-once right.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/sright for port.
 */

void
ipc_notify_msg_accepted_compat(
	ipc_port_t	port,
	mach_port_t	name)
{
	ipc_kmsg_t kmsg;
	notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped msg-accepted-compat (0x%08x, 0x%x)\n",
		       port, name);
		ipc_port_release_send(port);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_msg_accepted_compat_template;

	kmsg->ikm_header.msgh_remote_port = (mach_port_t) port;

	n->notify_port = (port_t)name;

	ipc_mqueue_send_always(kmsg);
}

/*
 *	Routine:	ipc_notify_port_destroyed_compat
 *	Purpose:
 *		Send a port-destroyed notification.
 *		Sends it to a send right instead of a send-once right.
 *	Conditions:
 *		Nothing locked.
 *		Consumes a ref/sright for port.
 *		Consumes a ref for right, which should be a receive right
 *		prepped for placement into a message.  (In-transit,
 *		or in-limbo if a circularity was detected.)
 */

void
ipc_notify_port_destroyed_compat(
	ipc_port_t	port,
	ipc_port_t	right)
{
	ipc_kmsg_t kmsg;
	notification_t *n;

	kmsg = ikm_alloc(sizeof *n);
	if (kmsg == IKM_NULL) {
		printf("dropped port-destroyed-compat (0x%08x, 0x%08x)\n",
		       port, right);
		ipc_port_release_send(port);
		ipc_port_release_receive(right);
		return;
	}

	ikm_init(kmsg, sizeof *n);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	n = (notification_t *) &kmsg->ikm_header;
	*n = ipc_notify_port_destroyed_compat_template;

	kmsg->ikm_header.msgh_remote_port = (mach_port_t) port;

	n->notify_port = (port_t) right;

	ipc_mqueue_send_always(kmsg);
}

#endif	/* MACH_IPC_COMPAT */
