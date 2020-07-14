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
/* -*- mode:C++; tab-width: 4 -*- */
// Brutally ripped from SCSI.i and some other stuff added...

#ifndef SECONDARYLOADER_H
#define SECONDARYLOADER_H	1

#include <Types.h>
#include <SimpleHFS.h>
#include <MacPartitions.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#ifndef __MWERKS__
#define __declspec(x)
#endif

enum {
	kBlockSizeLog2 = 9,
	kBlockSize = 1 << kBlockSizeLog2,
	
	kPageSizeLog2 = 12,
	kPageSize = 1 << kPageSizeLog2,
	
	kReadRawDisk = 0			// "partition" number for entire disk
};


typedef long CICell;

struct CIArgs {
	char *service;
	CICell nArgs;
	CICell nReturns;

	union {
		struct {			// nArgs=1, nReturns=1
			char *forth;
			CICell catchResult;
		} interpret_0_0;

		struct {			// nArgs=2, nReturns=1
			char *forth;
			CICell arg1;
			CICell catchResult;
		} interpret_1_0;

		struct {			// nArgs=2, nReturns=2
			char *forth;
			CICell arg1;
			CICell catchResult;
			CICell return1;
		} interpret_1_1;

		struct {			// nArgs=3, nReturns=2
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell catchResult;
			CICell return1;
		} interpret_2_1;

		struct {			// nArgs=4, nReturns=2
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell arg3;
			CICell catchResult;
			CICell return1;
		} interpret_3_1;

	  struct {            // nArgs=5, nReturns=1
            char *forth;
            CICell arg1;
            CICell arg2;
            CICell arg3;
            CICell arg4;
            CICell catchResult;
	  } interpret_4_0;

		struct {			// nArgs=1, nReturns=2
			char *forth;
			CICell catchResult;
			CICell return1;
		} interpret_0_1;

		struct {			// nArgs=1, nReturns=3
			char *forth;
			CICell catchResult;
			CICell return1;
			CICell return2;
		} interpret_0_2;

		struct {			// nArgs=1, nReturns=4
			char *forth;
			CICell catchResult;
			CICell return1;
			CICell return2;
			CICell return3;
		} interpret_0_3;

		struct {			// nArgs=2, nReturns=4
			char *forth;
			CICell arg1;
			CICell catchResult;
			CICell return1;
			CICell return2;
			CICell return3;
		} interpret_1_3;

		struct {			// nArgs=3, nReturns=4
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell catchResult;
			CICell return1;
			CICell return2;
			CICell return3;
		} interpret_2_3;

		struct {			// nArgs=3, nReturns=5
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell catchResult;
			CICell return1;
			CICell return2;
			CICell return3;
			CICell return4;
		} interpret_2_4;

		struct {			// nArgs=3, nReturns=1
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell catchResult;
		} interpret_2_0;

		struct {			// nArgs=3, nReturns=3
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell catchResult;
			CICell return1;
			CICell return2;
		} interpret_2_2;

		struct {			// nArgs=4, nReturns=1
			char *forth;
			CICell arg1;
			CICell arg2;
			CICell arg3;
			CICell catchResult;
		} interpret_3_0;

		struct {			// nArgs=3, nReturns=1
			CICell align;
			CICell size;
			CICell virt;
			CICell baseaddr;
		} claim;

		struct {			// nArgs=2, nReturns=0
			CICell size;
			CICell virt;
		} release;

		struct {			// nArgs=3, nReturns=1	( phandle buf buflen -- length )
			CICell phandle;			// IN parameter
			char *buf;				// IN parameter
			CICell buflen;			// IN parameter
			CICell length;			// RETURN value
		} packageToPath;

		struct {			// nArgs=3, nReturns=1	( phandle previous buf -- flag )
			CICell phandle;			// IN parameter
			char *previous;			// IN parameter
			char *buf;				// IN parameter
			CICell flag;			// RETURN value
		} nextprop;

		struct {			// nArgs=4, nReturns=1	( phandle name buf buflen -- size )
			CICell phandle;			// IN parameter
			char *name;				// IN parameter
			char *buf;				// IN parameter
			CICell buflen;			// IN parameter
			CICell size;			// RETURN value
		} getprop;

		struct {			// nArgs=4, nReturns=1	( phandle name buf buflen -- size )
			CICell phandle;			// IN parameter
			char *name;				// IN parameter
			char *buf;				// IN parameter
			CICell buflen;			// IN parameter
			CICell size;			// RETURN value
		} setprop;

		struct {			// nArgs=1, nReturns=1	( device-specifier -- ihandle )
			char *deviceSpecifier;	// IN parameter
			CICell ihandle;			// RETURN value
		} open;

		struct {			// nArgs=1, nReturns=1	( device-specifier -- ihandle )
			char *deviceSpecifier;	// IN parameter
			CICell phandle;			// RETURN value
		} finddevice;

		struct {			// nArgs=1, nReturns=1	( phandle -- peer-phandle )
			CICell phandle;			// IN parameter
			CICell peerPhandle;		// RETURN value
		} peer;

		struct {			// nArgs=1, nReturns=1	( phandle -- child-phandle )
			CICell phandle;			// IN parameter
			CICell childPhandle;	// RETURN value
		} child;

		struct {			// nArgs=1, nReturns=1	( phandle -- parent-phandle )
			CICell childPhandle;	// IN parameter
			CICell parentPhandle;	// RETURN value
		} parent;

		struct {			// nArgs=1, nReturns=0
			char *bootspec;
		} boot;
		
		struct {			// nArgs=7, nReturns=1
			char *method;
			CICell iHandle;
			CICell arg1;
			CICell arg2;
			CICell arg3;
			CICell arg4;
			CICell arg5;
			CICell catchResult;
		} callMethod_5_0;

	} args;

};
typedef struct CIArgs CIArgs;

typedef int (*ClientInterfacePtr) (CIArgs *args);
typedef int (*IntFuncPtr) ();
typedef jmp_buf LoaderJumpBuffer;

enum PixelColorValues {			// Pixel values for our display's CLUT
	kWhitePixel = 0x00,				// 100% white
	kLtGrayPixel = 0xF7,			//  75% white
	kGrayPixel = 0xF9,				//  50% white
	kDkGrayPixel = 0xFC,			//  25% white
	kBlackPixel = 0xFF,				//   0% white
	kBackgroundPixel = 0xF9			// Gray area fill value for screen background
};

enum {
	kIconHeight = 32,
	kIconWidth = 32
};


enum {kSecondaryLoaderVectorVersion = 1};

// VERY IMPORTANT: Only add things to the END of this list.  Update
// the version constant for major changes.  Each entry is a macro invocation
// of DO_VECTOR(F,R,P) where F is the function name, R is its return type and
// P is the parameter list for the function.
#define SECONDARY_LOADER_VECTORS	\
	DO_VECTOR (ReadPartitionBlocks,		void, (UInt32 partitionNumber, \
											   void *buffer, \
											   UInt32 blockNumber, \
											   UInt32 nBlocks)) \
	DO_VECTOR (TryPartition,			void, (int partitionNumber)) \
	DO_VECTOR (TryAllPartitionsOnDisk,	void, (int nPartitions, \
											   int partitionNumberToSkip)) \
	DO_VECTOR (AccessDevice,			int, (CICell deviceIHandle, \
											  struct SecondaryLoaderVector *vectorsP)) \
	DO_VECTOR (DeaccessDevice,			void, (CICell deviceIHandle, \
											   struct SecondaryLoaderVector *vectorsP)) \
	DO_VECTOR (TryThisDevice,			void, (CICell phandle)) \
	DO_VECTOR (TryThisUnit,				void, (CICell phandle, \
											   UInt32 unit)) \
	DO_VECTOR (SearchDeviceTree,		void, (CICell root)) \
	DO_VECTOR (ClaimMemory,				UInt32, (CICell virtual, \
												 CICell size, \
												 CICell alignment)) \
	DO_VECTOR (ReleaseMemory,			void, (CICell virtual, \
											   CICell size)) \
	DO_VECTOR (GetPackagePropertyString,char *, (CICell phandle, \
												 char *propertyName)) \
	DO_VECTOR (GetParentPHandle,		CICell, (CICell phandle)) \
	DO_VECTOR (GetPeerPHandle,			CICell, (CICell phandle)) \
	DO_VECTOR (GetChildPHandle,			CICell, (CICell phandle)) \
	DO_VECTOR (GetNextProperty,			CICell, (CICell phandle, \
												 char *previousPropertyNameP, \
												 char *thisPropertyNameP)) \
	DO_VECTOR (GetProperty,				CICell, (CICell phandle, \
												 char *name, \
												 void *bufP, \
												 CICell buflen)) \
	DO_VECTOR (GetFileExtents,			void, (ExtentsArray *extentsTreeExtentsP, \
											   UInt32 fileID, \
											   ExtentsArray *extentsBufferP)) \
	DO_VECTOR (ReadHFSNode,				int, (UInt32 nodeNumber, \
											  ExtentsArray *extentsBufferP, \
											  BTreeNode *nodeBufferP)) \
	DO_VECTOR (StringsAreEqualCaseInsensitive, int, (char *s1, \
													 char *s2)) \
	DO_VECTOR (PStringsArePreciselyEqual, int, (StringPtr string1, \
												StringPtr string2)) \
	DO_VECTOR (FindLeafNode,			UInt32, (ExtentsArray *extentsBufferP, \
												 CatalogKey *keyToFindP, \
												 int (*compareFunc) (CatalogKey *key1, \
																	 CatalogKey *key2), \
												 BTreeNode *nodeBufferP, \
												 int *nRecordsP)) \
	DO_VECTOR (FileIDCompare,			int, (CatalogKey *key1, \
											  CatalogKey *key2)) \
	DO_VECTOR (ExtentsIDCompare,		int, (CatalogKey *key1, \
											  CatalogKey *key2)) \
	DO_VECTOR (FileNameCompare,			int, (CatalogKey *key1, \
											  CatalogKey *key2)) \
	DO_VECTOR (FindBTreeRecord,			void *, (BTreeNode *nodeBufferP, \
												 int recordNumber)) \
	DO_VECTOR (FindFileRecord,			FileRecord *, (UInt32 node, \
													   BTreeNode *nodeBufferP, \
													   ExtentsArray *catalogExtentsP, \
													   CatalogKey *leafKeyP)) \
	DO_VECTOR (DataToCode,				void, (void *startAddress, \
											   UInt32 length)) \
	DO_VECTOR (BailToOpenFirmware,		void, (void)) \
	DO_VECTOR (StartSystem7,			void, (void)) \
	DO_VECTOR (ExpandTo8WithMask,		void, (UInt8 *expandedP, \
											   int expandedRowBytes, \
											   const UInt32 *sourceP, \
											   const UInt32 *maskP, \
											   const int srcDepth, \
											   const int width, \
											   const int height, \
											   const UInt8 *pixelValuesP, \
											   const UInt8 backgroundPixel)) \
	DO_VECTOR (FindAndOpenDisplay,		CICell, (int *widthP, \
												 int *heightP)) \
	DO_VECTOR (ShowWelcomeIcon,			void, ()) \
	DO_VECTOR (FillRectangle,			void, (UInt8 pixel, \
											   int x, \
											   int y, \
											   int width, \
											   int height)) \
	DO_VECTOR (DrawRectangle,			void, (UInt8 *pixelsP, \
											   int x, \
											   int y, \
											   int width, \
											   int height)) \
	DO_VECTOR (ReadRectangle,			void, (UInt8 *pixelsP, \
											   int x, \
											   int y, \
											   int width, \
											   int height)) \
	DO_VECTOR (SpinActivity,			void, ()) \
	DO_VECTOR (LoaderSetjmp,			int, (LoaderJumpBuffer buffer)) \
	DO_VECTOR (LoaderLongjmp,			void, (LoaderJumpBuffer buffer, \
											   int setjmpReturnValue)) \
	DO_VECTOR (ShowMessage,				void, (char *message)) \
	DO_VECTOR (FatalError,				void, (char *message)) \
	DO_VECTOR (DeathScreen,				void, (char *message)) \
	DO_VECTOR (MoveBytes,				void, (void *destinationP, \
											   void *sourceP, \
											   UInt32 length)) \
	DO_VECTOR (AppendBSSData,			void *, (void *dataP, \
												 UInt32 size))

struct SecondaryLoaderVector {
	UInt32 version;
	UInt32 nVectors;
#define DO_VECTOR(F,R,P)	R (*F) P;
	SECONDARY_LOADER_VECTORS
	int terminator;
#undef DO_VECTOR
};
// VERY IMPORTANT: Only add things to the END of this struct.  Update
// the version constant for major changes.
typedef struct SecondaryLoaderVector SecondaryLoaderVector;


#define VCALL(F)	(*gSecondaryLoaderVectors->F)

extern SecondaryLoaderVector *gSecondaryLoaderVectors;

typedef int (*ExtensionEntryPointer) (CICell diskIHandle,
									  SecondaryLoaderVector *secondaryLoaderVectorsP,
									  ClientInterfacePtr clientInterfacePtr);

typedef void (*TertiaryEntryPointer) (CICell diskIHandle,
									  int partitionNumber,
									  ClientInterfacePtr clientInterfacePtr,
									  CICell loadBase,
									  CICell loadSize);


__declspec(internal) int CallCI (CIArgs *ciArgsP);
__declspec(internal) void MoveBytes (void *destinationP, void *sourceP, UInt32 length);
__declspec(internal) void DataToCode (void *startAddress, UInt32 length);
__declspec(internal) void CIbreakpoint (void);


#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif

#endif
