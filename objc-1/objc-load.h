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
/*
 *	objc-load.h
 *	Copyright 1988, NeXT, Inc.
 */

#ifndef _OBJC_LOAD_H_
#define _OBJC_LOAD_H_

#import "objc.h"
#import "objc-class.h"
#import <streams/streams.h>
#import <mach-o/loader.h>

/* dynamically loading Mach-O object files that contain Objective-C code */

extern long objc_loadModules(
	char *moduleList[], 				/* input */
	NXStream *errorStream,				/* input (optional) */
	void (*loadCallback)(Class, Category),		/* input (optional) */
	struct mach_header **headerAddr,		/* output (optional) */
	char *debugFileName				/* input (optional) */
);

extern long objc_unloadModules(
	NXStream *errorStream,				/* input (optional) */
	void (*unloadCallback)(Class, Category)		/* input (optional) */
);

#endif /* _OBJC_LOAD_H_ */
