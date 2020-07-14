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
 	File:		PCI.h
 
 	Contains:	PCI Bus Interfaces.
 
 	Version:	PowerSurge 1.0.2
 
 	DRI:		Matthew Nelson
 
 	Copyright:	© 1993-1998 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				PCI.i
 					Revision:		22
 					Dated:			1/22/98
 					Last change by:	ngk
 					Last comment:	Change Types.i to MacTypes.i
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __PCI__
#define __PCI__

#ifndef __MACTYPES__
//#include <MacTypes.h>
#endif
#ifndef __NAMEREGISTRY__
//#include <NameRegistry.h>
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


		#if TARGET_CPU_68K && defined(IGNORE68KTRAPGLUE)
			#undef ONEWORDINLINE
			#undef TWOWORDINLINE
			#undef THREEWORDINLINE
			#undef FOURWORDINLINE
			#undef FIVEWORDINLINE
			#undef SIXWORDINLINE
			#undef SEVENWORDINLINE
			#undef EIGHTWORDINLINE
			#undef NINEWORDINLINE
			#undef TENWORDINLINE
			#undef ELEVENWORDINLINE
			#undef TWELVEWORDINLINE
			
			#define ONEWORDINLINE(w1)
			#define TWOWORDINLINE(w1,w2)
			#define THREEWORDINLINE(w1,w2,w3)
			#define FOURWORDINLINE(w1,w2,w3,w4)
			#define FIVEWORDINLINE(w1,w2,w3,w4,w5)
			#define SIXWORDINLINE(w1,w2,w3,w4,w5,w6)
			#define SEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7)
			#define EIGHTWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8)
			#define NINEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9)
			#define TENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10)
			#define ELEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11)
			#define TWELVEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12)
		#endif
	 
/* Types and structures for accessing the PCI Assigned-Address property.*/

#define kPCIAssignedAddressProperty "assigned-addresses"

enum {
	kPCIRelocatableSpace		= 0x80,
	kPCIPrefetchableSpace		= 0x40,
	kPCIAliasedSpace			= 0x20,
	kPCIAddressTypeCodeMask		= 0x03,
	kPCIConfigSpace				= 0,
	kPCIIOSpace					= 1,
	kPCI32BitMemorySpace		= 2,
	kPCI64BitMemorySpace		= 3
};

typedef UInt8 							PCIAddressSpaceFlags;

enum {
	kPCIDeviceNumberMask		= 0x1F,
	kPCIFunctionNumberMask		= 0x07
};

typedef UInt8 							PCIDeviceFunction;
typedef UInt8 							PCIBusNumber;
typedef UInt8 							PCIRegisterNumber;

struct PCIAssignedAddress {
	PCIAddressSpaceFlags 			addressSpaceFlags;
	PCIBusNumber 					busNumber;
	PCIDeviceFunction 				deviceFunctionNumber;
	PCIRegisterNumber 				registerNumber;
	UnsignedWide 					address;
	UnsignedWide 					size;
};
typedef struct PCIAssignedAddress		PCIAssignedAddress;
typedef PCIAssignedAddress *			PCIAssignedAddressPtr;
#define GetPCIIsRelocatable( AssignedAddressPtr )		((AssignedAddressPtr)->addressSpaceFlags & kPCIRelocatableSpace)
#define GetPCIIsPrefetchable( AssignedAddressPtr )		((AssignedAddressPtr)->addressSpaceFlags & kPCIPrefetchableSpace)
#define GetPCIIsAliased( AssignedAddressPtr )			((AssignedAddressPtr)->addressSpaceFlags & kPCIAliasedSpace)
#define GetPCIAddressSpaceType( AssignedAddressPtr )	((AssignedAddressPtr)->addressSpaceFlags & kPCIAddressTypeCodeMask)
#define GetPCIBusNumber( AssignedAddressPtr )			((AssignedAddressPtr)->busNumber)
#define GetPCIDeviceNumber( AssignedAddressPtr )		(((AssignedAddressPtr)->deviceFunctionNumber >> 3) & kPCIDeviceNumberMask)
#define GetPCIFunctionNumber( AssignedAddressPtr )		((AssignedAddressPtr)->deviceFunctionNumber & kPCIFunctionNumberMask)
#define GetPCIRegisterNumber( AssignedAddressPtr )		((AssignedAddressPtr)->registerNumber)

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 EndianSwap16Bit(__D0)
																							#endif
EXTERN_API( UInt16 )
EndianSwap16Bit					(UInt16 				data16)								ONEWORDINLINE(0xE158);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 EndianSwap32Bit(__D0)
																							#endif
EXTERN_API( UInt32 )
EndianSwap32Bit					(UInt32 				data32)								THREEWORDINLINE(0xE158, 0x4840, 0xE158);

EXTERN_API( OSErr )
ExpMgrConfigReadByte			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt8 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0620, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrConfigReadWord			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt16 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0621, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrConfigReadLong			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt32 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0622, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrConfigWriteByte			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt8 					value)								THREEWORDINLINE(0x303C, 0x0523, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrConfigWriteWord			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt16 				value)								THREEWORDINLINE(0x303C, 0x0524, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrConfigWriteLong			(RegEntryIDPtr 			node,
								 LogicalAddress 		configAddr,
								 UInt32 				value)								THREEWORDINLINE(0x303C, 0x0625, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOReadByte				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt8 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0626, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOReadWord				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt16 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0627, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOReadLong				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt32 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0628, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOWriteByte				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt8 					value)								THREEWORDINLINE(0x303C, 0x0529, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOWriteWord				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt16 				value)								THREEWORDINLINE(0x303C, 0x052A, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrIOWriteLong				(RegEntryIDPtr 			node,
								 LogicalAddress 		ioAddr,
								 UInt32 				value)								THREEWORDINLINE(0x303C, 0x062B, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrInterruptAcknowledgeReadByte (RegEntryIDPtr 		entry,
								 UInt8 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0411, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrInterruptAcknowledgeReadWord (RegEntryIDPtr 		entry,
								 UInt16 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0412, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrInterruptAcknowledgeReadLong (RegEntryIDPtr 		entry,
								 UInt32 *				valuePtr)							THREEWORDINLINE(0x303C, 0x0413, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrSpecialCycleWriteLong		(RegEntryIDPtr 			entry,
								 UInt32 				value)								THREEWORDINLINE(0x303C, 0x0419, 0xAAF3);

EXTERN_API( OSErr )
ExpMgrSpecialCycleBroadcastLong	(UInt32 				value)								THREEWORDINLINE(0x303C, 0x021A, 0xAAF3);


		#if TARGET_CPU_68K && defined(IGNORE68KTRAPGLUE)
			#if TARGET_OS_MAC && !TARGET_RT_MAC_CFM
				#undef ONEWORDINLINE
				#undef TWOWORDINLINE
				#undef THREEWORDINLINE
				#undef FOURWORDINLINE
				#undef FIVEWORDINLINE
				#undef SIXWORDINLINE
				#undef SEVENWORDINLINE
				#undef EIGHTWORDINLINE
				#undef NINEWORDINLINE
				#undef TENWORDINLINE
				#undef ELEVENWORDINLINE
				#undef TWELVEWORDINLINE
			
				#define ONEWORDINLINE(w1) = w1
				#define TWOWORDINLINE(w1,w2) = {w1,w2}
				#define THREEWORDINLINE(w1,w2,w3) = {w1,w2,w3}
				#define FOURWORDINLINE(w1,w2,w3,w4)  = {w1,w2,w3,w4}
				#define FIVEWORDINLINE(w1,w2,w3,w4,w5) = {w1,w2,w3,w4,w5}
				#define SIXWORDINLINE(w1,w2,w3,w4,w5,w6)	 = {w1,w2,w3,w4,w5,w6}
				#define SEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7) 	 = {w1,w2,w3,w4,w5,w6,w7}
				#define EIGHTWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8) 	 = {w1,w2,w3,w4,w5,w6,w7,w8}
				#define NINEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9}
				#define TENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10)  = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10}
				#define ELEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11}
				#define TWELVEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12}
			#endif
		#endif
	

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

#endif /* __PCI__ */

