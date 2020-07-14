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
    Copyright (c) 1987-98 Apple Computer, Inc.
    All Rights Reserved.

    THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
    The copyright notice above does not evidence any actual or
    intended publication of such source code.

    About vol.h:
    Contains public interfaces for the Rhapsody Volume/Directory ID (VDI) interface.

    To do:

    Change History:

     5-Jan-1999 Pat Dirks	Added new flag, 'kNoMapFS' as new OpenFork_VDI option.
     3-Dec-1998 Pat Dirks   Added new flag, 'kVolumeReadOnly' for GetVolumeInfo_VDI's attributes field.
     3-Dec-1998 Pat Dirks	Added 'flags' field to CommonAttributeInfo.  Added 'kObjectLocked' for attribute field.
    20-Nov-1998	Don Brady	Added constants from TextEncoding.h to enable specification of UTF-8 encoded
                            Unicode format (kTextEncodingUTF8) outside TextCommon.h definitions.
    17-Nov-1998	Pat Dirks	Changed to add _VDI suffix to all names, add unicodeCharacterCount argument,
                            nameEncodingFormat argument to GetCatalogInfo, ResolveFileID_VDI, and SearchParam.
    28-Jul-1998	Don Brady	Add kOpenReadOnly option for OpenFork_VDI (radar #2258546).
     8-Jul-1998	Pat Dirks	Added commonBitmap and volumeBitmap to VolumeAttributeInfo struct.
    28-May-1998	Pat Dirks	Changed to make GetCatalogInfo and SearchCatalog returns more
    						closely parallel; separate out name return and make it optional
    						in GetCatalogInfo.  Capitalize external type names.  Changed
    						'getVInfoBuf' to 'VolumeAttributeInfo'.  Changed 'SearchInfo'
    						to 'CatalogAttributeInfo' in 'SearchParam' struct.
     8-Apr-1998	chw			Changed arguments to useraccess_VDI to match kernel routine
    20-Mar-1998	Pat Dirks	Removed 'const' from pathname in GetCatalogInfo.
     5-Mar-1998	ser			Birth of a file.
*/

#ifndef __VOL__
#define __VOL__

// ************************************ I N C L U D E S ************************************

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/time.h>
#include <mach/message.h>
#include <sys/attr.h>

#define UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES 0

#ifdef __cplusplus
extern "C" {
#endif



// ************************************* D E F I N E S *************************************

// OpenFork_VDI fork names for standard HFS forks:

#define kHFSDataForkName "data"
#define kHFSResourceForkName "rsrc"

#define kReturnObjectName        0x00000001
#define kDontResolveAliases      0x00000002
#define kDontResolveSymlinks     0x00000004
#define kDontTranslateSeparators 0x00000008
#define kContiguous              0x00000010
#define kAllOrNothing            0x00000020
#define kOpenReadOnly            0x00000040
#define kNoMapFS                 0x00000080

#define kVolumeReadOnly          0x00008000

#define kObjectLocked            0x00000080

typedef enum  {
    kInvalidEncodingFormat = 0,
    kMacOSHFSFormat = 1
//  , kUnicodeUTF8Format = 2
//  , kUnicodeUTF7Format = 3
//  , kUnicode16BitFormat = 4
} FSTextEncodingFormat;


#ifndef __TEXTCOMMON__
/* Filename Encoding Values */
enum {
    kTextEncodingMacRoman		= 0L,
    kTextEncodingMacJapanese	= 1,

    kTextEncodingUnicodeV2_0	= 0x00000103,
};

#endif /* __TEXTCOMMON__ */

/* Text encodings not already defined in TextCommon.h: */
enum {
  kTextEncodingUTF8 = 0x08000103    /* UTF-8 encoded Unicode */
};

// ************************************ T Y P E D E F S ************************************


/* Data structures for the return of object information: */

struct CommonAttributeInfo
{
    short				objectType;
    dev_t				device;
    u_long				nameEncoding;
    u_long				objectID;
    u_long				instance;
    u_long				parentDirID;
    uid_t				uid;
    gid_t				gid;
    mode_t				mode;
    u_long				flags;
    nlink_t				nlink;
    u_long				finderInfo[8];
    struct timespec		creationTime;
    struct timespec		lastModificationTime;
    struct timespec		lastBackupTime;
    struct timespec		lastChangeTime;
};

struct DirectoryAttributeInfo
{
    u_long			numEntries;
};

struct FileAttributeInfo
{
    off_t			totalSize;		/* size of all forks, in blocks */
    u_long			blockSize;		/* Optimal file I/O blocksize */
    u_long			numForks;		/* Number of forks in the file */
    u_long			attributes;		/* See IM:Files 2-100 */
    off_t			dataLogicalLength;
    off_t			dataPhysicalLength;
    off_t			resourceLogicalLength;
    off_t			resourcePhysicalLength;
    dev_t			deviceType;
};

typedef struct CatalogAttributeInfo
{
    attrgroup_t					commonBitmap;
    attrgroup_t					fileBitmap;
    attrgroup_t					directoryBitmap;
    struct CommonAttributeInfo 	c;
    struct DirectoryAttributeInfo d;
    struct FileAttributeInfo	f;
} CatalogAttributeInfo, *CatalogAttributeInfoPtr;

typedef struct SearchParam
{
	CatalogAttributeInfoPtr searchInfo1;			// I - values and lower bounds
	CatalogAttributeInfoPtr searchInfo2;			// I - masks and upper bounds
	char			*targetNameString;
    size_t			unicodeCharacterCount;
    u_long			targetNameEncoding;
	size_t			bufferSize;
	void			*buffer;
	u_long			numFoundMatches;
	u_long			maxMatches;
    FSTextEncodingFormat nameEncodingFormat;
	u_long			timeLimit;						// I - max time to search
	struct searchstate	state;
} SearchParam, *SearchParamPtr;

typedef struct SearchInfoReturn
{
	u_long					size;
	CatalogAttributeInfo	ci;
	char					*name;
} SearchInfoReturn, *SearchInfoReturnPtr;

typedef struct VolumeAttributeInfo
{
    attrgroup_t			commonBitmap;
    attrgroup_t			volumeBitmap;
    fsvolid_t			volumeID;			// fill in later		
    u_char				name[512];			// Up to 255 Unicode chars 	
    u_long				nameEncoding;		// fill in later
    struct timespec		creationDate;		// fill in later
    struct timespec		lastModDate;		// fill in later
    struct timespec		lastBackupDate;		// fill in later
    u_long				attributes;			// See IM:Files 2-148
    u_long				numAllocBlocks;		// fill in later
    u_long				allocBlockSize;		// fill in later
    u_long				nextItemID;			// Next Dir or file ID
    u_long				numUnusedBlocks;	// fill in later
    u_long				signature;			// fill in later
    u_long				numFiles;			// Total number on volume
    u_long				numDirectories;		// Total number on volume
    u_long				finderInfo[8];		// fill in later
    u_long				mountFlags;			// flags specified at mount time
} VolumeAttributeInfo, *VolumeAttributeInfoPtr;

typedef struct VDISpec
{
    fsvolid_t			volID;
    u_long				dirID;
    u_char				name[512];			// Up to 255 Unicode chars 	
} VDISpec, *VDISpecPtr;

/* pointer to array of VDISpecs */
typedef VDISpecPtr VDIArrayPtr;

// *********************************** P R O T O T Y P E S ***********************************

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int FSChangeNotify_VDI 		(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theFileNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                struct timespec *theLastModDatePtr );
#endif

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int CancelChangeNotify_VDI 	(	fsvolid_t theVolID,
                              	u_long theDirID,
                                const char *theFileNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions );
#endif

int Create_VDI 				(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theFileNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                mode_t theMode );

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int CreateLink_VDI 			( 	fsvolid_t theVolID,
                                u_long theNewDirID,
                                const char *theNewNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theTargetDirID,
                                const char *theTargetNamePtr,
                                u_long theOptions );
#endif

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int CreateSymlink_VDI		(	fsvolid_t theNewVolID,
                                u_long theNewDirID,
                                const char *theNewNamePtr,
                                u_long unicodeCharacterCount,
                                u_long theNameEncoding,
                                fsvolid_t theTargetVolID,
                                u_long theTargetDirID,
                                const char *theTargetNamePtr,
                                u_long theOptions );
#endif

int ExchangeFiles_VDI		( 	fsvolid_t theVolID,
                                u_long theDirID1,
                                const char *theName1Ptr,
                                u_long unicodeCharacterCount1,
                                u_long nameEncoding,
                                u_long theDirID2,
                                const char *theName2Ptr,
                                u_long unicodeCharacterCount2,
                                u_long theOptions );

int ForkReserveSpace_VDI	( 	int theFD,
                                off_t theReservationSize,
                                u_long theOptions,
                                off_t *theSpaceAvailablePtr );

int GetCatalogInfo_VDI		(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theItemNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                CatalogAttributeInfoPtr theCatInfoPtr,
                                FSTextEncodingFormat nameEncodingFormat,
                                void* nameBuffer,
                                size_t nameBufferSize );

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int GetDirEntryAttr_VDI	(	void );
#endif

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int GetNamedAttribute_VDI	(	fsvolid_t theVolID,
                                u_long theItemID,
                                u_long theOptions,
                                const void *theAttributeNamePtr,
                                size_t theAttributeNameLength,
                                void *theBufferPtr,
                                size_t theBufferSize,
                                size_t *theSizeReturnedPtr );
#endif

#if UNIMPLEMENTED_VDI_ROUTINE_PROTOTYPES
int SetNamedAttribute_VDI  	(	fsvolid_t theVolID,
                                u_long theItemID,
                                u_long theOptions,
                                const void *theAttributeNamePtr,
                                size_t theAttributeNameLength,
                                void *theBufferPtr,
                                size_t theBufferSize );
#endif

int GetVolumeInfo_VDI		(	long theVolumeIndex,
                                fsvolid_t theVolID,
                                const char *theVolumeNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                VolumeAttributeInfoPtr theInfoPtr );

int MakeDir_VDI				(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theDirNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                mode_t theMode );

DIR *OpenDir_VDI			(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions );

int OpenFork_VDI			( 	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                const char *theForkNamePtr,
                                u_long theOptions );

int Remove_VDI				(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theFileNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions );

int RemoveDir_VDI			(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions );

int Rename_VDI				(	fsvolid_t theVolID,
                                u_long theOldDirID,
                                const char *theOldNamePtr,
                                u_long oldUnicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theNewDirID,
                                const char *theNewNamePtr,
                                u_long newUnicodeCharacterCount,
                                u_long theOptions );

int CreateFileID_VDI		(	fsvolid_t theVolID,
								u_long theDirID,
								const char *theNamePtr,
                                u_long unicodeCharacterCount,
								u_long nameEncoding,
                                u_long theOptions,
								u_long *theFileIDPtr );
								
int ResolveFileID_VDI		(	fsvolid_t theVolID,
                                u_long theFileID,
                                FSTextEncodingFormat nameEncodingFormat,
                                u_long *theDirIDPtr,
                                void* nameBuffer,
                                size_t nameBufferSize,
                                u_long *theOrigNameEncodingPtr );

int SearchCatalog_VDI( fsvolid_t volumeID, u_long options, SearchParam *searchPB );

int SetCatalogInfo_VDI		(	fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theItemNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                CatalogAttributeInfoPtr newInfoPtr );

int SetVolumeInfo_VDI		(	fsvolid_t theVolID,
                                u_long theOptions,
                                VolumeAttributeInfoPtr theInfoPtr );

int UserAccess_VDI 	(			fsvolid_t theVolID,
                                u_long theDirID,
                                const char *theNamePtr,
                                u_long unicodeCharacterCount,
                                u_long nameEncoding,
                                u_long theOptions,
                                uid_t theUserID,
                                gid_t *theGroupList,
                                int nGroups,
                                u_long theAccessRequired );


#ifdef __cplusplus
};
#endif

#endif	/* __VOL__ */
