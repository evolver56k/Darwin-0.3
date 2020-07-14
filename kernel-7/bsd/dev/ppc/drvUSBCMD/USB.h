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
 	File:		USB.h
 
 	Contains:	Public API for USB Services Library (and associated components)
 
 	Version:	
 
 	DRI:		Craig Keithley
 
 	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				USB.i
 					Revision:		71
 					Dated:			10/29/98
 					Last change by:	TC
 					Last comment:	[2280793]  Add constant kUSBDeviceDescriptorLength.
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __USB__
#define __USB__

#ifndef __MACTYPES__
#include "mactypes.h"
#endif
#ifndef __NAMEREGISTRY__
//#include <NameRegistry.h>
#endif
#ifndef __CODEFRAGMENTS__
//#include <CodeFragments.h>
#endif
#ifndef __DEVICES__
//#include <Devices.h>
#endif



#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

/* ************* Constants ************* */

enum {
	kUSBNoErr					= 0,
	kUSBNoTran					= 0,
	kUSBNoDelay					= 0,
	kUSBPending					= 1,							/* */
																/* USB assigned error numbers in range -6900 .. -6999 */
	kUSBBaseError				= -7000,						/* */
																/* USB Services Errors */
	kUSBInternalErr				= -6999,						/* Internal error */
	kUSBUnknownDeviceErr		= -6998,						/*  device ref not recognised */
	kUSBUnknownPipeErr			= -6997,						/*  Pipe ref not recognised */
	kUSBTooManyPipesErr			= -6996,						/*  Too many pipes */
	kUSBIncorrectTypeErr		= -6995,						/*  Incorrect type */
	kUSBRqErr					= -6994,						/*  Request error */
	kUSBUnknownRequestErr		= -6993,						/*  Unknown request */
	kUSBTooManyTransactionsErr	= -6992,						/*  Too many transactions */
	kUSBAlreadyOpenErr			= -6991,						/*  Already open */
	kUSBNoDeviceErr				= -6990,						/*  No device */
	kUSBDeviceErr				= -6989,						/*  Device error */
	kUSBOutOfMemoryErr			= -6988,						/*  Out of memory */
	kUSBNotFound				= -6987,						/*  Not found */
	kUSBPBVersionError			= -6986,						/*  Wrong pbVersion */
	kUSBPBLengthError			= -6985,						/*  pbLength too small */
	kUSBCompletionError			= -6984,						/*  no completion routine specified */
	kUSBFlagsError				= -6983,						/*  Unused flags not zeroed */
	kUSBAbortedError			= -6982,						/*  Pipe aborted */
	kUSBNoBandwidthError		= -6981,						/*  Not enough bandwidth available */
	kUSBPipeIdleError			= -6980,						/*  Pipe is Idle, it will not accept transactions */
	kUSBPipeStalledError		= -6979,						/*  Pipe has stalled, error needs to be cleared */
	kUSBUnknownInterfaceErr		= -6978,						/*  Interface ref not recognised */
	kUSBDeviceBusy				= -6977,						/*  Device is already being configured */
	kUSBDevicePowerProblem		= -6976,						/*  Device has a power problem */
																/* */
																/* USB Manager Errors */
	kUSBBadDispatchTable		= -6950,						/* Improper driver dispatch table */
	kUSBUnknownNotification		= -6949,						/* Notification type not defined */
	kUSBQueueFull				= -6948,						/* Internal queue maxxed */
																/* */
																/* Hardware Errors */
																/* Note pipe stalls are communication */
																/* errors. The affected pipe can not */
																/* be used until USBClearPipeStallByReference  */
																/* is used */
																/* kUSBEndpointStallErr is returned in */
																/* response to a stall handshake */
																/* from a device. The device has to be */
																/* cleared before a USBClearPipeStallByReference */
																/* can be used */
	kUSBLinkErr					= -6916,
	kUSBCRCErr					= -6915,						/*  Pipe stall, bad CRC */
	kUSBBitstufErr				= -6914,						/*  Pipe stall, bitstuffing */
	kUSBDataToggleErr			= -6913,						/*  Pipe stall, Bad data toggle */
	kUSBEndpointStallErr		= -6912,						/*  Device didn't understand */
	kUSBNotRespondingErr		= -6911,						/*  Pipe stall, No device, device hung */
	kUSBPIDCheckErr				= -6910,						/*  Pipe stall, PID CRC error */
	kUSBWrongPIDErr				= -6909,						/*  Pipe stall, Bad or wrong PID */
	kUSBOverRunErr				= -6908,						/*  Packet too large or more data than buffer */
	kUSBUnderRunErr				= -6907,						/*  Less data than buffer */
	kUSBRes1Err					= -6906,
	kUSBRes2Err					= -6905,
	kUSBBufOvrRunErr			= -6904,						/*  Host hardware failure on data in, PCI busy? */
	kUSBBufUnderRunErr			= -6903,						/*  Host hardware failure on data out, PCI busy? */
	kUSBNotSent1Err				= -6902,						/*  Transaction not sent */
	kUSBNotSent2Err				= -6901							/*  Transaction not sent */
};


enum {
																/* Flags */
	kUSBTaskTimeFlag			= 1,
	kUSBHubPower				= 2,
	kUSBPowerReset				= 4,
	kUSBHubReaddress			= 8,
	kUSBAddressRequest			= 16
};


enum {
																/* Hub messages */
	kUSBHubPortResetRequest		= 1
};

/* ************* types ************* */

typedef SInt32 							USBReference;
typedef USBReference 					USBDeviceRef;
typedef USBReference 					USBInterfaceRef;
typedef USBReference 					USBPipeRef;
typedef USBReference 					USBBusRef;
typedef UInt32 							USBPipeState;
typedef UInt32 							USBCount;
typedef UInt32 							USBFlags;
typedef UInt8 							USBRequest;
typedef UInt8 							USBDirection;
typedef UInt8 							USBRqRecipient;
typedef UInt8 							USBRqType;
typedef UInt16 							USBRqIndex;
typedef UInt16 							USBRqValue;



struct usbControlBits {
	UInt8 							BMRequestType;
	UInt8 							BRequest;
	USBRqValue 						WValue;
	USBRqIndex 						WIndex;
	UInt16 							reserved4;
};
typedef struct usbControlBits			usbControlBits;

struct USBIsocFrame {
	OSStatus 						frStatus;
	UInt16 							frReqCount;
	UInt16 							frActCount;
};
typedef struct USBIsocFrame				USBIsocFrame;

struct usbIsocBits {
	USBIsocFrame *					FrameList;
	UInt32 							NumFrames;
};
typedef struct usbIsocBits				usbIsocBits;

struct usbHubBits {
	UInt32 							Request;
	UInt32 							Spare;
};
typedef struct usbHubBits				usbHubBits;
typedef struct USBPB 					USBPB;
typedef CALLBACK_API_C( void , USBCompletion )(USBPB *pb);

struct USBPB {

	void *							qlink;
	UInt16 							qType;
	UInt16 							pbLength;
	UInt16 							pbVersion;
	UInt16 							reserved1;
	UInt32 							reserved2;

	OSStatus 						usbStatus;
	USBCompletion 					usbCompletion;
	UInt32 							usbRefcon;

	USBReference 					usbReference;

	void *							usbBuffer;
	USBCount 						usbReqCount;
	USBCount 						usbActCount;

	USBFlags 						usbFlags;

    union{

	usbControlBits 					cntl;
 

	usbIsocBits						isoc;

	usbHubBits						hub;

    }usb;

	UInt32 							usbFrame;

	UInt8 							usbClassType;
	UInt8 							usbSubclass;
	UInt8 							usbProtocol;
	UInt8 							usbOther;

	UInt32 							reserved6;
	UInt16 							reserved7;
	UInt16 							reserved8;

};

#if !defined(OLDUSBNAMES)
#define OLDUSBNAMES 0
#endif

//naga#if OLDUSBNAMES
#define usbBMRequestType  usb.cntl.BMRequestType
#define usbBRequest       usb.cntl.BRequest
#define usbWValue         usb.cntl.WValue
#define usbWIndex         usb.cntl.WIndex
//naga#endif
//naga  
typedef int CFragConnectionID;


struct uslReq {
	USBDirection 					usbDirection;
	USBRqType 						usbType;
	USBRqRecipient 					usbRecipient;
	USBRequest 						usbRequest;
};
typedef struct uslReq					uslReq;


enum {
																/* BT 19Aug98, bump up to v1.10 for Isoc*/
	kUSBCurrentPBVersion		= 0x0100,						/* v1.00*/
	kUSBIsocPBVersion			= 0x0109,						/* v1.10*/
	kUSBCurrentHubPB			= kUSBIsocPBVersion
};




#define kUSBNoCallBack ((void *)-1L)


typedef UInt8 							bcdUSB;

enum {
	kUSBControl					= 0,
	kUSBIsoc					= 1,
	kUSBBulk					= 2,
	kUSBInterrupt				= 3,
	kUSBAnyType					= 0xFF
};

/* endpoint type */

enum {
	kUSBOut						= 0,
	kUSBIn						= 1,
	kUSBNone					= 2,
	kUSBAnyDirn					= 3
};

/*USBDirection*/

enum {
	kUSBStandard				= 0,
	kUSBClass					= 1,
	kUSBVendor					= 2
};

/*USBRqType*/

enum {
	kUSBDevice					= 0,
	kUSBInterface				= 1,
	kUSBEndpoint				= 2,
	kUSBOther					= 3
};

/*USBRqRecipient*/

enum {
	kUSBRqGetStatus				= 0,
	kUSBRqClearFeature			= 1,
	kUSBRqReserved1				= 2,
	kUSBRqSetFeature			= 3,
	kUSBRqReserved2				= 4,
	kUSBRqSetAddress			= 5,
	kUSBRqGetDescriptor			= 6,
	kUSBRqSetDescriptor			= 7,
	kUSBRqGetConfig				= 8,
	kUSBRqSetConfig				= 9,
	kUSBRqGetInterface			= 10,
	kUSBRqSetInterface			= 11,
	kUSBRqSyncFrame				= 12
};

/*USBRequest*/


enum {
	kUSBDeviceDesc				= 1,
	kUSBConfDesc				= 2,
	kUSBStringDesc				= 3,
	kUSBInterfaceDesc			= 4,
	kUSBEndpointDesc			= 5,
	kUSBHIDDesc					= 0x21,
	kUSBReportDesc				= 0x22,
	kUSBPhysicalDesc			= 0x23,
	kUSBHUBDesc					= 0x29
};

/* descriptorType */

enum {
	kUSBActive					= 0,							/* Pipe can accept new transactions*/
	kUSBIdle					= 1,							/* Pipe will not accept new transactions*/
	kUSBStalled					= 2								/* An error occured on the pipe*/
};


enum {
	kUSB100mAAvailable			= 50,
	kUSB500mAAvailable			= 250,
	kUSB100mA					= 50,
	kUSBAtrBusPowered			= 0x80,
	kUSBAtrSelfPowered			= 0x40,
	kUSBAtrRemoteWakeup			= 0x20
};


enum {
	kUSBRel10					= 0x0100
};

#define USB_CONSTANT16(x)	((((x) >> 8) & 0x0ff) | ((x & 0xff) << 8))

enum {
	kUSBDeviceDescriptorLength	= 0x12
};


struct USBDeviceDescriptor {
	UInt8 							length;
	UInt8 							descType;
	UInt16 							usbRel;
	UInt8 							deviceClass;
	UInt8 							deviceSubClass;
	UInt8 							protocol;
	UInt8 							maxPacketSize;
	UInt16 							vendor;
	UInt16 							product;
	UInt16 							devRel;
	UInt8 							manuIdx;
	UInt8 							prodIdx;
	UInt8 							serialIdx;
	UInt8 							numConf;
	UInt16 							descEnd;					/* was "end", but this is reserved in some languages*/
};
typedef struct USBDeviceDescriptor		USBDeviceDescriptor;
#ifndef OLDCLASSNAMES
#ifndef __cplusplus
#define class deviceClass
#define subClass deviceSubClass
#endif
#endif
typedef USBDeviceDescriptor *			USBDeviceDescriptorPtr;

struct USBDescriptorHeader {
	UInt8 							length;
	UInt8 							descriptorType;
};
typedef struct USBDescriptorHeader		USBDescriptorHeader;
typedef USBDescriptorHeader *			USBDescriptorHeaderPtr;

struct USBConfigurationDescriptor {
	UInt8 							length;
	UInt8 							descriptorType;
	UInt16 							totalLength;
	UInt8 							numInterfaces;
	UInt8 							configValue;
	UInt8 							configStrIndex;
	UInt8 							attributes;
	UInt8 							maxPower;
};
typedef struct USBConfigurationDescriptor USBConfigurationDescriptor;
typedef USBConfigurationDescriptor *	USBConfigurationDescriptorPtr;

struct USBInterfaceDescriptor {
	UInt8 							length;
	UInt8 							descriptorType;
	UInt8 							interfaceNumber;
	UInt8 							alternateSetting;
	UInt8 							numEndpoints;
	UInt8 							interfaceClass;
	UInt8 							interfaceSubClass;
	UInt8 							interfaceProtocol;
	UInt8 							interfaceStrIndex;
};
typedef struct USBInterfaceDescriptor	USBInterfaceDescriptor;
typedef USBInterfaceDescriptor *		USBInterfaceDescriptorPtr;

struct USBEndPointDescriptor {
	UInt8 							length;
	UInt8 							descriptorType;
	UInt8 							endpointAddress;
	UInt8 							attributes;
	UInt16 							maxPacketSize;
	UInt8 							interval;
};
typedef struct USBEndPointDescriptor	USBEndPointDescriptor;
typedef USBEndPointDescriptor *			USBEndPointDescriptorPtr;

struct USBHIDDescriptor {
	UInt8 							descLen;
	UInt8 							descType;
	UInt16 							descVersNum;
	UInt8 							hidCountryCode;
	UInt8 							hidNumDescriptors;
	UInt8 							hidDescriptorType;
	UInt8 							hidDescriptorLengthLo;		/* can't make this a single 16bit value or the compiler will add a filler byte*/
	UInt8 							hidDescriptorLengthHi;
};
typedef struct USBHIDDescriptor			USBHIDDescriptor;
typedef USBHIDDescriptor *				USBHIDDescriptorPtr;

struct USBHIDReportDesc {
	UInt8 							hidDescriptorType;
	UInt8 							hidDescriptorLengthLo;		/* can't make this a single 16bit value or the compiler will add a filler byte*/
	UInt8 							hidDescriptorLengthHi;
};
typedef struct USBHIDReportDesc			USBHIDReportDesc;
typedef USBHIDReportDesc *				USBHIDReportDescPtr;

struct USBHubPortStatus {
	UInt16 							portFlags;					/* Port status flags */
	UInt16 							portChangeFlags;			/* Port changed flags */
};
typedef struct USBHubPortStatus			USBHubPortStatus;
typedef USBHubPortStatus *				USBHubPortStatusPtr;
/* ********* ProtoTypes *************** */
/* For dealing with endianisms */
EXTERN_API_C( UInt16 )
HostToUSBWord					(UInt16 				value);

EXTERN_API_C( UInt16 )
USBToHostWord					(UInt16 				value);

EXTERN_API_C( UInt32 )
HostToUSBLong					(UInt32 				value);

EXTERN_API_C( UInt32 )
USBToHostLong					(UInt32 				value);

/* Main prototypes */
/* Transfer commands */
EXTERN_API_C( OSStatus )
USBDeviceRequest				(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBOpenPipe						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBClosePipe					(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBBulkWrite					(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBBulkRead						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBIntRead						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBIsocRead						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBIsocWrite					(USBPB *				pb);

/* Pipe state control */
EXTERN_API_C( OSStatus )
USBClearPipeStallByReference	(USBPipeRef 			ref);

EXTERN_API_C( OSStatus )
USBAbortPipeByReference			(USBReference 			ref);

EXTERN_API_C( OSStatus )
USBResetPipeByReference			(USBReference 			ref);

EXTERN_API_C( OSStatus )
USBSetPipeIdleByReference		(USBPipeRef 			ref);

EXTERN_API_C( OSStatus )
USBSetPipeActiveByReference		(USBPipeRef 			ref);

EXTERN_API_C( OSStatus )
USBClosePipeByReference			(USBPipeRef 			ref);

EXTERN_API_C( OSStatus )
USBGetPipeStatusByReference		(USBReference 			ref,
								 USBPipeState *			state);


/* Configuration services */
EXTERN_API_C( OSStatus )
USBFindNextInterface			(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBOpenDevice					(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBNewInterfaceRef				(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBDisposeInterfaceRef			(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBConfigureInterface			(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBFindNextPipe					(USBPB *				pb);


/* Dealing with descriptors. */
/* Note most of this is temprorary */
EXTERN_API_C( OSStatus )
USBGetConfigurationDescriptor	(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBGetFullConfigurationDescriptor (USBPB *				pb);

EXTERN_API_C( OSStatus )
USBFindNextEndpointDescriptorImmediate (USBPB *			pb);

EXTERN_API_C( OSStatus )
USBFindNextInterfaceDescriptorImmediate (USBPB *		pb);

EXTERN_API_C( OSStatus )
USBFindNextAssociatedDescriptor	(USBPB *				pb);



/* Utility functions */
EXTERN_API_C( OSStatus )
USBResetDevice					(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBGetFrameNumberImmediate		(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBDelay						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBAllocMem						(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBDeallocMem					(USBPB *				pb);

/* Expert interface functions */
EXTERN_API_C( OSStatus )
USBExpertInstallInterfaceDriver	(USBDeviceRef 			ref,
								 USBDeviceDescriptorPtr  desc,
								 USBInterfaceDescriptorPtr  interfacePtr,
								 USBReference 			hubRef,
								 UInt32 				busPowerAvailable);

EXTERN_API_C( OSStatus )
USBExpertRemoveInterfaceDriver	(USBDeviceRef 			ref);

EXTERN_API_C( OSStatus )
USBExpertInstallDeviceDriver	(USBDeviceRef 			ref,
								 USBDeviceDescriptorPtr  desc,
								 USBReference 			hubRef,
								 UInt32 				port,
								 UInt32 				busPowerAvailable);

EXTERN_API_C( OSStatus )
USBExpertRemoveDeviceDriver		(USBDeviceRef 			ref);

EXTERN_API_C( OSStatus )
USBExpertStatus					(USBDeviceRef 			ref,
								 void *					pointer,
								 UInt32 				value);

EXTERN_API_C( OSStatus )
USBExpertFatalError				(USBDeviceRef 			ref,
								 OSStatus 				status,
								 void *					pointer,
								 UInt32 				value);

EXTERN_API_C( OSStatus )
USBExpertNotify					(void *					note);

EXTERN_API_C( OSStatus )
USBExpertSetDevicePowerStatus	(USBDeviceRef 			ref,
								 UInt32 				reserved1,
								 UInt32 				reserved2,
								 UInt32 				powerStatus,
								 UInt32 				busPowerAvailable,
								 UInt32 				busPowerNeeded);


enum {
	kUSBDevicePower_PowerOK		= 0,
	kUSBDevicePower_BusPowerInsufficient = 1,
	kUSBDevicePower_BusPowerNotAllFeatures = 2,
	kUSBDevicePower_SelfPowerInsufficient = 3,
	kUSBDevicePower_SelfPowerNotAllFeatures = 4,
	kUSBDevicePower_HubPortOk	= 5,
	kUSBDevicePower_HubPortOverCurrent = 6,
	kUSBDevicePower_BusPoweredHubOnLowPowerPort = 7,
	kUSBDevicePower_BusPoweredHubToBusPoweredHub = 8,
	kUSBDevicePower_Reserved3	= 9,
	kUSBDevicePower_Reserved4	= 10
};


/* For hubs only */
EXTERN_API_C( OSStatus )
USBHubAddDevice					(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBHubConfigurePipeZero			(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBHubSetAddress				(USBPB *				pb);

EXTERN_API_C( OSStatus )
USBHubDeviceRemoved				(USBPB *				pb);


EXTERN_API_C( UInt8 )
USBMakeBMRequestType			(UInt8 					direction,
								 UInt8 					reqtype,
								 UInt8 					recipient);

/* This not implemented until someone shows me a test case */
/*OSStatus USBControlRequest(USBPB *pb);*/

typedef UInt32 							USBLocationID;

enum {
	kUSBLocationNibbleFormat	= 0								/* Other values are reserved for future types (like when we have more than 16 ports per hub)*/
};



enum {
	kNoDeviceRef				= -1
};

/* Expert Notification Types*/
typedef UInt8 							USBNotificationType;
typedef UInt8 							USBDriverMessage;

enum {
	kNotifyAddDevice			= 0x00,
	kNotifyRemoveDevice			= 0x01,
	kNotifyAddInterface			= 0x02,
	kNotifyRemoveInterface		= 0x03,
	kNotifyGetDeviceDescriptor	= 0x04,
	kNotifyGetInterfaceDescriptor = 0x05,
	kNotifyGetNextDeviceByClass	= 0x06,
	kNotifyGetDriverConnectionID = 0x07,
	kNotifyInstallDeviceNotification = 0x08,
	kNotifyRemoveDeviceNotification = 0x09,
	kNotifyDeviceRefToBusRef	= 0x0A,
	kNotifyDriverNotify			= 0x0C,
	kNotifyParentNotify			= 0x0D,
	kNotifyAddDriverForReference = 0x0E,
	kNotifyAnyEvent				= 0xFF,
	kNotifyPowerState			= 0x17,
	kNotifyStatus				= 0x18,
	kNotifyFatalError			= 0x19
};

/*
   USB Manager wildcard constants for USBGetNextDeviceByClass
   and USBInstallDeviceNotification.
*/
typedef UInt16 							USBManagerWildcard;

enum {
	kUSBAnyClass				= 0xFFFF,
	kUSBAnySubClass				= 0xFFFF,
	kUSBAnyProtocol				= 0xFFFF,
	kUSBAnyVendor				= 0xFFFF,
	kUSBAnyProduct				= 0xFFFF
};




struct ExpertNotificationData {
	USBNotificationType 			notification;
	UInt8 							filler[1];					/* unused due to 2-byte 68k alignment*/
	USBDeviceRef *					deviceRef;
	UInt32 							busPowerAvailable;
	void *							data;
	UInt32 							info1;
	UInt32 							info2;
};
typedef struct ExpertNotificationData	ExpertNotificationData;
typedef ExpertNotificationData *		ExpertNotificationDataPtr;
/* Definition of function pointer passed in ExpertEntryProc*/
typedef CALLBACK_API_C( OSStatus , ExpertNotificationProcPtr )(ExpertNotificationDataPtr pNotificationData);
/* Definition of expert's callback installation function*/
typedef CALLBACK_API_C( OSStatus , ExpertEntryProcPtr )(ExpertNotificationProcPtr pExpertNotify);
/* Device Notification Callback Routine*/
typedef CALLBACK_API_C( void , USBDeviceNotificationCallbackProcPtr )(void *pb);
/* Device Notification Parameter Block*/

struct USBDeviceNotificationParameterBlock {
	UInt16 							pbLength;
	UInt16 							pbVersion;
	USBNotificationType 			usbDeviceNotification;
	UInt8 							reserved1[1];				/* needed because of 2-byte 68k alignment*/
	USBDeviceRef 					usbDeviceRef;
	UInt16 							usbClass;
	UInt16 							usbSubClass;
	UInt16 							usbProtocol;
	UInt16 							usbVendor;
	UInt16 							usbProduct;
	OSStatus 						result;
	UInt32 							token;
	USBDeviceNotificationCallbackProcPtr  callback;
	UInt32 							refcon;
};
typedef struct USBDeviceNotificationParameterBlock USBDeviceNotificationParameterBlock;
/* Forward declaration*/
typedef USBDeviceNotificationParameterBlock * USBDeviceNotificationParameterBlockPtr;
/* Definition of USBDriverNotificationCallback Routine*/
typedef CALLBACK_API_C( void , USBDriverNotificationCallbackPtr )(OSStatus status, UInt32 refcon);
/* Public Functions*/
EXTERN_API_C( OSStatus )
USBGetNextDeviceByClass			(USBDeviceRef *			deviceRef,
								 CFragConnectionID *	connID,
								 UInt16 				theClass,
								 UInt16 				theSubClass,
								 UInt16 				theProtocol);

EXTERN_API_C( OSStatus )
USBGetDeviceDescriptor			(USBDeviceRef *			deviceRef,
								 USBDeviceDescriptor *	deviceDescriptor,
								 UInt32 				size);

EXTERN_API_C( OSStatus )
USBGetInterfaceDescriptor		(USBInterfaceRef *		interfaceRef,
								 USBInterfaceDescriptor * interfaceDescriptor,
								 UInt32 				size);

EXTERN_API_C( OSStatus )
USBGetDriverConnectionID		(USBDeviceRef *			deviceRef,
								 CFragConnectionID *	connID);

EXTERN_API_C( void )
USBInstallDeviceNotification	(USBDeviceNotificationParameterBlock * pb);

EXTERN_API_C( OSStatus )
USBRemoveDeviceNotification		(UInt32 				token);

EXTERN_API_C( OSStatus )
USBDeviceRefToBusRef			(USBDeviceRef *			deviceRef,
								 USBBusRef *			busRef);

EXTERN_API_C( OSStatus )
USBDriverNotify					(USBReference 			reference,
								 USBDriverMessage 		mesg,
								 UInt32 				refcon,
								 USBDriverNotificationCallbackPtr  callback);

EXTERN_API_C( OSStatus )
USBExpertNotifyParent			(USBReference 			reference,
								 void *					pointer);

EXTERN_API_C( OSStatus )
USBAddDriverForReference		(USBReference 			reference);

typedef CALLBACK_API_C( void , HIDInterruptProcPtr )(UInt32 refcon, void *theData);
/* HID Install Interrupt prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDInstallInterruptProcPtr )(HIDInterruptProcPtr pInterruptProc, UInt32 refcon);
/* HID Poll Device prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDPollDeviceProcPtr )(void );
/* HID Control Device prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDControlDeviceProcPtr )(UInt32 theControlSelector, void *theControlData);
/* HID Get Device Info prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDGetDeviceInfoProcPtr )(UInt32 theInfoSelector, void *theInfo);
/* HID Enter Polled Mode prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDEnterPolledModeProcPtr )(void );
/* HID Exit Polled Mode prototype*/
typedef CALLBACK_API_C( OSStatus , USBHIDExitPolledModeProcPtr )(void );

struct USBHIDModuleDispatchTable {
	UInt32 							hidDispatchVersion;
	USBHIDInstallInterruptProcPtr 	pUSBHIDInstallInterrupt;
	USBHIDPollDeviceProcPtr 		pUSBHIDPollDevice;
	USBHIDControlDeviceProcPtr 		pUSBHIDControlDevice;
	USBHIDGetDeviceInfoProcPtr 		pUSBHIDGetDeviceInfo;
	USBHIDEnterPolledModeProcPtr 	pUSBHIDEnterPolledMode;
	USBHIDExitPolledModeProcPtr 	pUSBHIDExitPolledMode;
};
typedef struct USBHIDModuleDispatchTable USBHIDModuleDispatchTable;
typedef USBHIDModuleDispatchTable *		USBHIDModuleDispatchTablePtr;
/*	Prototypes Tue, Mar 17, 1998 4:54:30 PM	*/
EXTERN_API_C( OSStatus )
USBHIDInstallInterrupt			(HIDInterruptProcPtr 	HIDInterruptFunction,
								 UInt32 				refcon);

EXTERN_API_C( OSStatus )
USBHIDPollDevice				(void);

EXTERN_API_C( OSStatus )
USBHIDControlDevice				(UInt32 				theControlSelector,
								 void *					theControlData);

EXTERN_API_C( OSStatus )
USBHIDGetDeviceInfo				(UInt32 				theInfoSelector,
								 void *					theInfo);

EXTERN_API_C( OSStatus )
USBHIDEnterPolledMode			(void);

EXTERN_API_C( OSStatus )
USBHIDExitPolledMode			(void);

EXTERN_API_C( void )
HIDNotification					(UInt32 				devicetype,
								 UInt8 					NewHIDData[],
								 UInt8 					OldHIDData[]);


enum {
	kHIDRqGetReport				= 1,
	kHIDRqGetIdle				= 2,
	kHIDRqGetProtocol			= 3,
	kHIDRqSetReport				= 9,
	kHIDRqSetIdle				= 10,
	kHIDRqSetProtocol			= 11
};


enum {
	kHIDRtInputReport			= 1,
	kHIDRtOutputReport			= 2,
	kHIDRtFeatureReport			= 3
};


enum {
	kHIDBootProtocolValue		= 0,
	kHIDReportProtocolValue		= 1
};


enum {
	kHIDKeyboardInterfaceProtocol = 1,
	kHIDMouseInterfaceProtocol	= 2
};


enum {
	kHIDSetLEDStateByBits		= 1,
	kHIDSetLEDStateByBitMask	= 1,
	kHIDSetLEDStateByIDNumber	= 2,
	kHIDRemoveInterruptHandler	= 3,
	kHIDEnableDemoMode			= 4,
	kHIDDisableDemoMode			= 5
};


enum {
	kHIDGetLEDStateByBits		= 1,							/* not supported in 1.0 of keyboard module*/
	kHIDGetLEDStateByBitMask	= 1,							/* not supported in 1.0 of keyboard module*/
	kHIDGetLEDStateByIDNumber	= 2,
	kHIDGetDeviceCountryCode	= 3,							/* not supported in 1.0 HID modules*/
	kHIDGetDeviceUnitsPerInch	= 4,							/* only supported in mouse HID module*/
	kHIDGetInterruptHandler		= 5,
	kHIDGetCurrentKeys			= 6,							/* only supported in keyboard HID module*/
	kHIDGetInterruptRefcon		= 7,
	kHIDGetVendorID				= 8,
	kHIDGetProductID			= 9
};



enum {
	kNumLockLED					= 0,
	kCapsLockLED				= 1,
	kScrollLockLED				= 2,
	kComposeLED					= 3,
	kKanaLED					= 4
};


enum {
	kNumLockLEDMask				= 1 << kNumLockLED,
	kCapsLockLEDMask			= 1 << kCapsLockLED,
	kScrollLockLEDMask			= 1 << kScrollLockLED,
	kComposeLEDMask				= 1 << kComposeLED,
	kKanaLEDMask				= 1 << kKanaLED
};


enum {
	kUSBCapsLockKey				= 0x39,
	kUSBNumLockKey				= 0x53,
	kUSBScrollLockKey			= 0x47
};


struct USBMouseData {
	UInt16 							buttons;
	SInt16 							XDelta;
	SInt16 							YDelta;
};
typedef struct USBMouseData				USBMouseData;
typedef USBMouseData *					USBMouseDataPtr;

struct USBKeyboardData {
	UInt16 							keycount;
	UInt16 							usbkeycode[32];
};
typedef struct USBKeyboardData			USBKeyboardData;
typedef USBKeyboardData *				USBKeyboardDataPtr;

union USBHIDData {
	USBKeyboardData 				kbd;
	USBMouseData 					mouse;
};
typedef union USBHIDData				USBHIDData;
typedef USBHIDData *					USBHIDDataPtr;
EXTERN_API_C( void )
StartCompoundClassDriver		(USBDeviceRef 			device,
								 UInt16 				classID,
								 UInt16 				subClass);


enum {
	kUSBCompositeClass			= 0,
	kUSBAudioClass				= 1,
	kUSBCommClass				= 2,
	kUSBHIDClass				= 3,
	kUSBDisplayClass			= 4,
	kUSBPrintingClass			= 7,
	kUSBMassStorageClass		= 8,
	kUSBHubClass				= 9,
	kUSBDataClass				= 10,
	kUSBVendorSpecificClass		= 0xFF
};


enum {
	kUSBCompositeSubClass		= 0,
	kUSBHubSubClass				= 1
};


enum {
	kUSBHIDInterfaceClass		= 0x03
};


enum {
	kUSBNoInterfaceSubClass		= 0x00,
	kUSBBootInterfaceSubClass	= 0x01
};


enum {
	kUSBNoInterfaceProtocol		= 0x00,
	kUSBKeyboardInterfaceProtocol = 0x01,
	kUSBMouseInterfaceProtocol	= 0x02
};




enum {
	kServiceCategoryUSB			= FOUR_CHAR_CODE('usb ')		/* USB*/
};

/* SOMETHING NEEDS TO BE DONE WITH THIS - */

enum {
	kUSBTypeIsHub				= FOUR_CHAR_CODE('hubd'),		/* Hub*/
	kUSBTypeIsHID				= FOUR_CHAR_CODE('HIDd'),		/* Human Interface Device*/
	kUSBTypeIsDisplay			= FOUR_CHAR_CODE('disp'),		/* Display */
	kUSBTypeIsModem				= FOUR_CHAR_CODE('modm')		/* Modem*/
};


enum {
	kUSBDriverFileType			= FOUR_CHAR_CODE('ndrv'),
	kUSBDriverRsrcType			= FOUR_CHAR_CODE('usbd'),
	kUSBShimRsrcType			= FOUR_CHAR_CODE('usbs')
};


enum {
	kTheUSBDriverDescriptionSignature = FOUR_CHAR_CODE('usbd')
};


enum {
	kInitialUSBDriverDescriptor	= 0
};



typedef UInt32 							USBDriverDescVersion;
/*  Driver Loading Options*/
typedef UInt32 							USBDriverLoadingOptions;

enum {
	kUSBDoNotMatchGenericDevice	= 0x00000001,					/* Driver's VendorID must match Device's VendorID*/
	kUSBDoNotMatchInterface		= 0x00000002,					/* Do not load this driver as an interface driver.*/
	kUSBProtocolMustMatch		= 0x00000004,					/* Do not load this driver if protocol field doesn't match.*/
	kUSBInterfaceMatchOnly		= 0x00000008					/* Only load this driver as an interface driver.*/
};


enum {
	kClassDriverPluginVersion	= 0x00001100
};




struct USBDeviceInfo {
	UInt16 							usbVendorID;				/* USB Vendor ID*/
	UInt16 							usbProductID;				/* USB Product ID.*/
	UInt16 							usbDeviceReleaseNumber;		/* Release Number of Device*/
	UInt16 							usbDeviceProtocol;			/* Protocol Info.*/
};
typedef struct USBDeviceInfo			USBDeviceInfo;
typedef USBDeviceInfo *					USBDeviceInfoPtr;

struct USBInterfaceInfo {
	UInt8 							usbConfigValue;				/* Configuration Value*/
	UInt8 							usbInterfaceNum;			/* Interface Number*/
	UInt8 							usbInterfaceClass;			/* Interface Class*/
	UInt8 							usbInterfaceSubClass;		/* Interface SubClass*/
	UInt8 							usbInterfaceProtocol;		/* Interface Protocol*/
};
typedef struct USBInterfaceInfo			USBInterfaceInfo;
typedef USBInterfaceInfo *				USBInterfaceInfoPtr;

struct USBDriverType {
	Str31 							nameInfoStr;				/* Driver's name when loading into the Name Registry.*/
	UInt8 							usbDriverClass;				/* USB Class this driver belongs to.*/
	UInt8 							usbDriverSubClass;			/* Module type*/
	NumVersion 						usbDriverVersion;			/* Class driver version number.*/
};
typedef struct USBDriverType			USBDriverType;
typedef USBDriverType *					USBDriverTypePtr;

struct USBDriverDescription {
	OSType 							usbDriverDescSignature;		/* Signature field of this structure.*/
	USBDriverDescVersion 			usbDriverDescVersion;		/* Version of this data structure.*/
	USBDeviceInfo 					usbDeviceInfo;				/* Product & Vendor Info*/
	USBInterfaceInfo 				usbInterfaceInfo;			/* Interface info*/
	USBDriverType 					usbDriverType;				/* Driver Info.*/
	USBDriverLoadingOptions 		usbDriverLoadingOptions;	/* Options for class driver loading.*/
};
typedef struct USBDriverDescription		USBDriverDescription;
typedef USBDriverDescription *			USBDriverDescriptionPtr;
/*
   Dispatch Table
   Definition of class driver's HW Validation proc.
*/
typedef CALLBACK_API_C( OSStatus , USBDValidateHWProcPtr )(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);
/*
   Definition of class driver's device initialization proc.
   Called if the driver is being loaded for a device
*/
typedef CALLBACK_API_C( OSStatus , USBDInitializeDeviceProcPtr )(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
/* Definition of class driver's interface initialization proc.*/
typedef CALLBACK_API_C( OSStatus , USBDInitializeInterfaceProcPtr )(UInt32 interfaceNum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDevice, USBInterfaceRef interfaceRef);
/* Definition of class driver's finalization proc.*/
typedef CALLBACK_API_C( OSStatus , USBDFinalizeProcPtr )(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);

typedef UInt32 							USBDriverNotification;

enum {
	kNotifySystemSleepRequest	= 0x00000001,
	kNotifySystemSleepDemand	= 0x00000002,
	kNotifyHubEnumQuery			= 0x00000006,
	kNotifyChildMessage			= 0x00000007,
	kNotifyDriverBeingRemoved	= 0x0000000B
};

/*
   Definition of driver's notificatipn proc.      
   Added refcon for 1.1 version of dispatch table
*/
typedef CALLBACK_API_C( OSStatus , USBDDriverNotifyProcPtr )(USBDriverNotification notification, void *pointer, UInt32 refcon);

struct USBClassDriverPluginDispatchTable {
	UInt32 							pluginVersion;
	USBDValidateHWProcPtr 			validateHWProc;				/* Proc for driver to verify proper HW*/
	USBDInitializeDeviceProcPtr 	initializeDeviceProc;		/* Proc that initializes the class driver.*/
	USBDInitializeInterfaceProcPtr 	initializeInterfaceProc;	/* Proc that initializes a particular interface in the class driver.*/
	USBDFinalizeProcPtr 			finalizeProc;				/* Proc that finalizes the class driver.*/
	USBDDriverNotifyProcPtr 		notificationProc;			/* Proc to pass notifications to the driver.*/
};
typedef struct USBClassDriverPluginDispatchTable USBClassDriverPluginDispatchTable;
typedef USBClassDriverPluginDispatchTable * USBClassDriverPluginDispatchTablePtr;
/* Hub defines*/



enum {
	kUSBHubDescriptorType		= 0x29
};


enum {
																/* Hub features */
	kUSBHubLocalPowerChangeFeature = 0,
	kUSBHubOverCurrentChangeFeature = 1,						/* port features */
	kUSBHubPortConnectionFeature = 0,
	kUSBHubPortEnablenFeature	= 1,
	kUSBHubPortSuspecdFeature	= 2,
	kUSBHubPortOverCurrentFeature = 3,
	kUSBHubPortResetFeature		= 4,
	kUSBHubPortPowerFeature		= 8,
	kUSBHubPortLowSpeedFeature	= 9,
	kUSBHubPortConnectionChangeFeature = 16,
	kUSBHubPortEnableChangeFeature = 17,
	kUSBHubPortSuspendChangeFeature = 18,
	kUSBHubPortOverCurrentChangeFeature = 19,
	kUSBHubPortResetChangeFeature = 20
};



enum {
	kHubPortConnection			= 1,
	kHubPortEnabled				= 2,
	kHubPortSuspend				= 4,
	kHubPortOverCurrent			= 8,
	kHubPortBeingReset			= 16,
	kHubPortPower				= 0x0100,
	kHubPortSpeed				= 0x0200
};


enum {
	kHubLocalPowerStatus		= 1,
	kHubOverCurrentIndicator	= 2,
	kHubLocalPowerStatusChange	= 1,
	kHubOverCurrentIndicatorChange = 2
};


enum {
	off							= false,
	on							= true
};



struct hubDescriptor {
																/* See usbDoc pg 250?? */
	UInt8 							dummy;						/* to align charcteristics */

	UInt8 							length;
	UInt8 							hubType;
	UInt8 							numPorts;

	UInt16 							characteristics;
	UInt8 							powerOnToGood;				/* Port settling time, in 2ms */
	UInt8 							hubCurrent;

																/* These are received packed, will have to be unpacked */
	UInt8 							removablePortFlags[8];
	UInt8 							pwrCtlPortFlags[8];
};
typedef struct hubDescriptor			hubDescriptor;



#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __USB__ */
#define kprintf usb_donone
#define printf  usb_donone
void usb_donone(char *x,...);
