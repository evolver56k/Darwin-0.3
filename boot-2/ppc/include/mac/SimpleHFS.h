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
/* -*-mode:C; tab-width: 4 -*- */
#ifndef SIMPLEHFS_H
#define SIMPLEHFS_H 1

#include <Types.h>

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

enum {
	kMaxExtents = 1024,
	
	kMDBBlockNumber = 2,
	kCatalogFileID = 4
};

struct Extent {
	UInt16 start;
	UInt16 length;
} GCC_PACKED;
typedef struct Extent Extent;


struct ExtentsArray {
	UInt32 nExtents;					// Count of valid entries that follow
	Extent extents[kMaxExtents];		// List of extents ascending by allocation block number
} GCC_PACKED;
typedef struct ExtentsArray ExtentsArray;


struct MDB {
	UInt16 signature;			/* always $4244 for HFS volumes */
	UInt32 createTime;			/* date/time volume created */
	UInt32 modifyTime;			/* date/time volume last modified */
	UInt16 attributes;
	UInt16 rootValence;			/* number of files in the root directory */
	UInt16 bitmapStart;			/* first block of allocation bitmap */
	UInt16 reserved;			/* used by Mac for allocation ptr */
	UInt16 size;				/* number of allocation blocks on the volume */
	UInt32 allocSize;			/* size of an allocation block */
	UInt32 clumpSize;			/* minimum bytes per allocation */
	UInt16 allocStart;			/* 1st 512 byte block mapped in bitmap (bitmap is in AU's though).*/
	UInt32 nextFID;				/* next available file ID */
	UInt16 freeCount;			/* number of free blocks on the volume */
	UInt8 name[28];				// Volume name as pascal string
	UInt32 backupTime;			/* date/time volume last backed-up */
	UInt16 sequenceNum;			/* volume sequence number */
	UInt32 writeCount;			/* number of writes to the volume */
	UInt32 extClumpSize;		/* clump size of extents B-tree file */
	UInt32 dirClumpSize;		/* clump size of directory B-tree file */
	UInt16 dirsInRoot;			/* number of directories in the root directory */
	UInt32 fileCount;			/* number of files on the volume */
	UInt32 dirCount;			/* number of directories on the volume */
	UInt8 finderInfo[32];		/* Finder information */
	UInt8 reserved2[6];			/* used internally */
	UInt32 extFileSize;			/* LEOF and PEOF of extents B-tree file */
	Extent extentsExtents[3];     /* first three extents of extents B-tree file */
	UInt32 dirFileSize;			/* LEOF and PEOF of directory B-tree file */
	Extent catalogExtents[3];     /* first three extents of directory B-tree file */
	UInt8 filler [350];
} GCC_PACKED;
typedef struct MDB MDB;


struct CatalogKey {
	UInt8 keyLength;				// Length of entire key record in bytes
	UInt8 reserved;					// Used internally to Mac OS HFS
	UInt32 parentID;				// Parent directory ID
	Str31 name;						// Name of this catalog node (variable length Pascal string)
} GCC_PACKED;
typedef struct CatalogKey CatalogKey;


struct ExtentsKey {
	UInt8 keyLength;				// Length of entire key record in bytes
	UInt8 resourceForkFlag;			// 0x00 for data fork, 0xFF for resource fork
	UInt32 fileID;					// File ID of file this extent is for
	UInt16 allocationBlockNumber;	// Allocation block number within the file
} GCC_PACKED;
typedef struct ExtentsKey ExtentsKey;


enum {
	kTemporaryFileType = 0,			// File flavors
	kRegularFileType = 1,
	
	kInvalidRecord = 0,				// Record types
	kDirectoryRecord,
	kFileRecord,
	kInvertedRecord,
	kDeletedRecord
};

struct FileRecord {
	UInt8 recordType;				// Record type for this record
	UInt8 flavor;					// File's flavor (regular or temporary)
	UInt8 flags;					// For file in use and locked bits
	UInt8 version;					// Infamous file version (always zero)

	OSType type;					// File's type
	OSType creator;					// File's creator
	UInt16 attributes;				// Finder flags
	SInt16 desktopY;				// Desktop position
	SInt16 desktopX;
	UInt16 reserved1;				// Ancient history

	UInt32 fileID;					// File's unique file identification number

	UInt16 dataStart;				// First allocation block of data fork
	UInt32 dataSize;				// Logical EOF of data fork in bytes
	UInt32 dataPhysSize;			// Physical EOF of data fork in bytes

	UInt16 resStart;				// First allocation block of resource fork
	UInt32 resSize;					// Logical EOF of resource fork
	UInt32 resPhysSize;				// Physical EOF of resource fork

	UInt32 createTime;				// Date/time of file's creation
	UInt32 modifyTime;				// Date/time of file's last modification
	UInt32 backupTime;				// Date/time of file's last backup
	
	SInt16 iconID;					// Resource ID of icon to display
	UInt16 reserved[3];				// Unused?
	UInt8 script;					// International script code for name
	UInt8 xFlags;					// Unused?
	SInt16 commentID;				// ID of file comments
	UInt32 homeID;					// Directory ID of "put away" directory

	UInt16 clumpSize;				// Clump size for file allocations (alblks)
	Extent dataExtents[3];			// First three extents of data fork
	Extent resExtents[3];			// First three extents of resource fork
	UInt32 reserved2;				// Reserved longword
} GCC_PACKED;
typedef struct FileRecord FileRecord;

enum {
	kLeafNodeType = 0xFF			// nodeType field of BTreeNode for leaf nodes
};

struct BTreeHeaderNode {
	UInt32 fLink;					// Node number of logical successor leaf node
	UInt32 bLink;					// Node number of logical predecessor leaf node
	UInt8 nodeType;					// Type of this node
	UInt8 nodeLevel;				// Level of this node (leaf nodes are always 1)
	UInt16 nRecords;				// Number of records in this node
	UInt32 filler;
	UInt32 root;					// Block number within file of root node
	UInt32 nRecordsInTree;			// Number of records in the B*Tree
	UInt32 firstLeaf;				// Block number in file of leftmost leaf
	UInt32 lastLeaf;				// Block number in file of rightmost leaf
	UInt16 nodeSize;				// Size of each node in bytes
	UInt16 keySize;					// Maximum size of each key in bytes
	UInt32 nNodes;					// Total number of allocated nodes in B*Tree file
	UInt32 nFreeNodes;				// Total number of free nodes in B*Tree file
	UInt8 filler2[468];				// Pad out to a 512-byte block size
} GCC_PACKED;
typedef struct BTreeHeaderNode BTreeHeaderNode;


struct BTreeNode {
	UInt32 fLink;					// Node number of logical successor leaf node
	UInt32 bLink;					// Node number of logical predecessor leaf node
	UInt8 nodeType;					// Type of this node
	UInt8 nodeLevel;				// Level of this node (leaf nodes are always 1)
	UInt16 nRecords;				// Number of records in this node
	char filler[500];				// Nodes are always 512 bytes
} GCC_PACKED;
typedef struct BTreeNode BTreeNode;

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#endif
