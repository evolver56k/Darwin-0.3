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
 * Config.c - Config server.
 *
 * HISTORY
 * 11-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <mach/mach_types.h>
#import <driverkit/userConfigServer.h>
#import <driverkit/generalFuncs.h>
#import "ConfigPrivate.h"
#import "ConfigUtils.h"
#import "ConfigScan.h"
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <servers/netname.h>
#import <mach/message.h>
#import <mach/mach_error.h>
#import <mach/cthreads.h>
#import <kernserv/queue.h>
#import <mach/notify.h>
#import <syslog.h>
#ifdef	DEBUG
#import "ConfigUfs.h"
#endif	DEBUG

/*
 * prototypes for private functions.
 */
static void usage(char **argv);
static void server_loop();
boolean_t Config_server(msg_header_t *inp, msg_header_t *outp);
static void config_port_death(msg_header_t *inp);

/*
 * Static variables.
 */
static port_t config_port;		// we advertise this for all takers
static port_set_name_t config_port_set;	// the set on which we listen 
port_t device_master_port;		// used to communicate with kernel
port_t notify_port;			// that's ours
static boolean_t config_in_progress;	// prevents hazardous operations
					//   during initial config
queue_head_t driver_list;		// list of driver_entry_t's
mutex_t driver_list_lock;		// protects driver_list during initial
					//   config
condition_t driver_list_cond;
boolean_t driver_list_locked;

extern port_t device_master_self();

/*
 * User-specified variables.
 */
int verbose = 0;

void main(int argc, char **argv)
{
	int arg;
	kern_return_t krtn;
	cthread_t server_thread;
	
	if(argc < 1)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'v':
		    	verbose++;
			break;
		    default:
		    	usage(argv);
		}
	}
	
#ifdef	DDM_DEBUG
	IOInitDDM(200, "ConfigXpr");
	IOSetDDMMask(XPR_CONFIG_INDEX,
		XPR_CONFIG | XPR_SERVER | XPR_COMMON | XPR_ERR);
#endif	DDM_DEBUG

	/*
	 * Initialize global state.
	 */
	queue_init(&driver_list);
	driver_list_lock = mutex_alloc();
	driver_list_cond = condition_alloc();
	config_in_progress = TRUE;
	
	/*
	 * Set up various ports and start the server thread before initial 
	 * autoconfig so device drivers have a place to send messages.
	 *
	 * config_port is how we advertise our exported services to all.
	 */
	krtn = port_allocate(task_self(), &config_port);
	if(krtn) {
		mach_error("port_allocate", krtn);
		exit(1);
	}
	krtn = port_set_allocate(task_self(), &config_port_set);
	if(krtn) {
		mach_error("port_set_allocate", krtn);
		exit(1);
	}
	krtn = port_set_add(task_self(), config_port_set, config_port);
	if(krtn) {
		mach_error("port_set_add", krtn);
		exit(1);
	}
	
	/*
	 * we get port death notification on notify_port.
	 */
	krtn = port_allocate(task_self(), &notify_port);
	if(krtn) {
		mach_error("port_allocate", krtn);
		exit(1);
	}
	krtn = task_set_notify_port(task_self(), notify_port);
	if(krtn) {
		mach_error("task_set_notify_port", krtn);
		exit(1);
	}
	krtn = port_set_add(task_self(), config_port_set, notify_port);
	if(krtn) {
		mach_error("port_set_add", krtn);
		exit(1);
	}
	krtn = netname_check_in(name_server_port,
		CONFIG_SERVER_NAME,
		task_self(),
		config_port);
	if(krtn) {
		mach_error("netname_check_in", krtn);
		exit(1);
	}
	device_master_port = device_master_self();
	if(device_master_port == PORT_NULL) {
		syslog(LOG_ERR, "Config: Can't get device_master_port\n");
		exit(1);
	}

	server_thread = cthread_fork((cthread_fn_t)server_loop, (any_t)0);	
	
	/*
	 * Do initial autoconfig.
	 */
	config_scan(SCAN_ALL, IO_NULL_SLOT_ID, IO_NULL_DEVICE_TYPE);
	
	/*
	 * We're done; let the server_thread take it from here.
	 */
	config_in_progress = FALSE;
	cthread_exit(0);
}

static void usage(char **argv)
{
	printf("usage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\tv  verbose mode\n");
	exit(1);
}

	
/*
 * Main server loop. This thread starts running before autoconfiguration 
 * takes place to allow servicing of IORegisterDriver() calls during 
 * autoconfig. The config_in_progress flag prevents us from servicing any 
 * hazardous calls during this time - like IORescanDriver(). Such calls will
 * return a IO_CNF_BUSY status.
 */					
static void server_loop()
{
	msg_header_t *inp, *outp;
	kern_return_t krtn;
	boolean_t return_msg;
	
	inp = (msg_header_t *)malloc(MSG_SIZE_MAX);
	outp = (msg_header_t *)malloc(MSG_SIZE_MAX);
	while (1) {
		inp->msg_local_port = config_port_set;
		inp->msg_size = MSG_SIZE_MAX;
		krtn = msg_receive(inp, MSG_OPTION_NONE, 0);
		if(krtn) {
			mach_error("msg_receive", krtn);
			continue;
		}
		xpr_server("msg_id %d received\n", inp->msg_id, 2,3,4,5);
		return_msg = FALSE;
		if(inp->msg_local_port == config_port) {
			Config_server(inp, outp);
			return_msg = TRUE;
		}
		else if(inp->msg_local_port == notify_port) {
			config_port_death(inp);
		}
		else {
			printf("Config server_loop: Bogus msg_local_port\n");
		}
		
		if(return_msg) {
			/*
			 * Return message to client. We don't deal with 
			 * backed up port queues for now.
			 */
			krtn = msg_send(outp, SEND_TIMEOUT, 0);
			if(krtn) {
				mach_error("msg_receive", krtn);
				continue;
			}
		}
	}
	/* NOT REACHED */
}

/*
 * Handle port death notification. The only ports we're interested in are
 * driver_ports.
 */
static void config_port_death(msg_header_t *inp)
{
	notification_t 	*msgp = (notification_t *)inp;
	driver_entry_t 	*driver_entry;
	queue_head_t 	dev_list;
	dev_entry_t 	*dev_entry;
	IOReturn drtn;
	
	/*
	 * FIXME: use NOTIFY_PORT_DESTOYED??
	 */
	if(inp->msg_id != NOTIFY_PORT_DELETED) {
		xpr_server("config_port_death: weird msg_id (%d)\n",
			inp->msg_id, 2,3,4,5);
		return;
	}
	xpr_server("config_port_death: port %d\n", msgp->notify_port, 2,3,4,5);
	get_driver_list_lock();
	driver_entry = get_driver_entry(
		SK_DRIVER_PORT,
		msgp->notify_port,
		DEV_NUM_NULL,
		FID_NULL,
		IO_NULL_DEVICE_TYPE,
		IO_NULL_SLOT_ID,
		NULL);
	release_driver_list_lock();
	if(driver_entry == NULL) {
		xpr_server("config_port_death: not sig port\n", 
			1,2,3,4,5);
		return;
	}
	driver_entry->driver_port = PORT_NULL;
	
	/*
	 * Shut down all devices associated with this driver.
	 * First, move all of this driver's dev_entry's to our local
	 * queue.
	 */
	queue_init(&dev_list);
	while(!queue_empty(&driver_entry->dev_list)) {
		dev_entry = (dev_entry_t *)
			queue_first(&driver_entry->dev_list);
		queue_remove(&driver_entry->dev_list,
			dev_entry,
			dev_entry_t *,
			link);
		queue_enter(&dev_list,
			dev_entry,
			dev_entry_t *,
			link);
	}
	
	/*
	 * For each device, destroy the associated device port, then 
	 * create a new one for the same physical device.
	 */
	while(!queue_empty(&dev_list)) {
		dev_entry = (dev_entry_t *)queue_first(&dev_list);
		queue_remove(&dev_list,
			dev_entry,
			dev_entry_t *,
			link);
		if(dev_entry->dev_port) {
			drtn = _IODestroyDevicePort(device_master_port,
				dev_entry->dev_port);
			if(drtn) {
				xpr_common("config_port_death: "
				    "_IODestroyDevicePort() returned %s\n", 
				    IOFindNameForValue(drtn, dev_returns),
				    2,3,4,5);
				/* but keep going... */
			}
		}
		create_dev_entry(driver_entry,
			dev_entry->dev_number,
			dev_entry->dev_type,
			dev_entry->slot_id);
			
		/*
		 * Our dev_entry is a useless copy...
		 */
		free(dev_entry);
	}
	
	/*
	 * Now restart the driver.
	 */
	xpr_server("config_port_death: restarting driver %s\n", 
		IOCopyString(driver_entry->filename), 2,3,4,5);
	if(exec_driver(driver_entry) != IO_CNF_SUCCESS) {
		/*
		 * Couldn't exec this driver. Remove from driver_list.
		 */
		get_driver_list_lock();
		free_driver_entry(driver_entry);
		release_driver_list_lock();
	}
}

/*
 * These functions are called out from the mig-generated Config_server.
 */
 
/*
 * Each driver which we exec calls this exactly once. The purpose is to 
 * register a driver_port for the driver's driver_entry so we can detect the
 * death of the driver. 
 */
IOConfigReturn _IORegisterDriver(
	port_t dev_config_port,
	port_t driver_sig_port,
	port_t driver_port)
{
	driver_entry_t *driver_entry;
	IOConfigReturn rtn = IO_CNF_SUCCESS;
	
	xpr_server("driver_register driver_port %d\n", driver_port, 2,3,4,5);
	get_driver_list_lock();
	
	/*
	 * Make sure that this is a new registration for a device we
	 * know about.
	 */
	driver_entry = get_driver_entry(
		SK_SIG_PORT,
		driver_sig_port,
		DEV_NUM_NULL,
		FID_NULL,
		IO_NULL_DEVICE_TYPE,
		IO_NULL_SLOT_ID,
		NULL);
	if(driver_entry == NULL) {
		xpr_server("device_register: BAD driver_sig_port\n",
			 1,2,3,4,5);
		rtn = IO_CNF_NOT_REGISTERED;
		goto done;
	}
	if(driver_entry->driver_port != PORT_NULL) {
		xpr_server("device_register: REDUNDANT REGISTRATION\n",
			1,2,3,4,5);
		rtn = IO_CNF_REGISTERED;
		goto done;
	}
	driver_entry->driver_port = driver_port;
done:
	release_driver_list_lock();
	return rtn;
}

/*
 * Release all state associated with specified driver. Can only be called 
 * by drivers we exec.	
 */
IOConfigReturn _IODeleteDriver(
	port_t dev_config_port,
	port_t driver_sig_port)
{
	driver_entry_t *driver_entry;
	IOConfigReturn rtn = IO_CNF_SUCCESS;

	xpr_server("driver_delete\n", 1,2,3,4,5);
	
	get_driver_list_lock();
	driver_entry = get_driver_entry(
		SK_SIG_PORT,
		driver_sig_port,
		DEV_NUM_NULL,
		FID_NULL,
		IO_NULL_DEVICE_TYPE,
		IO_NULL_SLOT_ID,
		NULL);
	if(driver_entry == NULL) {
		xpr_server("driver_delete: BAD driver_sig_port\n",
			 1,2,3,4,5);
		rtn = IO_CNF_NOT_REGISTERED;
		goto done;
	}
	
	rtn = free_driver_entry(driver_entry);
done:
	release_driver_list_lock();
	return rtn;
}

#if	SUPPORT_DEVICE_DELETE
/*
 * Release state associated with one device. Can only be called 
 * by drivers we exec.
 *
 * FIXME:
 * problem - a driver deletes some ports via device_delete(), then crashes. 
 * On restart, should it have access to all of the dev_ports it initially 
 * had? Currently it won't because Config did device_destroy()'s on the 
 * dev_ports. Those dev_entry's will be missing from the driver_entry's
 * dev_list...
 *
 * Also, how to assign a deleted device to another driver if the deletor 
 * keeps running? A config_scan() will just add it to that driver's 
 * dev_list and since that driver's already running, it won't get exec'd. 
 *
 * SO...for now we don't support device_delete().
 */		
IOConfigReturn _IODeleteDevice(
	port_t dev_config_port,
	port_t driver_sig_port,
	dev_port_t dev_port)
{
	driver_entry_t *driver_entry;
	dev_entry_t *dev_entry;
	IOConfigReturn rtn = IO_CNF_SUCCESS;
	
	xpr_server("device_delete\n", 1,2,3,4,5);
	get_driver_list_lock();
	driver_entry = get_driver_entry(
		SK_DEV_PORT, 
		dev_port, 
		DEV_NUM_NULL,
		FID_NULL,
		IO_NULL_DEVICE_TYPE,
		IO_NULL_SLOT_ID,
		&dev_entry);
	if(driver_entry == NULL) {
		rtn = IO_CNF_NOT_REGISTERED;
		goto done;
	}
	if(dev_entry == NULL) {
		/*
		 * Huh? This should not happen...
		 */
		rtn = IO_CNF_NOT_REGISTERED;
		goto done;
	}
	if(driver_entry->driver_sig_port != driver_sig_port) {
		rtn = IO_CNF_NOT_REGISTERED;
		goto done;
	}
	rtn = free_dev_entry(dev_entry);
done:
	release_driver_list_lock();
	xpr_server("device_delete: returning %s\n", 
		reg_text(*rtn, config_returns), 2,3,4,5);
	return rtn;
}

#endif	SUPPORT_DEVICE_DELETE

/*
 * Do a complete rescan of all potential devices. Exec drivers for the
 * devices which we don't currently have registered drivers. This can be
 * called by anyone who can get dev_config_port. We return Busy status if
 * initial config is still in progress.
 */
IOConfigReturn _IORescanDriver(
	port_t dev_config_port)
{
	IOConfigReturn rtn;
	
	xpr_server("driver_rescan\n", 1,2,3,4,5);
	if(config_in_progress) {
		return(IO_CNF_BUSY);
	}
	rtn = config_scan(SCAN_ALL, IO_NULL_SLOT_ID, IO_NULL_DEVICE_TYPE);
	return rtn;
}

/*
 * Attempt to start a driver for device specified by slot_id and dev_type.
 * This can be called by anyone who can get dev_config_port. We return 
 * Busy status if initial config is still in progress.	
 */
IOConfigReturn _IOConfigDevice(
	port_t dev_config_port,
	IOSlotId slot_id,
	IODeviceType dev_type)
{
	IOConfigReturn rtn;

	xpr_server("driver_config\n", 1,2,3,4,5);
	if(config_in_progress) {
		return(IO_CNF_BUSY);
	}
	rtn = config_scan(SCAN_ONE, slot_id, dev_type);
	return rtn;
}

#ifdef	DEBUG

/*
 * For debugging new drivers. Delete state associated with specified slot_id 
 * and device_type. 
 */
IOConfigReturn _IODeleteDeviceByType(
	port_t configPort,
	IOSlotId slot_id,
	IODeviceType device_type)
{
	IOConfigReturn rtn;
	driver_entry_t *driver_entry;
	dev_entry_t *dev_entry;
	
	xpr_server("IODeleteDeviceByType: slot_id 0x%x device_type 0x%x\n",
		slot_id, device_type, 3,4,5);
		
	/*
	 * Get driver_entry associated with this slotId and deviceType.
	 */
	get_driver_list_lock();
	driver_entry = get_driver_entry(
		SK_DEV_INDEX,
		PORT_NULL,
		DEV_NUM_NULL,
		FID_NULL,
		device_type,
		slot_id,
		&dev_entry);
	if(driver_entry == NULL) {
		rtn = IO_CNF_NOT_FOUND;
		goto done;
	}
	
	rtn = free_driver_entry(driver_entry);
done:
	release_driver_list_lock();
	return rtn;

}
#endif	DEBUG
