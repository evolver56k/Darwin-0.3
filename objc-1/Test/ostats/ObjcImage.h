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
#import <objc/objc-runtime.h>

@interface ObjcImage : Object
{
	id regionManager;
	struct mach_header *machhdr;
	Module modPtr;
	int  modSize;

	int n_selrefs;
	int n_classes;
	int n_categories;
	int n_instanceMethods;
	int n_classMethods;
	int n_classesInUse;

	int n_instanceCache;
	int n_instanceCacheSlots;
	int n_instanceCacheOccupied;

	int n_classCache;
	int n_classCacheSlots;
	int n_classCacheOccupied;

	id pagehash;
	id instanceCachePageHash;
	id classCachePageHash;
}


@end







