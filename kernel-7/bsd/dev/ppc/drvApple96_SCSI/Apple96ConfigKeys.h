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
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96ConfigKeys.h - Configuration key strings shared with the SCSI Inspector.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Configuration keys for Apple 53C96 SCSI driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources.
 * 1997.03.27	MM		Curio does not support fast or synchronous.
 */

/*
 * These are accessible by SCSIInspector.
 */
#define CMD_QUEUE_ENABLE	"Cmd Queueing"

/*
 * These are only accessible by Configure's Expert mode.
 */
#define EXTENDED_TIMING		"Extended Timing"
#define SCSI_CLOCK_RATE		"SCSI Clock Rate"	/* in MHz */