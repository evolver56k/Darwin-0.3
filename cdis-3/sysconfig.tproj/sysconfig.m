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
 * Copyright 1993 NeXT, Inc.
 * All rights reserved.
 */

#import <libc.h>
#import <stdio.h>
#import <driverkit/IOConfigTable.h>
#import <streams/streams.h>
#import <Foundation/Foundation.h>
#import <objc/NXStringTable.h>

#import "FoundationExtras.h"

#define DRIVER_NAME	"Driver Name"
#define SERVER_NAME	"Server Name"
#define DEVICE_DIR	"/private/Drivers/" __ARCHITECTURE__
#define SYSTEM_CONFIG	DEVICE_DIR "/System.config/Default.table"
#define BOOT_DRIVERS	"Boot Drivers"

@interface IOConfigTable (StringTable)
- (void)printStringTable;
@end

@implementation IOConfigTable (StringTable)
- (void)printStringTable
{
   NXStringTable *stringTable;
   NXStream *stream = NXOpenFile(fileno(stdout), NX_WRITEONLY);
   
   stringTable = *(NXStringTable **)_private; // Yikes!
   [stringTable writeToStream:stream];
   NXClose(stream);
}
@end

void usage()
{
    fprintf(stderr,"usage: sysconfig [-b | -c | -d <driver>] key\n");
    fprintf(stderr,"  -b: list boot drivers\n");
    fprintf(stderr,"  -c: create new Boot Drivers list using added drivers\n");
}

int main(int argc, char **argv)
{
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	int i,j,c;
	List *configList;
	id configTable;
	NSMutableArray *driverList;
	int useSystemConfig = 0;
	int listBootDrivers = 0;
	int createBootDrivers = 0;
	char *driverName = NULL;
	NSString *configFile, *arch = nil;
	NSData *configData, *archData;
	NXStream *dataStream;
	char *keyName;
	const char *value, *addr;
	int errflg = 0, driverFound = 0, triedBoot = 0, size;
        int listAllDrivers = 0;
	
	while ((c = getopt(argc, argv, "csbd:a:l")) != EOF)
	    switch (c) {
	    case 's':
		    if (driverName)
			errflg++;
		    else
			useSystemConfig++;
		    break;
	    case 'd':
		    if (useSystemConfig)
			errflg++;
		    else
			driverName = optarg;
		    break;
	    case 'b':
		    listBootDrivers++;
		    break;
	    case 'c':
		    createBootDrivers++;
		    break;
	    case 'a':
		    arch = [NSString stringWithCString:optarg];
		    break;
            case 'l':
                    listAllDrivers++;
                    break;
            default:
		    errflg++;
		    break;
	    }
	if (errflg) {
	    usage();
	    exit(2);
	}
	
        if (listAllDrivers) {
            configList = [IOConfigTable tablesForBootDrivers];
            driverList = [[NSMutableArray alloc] init];

            for (i = 0; i < [configList count]; i++) {
                configTable = [configList objectAt:i];
                printf("/*  --- Table %d ---- */\n",i);
                [configTable printStringTable];
            }
            [pool release];
            exit(0);
        }
            
	if (listBootDrivers) {
	    configList = [IOConfigTable tablesForBootDrivers];
	    driverList = [[NSMutableArray alloc] init];
	    
	    for (i = 0; i < [configList count]; i++) {
		configTable = [configList objectAt:i];
		if (value = [configTable valueForStringKey:SERVER_NAME])
		    [driverList addStringIfAbsent:value];
	    }
	    printf("%s\n", [driverList compositeStrings]);
            [pool release];
	    exit(0);
	}
	
	if (createBootDrivers) {
	    configTable = [[NXStringTable alloc] init];
	    configFile = [NSString stringWithCString:SYSTEM_CONFIG];
	    configData = [NSData dataWithContentsOfFile:configFile];
	    archData = [configData subDataFromArchitecture:arch];
	    addr = [archData bytes];
	    size = [archData length];
	    dataStream = NXOpenMemory(addr, size, NX_READONLY);
	    if (![configTable readFromStream:dataStream]) {
		exit(1);
	    }
	    NXCloseMemory(dataStream, NX_FREEBUFFER);
	    driverList = [[NSMutableArray alloc] init];
	    configList = [IOConfigTable tablesForBootDrivers];

	    value = [configTable valueForStringKey:BOOT_DRIVERS];
	    [driverList addDelimitedStrings:value delimiters:DELIMITERS];
	    for (i = 0; i < [configList count]; i++) {
		configTable = [configList objectAt:i];
		if (value = [configTable valueForStringKey:SERVER_NAME]) {
		    [driverList addStringIfAbsent:value];
		}
	    }
	    printf("%s\n", [driverList compositeStrings]);
            [pool release];
	    exit(0);
	}
	
	if (driverName == NULL)
	    useSystemConfig++;
	if (useSystemConfig) {
	    configList = [[List alloc] init];
	    configTable = [IOConfigTable newFromSystemConfig];
	    [configList addObject:configTable];
	} else {
	    configList = [IOConfigTable tablesForBootDrivers];
	}
again:
	for (i=0; i < [configList count]; i++) {
            const char *tableDriverName;
            
	    configTable = [configList objectAt:i];
            tableDriverName = [configTable valueForStringKey:SERVER_NAME];
	    if (driverName == 0 ||
                (tableDriverName != 0 &&
		strcmp(driverName, tableDriverName) == 0)) {
		    driverFound++;
		    if (optind == argc) {
			[configTable printStringTable];
		    } else {
			for (j = optind; j < argc; j++) {
				keyName = argv[j];
				value = [configTable
					    valueForStringKey:keyName];
				if (value)
				    printf("%s\n",value);
				else
				    exit(1);
			}
		    }
	    }
	}
	if (driverName && driverFound == 0) {
	    if (triedBoot) {
		fprintf(stderr, "Driver '%s' not found\n",driverName);
		exit(3);
	    } else {
		triedBoot = 1;
		configList = [IOConfigTable tablesForInstalledDrivers];
		goto again;
	    }
	}
        [pool release];
	exit(0);
}
