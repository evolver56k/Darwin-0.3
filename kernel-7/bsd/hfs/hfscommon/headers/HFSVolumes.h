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
 	File:		HFSVolumes.h
 
	Contains:	On-disk data structures for HFS and HFS Plus volumes.

	Version:	Mac OS 8.1

	Copyright:	© 1984-1998 by Apple Computer, Inc.  All rights reserved.

	File Ownership:

		DRI:               Mark Day

		Other Contacts:    Deric Horn

		Technology:        File Systems

	Writers:

		(ngk)	Nick Kledzik
		(gap)	george puckett
		(djb)	Don Brady
		(msd)	Mark Day
		(DSH)	Deric Horn

	Change History (most recent first):

	From HFSVolumes.i:

	  <Rhap>	  4/9/98	djb		Use a different kHFSPlusMountVersion for Rhapsody.

		 <2>	 2/12/98	ngk		Clean up: add version field in this header, change Types.i to
									MacTypes.i, remove pragma comments, remove unnecesarry typedef
									struct Foo Foo statements.
		 <1>	  2/9/98	msd		first checked in
		 <0>	  2/5/98	msd		Imported from HFSVolumesPriv.i.  Added "HFS" to many names.
									Added reserved field to HFSPlusAttrInlineData.

	From HFSVolumesPriv.i:
	
	  <CS19>	11/16/97	djb		Bump volume format version for Unicode changes.
	  <CS18>	10/23/97	msd		Bug 1685113. Change kHFSPlusMountVersion to '8.10' to trigger
									MountCheck; it also happens to be a more correct value.
	  <CS17>	10/19/97	msd		Bug 1684586. Rename modifyDate to attributeModDate. GetCatInfo
									and SetCatInfo use only contentModDate.
	  <CS16>	 9/19/97	msd		Fix bug 1679778. In the LargeCatalogFile structure, change the
									name of the linkCount field to "reserved1" since we no longer
									support hard links.
	  <CS15>	 9/10/97	msd		Make sure fork and extent information is 8-byte aligned in
									attribute records.
	  <CS14>	  9/4/97	msd		Attributes are now identified by a single Unicode string (flags
									and OSTypes are gone), and may have extents (allowing them to be
									very large). Removed C define of bitmapFile. Removed
									kVolumeHardLinksBit. Added field lastMountedVersion to
									VolumeHeader; moved unused space near start of structure to make
									following fields nicely aligned. LargeExtentKey's keyLength is
									now a UInt16, like all the rest of the HFS Plus B-Trees. Change
									kHFSPlusVersion to $0003.
	  <CS13>	  9/2/97	DSH		Moved the VolumeHeader to sector 2 of HFS+ partition, bumped
									kHFSPlusVersion to 0X0002.
	  <CS12>	  8/5/97	msd		Change kHFSPlusSigWord to $482B ('H+'). Add enum for
									kHFSPlusVersion (set to $0001).
	  <CS11>	 6/24/97	djb		Add link count to HFS Plus file record. Add kVolumeHardLinksBit.
	  <CS10>	 6/20/97	msd		Remove the define for contentModDate.
	   <CS9>	 6/12/97	djb		Removed rootFileCount and rootFolderCount. Added new dates,
									permissions, and encodingBitmap, changed signature to "D8".
	   <CS8>	  6/6/97	djb		Removed enum names and #pragma enumsalwaysint.
	   <CS7>	  6/4/97	gap		Removed ForBride conditional. Caused problem in
									HFSVolumesPriv.a.
	   <CS6>	  6/4/97	gap		Add #pragma enumsalwaysint so file will work with latest
									Interfacer tool in Bride build.
	   <CS5>	 5/30/97	djb		Add constants for minimum key lengths.
	   <CS4>	 5/28/97	msd		Add AttributeKey and associated enums.
	   <CS3>	 5/20/97	DSH		Keep sigWord in sync with last build, D5.
	   <CS2>	 5/15/97	msd		Key length and fork type for extent records should be unsigned.
	   <CS1>	 4/28/97	djb		first checked in

	  <HFS7>	  4/7/97	msd		Add FileID for attributes BTree.
	  <HFS6>	  4/4/97	djb		Major update to HFS Plus structures.
	  <HFS5>	 3/17/97	DSH		Adding kNumHFSPlusExtents, and kNumHFSExtents.
	  <HFS4>	 2/19/97	djb		Latest HFS Plus format changes. Removed B-Tree Manager data
									structures since they're defined in BTreesPrivate.h.
	  <HFS3>	 2/13/97	msd		Add unions for extent key and data.
	  <HFS2>	 1/16/97	djb		Changed kLargeExtentKeyMaximumLength to 11.
	  <HFS1>	  1/9/97	djb		first checked in
		 <0>	  1/7/97	djb		Converted from HFSVolumePriv.h file.
*/
#ifndef __HFSVOLUMES__
#define __HFSVOLUMES__

#if TARGET_OS_MAC
	#ifndef __MACTYPES__
	#include <MacTypes.h>
	#endif
	#ifndef __FILES__
	#include <Files.h>
	#endif
	#ifndef __FINDER__
	#include <Finder.h>
	#endif
#endif /* TARGET_OS_MAC */

#if TARGET_OS_RHAPSODY
	#ifndef __MACOSTYPES__
	#include <MacOSTypes.h>
	#endif
	#ifndef __MACOSSTUBS__
	#include <MacOSStubs.h>
	#endif
#endif /* TARGET_OS_RHAPSODY */


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

#if TARGET_OS_MAC
	kHFSPlusMountVersion		= FOUR_CHAR_CODE('8.10')		/* will change as implementations change ('8.10' in Mac OS 8.1) */
#endif

#if TARGET_OS_RHAPSODY
	kHFSPlusMountVersion		= FOUR_CHAR_CODE('9.00')		/* will change as implementations change */
#endif


};


/* CatalogNodeID is used to track catalog objects */
typedef UInt32 							HFSCatalogNodeID;
/* Unicode strings are used for file and folder names (HFS Plus only) */

struct HFSUniStr255 {
	UInt16 							length;						/* number of unicode characters */
	UniChar 						unicode[255];				/* unicode characters */
};
typedef struct HFSUniStr255				HFSUniStr255;

typedef const HFSUniStr255 *			ConstHFSUniStr255Param;

enum {
	kHFSMaxVolumeNameChars		= 27,
	kHFSMaxFileNameChars		= 31,
	kHFSPlusMaxFileNameChars	= 255
};


/* Extent overflow file data structures */
/* HFS Extent key */

struct HFSExtentKey {
	UInt8 							keyLength;					/* length of key, excluding this field */
	UInt8 							forkType;					/* 0 = data fork, FF = resource fork */
	HFSCatalogNodeID 				fileID;						/* file ID */
	UInt16 							startBlock;					/* first file allocation block number in this extent */
};
typedef struct HFSExtentKey				HFSExtentKey;
/* HFS Plus Extent key */

struct HFSPlusExtentKey {
	UInt16 							keyLength;					/* length of key, excluding this field */
	UInt8 							forkType;					/* 0 = data fork, FF = resource fork */
	UInt8 							pad;						/* make the other fields align on 32-bit boundary */
	HFSCatalogNodeID 				fileID;						/* file ID */
	UInt32 							startBlock;					/* first file allocation block number in this extent */
};
typedef struct HFSPlusExtentKey			HFSPlusExtentKey;
/* Number of extent descriptors per extent record */

enum {
	kHFSExtentDensity			= 3,
	kHFSPlusExtentDensity		= 8
};

/* HFS extent descriptor */

struct HFSExtentDescriptor {
	UInt16 							startBlock;					/* first allocation block */
	UInt16 							blockCount;					/* number of allocation blocks */
};
typedef struct HFSExtentDescriptor		HFSExtentDescriptor;
/* HFS Plus extent descriptor */

struct HFSPlusExtentDescriptor {
	UInt32 							startBlock;					/* first allocation block */
	UInt32 							blockCount;					/* number of allocation blocks */
};
typedef struct HFSPlusExtentDescriptor	HFSPlusExtentDescriptor;
/* HFS extent record */

typedef HFSExtentDescriptor 			HFSExtentRecord[3];
/* HFS Plus extent record */
typedef HFSPlusExtentDescriptor 		HFSPlusExtentRecord[8];

/* Fork data info (HFS Plus only) - 80 bytes */

struct HFSPlusForkData {
	UInt64 							logicalSize;				/* fork's logical size in bytes */
	UInt32 							clumpSize;					/* fork's clump size in bytes */
	UInt32 							totalBlocks;				/* total blocks used by this fork */
	HFSPlusExtentRecord 			extents;					/* initial set of extents */
};
typedef struct HFSPlusForkData			HFSPlusForkData;
/* Permissions info (HFS Plus only) - 16 bytes */

struct HFSPlusPermissions {
	UInt32 							ownerID;					/* user or group ID of file/folder owner */
	UInt32 							groupID;					/* additional user of group ID */
	UInt32 							permissions;				/* permissions (bytes: unused, owner, group, everyone) */
	UInt32 							specialDevice;				/* UNIX: device for character or block special file */
};
typedef struct HFSPlusPermissions		HFSPlusPermissions;
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
	kHFSBogusExtentFileID		= 15,							/* Used for exchanging extents in extents file */
	kHFSFirstUserCatalogNodeID	= 16
};


/* HFS catalog key */

struct HFSCatalogKey {
	UInt8 							keyLength;					/* key length (in bytes) */
	UInt8 							reserved;					/* reserved (set to zero) */
	HFSCatalogNodeID 				parentID;					/* parent folder ID */
	Str31 							nodeName;					/* catalog node name */
};
typedef struct HFSCatalogKey			HFSCatalogKey;
/* HFS Plus catalog key */

struct HFSPlusCatalogKey {
	UInt16 							keyLength;					/* key length (in bytes) */
	HFSCatalogNodeID 				parentID;					/* parent folder ID */
	HFSUniStr255 					nodeName;					/* catalog node name */
};
typedef struct HFSPlusCatalogKey		HFSPlusCatalogKey;

/* Catalog record types */

enum {
																/* HFS Catalog Records */
	kHFSFolderRecord			= 0x0100,						/* Folder record */
	kHFSFileRecord				= 0x0200,						/* File record */
	kHFSFolderThreadRecord		= 0x0300,						/* Folder thread record */
	kHFSFileThreadRecord		= 0x0400,						/* File thread record */
																/* HFS Plus Catalog Records */
	kHFSPlusFolderRecord		= 1,							/* Folder record */
	kHFSPlusFileRecord			= 2,							/* File record */
	kHFSPlusFolderThreadRecord	= 3,							/* Folder thread record */
	kHFSPlusFileThreadRecord	= 4								/* File thread record */
};


/* Catalog file record flags */

enum {
	kHFSFileLockedBit			= 0x0000,						/* file is locked and cannot be written to */
	kHFSFileLockedMask			= 0x0001,
	kHFSThreadExistsBit			= 0x0001,						/* a file thread record exists for this file */
	kHFSThreadExistsMask		= 0x0002
};


/* HFS catalog folder record - 70 bytes */

struct HFSCatalogFolder {
	SInt16 							recordType;					/* record type */
	UInt16 							flags;						/* folder flags */
	UInt16 							valence;					/* folder valence */
	HFSCatalogNodeID 				folderID;					/* folder ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							modifyDate;					/* date and time of last modification */
	UInt32 							backupDate;					/* date and time of last backup */
	DInfo 							userInfo;					/* Finder information */
	DXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							reserved[4];				/* reserved - set to zero */
};
typedef struct HFSCatalogFolder			HFSCatalogFolder;
/* HFS Plus catalog folder record - 88 bytes */

struct HFSPlusCatalogFolder {
	SInt16 							recordType;					/* record type = HFS Plus folder record */
	UInt16 							flags;						/* file flags */
	UInt32 							valence;					/* folder's valence (limited to 2^16 in Mac OS) */
	HFSCatalogNodeID 				folderID;					/* folder ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							contentModDate;				/* date and time of last content modification */
	UInt32 							attributeModDate;			/* date and time of last attribute modification */
	UInt32 							accessDate;					/* date and time of last access (Rhapsody only) */
	UInt32 							backupDate;					/* date and time of last backup */
	HFSPlusPermissions 				permissions;				/* permissions (for Rhapsody) */
	DInfo 							userInfo;					/* Finder information */
	DXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							textEncoding;				/* hint for name conversions */
	UInt32 							reserved;					/* reserved - set to zero */
};
typedef struct HFSPlusCatalogFolder		HFSPlusCatalogFolder;
/* HFS catalog file record - 102 bytes */

struct HFSCatalogFile {
	SInt16 							recordType;					/* record type */
	SInt8 							flags;						/* file flags */
	SInt8 							fileType;					/* file type (unused ?) */
	FInfo 							userInfo;					/* Finder information */
	HFSCatalogNodeID 				fileID;						/* file ID */
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
	HFSExtentRecord 				dataExtents;				/* first data fork extent record */
	HFSExtentRecord 				rsrcExtents;				/* first resource fork extent record */
	UInt32 							reserved;					/* reserved - set to zero */
};
typedef struct HFSCatalogFile			HFSCatalogFile;
/* HFS Plus catalog file record - 248 bytes */

struct HFSPlusCatalogFile {
	SInt16 							recordType;					/* record type = HFS Plus file record */
	UInt16 							flags;						/* file flags */
	UInt32 							reserved1;					/* reserved - set to zero */
	HFSCatalogNodeID 				fileID;						/* file ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							contentModDate;				/* date and time of last content modification */
	UInt32 							attributeModDate;			/* date and time of last attribute modification */
	UInt32 							accessDate;					/* date and time of last access (Rhapsody only) */
	UInt32 							backupDate;					/* date and time of last backup */
	HFSPlusPermissions 				permissions;				/* permissions (for Rhapsody) */
	FInfo 							userInfo;					/* Finder information */
	FXInfo 							finderInfo;					/* additional Finder information */
	UInt32 							textEncoding;				/* hint for name conversions */
	UInt32 							reserved2;					/* reserved - set to zero */

																/* start on double long (64 bit) boundry */
	HFSPlusForkData 				dataFork;					/* size and block data for data fork */
	HFSPlusForkData 				resourceFork;				/* size and block data for resource fork */
};
typedef struct HFSPlusCatalogFile		HFSPlusCatalogFile;
/* HFS catalog thread record - 46 bytes */

struct HFSCatalogThread {
	SInt16 							recordType;					/* record type */
	SInt32 							reserved[2];				/* reserved - set to zero */
	HFSCatalogNodeID 				parentID;					/* parent ID for this catalog node */
	Str31 							nodeName;					/* name of this catalog node */
};
typedef struct HFSCatalogThread			HFSCatalogThread;
/* HFS Plus catalog thread record -- 264 bytes */

struct HFSPlusCatalogThread {
	SInt16 							recordType;					/* record type */
	SInt16 							reserved;					/* reserved - set to zero */
	HFSCatalogNodeID 				parentID;					/* parent ID for this catalog node */
	HFSUniStr255 					nodeName;					/* name of this catalog node (variable length) */
};
typedef struct HFSPlusCatalogThread		HFSPlusCatalogThread;

/*
  	These are the types of records in the attribute B-tree.  The values were chosen
  	so that they wouldn't conflict with the catalog record types.
*/

enum {
	kHFSPlusAttrInlineData		= 0x10,							/* if size <  kAttrOverflowSize */
	kHFSPlusAttrForkData		= 0x20,							/* if size >= kAttrOverflowSize */
	kHFSPlusAttrExtents			= 0x30							/* overflow extents for large attributes */
};


/*
  	HFSPlusAttrInlineData
  	For small attributes, whose entire value is stored within this one
  	B-tree record.
  	There would not be any other records for this attribute.
*/

struct HFSPlusAttrInlineData {
	UInt32 							recordType;					/*	= kHFSPlusAttrInlineData*/
	UInt32 							reserved;
	UInt32 							logicalSize;				/*	size in bytes of userData*/
	Byte 							userData[2];				/*	variable length; space allocated is a multiple of 2 bytes*/
};
typedef struct HFSPlusAttrInlineData	HFSPlusAttrInlineData;
/*
  	HFSPlusAttrForkData
  	For larger attributes, whose value is stored in allocation blocks.
  	If the attribute has more than 8 extents, there will be additonal
  	records (of type HFSPlusAttrExtents) for this attribute.
*/

struct HFSPlusAttrForkData {
	UInt32 							recordType;					/*	= kHFSPlusAttrForkData*/
	UInt32 							reserved;
	HFSPlusForkData 				theFork;					/*	size and first extents of value*/
};
typedef struct HFSPlusAttrForkData		HFSPlusAttrForkData;
/*
  	HFSPlusAttrExtents
  	This record contains information about overflow extents for large,
  	fragmented attributes.
*/

struct HFSPlusAttrExtents {
	UInt32 							recordType;					/*	= kHFSPlusAttrExtents*/
	UInt32 							reserved;
	HFSPlusExtentRecord 			extents;					/*	additional extents*/
};
typedef struct HFSPlusAttrExtents		HFSPlusAttrExtents;
/*	A generic Attribute Record*/

union HFSPlusAttrRecord {
	UInt32 							recordType;
	HFSPlusAttrInlineData 			inlineData;
	HFSPlusAttrForkData 			forkData;
	HFSPlusAttrExtents 				overflowExtents;
};
typedef union HFSPlusAttrRecord			HFSPlusAttrRecord;
/* Key and node lengths */

enum {
	kHFSPlusExtentKeyMaximumLength = sizeof(HFSPlusExtentKey) - sizeof(UInt16),
	kHFSExtentKeyMaximumLength	= sizeof(HFSExtentKey) - sizeof(UInt8),
	kHFSPlusCatalogKeyMaximumLength = sizeof(HFSPlusCatalogKey) - sizeof(UInt16),
	kHFSPlusCatalogKeyMinimumLength = kHFSPlusCatalogKeyMaximumLength - sizeof(HFSUniStr255) + sizeof(UInt16),
	kHFSCatalogKeyMaximumLength	= sizeof(HFSCatalogKey) - sizeof(UInt8),
	kHFSCatalogKeyMinimumLength	= kHFSCatalogKeyMaximumLength - sizeof(Str31) + sizeof(UInt8),
	kHFSPlusCatalogMinNodeSize	= 4096,
	kHFSPlusExtentMinNodeSize	= 512,
	kHFSPlusAttrMinNodeSize		= 4096
};


/* HFS and HFS Plus volume attribute bits */

enum {
																/* Bits 0-6 are reserved (always cleared by MountVol call) */
	kHFSVolumeHardwareLockBit	= 7,							/* volume is locked by hardware */
	kHFSVolumeUnmountedBit		= 8,							/* volume was successfully unmounted */
	kHFSVolumeSparedBlocksBit	= 9,							/* volume has bad blocks spared */
	kHFSVolumeNoCacheRequiredBit = 10,							/* don't cache volume blocks (i.e. RAM or ROM disk) */
	kHFSBootVolumeInconsistentBit = 11,							/* boot volume is inconsistent (System 7.6 and later) */
																/* Bits 12-14 are reserved for future use */
	kHFSVolumeSoftwareLockBit	= 15,							/* volume is locked by software */
	kHFSVolumeHardwareLockMask	= 1 << kHFSVolumeHardwareLockBit,
	kHFSVolumeUnmountedMask		= 1 << kHFSVolumeUnmountedBit,
	kHFSVolumeSparedBlocksMask	= 1 << kHFSVolumeSparedBlocksBit,
	kHFSVolumeNoCacheRequiredMask = 1 << kHFSVolumeNoCacheRequiredBit,
	kHFSBootVolumeInconsistentMask = 1 << kHFSBootVolumeInconsistentBit,
	kHFSVolumeSoftwareLockMask	= 1 << kHFSVolumeSoftwareLockBit,
	kHFSMDBAttributesMask		= 0x8380
};


/* Master Directory Block (HFS only) - 162 bytes */
/* Stored at sector #2 (3rd sector) */

struct HFSMasterDirectoryBlock {

																/* These first fields are also used by MFS */

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
	HFSExtentDescriptor 			drEmbedExtent;				/* embedded volume location and size (formerly drVBMCSize and drCtlCSize) */
	SInt32 							drXTFlSize;					/* size of extents overflow file */
	HFSExtentRecord 				drXTExtRec;					/* extent record for extents overflow file */
	SInt32 							drCTFlSize;					/* size of catalog file */
	HFSExtentRecord 				drCTExtRec;					/* extent record for catalog file */
};
typedef struct HFSMasterDirectoryBlock	HFSMasterDirectoryBlock;

/* HFSPlusVolumeHeader (HFS Plus only) - 512 bytes */
/* Stored at sector #2 (1st sector) and sector n-2 */

struct HFSPlusVolumeHeader {
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
	HFSCatalogNodeID 				nextCatalogID;				/* next unused catalog node ID */

	UInt32 							writeCount;					/* volume write count */
	UInt64 							encodingsBitmap;			/* which encodings have been use  on this volume */

	UInt8 							finderInfo[32];				/* information used by the Finder */

	HFSPlusForkData 				allocationFile;				/* allocation bitmap file */
	HFSPlusForkData 				extentsFile;				/* extents B-tree file */
	HFSPlusForkData 				catalogFile;				/* catalog B-tree file */
	HFSPlusForkData 				attributesFile;				/* extended attributes B-tree file */
	HFSPlusForkData 				startupFile;				/* boot file */
};
typedef struct HFSPlusVolumeHeader		HFSPlusVolumeHeader;

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

#endif /* __HFSVOLUMES__ */
