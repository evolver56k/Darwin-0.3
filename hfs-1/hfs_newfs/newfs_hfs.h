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
 * Copyright (c) 1998 Apple Computer, Inc. All Rights Reserved
 *
 *		MODIFICATION HISTORY (most recent first):
 *
 *	   30-Jul-1998	Don Brady		created
 */




enum {
	kMinHFSPlusVolumeSize	= (32*1024*1024),	// 32 MB
	
	kBytesPerSector			= 512,				// default block size	
	kBitsPerSector			= 4096,				// 512 * 8
	kLog2SectorSize			= 9,				// log2 of 512 when sector size = 512
	
	kHFSPlusDataClumpFactor	= 16,
	kHFSPlusRsrcClumpFactor = 16
};


enum {
	FALSE = 0,
	TRUE  = 1
};

/*
 *	This is the straight GMT conversion constant:
 *	00:00:00 January 1, 1970 - 00:00:00 January 1, 1904
 *	(3600 * 24 * ((365 * (1970 - 1904)) + (((1970 - 1904) / 4) + 1)))
 */
#define MAC_GMT_FACTOR		2082844800UL


/*	Test for error and return if error occurred*/
EXTERN_API_C( void )
ReturnIfError					(OSErr 					result);

#define	ReturnIfError(result)					if ( (result) != noErr ) return (result); else ;
/*	Test for passed condition and return if true*/
EXTERN_API_C( void )
ReturnErrorIf					(Boolean 				condition,
								 OSErr 					result);

#define	ReturnErrorIf(condition, error)			if ( (condition) )	return( (error) );
/*	Exit function on error*/
EXTERN_API_C( void )
ExitOnError						(OSErr 					result);

#define	ExitOnError( result )					if ( ( result ) != noErr )	goto ErrorExit; else ;
/*	Return the low 16 bits of a 32 bit value, pinned if too large*/
EXTERN_API_C( UInt16 )
LongToShort						(UInt32 				l);


struct DriveInfo {
	int			fd;
	int			totalSectors;
	int			sectorSize;
	int			sectorOffset;
};
typedef struct DriveInfo DriveInfo;


struct HFSDefaults {
	char 							sigWord[2];					/* signature word */
	long 							abSize;						/* allocation block size in bytes */
	long 							clpSize;					/* clump size in bytes */
	long 							nxFreeFN;					/* next free file number */
	long 							btClpSize;					/* B-Tree clump size in bytes */
	short 							rsrv1;						/* reserved */
	short 							rsrv2;						/* reserved */
	short 							rsrv3;						/* reserved */
};
typedef struct HFSDefaults HFSDefaults;

enum {
	kHFSPlusDefaultsVersion		= 1
};


struct HFSPlusDefaults {
	UInt16 							version;					/* version of this structure */
	UInt16 							flags;						/* currently undefined; pass zero */
	UInt32 							blockSize;					/* allocation block size in bytes */
	UInt32 							rsrcClumpSize;				/* clump size for resource forks */
	UInt32 							dataClumpSize;				/* clump size for data forks */
	UInt32 							nextFreeFileID;				/* next free file number */
	UInt32 							catalogClumpSize;			/* clump size for catalog B-tree */
	UInt32 							catalogNodeSize;			/* node size for catalog B-tree */
	UInt32 							extentsClumpSize;			/* clump size for extents B-tree */
	UInt32 							extentsNodeSize;			/* node size for extents B-tree */
	UInt32 							attributesClumpSize;		/* clump size for attributes B-tree */
	UInt32 							attributesNodeSize;			/* node size for attributes B-tree */
	UInt32 							allocationClumpSize;		/* clump size for allocation bitmap file */
};
typedef struct HFSPlusDefaults			HFSPlusDefaults;


extern OSErr InitHFSVolume (const DriveInfo *driveInfo, char* volumeName, HFSDefaults *defaults, UInt32 alignment);

extern OSErr InitializeHFSPlusVolume (const DriveInfo *driveInfo, char* volumeName, HFSPlusDefaults *defaults);

#if __STDC__
void	fatal(const char *fmt, ...);
#else
void	fatal();
#endif
