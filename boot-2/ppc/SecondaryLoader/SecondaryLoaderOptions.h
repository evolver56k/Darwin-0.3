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
#ifndef SECONDARY_LOADER_OPTIONS_H
#define SECONDARY_LOADER_OPTIONS_H 1

// Define to be able to load from HFS volumes
#ifndef HFS_SUPPORT
#define HFS_SUPPORT 0
#endif

// Make nonzero for trendy serialport style progress bar...
#define kShowProgress 1

// Define nonzero to ignore all secondary loader extensions
#define defeatExtensions 1

#define kDebugLotsDef	0

extern unsigned int slDebugFlag;
enum {
	kDebugLots		= 0x00000001,
	kDebugSARLD		= 0x00000002,
	kDebugLoadables		= 0x00000004,
	kDisableDisplays       	= 0x00010000,	// don't open displays
	kDisableNDRVs		= 0x00020000,	// don't load ndrv's
	kDisableDrivers		= 0x00040000,	// don't load drivers & tables
	kForceDrivers		= 0x00080000,	// force drivers with
						//   "Boot Driver" = "*" (not "") to load 
};


#endif
