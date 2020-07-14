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
 * ConfigUtils.h - prototypes for config_utils.c.
 *
 * HISTORY
 * 12-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#ifndef	_CONFIG_CONFIGUTILS_H_
#define _CONFIG_CONFIGUTILS_H_

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>

/*
 * Search key specification for get_driver_entry() and get_dev_entry().
 */
typedef enum {
	SK_DRIVER_PORT,
	SK_DEV_PORT,
	SK_SIG_PORT,
	SK_DEVNUM,
	SK_FID,
	SK_DEV_INDEX
} dev_search_key_t;

#define FID_NULL	((file_id_t)0)
#define DEV_NUM_NULL	((IODeviceNumber)0)

driver_entry_t *get_driver_entry(
	dev_search_key_t search_key,
	port_t search_port,
	IODeviceNumber dev_number,
	file_id_t fid,
	IODeviceType dev_type,
	IOSlotId slot_id,
	dev_entry_t **dev_entrypp);		// returned 
dev_entry_t *get_dev_entry(
	dev_search_key_t search_key,
	driver_entry_t *driver_entry,
	port_t search_port,
	IODeviceNumber dev_number,
	IODeviceType device_type,
	IOSlotId slot_id);
driver_entry_t *create_driver_entry(
	file_id_t fid,
	filename_t filename);
IOConfigReturn free_driver_entry(
	driver_entry_t *driver_entry);
dev_entry_t *create_dev_entry(
	driver_entry_t *driver_entry, 
	IODeviceNumber dev_number,
	IODeviceType dev_type, 
	IOSlotId slot_id);
IOConfigReturn free_dev_entry(
	dev_entry_t *dev_entry);
void get_driver_list_lock();
void release_driver_list_lock();

extern IONamedValue dev_returns[];
extern IONamedValue config_returns[];

#endif	_CONFIG_CONFIG_UTILS_H_
