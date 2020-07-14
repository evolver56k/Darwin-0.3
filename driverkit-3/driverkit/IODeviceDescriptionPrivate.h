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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * IODeviceDescription private interface.
 *
 * HISTORY
 *
 * 19 Jan 1994 David E. Bohman at NeXT
 *	Created.
 */

@interface IODeviceDescription(Private)

- _initWithDelegate:delegate;
- _delegate;
- (void *)_fetchItemList:	list
	    returnedNum:	(unsigned int *)returnedNum;
- (void *)_fetchRangeList:	list
	    returnedNum:	(unsigned int *)returnedNum;

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length
	selector:(SEL)selector
	isString:(BOOL)isString;

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen;
- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen;
- (char *) nodeName;

@end

#define MEM_MAPS_KEY 		"Memory Maps"
#define IRQ_LEVELS_KEY		"IRQ Levels"

