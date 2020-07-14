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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * i386/kernelDriver.h - i386 kernel-only driverkit functions.
 *
 * HISTORY
 * 1-Apr-93    Doug Mitchell at NeXT
 *      Created. 
 */

#ifdef	KERNEL

/*
 * Allocate memory guranteed to be in the low 16 megabytes of physical 
 * memory. Used when performing DMA which must deal with only 24 bits 
 * of address.
 *
 * Zero will also be returned if no low memory can be allocated. 
 *
 * Memory allocated by IOMallocLow() must be freed by IOFreeLow(). 
 */
void *IOMallocLow(int size);
void IOFreeLow(void *p, int size);

#endif	KERNEL
