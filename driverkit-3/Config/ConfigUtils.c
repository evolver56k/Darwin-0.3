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
 * ConfigUtils.c - common utility functions for Config server.
 *
 * HISTORY
 * 12-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <mach/mach_types.h>
#import <driverkit/userConfigServer.h>
#import "ConfigPrivate.h"
#import "ConfigUtils.h"
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <kernserv/queue.h>
#import <driverkit/generalFuncs.h>

IONamedValue search_keys[] = {
	{SK_DRIVER_PORT,	"Driver Port"		},
	{SK_DEV_PORT,		"Device Port"		},
	{SK_SIG_PORT,		"Driver Sig Port"	},
	{SK_DEVNUM,		"dev_number"		},	
	{SK_FID,		"FID"			},	
	{SK_DEV_INDEX,		"deviceIndex"		},	
	{0,			NULL			},
};

IONamedValue dev_returns[] = {
	{IO_R_SUCCESS,		"Success"				},
	{IO_R_PRIVILEGE,	"port/device access denied"		},
	{IO_R_NO_CHANNELS,	"no DMA channels available"		},
	{IO_R_NO_SPACE,		"no address space available for mapping"},
	{IO_R_NO_DEVICE,	"no more dev_numbers"			},
	{0,			NULL					},
};

IONamedValue config_returns[] = {
	{IO_CNF_SUCCESS,	"Success"				},
	{IO_CNF_NOT_REGISTERED,	"port not registered"			},
	{IO_CNF_NOT_FOUND,	"slot_id/dev_type not found"		},
	{IO_CNF_BUSY,		"Config Busy"				},
	{IO_CNF_REGISTERED,	"driver_port already registered"	},
	{IO_CNF_RESOURCE,	"System Resource Shortage"		},
	{IO_CNF_ACCESS,		"Device Access Denied"			},
	{0,			NULL					},
};

/*
 * Get a driver_entry_t (and possibly a dev_entry_t) from driver_list 
 * matching the specified parameter. *dev_entry is only returned if the
 * specified search_key necessitates searching through a driver_entry's
 * dev_list.
 * Returns NULL if specified driver_entry not found.
 * driver_list_lock must be held on entry.
 */
driver_entry_t *get_driver_entry(
	dev_search_key_t search_key,
	port_t search_port,
	IODeviceNumber dev_number,
	file_id_t fid,
	IODeviceType device_type,	// only deviceIndex is used
	IOSlotId slot_id,
	dev_entry_t **dev_entry_pp)
{
	driver_entry_t *driver_entry;
	boolean_t found = FALSE;
	dev_entry_t *dev_entry;
	
#if	DDM_DEBUG
	xpr_common("get_driver_port: search key %s\n", 
		IOFindNameForValue(search_key, search_keys), 2,3,4,5);
#endif	DDM_DEBUG
	driver_entry = (driver_entry_t *)queue_first(&driver_list);
	while(!queue_end(&driver_list, (queue_t)driver_entry)) {
		switch(search_key) {
		    case SK_DRIVER_PORT:
			if(driver_entry->driver_port == search_port) 
		    		found = TRUE;
			break;
		    case SK_DEV_PORT:
		    case SK_DEVNUM:
		    case SK_DEV_INDEX:
			/*
			 * Search this driver's dev_list for this dev_port
			 * or dev_number.
			 */
			dev_entry = get_dev_entry(
				search_key,
				driver_entry,
				search_port,
				dev_number,
				device_type,
				slot_id);
			if(dev_entry != NULL) {
				found = TRUE;
				if(dev_entry) {
					*dev_entry_pp = dev_entry;
				}
			}
			break;
			
		    case SK_SIG_PORT:
			if(driver_entry->driver_sig_port == search_port)
				found = TRUE;
			break;
			
		    case SK_FID:
			if(driver_entry->executable_fid == fid)
				found = TRUE;
			break;
			
		    default:
		    	printf("BOGUS SEARCH KEY IN get_driver_entry()\n");
			driver_entry = NULL;
			found = TRUE;
			break;
		}
		if(found)
			break;
		/*
		 * Try next entry.
		 */
		driver_entry = (driver_entry_t *)driver_entry->link.next;
	}
	if(found) {
		xpr_common("get_driver_port: found driver_entry 0x%x\n", 
			driver_entry, 2,3,4,5);
		if(driver_entry->filename) {
			xpr_common("    filename = %s\n", 
				IOCopyString(driver_entry->filename), 2,3,4,5);
		}
	}
	else {
		driver_entry = NULL;
		xpr_common("get_driver_port: driver_entry not found\n", 
			1,2,3,4,5);
	}
	return(driver_entry);
}

/*
 * Get a dev_entry_t from specified driver_entry's dev_list matching the
 * specified parameter.
 * Returns NULL if dev_entry not found.
 * driver_list_lock must be held on entry.
 */
dev_entry_t *get_dev_entry(
	dev_search_key_t search_key,
	driver_entry_t *driver_entry,
	port_t search_port,
	IODeviceNumber dev_number,
	IODeviceType dev_type,
	IOSlotId slot_id)
{
	dev_entry_t *dev_entry;
	boolean_t found = FALSE;
	
	dev_entry = (dev_entry_t *)queue_first(&driver_entry->dev_list);
	while(!queue_end(&driver_entry->dev_list, (queue_t)dev_entry)) {
		switch(search_key) {
		    case SK_DEV_PORT:
			if(dev_entry->dev_port == search_port) 
				found = TRUE;
			break;
		    case SK_DEVNUM:
			if(dev_entry->dev_number == dev_number)
				found = TRUE;
			break;
		    case SK_DEV_INDEX:
			if((dev_entry->dev_type == dev_type) &&
			   (dev_entry->slot_id == slot_id)) {
				found = TRUE;	
			}
			break;
		    default:
			printf("get_dev_entry: BOGUS SEARCH_KEY\n");
			dev_entry = NULL;
			found = TRUE;
			break;
		}
		if(found)
			break;
			
		/*
		 * Try next dev_entry.
		 */
		dev_entry = (dev_entry_t *)dev_entry->link.next;
	}
	if(found)
		return(dev_entry);
	else
		return(NULL);
}

/*
 * Create a new driver_entry_t, add it to driver_list. Caller must hold
 * driver_list_lock. No ports are allocated here (that's done in 
 * exec_driver().
 */
driver_entry_t *create_driver_entry(
	file_id_t fid,
	filename_t filename)
{
	driver_entry_t *driver_entry;
	
	xpr_common("create_driver_entry: filename %s\n", 
		IOCopyString(filename), 2,3,4,5);
	driver_entry = malloc(sizeof(*driver_entry));
	queue_init(&driver_entry->dev_list);
	driver_entry->executable_fid = fid;
	strncpy(driver_entry->filename, filename, FILENAME_SIZE);
	driver_entry->driver_port = PORT_NULL;
	driver_entry->driver_sig_port = PORT_NULL;
	driver_entry->boot_requestor = PORT_NULL;
	driver_entry->running = FALSE;
	queue_enter(&driver_list,
		driver_entry,
		driver_entry_t *,
		link);
	return(driver_entry);
}

/*
 * Release all state associated with specified driver_entry_t. Inform kernel 
 * that all associated dev_ports are to be destroyed.
 *
 * This is used by driver_delete() and upon port death notification of a
 * driver_port.
 *
 * driver_list_lock must be held on entry.
 */
IOConfigReturn free_driver_entry(
	driver_entry_t *driver_entry)
{
	dev_entry_t *dev_entry, *dev_entry_next;
	IOConfigReturn crtn = IO_CNF_SUCCESS;
	
	xpr_common("free_driver_entry driver_entry 0x%x\n", 
		driver_entry, 2,3,4,5);
	xpr_common("  filename %s\n", 
		IOCopyString(driver_entry->filename), 2,3,4,5);
	dev_entry = (dev_entry_t *)queue_first(&driver_entry->dev_list);
	
	/*
	 * First delete all of this driver's dev_ports.
	 */
	while(!queue_end(&driver_entry->dev_list, (queue_t)dev_entry)) {
		dev_entry_next = (dev_entry_t *)dev_entry->link.next;
		if(free_dev_entry(dev_entry))
			crtn = IO_CNF_ACCESS;
		dev_entry = dev_entry_next;
	}

	/*
	 * Now free up internal state.
	 */
	if(driver_entry->driver_sig_port != PORT_NULL)
		port_deallocate(task_self(), driver_entry->driver_sig_port);
	if(driver_entry->boot_requestor != PORT_NULL)
		port_deallocate(task_self(), driver_entry->boot_requestor);
	queue_remove(&driver_list,
		driver_entry,
		driver_entry_t *,
		link);
	free(driver_entry);
	return(crtn);
}

/*
 * Create a new dev_entry_t, add it to driver_entry's dev_list. This is the 
 * only place where we ask the kernel to create a dev_port.
 */
dev_entry_t *create_dev_entry(
	driver_entry_t *driver_entry, 
	IODeviceNumber dev_number,
	IODeviceType dev_type, 
	IOSlotId slot_id)
{	
	dev_entry_t *dev_entry;
	IOReturn drtn;
	
	xpr_common("create_dev_entry: dev_number %d dev_type 0x%x\n",
		dev_number, dev_type, 3,4,5);
	xpr_common("   slot_id 0x%x filename %s\n", 
		slot_id, IOCopyString(driver_entry->filename), 3,4,5);
	dev_entry = malloc(sizeof(*dev_entry));
	drtn = _IOCreateDevicePort(device_master_port,
		task_self(),
		dev_number,
		&dev_entry->dev_port);
	if(drtn) {
		IOLog("Config: _IOCreateDevicePort() returned %s\n",
			IOFindNameForValue(drtn, dev_returns), 2,3,4,5);
		free(dev_entry);
		return(NULL);
	}
	dev_entry->dev_type = dev_type;
	dev_entry->slot_id = slot_id;
	dev_entry->dev_number = dev_number;
	dev_entry->driver_entry = driver_entry;
	queue_enter(&driver_entry->dev_list,
		dev_entry,
		dev_entry_t *,
		link);
	return(dev_entry);
}


/*
 * Destroy state associated with specified dev_entry.
 */
IOConfigReturn free_dev_entry(
	dev_entry_t *dev_entry)
{
	IOReturn drtn;
	IOConfigReturn crtn = IO_CNF_SUCCESS;
	
	xpr_common("deleting dev_entry 0x%x\n", dev_entry, 2,3,4,5);
	xpr_common("   dev_type 0x%x  slot_id 0x%x\n", 
		dev_entry->dev_type, dev_entry->slot_id, 3,4,5);
	if(dev_entry->dev_port != PORT_NULL) {
		drtn = _IODestroyDevicePort(device_master_port,
			dev_entry->dev_port);
		if(drtn) {
			xpr_common("free_dev_entry: _IODestroyDevicePort() "
				"returned %s\n", 
				IOFindNameForValue(drtn, dev_returns),
				2,3,4,5);
			crtn = IO_CNF_ACCESS;
		}
	}
		
	/*
	 * Delete the dev_entry_t itself.
	 */
	queue_remove(&dev_entry->driver_entry->dev_list,
		dev_entry,
		dev_entry_t *,
		link);
	free(dev_entry);
	return(crtn);
}

/*
 * Get/release driver_list_lock as a sleep lock.
 */
void get_driver_list_lock()
{
	mutex_lock(driver_list_lock);
	while(driver_list_locked)
		condition_wait(driver_list_cond, driver_list_lock);
	driver_list_locked = TRUE;
	mutex_unlock(driver_list_lock);
}

void release_driver_list_lock()
{
	mutex_lock(driver_list_lock);
	driver_list_locked = FALSE;
	condition_signal(driver_list_cond);
	mutex_unlock(driver_list_lock);
}

