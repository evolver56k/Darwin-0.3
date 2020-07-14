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

#import <driverkit/IODevice.h>
#import <kernserv/kern_server_types.h>

kern_server_t PS2Keyboard_instance;

@interface PS2KeyboardKernelServerInstance : Object
{}
+ (kern_server_t *)kernelServerInstance;
@end

@implementation PS2KeyboardKernelServerInstance
+ (kern_server_t *)kernelServerInstance
{
	return &PS2Keyboard_instance;
}
@end

@interface PS2KeyboardVersion : IODevice
{}
+ (int)driverKitVersionForPS2Keyboard;
@end

@implementation PS2KeyboardVersion
+ (int)driverKitVersionForPS2Keyboard
{
	return IO_DRIVERKIT_VERSION;
}
@end

