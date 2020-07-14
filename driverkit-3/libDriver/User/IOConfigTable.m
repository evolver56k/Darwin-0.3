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
 * User level implementation of IOConfigTable.
 *
 * HISTORY
 * 27-Jan-93    Doug Mitchell at NeXT
 *      Created.
 */

#import <driverkit/IOConfigTable.h>
#import <driverkit/IODevice.h>
#import <driverkit/configTablePrivate.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/driverServer.h>
#import <driverkit/driverTypesPrivate.h>
#import <streams/streams.h>
#import <objc/NXStringTable.h>
#import <libc.h>

static void parseDriverList(const char *driverList, List *list);

@interface IOConfigTable(Private)
+ openForFile : (const char *)filename;
@end

/*
 * The _private ivar points to one of these.
 */
typedef struct {
	NXStringTable	*stringTable;
	const char 	*bundleName;		// associated directory 
} _configPriv;

@implementation IOConfigTable

- free
{
	_configPriv *configPriv = _private;
	
	if(configPriv) {
		if(configPriv->stringTable) {
			[configPriv->stringTable free];
		}
		if(configPriv->bundleName) {
			IOFree((void *)configPriv->bundleName, 
				strlen(configPriv->bundleName) + 1);
		}
		IOFree(configPriv, sizeof(_configPriv));
	}
	return [super free];
}

/*
 * Obtain the system-wide configuration table.
 */
#define SYSTEM_CONFIG_FROM_RPC	1

+ newFromSystemConfig
{
#if	SYSTEM_CONFIG_FROM_RPC
	
	IOReturn 	rtn;
	IOConfigData 	configData;
	unsigned 	size = IO_CONFIG_TABLE_SIZE;
	NXStream 	*stream;
	NXStringTable 	*stringTable;
	int 		len;
	IOConfigTable 	*newObj = [self alloc];
	port_t		deviceMaster = device_master_self();
	_configPriv	*configPriv;
	
	/*
	 * Get system config data from the kernel.
	 */
	rtn = _IOGetSystemConfig(deviceMaster,
		IO_CONFIG_TABLE_SIZE, 
		configData,
		&size);
	if(rtn) {
		IOLog("IOConfigTable: _IOGetSystemConfig: %s\n",
			[IODevice stringFromReturn:rtn]);
		return nil;
	}

	/*
	 * Create a stream, write raw table to it.
	 */
	stream = NXOpenMemory(NULL, 0, NX_READWRITE);
	len = strlen(configData);
	NXWrite(stream, configData, len);
	NXSeek(stream, 0, NX_FROMSTART);
	
	/*
	 * Create string table, write stream to it.
	 */
	stringTable = [[NXStringTable alloc] init];
	if([stringTable readFromStream:stream] == nil) {
		IOLog("IOConfigTable: readFromStream returned nil\n");
		[stringTable free];
		return nil;
	}
	configPriv = newObj->_private = IOMalloc(sizeof(_configPriv));
	configPriv->stringTable = stringTable;
	configPriv->bundleName = NULL;
	return newObj;

#else	SYSTEM_CONFIG_FROM_RPC
	return [self openForFile:IO_SYSTEM_CONFIG_FILE];
#endif	SYSTEM_CONFIG_FROM_RPC
}

/*
 * Obtain the configuration table for a specified device and unit number. 
 * ("Unit number" is terminology from IODevice.h. We've been calling it 
 * "instance number" in the context of system config and startup.)
 */
+ newForDriver				: (const char *)driverName
		   		   unit : (int)unit
{
	char 		filename[200];
	IOConfigTable	*newObj;
	_configPriv	*configPriv;
	int 		length;
	char		*bundleName;
	
	sprintf(filename, "%s%s%s/Instance%d%s", 
		IO_CONFIG_DIR, driverName, IO_BUNDLE_EXTENSION,
		unit, IO_TABLE_EXTENSION);
	newObj = (IOConfigTable *)[self openForFile:filename];
	if(newObj == nil) {
		return nil;
	}
	configPriv = newObj->_private;
	sprintf(filename, "%s%s%s", 
		IO_CONFIG_DIR, driverName, IO_BUNDLE_EXTENSION);
	length = strlen(filename) + 1;
	bundleName = IOMalloc(length);
	strcpy(bundleName, filename);
	configPriv->bundleName = bundleName;
	return newObj;
}

+ newDefaultTableForDriver		: (const char *)driverName
{
	char 		filename[200];
	IOConfigTable	*newObj;
	_configPriv	*configPriv;
	int 		length;
	char		*bundleName;
	
	sprintf(filename, "%s%s%s/%s", 
		IO_CONFIG_DIR, driverName, IO_BUNDLE_EXTENSION,
		IO_DEFAULT_TABLE_FILENAME);
	newObj = (IOConfigTable *)[self openForFile:filename];
	if(newObj == nil) {
		return nil;
	}
	configPriv = newObj->_private;
	sprintf(filename, "%s%s%s", 
		IO_CONFIG_DIR, driverName, IO_BUNDLE_EXTENSION);
	length = strlen(filename) + 1;
	bundleName = IOMalloc(length);
	strcpy(bundleName, filename);
	configPriv->bundleName = bundleName;
	return newObj;
}
	
	
/*
 * Obtain a list of instances of IOConfigTable, one per {boot, active} 
 * device on the system.
 */
+ (List *) tablesForInstalledDrivers
{
	List 		*list;
	IOConfigTable 	*systemConfig;
	const char	*drivers;
	
	list = [[List alloc] init];
	systemConfig = [self newFromSystemConfig];
	if(systemConfig == nil) {
		IOLog("tablesForInstalledDrivers: no system config file"
			" found\n");
		return nil;
	}
	drivers = [systemConfig valueForStringKey:"Boot Drivers"];
	parseDriverList(drivers, list);
	drivers = [systemConfig valueForStringKey:"Active Drivers"];
	parseDriverList(drivers, list);
	if([list count] == 0) {
	    	IOLog("installedDrivers: no drivers found\n");
	}
	return list;
}

/*
 * Obtain a list of instances of IOConfigTable, one per driver
 * loaded by the booter.
 */
+ (List *) tablesForBootDrivers
{
    List		*list;
    IOReturn		rtn;
    int			i;
    IOConfigData 	configData;
    unsigned 		size;
    port_t		deviceMaster = device_master_self();
    NXStream 		*stream;
    NXStringTable 	*stringTable;
    int 		len;
    IOConfigTable 	*newTable;
    _configPriv		*configPriv;
    
    list = [[List alloc] init];
    for (i=1; ; i++) {
	/*
	 * Get system config data from the kernel.
	 */
	size = IO_CONFIG_TABLE_SIZE;
	rtn = _IOGetDriverConfig(deviceMaster,
		i,
		IO_CONFIG_TABLE_SIZE, 
		configData,
		&size);
	if(rtn != IO_R_SUCCESS) {
	    if (rtn != IO_R_NO_DEVICE) {
		IOLog("IOConfigTable: _IOGetDriverConfig: %s\n",
			[IODevice stringFromReturn:rtn]);
		[list free];
		return nil;
	    }
	    break;
	}

	/*
	 * Create a stream, write raw table to it.
	 */
	stream = NXOpenMemory(NULL, 0, NX_READWRITE);
	len = strlen(configData);
	NXWrite(stream, configData, len);
	NXSeek(stream, 0, NX_FROMSTART);
    
	/*
	 * Create string table, write stream to it.
	 */
	stringTable = [[NXStringTable alloc] init];
	if([stringTable readFromStream:stream] == nil) {
		IOLog("IOConfigTable: readFromStream returned nil\n");
		[stringTable free];
		return nil;
	}
	newTable = [self alloc];
	configPriv = newTable->_private = IOMalloc(sizeof(_configPriv));
	configPriv->stringTable = stringTable;
	configPriv->bundleName = NULL;
	[list addObject:newTable];
    }
    return list;
}

/*
 * Obtain an NXBundle for a driver associated with current IOConfigTable
 * instance.
 */
- (NXBundle *)driverBundle; 

{
	NXBundle 	*bundle;
	_configPriv	*configPriv = _private;
			
	if(configPriv->bundleName == NULL) {
		/*
		 * Must be system config; no can do.
		 */
		return nil;
	}
	bundle = [NXBundle alloc];
	return [bundle initForDirectory:configPriv->bundleName];
}


/*
 * Obtain value for specified string key, string and unsigned int versions.
 *
 * String version. Returns NULL if key not found.
 */
- (const char *)valueForStringKey:(const char *)key
{
	_configPriv *configPriv = _private;
	NXStringTable *stringTable = configPriv->stringTable;
	
	return [stringTable valueForStringKey:key];
}

@end

@implementation IOConfigTable(Private)

+ openForFile : (const char *)filename
{
	IOConfigTable *newObj = [self alloc];
	NXStringTable *stringTable = [[NXStringTable alloc] init];
	_configPriv *configPriv;
	
	newObj->_private = nil;
	if([stringTable readFromFile:filename] == nil) {
		[newObj free];
		[stringTable free];
		return nil;
	}
	configPriv = newObj->_private = IOMalloc(sizeof(_configPriv));
	configPriv->stringTable = stringTable;
	configPriv->bundleName = NULL;
	return newObj;
}


@end

/*
 * parse driverList string; get an IOConfigTable for each unit 
 * of each device in the string; add each IOConfigTable to list.
 */
static void
parseDriverList(const char *driverList,
		List *list)
{
	IOConfigTable 	*driverConfig;
	char 		driverName[100];
	const char	*stringP;		// --> activeDrivers
	char 		*nameP;			// --> driverName
	int 		unit;
	BOOL		gotName;

	if(driverList == NULL) {
		return;
	}
	nameP = driverName;	
	gotName = NO;
	stringP = driverList;
	while(1) {
	    if((*stringP == ' ') || (*stringP == '\0')) {
		if(!gotName) {
		    /*
		     * Skip spaces (we haven't gotten a name yet).
		     */
		    if(*stringP == '\0') {
			break;			// trailing spaces
		    } 
		    goto nextChar;
		}
		*nameP = '\0';			// null-terminate driverName
		    
		/*
		 * Reset...
		 */
		nameP = driverName;
		gotName = NO;
		
		/*
		 * Get an IOCongTable for each possible
		 * instance of the driver.
		 */
		for(unit=0; ; unit++) {
		    driverConfig = [IOConfigTable newForDriver:driverName
					    unit:unit];
		    if(driverConfig) {
			[list addObject:driverConfig];
		    }
		    else {
			/*
			 * No more instances.
			 */
			if(*stringP) {
			    goto nextChar;
			}
			else {
			    goto out;
			}
		    }
		}   /* for unit */							
	    }	    /* space */
	    else {
	        gotName = YES;
	    	*nameP++ = *stringP;
	    }
nextChar:
	    stringP++;
	}
out:
	return;
}
