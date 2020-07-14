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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdeShared.h
 *
 * Definitions shared between the driver and the driver's inspector.
 *
 * HISTORY:
 * 1-Feb-1998	Joe Liu
 *	Created.
 */

#define IDE_MASTER_KEY				"Master"
#define IDE_SLAVE_KEY				"Slave"
#define IDE_MASTER_KEY_SEC			"Master Secondary"
#define IDE_SLAVE_KEY_SEC			"Slave Secondary"
#define MULTIPLE_SECTORS_ENABLE		"Multiple Sectors"
#define HOST_IORDY_SUPPORT			"IOCHRDY Support"	/* obsolete */
#define EIDE_SUPPORT 				"EIDE Support"
#define DRIVE_PARAMETERS 			"Disk Geometry"
#define ADDRESS_MODE 				"Address Mode"
#define BUS_TYPE 					"Bus Type"
#define MODES_MASK_MASTER			"Master Modes Mask"
#define MODES_MASK_SLAVE			"Slave Modes Mask"
#define MODES_MASK_MASTER_SEC		"Master Modes Mask Secondary"
#define MODES_MASK_SLAVE_SEC		"Slave Modes Mask Secondary"

// override[] Values
#define DEVICE_AUTO  (0)
#define DEVICE_ATA   (1)
#define DEVICE_ATAPI (2)
#define DEVICE_NONE  (3)

#define OVERRIDE_TABLE_SIZE		4

static const char * overrideTable[]= {
   "Auto",
#define OVERRIDE_AUTO	0
   "ATA",
   "ATAPI",
   "None",			// None must be the last entry
};

/*
 * Obsolete override parameters.
 */
#define ATA_LOCATION 			"ATA Drive"
#define ATAPI_LOCATION 			"ATAPI Device"

/*
 * This key was used in 3.3 release driver. We check for it for backwards
 * compatibility. This gets treated the same way as DRIVE_PARAMETERS. However
 * this can only enable the use of disk geometry. 
 */
#define USE_DISK_GEOMETRY 		"Use Disk Geometry"
