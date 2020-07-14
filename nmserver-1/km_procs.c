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

#include <mach/mach.h>

#ifndef WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "crypt.h"
#include "debug.h"
#include "key_defs.h"
#include "keyman.h"
#include "km_defs.h"
#include "lock_queue.h"
#include "lock_queue_macros.h"
#include "mem.h"
#include "netmsg.h"
#include <servers/nm_defs.h>
#include "nm_extra.h"
#include "port_defs.h"
#include "portrec.h"
#include "timer.h"
#include "nn_defs.h"	/* MEM_NNREC */


PRIVATE struct lock_queue	km_queue;

/*
 * km_procs_init
 *	Initialises the km_queue.
 *
 */
PUBLIC void km_procs_init()
{

    lq_init(&km_queue);
    RET;

}


void _km_dummy() { }



EXPORT void km_do_key_exchange(int client_id, int lient_retry, netaddr_t host_id)
{

    RET;

}

