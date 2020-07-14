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
/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * ConfigScan.h - interface to ConfigScan.c.
 *
 * HISTORY
 * 14-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#ifndef	_CONFIG_CONFIGSCAN_H_
#define _CONFIG_CONFIGSCAN_H_

typedef enum {SCAN_ONE, SCAN_ALL} scan_t;

IOConfigReturn config_scan(
	scan_t scan_type,
	IOSlotId slot_id,
	IODeviceType dev_type);
IOConfigReturn exec_driver(
	driver_entry_t *driver_entry);
#ifdef	DEBUG
void log_boot_servers(port_t boot_port);
#endif	DEBUG
int get_driver_rev(
	filename_t filename,
	unsigned short *revision);


#endif	_CONFIG_CONFIGSCAN_H_
