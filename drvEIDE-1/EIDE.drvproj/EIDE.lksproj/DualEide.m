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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * DualEide.m - Dual EIDE controller initialization. 
 *
 * HISTORY
 *
 * 1-Feb-1998	Joe Liu at Apple
 *		Tweaked code to support PIIX which uses the DualEIDE but with a PCI
 *		bus. Not sure if this change will work under 3.3.
 *
 * 18-Aug-1996 	Becky Divinski at NeXT
 *      Created. 
 */

#import <sys/systm.h>

#import <driverkit/KernBus.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/IODeviceDescription.h>
#import <driverkit/IODeviceDescriptionPrivate.h>

#import "DualEide.h"
#import "IdeCnt.h"
#import "IdeDisk.h"
#import "AtapiCnt.h"

/* Some mach stuff to create a mach port, kernel/driverkit/driverServerXXX.m */
extern port_t create_dev_port(KernDevice *kernDevice);

#define BUS_TYPE_KEY		"Bus Type"
#define KERN_BUS_FORMAT		"%sKernBus"
#define DEVICE_DESCR_FORMAT	"IO%sDeviceDescription"
#define DEFAULT_BUS			"EISA"	// don't change this! (see comment below)
#define NAME_BUF_LEN		128

__private_extern__ unsigned int	instanceNum = 0;
__private_extern__ BOOL         dualEide = NO;

#if (IO_DRIVERKIT_VERSION >= 400)

static IOEISADeviceDescription *getBusDescription(IOConfigTable *configTable)
{
    id		deviceDescription, ioDeviceDescription = nil;
    id		busClass, defaultBusClass, busDescriptionClass;
    id		defaultBus;
    const char	*busType, *busTypeString;
    char	*nameBuf = NULL;
    id		device;

    defaultBusClass = [KernBus lookupBusClassWithName:DEFAULT_BUS];	
    // XXX
    if (defaultBusClass == nil) {
        sprintf(nameBuf, "Missing %s kernel bus class", DEFAULT_BUS);
        panic(nameBuf);
    }

    defaultBus = [KernBus lookupBusInstanceWithName:DEFAULT_BUS busId:0];

    nameBuf = (char *)IOMalloc(NAME_BUF_LEN);

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

	/*
	 * Instead of messaging the 'busClass', we call defaultBusClass
	 * to create a new deviceDescription for us.
	 * This is because PCI for some reason does not allocate resources
	 * for the "I/O Ports" and the "IRQ Levels" key.
	 * The defaultBusClass, which is EISA, does allocate the resources.
	 */
    deviceDescription = [defaultBusClass 
			deviceDescriptionFromConfigTable:configTable];	
    if (deviceDescription == nil) {
        IOLog("getBusDescription: deviceDescriptionFromConfigTable failed");
		return nil;
    }

	/*
	 * Force the bus to be always EISA.
	 */
//	if ([deviceDescription bus] == nil) {
        [deviceDescription setBus:defaultBus];
//    }

    device = [[KernDevice alloc] initWithDeviceDescription:deviceDescription];
    if (device == nil) {
        IOLog("getBusDescription: initWithDeviceDescription failed ");
        return nil;	
    }

    [deviceDescription setDevice: device];
	
    sprintf(nameBuf, "IO%sDeviceDescription", busType);
    busDescriptionClass = objc_getClass((const char *)nameBuf);

    ioDeviceDescription = [[busDescriptionClass alloc]
                                _initWithDelegate: deviceDescription];
    [ioDeviceDescription setDevicePort:create_dev_port(device)];

    return (IOEISADeviceDescription *) ioDeviceDescription;
}

#else /* IO_DRIVERKIT_VERSION < 400 */

id		EISABusClass, theEISABus;

static IOEISADeviceDescription *getBusDescription(IOConfigTable *configTable)
{
    id		deviceDescription, ioDeviceDescription = nil;
    id		busClass, defaultBusClass, busDescriptionClass;
    id		defaultBus;
    id		device;
    char	*nameBuf;
    const char	*busType;
    char	busClassName[128];

    EISABusClass = objc_getClass("EISAKernBus");
    if (EISABusClass == nil)
        panic("Missing EISA kernel bus class");
    theEISABus = [[EISABusClass alloc] init];

    busType = [configTable valueForStringKey:"Bus Type"];

    if (busType) {
        sprintf(busClassName, "%sKernBus", busType);
        busClass = objc_getClass((const char *)busClassName);
    } else {
        busClass = nil;
    }

    if (busClass == nil)
        busClass = objc_getClass("EISAKernBus");

    deviceDescription = [theEISABus
                            deviceDescriptionFromConfigTable:configTable];	

    if (deviceDescription == nil) {
        IOLog("configureDriver: initFromConfigTable failed");
        return nil;
    }

    device = [[KernDevice alloc]
                    initWithDeviceDescription: deviceDescription];

    if (device == nil) {
        IOLog("configureDriver: initFromDeviceDescription failed");
        return nil;		
    }

    [deviceDescription setDevice:device];

    if (busType) {
        sprintf(busClassName, "IO%sDeviceDescription",
                busType);
        busDescriptionClass =
            objc_getClass((const char *)busClassName);
    } else {
        busDescriptionClass = nil;
    }

    if (busDescriptionClass == nil)
        busDescriptionClass =
            objc_getClass("IOEISADeviceDescription");

    ioDeviceDescription = [[busDescriptionClass alloc]
                                _initWithDelegate: deviceDescription];
    [ioDeviceDescription setDevicePort: create_dev_port(device)];

    return (IOEISADeviceDescription *) ioDeviceDescription;
}
#endif IO_DRIVERKIT_VERSION >= 400

@implementation DualEide

+(BOOL)probe:(IODeviceDescription *)devDesc
{
    IOEISADeviceDescription *primaryEISADeviceDescr;
	IOEISADeviceDescription *secondaryEISADeviceDescr;
    IORange		primaryPortRange;
	IORange		secondaryPortRange;
    IORange 	*portRangeList;

    unsigned int	numPortRanges;
    unsigned int	numInterrupts;

    unsigned int primaryInterrupt;
	unsigned int secondaryInterrupt;
    unsigned int *interruptList;
	unsigned int probeFailedCount = 0;

	IOReturn rtn;
    dualEide = YES;
	
    /*
     * Need to create a second device description to send to the second
     * instance.  Start with primary instance's configTable to create another
     * device description.
     */
    primaryEISADeviceDescr   = (IOEISADeviceDescription *) devDesc;
    secondaryEISADeviceDescr = getBusDescription([devDesc configTable]);
    if (secondaryEISADeviceDescr == nil) {
        return NO;
	}

    // Get the interrupts from the primary device description.
    numInterrupts = [primaryEISADeviceDescr numInterrupts];
    interruptList = [primaryEISADeviceDescr interruptList];	

    primaryInterrupt = *interruptList;
	++interruptList;
    secondaryInterrupt = *interruptList;

    // Get the port ranges from the primary device description.
    numPortRanges = [primaryEISADeviceDescr numPortRanges];
    portRangeList = [primaryEISADeviceDescr portRangeList];	

    primaryPortRange.start   = portRangeList->start;
    primaryPortRange.size    = portRangeList->size;
	++portRangeList;
    secondaryPortRange.start = portRangeList->start;
    secondaryPortRange.size  = portRangeList->size;

    // Put the correct interrupt into each device description 	
    [primaryEISADeviceDescr setInterruptList:&primaryInterrupt num:1];
    rtn = [secondaryEISADeviceDescr setInterruptList:&secondaryInterrupt
		num:1];
	if (rtn != IO_R_SUCCESS) {
	    IOLog("%s: Error in setInterruptList (%s)\n", [self name],
			[self stringFromReturn:rtn]);
	}

    // Put the correct port range into each device description
    [primaryEISADeviceDescr setPortRangeList:&primaryPortRange num:1];
    rtn = [secondaryEISADeviceDescr setPortRangeList:&secondaryPortRange
		num:1];
	if (rtn != IO_R_SUCCESS) {
	    IOLog("%s: Error in setPortRangeList (%s)\n", [self name],
			[self stringFromReturn:rtn]);
	}
	
    // Probe the two instances of this Dual EIDE personality.

	instanceNum = 0;	// start with instance 0
	if (![IdeController probe: primaryEISADeviceDescr]) {
#if 0
		IOLog("Instance 0 of Dual EIDE failed the probe.\n");
		dualEide = NO;
		return NO;
#else
		probeFailedCount++;
#endif
	}

	instanceNum++;		// advance to instance 1
	if (![IdeController probe: secondaryEISADeviceDescr]) {
#if 0
		IOLog("Instance 1 of Dual EIDE failed the probe.\n");
		dualEide = NO;
#else
		probeFailedCount++;	
#endif
	}
	
	if (probeFailedCount == 2)
		return NO;
	
    return YES;
}

@end
