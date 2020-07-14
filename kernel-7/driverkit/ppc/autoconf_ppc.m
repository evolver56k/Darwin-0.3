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
 * PPC Driverkit configuration.
 *
 * HISTORY
 *
 * 10 Dec 1998 Adam Wang
 *  Added USB matching
 * 27 Oct 1997 Simon Douglas
 *	Added support for tree probe, name matching.
 * 28 June 1994
 *	Derived from i386 version.
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

#import <machkit/NXLock.h>
#import <objc/List.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/KernStringList.h>
#import <driverkit/ppc/IOPPCDeviceDescription.h>
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
#import <driverkit/ppc/PPCKernBus.h>
#import <driverkit/ppc/PPCKernBusPrivate.h>

#import <machdep/ppc/kernBootStruct.h>
#import <machdep/ppc/DeviceTree.h>
#import <driverkit/ppc/IOTreeDevice.h>

/* Used for machine determination */
#import <machdep/ppc/powermac.h>
#import <machdep/ppc/powermac_gestalt.h>
#import <string.h>

// config tables passed by booter in /AAPL,loadables dir
static char ** allConfigTables;
static int numConfigTables;

#define BUS_LOAD_PRI_PROP	"\"Load Priority\" = \"10000\";"
// for debugging
#define ENET_LOAD_PRI_PROP	"\"Load Priority\" = \"7000\";"
#define VIA_LOAD_PRI_PROP	"\"Load Priority\" = \"6800\";"
// slightly higher than default so they come before any PCI SCSI
#define INT_IDE_LOAD_PRI_PROP   "\"Load Priority\" = \"5050\";"
#define INT_SCSI_LOAD_PRI_PROP	"\"Load Priority\" = \"5100\";"
#define EXT_SCSI_LOAD_PRI_PROP	"\"Load Priority\" = \"5200\";"
#define DISPLAY_LOAD_PRI	6000
#define DEFAULT_LOAD_PRI	5000
#define USBCMD_LOAD_PRI_PROP    "\"Load Priority\" = \"4000\";"

static const char * inKernelConfigTables =

"\"Bus Class\" = \"PPCKernBus\";"
"\"Class Names\" = \"\";"
"\"Family\" = \"Bus\";"
"\"Instance\" = \"0\";"
"\"Version\" = \"5.0\";"
"\0"
#if 		MK_HASDRIVERS
"\"Driver Name\" = \"BMacEnet\";"
"\"Matching\" = \"bmac bmac+\";"
ENET_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"MaceEnet\";"
"\"Matching\" = \"mace\";"
ENET_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"DECchip21041\";"
"\"Matching\" = \"pci1011,14\";"
"\"Share IRQ Levels\" = \"YES\";"
"\"Network Interface\" = \"AUTO\";"
ENET_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"Apple96_SCSI\";"
"\"Matching\" = \"53c94\";"
"\"Bus Type\" = \"PPC\";"		// why do they look for it?
EXT_SCSI_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"AppleMesh_SCSI\";"
"\"Matching\" = \"mesh\";"
"\"Bus Type\" = \"PPC\";"		// why do they look for it?
INT_SCSI_LOAD_PRI_PROP
"\0"
"\"Family\" = \"Disk\";"
"\"Driver Name\" = \"EIDE\";"
"\"Matching\" = \"ide ata ATA pci1095,646\";"
"\"Class Names\" = \"IdeController AtapiController\";"
"\"Block Major\" = \"3\";"
"\"Character Major\" = \"15\";"
INT_IDE_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"ApplePMU\";"
"\"Matching\" = \"via-pmu\";"
VIA_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"AppleCuda\";"
"\"Matching\" = \"via-cuda\";"
VIA_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"AppleUSBCMD\";"
"\"Matching\" = \"pci1095,670\";"
USBCMD_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"AppleUSBCMD\";"
"\"Matching\" = \"pci11c1,5801\";"
USBCMD_LOAD_PRI_PROP   
"\0"
"\"Driver Name\" = \"AppleUSBCMD\";"
"\"Matching\" = \"pciclass,0c0310\";"
USBCMD_LOAD_PRI_PROP  
"\0"
"\"Driver Name\" = \"AppleUSBCMD\";"
"\"Matching\" = \"usb\";"
USBCMD_LOAD_PRI_PROP  
"\0"
"\"Driver Name\" = \"AppleOHare\";"
"\"Matching\" = \"pci106b,7\";"
BUS_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"Symbios8xx\";"
"\"Matching\" = \"apple53C8xx Apple53C875Card\";"
"\"Class Names\" = \"Sym8xxController\";"
"\"Bus Type\" = \"PPC\";"
"\0"
"\"Driver Name\" = \"AdaptecU2SCSI\";"
"\"Matching\" = \"ADPT,2940U2B ADPT,3950U2B ADPT,2930CU ADPT,2930U\";"
"\"Class Names\" = \"AdaptecU2SCSI\";"
"\"Bus Type\" = \"PPC\";"
INT_SCSI_LOAD_PRI_PROP
"\0"
#endif		/* MK_HASDRIVERS */

#if		DK_HASDRIVERS
"\"Driver Name\" = \"IOMacRiscPCIBridge\";"
"\"Matching\" = \"bandit uni-north\";"
BUS_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"IOMacRiscVCIBridge\";"
"\"Matching\" = \"chaos\";"
BUS_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"IOGracklePCIBridge\";"
"\"Matching\" = \"grackle\";"
BUS_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"IOPCIBridge\";"
"\"Matching\" = \"pci-bridge\";"
BUS_LOAD_PRI_PROP
"\0"
"\"Driver Name\" = \"IONDRVFramebuffer\";"		/* shim for ndrv's */
"\"Matching\" = \"display\";"				/* matches device_type */
"\"Load Priority\" = \"6000\";"
"\0"
#endif		/* DK_HASDRIVERS */
"\"Driver Name\" = \"IODeviceTreeBus\";"
"\"Matching\" = \"cpu cpus l2-cache\";"		   	/* matches device_type */
"\0"
/************ must be last ************/
"\"Driver Name\" = \"IODeviceTreeBus\";"
"\"Matching\" = \"device-tree\";"			/* matches any tree "bus" node */
BUS_LOAD_PRI_PROP
"\0"
"\0"
;

void DeviceTreeProbe( void );
static int
MatchDriversForDevice( id ioDevice );
static char *
FindStringKey( const char * config, const char * key, char * value );
IOReturn
PublishDevice( id ioDevice );
static BOOL
StartDriver( IOConfigTable * configTable, id ioDeviceDescription );
BOOL
NewLoadDriver( const char * configData );

static void probe_indirect(const char **indirectNamep, id driverInstance);
const char *findBootConfigString(int n);
static void bootDriverInit(void);

id		defaultBus, defaultBusClass;
id		autoConfigTables;

/*
 * Native indirect driver classes.
 */
char *indirectDevList[] = {
        "ATADisk",
	"SCSIDisk",
	"SCSIGeneric",
	"IODiskPartition",
	"IOADBBus",
	"PPCKeyboard",
	"PPCMouse",
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
#define BUS_CLASS_KEY		"Bus Class"
#define BUS_TYPE_KEY		"Bus Type"
#define FAMILY_KEY		"Family"
#define BUS_FAMILY		"Bus"
#define KERN_BUS_FORMAT		"%sKernBus"
#define DEVICE_DESCR_FORMAT	"IO%sDeviceDescription"
#define DEFAULT_BUS		"PPC"
#define NAME_BUF_LEN		128


static BOOL
versionIsOK(const char *serverName)
{
    int version;

	return YES;    
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

#define BUILTIN_BUS_CLASS "PPCKernBus"	// XXX

/*
 * Eventually, this routine will only initialize the boot device(s) per 
 * input from the booter via KERNBOOTSTRUCT. 
 * For now, configure all native devices enumerated in the various
 * tables in device_configuration.c.
 */

void
probeNativeDevices(void)
{
	int		i;
	const char	*config;
	IOConfigTable	*configTable = nil;
	const char	*family, *busName;
	id		busClass;
	BOOL		gotBuiltinBus = NO;
	
	autoConfigTables = [[List alloc] init];

	//
	// Initialize drivers and modules loaded by booter.

	bootDriverInit();

	//
	// Find all bus drivers.
	//

	for(i=1; (config = findBootConfigString(i)); i++) {

		configTable = [IOConfigTable newForConfigData:config];
		family = [configTable valueForStringKey:FAMILY_KEY];
		if (family && strcmp(family, BUS_FAMILY) == 0) {
		    busName = [configTable valueForStringKey:BUS_CLASS_KEY];
		    if (!busName) {
			printf("autoconf: missing \"%s\" key\n",
			       BUS_CLASS_KEY);
			continue;
		    }
		    if (strcmp(busName, BUILTIN_BUS_CLASS) == 0) {
			gotBuiltinBus = YES;
		    }
		    busClass = objc_getClass(busName);
		    (void)[busClass probeBus:configTable];
		    [configTable freeString:busName];
		    [configTable freeString:family];
		} else {
		    if (family)
			[configTable freeString:family];
		    [configTable free];
		}
	}

	if (gotBuiltinBus == NO) {	// XXX
		busClass = objc_getClass(BUILTIN_BUS_CLASS);
		(void)[busClass probeBus:nil];
	}
	
	defaultBusClass = [KernBus lookupBusClassWithName:DEFAULT_BUS];	// XXX
	if (defaultBusClass == nil) {
		panic("Missing default kernel bus class");
	}
	defaultBus = [KernBus lookupBusInstanceWithName:DEFAULT_BUS busId:0];

	/*
	 */
        for( i = 1; (config = findBootConfigString(i)); i++) {
            NewLoadDriver( config );
	}

	/*
	 * Probe the device tree
	 */
	DeviceTreeProbe();

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
StartDriver( IOConfigTable * configTable, id ioDeviceDescription )
{
	const char	*className, *classNames;
	KernStringList  *classNameList = nil;
	const char	*serverName = NULL;
	Class		theClass;
	id		deviceDescription = nil;
	id		device = nil;
	id		newDescription;
	int		index, classesLoaded;
	BOOL		versionChecked = NO;
	const char	*placeholder;
	const char	*busType, *busTypeString;
	char		*nameBuf;
	id		busClass, busDescriptionClass;


	nameBuf = (char *)IOMalloc(NAME_BUF_LEN);

	serverName = [configTable valueForStringKey:SERVER_NAME_KEY];

	/*
	 * Check for a placeholder config table
	 * that represents a dynamically configured device.
	 * NOTE: this skips the driverkit version check. XXX
	 */
	if ((placeholder = [configTable valueForStringKey:"Dynamic"])) {
	    if (*placeholder == 'Y' || *placeholder == 'y') {
		[autoConfigTables addObject:configTable];
		[configTable freeString:placeholder];
		return YES;
	    } else {
		[configTable freeString:placeholder];
	    }
	}
	
        classNames = (char *)[configTable valueForStringKey:CLASS_NAMES_KEY];
        if (classNames == NULL) {
            classNames = (char *)[configTable
                                    valueForStringKey:DRIVER_NAME_KEY];
            if (classNames == NULL)
                    goto abort;
        }
	classNameList = [[KernStringList alloc]
			    initWithWhitespaceDelimitedString:classNames];
	[configTable freeString:classNames];

	busTypeString = [configTable valueForStringKey:BUS_TYPE_KEY];
	if (busTypeString == NULL || *busTypeString == '\0')
	    busType = DEFAULT_BUS;
	else
	    busType = busTypeString;

	sprintf(nameBuf, KERN_BUS_FORMAT, busType);
	busClass = objc_getClass((const char *)nameBuf);
	if (busClass == nil)
	    busClass = defaultBusClass;

	classesLoaded = 0;
	for (index = 0; index < [classNameList count]; index++) {
	    className = [classNameList stringAt:index];
	    theClass = objc_getClass(className);

	    if (theClass == nil) {
		    IOLog("configureDriver: "
		          "driver class '%s' was not loaded\n", className);
		    if( serverName)
			IOLog("Driver %s could not be configured\n", serverName);
		    goto abort;
	    }

#if DEBUG
	    if( ioDeviceDescription)
		kprintf("Probing class %s on device %s\n",
		    className, [ioDeviceDescription nodeName]);
#endif
	    
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
	     *  an DeviceDescription or an PPCDeviceDescription.
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

                    if( nil == ioDeviceDescription) {		
                        sprintf(nameBuf, "IO%sDeviceDescription", busType);
                        busDescriptionClass = objc_getClass((const char *)nameBuf);
                        newDescription = [[busDescriptionClass alloc]
                            _initWithDelegate:deviceDescription];
                        if (newDescription == nil)
                                    goto abort;
                    } else {
			newDescription = ioDeviceDescription;
                        [newDescription setDelegate:deviceDescription];
		    }
		    [newDescription setDevicePort:create_dev_port(device)];
		    break;
    
		case IO_IndirectDevice:
		case IO_PseudoDevice:
		
		    deviceDescription = [[KernDeviceDescription alloc]
					    initFromConfigTable:configTable];
		    if (deviceDescription == nil) {
			    goto abort;
		    }
		    
		    newDescription = [[IODeviceDescription alloc]
		    	_initWithDelegate:deviceDescription];
			
		    if (newDescription == nil)
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
    
	    if( [IODevice addLoadedClass:theClass 
			    description:newDescription] == IO_R_SUCCESS) {
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
	if (classesLoaded) {
	    return YES;
	} else {
	    [deviceDescription free];
	    [device free];
	    return NO;
	}

abort:

	if (serverName)
		[configTable freeString:serverName];
	if (busTypeString)
		[configTable freeString:busTypeString];

	if (deviceDescription) {
		[deviceDescription free];
	}
	if (device) {
		[device free];
	}

	return NO;
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

IOReturn
PublishDevice( id ioDevice )
{
    struct PendingProbe {
            struct PendingProbe   *	next;
            struct PendingProbe   *	prev;
	    int				priority;
            id				device;
    };
    struct PendingProbeQueue {
            struct PendingProbe   *	head;
            struct PendingProbe   *	zero;
            struct PendingProbe   *	tail;
    };
    int		    			priority;
    struct PendingProbe *		newProbe;
    struct PendingProbe *		elem;
    static struct PendingProbeQueue 	probeQueue;
    static int				outstanding = 0;
    static	    			probing = 0;

    priority = MatchDriversForDevice( ioDevice);
    if( priority < 0)
	return( IO_R_SUCCESS);		// no match, no probe now

    if( probing) {
        newProbe = IOMalloc( sizeof( struct PendingProbe));
	if( newProbe == NULL)
	    return( IO_R_NO_MEMORY);
        newProbe->device = ioDevice;
        newProbe->priority = priority;

        // queue it behind those with same priority
        for( elem = probeQueue.head;
            ( elem->next && (elem->priority >= priority));
            elem = elem->next )
		{}
	// queue before elem
	newProbe->next = elem;
	newProbe->prev = elem->prev;
	elem->prev->next = newProbe;
	elem->prev = newProbe;
        outstanding++;

    } else {

	probing++;
	probeQueue.head = (struct PendingProbe *) &probeQueue.zero;
	probeQueue.tail = (struct PendingProbe *) &probeQueue.head;

        StartDriver( [ioDevice configTable], ioDevice );

	while( outstanding) {
            // dequeue highest - must be one
	    newProbe = probeQueue.head;
	    probeQueue.head = newProbe->next;
            newProbe->next->prev = &probeQueue.head;

	    outstanding--;
	    ioDevice = newProbe->device;
	    priority = newProbe->priority;
	    IOFree( newProbe, sizeof( struct PendingProbe));

	    // start it
#if DEBUG
            kprintf("---- [%d] Driver probe on device %s\n", priority, [ioDevice nodeName] );
#endif
            StartDriver( [ioDevice configTable], ioDevice );
	}
	probing--;
    }

    return( IO_R_SUCCESS);
}

BOOL
NewLoadDriver( const char * configData )
{
    BOOL		ret = NO;
    id			ioDevice;
    IOConfigTable *	configTable;
    char *		tryMatch;
    char *		tryLoc;
    char *		location;
    enum { 		kStringSize = 128 };
    id *		failedList = nil;
    int			numFailed = 0;
    int			i;

    tryMatch = IOMalloc( kStringSize * 2);
    if( ! tryMatch)
	return( NO);
    tryLoc = tryMatch + kStringSize;

    if( FindStringKey( configData, "\"Location\"", tryLoc) && tryLoc[0] )
        location = tryLoc;
    else
        location = NULL;

    if( FindStringKey( configData, "\"Matching\"", tryMatch)) {

        while( (ioDevice = [IOTreeDevice findMatchingDevice:tryMatch location:location]) ) {
	    // Start the driver on all matching devices
            [ioDevice taken:YES];
            [[ioDevice propertyTable] addConfigData:configData];
#if DEBUG
	    kprintf("---- Driver probe on device %s\n", [ioDevice nodeName] );
#endif
            ret = StartDriver( [ioDevice configTable], ioDevice );

	    if( NO == ret) {
		numFailed++;
		failedList = realloc( failedList, numFailed * sizeof( id));
		if( failedList)
		    failedList[ numFailed - 1 ] = ioDevice;
	    }
        }
	if( failedList) {
            for( i = 0; i < numFailed; i++ )
                [failedList[ i ] taken:NO];
	    free( failedList );
	}

    } else {
	configTable = [IOConfigTable newForConfigData:configData];
        if( configTable) {
            ret = StartDriver( configTable, nil );
            if( ret == NO)
                [configTable free];
        }
    }

    IOFree( tryMatch, kStringSize * 2);
    return( ret);
}

static int
MatchDriversForDevice( id ioDevice )
{
    int				i, propLen;
    void	*		prop;
    int				priority = DEFAULT_LOAD_PRI;
    BOOL			found;
    const char * 		configData;
    IOPropertyTable	*	propTable = [ioDevice propertyTable];
    char *			tryMatch;
    char *			tryLoc;
    char *			location;
    enum { 			kStringSize = 128 };

    tryMatch = IOMalloc( kStringSize * 2);
    if( ! tryMatch)
	return( -1);
    tryLoc = tryMatch + kStringSize;

    found = NO; // NDRVForDevice( ioDevice);

    // look in tree for priority
    propLen = sizeof( priority);
    prop = &priority;
    if( [propTable getProperty:"AAPL,load-priority"
            flags:0 value:&prop length:&propLen] ) {
	// else defaults - different for ndrv's
	if( found)
            priority = DISPLAY_LOAD_PRI;
	else
            priority = DEFAULT_LOAD_PRI;
    }

    // look for a native XXXKit driver...
    for( i = 1; (NO == found) && (configData = findBootConfigString(i)); i++ ) {

        if( FindStringKey( configData, "\"Location\"", tryLoc) && tryLoc[0] )
	    location = tryLoc;
	else
	    location = NULL;

        if( FindStringKey( configData, "\"Matching\"", tryMatch)) {
            if( (found = [ioDevice match:tryMatch location:location])) {

                [propTable addConfigData:configData];
                // look in driver table for priority - overrides tree
                if( FindStringKey( configData, "\"Load Priority\"", tryMatch))
                    priority = strtol( tryMatch, 0, 10);
            }
        }
    }
    if( found)
        [ioDevice taken:YES];

    IOFree( tryMatch, kStringSize * 2);
    return( found ? priority : (-1));
}



/*
 *  Configure a driver based on some config information.  This is really just
 *  a wrapper around configureDriver that runs in its own thread, and needs to
 *  be called in the IOTask context.
 */
void
configureThread(struct probeDriverArgs *args)
{

	args->rtn = NewLoadDriver(args->configData);
	
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
	/* nope */
}

/*
 * Start up all non-native direct drivers. Called after probeHardware(). 
 */
void probeDirectDevices(void)
{
	/* nope */
}

/*
 * Obtain n'th boot config table.
 */
const char *findBootConfigString(int n)
{
    if( n < numConfigTables)
	return( allConfigTables[ n ]);
    else
	return( NULL);
}

static char *
FindStringKey( const char * config, const char * key, char * value )
{
    char  	* match;
    char 	* prop;
    char 	* end;

    do {
        match = strstr( config, key );
        if( match == NULL)
            continue;
        prop = strchr( match + strlen( key ), '\"');
        if( prop == NULL)
            continue;
        prop++;
        end = strchr( prop, '\"');
        if( end == NULL)
            continue;

        strncpy( value, prop, end - prop);
        value[ end - prop ] = 0;
        return( value);

    } while( NO);

    return( NULL);
}


/*
 * Make a list of config tables in the kernel and loaded by the booter.
 * Register objective-C code loaded by the booter.
 */

static void
bootDriverInit(void)
{
    IOReturn		err;
    DTEntry 		rootEntry, dtEntry;
    DTEntryIterator	dtIter;
    int			i, size, allocedTables;
    char	*	prop;
    char	*	bundleName;
    void	**	module;
    char	*	nextTable;

    if( (kSuccess != DTLookupEntry( 0, "/AAPL,loadables", &rootEntry))
    ||  (kSuccess != DTCreateEntryIterator ( rootEntry, &dtIter)))

	dtIter = NULL;

    for( i = 0; i < 2; i++) {
        numConfigTables = 1;		// System assumed
        if( dtIter) while( kSuccess == DTIterateEntries( dtIter, &dtEntry)) {

            // look for reloc on second pass
            if(   allConfigTables
                        && (kSuccess == DTGetProperty( dtEntry, "_reloc", &module, &size ))
                        && (size == sizeof( void *)) ) {

                err = objc_registerModule( *module, 0);
#if DEBUG
                kprintf("Registered objc @ %x\n", *module);
#endif
            }

            if( kSuccess != DTGetProperty( dtEntry, "name", &bundleName, &size ))
                continue;
            if( (kSuccess != DTGetProperty( dtEntry, "Instance0.table", &prop, &size ))
            &&  (kSuccess != DTGetProperty( dtEntry, "Default.table", &prop, &size )) )
                continue;

            if( 0 == strcmp( "System", bundleName)) {
                if( allConfigTables)
                    allConfigTables[ 0 ] = prop;

            } else for(     nextTable = prop;
                            (nextTable - prop) < size;
                            nextTable = nextTable + strlen( nextTable) + 1) {
                if( allConfigTables && (numConfigTables < allocedTables))
                    allConfigTables[ numConfigTables ] = (char *) nextTable;
                numConfigTables++;
            }
        }

        // now the linked in kernel tables
        for(    nextTable = inKernelConfigTables;
                *nextTable;
                nextTable = nextTable + strlen( nextTable) + 1) {
            if( allConfigTables && (numConfigTables < allocedTables))
                allConfigTables[ numConfigTables ] = (char *) nextTable;
            numConfigTables++;
        }

        // allocate for second pass
        if( nil == allConfigTables) {
            allocedTables = numConfigTables;
            allConfigTables = IOMalloc( allocedTables * sizeof( char *));
            if( allConfigTables)
                bzero( allConfigTables, allocedTables * sizeof( char *));
            if( dtIter)
                DTRestartEntryIteration( dtIter);
        }
    }
    if( dtIter)
        DTDisposeEntryIterator( dtIter);
}
