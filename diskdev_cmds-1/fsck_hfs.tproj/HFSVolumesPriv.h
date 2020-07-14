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
 	File:		HFSVolumesPriv.h
 
 	Contains:	On-disk data structures for HFS and HFS Plus volumes.
 
 	Version:	
 
 	DRI:		Don Brady
 
 	Copyright:	© 1984-1997 by Apple Computer, Inc.  All rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file contains unreleased SPI's
 
 	BuildInfo:	Built by:			
 				With Interfacer:	3.0d2   (PowerPC native)
 				From:				HFSVolumesPriv.i
 					Revision:		CS19
 					Dated:			11/16/97
 					Last change by:	djb
 					Last comment:	Bump volume format version for Unicode changes.
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __HFSVOLUMESPRIV__
#define __HFSVOLUMESPRIV__

#if TARGET_OS_MAC
#ifndef __TYPES__
#include <Types.h>
#endif
#ifndef __FILES__
#include <Files.h>
#endif
#ifndef __FINDER__
#include <Finder.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#ifndef __MYFILES__
#include "myFiles.h"
#endif
#endif 	/* TARGET_OS_MAC */

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

/* Signatures used to differentiate between HFS and HFS Plus volumes */

enum {
	kHFSSigWord					= 0x4244,						/* 'BD' in ASCII */
	kHFSPlusSigWord				= 0x482B,						/* 'H+' in ASCII */
	kHFSPlusVersion				= 0x0004,						/* will change as format changes (version 4 shipped with Mac OS 8.1) */
	kHFSPlusMountVersion		= FOUR_CHAR_CODE('8.10')		/* will change as implementations change */
};


/* CatalogNodeID is used to track catalog objects */
typedef UInt32 							CatalogNodeID;
/* Unicode strings are used for file and folder names (HFS Plus only) */
struct UniStr255 {
	UInt16 							length;						/* number of unicode characters */
	UniChar 						unicode[255];				/* unicode characters */
};
typedef struct UniStr255 UniStr255;

typedef const UniStr255 *				ConstUniStr255Param;

enum {
	kHFSMaxVolumeNameChars		= 27,
	kHFSMaxFileNameChars		= 31,
	kHFSPlusMaxFileNameChars	= 255,
	CMMaxCName					= kHFSMaxFileNameChars			/*€€ this will go away */
};


/* Extent overflow file data structures */
/* Small Extent key (HFS only) */
struct SmallExtentKey {
	UInt8 							keyLength;					/* length of key, excluding this field */
	UInt8 							forkType;					/* 0 = data fork, FF = resource fork */
	CatalogNodeID 					fileID;						/* file ID */
	UInt16 							startBlock;					/* first file allocation block number in this extent */
};
typedef struct SmallExtentKey SmallExtentKey;

/* Large Extent key (HFS Plus only) */
struct LargeExtentKey {
	UInt16 							keyLength;					/* length of key, excluding this field */
	UInt8 							forkType;					/* 0 = data fork, FF = resource fork */
	UInt8 							pad;						/* make the other fields align on 32-bit boundary */
	CatalogNodeID 					fileID;						/* file ID */
	UInt32 							startBlock;					/* first file allocation block number in this extent */
};
typedef struct LargeExtentKey LargeExtentKey;

/* Universal Extent Key */
union ExtentKey {
	SmallExtentKey 					small;
	LargeExtentKey 					large;
};
typedef union ExtentKey ExtentKey;


enum {
	kSmallExtentDensity			= 3,
	kLargeExtentDensity			= 8
};

/* Small extent descriptor (HFS only) */
struct SmallExtentDescriptor {
	UInt16 							startBlock;					/* first allocation block */
	UInt16 							blockCount;					/* number of allocation blocks */
};
typedef struct SmallExtentDescriptor SmallExtentDescriptor;

/* Large extent descriptor (HFS Plus only) */
struct LargeExtentDescriptor {
	UInt32 							startBlock;					/* first allocation block */
	UInt32 							blockCount;					/* number of allocation blocks */
};
typedef struct LargeExtentDescriptor LargeExtentDescriptor;

/* Universal extent descriptor */
union ExtentDescriptor {
	SmallExtentDescriptor 			small;
	LargeExtentDescriptor 			large;
};
typedef union ExtentDescriptor ExtentDescriptor;

/* Small extent record (HFS only) */
typedef SmallExtentDescriptor 			SmallExtentRecord[3];
/* Large extent record (HFS Plus only) */
typedef LargeExtentDescriptor 			LargeExtentRecord[8];
/* Universal extent record */
union ExtentRecord {
	SmallExtentRecord 				small;
	LargeExtentRecord 				large;
};
typedef union ExtentRecord ExtentRecord;


/* Fork data info (HFS Plus only) - 80 bytes */
struct ForkData {
	UInt64 							logicalSize;				/* fork's logical size in bytes */
	UInt32 							clumpSize;					/* fork's clump size in bytes */
	UInt32 							totalBlocks;				/* total blocks used by this fork */
	LargeExtentRecord 				extents;					/* initial set of extents */
};
typedef struct ForkData ForkData;

/* Permissions info (HFS Plus only) - 16 bytes */
struct Permissions {
	UInt32 							ownerID;					/* user or group ID of file/folder owner */
	UInt32 							groupID;					/* additional user of group ID */
	UInt32 							permissions;				/* permissions (bytes: unused, owner, group, everyone) */
	UInt32 							specialDevice;				/* UNIX: device for character or block special file */
};
typedef struct Permissions Permissions;

/* Catalog file data structures */

enum {
	kHFSRootParentID			= 1,							/* Parent ID of the root folder */
	kHFSRootFolderID			= 2,							/* Folder ID of the root folder */
	kHFSExtentsFileID			= 3,							/* File ID of the extents file */
	kHFSCatalogFileID			= 4,							/* File ID of the catalog file */
	kHFSBadBlockFileID			= 5,							/* File ID of the bad allocation block file */
	kHFSAllocationFileID		= 6,							/* File ID of the allocation file (HFS Plus only) */
	kHFSStartupFileID			= 7,							/* File ID of the startup file (HFS Plus only) */
	kHFSAttributesFileID		= 8,							/* File ID of the attribute file (HFS Plus only) */
	kBogusExtentFileID			= 15,							/* Used for exchanging extents in extents file */
	kFirstFreeCatalogNodeID		= 16
};


/* Small catalog key (HFS only) */
struct SmallCatalogKey {
	UInt8 							keyLength;					/* key length (in bytes) */
	UInt8 							reserved;					/* reserved (set to zero) */
	CatalogNodeID 					parentID;					/* parent folder ID */
	Str31 							nodeName;					/* catalog node name */
};
typedef struct SmallCatalogKey SmallCatalogKey;

/* Large catalog key (HFS Plus only) */
struct LargeCatalogKey {
	UInt16 							keyLength;					/* key length (in bytes) */
	CatalogNodeID 					parentID;					/* parent folder ID */
	UniStr255 						nodeName;					/* catalog node name */
};
typedef struct LargeCatalogKey LargeCatalogKey;

/* Universal catalog key */
union CatalogKey {
	SmallCatalogKey 				small;
	LargeCatalogKey 				large;
};
typedef union CatalogKey CatalogKey;


/* Catalog record types */

enum {
																/* HFS Catalog Records */
	kSmallFolderRecord			= 0x0100,						/* Folder record */
	kSmallFileRecord			= 0x0200,						/* File record */
	kSmallFolderThreadRecord	= 0x0300,						/* Folder thread record */
	kSmallFileThreadRecord		= 0x0400,						/* File thread record */
																/* HFS Plus Catalog Records */
	kLargeFolderRecord			= 1,							/* Folder record */
	kLargeFileRecord			= 2,							/* File record */
	kLargeFolderThreadRecord	= 3,							/* Folder thread record */
	kLargeFileThreadRecord		= 4								/* File thread record */
};


/* Catalog file record flags */

enum {
	kFileLockedBit				= 0x0000,						/* file is locked and cannot be written to */
	kFileLockedMask				= 0x0001,
	kFileThreadExistsBit		= 0x0001,						/* a file thread record exists for this file */
	kFileThreadExistsMask		= 0x0002
};


/* Small catalog folder record (HFS only) - 70 bytes */
struct SmallCatalogFolder {
	SInt16 							recordType;					/* record type */
	UInt16 							flags;						/* folder flags */
	UInt16 							valence;					/* folder valence */
	CatalogNodeID 					folderID;					/* folder ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							modifyDate;					/* date and time of last modification */
	UInt32 							backupDate;					/* date and time of last backup */
	DInfo 							userInfo;					/* Finder information */
	DXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							reserved[4];				/* reserved - set to zero */
};
typedef struct SmallCatalogFolder SmallCatalogFolder;

/* Large catalog folder record (HFS Plus only) - 88 bytes */
struct LargeCatalogFolder {
	SInt16 							recordType;					/* record type = HFS Plus folder record */
	UInt16 							flags;						/* file flags */
	UInt32 							valence;					/* folder's valence (limited to 2^16 in Mac OS) */
	CatalogNodeID 					folderID;					/* folder ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							contentModDate;				/* date and time of last content modification */
	UInt32 							attributeModDate;			/* date and time of last attribute modification */
	UInt32 							accessDate;					/* date and time of last access (Rhapsody only) */
	UInt32 							backupDate;					/* date and time of last backup */
	Permissions 					permissions;				/* permissions (for Rhapsody) */
	DInfo 							userInfo;					/* Finder information */
	DXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							textEncoding;				/* hint for name conversions */
	UInt32 							reserved;					/* reserved - set to zero */
};
typedef struct LargeCatalogFolder LargeCatalogFolder;

/* Small catalog file record (HFS only) - 102 bytes */
struct SmallCatalogFile {
	SInt16 							recordType;					/* record type */
	SInt8 							flags;						/* file flags */
	SInt8 							fileType;					/* file type (unused ?) */
	FInfo 							userInfo;					/* Finder information */
	CatalogNodeID 					fileID;						/* file ID */
	UInt16 							dataStartBlock;				/* not used - set to zero */
	SInt32 							dataLogicalSize;			/* logical EOF of data fork */
	SInt32 							dataPhysicalSize;			/* physical EOF of data fork */
	UInt16 							rsrcStartBlock;				/* not used - set to zero */
	SInt32 							rsrcLogicalSize;			/* logical EOF of resource fork */
	SInt32 							rsrcPhysicalSize;			/* physical EOF of resource fork */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							modifyDate;					/* date and time of last modification */
	UInt32 							backupDate;					/* date and time of last backup */
	FXInfo 							finderInfo;					/* additional Finder information */
	UInt16 							clumpSize;					/* file clump size (not used) */
	SmallExtentRecord 				dataExtents;				/* first data fork extent record */
	SmallExtentRecord 				rsrcExtents;				/* first resource fork extent record */
	UInt32 							reserved;					/* reserved - set to zero */
};
typedef struct SmallCatalogFile SmallCatalogFile;

/* Large catalog file record (HFS Plus only) - 248 bytes */
struct LargeCatalogFile {
	SInt16 							recordType;					/* record type = HFS Plus file record */
	UInt16 							flags;						/* file flags */
	UInt32 							reserved1;					/* reserved - set to zero */
	CatalogNodeID 					fileID;						/* file ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							contentModDate;				/* date and time of last content modification */
	UInt32 							attributeModDate;			/* date and time of last attribute modification */
	UInt32 							accessDate;					/* date and time of last access (Rhapsody only) */
	UInt32 							backupDate;					/* date and time of last backup */
	Permissions 					permissions;				/* permissions (for Rhapsody) */
	FInfo 							userInfo;					/* Finder information */
	FXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							textEncoding;				/* hint for name conversions */
	UInt32 							reserved2;					/* reserved - set to zero */

																/* start on double long (64 bit) boundry */
	ForkData 						dataFork;					/* size and block data for data fork */
	ForkData 						resourceFork;				/* size and block data for resource fork */
};
typedef struct LargeCatalogFile LargeCatalogFile;

/* Small catalog thread record (HFS only) - 46 bytes */
struct SmallCatalogThread {
	SInt16 							recordType;					/* record type */
	SInt32 							reserved[2];				/* reserved - set to zero */
	CatalogNodeID 					parentID;					/* parent ID for this catalog node */
	Str31 							nodeName;					/* name of this catalog node */
};
typedef struct SmallCatalogThread SmallCatalogThread;

/* Large catalog thread record (HFS Plus only) -- 264 bytes */
struct LargeCatalogThread {
	SInt16 							recordType;					/* record type */
	SInt16 							reserved;					/* reserved - set to zero */
	CatalogNodeID 					parentID;					/* parent ID for this catalog node */
	UniStr255 						nodeName;					/* name of this catalog node (variable length) */
};
typedef struct LargeCatalogThread LargeCatalogThread;

/* Universal catalog data record */
union CatalogRecord {
	SInt16 							recordType;
	SmallCatalogFolder 				smallFolder;
	SmallCatalogFile 				smallFile;
	SmallCatalogThread 				smallThread;
	LargeCatalogFolder 				largeFolder;
	LargeCatalogFile 				largeFile;
	LargeCatalogThread 				largeThread;
};
typedef union CatalogRecord CatalogRecord;

/*
  	Key for records in the attributes file.  Fields are compared in the order:
  		cnid, attributeName, startBlock
*/
struct AttributeKey {
	UInt16 							keyLength;					/* must set kBTBigKeysMask and kBTVariableIndexKeysMask in BTree header's attributes */
	UInt16 							pad;
	CatalogNodeID 					cnid;						/* file or folder ID */
	UInt32 							startBlock;					/* block # relative to start of attribute */
	UniStr255 						attributeName;				/* variable length */
};
typedef struct AttributeKey AttributeKey;

/*
  	These are the types of records in the attribute B-tree.  The values were chosen
  	so that they wouldn't conflict with the catalog record types.
*/

enum {
	kAttributeInlineData		= 0x10,							/* if size <  kAttrOverflowSize */
	kAttributeForkData			= 0x20,							/* if size >= kAttrOverflowSize */
	kAttributeExtents			= 0x30							/* overflow extents for large attributes */
};


/*
  	AttributeInlineData
  	For small attributes, whose entire value is stored within this one
  	B-tree record.  The key for this record is of the form:
  		{ CNID, name, 0 }.		(startBlock == 0)
  	There would not be any other records for this attribute.
*/
struct AttributeInlineData {
	UInt32 							recordType;					/*	= kAttributeInlineData*/
	UInt32 							logicalSize;				/*	size in bytes of userData*/
	Byte 							userData[2];				/*	variable length; space allocated is a multiple of 2 bytes*/
};
typedef struct AttributeInlineData AttributeInlineData;

/*
  	AttributeForkData
  	For larger attributes, whose value is stored in allocation blocks.
  	The key for this record is of the form:
  		{ CNID, name, 0 }.		(startBlock == 0)
  	If the attribute has more than 8 extents, there will be additonal
  	records (of type AttributeExtents) for this attribute.
*/
struct AttributeForkData {
	UInt32 							recordType;					/*	= kAttributeForkData*/
	UInt32 							reserved;
	ForkData 						theFork;					/*	size and first extents of value*/
};
typedef struct AttributeForkData AttributeForkData;

/*
  	AttributeExtents
  	This record contains information about overflow extents for large,
  	fragmented attributes.  The key for this record is of the form:
  		{ CNID, name, !=0 }.	(startBlock != 0)
  	The startBlock field of the key is the first allocation block number
  	(relative to the start of the attribute value) represented by the
  	extents in this record.
*/
struct AttributeExtents {
	UInt32 							recordType;					/*	= kAttributeExtents*/
	UInt32 							reserved;
	LargeExtentRecord 				extents;					/*	additional extents*/
};
typedef struct AttributeExtents AttributeExtents;

/*	A generic Attribute Record*/
union AttributeRecord {
	UInt32 							recordType;
	AttributeInlineData 			inlineData;
	AttributeForkData 				forkData;
	AttributeExtents 				overflowExtents;
};
typedef union AttributeRecord AttributeRecord;

/* Key and node lengths */

enum {
	kLargeExtentKeyMaximumLength = sizeof(LargeExtentKey) - sizeof(UInt16),
	kSmallExtentKeyMaximumLength = sizeof(SmallExtentKey) - sizeof(UInt8),
	kLargeCatalogKeyMaximumLength = sizeof(LargeCatalogKey) - sizeof(UInt16),
	kLargeCatalogKeyMinimumLength = kLargeCatalogKeyMaximumLength - sizeof(UniStr255) + sizeof(UInt16),
	kSmallCatalogKeyMaximumLength = sizeof(SmallCatalogKey) - sizeof(UInt8),
	kSmallCatalogKeyMinimumLength = kSmallCatalogKeyMaximumLength - sizeof(Str31) + sizeof(UInt8),
	kAttributeKeyMaximumLength	= sizeof(AttributeKey) - sizeof(UInt16),
	kAttributeKeyMinimumLength	= kAttributeKeyMaximumLength - sizeof(UniStr255) + sizeof(UInt16),
	kLargeCatalogMinimumNodeSize = 4096,
	kLargeExtentMinimumNodeSize	= 512,
	kAttributeMinimumNodeSize	= 4096
};



enum {
																/* Bits 0-6 are reserved (always cleared by MountVol call) */
	kVolumeHardwareLockBit		= 7,							/* volume is locked by hardware */
	kVolumeUnmountedBit			= 8,							/* volume was successfully unmounted */
	kVolumeSparedBlocksBit		= 9,							/* volume has bad blocks spared */
	kVolumeNoCacheRequiredBit	= 10,							/* don't cache volume blocks (i.e. RAM or ROM disk) */
	kBootVolumeInconsistentBit	= 11,							/* boot volume is inconsistent (System 7.6) */
																/* Bits 12-14 are reserved for future use */
	kVolumeSoftwareLockBit		= 15,							/* volume is locked by software */
	kVolumeHardwareLockMask		= 1 << kVolumeHardwareLockBit,
	kVolumeUnmountedMask		= 1 << kVolumeUnmountedBit,
	kVolumeSparedBlocksMask		= 1 << kVolumeSparedBlocksBit,
	kVolumeNoCacheRequiredMask	= 1 << kVolumeNoCacheRequiredBit,
	kBootVolumeInconsistentMask	= 1 << kBootVolumeInconsistentBit,
	kVolumeSoftwareLockMask		= 1 << kVolumeSoftwareLockBit,
	kMDBAttributesMask			= 0x8380
};


/* Master Directory Block (HFS only) - 162 bytes */
/* Stored at sector #2 (3rd sector) */
struct MasterDirectoryBlock {
	UInt16 							drSigWord;					/* volume signature */
	UInt32 							drCrDate;					/* date and time of volume creation */
	UInt32 							drLsMod;					/* date and time of last modification */
	UInt16 							drAtrb;						/* volume attributes */
	SInt16 							drNmFls;					/* number of files in root folder */
	UInt16 							drVBMSt;					/* first block of volume bitmap */
	UInt16 							drAllocPtr;					/* start of next allocation search */
	UInt16 							drNmAlBlks;					/* number of allocation blocks in volume */
	SInt32 							drAlBlkSiz;					/* size (in bytes) of allocation blocks */
	SInt32 							drClpSiz;					/* default clump size */
	SInt16 							drAlBlSt;					/* first allocation block in volume */
	UInt32 							drNxtCNID;					/* next unused catalog node ID */
	SInt16 							drFreeBks;					/* number of unused allocation blocks */
	Str27 							drVN;						/* volume name */

																/* Master Directory Block extensions for HFS */

	UInt32 							drVolBkUp;					/* date and time of last backup */
	UInt16 							drVSeqNum;					/* volume backup sequence number */
	SInt32 							drWrCnt;					/* volume write count */
	SInt32 							drXTClpSiz;					/* clump size for extents overflow file */
	SInt32 							drCTClpSiz;					/* clump size for catalog file */
	SInt16 							drNmRtDirs;					/* number of directories in root folder */
	SInt32 							drFilCnt;					/* number of files in volume */
	SInt32 							drDirCnt;					/* number of directories in volume */
	SInt32 							drFndrInfo[8];				/* information used by the Finder */
	UInt16 							drEmbedSigWord;				/* embedded volume signature (formerly drVCSize) */
	SmallExtentDescriptor 			drEmbedExtent;				/* embedded volume location and size (formerly drVBMCSize and drCtlCSize) */
	SInt32 							drXTFlSize;					/* size of extents overflow file */
	SmallExtentRecord 				drXTExtRec;					/* extent record for extents overflow file */
	SInt32 							drCTFlSize;					/* size of catalog file */
	SmallExtentRecord 				drCTExtRec;					/* extent record for catalog file */
};
typedef struct MasterDirectoryBlock MasterDirectoryBlock;

/* VolumeHeader (HFS Plus only) - 512 bytes */
/* Stored at sector #0 (1st sector) and last sector */
struct VolumeHeader {
	UInt16 							signature;					/* volume signature == 'H+' */
	UInt16 							version;					/* current version is kHFSPlusVersion */
	UInt32 							attributes;					/* volume attributes */
	UInt32 							lastMountedVersion;			/* implementation version which last mounted volume */
	UInt32 							reserved;					/* reserved - set to zero */

	UInt32 							createDate;					/* date and time of volume creation */
	UInt32 							modifyDate;					/* date and time of last modification */
	UInt32 							backupDate;					/* date and time of last backup */
	UInt32 							checkedDate;				/* date and time of last disk check */

	UInt32 							fileCount;					/* number of files in volume */
	UInt32 							folderCount;				/* number of directories in volume */

	UInt32 							blockSize;					/* size (in bytes) of allocation blocks */
	UInt32 							totalBlocks;				/* number of allocation blocks in volume (includes this header and VBM*/
	UInt32 							freeBlocks;					/* number of unused allocation blocks */

	UInt32 							nextAllocation;				/* start of next allocation search */
	UInt32 							rsrcClumpSize;				/* default resource fork clump size */
	UInt32 							dataClumpSize;				/* default data fork clump size */
	CatalogNodeID 					nextCatalogID;				/* next unused catalog node ID */

	UInt32 							writeCount;					/* volume write count */
	UInt64 							encodingsBitmap;			/* which encodings have been use  on this volume */

	UInt8 							finderInfo[32];				/* information used by the Finder */

	ForkData 						allocationFile;				/* allocation bitmap file */
	ForkData 						extentsFile;				/* extents B-tree file */
	ForkData 						catalogFile;				/* catalog B-tree file */
	ForkData 						attributesFile;				/* extended attributes B-tree file */
	ForkData 						startupFile;				/* boot file */
};
typedef struct VolumeHeader VolumeHeader;


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

#endif /* __HFSVOLUMESPRIV__ */

