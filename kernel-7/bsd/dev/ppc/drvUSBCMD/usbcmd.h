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

//#import <mach/std_types.h>
#import <kernserv/ns_timer.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/ppc/IOPCIDevice.h>
#import <driverkit/IOPower.h>
#import <machdep/ppc/proc_reg.h>
#define kSAFE_USB_KMEMSIZE	2 * 1024	//"safe" amount to kalloc() for USB
#define bit0			0x00000001
#define bit1			0x00000002
#define bit2			0x00000004
#define bit3			0x00000008
#define bit4			0x00000010
#define bit5			0x00000020
#define bit6			0x00000040
#define bit7			0x00000080
#define bit8			0x00000100
#define bit9			0x00000200
#define bit10			0x00000400
#define bit11			0x00000800
#define bit12			0x00001000
#define bit13			0x00002000
#define bit14			0x00004000
#define bit15			0x00008000
#define bit16			0x00010000
#define bit17			0x00020000
#define bit18			0x00040000
#define bit19			0x00080000
#define bit20			0x00100000
#define bit21			0x00200000
#define bit22			0x00400000
#define bit23			0x00800000
#define bit24			0x01000000
#define bit25			0x02000000
#define bit26			0x04000000
#define bit27			0x08000000
#define bit28			0x10000000
#define bit29			0x20000000
#define bit30			0x40000000
#define bit31			0x80000000

////////////////////////////////////////////////////////////////////////////////
//
// OHCI register file.
//

enum
{
	kOHCIBit0					= (1 << 0),
	kOHCIBit1					= (1 << 1),
	kOHCIBit2					= (1 << 2),
	kOHCIBit3					= (1 << 3),
	kOHCIBit4					= (1 << 4),
	kOHCIBit5					= (1 << 5),
	kOHCIBit6					= (1 << 6),
	kOHCIBit7					= (1 << 7),
	kOHCIBit8					= (1 << 8),
	kOHCIBit9					= (1 << 9),
	kOHCIBit10					= (1 << 10),
	kOHCIBit11					= (1 << 11),
	kOHCIBit12					= (1 << 12),
	kOHCIBit13					= (1 << 13),
	kOHCIBit14					= (1 << 14),
	kOHCIBit15					= (1 << 15),
	kOHCIBit16					= (1 << 16),
	kOHCIBit17					= (1 << 17),
	kOHCIBit18					= (1 << 18),
	kOHCIBit19					= (1 << 19),
	kOHCIBit20					= (1 << 20),
	kOHCIBit21					= (1 << 21),
	kOHCIBit22					= (1 << 22),
	kOHCIBit23					= (1 << 23),
	kOHCIBit24					= (1 << 24),
	kOHCIBit25					= (1 << 25),
	kOHCIBit26					= (1 << 26),
	kOHCIBit27					= (1 << 27),
	kOHCIBit28					= (1 << 28),
	kOHCIBit29					= (1 << 29),
	kOHCIBit30					= (1 << 30),
	kOHCIBit31					= (1 << 31)
};


#ifdef OMIT
#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1

    typedef unsigned int    UInt32;         // A 32-bit unsigned integer
    typedef unsigned char   UInt8;          // A "byte-sized" integer
    typedef signed char SInt8;
    typedef signed int      SInt32;         // A 32-bit signed integer
    typedef boolean_t       Boolean;        // TRUE/FALSE value (YES/NO in NeXT)
    typedef signed int  OSErr;
    typedef signed int  OSStatus;

#endif /* __APPLE_TYPES_DEFINED__ */
#endif


typedef long InterruptMemberNumber;
typedef Boolean InterruptSourceState;

//typedef InterruptSourceState (*InterruptDisabler)(InterruptSetMember ISTmember, void *refCon);
typedef InterruptSourceState (*InterruptDisabler)(void  *ISTmember, void *refCon);

//typedef InterruptMemberNumber (*InterruptHandler)(InterruptSetMember ISTmember, void *refCon, UInt32 theIntCount);
typedef InterruptMemberNumber (*InterruptHandler)(void *ISTmember, void *refCon, UInt32 theIntCount);

//typedef void (*InterruptEnabler)(InterruptSetMember ISTmember, void *refCon);
typedef void (*InterruptEnabler)(void *ISTmember, void *refCon); //fake it since I can't find InterruptSetMember

/** Can't find OpaqueRef anywhere
typedef struct OpaqueRef *KernelID;
typedef KernelID InterruptSetID;  //A.W. doesn't seem that useful in C code

From Bill's DriverServices.h
struct InterruptSetMember {
	InterruptSetID 					setID;
	InterruptMemberNumber 			member;
};

typedef struct InterruptSetMember		InterruptSetMember;

enum {
	kISTChipInterruptSource		= 0,
	kISTOutputDMAInterruptSource = 1,
	kISTInputDMAInterruptSource	= 2,
	kISTPropertyMemberCount		= 3
};


typedef InterruptSetMember 				ISTProperty[3];
****/



#define OHCIBitRange(start, end)					\
(													\
	((((UInt32) 0xFFFFFFFF) << (31 - (end))) >>		\
	 ((31 - (end)) + (start))) <<					\
	(start)											\
)

#define OHCIBitRangePhase(start, end)				\
	(start)



#define EndianSwapImm32Bit(data32)					\
(													\
	(((UInt32) data32) >> 24)				|		\
	((((UInt32) data32) >> 8) & 0xFF00)		|		\
	((((UInt32) data32) << 8) & 0xFF0000)	|		\
	(((UInt32) data32) << 24)						\
)

typedef struct OHCIRegistersStruct
								OHCIRegisters,
								*OHCIRegistersPtr;

typedef struct OHCIEndpointDescriptorStruct
								OHCIEndpointDescriptor,
								*OHCIEndpointDescriptorPtr;

typedef struct OHCIGeneralTransferDescriptorStruct
								OHCIGeneralTransferDescriptor,
								*OHCIGeneralTransferDescriptorPtr;


struct OHCIPhysicalLogicalStruct
{
	UInt32						LogicalStart;
	UInt32						LogicalEnd;
	UInt32						PhysicalStart;
	UInt32						PhysicalEnd;
	UInt32						type;
	UInt32						pNext;
	
};



typedef struct OHCIPhysicalLogicalStruct
								OHCIPhysicalLogical,
								*OHCIPhysicalLogicalPtr;




// OHCI register file.
// The following fields exactly match the OHCI hardware registers
//	 (page 5-1 of CMD specs)
struct OHCIRegistersStruct
{
	// Control and status group.
	volatile UInt32				hcRevision;
	volatile UInt32				hcControl;
	volatile UInt32				hcCommandStatus;
	volatile UInt32				hcInterruptStatus;
	volatile UInt32				hcInterruptEnable;
	volatile UInt32				hcInterruptDisable;

	// Memory pointer group.
	volatile UInt32				hcHCCA;
	volatile UInt32				hcPeriodCurrentED;
	volatile UInt32				hcControlHeadED;
	volatile UInt32				hcControlCurrentED;
	volatile UInt32				hcBulkHeadED;
	volatile UInt32				hcBulkCurrentED;
	volatile UInt32				hcDoneHead;

	// Frame counter group.
	volatile UInt32				hcFmInterval;
	volatile UInt32				hcFmRemaining;
	volatile UInt32				hcFmNumber;
	volatile UInt32				hcPeriodicStart;
	volatile UInt32				hcLSThreshold;

	// Root hub group.
	volatile UInt32				hcRhDescriptorA;
	volatile UInt32				hcRhDescriptorB;
	volatile UInt32				hcRhStatus;
	volatile UInt32				hcRhPortStatus[2];
};

// hcControl register defs.
enum
{
	kOHCIHcControl_CBSR			= OHCIBitRange (0, 1),
	kOHCIHcControl_CBSRPhase	= OHCIBitRangePhase (0, 1),
	kOHCIHcControl_PLE			= kOHCIBit2,
	kOHCIHcControl_IE			= kOHCIBit3,
	kOHCIHcControl_CLE			= kOHCIBit4,
	kOHCIHcControl_BLE			= kOHCIBit5,
	kOHCIHcControl_HCFS			= OHCIBitRange (6, 7),
	kOHCIHcControl_HCFSPhase	= OHCIBitRangePhase (6, 7),
	kOHCIHcControl_IR			= kOHCIBit8,
	kOHCIHcControl_RWC			= kOHCIBit9,
	kOHCIHcControl_RWE			= kOHCIBit10,

	kOHCIHcControl_Reserved		= OHCIBitRange (11, 31),

	kOHCIFunctionalState_Reset			= 0,
	kOHCIFunctionalState_Resume			= 1,
	kOHCIFunctionalState_Operational	= 2,
	kOHCIFunctionalState_Suspend		= 3
};

// hcCommandStatus register defs.
enum
{
	kOHCIHcCommandStatus_HCR		= kOHCIBit0,
	kOHCIHcCommandStatus_CLF		= kOHCIBit1,
	kOHCIHcCommandStatus_BLF		= kOHCIBit2,
	kOHCIHcCommandStatus_OCR		= kOHCIBit3,
	kOHCIHcCommandStatus_SOC		= OHCIBitRange (16, 17),
	kOHCIHcCommandStatus_SOCPhase	= OHCIBitRangePhase (16, 17),

	kOHCIHcCommandStatus_Reserved	= OHCIBitRange (4, 15) | OHCIBitRange (18, 31)
};

// hcInterrupt register defs.
enum
{
	kOHCIHcInterrupt_SO				= kOHCIBit0,
	kOHCIHcInterrupt_WDH			= kOHCIBit1,
	kOHCIHcInterrupt_SF				= kOHCIBit2,
	kOHCIHcInterrupt_RD				= kOHCIBit3,
	kOHCIHcInterrupt_UE				= kOHCIBit4,
	kOHCIHcInterrupt_FNO			= kOHCIBit5,
	kOHCIHcInterrupt_RHSC			= kOHCIBit6,
	kOHCIHcInterrupt_OC				= kOHCIBit30,
	kOHCIHcInterrupt_MIE			= kOHCIBit31
};

/* ******************************************************************************
** 
** RegEntryID	:	The Global x-Namespace Entry Identifier
//
*/
/* These are borrowed from nameregistry.h */
/* Naga Longterm: Better merge this with nameregistry.h */
struct RegEntryID {
	UInt8							opaque[16];
};
typedef struct RegEntryID RegEntryID, *RegEntryIDPtr;
typedef UInt32 RegPropertyValueSize;
typedef char RegPropertyName;

////////////////////////////////////////////////////////////////////////////////
//
// OHCI UIM data records.
//

typedef short RootHubID;

// Interrupt head struct
struct OHCIIntHeadStruct
{
	OHCIEndpointDescriptorPtr				pHead;
	OHCIEndpointDescriptorPtr				pTail;
	UInt32									pHeadPhysical;
	int										nodeBandwidth;
};

typedef struct OHCIIntHeadStruct
								OHCIIntHead,
								*OHCIIntHeadPtr;


struct OHCIUIMDataStruct
{
	RegEntryID							ohciRegEntryID;			// Name Registry entry of OHCI.
//	UIMID								uimID;					// ID for OHCI UIM.
	RootHubID							rootHubID;				// Status of root hub, if 0, not enitialized otherwise has virtual ID number

	OHCIRegistersPtr					pOHCIRegisters;			// Pointer to base address of OHCI registers.
	Ptr									pHCCA,					// Pointer to HCCA.
										pHCCAAllocation;		// Pointer to memory allocated for HCCA.
	OHCIIntHead							pInterruptHead[64];		// ptr to private list of all interrupts heads 			
	UInt32								pIsochHead;				// ptr to Isochtonous list
	UInt32								pBulkHead;				// ptr to Bulk list
	UInt32								pControlHead;			// ptr to Control list
	OHCIPhysicalLogicalPtr				pPhysicalLogical;		// ptr to list of memory maps
	OHCIGeneralTransferDescriptorPtr	pFreeTD;				// list of Availabble Trasfer Descriptors
	OHCIEndpointDescriptorPtr			pFreeED;				// List of available Endpoint Descriptors
	OHCIGeneralTransferDescriptorPtr	pPendingTD;				// list of non processed Trasfer Descriptors

	UInt32								pageSize;
	
	// can't find includes for opaque InterruptSetMember					interruptSetMember;
	void								*oldInterruptRefCon;
	InterruptHandler					oldInterruptHandler;
	InterruptEnabler					interruptEnabler;
	InterruptDisabler					interruptDisabler;
	struct  {
		UInt32							scheduleOverrun;
		UInt32							unrecoverableError;
		UInt32							frameNumberOverflow;
		UInt32							ownershipChange;
	} errors;

};


typedef struct OHCIUIMDataStruct
								OHCIUIMData,
								*OHCIUIMDataPtr;
							

struct OHCIEndpointDescriptorStruct
{
	UInt32						dWord0;				// control
	UInt32						dWord1;				// pointer to last TD
	UInt32						dWord2;				// pointer to first TD
	UInt32						dWord3;				// Pointer to next ED
	UInt32						pVirtualNext;
	UInt32						pPhysical;
	UInt32						pVirtualTailP;
	UInt32						pVirtualHeadP;
};

struct OHCIGeneralTransferDescriptorStruct
{
	volatile UInt32				dWord0;				// Data controlling transfer.
	volatile UInt32				dWord1;				// Current buffer pointer.
	volatile UInt32				dWord2;				// Pointer to next transfer descriptor.
	UInt32						dWord3;				// Pointer to end of buffer.
	//WARNING... remove for now, can't find #include anywhere CallBackFuncPtr 			CallBack;			// only used if last TD, other wise its  nil
	long						refcon;				// 
	UInt32						pPhysical;
	UInt32						pVirtualNext;
	UInt32						pIntBack;
	UInt32						pEndpoint;			// pointer to TD's Endpoint
	UInt32						bufferSize;			// used only by control transfers to keep track of data buffers size leftover
	UInt32						reserved2;			// 
};

@interface AppleUSBCMD:IODirectDevice 
{

	unsigned long	ioBase;	// USB Host Controller registers base
	port_t		port;			// our interrupt port
	id			theirId;		// ADB client's id for input callback
	unsigned int	reg_HcFmInterval, reg_HcCommandStatus, reg_HcControl;
	//Globals -yuck from MacOS OHCIUIM.c


}


+ (Boolean)probe : (IOPCIDevice *) devDesc;	// initialize the driver

- initFromDeviceDescription : (IODeviceDescription *)deviceDescription;

//- (Boolean)enableCMDChip; 
//- (Boolean) SetupHCDLists;
- free; 

- (unsigned int) getADBKeyboardID;

- (void)interruptOccurred;

@end










