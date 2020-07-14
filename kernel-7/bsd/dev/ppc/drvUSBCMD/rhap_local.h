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

typedef UInt16 UnitNumber;

typedef UInt32 DriverOpenCount;

typedef SInt16 DriverRefNum;

typedef SInt16 DriverFlags;

typedef UInt32 IOCommandCode;


enum {
	kOpenCommand				= 0,
	kCloseCommand				= 1,
	kReadCommand				= 2,
	kWriteCommand				= 3,
	kControlCommand				= 4,
	kStatusCommand				= 5,
	kKillIOCommand				= 6,
	kInitializeCommand			= 7,							/* init driver and device*/
	kFinalizeCommand			= 8,							/* shutdown driver and device*/
	kReplaceCommand				= 9,							/* replace an old driver*/
	kSupersededCommand			= 10							/* prepare to be replaced by a new driver*/
};
//naga
//typedef KernelID IOCommandID;

typedef UInt32 IOCommandKind;
//naga
typedef int RegEntryID;

enum {
	kSynchronousIOCommandKind	= 0x00000001,
	kAsynchronousIOCommandKind	= 0x00000002,
	kImmediateIOCommandKind		= 0x00000004
};

struct DriverInitInfo {
	DriverRefNum					refNum;
	RegEntryID						deviceEntry;
};
struct DriverFinalInfo {
	DriverRefNum					refNum;
	RegEntryID						deviceEntry;
};
typedef struct DriverInitInfo DriverInitInfo, *DriverInitInfoPtr;

typedef struct DriverInitInfo DriverReplaceInfo, *DriverReplaceInfoPtr;

typedef struct DriverFinalInfo DriverFinalInfo, *DriverFinalInfoPtr;

typedef struct DriverFinalInfo DriverSupersededInfo, *DriverSupersededInfoPtr;

/* Contents are command specific*/
union IOCommandContents {
	//nagaParmBlkPtr						pb;
	DriverInitInfoPtr				initialInfo;
	DriverFinalInfoPtr				finalInfo;
	DriverReplaceInfoPtr			replaceInfo;
	DriverSupersededInfoPtr			supersededInfo;
};
typedef union IOCommandContents IOCommandContents;

//nagatypedef OSErr (DriverEntryPoint)(AddressSpaceID SpaceID, IOCommandID CommandID, IOCommandContents Contents, IOCommandCode Code, IOCommandKind Kind);

/* Driver Typing Information Used to Match Drivers With Devices */
struct DriverType {
	Str31							nameInfoStr;				/* Driver Name/Info String*/
	NumVersion						version;					/* Driver Version Number*/
};
typedef struct DriverType DriverType, *DriverTypePtr;

/* OS Runtime Information Used to Setup and Maintain a Driver's Runtime Environment */
typedef OptionBits RuntimeOptions;


enum {
	kDriverIsLoadedUponDiscovery = 0x00000001,					/* auto-load driver when discovered*/
	kDriverIsOpenedUponLoad		= 0x00000002,					/* auto-open driver when loaded*/
	kDriverIsUnderExpertControl	= 0x00000004,					/* I/O expert handles loads/opens*/
	kDriverIsConcurrent			= 0x00000008,					/* supports concurrent requests*/
	kDriverQueuesIOPB			= 0x00000010					/* device manager doesn't queue IOPB*/
};

struct DriverOSRuntime {
	RuntimeOptions					driverRuntime;				/* Options for OS Runtime*/
	Str31							driverName;					/* Driver's name to the OS*/
	UInt32							driverDescReserved[8];		/* Reserved area*/
};
typedef struct DriverOSRuntime DriverOSRuntime, *DriverOSRuntimePtr;

/* OS Service Information Used To Declare What APIs a Driver Supports */
typedef UInt32 ServiceCount;

struct DriverServiceInfo {
	OSType							serviceCategory;			/* Service Category Name*/
	OSType							serviceType;				/* Type within Category*/
	NumVersion						serviceVersion;				/* Version of service*/
};
typedef struct DriverServiceInfo DriverServiceInfo, *DriverServiceInfoPtr;

struct DriverOSService {
	ServiceCount					nServices;					/* Number of Services Supported*/
	DriverServiceInfo				service[1];					/* The List of Services (at least one)*/
};
typedef struct DriverOSService DriverOSService, *DriverOSServicePtr;

/* Categories */

enum {
	kServiceCategoryDisplay		= 'disp',						/* Display Manager*/
	kServiceCategoryOpenTransport = 'otan',						/* Open Transport*/
	kServiceCategoryBlockStorage = 'blok',						/* Block Storage*/
	kServiceCategoryNdrvDriver	= 'ndrv',						/* Generic Native Driver*/
	kServiceCategoryScsiSIM		= 'scsi'
};

/* Ndrv ServiceCategory Types */
enum {
	kNdrvTypeIsGeneric			= 'genr',						/* generic*/
	kNdrvTypeIsVideo			= 'vido',						/* video*/
	kNdrvTypeIsBlockStorage		= 'blok',						/* block storage*/
	kNdrvTypeIsNetworking		= 'netw',						/* networking*/
	kNdrvTypeIsSerial			= 'serl',						/* serial*/
	kNdrvTypeIsSound			= 'sond',						/* sound*/
	kNdrvTypeIsBusBridge		= 'brdg'
};

/*	The Driver Description */
enum {
	kTheDescriptionSignature	= 'mtej'
};

typedef UInt32 DriverDescVersion;


enum {
	kInitialDriverDescriptor	= 0
};

struct DriverDescription {
	OSType							driverDescSignature;		/* Signature field of this structure*/
	DriverDescVersion				driverDescVersion;			/* Version of this data structure*/
	DriverType						driverType;					/* Type of Driver*/
	DriverOSRuntime					driverOSRuntimeInfo;		/* OS Runtime Requirements of Driver*/
	DriverOSService					driverServices;				/* Apple Service API Membership*/
};
typedef struct DriverDescription DriverDescription, *DriverDescriptionPtr;

#if 0 
/* Driver Loader API */
#define DECLARE_DRIVERDESCRIPTION(N_ADDITIONAL_SERVICES)  \
	struct {					\
	DriverDescription	fixed;	\
	DriverServiceInfo	additional_service[N_ADDITIONAL_SERVICES-1]; \
	};
extern SInt16 HigherDriverVersion(NumVersion *driverVersion1, NumVersion *driverVersion2);
extern OSErr VerifyFragmentAsDriver(CFragConnectionID fragmentConnID, DriverEntryPointPtr *fragmentMain, DriverDescriptionPtr *driverDesc);
extern OSErr GetDriverMemoryFragment(Ptr memAddr, long length, ConstStr63Param fragName, CFragConnectionID *fragmentConnID, DriverEntryPointPtr *fragmentMain, DriverDescriptionPtr *driverDesc);
extern OSErr GetDriverDiskFragment(FSSpecPtr fragmentSpec, CFragConnectionID *fragmentConnID, DriverEntryPointPtr *fragmentMain, DriverDescriptionPtr *driverDesc);
extern OSErr InstallDriverFromFragment(CFragConnectionID fragmentConnID, RegEntryIDPtr device, UnitNumber beginningUnit, UnitNumber endingUnit, DriverRefNum *refNum);
extern OSErr InstallDriverFromFile(FSSpecPtr fragmentSpec, RegEntryIDPtr device, UnitNumber beginningUnit, UnitNumber endingUnit, DriverRefNum *refNum);
extern OSErr InstallDriverFromMemory(Ptr memory, long length, ConstStr63Param fragName, RegEntryIDPtr device, UnitNumber beginningUnit, UnitNumber endingUnit, DriverRefNum *refNum);
extern OSErr InstallDriverFromDisk(Ptr theDriverName, RegEntryIDPtr theDevice, UnitNumber theBeginningUnit, UnitNumber theEndingUnit, DriverRefNum *theRefNum);
extern OSErr FindDriversForDevice(RegEntryIDPtr device, FSSpec *fragmentSpec, DriverDescription *fileDriverDesc, Ptr *memAddr, long *length, StringPtr fragName, DriverDescription *memDriverDesc);
extern OSErr FindDriverCandidates(RegEntryIDPtr deviceID, Ptr *propBasedDriver, RegPropertyValueSize *propBasedDriverSize, StringPtr deviceName, DriverType *propBasedDriverType, Boolean *gotPropBasedDriver, FileBasedDriverRecordPtr fileBasedDrivers, ItemCount *nFileBasedDrivers);
extern OSErr ScanDriverCandidates(RegEntryIDPtr deviceID, FileBasedDriverRecordPtr fileBasedDrivers, ItemCount nFileBasedDrivers, FileBasedDriverRecordPtr matchingDrivers, ItemCount *nMatchingDrivers);
extern OSErr GetDriverForDevice(RegEntryIDPtr device, CFragConnectionID *fragmentConnID, DriverEntryPointPtr *fragmentMain, DriverDescriptionPtr *driverDesc);
extern OSErr InstallDriverForDevice(RegEntryIDPtr device, UnitNumber beginningUnit, UnitNumber endingUnit, DriverRefNum *refNum);
extern OSErr SetDriverClosureMemory(CFragConnectionID fragmentConnID, Boolean holdDriverMemory);
extern OSErr ReplaceDriverWithFragment(DriverRefNum theRefNum, CFragConnectionID fragmentConnID);
extern OSErr GetDriverInformation(DriverRefNum refNum, UnitNumber *unitNum, DriverFlags *flags, DriverOpenCount *count, StringPtr name, RegEntryID *device, CFragHFSLocator *driverLoadLocation, CFragConnectionID *fragmentConnID, DriverEntryPointPtr *fragmentMain, DriverDescription *driverDesc);
extern OSErr OpenInstalledDriver(DriverRefNum refNum, SInt8 ioPermission);
extern OSErr RenameDriver(DriverRefNum refNum, StringPtr newDriverName);
extern OSErr RemoveDriver(DriverRefNum refNum, Boolean immediate);
extern OSErr LookupDrivers(UnitNumber beginningUnit, UnitNumber endingUnit, Boolean emptyUnits, ItemCount *returnedRefNums, DriverRefNum *refNums);
extern UnitNumber HighestUnitNumber(void);
extern OSErr DriverGestaltOn(DriverRefNum refNum);
extern OSErr DriverGestaltOff(DriverRefNum refNum);
extern Boolean DriverGestaltIsOn(DriverFlags flags);

enum {
	uppControlPanelDefProcInfo = kPascalStackBased
		 | RESULT_SIZE(SIZE_CODE(sizeof(long)))
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short)))
		 | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short)))
		 | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(short)))
		 | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(EventRecord*)))
		 | STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(long)))
		 | STACK_ROUTINE_PARAMETER(7, SIZE_CODE(sizeof(DialogPtr)))
};
#endif
