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

#import <objc/Object.h>
#import <objc/objc-load.h>
#import <string.h>
#import <stdio.h>
#import "Car.h"

@interface Jar : Object @end

@interface Bar : Object @end

@implementation Bar

static int myHandler(char *name)
{
	char buf[256];
	char *names[] = { buf, 0 };

	strcpy(buf,name); strcat(buf,".o");
	
	objc_loadModule(names, 0, 0);

	return 1;
}

+ finishLoading:hdr
{
	Car *aCar;

	objc_setClassHandler(myHandler);

	aCar = [objc_getClass("Car") new];

	printf("+++ loading class Bar\n");

	// dynamically loaded.
	if ([aCar respondsTo:@selector(drive)]) {
		printf("aCar can `drive'\n");
		[aCar drive];
	} else
		printf("aCar cannot `drive'\n");

	if ([aCar respondsTo:@selector(stop)]) {
		printf("aCar can `stop'\n");
		[aCar stop];
	} else {
		printf("aCar cannot `stop'\n");
#if 0
		{
		char *names[] = { "CategoryOfCar.o", 0 };
		objc_loadModule(names, 0, 0);
		[aCar stop];
		objc_unloadLastModule(0);
		[aCar stop];
		}
#endif
	}
	return self;
}

+ startUnloading
{
	printf("+++ unloading class Bar\n");
	return self;
}

@end
