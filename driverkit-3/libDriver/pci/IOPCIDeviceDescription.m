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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * PCI device description class.
 *
 * HISTORY
 *
 * 19 Aug 1994 Dean Reece at NeXT
 *	Complete re-write (it was just a placeholder)
 *
 * 19 Jul 1994 Curtis Galloway at NeXT
 *	Created.
 */
 
#define KERNEL_PRIVATE	1

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/i386/IOPCIDeviceDescriptionPrivate.h>
#import <driverkit/i386/PCIKernBus.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/i386/directDevice.h>

struct _pci_private {
    BOOL		valid;
    unsigned char	devNum, funNum, busNum;
};

@implementation IOPCIDeviceDescription(Private)

/*
 * Here we init the PCI device description by calling thePCIBus to
 * decode our configTable and extract the bits necessary to determine
 * the PCI address.  If the thePCIBus object is absent or claims no
 * PCI bus, then we just set valid to NO.
 *
 * XXX Implementation Note:  It seems odd that thePCIBus object handles
 * parsing the configTable, rather than doing it here.  This division
 * of functionality was choosen to allow easy replacement; thePCIBus is
 * a loadable driver, but IOPCIDeviceDescription is statically linked
 * into the kernel.  If/when the driverkit stuff gets moved to a loadable
 * module, [thePCIBus configAddress] should be moved here.
 */
- _initWithDelegate:delegate
{
    struct _pci_private *private;
    id	thePCIBus = [KernBus lookupBusInstanceWithName:"PCI" busId:0];

    [super _initWithDelegate:delegate];

    _pci_private = (void *)IOMalloc(sizeof(struct _pci_private));
    private = (struct _pci_private *)_pci_private;

    private->valid =  ( (thePCIBus != nil) &&
	([thePCIBus isPCIPresent] == YES) &&
	([thePCIBus configAddress: delegate
			   device: &(private->devNum)
			 function: &(private->funNum)
			      bus: &(private->busNum)] == IO_R_SUCCESS)
	     );
    return self;	
}

@end

@implementation IOPCIDeviceDescription

- free
{
    struct _pci_private *private = (struct _pci_private *)_pci_private;

    IOFree(private, sizeof(struct _pci_private));
    return [super free];
}

/*
 * getPCIdevice is the whole purpose for this object.  This method allows
 * callers to get the PCI config address of the PCI device associated
 * with this device description.  If all goes well, the three params are
 * filled in and IO_R_SUCCESS is returned.  There are a variety of reasons
 * that the address couldn't be known, in which case an appropriate code is
 * returned and the parameters are left untouched.  It is acceptable for any
 * of the parameter pointers to be NULL.
 */
- (IOReturn) getPCIdevice: (unsigned char *) devNum
		 function: (unsigned char *) funNum
		      bus: (unsigned char *) busNum
{
    struct _pci_private *private = (struct _pci_private *)_pci_private;

    if (private->valid) {
	if (devNum) *devNum = private->devNum;
	if (funNum) *funNum = private->funNum;
	if (busNum) *busNum = private->busNum;
	return IO_R_SUCCESS;
    }
    return IO_R_NO_DEVICE;
}

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    [super property_IODeviceType:types length:maxLen];
    strcat( types, " "IOTypePCI);
    return( self);
}

- property_IOSlotName:(char *)name length:(unsigned int *)maxLen
{
    struct _pci_private *private = (struct _pci_private *)_pci_private;

    if (private->valid) {
	sprintf( name, "Dev=%d Func=%d Bus=%d",
            private->devNum, private->funNum, private->busNum );
	return self;
    }
    return nil;
}

@end
