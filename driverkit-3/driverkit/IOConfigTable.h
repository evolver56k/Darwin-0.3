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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 * 
 * IOConfigTable object. Usable both in the kernel and user space.
 *
 * HISTORY
 * 29-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <objc/Object.h>
#ifndef	KERNEL
#import <objc/List.h>
#import <objc/NXBundle.h>
#endif	KERNEL

@interface IOConfigTable : Object
{
@private
	/*
	 * In the kernel, this will be a char * to the contents of the
	 * relevant file. In user space, this will be an id of 
	 * an NXStringTable.
	 */
	void 	*_private;
}

/*
 * Obtain the system-wide configuration table.
 */
+ newFromSystemConfig;

#ifndef	KERNEL

/*
 * Obtain the configuration table for a specified driver and unit number. 
 */
+ newForDriver				: (const char *)driverName
		   		   unit : (int)unit;
	
/*
 * Obtain default configuration table for specified driver.
 */
+ newDefaultTableForDriver		: (const char *)driverName;

/*
 * Obtain a list of instances of IOConfigTable, one per active device on 
 * the system.
 */
+ (List *) tablesForInstalledDrivers;

/*
 * Obtain a list of instances of IOConfigTable, one per driver
 * loaded by the booter.
 */
+ (List *) tablesForBootDrivers;

/*
 * Obtain an NXBundle for a driver associated with current IOConfigTable
 * instance.
 */
- (NXBundle *)driverBundle;

#endif	KERNEL

/*
 * Obtain value for specified string key. Returns NULL if key not found.
 */
- (const char *)valueForStringKey:(const char *)key;

- free;

#ifdef	KERNEL

/*
 * Free a string obtained by -stringValueForStringKey:
 */
+ (void)freeString : (const char *)string;
- (void)freeString : (const char *)string;

#endif	KERNEL

@end
