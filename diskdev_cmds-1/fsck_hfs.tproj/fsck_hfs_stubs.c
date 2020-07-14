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
        File:           fsck_hfs_stubs.c

        Contains:       stubs for the MacOS X Server fsck command line

        Version:        HFS Plus 1.0

        Copyright:      \xa9 1998-1999 by Apple Computer, Inc., all rights reserved.

        File Ownership:

                DRI:                    Cliff Ritchie

                Other Contact:          Clark Warner

                Technology:                     xxx put technology here xxx

        Writers:

                (chw)   Clark Warner

        Change History (most recent first):

        2/9/99  Chw     Added file header and fixed 64 bit math for seek in ReadWriteBlock.
                        This fix allows Disk First Aid for MacOS X server to work on > 4GB volumes

*/

#include <unistd.h>
#ifndef __MWERKS__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <dev/disk.h>
#else
typedef	unsigned short	mode_t;
#define _STAT
#include <fcntl.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "MacOSTypes.h"
#include "ScavDefs.h"
#include "Prototypes.h"
#include "FileMgrInternal.h"

pascal void DebugStr(ConstStr255Param debuggerMsg)
{
	printf("DebugStr: %s\n",debuggerMsg);
	exit(1);
}

unsigned long gApplScratch[32] = {32*0};

void SetApplScratchValue( short whichLong, UInt32 value )
{
	*(gApplScratch + whichLong) = value;
}

UInt32 GetApplScratchValue( short whichLong )
{
	return *(gApplScratch + whichLong);
}

DrvQEl gDrvQEl;

OSErr FindDrive (short *driverRefNum, DrvQEl **dqe, short driveNumber)
{
	*driverRefNum = 666;
	*dqe = &gDrvQEl;
	return 0;
}

ExtendedVCB gExtendedVCB;

OSErr GetVCBDriveNum(ExtendedVCB **vcb, short driveNumber)
{
	*vcb = &gExtendedVCB;
	return 0;
}

pascal void BlockMoveData(const void *srcPtr, void *destPtr, long byteCount)
{
	memcpy(destPtr, srcPtr, byteCount);
}

short CheckPause( short MsgID, short *UserCmd )
{
}

pascal unsigned long TickCount(void)
{
	return 0;
}

pascal signed short MemError( void )
{
	return 0;
}

pascal signed short FlushVol(const unsigned char *volName, short vRefNum)
{
	return 0;
}

void HLock(char **h)
{
}

char **TempNewHandle (long logicalSize, signed short *resultCode)
{
	return NewHandle(logicalSize);
}

UInt32 CGetVolSiz( DrvQEl *DrvPtr )
{
	int fd = DrvPtr->fd;
	int deviceNumBlocks = 0;

#ifndef __MWERKS__
	ioctl(fd, DKIOCNUMBLKS, &deviceNumBlocks);
#else
//	deviceNumBlocks = 20480; /*10mb partition*/
 	deviceNumBlocks = 155656;
#endif

	return deviceNumBlocks;
}

OSErr GetBlock_FSGlue( int flags, UInt32 block, Ptr *buffer, int refNum, ExtendedVCB *vcb )
{
	return GetBlock_glue(flags, block, buffer, refNum, vcb);
}

#if 0
OSErr GetBlock_glue(UInt16 flags, UInt32 nodeNumber, Ptr *nodeBuffer, UInt16 refNum, ExtendedVCB *vcb)
{
	char *p = NULL;

	OSErr err;
	UInt32 diskBlock;
	UInt32 contiguousBytes;
	
	int readResult;
	off_t lseekResult;
	
	int fd;
	off_t offset;
	long numBytes;
	char pathname[32] = "/dev/sd0_hfs_a";
	
	int deviceBlockSize;
	int deviceNumBlocks;
	
	fd = open( pathname, O_RDONLY );
	if (fd < 0)
		return -77771;

#ifndef __MWERKS__
	ioctl( fd, DKIOCBLKSIZE, &deviceBlockSize );	
	ioctl( fd, DKIOCNUMBLKS, &deviceNumBlocks );
#else
	deviceBlockSize = 512;
	deviceNumBlocks = 20480; /*10mb partition*/
#endif

	if (refNum != 0xFFFF)
	{
		err = MapFileBlockC( vcb, GetFileControlBlock(refNum), deviceBlockSize, deviceBlockSize*nodeNumber,
			&diskBlock, &contiguousBytes );
		if (err)
			return -77772;
	}
	else
		diskBlock = nodeNumber;

	offset = diskBlock * deviceBlockSize;
	lseekResult = lseek( fd, offset, SEEK_SET );
	if ( lseekResult != offset )
		return -77773;

	p = malloc(deviceBlockSize);

	numBytes = deviceBlockSize;
	readResult = read( fd, p, numBytes );
	if ( readResult != numBytes )
		return -77774;

	*nodeBuffer = p;

	close(fd);
}

Boolean BlockCameFromDisk(void)
{
	return false;
}

#endif

struct CacheBlock {
	UInt32					blockType;
	short					flags;						//	status flags
	short					vRefNum;					//	vRefNum node belongs to
	ExtendedVCB				*vcb;						//	VCB for writes
	UInt32					blockNum;
	SInt16					refNum;
	SInt16					reserved;
	UInt32					blockSize;					//	This is repetitive, but the BTree node cache doesn't pass blockSize into ReleaseCacheBlock
	struct CacheBlock		*nextMRU;					//	next node in MRU order
	struct CacheBlock		*nextLRU;					//	next node in LRU order
	Ptr						buffer;						//	This always points right after ths field, but makes code cleaner (abstraction)
};
typedef struct CacheBlock CacheBlock;

OSErr ReadWriteBlock( CacheBlock *cBlock, Boolean isARead )
{
	OSErr	err				= 0;
	UInt32	bytesRead		= 0;
	UInt32	diskBlock;
	UInt32	contiguousBytes;
	
	long	ioReqCount;	
	long 	ioActCount;
	Ptr 	ioBuffer;
	off_t 	ioWPosOffset;

	int fd = (int)cBlock->vcb->vcbDrvNum;
	
	int deviceBlockSize;
	int deviceNumBlocks;

	off_t seek_off = 0;
	
#ifndef __MWERKS__
	ioctl( fd, DKIOCBLKSIZE, &deviceBlockSize );	
	ioctl( fd, DKIOCNUMBLKS, &deviceNumBlocks );
#else
	deviceBlockSize = 512;
//	deviceNumBlocks = 20480; /*10mb partition*/
 	deviceNumBlocks = 155656;
#endif

	do
	{
		if ( cBlock->refNum > 0 )			//	It's a file
		{
			err = MapFileBlockC( cBlock->vcb, GetFileControlBlock(cBlock->refNum), cBlock->blockSize,
				(cBlock->blockNum * cBlock->blockSize) + bytesRead, &diskBlock, &contiguousBytes );
			ioReqCount	= cBlock->blockSize - bytesRead;
			if ( contiguousBytes < ioReqCount )
				ioReqCount = contiguousBytes;
		}
		else								//	else read volume block
		{
			diskBlock	= cBlock->blockNum;
			ioReqCount	= cBlock->blockSize;
		}

		ioBuffer = cBlock->buffer + bytesRead;

		ioWPosOffset = ((off_t) diskBlock) << Log2BlkLo;

		seek_off = lseek( fd, ioWPosOffset, SEEK_SET );

		if ( isARead == true )
			ioActCount = read(fd, ioBuffer, ioReqCount);
		else
			ioActCount = write(fd, ioBuffer, ioReqCount);
#ifdef __MWERKS__
		if (ioReqCount!=ioActCount)
			__assertion_failed("ioReqCount!=ioActCount", __FILE__, __LINE__);
#endif

		bytesRead += ioActCount;
	
	} while ( (err == noErr) && (bytesRead < cBlock->blockSize) );
	
	return( err );
}

OSErr RelBlock_FSGlue( Ptr buffer, int flags )
{
	return RelBlock_glue(buffer,flags);
}

#if 0
OSErr RelBlock_glue (Ptr nodeBuffer, UInt16 flags)
{
	return 0;
}
#endif

SInt32 CalculateSafePhysicalTempMem( void )
{
	return LONG_MAX /*2147483647L*/;
}

void ClearMemory(void *start, UInt32 length)
{
	UInt8 *bytePtr = (UInt8 *)start;
	
	if ( bytePtr == 0 || length == 0)
		return;

	do
		*bytePtr++ = 0;
	while ( --length );
}

OSErr MountCheck( ExtendedVCB* volume, UInt32 *consistencyStatus )
{
	*consistencyStatus = 0;
	return 0;
}

pascal void GetDateTime(unsigned long *secs)
{
	*secs = 0;
}

OSErr C_FlushCache( ExtendedVCB *vcb, UInt32 flags, UInt32 refNum )
{
/*  Note: This function is still in assembly  */
	return 0;
}

OSErr C_FlushMDB( ExtendedVCB *volume )
{
/*  Note: This function is still in assembly  */
	return 0;
}

UInt32 LocalToUTC(UInt32 localTime)
{
	return localTime;
}

OSErr GetDiskBlocks( short driveNumber, unsigned long *numBlocks)
{
	short driverRefNum;
	DrvQEl *dqe;

	FindDrive (&driverRefNum, &dqe, driveNumber);
	*numBlocks = CGetVolSiz( dqe );
	return 0;
}
