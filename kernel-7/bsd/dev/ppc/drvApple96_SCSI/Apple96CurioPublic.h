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

/**
 * Copyright (c) 1994-1996 NeXT Software, Inc.  All rights reserved. 
 * Copyright 1993-1995 by Apple Computer, Inc., all rights reserved.
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96CurioPublic.h - Hardware (chip) definitions for Apple 53c96 SCSI interface.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver
 * using design concepts from Copland DR2 Curio and MESH SCSI plugins.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"

@interface Apple96_SCSI(CurioPublic)

- (IOReturn)		hardwareChipSelfTest;
- (IOReturn)		curioHardwareReset
						: (Boolean) resetSCSIBus
			reason		: (const char *) reason;
- (UInt32)			curioDMAComplete;

- (void)			logRegisters
						: (Boolean) examineInterruptRegister
			reason		: (const char *) reason;
- (void)			logCommand
						: (const CommandBuffer *) commandPtr
			logMemory	: (Boolean) isLogMemoryNeeded
			reason		: (const char *) reason;
- (void)	    logMemory
						: (const void *) buffer
			length		: (UInt32) length
			reason		: (const char *) reason;

- (void)			logTimestamp
						: (const char *) reason;

@end /* Apple96_SCSI(CurioPublic) */




		
