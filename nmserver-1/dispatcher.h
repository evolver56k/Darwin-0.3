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

#ifndef	_DISPATCHER_
#define	_DISPATCHER_

#include "disp_hdr.h"

/*
 * Dispatcher switch.
 */
typedef struct {
    int		(*disp_indata)();
    int		(*disp_inprobe)();
    int		(*disp_indata_simple)();
    int		(*disp_rr_simple)();
    int		(*disp_in_request)();
} dispatcher_switch_t;

extern dispatcher_switch_t	dispatcher_switch[DISP_TYPE_MAX];


/*
 * Functions exported by the dispatcher module.
 */

extern boolean_t disp_init();
/*
*/

extern int disp_indata();
/*
int		trid;
sbuf_ptr_t	data;
netaddr_t	from;
int		(*tr_cleanup)();
int		trmod;
int		client_id;
int		crypt_level;
boolean_t	broadcast;
*/

extern int disp_inprobe();
/*
int		trid;
sbuf_ptr_t	pkt;
netaddr_t	from;
int		(*(*cancel))();
int		trmod;
int		*client_id;
int		crypt_level;
boolean_t	broadcast;
*/

extern int disp_indata_simple();
/*
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
*/

extern int disp_rr_simple();
/*
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
*/

extern int disp_in_request();
/*
int		trmod;
int		trid;
sbuf_ptr_t	data_ptr;
netaddr_t	from;
int		crypt_level;
boolean_t	broadcast;
*/

#endif	_DISPATCHER_
