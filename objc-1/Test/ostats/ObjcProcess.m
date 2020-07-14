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

#import <objc/Storage.h>
#import <objc/List.h>
#import <objc/HashTable.h>
#import <objc/objc-runtime.h>


#import <sys/file.h>
#import <string.h>
#import <sys/loader.h>
#import <sys/table.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <ldsyms.h>
#import <mach.h>
#import <stdlib.h>

#import "RegionManager.h"
#import "ObjcProcess.h"
#import "ObjcImage.h"
#import <sys/loader.h>

struct section *getsectbynamefromheader(struct mach_header *, char *, char *);
int table(int, int, void *, int, int);
int open(const char *, int, ...);
int close(int);
kern_return_t map_fd(int, vm_offset_t, vm_offset_t *, boolean_t, vm_size_t);

@implementation ObjcProcess


+ newFromTask:(vm_task_t)task
{
	struct mach_headers **machhdrs;

	self = [super new];
	regionManager = [RegionManager newTask:task];
	imageList = [List new];

	machhdrs = [regionManager getMachHeaders];
	while (*machhdrs) {
		id image = [ObjcImage newFromHeader:*machhdrs];
		[[image regionManager:regionManager] getModules];
		[imageList addObject:image];
		machhdrs++;
	}
	return self;
}


#if 0

-(List *)getClasses
{
	struct objc_symtab *memSymtab;
	struct objc_module *memModules, *memModule, *endOfModules;
	struct mach_header *memHeader;
	int numClass, headerIndex;
	Class memClass, memMetaClass;
	List *orphans;
	int memSize;
	orphans = [List new];
	for (headerIndex = 0; headerIndex < machhdrs[headerIndex]; headerIndex++) {
	
		memHeader = machhdrs[headerIndex];

		memModules = [regionManager 
				getSectData: SEG_OBJC 
				section: SECT_OBJC_MODULES 
				size: &memSize 
				forHeader: memHeader]; 

		endOfModules =  (struct objc_module *)((char *)memModules + memSize);																	
		for (	memModule = memModules;
			memModule < endOfModules; 
			memModule =  (struct objc_module *)((STR)memModule + memModule->size)) {
			if (memSymtab = WARP(memModule->symtab)) {
				for (numClass = 0; numClass < memSymtab->cls_def_cnt; numClass++) {
					if (memClass = WARP(memSymtab->defs[numClass])) {
						[self adopt: orphans with: memClass];
					}
				}
			}
		}
	}
	return orphans;
}

#endif


@end












