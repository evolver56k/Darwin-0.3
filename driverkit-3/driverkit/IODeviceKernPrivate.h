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
 * IODeviceKernPrivate.h - private kernel IODevice catagory.
 *
 * HISTORY
 * 14-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/IODevice.h>
#import <objc/List.h>

@interface IODevice(kernelPrivate)

/*
 * A new class has been loaded into the kernel. Connect the new class
 * with all existing classes, probing as appropriate. Returns
 * IO_R_NO_DEVICE if no devices were instantiated, else returns IO_R_SUCCESS.
 */
+ (IOReturn)addLoadedClass : newClass
	       description : deviceDescription;
	     
/*
 * Probe for any indirect device classes in the system which are 
 * interested in connecting to a newly instantiated device of any kind.
 * Used only by -registerDevice.
 */
+(void)connectToIndirectDevices : newObject;

/*
 * Returns a list of the registered objects for the driver class.
 */
+ (List *)objectsForDriverClass : (Class *)driverClass;

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen;
- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen;

@end


