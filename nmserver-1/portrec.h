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

#ifndef	_PORTREC_
#define	_PORT_REC_

#include <mach/boolean.h>

#include "lock_queue.h"
#include "lock_queue_macros.h"
#include "port_defs.h"

/*
 * The null network port.
 */
extern network_port_t		null_network_port;


/*
 * Functions exported.
 */

extern boolean_t pr_init();
/*
*/

extern void pr_destroy();
/*
port_rec_ptr_t		port_rec_ptr;
*/

extern port_rec_ptr_t pr_np_puid_lookup();
/*
np_uid_t		np_puid;
*/

extern port_rec_ptr_t pr_nportlookup();
/*
network_port_ptr_t	nport_ptr;
*/

extern port_rec_ptr_t pr_ntran();
/*
network_port_ptr_t	nport_ptr;
*/

extern port_rec_ptr_t pr_lportlookup();
/*
port_t			lport;
*/

extern port_rec_ptr_t pr_ltran();
/*
port_t			lport;
*/

extern boolean_t pr_nport_equal();
/*
network_port_ptr_t	nport_ptr_1, nport_ptr_2;
*/

extern void pr_nporttostring();
/*
char			*nport_str;
network_port_ptr_t	nport_ptr;
*/

extern lock_queue_t pr_list();
/*
*/


#endif	_PORT_REC_
