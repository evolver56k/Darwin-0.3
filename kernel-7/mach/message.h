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
 * Copyright (c) 1992-1987 Carnegie Mellon University
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
 *	File:	mach/message.h
 *
 *	Mach IPC message and primitive function definitions.
 */

#ifndef	_MACH_MESSAGE_H_
#define _MACH_MESSAGE_H_

#import <mach/kern_return.h>
#import <mach/port.h>


/*
 *  The timeout mechanism uses mach_msg_timeout_t values,
 *  passed by value.  The timeout units are milliseconds.
 *  It is controlled with the MACH_SEND_TIMEOUT
 *  and MACH_RCV_TIMEOUT options.
 */

typedef natural_t mach_msg_timeout_t;

/*
 *  The value to be used when there is no timeout.
 *  (No MACH_SEND_TIMEOUT/MACH_RCV_TIMEOUT option.)
 */

#define MACH_MSG_TIMEOUT_NONE		((mach_msg_timeout_t) 0)

/*
 *  The kernel uses MACH_MSGH_BITS_COMPLEX as a hint.  It it isn't on, it
 *  assumes the body of the message doesn't contain port rights or OOL
 *  data.  The field is set in received messages.  A user task must
 *  use caution in interpreting the body of a message if the bit isn't
 *  on, because the mach_msg_type's in the body might "lie" about the
 *  contents.  If the bit isn't on, but the mach_msg_types
 *  in the body specify rights or OOL data, the behaviour is undefined.
 *  (Ie, an error may or may not be produced.)
 *
 *  The value of MACH_MSGH_BITS_REMOTE determines the interpretation
 *  of the msgh_remote_port field.  It is handled like a msgt_name.
 *
 *  The value of MACH_MSGH_BITS_LOCAL determines the interpretation
 *  of the msgh_local_port field.  It is handled like a msgt_name.
 *
 *  MACH_MSGH_BITS() combines two MACH_MSG_TYPE_* values, for the remote
 *  and local fields, into a single value suitable for msgh_bits.
 *
 *  MACH_MSGH_BITS_COMPLEX_PORTS, MACH_MSGH_BITS_COMPLEX_DATA, and
 *  MACH_MSGH_BITS_CIRCULAR should be zero; they are used internally.
 *
 *  The unused bits should be zero.
 */

#define MACH_MSGH_BITS_ZERO		0x00000000
#define MACH_MSGH_BITS_REMOTE_MASK	0x000000ff
#define MACH_MSGH_BITS_LOCAL_MASK	0x0000ff00
#define MACH_MSGH_BITS_COMPLEX		0x80000000U
#define	MACH_MSGH_BITS_CIRCULAR		0x40000000	/* internal use only */
#define	MACH_MSGH_BITS_COMPLEX_PORTS	0x20000000	/* internal use only */
#define	MACH_MSGH_BITS_COMPLEX_DATA	0x10000000	/* internal use only */
#define	MACH_MSGH_BITS_OLD_FORMAT	0x08000000	/* internal use only */
#define MACH_MSGH_BITS_NO_TRAILER	0x08000000	/* non standard */
#define	MACH_MSGH_BITS_UNUSED		0x07ff0000

#define	MACH_MSGH_BITS_PORTS_MASK				\
		(MACH_MSGH_BITS_REMOTE_MASK|MACH_MSGH_BITS_LOCAL_MASK)

#define MACH_MSGH_BITS(remote, local)				\
		((remote) | ((local) << 8))
#define	MACH_MSGH_BITS_REMOTE(bits)				\
		((bits) & MACH_MSGH_BITS_REMOTE_MASK)
#define	MACH_MSGH_BITS_LOCAL(bits)				\
		(((bits) & MACH_MSGH_BITS_LOCAL_MASK) >> 8)
#define	MACH_MSGH_BITS_PORTS(bits)				\
		((bits) & MACH_MSGH_BITS_PORTS_MASK)
#define	MACH_MSGH_BITS_OTHER(bits)				\
		((bits) &~ MACH_MSGH_BITS_PORTS_MASK)

/*
 *  Every message starts with a message header.
 *  Following the message header are zero or more pairs of
 *  type descriptors (mach_msg_type_t/mach_msg_type_long_t) and
 *  data values.  The size of the message must be specified in bytes,
 *  and includes the message header, type descriptors, inline
 *  data, and inline pointer for out-of-line data.
 *
 *  The msgh_remote_port field specifies the destination of the message.
 *  It must specify a valid send or send-once right for a port.
 *
 *  The msgh_local_port field specifies a "reply port".  Normally,
 *  This field carries a send-once right that the receiver will use
 *  to reply to the message.  It may carry the values MACH_PORT_NULL,
 *  MACH_PORT_DEAD, a send-once right, or a send right.
 *
 *  The msgh_seqno field carries a sequence number associated with the
 *  received-from port.  A port's sequence number is incremented every
 *  time a message is received from it.  In sent messages, the field's
 *  value is ignored.
 *
 *  The msgh_id field is uninterpreted by the message primitives.
 *  It normally carries information specifying the format
 *  or meaning of the message.
 */

typedef unsigned int mach_msg_bits_t;
typedef	unsigned int mach_msg_size_t;
typedef integer_t mach_msg_id_t;

#define MACH_MSG_SIZE_NULL (mach_msg_size_t *) 0

typedef	struct {
    mach_msg_bits_t	msgh_bits;
    mach_msg_size_t	msgh_size;
    mach_port_t		msgh_remote_port;
    mach_port_t		msgh_local_port;
    mach_port_seqno_t	msgh_seqno;
#define msgh_reserved	msgh_seqno
    mach_msg_id_t	msgh_id;
} mach_msg_header_t;

#define MACH_MSG_NULL (mach_msg_header_t *) 0

/*
 *  There is no fixed upper bound to the size of Mach messages.
 */

#define	MACH_MSG_SIZE_MAX	((mach_msg_size_t) ~0)

/*
 *  The msgt_number field specifies the number of data elements.
 *  The msgt_size field specifies the size of each data element, in bits.
 *  The msgt_name field specifies the type of each data element.
 *  If msgt_inline is TRUE, the data follows the type descriptor
 *  in the body of the message.  If msgt_inline is FALSE, then a pointer
 *  to the data should follow the type descriptor, and the data is
 *  sent out-of-line.  In this case, if msgt_deallocate is TRUE,
 *  then the out-of-line data is moved (instead of copied) into the message.
 *  If msgt_longform is TRUE, then the type descriptor is actually
 *  a mach_msg_type_long_t.
 *
 *  The actual amount of inline data following the descriptor must
 *  a multiple of the word size.  For out-of-line data, this is a
 *  pointer.  For inline data, the supplied data size (calculated
 *  from msgt_number/msgt_size) is rounded up.  This guarantees
 *  that type descriptors always fall on word boundaries.
 *
 *  For port rights, msgt_size must be 8*sizeof(mach_port_t).
 *  If the data is inline, msgt_deallocate should be FALSE.
 *  The msgt_unused bit should be zero.
 *  The msgt_name, msgt_size, msgt_number fields in
 *  a mach_msg_type_long_t should be zero.
 */

typedef unsigned int mach_msg_type_name_t;
typedef unsigned int mach_msg_type_size_t;
typedef natural_t  mach_msg_type_number_t;

typedef struct  {
    unsigned int	msgt_name : 8,
			msgt_size : 8,
			msgt_number : 12,
			msgt_inline : 1,
			msgt_longform : 1,
			msgt_deallocate : 1,
			msgt_unused : 1;
} mach_msg_type_t;

typedef	struct	{
    mach_msg_type_t	msgtl_header;
    unsigned short	msgtl_name;
    unsigned short	msgtl_size;
    natural_t		msgtl_number;
} mach_msg_type_long_t;


/*
 *	Known values for the msgt_name field.
 *
 *	The only types known to the Mach kernel are
 *	the port types, and those types used in the
 *	kernel RPC interface.
 */

#define MACH_MSG_TYPE_UNSTRUCTURED	0
#define MACH_MSG_TYPE_BIT		0
#define MACH_MSG_TYPE_BOOLEAN		0
#define MACH_MSG_TYPE_INTEGER_16	1
#define MACH_MSG_TYPE_INTEGER_32	2
#define MACH_MSG_TYPE_CHAR		8
#define MACH_MSG_TYPE_BYTE		9
#define MACH_MSG_TYPE_INTEGER_8		9
#define MACH_MSG_TYPE_REAL		10
#define MACH_MSG_TYPE_INTEGER_64	11
#define MACH_MSG_TYPE_STRING		12
#define MACH_MSG_TYPE_STRING_C		12

/*
 *  Values used when sending a port right.
 */

#define MACH_MSG_TYPE_MOVE_RECEIVE	16	/* Must hold receive rights */
#define MACH_MSG_TYPE_MOVE_SEND		17	/* Must hold send rights */
#define MACH_MSG_TYPE_MOVE_SEND_ONCE	18	/* Must hold sendonce rights */
#define MACH_MSG_TYPE_COPY_SEND		19	/* Must hold send rights */
#define MACH_MSG_TYPE_MAKE_SEND		20	/* Must hold receive rights */
#define MACH_MSG_TYPE_MAKE_SEND_ONCE	21	/* Must hold receive rights */

/*
 *  Structures which define the body of
 *  a message in the untyped IPC.
 */

typedef unsigned int mach_msg_copy_options_t;

#define MACH_MSG_PHYSICAL_COPY		0
#define MACH_MSG_VIRTUAL_COPY   	1
#define MACH_MSG_ALLOCATE		2
#define MACH_MSG_OVERWRITE		3
#define MACH_MSG_KALLOC_COPY_T		4	/* used internally */
#define MACH_MSG_PAGE_LIST_COPY_T	5	/* used internally */


typedef unsigned int mach_msg_descriptor_type_t;

#define MACH_MSG_PORT_DESCRIPTOR 	0
#define MACH_MSG_OOL_DESCRIPTOR  	1
#define MACH_MSG_OOL_PORTS_DESCRIPTOR 	2

typedef struct
{
  void*				pad1;
  mach_msg_size_t		pad2;
  unsigned int			pad3		:24;
  mach_msg_descriptor_type_t	type		:8;
} mach_msg_type_descriptor_t;

typedef struct
{
  mach_port_t			name;
  mach_msg_size_t		pad1;
  unsigned int			pad2		:16;
  mach_msg_type_name_t		disposition	:8;
  mach_msg_descriptor_type_t	type		:8;
} mach_msg_port_descriptor_t;

typedef struct
{
  void*				address;
  mach_msg_size_t       	size;
  boolean_t     		deallocate	:8;
  mach_msg_copy_options_t       copy		:8;
  unsigned int     		pad1		:8;
  mach_msg_descriptor_type_t    type		:8;
} mach_msg_ool_descriptor_t;

typedef struct
{
  void*				address;
  mach_msg_size_t		count;
  boolean_t     		deallocate	:8;
  mach_msg_copy_options_t       copy		:8;
  mach_msg_type_name_t		disposition	:8;
  mach_msg_descriptor_type_t	type		:8;
} mach_msg_ool_ports_descriptor_t;

typedef union
{
  mach_msg_type_descriptor_t		type;
  mach_msg_port_descriptor_t		port;
  mach_msg_ool_descriptor_t		out_of_line;
  mach_msg_ool_ports_descriptor_t	ool_ports;
} mach_msg_descriptor_t;

#define MACH_MSG_DESCRIPTOR_NULL (mach_msg_descriptor_t *) 0

typedef struct
{
  mach_msg_size_t		msgh_descriptor_count;
} mach_msg_body_t;

#define MACH_MSG_BODY_NULL (mach_msg_body_t *) 0

typedef struct
{
  mach_msg_header_t		header;
  mach_msg_body_t		body;
} mach_msg_base_t;

typedef	unsigned int mach_msg_trailer_type_t;

#define	MACH_MSG_TRAILER_FORMAT_0	0

typedef	unsigned int mach_msg_trailer_size_t;

typedef struct 
{
  mach_msg_trailer_type_t	msgh_trailer_type;
  mach_msg_trailer_size_t	msgh_trailer_size;
} mach_msg_trailer_t;

typedef struct
{
  mach_msg_trailer_type_t       msgh_trailer_type;
  mach_msg_trailer_size_t       msgh_trailer_size;
  mach_port_seqno_t             msgh_seqno;
} mach_msg_seqno_trailer_t;

typedef struct
{
  unsigned int			val[2];
} security_id_t;

typedef struct 
{
  mach_msg_trailer_type_t	msgh_trailer_type;
  mach_msg_trailer_size_t	msgh_trailer_size;
  mach_port_seqno_t		msgh_seqno;
  security_id_t			msgh_sender;
} mach_msg_security_trailer_t;

typedef mach_msg_security_trailer_t	mach_msg_format_0_trailer_t;

#define MACH_MSG_TRAILER_FORMAT_0_SIZE	sizeof(mach_msg_format_0_trailer_t)
#define MACH_MSG_TRAILER_MINIMUM_SIZE	sizeof(mach_msg_trailer_t)
#define MAX_TRAILER_SIZE		MACH_MSG_TRAILER_FORMAT_0_SIZE	

#define	ANONYMOUS_SECURITY_ID_VALUE	(security_id_t) { 0, 0 }
#define	KERNEL_SECURITY_ID_VALUE	(security_id_t) { 0, 1 }

/*
 *  Values received/carried in messages.  Tells the receiver what
 *  sort of port right he now has.
 *
 *  MACH_MSG_TYPE_PORT_NAME is used to transfer a port name
 *  which should remain uninterpreted by the kernel.  (Port rights
 *  are not transferred, just the port name.)
 */

#define MACH_MSG_TYPE_PORT_NAME		15
#define MACH_MSG_TYPE_PORT_RECEIVE	MACH_MSG_TYPE_MOVE_RECEIVE
#define MACH_MSG_TYPE_PORT_SEND		MACH_MSG_TYPE_MOVE_SEND
#define MACH_MSG_TYPE_PORT_SEND_ONCE	MACH_MSG_TYPE_MOVE_SEND_ONCE

#define MACH_MSG_TYPE_LAST		22		/* Last assigned */

/*
 *  A dummy value.  Mostly used to indicate that the actual value
 *  will be filled in later, dynamically.
 */

#define MACH_MSG_TYPE_POLYMORPHIC	((mach_msg_type_name_t) -1)

/*
 *	Is a given item a port type?
 */

#define MACH_MSG_TYPE_PORT_ANY(x)			\
	(((x) >= MACH_MSG_TYPE_MOVE_RECEIVE) &&		\
	 ((x) <= MACH_MSG_TYPE_MAKE_SEND_ONCE))

#define	MACH_MSG_TYPE_PORT_ANY_SEND(x)			\
	(((x) >= MACH_MSG_TYPE_MOVE_SEND) &&		\
	 ((x) <= MACH_MSG_TYPE_MAKE_SEND_ONCE))

#define	MACH_MSG_TYPE_PORT_ANY_RIGHT(x)			\
	(((x) >= MACH_MSG_TYPE_MOVE_RECEIVE) &&		\
	 ((x) <= MACH_MSG_TYPE_MOVE_SEND_ONCE))

typedef integer_t mach_msg_option_t;

#define MACH_MSG_OPTION_NONE	0x00000000

#define	MACH_SEND_MSG		0x00000001
#define	MACH_RCV_MSG		0x00000002

#define MACH_SEND_TIMEOUT	0x00000010
#define MACH_SEND_NOTIFY	0x00000020	/* internal use only */
#define MACH_SEND_INTERRUPT	0x00000040	/* libmach implements */
#define MACH_SEND_CANCEL	0x00000080
#define MACH_SEND_ALWAYS	0x00010000	/* internal use only */
#define MACH_SEND_TRAILER	0x00020000
#define MACH_SEND_SWITCH	0x00080000	/* internal use only */

#define MACH_RCV_TIMEOUT	0x00000100
#define MACH_RCV_NOTIFY		0x00000200
#define MACH_RCV_INTERRUPT	0x00000400	/* libmach implements */
#define MACH_RCV_LARGE		0x00000800
#define MACH_RCV_OVERWRITE	0x00001000
#define MACH_RCV_OLD_FORMAT	0x00008000	/* internal use only */
#define MACH_RCV_NO_TRAILER	0x00010000	/* non standard */

/* 
 * NOTE: a 0x00------ RCV mask implies to ask for
 * a MACH_MSG_TRAILER_FORMAT_0 with 0 Elements, 
 * which is equivalent to a mach_msg_trailer_t.
 */
#define MACH_RCV_TRAILER_NULL   0
#define MACH_RCV_TRAILER_SEQNO  1
#define MACH_RCV_TRAILER_SENDER 2

#define MACH_RCV_TRAILER_TYPE(x)     (((x) & 0xf) << 28) 
#define MACH_RCV_TRAILER_ELEMENTS(x) (((x) & 0xf) << 24)  
#define MACH_RCV_TRAILER_MASK 	     ((0xff << 24))

extern mach_msg_size_t trailer_size[];

#define GET_RCV_ELEMENTS(y) (((y) >> 24) & 0xf)
#define REQUESTED_TRAILER_SIZE(y) (trailer_size[GET_RCV_ELEMENTS(y)])

/*
 *  Much code assumes that mach_msg_return_t == kern_return_t.
 *  This definition is useful for descriptive purposes.
 *
 *  See <mach/error.h> for the format of error codes.
 *  IPC errors are system 4.  Send errors are subsystem 0;
 *  receive errors are subsystem 1.  The code field is always non-zero.
 *  The high bits of the code field communicate extra information
 *  for some error codes.  MACH_MSG_MASK masks off these special bits.
 */

typedef kern_return_t mach_msg_return_t;

#define MACH_MSG_SUCCESS		0x00000000

#define	MACH_MSG_MASK			0x00003c00
		/* All special error code bits defined below. */
#define	MACH_MSG_IPC_SPACE		0x00002000
		/* No room in IPC name space for another capability name. */
#define	MACH_MSG_VM_SPACE		0x00001000
		/* No room in VM address space for out-of-line memory. */
#define	MACH_MSG_IPC_KERNEL		0x00000800
		/* Kernel resource shortage handling an IPC capability. */
#define	MACH_MSG_VM_KERNEL		0x00000400
		/* Kernel resource shortage handling out-of-line memory. */

#define MACH_SEND_IN_PROGRESS		0x10000001
		/* Thread is waiting to send.  (Internal use only.) */
#define MACH_SEND_INVALID_DATA		0x10000002
		/* Bogus in-line data. */
#define MACH_SEND_INVALID_DEST		0x10000003
		/* Bogus destination port. */
#define MACH_SEND_TIMED_OUT		0x10000004
		/* Message not sent before timeout expired. */
#define MACH_SEND_WILL_NOTIFY		0x10000005
		/* Msg-accepted notification will be generated. */
#define MACH_SEND_NOTIFY_IN_PROGRESS	0x10000006
		/* Msg-accepted notification already pending. */
#define MACH_SEND_INTERRUPTED		0x10000007
		/* Software interrupt. */
#define MACH_SEND_MSG_TOO_SMALL		0x10000008
		/* Data doesn't contain a complete message. */
#define MACH_SEND_INVALID_REPLY		0x10000009
		/* Bogus reply port. */
#define MACH_SEND_INVALID_RIGHT		0x1000000a
		/* Bogus port rights in the message body. */
#define MACH_SEND_INVALID_NOTIFY	0x1000000b
		/* Bogus notify port argument. */
#define MACH_SEND_INVALID_MEMORY	0x1000000c
		/* Invalid out-of-line memory pointer. */
#define MACH_SEND_NO_BUFFER		0x1000000d
		/* No message buffer is available. */
#define MACH_SEND_NO_NOTIFY		0x1000000e
		/* Resource shortage; can't request msg-accepted notif. */
#define MACH_SEND_INVALID_TYPE		0x1000000f
		/* Invalid msg-type specification. */
#define MACH_SEND_INVALID_HEADER	0x10000010
		/* A field in the header had a bad value. */
#define MACH_SEND_INVALID_TRAILER	0x10000011
		/* The trailer to be sent does not match kernel format. */

#define MACH_RCV_IN_PROGRESS		0x10004001
		/* Thread is waiting for receive.  (Internal use only.) */
#define MACH_RCV_INVALID_NAME		0x10004002
		/* Bogus name for receive port/port-set. */
#define MACH_RCV_TIMED_OUT		0x10004003
		/* Didn't get a message within the timeout value. */
#define MACH_RCV_TOO_LARGE		0x10004004
		/* Message buffer is not large enough for inline data. */
#define MACH_RCV_INTERRUPTED		0x10004005
		/* Software interrupt. */
#define MACH_RCV_PORT_CHANGED		0x10004006
		/* Port moved into a set during the receive. */
#define MACH_RCV_INVALID_NOTIFY		0x10004007
		/* Bogus notify port argument. */
#define MACH_RCV_INVALID_DATA		0x10004008
		/* Bogus message buffer for inline data. */
#define MACH_RCV_PORT_DIED		0x10004009
		/* Port/set was sent away/died during receive. */
#define	MACH_RCV_IN_SET			0x1000400a
		/* Port is a member of a port set. */
#define	MACH_RCV_HEADER_ERROR		0x1000400b
		/* Error receiving message header.  See special bits. */
#define	MACH_RCV_BODY_ERROR		0x1000400c
		/* Error receiving message body.  See special bits. */
#define	MACH_RCV_INVALID_TYPE		0x1000400d
		/* Invalid msg-type specification in scatter list. */
#define	MACH_RCV_SCATTER_SMALL		0x1000400e
		/* Out-of-line overwrite region is not large enough */
#define MACH_RCV_INVALID_TRAILER	0x1000400f
	/* The trailer type or the number of trailer elements aren't supported */

extern mach_msg_return_t	mach_msg(
					mach_msg_header_t *msg,
					mach_msg_option_t option,
					mach_msg_size_t send_size,
					mach_msg_size_t rcv_limit,
					mach_port_t rcv_name,
					mach_msg_timeout_t timeout,
					mach_port_t notify);

extern mach_msg_return_t	mach_msg_overwrite(
					mach_msg_header_t *msg,
					mach_msg_option_t option,
					mach_msg_size_t send_size,
					mach_msg_size_t rcv_limit,
					mach_port_t rcv_name,
					mach_msg_timeout_t timeout,
					mach_port_t notify,
					mach_msg_header_t *rcv_msg,
					mach_msg_size_t rcv_msg_size);

extern mach_msg_return_t	mach_msg_simple_trap(
					mach_msg_header_t *msg,
					mach_msg_option_t option,
					mach_msg_size_t send_size,
					mach_msg_size_t rcv_limit,
					mach_port_t rcv_name);

extern mach_msg_return_t	mach_msg_trap(
					mach_msg_header_t *msg,
					mach_msg_option_t option,
					mach_msg_size_t send_size,
					mach_msg_size_t rcv_limit,
					mach_port_t rcv_name,
					mach_msg_timeout_t timeout,
					mach_port_t notify);

extern mach_msg_return_t	mach_msg_overwrite_trap(
					mach_msg_header_t *msg,
					mach_msg_option_t option,
					mach_msg_size_t send_size,
					mach_msg_size_t rcv_limit,
					mach_port_t rcv_name,
					mach_msg_timeout_t timeout,
					mach_port_t notify,
					mach_msg_header_t *rcv_msg,
					mach_msg_size_t rcv_msg_size);


/* Definitions for the old IPC interface. */

/*
 *	Message data structures.
 *
 *	Messages consist of two parts: a fixed-size header, immediately
 *	followed by a variable-size array of typed data items.
 *
 */

typedef	unsigned int	msg_size_t;

typedef	struct {
		unsigned int	msg_unused : 24,
				msg_simple : 8;
		msg_size_t	msg_size;
		integer_t	msg_type;
		port_t		msg_local_port;
		port_t		msg_remote_port;
		integer_t	msg_id;
} msg_header_t;

#define MSG_SIZE_MAX	8192

/*
 *	Known values for the msg_type field.
 *	These are Accent holdovers, which should be purged when possible.
 *
 *	Only one bit in the msg_type field is used by the kernel.
 *	Others are available to user applications.  See <msg_type.h>
 *	for system application-assigned values.
 */

#define MSG_TYPE_NORMAL		0
#define MSG_TYPE_EMERGENCY	1

/*
 *	Each data item is preceded by a description of that
 *	item, including what type of data, how big it is, and
 *	how many of them are present.
 *
 *	The actual data will either follow this type
 *	descriptor ("inline") or will be specified by a pointer.
 *
 *	If the type name, size, or number is too large to be encoded
 *	in this structure, the "longform" option may be selected,
 *	and those fields must immediately follow in full integer fields.
 *
 *	For convenience, out-of-line data regions or port rights may
 *	be deallocated when the message is sent by specifying the
 *	"deallocate" field.  Beware: if the data item in question is both
 *	out-of-line and contains port rights, then both will be deallocated.
 */

typedef struct  {
	unsigned int	msg_type_name : 8,
				/* What kind of data */
			msg_type_size : 8,
				/* How many bits is each item */
			msg_type_number : 12,
				/* How many items are there */
			msg_type_inline : 1,
				/* If true, data follows; else a pointer */
			msg_type_longform : 1,
				/* Name, size, number follow: see above */
			msg_type_deallocate : 1,
				/* Deallocate port rights or memory */
			msg_type_unused : 1;
} msg_type_t;

typedef	struct	{
	msg_type_t	msg_type_header;
	unsigned short	msg_type_long_name;
	unsigned short	msg_type_long_size;
	natural_t	msg_type_long_number;
} msg_type_long_t;

/*
 *	Known values for the msg_type_name field.
 *
 *	The only types known to the Mach kernel are
 *	the port types, and those types used in the
 *	kernel RPC interface.
 */

#define MSG_TYPE_UNSTRUCTURED	0
#define MSG_TYPE_BIT		0
#define MSG_TYPE_BOOLEAN	0
#define MSG_TYPE_INTEGER_16	1
#define MSG_TYPE_INTEGER_32	2
#define MSG_TYPE_PORT_OWNERSHIP	3	/* obsolete */
#define MSG_TYPE_PORT_RECEIVE	4	/* obsolete */
#define MSG_TYPE_PORT_ALL	5
#define MSG_TYPE_PORT		6
#define MSG_TYPE_CHAR		8
#define MSG_TYPE_BYTE		9
#define MSG_TYPE_INTEGER_8	9
#define MSG_TYPE_REAL		10
#define MSG_TYPE_STRING		12
#define MSG_TYPE_STRING_C	12
/*	MSG_TYPE_INVALID	13	unused */
/*				14	unused */

#define MSG_TYPE_PORT_NAME	15		/* A capability name */
#define MSG_TYPE_LAST		16		/* Last assigned */

#define MSG_TYPE_POLYMORPHIC	((unsigned int) -1)

/*
 *	Is a given item a port type?
 */

#define MSG_TYPE_PORT_ANY(x)	\
	(((x) == MSG_TYPE_PORT) || ((x) == MSG_TYPE_PORT_ALL))

/*
 *	Other basic types
 */

typedef natural_t	msg_timeout_t;

/*
 *	Options to IPC primitives.
 *
 *	These can be combined by or'ing; the combination RPC call
 *	uses both SEND_ and RCV_ options at once.
 */

typedef	integer_t		msg_option_t;

#define MSG_OPTION_NONE	0x0000	/* Terminate only when message op works */

#define SEND_TIMEOUT	0x0001	/* Terminate on timeout elapsed */
#define SEND_NOTIFY	0x0002	/* Terminate with reply message if need be */

#define SEND_INTERRUPT	0x0004	/* Terminate on software interrupt */

#define SEND_SWITCH	0x0020	/* Use handoff scheduling */

#define RCV_TIMEOUT	0x0100	/* Terminate on timeout elapsed */
#define RCV_NO_SENDERS	0x0200	/* Terminate if I'm the only sender left */
#define RCV_INTERRUPT	0x0400	/* Terminate on software interrupt */
#define RCV_LARGE	0x1000

/*
 *	Returns from IPC primitives.
 *
 *	Values are separate in order to allow RPC users to
 *	distinguish which operation failed; for successful completion,
 *	this doesn't matter.
 */

typedef	int		msg_return_t;

#define SEND_SUCCESS		0

#define SEND_ERRORS_START	-100
#define SEND_INVALID_MEMORY	-101	/* Message or OOL data invalid */
#define SEND_INVALID_PORT	-102	/* Reference to inacessible port */
#define SEND_TIMED_OUT		-103	/* Terminated due to timeout */
#define SEND_WILL_NOTIFY	-105	/* Msg accepted provisionally */
#define SEND_NOTIFY_IN_PROGRESS	-106	/* Already awaiting a notification */
#define SEND_KERNEL_REFUSED	-107	/* Message to the kernel refused */
#define SEND_INTERRUPTED	-108	/* Software interrupt during send */
#define SEND_MSG_TOO_LARGE	-109	/* Message specified was too large */
#define SEND_MSG_TOO_SMALL	-110	/* Data specified exceeds msg size */
/*	SEND_MSG_SIZE_CHANGE	-111	   Msg size changed during copy */
#define SEND_ERRORS_END		-111

#define msg_return_send(x)	((x) < SEND_ERRORS_START &&	\
						(x) > SEND_ERRORS_END)

#define RCV_SUCCESS		0

#define RCV_ERRORS_START	-200
#define RCV_INVALID_MEMORY	-201
#define RCV_INVALID_PORT	-202
#define RCV_TIMED_OUT		-203
#define RCV_TOO_LARGE		-204	/* Msg structure too small for data */
#define RCV_NOT_ENOUGH_MEMORY	-205	/* Can't find space for OOL data */
#define RCV_ONLY_SENDER		-206	/* Receiver is only sender */
#define RCV_INTERRUPTED		-207
#define RCV_PORT_CHANGE		-208	/* Port was put in a set */
#define RCV_ERRORS_END		-209

#define msg_return_rcv(x)	((x) < RCV_ERRORS_START &&	\
						(x) > RCV_ERRORS_END)

#define RPC_SUCCESS		0

/*
 *	The IPC primitive functions themselves
 */

msg_return_t	msg_send(
	msg_header_t	*header,
	msg_option_t	option,
	msg_timeout_t	timeout);

msg_return_t	msg_receive(
	msg_header_t	*header,
	msg_option_t	option,
	msg_timeout_t	timeout);

msg_return_t	msg_rpc(
	msg_header_t	*header,	/* in/out */
	msg_option_t	option,
	msg_size_t	rcv_size,
	msg_timeout_t	send_timeout,
	msg_timeout_t	rcv_timeout);

msg_return_t	msg_send_trap(
	msg_header_t	*header,
	msg_option_t	option,
	msg_size_t	send_size,
	msg_timeout_t	timeout);

msg_return_t	msg_receive_trap(
	msg_header_t	*header,
	msg_option_t	option,
	msg_size_t	rcv_size,
	port_name_t	rcv_name,
	msg_timeout_t	timeout);

msg_return_t	msg_rpc_trap(
	msg_header_t	*header,	/* in/out */
	msg_option_t	option,
	msg_size_t	send_size,
	msg_size_t	rcv_size,
	msg_timeout_t	send_timeout,
	msg_timeout_t	rcv_timeout);

#endif	/* _MACH_MESSAGE_H_ */
