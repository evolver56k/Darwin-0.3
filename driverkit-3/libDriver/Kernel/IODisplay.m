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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * IODisplay.m - Abstract superclass for all IODisplay objects.
 *
 *
 * HISTORY
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 */

#define DRIVER_PRIVATE 1
#define KERNEL 1

#import <driverkit/driverTypes.h>
#import <driverkit/driverTypesPrivate.h>
#import <driverkit/kernelDriver.h>
#import	<driverkit/IODisplay.h>
#import <driverkit/IODisplayPrivate.h>

@implementation IODisplay

/* Allocate a console support info structure based on this display. This
 * structure, and the functions in it, are used to display alert and
 * console windows.
 */
- (IOConsoleInfo *)allocateConsoleInfo;
{
    return (IOConsoleInfo *)0;
}

- (port_t)devicePort
{
    return [[self deviceDescription] devicePort];
}


- hideCursor: (int)token;
{
    return self;
}

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;
{
    return self;
}

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;
{
    return self;
}

- setBrightness:(int)level token:(int)t;
{
    return self;
}

/* Return a pointer to an IODisplayInfo describing the display.
 */
- (IODisplayInfo *)displayInfo;
{
    return &_display;
}

/* Return the registration token for this display.
 */
- (int)token;
{
    return _token;
}

/* Set the token for this display.
 */
- (void)setToken:(int)token;
{
    _token = token;
}

- (IOReturn)getIntValues:(unsigned int *)parameterArray
		forParameter:(IOParameterName)parameterName
		count:(unsigned int *)count
{
    unsigned maxCount;

    maxCount = *count;
    if (strcmp(parameterName, IO_GET_DISPLAY_PORT) == 0
	|| strcmp(parameterName, "IO_Display_GetPort") == 0) {

        port_t kernPort, userPort;

	if (maxCount < IO_GET_DISPLAY_PORT_SIZE)
	    return IO_R_INVALID_ARG;
	kernPort = [self devicePort];
	userPort = IOConvertPort(kernPort, IO_KernelIOTask, IO_CurrentTask);
	parameterArray[0] = (unsigned)userPort;
	*count = 1;
	return IO_R_SUCCESS;
    } else {
	return [super getIntValues:parameterArray 
	    forParameter:parameterName
	    count:count];
    }
}

- (IOReturn)getCharValues:(unsigned char *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
{
    IOConfigTable *configTable;
    const char *value;
    unsigned int length;

    configTable = [[self deviceDescription] configTable];
    if (configTable != nil) {
	value = [configTable valueForStringKey:parameterName];
	if (value != 0) {
	    length = strlen(value) + 1;
	    if (length <= *count) {
		strcpy(parameterArray, value);
		*count = length;
		return IO_R_SUCCESS;
	    }
	}
    }
    return [super getCharValues:parameterArray 
	forParameter:parameterName
	count:count];
}
@end
