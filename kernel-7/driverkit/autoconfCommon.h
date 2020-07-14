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

/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * autoconfCommon.h - machine-independent driverkit-style 
 *		      autoconfiguration declarations.
 *
 * HISTORY
 * 23-Jan-93    Doug Mitchell at NeXT
 *      Created.
 */

#ifdef	DRIVER_PRIVATE

#import <objc/objc.h>

#import <mach/port.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODeviceDescription.h>

/*
 * Routines to be implemented in machine-dependent layer.
 */
 
/*
 * Start up native devices (those with hard-coded configuration).
 */
void probeNativeDevices(void);

/*
 * Perform machine dependent hardware probe/config.
 */
void probeHardware(void);

/*
 * Start up all non-native direct drivers. Called after probeHardware(). 
 */
void probeDirectDevices(void);

/*
 * Passed to configureThread() from kern_IOProbeDriver().
 */
struct probeDriverArgs {
	id		waitLock;
	unsigned char	*configData;
	BOOL		rtn;
};

extern void configureThread(struct probeDriverArgs *args);

/*
 * Data to be declared in machine-dependent layer.
 */
extern char 		*indirectDevList[];	// indirect device list
extern char		*pseudoDevList[];	// pseudo device list

#endif	/* DRIVER_PRIVATE */
