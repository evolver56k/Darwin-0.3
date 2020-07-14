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
 * ConfigPrivate.h - private #defines for Config server.
 *
 * HISTORY
 * 11-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#ifndef	_CONFIG_PRIVATE_H_
#define _CONFIG_PRIVATE_H_

#import <bsd/sys/types.h> 
#import <driverkit/debugging.h>
#import <kernserv/queue.h>
#import <mach/cthreads.h>
#import <bsd/sys/dir.h>

/*
 * Typedefs for identifying executable driver files.
 */
typedef unsigned long file_id_t;		// like fileno
#define FILENAME_SIZE 	(MAXNAMLEN+1)
typedef char filename_t[FILENAME_SIZE];		// human-readable name

/*
 * A list of these is maintained in driver_list. One driver_entry_t per driver.
 * dev_list contains a list of dev_entry_t's, allowing mapping multiple devices
 * to one driver.
 */
typedef struct {
	queue_head_t	dev_list;
	file_id_t	executable_fid;	
	filename_t	filename;		// name of executable
	port_t		driver_port;		// created by driver
	port_t		driver_sig_port;	// created by us
	port_t		boot_requestor;		// requestor_port for 
						//   bootstrap_subset()
	boolean_t	running;		// we've exec'd it
	queue_chain_t 	link;
} driver_entry_t;

/*
 * One per dev_port to which driver has rights.
 */
typedef struct {	
	port_t		dev_port;		// created by kernel
	IODeviceType	dev_type;
	IOSlotId 	slot_id;
	IODeviceNumber	dev_number;
	driver_entry_t	*driver_entry;		// in whose dev_list this 
						//   resides
	queue_chain_t	link;
} dev_entry_t;	


/*
 * XPR control.
 */
#define XPR_CONFIG_INDEX	0

#ifdef	DDM_DEBUG

#define XPR_CONFIG	0x00000001
#define XPR_SERVER	0x00000002
#define XPR_COMMON	0x00000004
#define XPR_ERR		0x00000008

#define xpr_config(x, a, b, c, d, e) {					\
	if(IODDMMasks[XPR_CONFIG_INDEX] & XPR_CONFIG) {			\
		IOAddDDMEntry(x, (int)a, (int)b, (int)c, (int)d, (int)e);	\
		if(verbose)						\
			printf(x, a, b, c, d, e);			\
	}								\
}
#define xpr_server(x, a, b, c, d, e) {					\
	if(IODDMMasks[XPR_CONFIG_INDEX] & XPR_SERVER) {			\
		IOAddDDMEntry(x, (int)a, (int)b, (int)c, (int)d, (int)e);	\
		if(verbose)						\
			printf(x, a, b, c, d, e);			\
	}								\
}
#define xpr_common(x, a, b, c, d, e) {					\
	if(IODDMMasks[XPR_CONFIG_INDEX] & XPR_COMMON) {			\
		IOAddDDMEntry(x, (int)a, (int)b, (int)c, (int)d, (int)e);	\
		if(verbose)						\
			printf(x, a, b, c, d, e);			\
	}								\
}
#define xpr_err(x, a, b, c, d, e) {					\
	if(IODDMMasks[XPR_CONFIG_INDEX] & XPR_ERR) {			\
		IOAddDDMEntry(x, (int)a, (int)b, (int)c, (int)d, (int)e);	\
	}								\
	printf(x, a, b, c, d, e);					\
}

#else	DDM_DEBUG

#define xpr_config(x, a, b, c, d, e)
#define xpr_server(x, a, b, c, d, e)
#define xpr_common(x, a, b, c, d, e)
#define xpr_err(x, a, b, c, d, e)

#endif	DDM_DEBUG

/*
 * Externs.
 */
extern queue_head_t driver_list;
extern mutex_t driver_list_lock;
extern condition_t driver_list_cond;
extern boolean_t driver_list_locked;
extern int verbose;
extern port_t device_master_port;

#endif	_CONFIG_PRIVATE_H_
