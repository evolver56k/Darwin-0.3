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

#ifndef	_KM_DEFS_
#define	_KM_DEFS_

#include "key_defs.h"
#include <servers/nm_defs.h>
#include "timer.h"

typedef struct {
    netaddr_t	kr_host_id;
    nmkey_t	kr_key;
    nmkey_t	kr_mpkey;
    nmkey_t	kr_mpikey;
} key_rec_t, *key_rec_ptr_t;

#define KEY_REC_NULL	((key_rec_ptr_t)0)

/*
 * Definition of queue holding pending key exchange requests.
 */
typedef struct kmq_entry {
    struct kmq_entry	*next;
    struct timer	kmq_timer;
    netaddr_t		kmq_host_id;
    int			kmq_client_id;
    int			(*kmq_client_retry)();
} kmq_entry_t, *kmq_entry_ptr_t;


#define KM_RETRY_INTERVAL	60


/*
 * External definitions for functions implemented by km_utils.c
 */

extern key_rec_ptr_t km_host_enter();
/*
netaddr_t	host_id;
*/

extern key_rec_ptr_t km_host_lookup();
/*
netaddr_t	host_id;
*/

extern void km_utils_init();
/*
*/

extern void km_procs_init();
/*
*/

#endif	_KM_DEFS_
