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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * generalFuncsPrivate.m - Kernel internal (KERNEL_PRIVATE) portion of 
 * generalFuncs module.
 *
 * HISTORY
 * 31-Oct-91    Doug Mitchell at NeXT
 *      Created.
 */

#define ARCH_PRIVATE	1

#import <mach/mach_types.h>

#import <objc/objc.h> 
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <stdarg.h>
#import <kern/sched.h>
#import <machkit/NXLock.h>
#import <machine/param.h>

#import <mach/vm_param.h>

extern void	*_io_vm_task(void *);
extern void	*_io_vm_task_self(void);
extern void	*_io_vm_task_pmap(vm_task_t	task);
extern void	*_io_vm_task_current(void);
extern void	*_io_vm_task_buf(void *);
extern void 	*_io_convert_port_in(port_t port);
extern port_t	_io_convert_port_out(void *kport);
extern port_t	_io_host_priv_self(void);
extern void 	*_io_get_kern_port(port_name_t port);
extern port_name_t	_io_task_get_port(void *kport);

	


/*
 * Static variables for this module.
 */
static IOThreadFunc threadArgFcn;
static void *threadArgArg;
static int libInitialized;

/*
 * globals shared with libDriver's libIO.m and the kern_dev server.
 */
port_name_t IOTask;			// IOTask's version of its own 
					//    task_t
task_t IOTask_kern;			// kernel internal version of IOTask

static id threadArgLock;		// NXLock 


#ifdef	DEBUG
extern void iotaskTest();
#endif	DEBUG;

/*
 * Local prototypes.
 */
static void ioThreadStart();

void IOInitGeneralFuncs(void)
{
	if(libInitialized) {
		return;	
	}

	threadArgLock = [NXLock new];

	libInitialized = 1;
}

/*
 * We pass an argument to a new thread by saving fcn and arg in some
 * locked variables and starting the thread at ioThreadStart(). This
 * function retrives fcn and arg and makes the appropriate call.
 *
 * Note in the kernel, IOThread == thread_t. 
 */
IOThread IOForkThread(IOThreadFunc fcn, void *arg)
{
	IOThread thread;
	/*
	 * We do the lock, ioThreadStart() does the unlock.
	 */
	[threadArgLock lock];
	threadArgFcn = fcn;
	threadArgArg = arg;
	thread = (IOThread)kernel_thread(IOTask_kern, ioThreadStart);
	(void) thread_priority(thread, MAXPRI_USER, FALSE);
	return(thread);
}

/*
 * All kernel IOThreads start here.
 */
static void ioThreadStart()
{
	IOThreadFunc fcn = threadArgFcn;
	void *arg = threadArgArg;

	[threadArgLock unlock];
	
	(void) thread_wire(1, (thread_t)current_thread_EXTERNAL(), TRUE);

	(*fcn)(arg);
	
	/*
	 * And just in case...
	 */
	IOExitThread();
}

IOReturn
IOSetThreadPriority(IOThread iothread, int priority)
{
    thread_t		thread = (thread_t)iothread;
    kern_return_t	result;
    
    result = thread_priority(thread, priority, FALSE);
    if (result == KERN_INVALID_ARGUMENT)
    	return (IO_R_INVALID_ARG);
    else if (result == KERN_FAILURE)
    	return (IO_R_PRIVILEGE);
	
    return (IO_R_SUCCESS);
}

IOReturn
IOSetThreadPolicy(IOThread iothread, int policy)
{
    thread_t		thread = (thread_t)iothread;
    int			policy_data;
    kern_return_t	result;
    
    if (policy == POLICY_FIXEDPRI)
    	policy_data = min_quantum;
    else
    	policy_data = 0;
    
    result = thread_policy(thread, policy, policy_data);
    if (result == KERN_INVALID_ARGUMENT)
    	return (IO_R_INVALID_ARG);
    else if (result == KERN_FAILURE)
    	return (IO_R_PRIVILEGE);
	
    return (IO_R_SUCCESS);
}

void IOSuspendThread(IOThread thread)
{
	thread_suspend((thread_t)thread);
}

void IOResumeThread(IOThread thread)
{
	thread_resume((thread_t)thread);
}

volatile void IOExitThread()
{
	thread_terminate((thread_t)current_thread_EXTERNAL());
	(volatile void)thread_halt_self(); 
}


/*
 * This returns the kernel's map under the assumption that this function 
 * is called in preparation for setting up a DMA via IOEnqueueDmaInt(). 
 * The IOTask itself can not access memory in its vm_map (IOTask_kern->map).
 * For actual VM operations using the Mach API, use task_self().
 */
vm_task_t IOVmTaskSelf()
{
	return((vm_task_t)_io_vm_task_self());
}

/*
 * Returns the current task's vm_task_t.
 */
vm_task_t IOVmTaskCurrent()
{

	return ((vm_task_t)_io_vm_task_current());
}

/*
 * Set the current thread's UNIX errno.
 */
void IOSetUNIXError(int errno)
{
#warning u.u_error passing disabled
#if 0
	u.u_error = errno;
#endif 0
}
 
/*
 * Get an IOTask version of a kern_port_t. 
 */
port_name_t IOTaskGetPort(port_t kern_port)
{
	void *			kport = (void *)kern_port;

	return (_io_task_get_port(kport));
}

/* 
 * Get a kern_port_t version of an IOTask port.
 */
port_t IOGetKernPort(port_name_t userPort)
{
	return ((port_t)_io_get_kern_port(userPort));
}


/*
 * Convert any kind of port to any other kind.
 * Returns PORT_NULL on error.
 */
port_t IOConvertPort(port_t inPort,	// to be converted
	IOIPCSpace from,		// IPC space of inPort
	IOIPCSpace to)			// IPCSpace of returned port
{
	void * 		inPortKern;
	port_t 		outPort;
	
	/*
	 * First get a kern_port_t version of inPort.
	 */
	switch(from) {
	    case IO_Kernel:
	    	inPortKern = (void *)inPort;
		break;
	    case IO_KernelIOTask:
	    	inPortKern = (void *)IOGetKernPort(inPort);
		break;
	    case IO_CurrentTask:
	    		inPortKern = _io_convert_port_in(inPort);
		if(!inPortKern) {
			IOLog("IOConvertPort: Bad Port\n");
			return PORT_NULL;
		}
	    	break;
	}
	
	/*
	 * Now convert as appropriate.
	 */
	switch(to) {
	    case IO_Kernel:
	    	outPort = (port_t)inPortKern;
		break;
	    case IO_KernelIOTask:
	    	outPort = IOTaskGetPort((port_t)inPortKern);
		break;
	    case IO_CurrentTask:
	    	outPort = _io_convert_port_out(inPortKern);
		break;
	}
	return outPort;
}

/*
 * Returns vm_task_t associated with a struct buf. 
 */
vm_task_t IOVmTaskForBuf(struct buf *buf)
{
	return ((vm_task_t)_io_vm_task_buf(buf));
}

/*
 * Obtain the IOTask version of host_priv_self() (the privileged host port).
 */

port_t IOHostPrivSelf()
{
	return (_io_host_priv_self());
}

/*
 *  Find a physical address (if any) for the specified virtual address.
 */
IOReturn IOPhysicalFromVirtual(vm_task_t task, 
	vm_address_t virtualAddress,
	unsigned *physicalAddress)
{
	*physicalAddress = pmap_extract(_io_vm_task_pmap(task),
		virtualAddress);
	if(*physicalAddress == 0) {
		return IO_R_INVALID_ARG;
	}
	else {
		return IO_R_SUCCESS;
	}
}

/*
 * Create a mapping in the IOTask address map
 * for the specified physical region.  Assumes
 * that the physical memory is already wired.
 */
IOReturn
IOMapPhysicalIntoIOTask(
    unsigned		physicalAddress,
    unsigned		length,
    vm_address_t	*virtualAddressP
)
{
    kern_return_t	result;
    vm_offset_t		vAddr;
    
    result = vm_allocate(_io_vm_task_self(), virtualAddressP, length, TRUE);
    if (result != KERN_SUCCESS)
    	return (IO_R_NO_SPACE);
	
    vAddr = *virtualAddressP;
    vAddr = trunc_page(vAddr);
    length = round_page(length);
    
    physicalAddress = trunc_page(physicalAddress);
    
    while (length > 0) {
    	pmap_enter(_io_vm_task_pmap((vm_task_t)_io_vm_task_self()),
		vAddr, physicalAddress, VM_PROT_READ|VM_PROT_WRITE, TRUE);
		
	vAddr += PAGE_SIZE; length -= PAGE_SIZE; physicalAddress += PAGE_SIZE;
    }
    
    return (IO_R_SUCCESS);
}

IOReturn
IOMapPhysicalIntoIOTaskUnaligned(
    unsigned		physicalAddress,
    unsigned		length,
    vm_address_t	*virtualAddressP
)
{
    IOReturn		result;

    /* Recompute the length based on the actual region requested */
    length = round_page(physicalAddress + length) - physicalAddress;

    result = IOMapPhysicalIntoIOTask(physicalAddress, length, virtualAddressP);
    if (result == IO_R_SUCCESS) {
	/* Now remap the alloced pointer to point to the right place */ 
	*virtualAddressP += physicalAddress - trunc_page(physicalAddress);
    }
    return result;
}

/*
 * Destroy a mapping created by
 * IOTaskCreateVirtualFromPhysical()
 */
IOReturn
IOUnmapPhysicalFromIOTask(
    vm_address_t	virtualAddress,
    unsigned		length
)
{
    (void) vm_deallocate(_io_vm_task_self(), virtualAddress, length);

    return (IO_R_SUCCESS);
}

/* end of generalFuncsPrivate.m */
