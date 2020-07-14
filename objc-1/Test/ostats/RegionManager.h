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

#import <mach.h>

#import <objc/Storage.h>

typedef struct _Region {
	vm_address_t	address;
	vm_size_t	size;
	vm_prot_t	protection;
	vm_prot_t	maxProtection;
	vm_inherit_t	inheritance;
	boolean_t	shared;
	port_t		objectName;
	vm_offset_t	offset;
	pointer_t		data;
	unsigned int	dataCount;
	vm_address_t	maxAddress;
	long			displacement;
	pointer_t		maxData;
} Region;

#define WARP(p) \
	((typeof(p))[regionManager pointerFor: p withSize: sizeof(*p)])
	
#define WARPID(id) \
	([regionManager pointerForID: id])
	
#define UNWARP(p) \
	((typeof(p))[regionManager originalPointerFor: p])
	
#define AREF(array, index) \
	(*(typeof(array))[regionManager pointerFor: array + index])
	
@interface RegionManager : Object
{
	Storage *regions;
	vm_task_t task;
}

+newTask: (vm_task_t)theTask;
-(Region *)regionFor: (void *)pointer;
-(void *)pointerFor: (void *)pointer;
-(void *)pointerFor: (void *)pointer withSize: (int)size;
-(void *)originalPointerFor: (void *)pointer;
-(BOOL)getDataAt: (void *)start for: (int)numBytes into: (void *)data;
-(struct mach_header **)getMachHeaders;
-(struct mach_header *)getMachHeader;
-(void *)getSectData: (STR)segName section: (STR)sectName size: (int *)pSize forHeader: (struct mach_header *)header;
-(void *)getSectData: (STR)segName section: (STR)sectName size: (int *)pSize;
-(void *)originalPointerFor: (void *)pointer;
-invalidate;
-(id)pointerForID: (id)pointer;

@end

extern void *getsectdatafromheader(struct mach_header *, char *, char *, int *);













