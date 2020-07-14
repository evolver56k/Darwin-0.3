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
#import "RegionManager.h"

#import <stdlib.h>
#import <string.h>
#import <sys/loader.h>

@implementation RegionManager

-initialize:  (vm_task_t)theTask
{
	regions = [Storage	newCount:	0 
					elementSize:	sizeof(Region) 
					description:	@encode(Region)];						
	task = theTask;
	return self;
}
+newTask: (vm_task_t)theTask
{
	return [[super new] initialize: theTask];
}

-free
{
	int count;
	Region *region;
	for (count = [regions count]; count; count--) {
		region = [regions elementAt: (count - 1)];
		vm_deallocate(task_self(), region->data, region->dataCount);
	}
	[regions free];
	return [super free];
}

-(Region *)regionFor: (void *)pointer
{
	Region newRegion;
	kern_return_t error = NO;
	Region *region = NULL;
	int count;
	BOOL found;
	for (count = [regions count], found = NO; count && !found; count--) {
		region = [regions elementAt: (count - 1)];
		if (	(region->address <= (pointer_t)pointer) 
			&& ((pointer_t)pointer < (region->maxAddress)))
			found = YES;
	}
	if (!found) {
		newRegion.address = (vm_address_t)pointer;
		error = vm_region(	task, 
						&newRegion.address,
						&newRegion.size,
						&newRegion.protection,
						&newRegion.maxProtection,
						&newRegion.inheritance,
						&newRegion.shared,
						&newRegion.objectName,
						&newRegion.offset);
		if (!error && (newRegion.protection & VM_PROT_READ)) {
			error = vm_read(	task,
							newRegion.address,
							newRegion.size,
							&newRegion.data,
							&newRegion.dataCount);
			if (!error) {
				newRegion.maxAddress = newRegion.address + newRegion.size;
				newRegion.maxData = newRegion.data + newRegion.size;
				newRegion.displacement = newRegion.data - newRegion.address;
				[regions addElement: &newRegion];
				region = &newRegion;
			} else
				region = NULL;
		} else
			region = NULL;
	}
	return region;
}

-invalidate
{
	int count;
	Region *region;
	for (count = [regions count]; count; count--) {
		region = [regions elementAt: (count - 1)];
		vm_deallocate(task_self(), region->data, region->dataCount);
	}
	[regions empty];
	return self;
}

-(Region *)oldRegionFor: (void *)pointer
{
	Region *region = NULL;
	int count;
	BOOL found;
	for (count = [regions count], found = NO; count && !found; count--) {
		region = [regions elementAt: (count - 1)];
		if ((region->data <= (pointer_t)pointer) 
			&& ((pointer_t)pointer < region->maxData))
			found = YES;
	}
	if (found)
		return region;
	else
		return NULL;
}

-(void *)originalPointerFor: (void *)pointer
{	
	Region *region = [self oldRegionFor: pointer];
	if (region)
		return (region->address + (pointer - region->data));
	else 
		return NULL;
}

-(void *)pointerFor: (void *)pointer
{
	Region *region;
	if (pointer && (region = [self  regionFor: pointer])) 
		return (void *)((pointer_t)pointer + region->displacement);
	else
		return NULL;
}

-(void *)pointerFor: (void *)pointer withSize: (int)size
{
	Region *region;
	pointer_t newPointer;
	if (pointer && (region = [self regionFor: pointer])) {
		newPointer = (pointer_t)pointer + region->displacement;
		if ((newPointer + size) < region->maxData)
			return (void *)newPointer;
		else
			return NULL;
	} else
		return NULL;
}

-(id)pointerForID: (id)pointer
{
	Region *region;
	pointer_t newID;
	Class theClass;
	if (pointer && (region = [self regionFor: pointer])) {
		newID = (pointer_t)pointer + region->displacement;
		if (	((newID + sizeof(id)) < region->maxData) 
		&&	(theClass = [self pointerFor: ((id)newID)->isa withSize: sizeof(Class)])
		&&	((newID + theClass->instance_size) < region->maxData))
			return (id)newID;
		else
			return NULL;
	} else
		return NULL;
}
			

-(BOOL)getDataAt: (void *)start for: (int)numBytes into: (void *)data
{
	Region *region = [self regionFor: start];
	if (region) {
		if (((pointer_t)start + numBytes) < (region->address + region->size)) {
			memcpy(data, (void *)(region->data + ((pointer_t)start - region->address)), numBytes);
			return YES;
		} else
			return NO;
	} else
		return NO;
}

-(struct mach_header *)getMachHeader
{
	BOOL found;
	Region *region;
	Region newRegion;
	struct mach_header *myHeader = NULL, *hisHeader;
	for (	found = NO, hisHeader = 0x0000; 
		!found; 
		hisHeader = (struct mach_header *)(region->address + region->size)) {
		region = [self regionFor: hisHeader];
		if (region) { 
			myHeader = (struct mach_header *)(region->data + ((pointer_t)hisHeader - region->address));
			if (myHeader->magic == MH_MAGIC)
				found = YES;
		} else {
			newRegion.address = (vm_address_t) hisHeader;
			vm_region(	task, 
						&newRegion.address,
						&newRegion.size,
						&newRegion.protection,
						&newRegion.maxProtection,
						&newRegion.inheritance,
						&newRegion.shared,
						&newRegion.objectName,
						&newRegion.offset);
			region = &newRegion;
		}
	}
	return myHeader;
}

-(struct mach_header **)getMachHeaders
{
	struct mach_header *myHeader, **headers;
	struct fvmlib_command *loadCmd;
	int numHeaders, i, headerIndex;
	myHeader = [self getMachHeader];
	for (	i = 0, numHeaders = 0, loadCmd = (struct fvmlib_command *)(myHeader + 1); 
		i < myHeader->ncmds;
		i++, loadCmd = (struct fvmlib_command *)((char *)loadCmd + loadCmd->cmdsize)) {
		if (loadCmd->cmd == LC_LOADFVMLIB)
			numHeaders++;
	}
	headers = malloc((numHeaders + 2) * sizeof(*headers));
	headers[0] = myHeader;
	for (	headerIndex = 1, loadCmd = (struct fvmlib_command *)(myHeader + 1); 
		headerIndex <= numHeaders;
		loadCmd = (struct fvmlib_command *)((char *)loadCmd + loadCmd->cmdsize)) {
		if (loadCmd->cmd == LC_LOADFVMLIB) {
			headers[headerIndex] = [self pointerFor: (void *)(loadCmd->fvmlib.header_addr)];
			/* snaroff */
			headers[headerIndex]->flags =  
				(char *)[self originalPointerFor:loadCmd] + 
					loadCmd->fvmlib.name.offset;
			headerIndex++;
		}
	}
	headers[headerIndex] = NULL;
	return headers;
}

-(void *)	getSectData: 	(STR)segName 
		section: 		(STR)sectName 
		size: 		(int *)pSize 
		forHeader:	(struct mach_header *)header
{
	void *data = getsectdatafromheader(header, segName, sectName, pSize);
	return [self pointerFor: data];
}

-(void *)	getSectData: 	(STR)segName 
		section: 		(STR)sectName 
		size: 		(int *)pSize 
{
	return [self getSectData: segName section: sectName size: pSize forHeader: [self getMachHeader]];
}

@end

















