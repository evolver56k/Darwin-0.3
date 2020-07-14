/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Driverkit kernel support.
 *
 * HISTORY
 *
 * 15 Jan 1998	   Martin Minow
 *   Added kern_IOCreateMachPort
 * 28 Aug 1992	Joe Pasqua
 *	Use new interface to interrupt mechanism.
 * 25 Aug 1992	Joe Pasqua
 *	Added minimal implementations of kern{IOAttach|Detach}Interrupt,
 *	kern_IOCreateDevicePort.
 * 9 August 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <ipc/ipc_space.h>

#import <kern/kern_port.h>
#import	<kern/task.h>

#import <vm/vm_kern.h>

#import <driverkit/generalFuncs.h>
#import <driverkit/IODevice.h>
#import <driverkit/IODeviceParams.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/KernBus.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/driverServerXXX.h>
#import <driverkit/autoconfCommon.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/driverTypesPrivate.h>
#import <driverkit/configTableKern.h>

#ifdef __i386__
#import <driverkit/i386/driverServer.h>
#import <driverkit/i386/EISAKernBus.h>
#endif

#ifdef __ppc__
#import <driverkit/ppc/PPCKernBus.h>
#endif

#import <sys/param.h>
#import <sys/systm.h>
#import <machkit/NXLock.h>
#import <objc/List.h>

void
dev_server_init(
    void
)
{
#if	i386
    /*
     * Initialize dma controller(s).
     */
    dma_initialize();
    
    /*
     * Initialize DMA lock mechanism.
     */
    initDmaLock();
#endif	i386
}

KernDevice *
convert_port_to_dev(
    port_t	dev_port
)
{
    KernDevice		*kernDevice = nil;
    kern_port_t		port = (kern_port_t)dev_port;

    if (port == IP_NULL)
	return nil;

    ip_lock(port);

    if(ip_kotype(port) == IKOT_DEVICE)
	kernDevice = (KernDevice *)port->ip_kobject;

    ip_unlock(port);

    return kernDevice;
}

port_t
create_dev_port(
    KernDevice		*kernDevice
)
{
    kern_port_t		devicePort;
    if ((devicePort = ipc_port_alloc_kernel()) == IP_NULL)
    	return (PORT_NULL);

    ipc_kobject_set(devicePort, (unsigned int)kernDevice, IKOT_DEVICE);
    
    return (IOConvertPort((port_t)devicePort,
			    IO_Kernel, IO_KernelIOTask));
}

void
destroy_dev_port(
    port_t		port
)
{
    kern_port_t		devicePort;

    if (port != PORT_NULL) {
    	devicePort = (kern_port_t)IOConvertPort(port,
						IO_KernelIOTask, IO_Kernel);
	ip_lock(devicePort);
    	ipc_port_destroy(devicePort);
    }
}

/*
 * Determine deviceType and deviceName of specified objectNumber.
 */
IOReturn kern_IOLookupByObjectNumber(
	host_t 		device_master,
	IOObjectNumber 	objectNumber,
	IOString 	*deviceKind,		// returned
	IOString 	*deviceName)		// returned
{
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	return  [IODevice lookupByObjectNumber:objectNumber
		deviceKind:deviceKind
		deviceName:deviceName];
}

/*
 * Determine deviceType and IOObjectNumber of specified IOString.
 */
IOReturn kern_IOLookupByDeviceName(
	host_t 		device_master,
	IOString 	deviceName,
	IOObjectNumber 	*objectNumber,		// returned
	IOString 	*deviceKind)		// returned
{
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	return [IODevice lookupByDeviceName:deviceName
		objectNumber:objectNumber
		deviceKind:deviceKind];
}

/*
 * Get/set parameter RPCs. 
 */
 
IOReturn kern_IOGetIntValues(
	host_t 		device_master,
	IOObjectNumber 	objectNumber,
	IOParameterName parameterName,
	unsigned int	 maxCount,		// 0 means "as much as
						//    possible"
	unsigned int 	*parameterArray,	// data returned here
	unsigned int 	*returnedCount)		// size returned here
{	
	IOReturn rtn;
	unsigned count = maxCount;
	
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	rtn = [IODevice getIntValues:parameterArray
		forParameter:parameterName
		objectNumber:objectNumber
		count:&count];
	*returnedCount = count;
	return rtn;
}

IOReturn kern_IOGetCharValues(
	host_t 		device_master,
	IOObjectNumber 	objectNumber,
	IOParameterName parameterName,
	unsigned int 	maxCount,		// 0 means "as much as
						//    possible"
	unsigned char 	*parameterArray,	// data returned here
	unsigned int 	*returnedCount)		// size returned here
{	
	IOReturn rtn;
	unsigned count = maxCount;
	
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	rtn = [IODevice getCharValues:parameterArray
		forParameter:parameterName
		objectNumber:objectNumber
		count:&count];
	*returnedCount = count;
	return rtn;
}

IOReturn kern_IOSetIntValues(
	host_priv_t device_master,
	IOObjectNumber objectNumber,
	IOParameterName parameterName,
	unsigned int *parameterArray,
	unsigned int count)			// size of parameterArray
{	
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	return [IODevice setIntValues:parameterArray
		forParameter:parameterName
		objectNumber:objectNumber
		count:count];
}

IOReturn kern_IOSetCharValues(
	host_t 		device_master,
	IOObjectNumber 	objectNumber,
	IOParameterName parameterName,
	unsigned char 	*parameterArray,
	unsigned int 	count)			// size of parameterArray
{	
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	return [IODevice setCharValues:parameterArray
		forParameter:parameterName
		objectNumber:objectNumber
		count:count];
}

IOReturn kern_IOLookUpByStringPropertyList(
	host_priv_t 	device_master,
	unsigned char *	values,
	unsigned int 	valuesLen,		// Mig added
	unsigned char *	results,
	unsigned int 	*returnedCount,		// size returned here
	unsigned int	length)
{
	IOReturn	rtn;

	rtn =  [IODevice lookUpByStringPropertyList:(const char *)values
                results      : (char *)results
                maxLength    : (unsigned int)length];

	if( rtn == IO_R_SUCCESS)
	    *returnedCount = strlen( results) + 1;

	return( rtn);
}

IOReturn kern_IOGetStringPropertyList(
	host_priv_t 	device_master,
	IOObjectNumber 	objectNumber,
	unsigned char *	names,
	unsigned int 	nameLen,		// Mig added
	unsigned char *	results,
	unsigned int 	*returnedCount,		// size returned here. Mig added
	unsigned int	length)
{
        IOReturn	rtn;

	rtn =   [IODevice getStringPropertyList:objectNumber
                names        : (const char *)names
                results      : (char *)results
                maxLength    : (unsigned int)length];

	if( rtn == IO_R_SUCCESS)
	    *returnedCount = strlen( results) + 1;

	return( rtn);
}

IOReturn kern_IOGetByteProperty(
	host_priv_t 	device_master,
	IOObjectNumber 	objectNumber,
	unsigned char *	name,
	unsigned int 	nameLen,		// Mig added
	unsigned char *	results,
	unsigned int 	*returnedCount,		// size returned here. Mig added
	unsigned int	length)
{
        IOReturn	rtn;
	*returnedCount = length;

	rtn =   [IODevice getByteProperty:objectNumber
                name         : (const char *)name
                results      : (char *)results
                maxLength    : (unsigned int *)returnedCount];

	return( rtn);
}

IOReturn kern_IOServerConnect(
	host_t 		device_master,
	IOObjectNumber 	objectNumber,
	ipc_port_t      taskIPCPort,
	ipc_port_t     *serverIPCPort)
{
	extern task_t IOTask_kern;	// kernel internal version of IOTask
	IOReturn rtn;

	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	rtn = [IODevice serverConnect: (port_t *) serverIPCPort
			 objectNumber: objectNumber
			     taskPort: (port_t) taskIPCPort];
	if (MACH_PORT_VALID(*serverIPCPort))
	    (void) ipc_object_copyin_compat(IOTask_kern->itk_space,
			*serverIPCPort, MSG_TYPE_PORT, FALSE, serverIPCPort);

	return rtn;
}

IOReturn kern_IOCallDeviceMethod(
	host_priv_t 	device_master,
	IOObjectNumber 	objectNumber,
	char *		methodName,
	unsigned char 	*inputParams,
	unsigned int 	inputCount,		// size of parameterArray
	unsigned int 	*maxCount,		// caller's output buffer size
						// on return the needed size
	unsigned char 	*outputParams,		// data returned here
	unsigned int 	*returnedCount)		// size to be returned here
{	
	IOReturn rtn;
	unsigned count = *maxCount;

	rtn = [IODevice callDeviceMethod : methodName
			inputParams : inputParams
			inputCount : inputCount
			outputParams : outputParams
			outputCount : &count
			privileged : device_master
			objectNumber : objectNumber];

	if( count <= *maxCount)
	    *returnedCount = count;		// how much was returned
	else
	    *returnedCount = *maxCount;

	*maxCount = count;			// how much is available

	return rtn;
}

#ifdef i386

IOReturn
kern_IOGetEISADeviceConfig(
    KernDevice			*device,
    IOEISAInterruptList		interrupts,
    unsigned int		*interruptNum,
    IOEISADMAChannelList	dmaChannels,
    unsigned int		*dmaChannelNum,
    IOEISAPortMap		ioRegions,
    unsigned int		*ioRegionNum,
    IOEISAMemoryMap		memoryRegions,
    unsigned int		*memoryRegionNum
)
{
    int			i, num;
    Range		range;
    id			list, deviceDescription;

    if (device == nil)
	return IO_R_PRIVILEGE;
	
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:IRQ_LEVELS_KEY];
    num = [list count];
    if (num > IO_NUM_EISA_INTERRUPTS)
    	num = IO_NUM_EISA_INTERRUPTS;
    for (i = 0; i < num; i++)
    	interrupts[i] = [[list objectAt:i] item];
    *interruptNum = num;

    list = [deviceDescription resourcesForKey:DMA_CHANNELS_KEY];
    num = [list count];
    if (num > IO_NUM_EISA_DMA_CHANNELS)
    	num = IO_NUM_EISA_DMA_CHANNELS;
    for (i = 0; i < num; i++)
    	dmaChannels[i] = [[list objectAt:i] item];
    *dmaChannelNum = num;

    list = [deviceDescription resourcesForKey:IO_PORTS_KEY];
    num = [list count];
    if (num > IO_NUM_EISA_PORT_RANGES)
    	num = IO_NUM_EISA_PORT_RANGES;
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	ioRegions[i].start = range.base;
	ioRegions[i].size = range.length;
    }
    *ioRegionNum = num;

    list = [deviceDescription resourcesForKey:MEM_MAPS_KEY];
    num = [list count];
    if (num > IO_NUM_EISA_MEMORY_RANGES)
    	num = IO_NUM_EISA_MEMORY_RANGES;
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	memoryRegions[i].start = range.base;
	memoryRegions[i].size = range.length;
    }
    *memoryRegionNum = num;
    
    return IO_R_SUCCESS;
}

IOReturn
kern_IOMapEISADevicePorts(
    KernDevice		*device,
    thread_t		thread
)
{
    IOReturn	rtn;

    if (device == nil)
	return (IO_R_PRIVILEGE);
	
    if (thread->task->kernel_vm_space)
	return (IO_R_SUCCESS);
	
    rtn = kern_dev_map_port_com(device, thread, FALSE);
    
    return (rtn);
}

IOReturn
kern_IOUnMapEISADevicePorts(
    KernDevice		*device,
    thread_t		thread
)
{
    IOReturn	rtn;

    if (device == nil)
	return (IO_R_PRIVILEGE);
	
    if (thread->task->kernel_vm_space)
	return (IO_R_SUCCESS);
	
    rtn = kern_dev_map_port_com(device, thread, TRUE);
    
    return (rtn);
}

IOReturn
kern_IOMapEISADeviceMemory(
    KernDevice		*device,
    task_t 		task,
    vm_offset_t 	phys,
    vm_size_t		length,
    vm_offset_t 	*addr,				// in/out
    BOOL 		anywhere,
    IOCache 	cache
)
{
    IOReturn	rtn;

    if (device == nil)
	return IO_R_PRIVILEGE;
    
    rtn = kern_dev_map_phys(
			    device,
			    task->map,
			    phys,
			    length,
			    addr,
			    anywhere,
			    cache);
    
    return (rtn);
}

IOReturn
kern_dev_map_port_com(
    KernDevice		*device,
    thread_t		thread,
    boolean_t		unmap
)
{
    id			list, deviceDescription;
    Range		range;
    int			i, num;
    
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:IO_PORTS_KEY];
    num = [list count];

    /*
     * Determine what is the highest port
     * number we need to map.
     */
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
    	(void) task_map_io_ports(thread->task, range.base, range.length, unmap);
    }
    
    return (IO_R_SUCCESS);
}

#endif

IOReturn
kern_dev_map_phys(
    KernDevice		*device,
    vm_map_t		map,
    vm_offset_t		phys,
    vm_size_t		length,
    vm_offset_t		*addr,
    boolean_t		anywhere,
    IOCache		cache
)
{
    id			list, deviceDescription;
    kern_return_t	result;
    vm_offset_t		vaddr;
    Range		range;
    int			i, num;
    cache_spec_t	caching;
    
    switch (cache) {
    
    case IO_CacheOff:
    	caching = cache_disable;
	break;
	
    case IO_WriteThrough:
    	caching = cache_writethrough;
	break;
	
    default:
    	caching = cache_default;
	break;
    }
    
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:MEM_MAPS_KEY];
    num = [list count];
    
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	if (range.base <= phys &&
	    (range.base + range.length) >= (phys + length))
	    break;
    }
	    
    if (i == num)
    	return (IO_R_PRIVILEGE);
	
    if (anywhere)
    	*addr = vm_map_min(map);
    else
    	*addr = trunc_page(*addr);
	
    if (map == kernel_map) {
	if (!anywhere &&
	    *addr < 1*1024*1024)	// In the hole... (between 640K and 1M)
	    return (IO_R_SUCCESS);
    }
	
    result = vm_map_find(
    			map,
			VM_OBJECT_NULL, (vm_offset_t) 0,
			addr, length,
			anywhere);

    if (result != KERN_SUCCESS)
    	return (IO_R_NO_SPACE);

    vaddr = *addr;
    vaddr = trunc_page(vaddr);
    length = round_page(length);
	
    (void) vm_map_inherit(
    			map,
    			vaddr, vaddr + length,
			VM_INHERIT_NONE);

    phys = trunc_page(phys);
    
    while (length > 0) {
	pmap_enter_cache_spec(
			    map->pmap,
			    vaddr,
			    phys,
			    VM_PROT_READ | VM_PROT_WRITE,
			    TRUE,
			    caching);
		
	vaddr += PAGE_SIZE; length -= PAGE_SIZE; phys += PAGE_SIZE;
    }
    
    return (IO_R_SUCCESS);
}

IOReturn
kern_IOProbeDriver(host_priv_t device_master, 
	unsigned char *configData, 
	unsigned int count)
{
  	struct probeDriverArgs	args;
	NXConditionLock		*loadDeviceLock;
	char			*configString;

	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	configString = IOMalloc(count + 1);
	if (configString == NULL)
	    return IO_R_NO_SPACE;
	bcopy(configData, configString, count);
	configString[count] = '\0';

	/*
	 *  We need to configure the driver from the IOTask context, so...
	 */

	/*
	 * Set up a lock so we know when autoconf is complete.
	 */
	loadDeviceLock = [NXConditionLock alloc];
	[loadDeviceLock initWith:NO];

	args.waitLock = loadDeviceLock;
	args.configData = configString;
	IOForkThread((IOThreadFunc) configureThread, (void *) &args);
	
	[loadDeviceLock lockWhen:YES];
	[loadDeviceLock free];
	IOFree(configString, count + 1);

	return (args.rtn ? IO_R_SUCCESS : IO_R_NO_DEVICE);
}

extern const char *findBootConfigString(int n);

IOReturn kern_IOGetDriverConfig(host_t device_master,
	unsigned driverNum,
	unsigned maxDataSize,
	IOConfigData configData,
	unsigned int *configDataCnt)
{
	int configLength;
	const char *configString;
	
	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	if(maxDataSize > (IO_CONFIG_TABLE_SIZE - 1)) {
		maxDataSize = IO_CONFIG_TABLE_SIZE - 1;
	}
	configString = findBootConfigString(driverNum);
	if (configString == NULL) {
	    return IO_R_NO_DEVICE;
	}
	configLength = strlen(configString);
	if(maxDataSize > configLength) {
		maxDataSize = configLength;
	}
	bcopy(configString, configData, maxDataSize);
	configData[maxDataSize] = '\0';
	*configDataCnt = maxDataSize + 1;
	return IO_R_SUCCESS;
}

IOReturn kern_IOGetSystemConfig(host_t device_master,
	unsigned maxDataSize,
	IOConfigData configData,
	unsigned int *configDataCnt)
{
	return kern_IOGetDriverConfig(
	    device_master,
	    0,
	    maxDataSize,
	    configData,
	    configDataCnt);
}

IOReturn kern_IOUnloadDriver(host_t device_master,
	IOConfigData configData,
	unsigned int configDataSize)
{
	const char	*className;
	IOConfigTable	*configTable = nil;
	Class		theClass;

	if (device_master == HOST_NULL)
		return IO_R_PRIVILEGE;

	configTable = [IOConfigTable newForConfigData:configData];
	className = [configTable valueForStringKey:"Driver Name"];
	theClass = objc_getClass(className);
	if (theClass == nil) {
		IOLog("IOUnloadDriver: Couldn't find class named %s\n", 
			className);
		if(configTable)
			[configTable free];
		return IO_R_INVALID_ARG;
	}
	
	[theClass unregisterClass: theClass];
	if (configTable)
		[configTable free];
	// FIXME: should we do [theClass free] here?
	return IO_R_SUCCESS;
}

#if defined(hppa) || defined(sparc) /* [*/

static int mapfun(
	dev_t		dev,
	vm_offset_t	addr,
	int 		prot
)
{
	vm_offset_t phys = pmap_extract(kernel_pmap, addr);
	return(atop(phys));
}

IOReturn kern_IOMapLockShmem(
	KernDevice  *device,
	task_t       task,
	vm_size_t    size,
	vm_offset_t *shmem)
{
    vm_object_t vmobject;
    vm_map_t    task_map;

    task_map = task->map;
    if (task_map == VM_MAP_NULL)
	return(IO_R_VM_FAILURE);
    size = round_page(size);
    
    /* Create a fake object to "hold" these pages for user mapping. */
    vmobject = (vm_object_t)vm_object_special(0, mapfun, 0, *shmem, size);

    /*
    ** Find some memory of the same size in 'task'.  We use vm_map_find()
    ** to do this. vm_map_find inserts the found memory object in the
    ** target task's map as a side effect.  Ensure that the addresses
    ** are equivalently mapped to avoid aliasing problems.
    */
    if (vm_map_find(task_map, vmobject, 0, shmem, size, FALSE))
    { // OK, can't get that, so let it find memory 
	if (vm_map_find(task_map, vmobject, 0, shmem, size, TRUE))
	    return(IO_R_NO_MEMORY);
    }

    return(IO_R_SUCCESS); 
}

IOReturn kern_IOUnMapLockShmem(
	KernDevice  *device,
	task_t       task,
	vm_size_t    size,
	vm_offset_t *shmem)
{
    vm_offset_t off;
    kern_return_t krtn;
    vm_map_t           task_map;

    task_map = task->map;
    if (task_map == VM_MAP_NULL)
	return(IO_R_VM_FAILURE);
    size = round_page(size);

    // Pull the shared pages out of the task map
    for ( off = 0; off < size; off += PAGE_SIZE )
    {
	pmap_remove(task_map->pmap, *shmem + off, *shmem + off + PAGE_SIZE);
    }
    // Free the former shmem area in the task
    krtn = vm_map_remove(task_map, *shmem, *shmem + size );
    if (krtn)
    {
	IOLog("IOUnMapLockShmem: vm_map_remove() returned %d\n", krtn);
    }
   
    // This deallocate is commented out because it caused some strange
    // kernel hang -- Should be looked into some time.
    // Possibly the ref_cnt is not properly incremented.
//    vm_map_deallocate(task_map);	// discard our reference
    return(krtn);
}
#endif /*]*/

IOReturn
kern_IOMapDeviceMemory(
    KernDevice		*device,
    task_t 		task,
    vm_offset_t 	phys,
    vm_size_t		length,
    vm_offset_t 	*addr,				// in/out
    BOOL 		anywhere,
    IOCache 	cache
)
{
    IOReturn	rtn;

    if (device == nil)
	return IO_R_PRIVILEGE;
    
    rtn = kern_dev_map_phys(
			    device,
			    task->map,
			    phys,
			    length,
			    addr,
			    anywhere,
			    cache);
    
    return (rtn);
}

#if hppa /* [ */
IOReturn
kern_IOGetDeviceConfig(
    KernDevice			*device,
    IOHPPAInterruptList		interrupts,
    unsigned int		*interruptNum,
    IOHPPAMemoryMap		memoryRegions,
    unsigned int		*memoryRegionNum
)
{
    int			i, num;
    Range		range;
    id			list, deviceDescription;

    if (device == nil)
	return IO_R_PRIVILEGE;
	
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:IRQ_LEVELS_KEY];
    num = [list count];
    if (num > IO_NUM_HPPA_INTERRUPTS)
    	num = IO_NUM_HPPA_INTERRUPTS;
    for (i = 0; i < num; i++)
    	interrupts[i] = [[list objectAt:i] item];
    *interruptNum = num;

    list = [deviceDescription resourcesForKey:MEM_MAPS_KEY];
    num = [list count];
    if (num > IO_NUM_HPPA_MEMORY_RANGES)
    	num = IO_NUM_HPPA_MEMORY_RANGES;
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	memoryRegions[i].start = range.base;
	memoryRegions[i].size = range.length;
    }
    *memoryRegionNum = num;
    
    return IO_R_SUCCESS;
}
#endif /* ] */

#if sparc /*[*/

IOReturn
kern_IOMapSparcDeviceMemory(
    KernDevice		*device,
    task_t 		task,
    vm_offset_t 	phys,
    vm_size_t		length,
    vm_offset_t 	*addr,				// in/out
    BOOL 		anywhere,
    IOCache 	cache
)
{
    IOReturn	rtn;
    id			list, deviceDescription;
    kern_return_t	result;
    vm_offset_t		vaddr;
	vm_map_t 	map;
    Range		range;
    int			i, num;
    cache_spec_t	caching;
	unsigned int paddr,spaceid;
    
     if (device == nil)
	return IO_R_PRIVILEGE;
  	map = task->map;
    if (anywhere)
    	*addr = vm_map_min(map);
    else
    	*addr = trunc_page(*addr);
#if 0	
// needed only for intel
    if (map == kernel_map) {
	if (!anywhere &&
	    *addr < 1*1024*1024)	// In the hole... (between 640K and 1M)
	    return (IO_R_SUCCESS);
    }
#endif
    result = vm_map_find(
    			map,
			VM_OBJECT_NULL, (vm_offset_t) 0,
			addr, length,
			anywhere);

    if (result != KERN_SUCCESS)
    	return (IO_R_NO_SPACE);
    vaddr = *addr;
    vaddr = trunc_page(vaddr);
    length = round_page(length);
	
    (void) vm_map_inherit(
    			map,
    			vaddr, vaddr + length,
			VM_INHERIT_NONE);
	// Mapping the frame buffer in user context
	spaceid = (phys ) & 0x0f;	// 4bits space id which is packed in bits 24-31;
	paddr = ((unsigned int)(phys & 0xffffff00));
	pmap_enter_range( map->pmap,
                          vaddr, paddr,
                          spaceid, length,
                          VM_PROT_READ | VM_PROT_WRITE, FALSE, TRUE );
    return (IO_R_SUCCESS);


}

IOReturn
kern_IOGetDeviceConfig(
    KernDevice			*device,
    IOSPARCInterruptList		interrupts,
    unsigned int		*interruptNum,
    IOSPARCMemoryMap		memoryRegions,
    unsigned int		*memoryRegionNum
)
{
    int			i, num;
    Range		range;
    id			list, deviceDescription;

    if (device == nil)
	return IO_R_PRIVILEGE;
	
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:IRQ_LEVELS_KEY];
    num = [list count];
    if (num > IO_NUM_SPARC_INTERRUPTS)
    	num = IO_NUM_SPARC_INTERRUPTS;
    for (i = 0; i < num; i++)
    	interrupts[i] = [[list objectAt:i] item];
    *interruptNum = num;

    list = [deviceDescription resourcesForKey:MEM_MAPS_KEY];
    num = [list count];
    if (num > IO_NUM_SPARC_MEMORY_RANGES)
    	num = IO_NUM_SPARC_MEMORY_RANGES;
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	memoryRegions[i].start = range.base;
	memoryRegions[i].size = range.length;
    }
    *memoryRegionNum = num;
    
    return IO_R_SUCCESS;
}


#endif /*]*/

#if ppc /* [ */
IOReturn
kern_IOGetDeviceConfig(
    KernDevice			*device,
    IOPPCInterruptList		interrupts,
    unsigned int		*interruptNum,
#ifdef notyet 
    IOPPCDMAChannelList	dmaChannels,
    unsigned int		*dmaChannelNum,
#endif
    IOPPCMemoryMap		memoryRegions,
    unsigned int		*memoryRegionNum
)
{
    int			i, num;
    Range		range;
    id			list, deviceDescription;

    if (device == nil)
	return IO_R_PRIVILEGE;
	
    deviceDescription = [device deviceDescription];
    
    list = [deviceDescription resourcesForKey:IRQ_LEVELS_KEY];
    num = [list count];
    if (num > IO_NUM_PPC_INTERRUPTS)
    	num = IO_NUM_PPC_INTERRUPTS;
    for (i = 0; i < num; i++)
    	interrupts[i] = [[list objectAt:i] item];
    *interruptNum = num;

#ifdef notyet 
   list = [deviceDescription resourcesForKey:DMA_CHANNELS_KEY];
    num = [list count];
    if (num > IO_NUM_PPC_DMA_CHANNELS)
    	num = IO_NUM_PPC_DMA_CHANNELS;
    for (i = 0; i < num; i++)
    	dmaChannels[i] = [[list objectAt:i] item];
    *dmaChannelNum = num;
#endif
    list = [deviceDescription resourcesForKey:MEM_MAPS_KEY];
    num = [list count];
    if (num > IO_NUM_PPC_MEMORY_RANGES)
    	num = IO_NUM_PPC_MEMORY_RANGES;
    for (i = 0; i < num; i++) {
    	range = [[list objectAt:i] range];
	memoryRegions[i].start = range.base;
	memoryRegions[i].size = range.length;
    }
    *memoryRegionNum = num;


    
    return IO_R_SUCCESS;
}
#endif /* ] */
