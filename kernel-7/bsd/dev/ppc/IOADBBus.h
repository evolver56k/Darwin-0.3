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

/* Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 *
 * IOADBBus.h - This file contains the definition of the
 * IOADBBus Class, which is an indirect driver designed to
 * communicate with devices on the ADB bus.
 *
 * Note: this API is very preliminary and is expected to change drastically
 * in future releases, caveat emptor.  In the future it will probably be based
 * on some extension to the PortDevices protocol.
 *
 * HISTORY
 *  1997-12-19    Brent Schorsch (schorsch) created IOADBDevice.h
 *  1998-01-19    Dave Suurballe transformed into IOADBBus.h (DS2)
 */

#import <objc/Object.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>		// DS2

typedef void (*autopoll_callback)(int, unsigned char *, int, void *);

#ifndef ADB_TYPES		/* added so can include IOADBDevice.h also */
#define ADB_TYPES

typedef unsigned long int IOADBDeviceState;

enum
{
    /*
     * set if the adb device is available to get and object reference
     * (and therefore get/set registers)
     */
    kIOADBDeviceAvailable = 0x00000001
};

#define IO_ADB_MAX_DEVICE	16
#define IO_ADB_MAX_PACKET	 8

typedef struct _adbDeviceInfo
{
    int originalAddress;	// type
    int address;		// at the present moment, the adbAddress
    int originalHandlerID;	// handler ID of device when first probed
    int handlerID;		// handler ID of device at the present moment
    long uniqueID;
    unsigned long flags;	// see previous enum for flag bits
} IOADBDeviceInfo;

#endif ADB_TYPES


// DS2...
@protocol ADBprotocol

- (IOReturn) GetTable: (IOADBDeviceInfo *) table
	             : (int *) lenP;

- (IOReturn) getADBInfo: (int) whichDevice
		       : (IOADBDeviceInfo *) deviceInfo;

- (IOReturn) flushADBDevice: (int) whichDevice;

- (IOReturn) readADBDeviceRegister: (int) whichDevice
				  : (int) whichRegister
			          : (unsigned char *) buffer
			          : (int *) length;

- (IOReturn) writeADBDeviceRegister: (int) whichDevice
				   : (int) whichRegister
			           : (unsigned char *) buffer
			           : (int) length;

- (IOReturn) adb_register_handler: (int) type
        			 : (autopoll_callback) handler;

@end
// ...DS2

@interface IOADBBus : IODevice	<ADBprotocol>	// DS2
{
    void *_priv;				// DS2
}

// Class methods

// DS2...

+ (BOOL) probe: (id) deviceDescription;

- initFromDeviceDescription:(IODeviceDescription *)deviceDescription;

+ (IODeviceStyle) deviceStyle;

+ (Protocol **)requiredProtocols;

- free;

// ...DS2

/*
 * Call this class method with an array that is IO_ADB_MAX_DEVICE long.
 */
- (IOReturn) GetTable: (IOADBDeviceInfo *) table
	             : (int *) lenP;

// DS2 - initForDevice: (long) uniqueID
//		      : (IOReturn *) result;

- (IOReturn) getADBInfo: (int) whichDevice			// DS2
		       : (IOADBDeviceInfo *) deviceInfo;	// DS2

- (IOReturn) flushADBDevice: (int) whichDevice;			// DS2

/*
 * Note, the buffers must be IO_ADM_MAX_PACKET long.
 */
- (IOReturn) readADBDeviceRegister: (int) whichDevice		// DS2
				  : (int) whichRegister		// DS2
			          : (unsigned char *) buffer
			          : (int *) length;

- (IOReturn) writeADBDeviceRegister: (int) whichDevice		// DS2
				   : (int) whichRegister	// DS2
			           : (unsigned char *) buffer
			           : (int) length;

/* The state functions below are not currently implemented */
/*
 * Set the state for the port device.
 */
- (IOReturn) setState: (int) whichDevice			// DS2
		     : (IOADBDeviceState) state			// DS2
		     : (IOADBDeviceState) mask;

/*
 * Get the state for the port device.
 */
- (IOADBDeviceState) getState: (int) whichDevice;		// DS2

- (IOReturn) adb_register_handler: (int) type			// DS2
        			 : (autopoll_callback) handler;
/*
 * Wait for the atleast one of the state bits defined in mask to be equal 
 * to the value defined in state.
 * Check on entry then sleep until necessary.
 */
- (IOReturn) watchState: (int) whichDevice			// DS2
		       : (IOADBDeviceState *) state		// DS2
                       : (IOADBDeviceState) mask;


@end
