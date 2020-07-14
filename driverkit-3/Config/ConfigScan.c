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
 * ConfigScan.c - routines to scan physical devices and start up drivers.
 *
 * HISTORY
 * 14-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <mach/mach_types.h>
#import <driverkit/userConfigServer.h>
#import "ConfigPrivate.h"
#import "ConfigUtils.h"
#import "ConfigScan.h"
#import "ConfigUfs.h"
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <kernserv/queue.h>
#import <bsd/sys/dir.h>
#import <servers/bootstrap.h>
#import <driverkit/generalFuncs.h>
#import <mach/mig_errors.h>
/*
 * Main device scan function.
 * 
 * Search thru all hardware devices. Map devices onto driver_entry's and 
 * dev_entry's. Launch all drivers. 
 *
 * Also used to scan for a driver of one particular deviceType and slotId
 * if scan_type is SCAN_ONE.
 */
IOConfigReturn config_scan(
	scan_t scan_type,
	IOSlotId slot_id_arg,
	IODeviceType dev_type_arg)
{
	IOReturn drtn;
	IODeviceNumber dev_number;
	IOSlotId slot_id;
	IODeviceType dev_type;
	dev_entry_t *dev_entry;
	driver_entry_t *driver_entry;
	filename_t filename;
	file_id_t fid;
	IOConfigReturn crtn;
	BOOL in_use;
	
	xpr_common("config_scan\n", 1,2,3,4,5);

	if(scan_type == SCAN_ONE) {
		crtn = IO_CNF_NOT_FOUND;	// until we find it
	}
	else {
		crtn = IO_CNF_SUCCESS;		// can't fail 
	}
	
	/*
	 * First create driver_list. We scan all of the devices before 
	 * exec'ing any drivers to ensure that a driver has access to all of 
	 * its appropriate dev_ports when it is exec'd.
	 *
	 * Note that we hold driver_list_lock through this whole block; 
	 * if this is the initial config loop, no drivers will have been
	 * started yet.
	 */
	dev_number = 0;
	get_driver_list_lock();
	do {
		drtn = _IOLookupByDeviceNumber(device_master_port,
			dev_number,
			&slot_id,
			&dev_type,
			&in_use);
		if(drtn) {
			xpr_common("config_scan: _IOLookupByDeviceNumber "
				"returned "
				"%s\n", IOFindNameForValue(drtn, dev_returns), 
				2,3,4,5);
			if(drtn == MIG_BAD_ID) {
				printf("_IOLookupByObjectNumber() not "
					"supported on "
					"this machine; Config aborting\n");
				exit(1);
			}
			goto next_dev_num;
		}
		if(in_use)
			goto next_dev_num;
		if(dev_type == IO_NULL_DEVICE_TYPE)
			goto next_dev_num;
		if(scan_type == SCAN_ONE) {
			if((dev_type != dev_type_arg) || 
			   (slot_id != slot_id_arg))
			   	goto next_dev_num;
		}
		
		/*
		 * Looks like a valid device. Do we already have a driver
		 * mapping for it?
		 */
		driver_entry = get_driver_entry(SK_DEVNUM,
			PORT_NULL,
			dev_number,
			FID_NULL,
			IO_NULL_DEVICE_TYPE,
			IO_NULL_SLOT_ID,
			&dev_entry);
		if(driver_entry) {
			xpr_common("config_scan: dev_num %d already mapped\n",
				dev_number, 2,3,4,5);
			goto next_dev_num;
		}
		else {
			xpr_common("config_scan: NEW device\n", 1,2,3,4,5);
			xpr_common("  dev_num %d  dev_type 0x%x  slot_id "
				"0x%x\n", dev_number, dev_type, slot_id, 4,5);
		}
		
		/*
		 * Get an executable for this device.
		 */
		if(!get_driver_file(dev_type, slot_id, &filename, &fid)) {
			goto next_dev_num;
		}
		
		/*
		 * We have a driver for this device. See if this driver is
		 * an alias (link) to a driver we already know about.
		 */
		driver_entry = get_driver_entry(SK_FID,
			PORT_NULL,
			DEV_NUM_NULL,
			fid,
			IO_NULL_DEVICE_TYPE,
			IO_NULL_SLOT_ID,
			&dev_entry);
		if(driver_entry == NULL) {
			
			/*
			 * create a new driver_entry.
			 */
			driver_entry = create_driver_entry(fid, filename);
			if(driver_entry == NULL) {
				/* huh? */
				goto next_dev_num;
			}
		}
		
		/*
		 * Add this device to the driver. We're either adding a new
		 * device to an existing driver or adding the first 
		 * device.
		 *
		 * Note that an existing dev_entry in the driver_entry 
		 * matching this slot_id and dev_type is OK (we don't 
		 * check for it); this would be a "multiple devices of 
		 * same type" configuration. We need a port for each one.
		 */
		dev_entry = create_dev_entry(driver_entry,
			dev_number,
			dev_type,
			slot_id);

next_dev_num:
		dev_number++;
	} while(drtn != IO_R_NO_DEVICE);
	release_driver_list_lock();
	
	/*
	 * exec new drivers. We have to periodically give server_loop() a 
	 * crack at executing driver_register() and driver_delete() calls.
	 * Unfortunately, a driver_delete() can cause us great hassle unless
	 * we're really careful and not hold on to any driver_entry's while
	 * we're not holding the lock. So we scan from the start of 
	 * driver_list each time thru the loop, looking for the first 
	 * non-running driver. 
	 *
	 * If this is a SCAN_ONE operation, this should go really quickly...
	 * I don't think there is any way any more than one driver could 
	 * be in a "not running" state.
	 */
	do {
		get_driver_list_lock();
		driver_entry = (driver_entry_t *)queue_first(&driver_list); 
		while(!queue_end(&driver_list, (queue_t)driver_entry)) {
			if(!driver_entry->running) {
				if(exec_driver(driver_entry) == IO_CNF_SUCCESS)
					crtn = IO_CNF_SUCCESS;
				else {
					/*
					 * Couldn't exec this driver. Remove 
					 * from	driver_list.
					 */
					free_driver_entry(driver_entry);
				}
				break;
			}
			driver_entry = 
				(driver_entry_t *)driver_entry->link.next;
		}
		
		/*
		 * Either got to the end of the queue or started a 
		 * non-running driver. End of queue means we're done (we 
		 * started up all drivers). Otherwise, go back to the
		 * head of the queue.
		 */
		if(queue_end(&driver_list, (queue_t)driver_entry))
			break;
		release_driver_list_lock();
	} while(!queue_end(&driver_list, (queue_t)driver_entry));
	release_driver_list_lock();
	xpr_common("config_scan returning %s\n", 
		IOFindNameForValue(crtn, config_returns), 2,3,4,5);
	return(crtn);
}

/*
 * Start up or restart a driver. Place all necessary ports in the driver's 
 * bootstrap namespace. If driver_entry->running is TRUE, this is 
 * a restart; we have to clean up the (now useless) bootstrap port which 
 * the deceased driver used.
 */
IOConfigReturn exec_driver(
	driver_entry_t *driver_entry)
{
	kern_return_t krtn;
	port_t old_bootstrap_port;
	port_t subset_port;
	dev_entry_t *dev_entry; 
	filename_t portname;
	filename_t exec_name;
	int pid;
	
	xpr_common("exec_driver: filename %s\n", 
		IOCopyString(driver_entry->filename), 2,3,4,5);
	sprintf(exec_name, "%s/%s", DRIVER_PATH, driver_entry->filename);
	if(driver_entry->running) {
		/*
		 * Kill off dead driver's bootstrap and signature ports.
		 */
		xpr_common(" ...deleting requestor and sig ports\n", 
			1,2,3,4,5);
		krtn = port_deallocate(task_self(), 
			driver_entry->boot_requestor);
		if(krtn) {
			xpr_err("exec_driver: port_deallocate(): %s\n", 
			 	mach_error_string(krtn), 2,3,4,5);
		}
		krtn = port_deallocate(task_self(), 
			driver_entry->driver_sig_port);
		if(krtn) {
			xpr_err("exec_driver: port_deallocate(): %s\n",
				mach_error_string(krtn), 2,3,4,5);
		}
		driver_entry->running = FALSE;
	}
	
	/*
	 * Set up a signature port for authentication.
	 */
	krtn = port_allocate(task_self(), &driver_entry->driver_sig_port);
	if(krtn) {
		xpr_err("exec_driver: port_allocate(): %s\n",
			mach_error_string(krtn), 2,3,4,5);
		return(IO_CNF_RESOURCE);
	}
	 
	/*
	 * Set up a bootstrap subset port for the driver task. Its lifetime
	 * will be the lifetime of driver_entry->boot_requestor.
	 */
	krtn = port_allocate(task_self(), &driver_entry->boot_requestor);
	if(krtn) {
		xpr_err("exec_driver: port_allocate(): %s\n",
			mach_error_string(krtn), 2,3,4,5);
		return(IO_CNF_RESOURCE);
	}
	krtn = bootstrap_subset(bootstrap_port,
		driver_entry->boot_requestor,		/* requestor */
		&subset_port);
	if(krtn) {
		xpr_err("exec_driver: bootstrap_subset() returned %d\n",
			krtn, 2,3,4,5);
		return(IO_CNF_RESOURCE);
	}
	
	/*
	 * Advertise the following on the bootstrap subset port:
	 * -- dev_port for each of this driver's dev_entry's
	 * -- driver_sig_port
	 */
	xpr_common("exec_driver: adding port driver_sig_port\n", 1,2,3,4,5);
	krtn = bootstrap_register(subset_port,
		SIG_PORT_NAME,
		driver_entry->driver_sig_port);
	if(krtn) {
		xpr_err("exec_driver: bootstrap_register(): %s\n",
			mach_error_string(krtn), 2,3,4,5);
		return(IO_CNF_RESOURCE);
	}
	dev_entry = (dev_entry_t *)queue_first(&driver_entry->dev_list);
	while(!queue_end(&driver_entry->dev_list, (queue_t)dev_entry)) {
		sprintf(portname, "dev_port_%u", dev_entry->dev_number);
		xpr_common("exec_driver: adding port %s\n", 
			IOCopyString(portname), 2,3,4,5);
		krtn = bootstrap_register(subset_port,
			portname,
			dev_entry->dev_port); 
		if(krtn) {
			printf("OOPS\n");
			xpr_err("exec_driver: bootstrap_register(%s): %s\n",
				IOCopyString(portname), 
				mach_error_string(krtn),
				3,4,5);
			xpr_err("   filename %s dev_number %d dev_port %d\n",
				IOCopyString(driver_entry->filename),
				dev_entry->dev_number, dev_entry->dev_port,
				4,5);
#ifdef	DEBUG 
			log_boot_servers(subset_port);
#endif	DEBUG
			return(IO_CNF_RESOURCE);
		}
		dev_entry = (dev_entry_t *)dev_entry->link.next;
	}
	
	/*
	 * OK, set out bootstrap port to the subset port, and fork off the
	 * driver. Then restore our task's bootstrap_port. Note 
	 * "bootstrap_port" is a crufty mach global.
	 */
	old_bootstrap_port = bootstrap_port;
	krtn = task_set_bootstrap_port(task_self(), subset_port);
	if(krtn) {
		xpr_err("exec_driver: task_set_bootstrap_port(): %s\n",
			mach_error_string(krtn), 2,3,4,5);
		return(IO_CNF_RESOURCE);
	}
	bootstrap_port = subset_port;
	driver_entry->running = TRUE;
	if((pid = fork()) == 0) {
		printf("...forking off %s\n", exec_name);
		execl(exec_name, exec_name, NULL);
		/*
		 * FIXME: what do we do here? Should get rid of driver_entry,
		 * but we've already forked! Currently we have no way to
		 * do this without looking up our device ports with
		 * the bootstrap server, registering via IORegisterDriver(),
		 * then calling IODeleteDriver(). What a pain. Let's skip
		 * it for now; developers can use configutil -t to clean
		 * up.
		 */
		perror("execl");
		xpr_err("Config: execl(%s) FAILED\n", 
			IOCopyString(exec_name), 2,3,4,5);
		exit(1);
	}

	/* 
	 * Back to our old self.
	 */
	bootstrap_port = old_bootstrap_port;
	krtn = task_set_bootstrap_port(task_self(), old_bootstrap_port);
	if(krtn) {
		xpr_err("exec_driver: task_set_bootstrap_port(): %s\n",
			mach_error_string(krtn), 2,3,4,5);
	}
	return(IO_CNF_SUCCESS);
}

#ifdef	DEBUG
void log_boot_servers(port_t boot_port)
{
	    u_int i;
	    name_array_t service_names;
	    unsigned int service_cnt;
	    name_array_t server_names;
	    unsigned int server_cnt;
	    bool_array_t service_active;
	    unsigned int service_active_cnt;
	    kern_return_t krtn;
	    
	    krtn = bootstrap_info(boot_port, 
		    &service_names, 
		    &service_cnt,
		    &server_names, 
		    &server_cnt, 
		    &service_active, 
		    &service_active_cnt);
	    if (krtn != BOOTSTRAP_SUCCESS)
		    printf("ERROR:  info failed: %d", krtn);
	    else {
	    	printf("log_boot_server: service_cnt = %d\n", service_cnt);
		for (i = 0; i < service_cnt; i++)
		    printf("Name: %-15s   Server: %-15s    "
			    "Active: %-4s",
			service_names[i],
			server_names[i][0] == '\0' ? 
			    "Unknown" : server_names[i],
			service_active[i] ? "Yes\n" : "No\n");
	    }
}
#endif	DEBUG

/*
 * Extract revision from a filename_t. Returns 0 if OK, -1 on error (i.e.,
 * illegal filename format). This is (and should remain) file system
 * independent - internally, we use filename_t's exclusively.
 */
int get_driver_rev(
	filename_t filename,
	unsigned short *revision)
{
	char rev_string[9];
	unsigned int rev;
	int bar[3];		/* position of underbars */
	int digit;
	int bar_num = 0;
	int rev_length;
	
	/*
	 * We're pretty strict about the format....
	 * 
	 * devr_<DEV_INDEX>_<REVISION>_<human_readable_name>
	 *   ...or...
	 * devs_<SLOT_ID>_<REVISION>_<human_readable_name>
	 *
	 * This routine returns <REVISION> as a binary number (assuming that
	 * in the filename it's in ASCII hex).
	 */
	bar[0] = bar[1] = bar[2] = -1;
	for(digit=0; digit<FILENAME_SIZE; digit++) {
		if(filename[digit] == '_') {
			bar[bar_num++] = digit;
			if(bar_num == 3)
				break;
		}
		if(filename[digit] == '\0')
			break;
	}
	
	/*
	 * Verify proper filename format.
	 */
	if(bar_num != 3)
		return(-1);
	rev_length = bar[2] - bar[1] - 1;
	if((rev_length > 8) || (rev_length == 0))
		return(-1);
	strncpy(rev_string, &filename[bar[1]+1], rev_length);
	rev_string[rev_length] = '\0';
	rev = atoh(rev_string);
	xpr_common("get_driver_rev(%s) returning 0x%x\n",
		IOCopyString(filename), rev, 3,4,5);
	*revision = rev;
	return(0);	
}
