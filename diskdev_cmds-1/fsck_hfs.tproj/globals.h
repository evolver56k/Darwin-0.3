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
//#define TARGET_OS_RHAPSODY 1
//#define ppc 1
//#define FORDISKFIRSTAID 1
//#define pascal
#ifndef __GNUC__
#define __GNUC__ 1
#endif

#if defined(powerc) || defined (__powerc)
	#pragma options align=mac68k
#endif

#include <Errors.h>

//#include "DFALowMem.h"

char *LMGetFCBSPtr( void );
char *LMGetFSMVars( void );
typedef unsigned long *QHdrPtr;
QHdrPtr LMGetVCBQHdr( void );
unsigned long GetDFALowMem( signed short whichLowMem );

//typedef unsigned short UniChar;
typedef unsigned short *UniCharArrayPtr;
typedef const unsigned short *ConstUniCharArrayPtr;

//UniChar *Get_gLowerCaseTable(void);

enum {
	kHasCustomIcon = 0x400
};

extern pascal unsigned long TickCount(void);

void UseDFALowMems(unsigned char useDFALowMems);

extern pascal char *NewPtrClear(long byteCount);
extern pascal char *NewPtr(long byteCount);

#define	Blk_Size 512

//#define LMSetFSFCBLen( value )	( (void)  SetDFALowMem( kLMFSFCBLen, (UInt32) value ) )

void SetDFALowMem(signed short whichLowMem, unsigned long value);

extern pascal void BlockMoveData(const void *srcPtr, void *destPtr, long byteCount);

//void LMSetFCBSPtr(void *value);

//void LMSetFSMVars(char *value);

//#define TempNewHandle NewHandle
extern pascal char **NewHandle(long byteCount);
extern pascal char **NewHandleClear(long byteCount);

#define DisposPtr(p) DisposePtr(p)
extern pascal void DisposePtr(char *p);

extern pascal signed short MemError( void );

extern pascal void DisposeHandle(char **h);

#define	kCalculatedExtentRefNum 0x0002

#define	kCalculatedCatalogRefNum 0x0060

extern pascal char **RecoverHandle(char *p);

struct QElem
{
	int fd;
};
typedef struct QElem QElem;
typedef QElem *QElemPtr;

typedef unsigned long TextEncoding;

struct WDCBRec
{
	int dummy;
};
typedef struct WDCBRec WDCBRec;


struct DeferredTask
{
	int dummy;
};
typedef struct DeferredTask DeferredTask;

struct ConversionContext
{
	int dummy;
};
typedef struct ConversionContext ConversionContext;

typedef WDCBRec *WDCBRecPtr;

typedef void *IOCompletionUPP;

struct CMovePBRec
{
	int dummy;
};
typedef struct CMovePBRec CMovePBRec;

extern pascal signed short FlushVol(const unsigned char *volName, short vRefNum);

enum {
	gbDefault					= 0,							/* default value - read if not found */
																/*	bits and masks */
	gbReadBit					= 0,							/* read block from disk (forced read) */
	gbReadMask					= 0x0001,
	gbExistBit					= 1,							/* get existing cache block */
	gbExistMask					= 0x0002,
	gbNoReadBit					= 2,							/* don't read block from disk if not found in cache */
	gbNoReadMask				= 0x0004,
	gbReleaseBit				= 3,							/* release block immediately after GetBlock */
	gbReleaseMask				= 0x0008
};

char **TempNewHandle (long logicalSize, signed short *resultCode);

extern void HLock(char **h);

enum {
	rbDefault					= 0,																/*	bits and masks */
	rbWriteBit					= 0,
	rbWriteMask					= 0x0001,
	rbTrashBit					= 1,
	rbTrashMask					= 0x0002,
	rbDirtyBit					= 2,
	rbDirtyMask					= 0x0004,
	rbFreeBit					= 3,
	rbFreeMask					= 0x000A
};

#define offsetof(structure,field) ((size_t)&((structure *) 0)->field)

//void *LMGetFCBSPtr();

enum {
	fcbWriteBit					= 0,
	fcbWriteMask				= 0x01,
	fcbResourceBit				= 1,
	fcbResourceMask				= 0x02,
	fcbWriteLockedBit			= 2,
	fcbWriteLockedMask			= 0x04,
	fcbSharedWriteBit			= 4,
	fcbSharedWriteMask			= 0x10,
	fcbFileLockedBit			= 5,
	fcbFileLockedMask			= 0x20,
	fcbOwnClumpBit				= 6,
	fcbOwnClumpMask				= 0x40,
	fcbModifiedBit				= 7,
	fcbModifiedMask				= 0x80
};

signed short PtrAndHand(const void *ptr1, char **hand2, long size);

enum
{
	kTextEncodingMacUkrainian = 0x98,
	kTextEncodingMacFarsi = 0x8C,
	kTextEncodingMacRoman = 0L
};

pascal void GetDateTime(unsigned long *secs);

pascal void SpinCursor(short increment);

pascal long GetHandleSize(char** h);

unsigned char LMGetHFSFlags( void );

char *NewPtrSysClear(long byteCount);

char *NewPtrSys(long byteCount);

enum {
	chNoBuf						= 1,							/* no free cache buffers (all in use) */
	chInUse						= 2,							/* requested block in use */
	chnotfound					= 3,							/* requested block not found */
	chNotInUse					= 4								/* block being released was not in use */
};

