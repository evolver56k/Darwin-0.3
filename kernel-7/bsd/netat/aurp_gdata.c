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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	File: gdata.c
 */
#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <rtmp.h>
#include <routing_tables.h>
#include <at_aurp.h>

short RT_maxentry;
short ZT_maxentry;
#ifdef _AIX
RT_entry *RT_table;
ZT_entry *ZT_table;
#endif
void *RT_lock;
RT_entry *(*RT_insert)();
RT_entry *(*RT_delete)();
RT_entry *(*RT_lookup)();
void (*ZT_set_zmap)();
int  (*ZT_add_zname)();
int  (*ZT_get_zindex)();
void (*ZT_remove_zones)();

extern dbgBits_t dbgBits;
atlock_t aurpgen_lock;
gref_t *aurp_gref;
unsigned char dst_addr_cnt;
unsigned char net_access_cnt;
unsigned char net_export;
unsigned short rcv_connection_id;
int net_port;
void *update_tmo;
aurp_state_t aurp_state[256];
unsigned short net_access[AURP_MaxNetAccess];
