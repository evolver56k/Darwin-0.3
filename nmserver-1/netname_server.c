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
/* Module netname */

#define EXPORT_BOOLEAN
#ifdef NeXT_PDO
#include <mach/mach.h>
#else
#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#endif

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

#define novalue void

#define msg_request_port	msg_local_port
#define msg_reply_port		msg_remote_port
#include "netname_defs.h"

/* Routine netname_check_in */
mig_internal novalue _Xnetname_check_in
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t port_nameType;
		netname_name_t port_name;
		msg_type_t signatureType;
		port_t signature;
		msg_type_t port_idType;
		port_t port_id;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t _netname_check_in (port_t server_port, netname_name_t port_name, port_t signature, port_t port_id);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t signatureCheck = {
		/* msg_type_name = */		MSG_TYPE_PORT,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t port_idCheck = {
		/* msg_type_name = */		MSG_TYPE_PORT,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0
	};
#endif	UseStaticMsgType

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 132) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->port_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->port_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->port_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->port_nameType.msg_type_long_number != 1) ||
	    (In0P->port_nameType.msg_type_long_size != 640))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->signatureType != * (int *) &signatureCheck)
#else	UseStaticMsgType
	if ((In0P->signatureType.msg_type_inline != TRUE) ||
	    (In0P->signatureType.msg_type_longform != FALSE) ||
	    (In0P->signatureType.msg_type_name != MSG_TYPE_PORT) ||
	    (In0P->signatureType.msg_type_number != 1) ||
	    (In0P->signatureType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->port_idType != * (int *) &port_idCheck)
#else	UseStaticMsgType
	if ((In0P->port_idType.msg_type_inline != TRUE) ||
	    (In0P->port_idType.msg_type_longform != FALSE) ||
	    (In0P->port_idType.msg_type_name != MSG_TYPE_PORT) ||
	    (In0P->port_idType.msg_type_number != 1) ||
	    (In0P->port_idType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = _netname_check_in(In0P->Head.msg_request_port, In0P->port_name, In0P->signature, In0P->port_id);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 32;	

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Routine netname_look_up */
mig_internal novalue _Xnetname_look_up
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t host_nameType;
		netname_name_t host_name;
		msg_type_long_t port_nameType;
		netname_name_t port_name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t port_idType;
		port_t port_id;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t _netname_look_up (port_t server_port, netname_name_t host_name, netname_name_t port_name, port_t *port_id);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t port_idType = {
		/* msg_type_name = */		MSG_TYPE_PORT,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0,
	};
#endif	UseStaticMsgType

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 208) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->host_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->host_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->host_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->host_nameType.msg_type_long_number != 1) ||
	    (In0P->host_nameType.msg_type_long_size != 640))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->port_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->port_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->port_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->port_nameType.msg_type_long_number != 1) ||
	    (In0P->port_nameType.msg_type_long_size != 640))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = _netname_look_up(In0P->Head.msg_request_port, In0P->host_name, In0P->port_name, &OutP->port_id);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->port_idType = port_idType;
#else	UseStaticMsgType
	OutP->port_idType.msg_type_name = MSG_TYPE_PORT;
	OutP->port_idType.msg_type_size = 32;
	OutP->port_idType.msg_type_number = 1;
	OutP->port_idType.msg_type_inline = TRUE;
	OutP->port_idType.msg_type_longform = FALSE;
	OutP->port_idType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine netname_check_out */
mig_internal novalue _Xnetname_check_out
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t port_nameType;
		netname_name_t port_name;
		msg_type_t signatureType;
		port_t signature;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t _netname_check_out (port_t server_port, netname_name_t port_name, port_t signature);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t signatureCheck = {
		/* msg_type_name = */		MSG_TYPE_PORT,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0
	};
#endif	UseStaticMsgType

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 124) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->port_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->port_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->port_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->port_nameType.msg_type_long_number != 1) ||
	    (In0P->port_nameType.msg_type_long_size != 640))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->signatureType != * (int *) &signatureCheck)
#else	UseStaticMsgType
	if ((In0P->signatureType.msg_type_inline != TRUE) ||
	    (In0P->signatureType.msg_type_longform != FALSE) ||
	    (In0P->signatureType.msg_type_name != MSG_TYPE_PORT) ||
	    (In0P->signatureType.msg_type_number != 1) ||
	    (In0P->signatureType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = _netname_check_out(In0P->Head.msg_request_port, In0P->port_name, In0P->signature);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 32;	

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Routine netname_version */
mig_internal novalue _Xnetname_version
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t versionType;
		netname_name_t version;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t _netname_version (port_t server_port, netname_name_t version);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_long_t versionType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_STRING,
		/* msg_type_long_size = */	640,
		/* msg_type_long_number = */	1,
	};
#endif	UseStaticMsgType

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 24) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

	OutP->RetCode = _netname_version(In0P->Head.msg_request_port, OutP->version);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 124;	

#if	UseStaticMsgType
	OutP->versionType = versionType;
#else	UseStaticMsgType
	OutP->versionType.msg_type_long_name = MSG_TYPE_STRING;
	OutP->versionType.msg_type_long_size = 640;
	OutP->versionType.msg_type_long_number = 1;
	OutP->versionType.msg_type_header.msg_type_inline = TRUE;
	OutP->versionType.msg_type_header.msg_type_longform = TRUE;
	OutP->versionType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

boolean_t netname_server
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	register msg_header_t *InP =  InHeadP;
	register death_pill_t *OutP = (death_pill_t *) OutHeadP;

#if	UseStaticMsgType
	static const msg_type_t RetCodeType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0,
	};
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = sizeof *OutP;
	OutP->Head.msg_type = InP->msg_type;
	OutP->Head.msg_local_port = PORT_NULL;
	OutP->Head.msg_remote_port = InP->msg_reply_port;
	OutP->Head.msg_id = InP->msg_id + 100;

#if	UseStaticMsgType
	OutP->RetCodeType = RetCodeType;
#else	UseStaticMsgType
	OutP->RetCodeType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->RetCodeType.msg_type_size = 32;
	OutP->RetCodeType.msg_type_number = 1;
	OutP->RetCodeType.msg_type_inline = TRUE;
	OutP->RetCodeType.msg_type_longform = FALSE;
	OutP->RetCodeType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType
	OutP->RetCode = MIG_BAD_ID;

	if ((InP->msg_id > 1043) || (InP->msg_id < 1040))
		return FALSE;
	else {
		typedef novalue (*SERVER_STUB_PROC)
			(msg_header_t *, msg_header_t *);
		static const SERVER_STUB_PROC routines[] = {
			_Xnetname_check_in,
			_Xnetname_look_up,
			_Xnetname_check_out,
			_Xnetname_version,
		};

		if (routines[InP->msg_id - 1040])
			(routines[InP->msg_id - 1040]) (InP, &OutP->Head);
		 else
			return FALSE;
	}
	return TRUE;
}
