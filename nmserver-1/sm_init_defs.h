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

#ifndef	_SM_INIT_DEFS_
#define	_SM_INIT_DEFS_

#include <sys/mach_param.h>

#define NAME_SERVER_INDEX	0
#define AUTH_SERVER_INDEX	1
#define SM_INIT_INDEX_MAX	1

#undef NAME_SERVER_SLOT
#undef ENVIRONMENT_SLOT
#undef SERVICE_SLOT

#define OLD_NAME_SERVER_SLOT	0
#define NETNAME_SLOT		0
#define SM_INIT_SLOT		1
#define AUTH_PRIVATE_SLOT	1
#define OLD_ENV_SERVER_SLOT	1
#define AUTH_SERVER_SLOT	2
#define OLD_SERVICE_SLOT	2
#define KM_SERVICE_SLOT		2
#define NAME_SERVER_SLOT	3
#define NAME_PRIVATE_SLOT	3
#define SM_INIT_SLOTS_USED	4

#if (SM_INIT_SLOTS_USED > TASK_PORT_REGISTER_MAX)
#error Things are not going to work!
#endif


#define SM_INIT_FAILURE		11881

#endif	_SM_INIT_DEFS_
