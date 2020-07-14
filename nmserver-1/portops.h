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

#ifndef _PORTOPS_
#define _PORTOPS_

/*
 * Sizes used by po_translate_[ln]port_rights.
 */
#define PO_MAX_NPD_ENTRY_SIZE	48
#define PO_MIN_NPD_ENTRY_SIZE	28
#define PO_NPD_SEG_SIZE		256

/*
 * Completion codes passed to po_port_rights_commit.
 */
#define PO_RIGHTS_XFER_SUCCESS	0
#define PO_RIGHTS_XFER_FAILURE	1


/*
 * Functions exported by the port operations module.
 */
#include <mach/boolean.h>

extern boolean_t po_init();

extern boolean_t po_check_ipc_seq_no();
/*
port_rec_ptr_t	portrec_ptr;
netaddr_t	host_id;
long		ipc_seq_no;
*/

extern long po_create_token();
/*
port_rec_ptr_t		port_rec_ptr;
secure_info_ptr_t	token_ptr;
*/

extern void po_notify_port_death();
/*
port_rec_ptr_t	port_rec_ptr;
*/

extern void po_port_deallocate();
/*
port_t			lport;
*/

extern int po_translate_lport_rights();
/*
int		client_id;
port_t		lport;
int		right;
int		security_level;
netaddr_t	destination_hint;
pointer_t	port_data;	To be sent to the remote network server.
*/

extern void po_port_rights_commit();
/*
int		client_id;
int		completion_code;
netaddr_t	destination;
*/

extern int po_translate_nport_rights();
/*
netaddr_t	source;
pointer_t	port_data;	Received from the remote network server.
int		security_level;
port_t		*lport;
int		*right;
*/

#endif _PORTOPS_
