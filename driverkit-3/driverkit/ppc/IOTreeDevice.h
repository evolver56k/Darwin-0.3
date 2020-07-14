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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in.
 */

#import <driverkit/IODirectDevice.h>
#import <driverkit/ppc/IOPPCDeviceDescriptionPriv.h>

#import <driverkit/ppc/IOPropertyTable.h>

struct IOApertureInfo {
    IOPhysicalAddress	physical;
    IOLogicalAddress	logical;
    IOByteCount		length;
    IOCacheMode		cacheMode;
    IOOptionBits	usage;
};
typedef struct IOApertureInfo IOApertureInfo;

/* Remove aliases from the head of the path 
 * & returns difference in length */
int IODealiasPath( char * outputPath, const char * inputPath);

/* Replace head of path with matching alias */ 
void IOAliasPath( char * fullPath);

@interface IOTreeDevice : IOPPCDeviceDescription
{
@private
    void	*_tree_private;
    int		_IOTreeDevice_reserved[8];
}

- setDelegate:delegate;
- parent;

- propertyTable;
- (char *) nodeName;
- publish;
- (BOOL) match:(const char *)key location:(const char *)location;
- taken:(BOOL)taken;
+ findMatchingDevice:(const char *)key location:(const char *)location;

- (IOReturn) getApertures:(IOApertureInfo *)info items:(UInt32 *)items;

- getResources;

enum {
	kMaxNVNameLen	= 4,
	kMaxNVDataLen	= 8
};

- (IOReturn) readNVRAMProperty:(char *)name
	value:(void *)buffer length:(ByteCount *)length;
- (IOReturn) writeNVRAMProperty:(const char *)name
	value:(void *)value length:(ByteCount)length;
- denyNVRAM:(BOOL)flag;

@end

