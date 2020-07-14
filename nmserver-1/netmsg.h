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

#ifndef _NETMSG_
#define _NETMSG_

#include "config.h"
#include "debug.h"

/*
 * Function argument order is IN, INOUT, OUT, REF. These psuedo-keywords are
 * used to label the arguments. For example: 
 *
 * Char FunReturningChar(   IN arg1, arg2, INOUT arg3, OUT arg4, arg5, REF arg6) 
 *
 * IN    indicates arg value is read, and not written. All non-pointer arguments
 * will be IN. 
 *
 * INOUT indicates arg value may be both read and written. 
 *
 * OUT   indicates arg value is only written. INOUT and OUT apply to values
 * referenced by pointers. 
 *
 * REF   indicates arg is a pointer to shared data.  Data may be read or
 * written, but may not meaningfully be copied. 
 *
 * Note: 
 *
 * Functions for which all arguments are IN need not use any of these keywords. 
 *
 */

#define IN
#define INOUT
#define OUT
#define REF

#define EXPORT
#define PUBLIC

#include "debug.h"

#define DEBUGOFF	1
#define LOCKTRACING	1
#define tracing_on	debug.tracing
#define trace_lock	log_lock
#define PRIVATE static
#include "trace.h"


extern port_set_name_t nm_port_set;

#endif _NETMSG_
