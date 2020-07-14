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
/* 	Property names for getStringPropertyList 
*/

#ifndef __IOPROPERTIES__
#define __IOPROPERTIES__ 1


#define IOPropMachineClass	"IOMachineClass"
#define IOPropMachineGestalt	"IOMachineGestalt"
#define IOPropSlotConfiguration	"IOSlotConfiguration"
#define IOPropDeviceClass	"IODeviceClass"
#define IOPropDeviceType	"IODeviceType"
#define IOPropDriverNames	"IODriverNames"
#define IOPropDevicePath	"IODevicePath"
#define IOPropUnit		"IOUnit"
#define IOPropConnectorName	"IOConnectorName"
#define IOPropSlotName		"IOSlotName"
#define IOPropPartitionNumber	"IOPartitionNumber"

#define IOClassSCSIController	"IOSCSIController"
#define IOClassIDEController	"IOIDEController"
#define IOClassATAPIController	"IOATAPIController"
#define IOClassBlock		"IOBlock"
#define IOClassDisk		"IODisk"
#define IOClassDiskPartition   	"IODiskPartition"
#define IOClassNetwork		"IONetwork"
#define IOClassEthernet		"IOEthernet"
#define IOClassFramebuffer	"IOFramebuffer"

#define IOTypePPC		"IOPPC"
#define IOTypePCComp		"IOPCComp"
#define IOTypePCI		"IOPCI"
#define IOTypeSCSI		"IOSCSI"
#define IOTypeIDE		"IOIDE"
#define IOTypeATAPI		"IOATAPI"
#define IOTypeFloppy		"IOFloppy"

// SCSI / ATAPI devices
#define IOTypeTape		"IOTape"
#define IOTypePrinter		"IOPrinter"
#define IOTypeWORM		"IOWorm"
#define IOTypeCDROM		"IOCDROM"
#define IOTypeScanner		"IOScanner"
#define IOTypeOptical		"IOOptical"
#define IOTypeChanger		"IOChanger"

// Connectors
#define IOConnectInternal    	"Internal"
#define IOConnectExternal    	"External"
#define IOConnectIntExt    	"Internal/External"

// Block devices
#define IOTypeRemovableDisk    	"IORemovableDisk"
#define IOTypeWriteProtectedDisk "IOWriteProtectedDisk"

// Disk Partitions
#define IOTypeUFS	    	"IOUFS"
#define IOTypeHFS	    	"IOHFS"

// IOPropSlotConfiguration: configuration
#define IO_SLOTS_UNKNOWN	0
#define IO_SLOTS_VERTICAL	1
#define IO_SLOTS_HORIZONTAL	2

// IOPropSlotConfiguration: direction
#define IO_SLOTS_TOP2BOTTOM    	0
#define IO_SLOTS_LEFT2RIGHT	0
#define IO_SLOTS_BOTTOM2TOP    	1
#define IO_SLOTS_RIGHT2LEFT	1

typedef struct {
	unsigned int	spare:16;
	unsigned int	configuration:7;
	unsigned int	direction :1;
	unsigned int	numberSlots:8;
} IOSlotConfiguration;

#endif  // __IOPROPERTIES__
