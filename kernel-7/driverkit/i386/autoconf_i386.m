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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * 486 Driverkit configuration.
 *
 * HISTORY
 *
 * 28 Jan 1993  Brian Pinkerton at NeXT
 *	Added support for new, MI autoconf logic, and new driverkit API.
 * 10 Nov 1992	Brian Pinkerton at NeXT
 *	Added indirect probe logic.
 * 20 Aug 1992	Joe Pasqua
 *	Init intrReq & intrAttached of kern_dev in configure_device_class().
 * 3 August 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <kern/mach_header.h>

#import <machkit/NXLock.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/KernStringList.h>
#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/IODeviceKernPrivate.h>
#import <driverkit/IODeviceParams.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/configTableKern.h>
#import <driverkit/KernDevice.h>
#import <driverkit/IOConfigTable.h>
#import <driverkit/driverTypesPrivate.h>
#import <driverkit/autoconfCommon.h>
#import <driverkit/i386/EISAKernBus.h>
#import <driverkit/i386/PCMCIA.h>
#import <driverkit/i386/PCMCIAKernBus.h>
#import <driverkit/i386/PCIKernBus.h>

#import <machdep/i386/kernBootStruct.h>

boolean_t eisa_id(int slot, unsigned int *_id);

static void probe_indirect(const char **indirectNamep, id driverInstance);

static BOOL configureDriver(const char *configData);
const char *findBootConfigString(int n);
static void bootDriverInit(void);

id		defaultBus, defaultBusClass;
static BOOL	printedPCMCIAMessage;

/*
 * Native indirect driver classes.
 */
char *indirectDevList[] = {
	"SCSIDisk",
	"SCSIGeneric",
	"IODiskPartition",
	NULL
};

/*
 * Pseudo device list.
 */
char *pseudoDevList[] = {
	"EventDriver",
	"kmDevice",
	NULL
};

#define SERVER_NAME_KEY		"Server Name"
#define CLASS_NAMES_KEY 	"Class Names"
#define DRIVER_NAME_KEY 	"Driver Name"
#define BUS_TYPE_KEY		"Bus Type"
#define BUS_CLASS_KEY		"Bus Class"
#define FAMILY_KEY		"Family"
#define BUS_FAMILY		"Bus"
#define KERN_BUS_FORMAT		"%sKernBus"
#define DEVICE_DESCR_FORMAT	"IO%sDeviceDescription"
#define DEFAULT_BUS		"EISA"
#define NAME_BUF_LEN		128


BOOL is_ISA;

static BOOL
versionIsOK(const char *serverName)
{
    int version;
    
    /*
     * Right now, if serverName is NULL, that means the driver
     * is compiled into the kernel, so don't check the version.
     * This may change later, however.
     */
    
    if (serverName != NULL) {
	version = [IODevice
		driverKitVersionForDriverNamed:(char *)serverName];
	if (version == -1)
	    version = 310;
	/*
	 * TODO: revisit this policy decision if DriverKit
	 * compatibility changes in the future.
	 */
	if (version <= 310) {
	    IOLog("WARNING: driver %s uses incompatible DriverKit version %d\n", serverName, version);
	    IOLog("Driver %s could not be configured\n", 
		serverName);
	    return NO;
	}
    }
    return YES;
}

/*
 * Eventually, this routine will only initialize the boot device(s) per 
 * input from the booter via KERNBOOTSTRUCT. 
 * For now, configure all native devices enumerated in the various
 * tables in device_configuration.c.
 */
void
probeNativeDevices(void)
{
	int		slot;
	unsigned int	_id;
	char		**configData;
	int		i;
	const char	*config;
	IOConfigTable	*configTable = nil;
	const char	*family, *busName;
	char		*nameBuf;
	id		busClass;
	
	//
	// Initialize drivers and modules loaded by booter.
	
	bootDriverInit();

	nameBuf = (char *)IOMalloc(NAME_BUF_LEN);

	//
	// Find all bus drivers.
	//
	for(i=1; ; i++) {
		config = findBootConfigString(i);
		if(config == NULL) {
			break;
		}
		configTable = [IOConfigTable newForConfigData:config];
		family = [configTable valueForStringKey:FAMILY_KEY];
		if (family && strcmp(family, BUS_FAMILY) == 0) {
		    busName = [configTable valueForStringKey:BUS_CLASS_KEY];
		    busClass = objc_getClass(busName);
		    (void)[busClass probeBus:configTable];
		}
		if (family)
		    [configTable freeString:family];
	}
	
	defaultBusClass = [KernBus lookupBusClassWithName:DEFAULT_BUS];	// XXX
	if (defaultBusClass == nil) {
		sprintf(nameBuf, "Missing %s kernel bus class", DEFAULT_BUS);
		panic(nameBuf);
	}
	defaultBus = [KernBus lookupBusInstanceWithName:DEFAULT_BUS busId:0];
	
	/*
	 * For now this is the only
	 * concession to EISA bus.
	 */
	if (eisa_id(0, &_id)) {
		printf("CPU:\tEISA id %08x\n", _id);
		for (slot = 1; slot <= 0xf; slot++) {
		   	if (eisa_id(slot, &_id))
				printf("slot %x:\tEISA id %08x\n", slot, _id);
		}
		led_msg("NeXT");
		is_ISA = NO;
	}
	else {
	   	printf("ISA bus\n");
   		is_ISA = YES;
 	}


	printf("DriverKit version %d\n",[IODevice driverKitVersion]);

	/*
	 * Configure all devices for which the booter has given us config info.
	 * The zeroth config string is the system config table.
	 */
	for(i=1; ; i++) {
		config = findBootConfigString(i);
		if(config == NULL) {
			break;
		}
	   	configureDriver(config);
	}
	IOFree(nameBuf, NAME_BUF_LEN);
}


@protocol OldProbeMethods
+ probe;
+ probe:(int)a deviceMaster:(port_t) b;
@end

/* 
 * Called from MD probeNativeDevices() and from configureThread(), the
 * IOTask version of IOProbeDriver().
 *
 * Given a class name and an ASCII representation of a config file,
 * create a kern_dev and an IODeviceDescription and probe the class. 
 * This is used both at boot time - for the boot device and (temporarily) 
 * any native drivers with static config tables - and by IOProbeDriver().
 * 
 * Returns YES if driver was started successfully, else returns NO.
*/
static BOOL
configureDriver(
    const char *configData
)
{
	const char	*className, *classNames;
	KernStringList  *classNameList = nil;
	const char	*serverName = NULL;
	IOConfigTable	*configTable = nil;
	Class		theClass;
	id		deviceDescription = nil;
	id		device = nil;
	id		ioDeviceDescription = nil;
	IOReturn	drtn;
	int		index, classesLoaded, version;
	BOOL		versionChecked = NO;
	const char	*busType, *busTypeString;
	char		*nameBuf;
	id		busClass, busDescriptionClass;

	nameBuf = (char *)IOMalloc(NAME_BUF_LEN);
	
	configTable = [IOConfigTable newForConfigData:configData];

	serverName = [configTable valueForStringKey:SERVER_NAME_KEY];
	
	classNames = (char *)[configTable valueForStringKey:CLASS_NAMES_KEY];
	if (classNames == NULL)
	    classNames = (char *)[configTable 
				     valueForStringKey:DRIVER_NAME_KEY];
	classNameList = [[KernStringList alloc]
			    initWithWhitespaceDelimitedString:classNames];
	IOFree(classNames, strlen(classNames) + 1);
	busTypeString = [configTable valueForStringKey:BUS_TYPE_KEY];
	if (busTypeString == NULL || *busTypeString == '\0')
	    busType = DEFAULT_BUS;
	else
	    busType = busTypeString;

	sprintf(nameBuf, KERN_BUS_FORMAT, busType);
	busClass = objc_getClass((const char *)nameBuf);
	if (busClass == nil)	{
	    busClass = defaultBusClass;
	    busType = DEFAULT_BUS;
	}

	classesLoaded = 0;
	for (index = 0; index < [classNameList count]; index++) {
	    className = [classNameList stringAt:index];
	    theClass = objc_getClass(className);

	    if (theClass == nil) {
		    IOLog("configureDriver: "
		          "driver class '%s' was not loaded\n", className);
		    IOLog("Driver %s could not be configured\n", serverName);
		    goto abort;
	    }
	    
	    if (versionChecked == NO) {
		/*
		 * Right now, if serverName is NULL, that means the driver
		 * is compiled into the kernel, so don't check the version.
		 * This may change later, however.
		 */
    
		if (serverName != NULL) {
		    if (versionIsOK(serverName) == NO)
			goto abort;
		}
		versionChecked = YES;
	    }

	    /*
	     * Check to see if bus wants to configure driver itself.
	     */
	    if ([busClass configureDriverWithTable:configTable]) {
		classesLoaded = 1;
		/* Return success. */
		break;
	    }
	    
	    /*
	     *  Depending on the deviceStyle, we either make
	     *  an DeviceDescription or the bus type device description.
	     */
	    switch ([theClass deviceStyle]) {
		case IO_DirectDevice:
		    deviceDescription = [busClass
			deviceDescriptionFromConfigTable:configTable];	
    
		    if (deviceDescription == nil) {
			    IOLog("configureDriver: initFromConfigTable "
					"failed for class %s\n", className);
				goto abort;
		    }
		    
		    if ([deviceDescription bus] == nil) {
			[deviceDescription setBus:defaultBus];
		    }
		    
		    if ([[deviceDescription bus] allocateResourcesForDeviceDescription:
				    deviceDescription] == nil) {
			    IOLog("configureDriver: could not allocate "
				  "resources for class %s\n", className);
				goto abort;
		    }

		    device = [[KernDevice alloc]
		    	initWithDeviceDescription:deviceDescription];
    
		    if (device == nil) {
			    IOLog("configureDriver: initFromDeviceDescription "
					"failed for class %s\n", className);
				goto abort;
		    }
		    
		    [deviceDescription setDevice:device];
		    
		    sprintf(nameBuf, "IO%sDeviceDescription", busType);
		    busDescriptionClass = objc_getClass((const char *)nameBuf);

		    ioDeviceDescription = [[busDescriptionClass alloc]
			_initWithDelegate:deviceDescription];
			
		    if (ioDeviceDescription == nil) {
		    		goto abort;
		    }
				
		    [ioDeviceDescription 
		    		setDevicePort:create_dev_port(device)];
		    break;
    
		case IO_IndirectDevice:
		case IO_PseudoDevice:
		    deviceDescription = [[KernDeviceDescription alloc]
					    initFromConfigTable:configTable];
		    if (deviceDescription == nil) {
			    goto abort;
		    }
		    
		    ioDeviceDescription = [[IODeviceDescription alloc]
		    	_initWithDelegate:deviceDescription];
			
		    if (ioDeviceDescription == nil)
		    		goto abort;

		    break;

		default:
		    IOLog("Invalid style for class %s", className);
		    goto abort;
	    }
    
	    /*
	     *  Now that we have a valid device description for the device,
	     *  probe the sucker.
	     */
	    if (![theClass respondsTo:@selector(probe:)]) {
		    IOLog("configureDriver: Class %s does not respond "
		          "to probe:\n", className);
		    continue;
	    }
    
	    if (
		[IODevice addLoadedClass:theClass 
			    description:ioDeviceDescription] == IO_R_SUCCESS) {
		    classesLoaded++;
	    }
	}

	/*
	 * Only free resources if there was a fatal error before
	 * addLoadedClass: was called, because the class may cache
	 * the device description and config table for later use.
	 */
	IOFree(nameBuf, NAME_BUF_LEN);
	[classNameList free];
	if (serverName)
	    [configTable freeString:serverName];
	if (busTypeString)
	    [configTable freeString:busTypeString];
	if (classesLoaded)
	    return YES;
	else {
	    [configTable free];
	    [ioDeviceDescription free];
	    [deviceDescription free];
	    [device free];
	    return NO;
	}

abort:
	if (serverName)
		[configTable freeString:serverName];
	if (busTypeString)
		[configTable freeString:busTypeString];
	if (configTable)
		[configTable free];

	if (ioDeviceDescription) {
		[ioDeviceDescription free];
	}
	if (deviceDescription) {
		[deviceDescription free];
	}
	if (device) {
		[device free];
	}

	return NO;
}


static boolean_t	isEISA=FALSE;

boolean_t
eisa_present(
    void
)
{
    static boolean_t	checked;

    if (!checked) {
	if (strncmp((char *)0xfffd9, "EISA", 4) == 0)
	    isEISA = TRUE;
	    
	checked = TRUE;
    }
    
    return (isEISA);
}

boolean_t
eisa_id(
    int			slot,
    unsigned int	*_id
)
{
    unsigned char	*ids = (unsigned char *)_id;
    IOEISAPortAddress		port;
    
    if (!eisa_present())
	return (FALSE);
    
    port = (slot << 12) | 0x0C80;
    
    ids[3] = inb(port);
    ids[2] = inb(++port);
    ids[1] = inb(++port);
    ids[0] = inb(++port);
    
    if (*_id == 0xffffffff)
    	return (FALSE);
	
    return (TRUE);
}


/*
 * probe all indirect drivers associated with specified driver.
 * As of mk-149.10, this is only used for IdeDisk.
 */
static void
probe_indirect(const char **indirectNamep, id driverInstance)
{
	id driverClass;
	
	if(indirectNamep == NULL) {
		return;
	}
	for(;  *indirectNamep; indirectNamep++) {
		driverClass = objc_getClass(*indirectNamep);
		if(driverClass == nil) {
			printf("probe_indirect: Class %s does not exist\n",
				*indirectNamep);
			continue;
		}
		if(![driverClass respondsTo:@selector(probe:)]) {
			printf("probe_indirect: Class %s does not respond to"
				" probe:\n", *indirectNamep);
			continue;
		}

		[driverClass probe:driverInstance]; 
	}
}

/*
 *  Configure a driver based on some config information.  This is really just
 *  a wrapper around configureDriver that runs in its own thread, and needs to
 *  be called in the IOTask context.
 */
void
configureThread(struct probeDriverArgs *args)
{
	args->rtn = configureDriver(args->configData);
	
	/*
	 * Notify parent that we're finished, then terminate.
	 */
	[args->waitLock lock];
	[args->waitLock unlockWith:YES];
	IOExitThread();
}

/*
 * Perform machine dependent hardware probe/config.
 */
void probeHardware(void)
{
}

/*
 * Start up all non-native direct drivers. Called after probeHardware(). 
 */
void probeDirectDevices(void)
{
	/* nope */
}

/*
 * Obtain n'th string in KERNBOOSTRUCT.config[]. Returns pointer to desired
 * string if found, else returns NULL.
 * KERNBOOTSTRUCT.config is a set of contiguous strings, NULL separated. 
 * The end of the list is delineated by a double NULL.
 */
const char *findBootConfigString(int n)
{
	KERNBOOTSTRUCT *kernBootStruct = KERNSTRUCT_ADDR;
	int stringsFound;
	const char *cp = kernBootStruct->config;
	int currentIndex = 0;
	int length;
	
	if(*cp == '\0') {
		IOLog("WARNING: No config table in KERNBOOTSTRUCT!\n");
		return NULL;
	}
	for(stringsFound=0; stringsFound<n; stringsFound++) {
		length = strlen(cp);
		currentIndex += (length + 1);
		cp += (length + 1);
		if( (length == 0) || 
		    (currentIndex > CONFIG_SIZE) ||
		    (*cp == '\0')) {
			/*
			 * Explicit end of list or beyond boundary.
			 */
			return NULL;
		}
	}
	return cp;
}


static void
bootDriverInit(void)
{
    KERNBOOTSTRUCT *bootstruct = KERNSTRUCT_ADDR;
    int i;
    struct section *sp;
    struct mach_header *hdr;
    
    for (i = 0; i < bootstruct->numBootDrivers; i++) {
	hdr = (struct mach_header *)bootstruct->driverConfig[i].address;
    	/* Zero out bss */
	sp = getsectbynamefromheader(hdr,"__DATA","__bss");
	if (sp)
	    bzero(sp->addr, sp->size);
	(void)objc_registerModule(hdr, 0);
    }
}
