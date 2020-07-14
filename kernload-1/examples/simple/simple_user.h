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
#include "simple.h"
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/msg_type.h>
#if	!defined(KERNEL) && !defined(MIG_NO_STRINGS)
#include <strings.h>
#endif
/* LINTLIBRARY */

extern port_t mig_get_reply_port();
extern void mig_dealloc_reply_port();

#ifndef	mig_internal
#define	mig_internal	static
#endif

#ifndef	TypeCheck
#define	TypeCheck 1
#endif

#ifndef	UseExternRCSId
#ifdef	hc
#define	UseExternRCSId		1
#endif
#endif

#ifndef	UseStaticMsgType
#if	!defined(hc) || defined(__STDC__)
#define	UseStaticMsgType	1
#endif
#endif

#define msg_request_port	msg_remote_port
#define msg_reply_port		msg_local_port


/* SimpleRoutine puts */
mig_external kern_return_t simple_puts (
	port_t simple_port,
	simple_msg_t string)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t stringType;
		simple_msg_t string;
	} Request;

	union {
		Request In;
	} Mess;

	register Request *InP = &Mess.In;

	unsigned int msg_size = 292;

#if	UseStaticMsgType
	static const msg_type_long_t stringType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_STRING,
		/* msg_type_long_size = */	2048,
		/* msg_type_long_number = */	1,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	InP->stringType = stringType;
#else	UseStaticMsgType
	InP->stringType.msg_type_long_name = MSG_TYPE_STRING;
	InP->stringType.msg_type_long_size = 2048;
	InP->stringType.msg_type_long_number = 1;
	InP->stringType.msg_type_header.msg_type_inline = TRUE;
	InP->stringType.msg_type_header.msg_type_longform = TRUE;
	InP->stringType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	(void) strncpy(InP->string /* string */, /* string */ string, 256);
	InP->string /* string */[255] = '\0';

	InP->Head.msg_simple = TRUE;
	InP->Head.msg_size = msg_size;
	InP->Head.msg_type = MSG_TYPE_NORMAL;
	InP->Head.msg_request_port = simple_port;
	InP->Head.msg_reply_port = PORT_NULL;
	InP->Head.msg_id = 100;

	return msg_send(&InP->Head, MSG_OPTION_NONE, 0);
}

/* Routine vers */
mig_external kern_return_t simple_vers (
	port_t simple_port,
	simple_msg_t string)
{
	typedef struct {
		msg_header_t Head;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t stringType;
		simple_msg_t string;
	} Reply;

	union {
		Request In;
		Reply Out;
	} Mess;

	register Request *InP = &Mess.In;
	register Reply *OutP = &Mess.Out;

	msg_return_t msg_result;

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size = 24;

#if	UseStaticMsgType
	static const msg_type_t RetCodeCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0
	};
#endif	UseStaticMsgType

	InP->Head.msg_simple = TRUE;
	InP->Head.msg_size = msg_size;
	InP->Head.msg_type = MSG_TYPE_NORMAL | MSG_TYPE_RPC;
	InP->Head.msg_request_port = simple_port;
	InP->Head.msg_reply_port = mig_get_reply_port();
	InP->Head.msg_id = 101;

	msg_result = msg_rpc(&InP->Head, MSG_OPTION_NONE, sizeof(Reply), 0, 0);
	if (msg_result != RPC_SUCCESS) {
		if (msg_result == RCV_INVALID_PORT)
			mig_dealloc_reply_port();
		return msg_result;
	}

#if	TypeCheck
	msg_size = OutP->Head.msg_size;
	msg_simple = OutP->Head.msg_simple;
#endif	TypeCheck

	if (OutP->Head.msg_id != 201)
		return MIG_REPLY_MISMATCH;

#if	TypeCheck
	if (((msg_size != 300) || (msg_simple != TRUE)) &&
	    ((msg_size != sizeof(death_pill_t)) ||
	     (msg_simple != TRUE) ||
	     (OutP->RetCode == KERN_SUCCESS)))
		return MIG_TYPE_ERROR;
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &OutP->RetCodeType != * (int *) &RetCodeCheck)
#else	UseStaticMsgType
	if ((OutP->RetCodeType.msg_type_inline != TRUE) ||
	    (OutP->RetCodeType.msg_type_longform != FALSE) ||
	    (OutP->RetCodeType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (OutP->RetCodeType.msg_type_number != 1) ||
	    (OutP->RetCodeType.msg_type_size != 32))
#endif	UseStaticMsgType
		return MIG_TYPE_ERROR;
#endif	TypeCheck

	if (OutP->RetCode != KERN_SUCCESS)
		return OutP->RetCode;

#if	TypeCheck
	if ((OutP->stringType.msg_type_header.msg_type_inline != TRUE) ||
	    (OutP->stringType.msg_type_header.msg_type_longform != TRUE) ||
	    (OutP->stringType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (OutP->stringType.msg_type_long_number != 1) ||
	    (OutP->stringType.msg_type_long_size != 2048))
		return MIG_TYPE_ERROR;
#endif	TypeCheck

	(void) strncpy(string /* string */, /* string */ OutP->string, 256);
	string /* string */[255] = '\0';

	return OutP->RetCode;
}
