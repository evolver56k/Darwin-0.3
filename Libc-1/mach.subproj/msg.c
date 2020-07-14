/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log: msg.c,v $
 * Revision 1.1.1.1  1999/04/14 23:19:15  wsanchez
 * Import of Libc-78-8
 *
 * Revision 1.1.1.1.38.3  1999/03/16 15:47:15  wsanchez
 * Substitute License
 *
 * Revision 1.3  89/05/05  18:46:21  mrt
 * 	Cleanup for Mach 2.5
 * 
 * 10-Aug-90  Morris Meyer (mmeyer) at NeXT, Inc.
 *	Fixed timeout and size parameters for msg_send(), msg_receive()
 *	and msg_rpc().
 *
 *  1-Feb-89  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Fixed msg_{receive,send} if-if-else bug.
 *
 * 21-Oct-88  Richard Draves (rpd) at Carnegie-Mellon University
 *	Added msg_send wrapper, which handles SEND_INTERRUPT.
 *	Fixed bug in msg_rpc wrapper; it gave the wrong size to	msg_receive_.
 *	Converted to first try the new *_trap calls and fall back on
 *	the (renamed) *_old calls if they don't work.
 *
 * 19-May-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed the test for interupts in msg_rpc_.
 *	(Copied from mwyoung's version.)
 */

#if NeXT
#include <mach/features.h>
#else
#define KERNEL_FEATURES 1
#endif

#include <mach/kern_return.h>
#include <mach/message.h>

extern msg_return_t msg_send_trap();
extern msg_return_t msg_receive_trap();
extern msg_return_t msg_rpc_trap();

msg_return_t	msg_send(header, option, timeout)
	msg_header_t	*header;
	msg_option_t	option;
	msg_timeout_t	timeout;
{
	register
	msg_return_t	result;


	result = msg_send_trap(header, option, header->msg_size, timeout);
	if (result == SEND_SUCCESS)
		return result;

	if ((result == SEND_INTERRUPTED) &&
		 !(option & SEND_INTERRUPT))
		do
			result = msg_send_trap(header, option,
					       header->msg_size, timeout);
		while (result == SEND_INTERRUPTED);

	return result;
}

msg_return_t	msg_receive(header, option, timeout)
	msg_header_t	*header;
	msg_option_t	option;
	msg_timeout_t	timeout;
{
	register
	msg_return_t	result;

	result = msg_receive_trap(header, option, header->msg_size,
				  header->msg_local_port, timeout);
	if (result == RCV_SUCCESS)
		return result;

	if ((result == RCV_INTERRUPTED) &&
		 !(option & RCV_INTERRUPT))
		do
			result = msg_receive_trap(header, option,
						  header->msg_size,
						  header->msg_local_port,
						  timeout);
		while (result == RCV_INTERRUPTED);

	return result;
}

msg_return_t	msg_rpc(header, option, rcv_size, send_timeout, rcv_timeout)
	msg_header_t	*header;
	msg_option_t	option;
	msg_size_t	rcv_size;
	msg_timeout_t	send_timeout;
	msg_timeout_t	rcv_timeout;
{
	register
	msg_return_t	result;
	
	result = msg_rpc_trap(header, option, header->msg_size,
			      rcv_size, send_timeout, rcv_timeout);
	if (result == RPC_SUCCESS)
		return result;

	if ((result == SEND_INTERRUPTED) &&
		   !(option & SEND_INTERRUPT)) {
		do
			result = msg_rpc_trap(header, option,
					      header->msg_size, rcv_size,
					      send_timeout, rcv_timeout);
		while (result == SEND_INTERRUPT);
	}

	if ((result == RCV_INTERRUPTED) &&
	    !(option & RCV_INTERRUPT))
		do
			result = msg_receive_trap(header, option, rcv_size,
						  header->msg_local_port,
						  rcv_timeout);
		while (result == RCV_INTERRUPTED);

	return result;
}
