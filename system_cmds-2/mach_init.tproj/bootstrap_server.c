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
/* Module bootstrap */

#define EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/mig_errors.h>

#ifndef	mig_internal
#define	mig_internal	static
#endif

#ifndef	mig_external
#define	 mig_external	extern
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
#include <mach/std_types.h>
#include <servers/bootstrap_defs.h>

/* Routine bootstrap_check_in */
mig_internal novalue _Xbootstrap_check_in
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_all_t service_port;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_check_in (port_t bootstrap_port, name_t service_name, port_all_t *service_port);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t service_portType = {
		/* msg_type_name = */		MSG_TYPE_PORT_ALL,
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
	if ((msg_size != 164) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->service_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_nameType.msg_type_long_number != 1) ||
	    (In0P->service_nameType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_check_in(In0P->Head.msg_request_port, In0P->service_name, &OutP->service_port);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->service_portType = service_portType;
#else	UseStaticMsgType
	OutP->service_portType.msg_type_name = MSG_TYPE_PORT_ALL;
	OutP->service_portType.msg_type_size = 32;
	OutP->service_portType.msg_type_number = 1;
	OutP->service_portType.msg_type_inline = TRUE;
	OutP->service_portType.msg_type_longform = FALSE;
	OutP->service_portType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_register */
mig_internal novalue _Xbootstrap_register
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
		msg_type_t service_portType;
		port_t service_port;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_register (port_t bootstrap_port, name_t service_name, port_t service_port);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t service_portCheck = {
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
	if ((msg_size != 172) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->service_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_nameType.msg_type_long_number != 1) ||
	    (In0P->service_nameType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->service_portType != * (int *) &service_portCheck)
#else	UseStaticMsgType
	if ((In0P->service_portType.msg_type_inline != TRUE) ||
	    (In0P->service_portType.msg_type_longform != FALSE) ||
	    (In0P->service_portType.msg_type_name != MSG_TYPE_PORT) ||
	    (In0P->service_portType.msg_type_number != 1) ||
	    (In0P->service_portType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_register(In0P->Head.msg_request_port, In0P->service_name, In0P->service_port);
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

/* Routine bootstrap_look_up */
mig_internal novalue _Xbootstrap_look_up
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_t service_port;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_look_up (port_t bootstrap_port, name_t service_name, port_t *service_port);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t service_portType = {
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
	if ((msg_size != 164) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->service_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_nameType.msg_type_long_number != 1) ||
	    (In0P->service_nameType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_look_up(In0P->Head.msg_request_port, In0P->service_name, &OutP->service_port);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->service_portType = service_portType;
#else	UseStaticMsgType
	OutP->service_portType.msg_type_name = MSG_TYPE_PORT;
	OutP->service_portType.msg_type_size = 32;
	OutP->service_portType.msg_type_number = 1;
	OutP->service_portType.msg_type_inline = TRUE;
	OutP->service_portType.msg_type_longform = FALSE;
	OutP->service_portType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_look_up_array */
mig_internal novalue _Xbootstrap_look_up_array
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_namesType;
		name_array_t service_names;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t service_portsType;
		port_array_t service_ports;
		msg_type_t all_services_knownType;
		boolean_t all_services_known;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_look_up_array (port_t bootstrap_port, name_array_t service_names, unsigned int service_namesCnt, port_array_t *service_ports, unsigned int *service_portsCnt, boolean_t *all_services_known);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_long_t service_portsType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_PORT,
		/* msg_type_long_size = */	32,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t all_services_knownType = {
		/* msg_type_name = */		MSG_TYPE_BOOLEAN,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
		/* msg_type_unused = */		0,
	};
#endif	UseStaticMsgType

	unsigned int service_portsCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 40) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_namesType.msg_type_header.msg_type_inline != FALSE) ||
	    (In0P->service_namesType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_namesType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_namesType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_look_up_array(In0P->Head.msg_request_port, In0P->service_names, In0P->service_namesType.msg_type_long_number, &OutP->service_ports, &service_portsCnt, &OutP->all_services_known);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 56;	

#if	UseStaticMsgType
	OutP->service_portsType = service_portsType;
#else	UseStaticMsgType
	OutP->service_portsType.msg_type_long_name = MSG_TYPE_PORT;
	OutP->service_portsType.msg_type_long_size = 32;
	OutP->service_portsType.msg_type_header.msg_type_inline = FALSE;
	OutP->service_portsType.msg_type_header.msg_type_longform = TRUE;
	OutP->service_portsType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->service_portsType.msg_type_long_number /* service_portsCnt */ = /* service_portsType.msg_type_long_number */ service_portsCnt;

#if	UseStaticMsgType
	OutP->all_services_knownType = all_services_knownType;
#else	UseStaticMsgType
	OutP->all_services_knownType.msg_type_name = MSG_TYPE_BOOLEAN;
	OutP->all_services_knownType.msg_type_size = 32;
	OutP->all_services_knownType.msg_type_number = 1;
	OutP->all_services_knownType.msg_type_inline = TRUE;
	OutP->all_services_knownType.msg_type_longform = FALSE;
	OutP->all_services_knownType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_status */
mig_internal novalue _Xbootstrap_status
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_activeType;
		boolean_t service_active;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_status (port_t bootstrap_port, name_t service_name, boolean_t *service_active);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t service_activeType = {
		/* msg_type_name = */		MSG_TYPE_BOOLEAN,
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
	if ((msg_size != 164) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->service_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_nameType.msg_type_long_number != 1) ||
	    (In0P->service_nameType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_status(In0P->Head.msg_request_port, In0P->service_name, &OutP->service_active);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->service_activeType = service_activeType;
#else	UseStaticMsgType
	OutP->service_activeType.msg_type_name = MSG_TYPE_BOOLEAN;
	OutP->service_activeType.msg_type_size = 32;
	OutP->service_activeType.msg_type_number = 1;
	OutP->service_activeType.msg_type_inline = TRUE;
	OutP->service_activeType.msg_type_longform = FALSE;
	OutP->service_activeType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_info */
mig_internal novalue _Xbootstrap_info
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t service_namesType;
		name_array_t service_names;
		msg_type_long_t server_namesType;
		name_array_t server_names;
		msg_type_long_t service_activeType;
		bool_array_t service_active;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_info (port_t bootstrap_port, name_array_t *service_names, unsigned int *service_namesCnt, name_array_t *server_names, unsigned int *server_namesCnt, bool_array_t *service_active, unsigned int *service_activeCnt);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_long_t service_namesType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	TRUE,
	},
		/* msg_type_long_name = */	MSG_TYPE_STRING,
		/* msg_type_long_size = */	1024,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t server_namesType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	TRUE,
	},
		/* msg_type_long_name = */	MSG_TYPE_STRING,
		/* msg_type_long_size = */	1024,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t service_activeType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	TRUE,
	},
		/* msg_type_long_name = */	MSG_TYPE_BOOLEAN,
		/* msg_type_long_size = */	32,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

	unsigned int service_namesCnt;
	unsigned int server_namesCnt;
	unsigned int service_activeCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 24) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_info(In0P->Head.msg_request_port, &OutP->service_names, &service_namesCnt, &OutP->server_names, &server_namesCnt, &OutP->service_active, &service_activeCnt);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 80;	

#if	UseStaticMsgType
	OutP->service_namesType = service_namesType;
#else	UseStaticMsgType
	OutP->service_namesType.msg_type_long_name = MSG_TYPE_STRING;
	OutP->service_namesType.msg_type_long_size = 1024;
	OutP->service_namesType.msg_type_header.msg_type_inline = FALSE;
	OutP->service_namesType.msg_type_header.msg_type_longform = TRUE;
	OutP->service_namesType.msg_type_header.msg_type_deallocate = TRUE;
#endif	UseStaticMsgType

	OutP->service_namesType.msg_type_long_number /* service_namesCnt */ = /* service_namesType.msg_type_long_number */ service_namesCnt;

#if	UseStaticMsgType
	OutP->server_namesType = server_namesType;
#else	UseStaticMsgType
	OutP->server_namesType.msg_type_long_name = MSG_TYPE_STRING;
	OutP->server_namesType.msg_type_long_size = 1024;
	OutP->server_namesType.msg_type_header.msg_type_inline = FALSE;
	OutP->server_namesType.msg_type_header.msg_type_longform = TRUE;
	OutP->server_namesType.msg_type_header.msg_type_deallocate = TRUE;
#endif	UseStaticMsgType

	OutP->server_namesType.msg_type_long_number /* server_namesCnt */ = /* server_namesType.msg_type_long_number */ server_namesCnt;

#if	UseStaticMsgType
	OutP->service_activeType = service_activeType;
#else	UseStaticMsgType
	OutP->service_activeType.msg_type_long_name = MSG_TYPE_BOOLEAN;
	OutP->service_activeType.msg_type_long_size = 32;
	OutP->service_activeType.msg_type_header.msg_type_inline = FALSE;
	OutP->service_activeType.msg_type_header.msg_type_longform = TRUE;
	OutP->service_activeType.msg_type_header.msg_type_deallocate = TRUE;
#endif	UseStaticMsgType

	OutP->service_activeType.msg_type_long_number /* service_activeCnt */ = /* service_activeType.msg_type_long_number */ service_activeCnt;

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_subset */
mig_internal novalue _Xbootstrap_subset
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_t requestor_portType;
		port_t requestor_port;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t subset_portType;
		port_t subset_port;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_subset (port_t bootstrap_port, port_t requestor_port, port_t *subset_port);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t requestor_portCheck = {
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
	static const msg_type_t subset_portType = {
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
	if ((msg_size != 32) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->requestor_portType != * (int *) &requestor_portCheck)
#else	UseStaticMsgType
	if ((In0P->requestor_portType.msg_type_inline != TRUE) ||
	    (In0P->requestor_portType.msg_type_longform != FALSE) ||
	    (In0P->requestor_portType.msg_type_name != MSG_TYPE_PORT) ||
	    (In0P->requestor_portType.msg_type_number != 1) ||
	    (In0P->requestor_portType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_subset(In0P->Head.msg_request_port, In0P->requestor_port, &OutP->subset_port);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->subset_portType = subset_portType;
#else	UseStaticMsgType
	OutP->subset_portType.msg_type_name = MSG_TYPE_PORT;
	OutP->subset_portType.msg_type_size = 32;
	OutP->subset_portType.msg_type_number = 1;
	OutP->subset_portType.msg_type_inline = TRUE;
	OutP->subset_portType.msg_type_longform = FALSE;
	OutP->subset_portType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine bootstrap_create_service */
mig_internal novalue _Xbootstrap_create_service
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_t service_port;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t x_bootstrap_create_service (port_t bootstrap_port, name_t service_name, port_t *service_port);

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t service_portType = {
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
	if ((msg_size != 164) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->service_nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->service_nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->service_nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->service_nameType.msg_type_long_number != 1) ||
	    (In0P->service_nameType.msg_type_long_size != 1024))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = x_bootstrap_create_service(In0P->Head.msg_request_port, In0P->service_name, &OutP->service_port);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;	

#if	UseStaticMsgType
	OutP->service_portType = service_portType;
#else	UseStaticMsgType
	OutP->service_portType.msg_type_name = MSG_TYPE_PORT;
	OutP->service_portType.msg_type_size = 32;
	OutP->service_portType.msg_type_number = 1;
	OutP->service_portType.msg_type_inline = TRUE;
	OutP->service_portType.msg_type_longform = FALSE;
	OutP->service_portType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

boolean_t bootstrap_server
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

	if ((InP->msg_id > 410) || (InP->msg_id < 400))
		return FALSE;
	else {
		typedef novalue (*SERVER_STUB_PROC)
			(msg_header_t *, msg_header_t *);
		static const SERVER_STUB_PROC routines[] = {
			0,
			0,
			_Xbootstrap_check_in,
			_Xbootstrap_register,
			_Xbootstrap_look_up,
			_Xbootstrap_look_up_array,
			0,
			_Xbootstrap_status,
			_Xbootstrap_info,
			_Xbootstrap_subset,
			_Xbootstrap_create_service,
		};

		if (routines[InP->msg_id - 400])
			(routines[InP->msg_id - 400]) (InP, &OutP->Head);
		 else
			return FALSE;
	}
	return TRUE;
}
