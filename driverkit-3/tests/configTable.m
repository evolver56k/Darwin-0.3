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
 * IOConfigTable test. Assumes the presence of driver bundles and a
 * system bundle in the usual place (currently in /usr/Devices).
 * Dumps contents of IOConfigTable instances to stdout.
 */

#import <driverkit/IOConfigTable.h>
#import <objc/List.h>
#import <libc.h>

#define TEST_DRIVER_BUNDLE	1

typedef enum {All, One, System} Mode;

static int reportAll();
static int reportOne(const char *driverName, int unitNum);
static int reportSystem(int verbose);
static int dumpConfigInfo (IOConfigTable *configTable, const char **keys);

static char *driverKeys[] = {
	"Driver Name",
	"Instance",
	"Type",
	"Version",
	"DMA Channels",
	"IRQ Levels",
	"I/O Ports",
	"Memory Maps",
	NULL	
};

static char *systemKeys[] = {
	"Source",
	"Version",
	"Machine Name",
	"Boot Drivers",
	"Active Drivers",
	NULL
};

static char *verboseSystemKeys[] = {
	"Driver Path",
	"Bus Type",
	"Kernel",
	"Device",
	"Memory",
	"Root Device",
	"Uses FPU",
	"Memory Used",
	"User Mode",
	"Kernel Flags",
	NULL
};


static void usage(char **argv) {
	printf("Usage: \n");
	printf("\t%s a(ll)                 -- displays all drivers\n",
		argv[0]);
	printf("\t%s d=driverName [unit]   -- display one driver\n", argv[0]);
	printf("\t%s s(ystem) [v]          -- display system config "
		"(verbose)\n", argv[0]);
	exit(1);
}

int main(int argc, char **argv)
{
	int verbose = 0;
	Mode mode;
	char *driverName;
	int unitNum;
	
	if(argc < 2) {
		usage(argv);
	}
	switch(argv[1][0]) {
	    case 'a':
		mode = All;
		break;
		
	    case 'd':
		mode = One;
		driverName = &argv[1][2];
	        switch(argc) {
		    case 2:
		    	unitNum = -1;	// means 'all instances'
			break;
		    case 3:
			unitNum = atoi(argv[2]);
			break;
		    default:
			usage(argv);
		}    	
		break;
		
	    case 's':
	    	switch(argc) {
		    case 2:
		    	break;
		    case 3:
		    	if(argv[2][0] == 'v') {
				verbose = 1;
			}
			else {
				usage(argv);
			}
			break;
		    default:
		    	usage(argv);
		}
		mode = System;
		break;
	    default:
		usage(argv);
	}
	
	switch(mode) {
	    case All:
	    	return reportAll();
	    case One:
	    	return reportOne(driverName, unitNum);
	    case System:
	    	return reportSystem(verbose);
	}
	return 0;
}

static int reportAll()
{
	List *list = [IOConfigTable tablesForInstalledDrivers];
	int index;
	IOConfigTable *driverConfig;
	
	if(list == nil) {
		printf("installedDrivers returned nil\n");
		return 1;
	}
	printf("Config Table info (all devices):\n");
	for(index=0; ; index++) {
		driverConfig = [list objectAt:index];
		if(driverConfig == nil) {
			break;
		}
		dumpConfigInfo(driverConfig, driverKeys);
		printf("\n");
	}
	printf("%d Drivers found\n", index);
	[list free];
	return 0;
}

static int doReportOne(const char *driverName, int unitNum)
{
	IOConfigTable *driverConfig;
	driverConfig = [IOConfigTable newForDriver:driverName unit:unitNum];
	if(driverConfig == nil) {
		return 1;
	}
	printf("Config Table for %s unit %d\n", driverName, unitNum);
	dumpConfigInfo(driverConfig, driverKeys);

#if	TEST_DRIVER_BUNDLE
	/*
	 * Test -driverBundle...
	 */
	{
		id bundle;
		const char *dir;
		
		bundle = [driverConfig driverBundle];
		if(bundle == nil) {
			printf("driverBundle returned nil\n");
			return 1;
		}
		dir = [bundle directory];
		printf("  bundle dir = %s\n", dir);
		[bundle free];
	}
#endif	TEST_DRIVER_BUNDLE

	[driverConfig free];
	return 0;
}

static int reportOne(const char *driverName, int unitNum)
{
	BOOL foundOne;
	
	if(unitNum >= 0) {
		if(doReportOne(driverName, unitNum)) {
			printf("Driver config table for %s unit %d "
				"not found\n", driverName, unitNum);
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
	    foundOne = NO;
	    
	    for(unitNum=0; ; unitNum++) {
		if(doReportOne(driverName, unitNum)) {
		    if(foundOne) {
			/*
			 * Normal termination.
			 */
			return 0;
		    }
		    else {
			printf("Driver config tables for %s not found\n",
				driverName);
			return 1;
		    }
		}
		else {
			foundOne = YES;
		}
	    }
	}
}

static int reportSystem(int verbose)
{
	IOConfigTable *systemConfig;
	
	systemConfig = [IOConfigTable newFromSystemConfig];
	if(systemConfig == nil) {
		printf("System Config Table not found\n");
		return 1;
	}
	printf("System config table:\n");
	dumpConfigInfo(systemConfig, systemKeys);
	if(verbose) {
		dumpConfigInfo(systemConfig, verboseSystemKeys);
	}
	[systemConfig free];
	return 0;
}

static int dumpConfigInfo (IOConfigTable *configTable, const char **keys)
{
	const char *value;
	const char **key;
	
	for(key=keys; *key; key++) {
		value = [configTable valueForStringKey:*key];
		if(value) {
			printf("  %s = %s\n", *key, value);
		}
	}
	return 0;
}
