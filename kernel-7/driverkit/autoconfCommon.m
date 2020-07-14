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
 * autoconfCommon.m - machine-independent driverkit-style 
 *		      autoconfiguration routines.
 *
 * HISTORY
 * 23-Jan-93    Doug Mitchell at NeXT
 *      Created.
 */

#import <mach/mach_types.h>

#import <driverkit/autoconfCommon.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <machkit/NXLock.h>
#import <driverkit/driverTypes.h>
#import <driverkit/volCheck.h>

#import <vm/vm_kern.h>

#import <sys/param.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <sys/buf.h>
#import <sys/systm.h>
#import <dev/busvar.h>

extern task_t IOTask_kern;           // kernel internal version of IOTask
extern port_t IOTask;     	     // IOTask port name in IOTask space.

/*
 * Static data.
 */
static id autoconfLock;

/*
 * Static routines.
 */
static void registerIndirClasses();
static void autoconfInt();
static void probePseudoDevices();

/* Forward declaration */
extern port_name_t _io_task_get_port(void *kport);

/*
 * Top-level autoconf routine.
 */
void autoconf(void)
{
	struct pseudo_init *pi;
	static void IOTaskInit(void);

	xpr_conf("autoconf\n", 1,2,3,4,5);
	IOTaskInit();
	IOInitGeneralFuncs();
	volCheckInit();
	xpr_conf("autoconf: libIO initialized\n", 1,2,3,4,5);

	/* 
	 * Early startup for bsd pseudodevices.
	 */
	for (pi = pseudo_inits; pi->ps_func; pi++)
		(*pi->ps_func) (pi->ps_count);

	/*
	 * Set up a lock so we know when autoconf is complete.
	 */
	autoconfLock = [NXConditionLock alloc];
	[autoconfLock initWith:NO];
	xpr_conf("autoconf: forking off thread\n", 1,2,3,4,5);
	
	/*
	 * Note since we are not currently in the IOTask context, we
	 * can't use IOForkThread.
	 */
	kernel_thread(IOTask_kern, autoconfInt, (void *)0);
	[autoconfLock lockWhen:YES];
	[autoconfLock free];
}

/*
 * One-time only init for this module. 
 */
#ifdef	i386
extern queue_head_t dmaBufQueue;
#endif	i386

static void IOTaskInit(void)
{
	kern_return_t krtn;
	extern task_t		IOTask_kern;
	extern port_name_t	IOTask;

#ifdef	i386
	queue_init(&dmaBufQueue);
#endif	i386	

	/*
	 * Start up IOTask. No threads needed yet, just the task.
	 */
#undef task_create
	krtn = task_create(kernel_task,	
		FALSE,
		&IOTask_kern);
	if(krtn != KERN_SUCCESS) {
		IOLog("IOLibIOInit task_create returned %d\n", krtn);
		return;
	}
	task_deallocate(IOTask_kern); // extra ref for convert_task_to_port()
	(void) vm_map_deallocate(IOTask_kern->map);
	IOTask_kern->map = kernel_map;
	
	/*
	 * This allows use of mach RPCs as if from user space, while running
	 * in the kernel's pmap.
	 */
	IOTask_kern->kernel_vm_space = TRUE;
	
	IOTask_kern->proc = kernproc;
	
	(void) processor_set_policy_enable(&default_pset, POLICY_FIXEDPRI);
	
	/*
	 * Get IOTask's task port in its own IPC space. 
	 */
	IOTask = _io_task_get_port(IOTask_kern->itk_sself);
}

void *_io_vm_task_buf(
	struct buf	*bp
)
{
	task_t		task;

	if((bp->b_flags & (B_PHYS|B_KERNSPACE)) == B_PHYS) {
		/*
		 * Physical I/O to user space.
		 */
		task = bp->b_proc->task;
	}
	else {
		/*
		 * Either block I/O (always kernel space) or physical I/O
		 * to kernel space (e.g., loadable file system).
		 */
		task = IOTask_kern;
	}

	return task->map;
}

void *_io_vm_task_self(void)
{
	return (IOTask_kern->map);
}

void *_io_vm_task_current(void)
{
	return (current_task()->map);
}

void *_io_vm_task(
	task_t		task
)
{
	return (task->map);
}

void *_io_vm_task_pmap(
	vm_map_t	map
)
{
	return (vm_map_pmap(map));
}

port_name_t _io_task_get_port(
    void *		kport
)
{
	/* External references to the mach system */
	extern void port_reference();
	extern void object_copyout();

	port_name_t	port_name;

	port_reference(kport);
	object_copyout(IOTask_kern,
		kport,
		MSG_TYPE_PORT,
		&port_name);

	return (port_name);
}

void *_io_get_kern_port(
    port_name_t		port
)
{
	/* External references to the mach system */
	extern boolean_t object_copyin();

	void *		kport;

	if (!object_copyin(IOTask_kern,
				port,
				MSG_TYPE_PORT,
				FALSE,
				&kport))
		return (0);
		
	return (kport);
}

void *_io_convert_port_in(
    port_name_t		port
)
{
	/* External references to the mach system */
	extern boolean_t object_copyin();

	void * 		kport;

	if (!object_copyin(current_task(),
				port,
				MSG_TYPE_PORT,
				FALSE,
				&kport))
		return (0);
		
	return (kport);
}

port_name_t _io_convert_port_out(
    void		*kport
)
{
	/* External references to the mach system */
	extern void port_reference();
	extern void object_copyout();

	port_name_t	port_name;

	port_reference(kport);
	object_copyout(current_task(),
			kport,
			MSG_TYPE_PORT,
			&port_name);
			
	return (port_name);
}

port_t	_io_host_priv_self(void)
{
	return _io_convert_port_out((void *)realhost.host_priv_self);
}

/*
 * This rest of this module runs as a thread in IOTask to allow for 
 * each driver's startup code to execute in an IOTask thread context, 
 * so standard Mach calls can be executed during probe.
 * 
 * Note that all of the +probe: calls below may result in indirect classes
 * being probed via IODevice's +connectToIndirectDevices method.
 */

static void autoconfInt(void)
{
	
	xpr_conf("autoconf_int: starting\n", 1,2,3,4,5);
		  
	/*
	 * Register indirect device classes.
	 */
	registerIndirClasses();
	
	/*
	 * Start up drivers for native devices.
	 */
	probeNativeDevices();
	  
   	/*
    	 * Perform machine dependent hardware probe/config.
    	 */
	probeHardware();
	
    	/*
    	 * Start up non-native direct drivers.
    	 */
	probeDirectDevices();
	
	/*
	 * Start up pseuododevices.
	 */
	probePseudoDevices();
	
	/*
	 * Notify parent that we're finished, then terminate.
	 */
	[autoconfLock lock];
	[autoconfLock unlockWith:YES];
	IOExitThread();
}

static void registerIndirClasses(void)
{
	char 	**indClassName = indirectDevList;
	id	indClassId;
	for(indClassName=indirectDevList; 
	    *indClassName;
	    indClassName++) {
		indClassId = objc_getClass(*indClassName);
		if(indClassId == nil) {
			IOLog("registerIndirectClasses: Class %s does not "
				"exist\n", *indClassName);
			continue;
		}
		/*
		 * Objc runtime does an +initialize on the class here; the 
		 * class must call registerClass at that time.
		 */
		xpr_conf("init'ing indirect class %s\n", *indClassName,
			2,3,4,5);
		[indClassId name];
	}
}

static void probePseudoDevices(void)
{
	char	 		**pseudoClassName;
	id 			driverClass;
			
	for(pseudoClassName = pseudoDevList;
	    *pseudoClassName;
	    pseudoClassName++) {
		xpr_conf("pseudo dev %s found\n",
			*pseudoClassName, 2,3,4,5);
		driverClass = objc_getClass(*pseudoClassName);
		if(driverClass == nil) {
			IOLog("probePseudoDevices: class %s not found\n",
				*pseudoClassName);
			continue;
		}
		[IODevice addLoadedClass:driverClass
			description:nil];	
	}
}

#import <driverkit/IODeviceParams.h>
#import <driverkit/IOPower.h>

static void

_io_sendPowerMessage(
    SEL selector,
    void *arg
)
{
    IOObjectNumber i = 0;
    id driverObject;
    IOReturn rtn;
    
    /* Look for devices that conform to IOPower protocol */
    while ((rtn = [IODevice lookupByObjectNumber:i++
			    instance:&driverObject]) != IO_R_NO_DEVICE) {
	if (rtn == IO_R_OFFLINE)
	    continue;
	if ([[driverObject class] conformsTo:@protocol(IOPower)]) {
	    [driverObject perform:selector with:arg];
	}
    }
}


void
_io_setDriverPowerState(
    PMPowerState state
)
{
    _io_sendPowerMessage(@selector(setPowerState:), (void *)state);
}

void
_ioSetDriverPowerManagementState(
    PMPowerManagementState state
)
{
    _io_sendPowerMessage(@selector(setPowerManagement:), (void *)state);
}

