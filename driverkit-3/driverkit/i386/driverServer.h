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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * i386-specific driverServer RPCs.
 *
 * HISTORY
 *
 * 29 September 1992 David E. Bohman at NeXT
 *	Created.
 */

#import <driverkit/i386/driverTypesPrivate.h>

/*
 * EISA Specific calls.
 */
 
/* Routine IOGetEISADeviceConfig */
IOReturn _IOGetEISADeviceConfig (
	port_t device,
	IOEISAInterruptList irq_conf,
	unsigned int *irq_confCnt,
	IOEISADMAChannelList dma_conf,
	unsigned int *dma_confCnt,
	IOEISAPortMap io_conf,
	unsigned int *io_confCnt,
	IOEISAMemoryMap phys_conf,
	unsigned int *phys_confCnt);

/* Routine IOMapEISADevicePorts */
IOReturn _IOMapEISADevicePorts (
	port_t device,
	thread_t thread);

/* Routine IOUnMapEISADevicePorts */
IOReturn _IOUnMapEISADevicePorts (
	port_t device,
	thread_t thread);

/* Routine IOMapEISADeviceMemory */
IOReturn _IOMapEISADeviceMemory (
	port_t device,
	task_t target_task,
	vm_offset_t phys,
	vm_size_t length,
	vm_offset_t *addr,
	BOOL anywhere,
	IOCache cache);

/* Routine IOEnableEISAInterrupt */
IOReturn _IOEnableEISAInterrupt (
	port_t device,
	int interrupt);

/* Routine IODisableEISAInterrupt */
IOReturn _IODisableEISAInterrupt (
	port_t device,
	int interrupt);
