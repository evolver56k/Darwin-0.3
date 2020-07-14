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


#import "RegionManager.h"
#import "ObjcImage.h"
#import <sys/loader.h>

struct section *getsectbynamefromheader(struct mach_header *, char *, char *);

@implementation ObjcImage


+ newFromHeader:(struct mach_header *)mh
{
	self = [super new];
	machhdr = mh;
	pagehash = [HashTable newKeyDesc:"i"];
	instanceCachePageHash = [HashTable newKeyDesc:"i"];
	classCachePageHash = [HashTable newKeyDesc:"i"];
	return self;	
}

- regionManager:region
{
	regionManager = region;
	return self;
}

- getModules
{
	Module mod;
	Symtab symtab;

	modPtr = [regionManager 
				getSectData: SEG_OBJC 
				section: SECT_OBJC_MODULES 
				size: &modSize 
				forHeader: machhdr]; 

	//printf("%x\n",modPtr+modSize);

	for (	mod = modPtr; 
		mod < (STR)modPtr + modSize;  
		mod = (Module)((STR)mod + mod->size)) {
			char *name = mod->name;
			int idx;

			/* relocate module */
			mod->name = WARP(mod->name);
			//printf("module = %x, %s (%x, %x)\n",mod,mod->name, UNWARP(mod->name), name);

			if (symtab = mod->symtab = WARP(mod->symtab)) {

				/* collect some statistics */
				n_selrefs += symtab->sel_ref_cnt;
				n_classes += symtab->cls_def_cnt;
				n_categories += symtab->cat_def_cnt;

				/* relocate classes */
				for (idx = 0; idx < symtab->cls_def_cnt; idx++) {

					Class myClass, myMetaClass;
					struct objc_method_list *methods;
					void *page;

	   				page = (unsigned int)symtab->defs[idx] - 
							((unsigned int)symtab->defs[idx] % 0x2000);
	  				[pagehash insertKey:page value:symtab->defs[idx]];

					if (myClass = symtab->defs[idx] = WARP(symtab->defs[idx])) {
						myClass->name = WARP(myClass->name);
						myMetaClass = myClass->isa = WARP(myClass->isa);
						if (CLS_GETINFO(myMetaClass, CLS_INITIALIZED))
							n_classesInUse++;

						/* relocate instance methods */						myClass->methods = methods = WARP(myClass->methods);
						if (methods)
								n_instanceMethods += methods->method_count;

						while (methods && methods->method_next) {
							/* we have some categories */							methods->method_next = methods = 									WARP(methods->method_next);							if (methods)
								n_instanceMethods += methods->method_count;
						}

						/* relocate instance method cache */
						if (myClass->cache) {
								page = (unsigned int)myClass->cache - 
									((unsigned int)myClass->cache % 0x2000);
	  							[instanceCachePageHash
									 insertKey:page value:myClass->cache];

								myClass->cache = WARP(myClass->cache);
																		n_instanceCacheSlots += (myClass->cache->mask+1);
								n_instanceCacheOccupied += myClass->cache->occupied;
								n_instanceCache++;						}

						/* relocate class methods */						myMetaClass->methods = methods = WARP(myMetaClass->methods);
						if (methods)
								n_classMethods += methods->method_count;

						while (methods && methods->method_next) {
							/* we have some categories */							methods->method_next = methods = 									WARP(methods->method_next);							if (methods)
								n_classMethods += methods->method_count;
						}
						
						/* relocate class method cache */
						if (myMetaClass->cache) {
																		page = (unsigned int)myMetaClass->cache - 
									((unsigned int)myMetaClass->cache % 0x2000);
	  							[classCachePageHash
									 insertKey:page value:myMetaClass->cache];

								myMetaClass->cache = WARP(myMetaClass->cache);
																		n_classCacheSlots += (myMetaClass->cache->mask+1);
								n_classCacheOccupied += myMetaClass->cache->occupied;
								n_classCache++;						}

																//printf("class = %s\n",myClass->name);
					}
				}

				/* relocate categories */
				for (idx = 0; idx < symtab->cat_def_cnt; idx++) {

					Category myCategory;

					if (myCategory = symtab->defs[idx + symtab->cls_def_cnt] = 
								WARP(symtab->defs[idx + symtab->cls_def_cnt])) {
						myCategory->category_name = WARP(myCategory->category_name);
						//printf("category = %s\n",myCategory->category_name);
					}
				}
			}
		}
	printf("\n************************************************"
		"*************************\n");
	if (machhdr->filetype == MH_EXECUTE)
		printf("\nApplication image statistics\n\n");
	else if (machhdr->filetype == MH_FVMLIB) {
		printf("\nShared library image statistics for %s\n\n", WARP((char *)machhdr->flags));
	}
		
	printf("%d classes (%d categories) - class descriptors spread across %d pages.\n", 
				n_classes, n_categories, [pagehash count]);
	printf("%d classes currently in use\n", n_classesInUse);
	printf("%d instance methods, %d class methods\n",n_instanceMethods, n_classMethods);
	printf("%d selector references\n",n_selrefs);

	printf("\nMethod Cache Info (caches are dynamically allocated):\n\n");
	if (n_instanceCache) {
		printf("%d instance caches - caches spread across %d pages.\n",
				n_instanceCache, [instanceCachePageHash count]);
		printf("%d currently in use / %d method slots (%3.2f%% full).\n",
			n_instanceCacheOccupied, n_instanceCacheSlots,
			(float)n_instanceCacheOccupied/n_instanceCacheSlots * 100);
		printf("%d bytes to cache instance methods ",
			((n_instanceCache * sizeof(struct objc_cache)) + 
						(n_instanceCacheSlots * sizeof(Method))));
	  	printf("(%d bytes per class)\n",
				((n_instanceCache * sizeof(struct objc_cache)) + 
						(n_instanceCacheSlots * sizeof(Method)))/n_instanceCache);
	}
	if (n_classCache) {
		printf("%d class caches - caches spread across %d pages.\n",
				n_classCache, [classCachePageHash count]);
		printf("%d currently in use / %d method slots (%3.2f%% full).\n",
			n_classCacheOccupied, n_classCacheSlots,
			(float)n_classCacheOccupied/n_classCacheSlots * 100);
		printf("%d bytes to cache class methods ",
			((n_classCache * sizeof(struct objc_cache)) + 
						(n_classCacheSlots * sizeof(Method))));
	  	printf("(%d bytes per meta class)\n",
				((n_classCache * sizeof(struct objc_cache)) + 
						(n_classCacheSlots * sizeof(Method)))/n_classCache);
	}
	return self;
}


@end












