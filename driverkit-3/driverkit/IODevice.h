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
 * IODevice.h - Interface for generic I/O device class.
 *
 * HISTORY
 * 13-Jan-98	Martin Minow at Apple
 *	Added createMachPort
 * 28-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <objc/Object.h>
#ifdef	KERNEL
#import <mach/mach_user_internal.h>
#endif	KERNEL
#import <driverkit/IODeviceDescription.h>
#import <driverkit/devsw.h>
#import <driverkit/IOProperties.h>

#define IO_DRIVERKIT_VERSION	500

@interface IODevice: Object
{
@private
	int		_unit;			// like a minor number. Use
						// is unique per device.
	IOString	_deviceName;		// E.g., "sd0a". Obtained
						//   via -name.
	IOString	_location;		// E.g., "0xf7f04000"
	IOString	_deviceKind;		// E.g., "SCSIDisk"
        id		__deviceDescription;
	int		_IODevice_reserved[3];
}

/*
 * Probe method used by direct, indirect, and pseudo devices. 
 * Implemented by most (not all) subclasses.
 * 
 * Create an instance of subclass to be associated with specified 
 * deviceDescription. Returns YES if any devices were instantiated.
 */
+ (BOOL)probe : (id)deviceDescription;

/*
 * Report basic style of driver (direct, indirect, or pseudo). Must be 
 * implemented by subclass.
 */
+ (IODeviceStyle)deviceStyle;

/*
 * Report protocols needed. Kernel-level indirect devices must implement
 * this.
 */
+ (Protocol **)requiredProtocols;

/*
 * Must be called from leaf subclass's +initialize method if 
 * [super initialize] is not called there. 
 */
+ (void)registerClass : classId;

/*
 * Called if/when a class is being removed from an executable's address
 * space.
 */
+ (void)unregisterClass : classId;

/*
 * Called to get/set a class's major numbers for block and
 * character devices.  If the major number is not set,
 * it defaults to -1.
 */
+ (void)setBlockMajor		: (int)bmajor;
+ (int)blockMajor;
+ (void)setCharacterMajor	: (int)cmajor;
+ (int)characterMajor;

/*
 * Add functions for this class to the cdevsw/bdevsw tables.
 * The major number to use is taken from the class' config table
 * in the keys "Character Major" and "Device Major" if present.
 * If the keys are not present, the first available major number
 * will be used.  If there is no room in the devsw table, returns FALSE.
 * The class' +characterMajor and +blockMajor methods can be used
 * to find the major numbers assigned to the class.
 */
+ (BOOL)addToCdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    read:	(IOSwitchFunc) readFunc
    write:	(IOSwitchFunc) writeFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    stop:	(IOSwitchFunc) stopFunc
    reset:	(IOSwitchFunc) resetFunc
    select:	(IOSwitchFunc) selectFunc
    mmap:	(IOSwitchFunc) mmapFunc
    getc:	(IOSwitchFunc) getcFunc
    putc:	(IOSwitchFunc) putcFunc;
+ (BOOL) addToBdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    strategy:	(IOSwitchFunc) strategyFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    dump:	(IOSwitchFunc) dumpFunc
    psize:	(IOSwitchFunc) psizeFunc
    isTape:	(BOOL) isTape;
+ (BOOL)removeFromCdevsw;
+ (BOOL)removeFromBdevsw;

/*
 * +driverKitVersion returns the version number of the currently
 * running DriverKit objects.
 * +driverKitVersionForDriverNamed: returns the version number of
 * DriverKit that the specified driver was compiled under.
 */
 
+ (int) driverKitVersion;
+ (int) driverKitVersionForDriverNamed:(char *)driverName;

/*
 * Initialize common instance variables. Typically invoked
 * via [super init:] in subclass's init: method.
 */
- init;

/* 
 * Initialize per specified deviceDescription. Returns nil on error. 
 * This is actually a nop at the IODevice level; subclasses may do with it 
 * as they see fit.
 */
- initFromDeviceDescription : deviceDescription;
- deviceDescription;
- (void)setDeviceDescription:deviceDescription;

/*
 * Free up resources used by this device; invoke Object's free. Instance 
 * will be "gone" upon return. Typically invoked by subclass; each subclass
 * should implement this method to free up resources particular to that 
 * subclass.
 */
- free;

/*
 * Register/unregister instance with current name space.  
 * Device must be completely initialized and ready for I/O at the 
 * time -registerDevice is called.
 */
- registerDevice;		// nil return means failure
- (void)unregisterDevice;
- registerLoudly;

/*
 * Returns a string representation of the hardware's location.
 * Indirect drivers return the direct device's path they were probed on.
 */
- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias;

/*
 * Returns the tail of the matchPath parameter if the head matches the
 * device's path, else returns nil. Indirect drivers match on the
 * direct device's path they were probed on.
 */
- (char *) matchDevicePath:(char *)matchPath;

/*
 * Get/Set instance variables. 
 */
- (void)setName			: (const char *)name;	
- (const char *)name;	
- (void)setDeviceKind		: (const char *)type;	
- (const char *)deviceKind;	
- (void)setLocation		: (const char *)location;	
- (const char *)location;	
- (void)setUnit			: (unsigned)unit;
- (unsigned)unit;

/*
 * Obtain device parameters.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out
			
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

/*
 * Look up a property or properties in the driver / device stack
 */

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length;

- getStringPropertyList:(const char *)names
	results:(char *) results
	maxLength:(unsigned int)length;

	  
/*
 * - serverConnect:taskPort: notifies an object that is a sub class of
 * IODevice that a client task is interested in communicating with it.
 * The IODevice superclass implements this method returning IO_R_UNSUPPORTED.
 * If a subclass, such as the IOSCSIController class, provices direct RPC
 * service, it will override serverConnect:taskPort: and, when called, create a
 * port, returning IO_R_SUCCESS to IODeviceMaster, which will return send right
 * to the non-privileged client. The client and IODevice are subsequently
 * responsible for all port management.
 *
 * Note that the port returned by the client device must be in the IOTask name
 * space.  MACH_USER_API will help this as port_allocate will return the right
 * type of name.
 *
 * The task port is provided to help the device track the client. NB this
 * port is currently a naked kernel port right and should be copied out or
 * port released as appropriate.
 */

- (IOReturn) serverConnect: (port_t *)   machPort   // out - IOTask space
	          taskPort: (port_t)     taskPort;  // in  - ipc_port_t

/*
 * Get/Set parameter equivalents of common methods. Used for RPC
 * interface. Each is used in getCharValues except IO_UNIT, which returns
 * one int in getIntValues.
 */
#define	IO_CLASS_NAME		"IOClassName"		/* +name */
#define IO_DEVICE_NAME		"IODeviceName"		/* -name */
#define IO_DEVICE_KIND		"IODeviceKind"		/* -deviceKind */
#define IO_UNIT			"IOUnit"		/* -unit */
#define IO_BLOCK_MAJOR		"IOBlockMajor"		/* -blockMajor */
#define IO_CHARACTER_MAJOR	"IOCharacterMajor"	/* -characterMajor */

/*
 * Convert an IOReturn to text. Subclasses which add additional
 * IOReturn's should override this method and call [super stringFromReturn] 
 * if the desired value is not found.
 */
- (const char *)stringFromReturn : (IOReturn)rtn;
+ (const char *)stringFromReturn : (IOReturn)rtn;

/*
 * Convert an IOReturn to a Unix errno.
 */
- (int)errnoFromReturn : (IOReturn)rtn;

@end		/* IODevice */


