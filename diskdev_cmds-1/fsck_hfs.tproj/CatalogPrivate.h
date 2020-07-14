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
	File:		CatalogPrivate.h

	Contains:	Private Catalog Manager interfaces.

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(JL)	Jim Luther
		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	  <CS10>	11/20/97	djb		Radar #2002357. Fixing retry mechanism.
	   <CS9>	11/17/97	djb		PrepareInputName routine now returns an error.
	   <CS8>	11/13/97	djb		Radar #1683572. Move CatalogIterator to this file from
									FileMgrInternal.i. Double size of short unicode name.
	   <CS7>	10/31/97	JL		#2000184 - Changed prototypes for CreateFileThreadID and
									ExchangeFiles.
	   <CS6>	10/17/97	msd		In CatalogCacheGlobals, add room for a single UniStr255 so
									catalog iterators can step over long Unicode names.
	   <CS5>	10/17/97	djb		Add ConvertInputNameToUnicode for Catalog Create/Rename.
	   <CS4>	 10/1/97	djb		Change catalog iterator implementation.
	   <CS3>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS2>	 6/24/97	djb		Add LocateCatalogNodeByMangledName routine.
	   <CS1>	 6/24/97	djb		first checked in
*/

#ifndef	__CATALOGPRIVATE__
#define __CATALOGPRIVATE__

#include	"FileMgrInternal.h"
#include	"BTreesInternal.h"
#include	"HFSVolumesPriv.h"

// private catalog data cache



enum {
	kCatalogIteratorCount = 16		// total number of Catalog iterators (shared by all HFS/HFS Plus volumes)
};


// Catalog Iterator Types
enum {
	kIterateAll,			// from GetCatInfo call
	kIterateFilesOnly		// from GetFileInfo call
};


// Catalog Iterator Name Types
enum {
	kShortPascalName,
	kShortUnicodeName,
	kLongUnicodeName	// non-local name
};


// short unicode name (used by CatalogIterator)
struct UniStr63 {
	UInt16		length;					/* number of unicode characters */
	UniChar		unicode[63];			/* unicode characters */
};
typedef struct UniStr63 UniStr63;


struct CatalogIterator
{
	struct CatalogIterator *	nextMRU;	//	next iterator in MRU order
	struct CatalogIterator *	nextLRU;	//	next iterator in LRU order

	UInt16				iteratorType;		// {0 = GetCatInfo, 1 = GetFileInfo}
	SInt16				volRefNum;
	CatalogNodeID		folderID;
	SInt16				currentIndex;
	SInt16				fileIndex;			// for GetFileInfo (to skip folders)
	UInt32				btreeNodeHint;		// node the key was last seen in
	UInt16				btreeIndexHint;		// index the key was last seen at
	UInt16				nameType;			// { 0 = Pascal, 1 = Unicode, 3 = long name}
	CatalogNodeID		parentID;			// parent folder ID
	union
	{
		Str31			pascalName;
		UniStr63		unicodeName;
		UniStr255 *		longNamePtr;
	} folderName;
};
typedef struct CatalogIterator CatalogIterator;


struct CatalogCacheGlobals {
	UInt32				iteratorCount;	//	Number of iterators in cache
	CatalogIterator *	mru;
	CatalogIterator *	lru;
	UInt32				reserved;
	UniStr255			longName;		//	used by a single kLongUnicodeName iterator
};
typedef struct CatalogCacheGlobals CatalogCacheGlobals;


struct CatalogDataCache
{
	Str31				lastNamePascal;		//€€ not needed for HFS volumes!
	UniStr255			lastNameUnicode;	//€€ not needed for HFS volumes!
	TextEncoding		lastEncoding;
};
typedef struct CatalogDataCache CatalogDataCache;


// Private Catalog Macros

// We keep a copy of the text encoding in the upper byte of the catalog hint (yuck!).
// This overloading was done in the first release since the client is in 68K assembly
// and it saves information (like the hint) in registers.
//
// The lower 24 bits is the b-tree node hint which allows for 16 million nodes (or a
// 67 GB Catalog B-tree).
//
// The text encoding needs to be a formal parameter in future releases!!
//
extern TextEncoding GetTextEncodingFromHint(UInt32 hint);
#define	GetTextEncodingFromHint(hint)				((hint)>>24)

extern void SetTextEncodingInHint(TextEncoding encoding, UInt32 *hint);
#define	SetTextEncodingInHint(encoding, hint)		(*(hint) += ((encoding)<<24))

extern UInt32 GetBTreeNodeFromHint(UInt32 hint);
#define	GetBTreeNodeFromHint(hint)					((hint) & 0x00FFFFFF)


//
// Private Catalog Manager Routines (for use only by Catalog Manager, CatSearch and FileID Services)
//

extern	OSErr	LocateCatalogThread( const ExtendedVCB *volume, CatalogNodeID nodeID, CatalogRecord *threadData,
									 UInt16 *threadSize, UInt32 *threadHint);

extern	OSErr	LocateCatalogNode(	const ExtendedVCB *volume, CatalogNodeID folderID, const CatalogName *name,
									UInt32 hint, CatalogKey *key, CatalogRecord *data, UInt32 *newHint);

extern OSErr	LocateCatalogRecord( const ExtendedVCB *volume, CatalogNodeID folderID, const CatalogName *name,
									 UInt32 hint, CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *newHint);

extern OSErr	LocateCatalogNodeWithRetry ( const ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param pascalName,
											 CatalogName *unicodeName, UInt32 hint, CatalogKey *keyPtr, CatalogRecord *dataPtr,
											 UInt32 *newHint );

extern OSErr	LocateCatalogNodeByMangledName( const ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param name,
												CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *hintPtr );

extern OSErr	FlushCatalog( ExtendedVCB *volume);

extern void		InvalidateCatalogNodeCache( ExtendedVCB *volume, CatalogNodeID parentID );

extern OSErr	UpdateFolderCount( ExtendedVCB *volume, CatalogNodeID parentID, const CatalogName *name, SInt16 newType,
								   UInt32 hint, SInt16 valenceDelta);

extern UInt16	GetCatalogRecordSize( const CatalogRecord *dataRecord);

extern OSErr	PrepareInputName( ConstStr31Param name, Boolean unicode, TextEncoding encoding, CatalogName *catalogName);

extern void		ConvertInputNameToUnicode(ConstStr31Param name, TextEncoding encodingHint,
										  TextEncoding *actualEncoding, CatalogName *catalogName);

extern	void	BuildCatalogKey( CatalogNodeID parentID, const CatalogName *name, Boolean isHFSPlus,
								 CatalogKey *key);

extern	void	UpdateCatalogName( ConstStr31Param srcName, Str31 destName);

extern UInt32	CatalogNameLength( const CatalogName *name, Boolean isHFSPlus);

extern void		CopyCatalogName( const CatalogName *srcName, CatalogName *dstName, Boolean isHFSPLus);

extern OSErr	ResolveFileID( ExtendedVCB *vcb, CatalogNodeID fileID, CatalogNodeID *parentID, Str31 name );

extern OSErr	CreateFileThreadID( FIDParam *filePB, WDCBRecPtr *wdcbPtr );

extern OSErr	ExchangeFiles( FIDParam *filePB, WDCBRecPtr *wdcbPtr );

extern void		CopyCatalogNodeData( const ExtendedVCB *volume, const CatalogRecord *dataPtr, CatalogNodeData *nodeData);

extern void		UpdateVolumeEncodings( ExtendedVCB *volume, TextEncoding encoding);

extern void		AdjustVolumeCounts( ExtendedVCB *volume, SInt16 type, SInt16 delta );


// Catalog Iterator Routines

extern CatalogIterator* GetCatalogIterator( const ExtendedVCB *volume, CatalogNodeID folderID, UInt16 index, UInt16 iterationType );

extern void		TrashCatalogIterator( const ExtendedVCB *volume, CatalogNodeID folderID );

void			AgeCatalogIterator( CatalogIterator *catalogIterator );

extern void		UpdateBtreeIterator( const CatalogIterator *catalogIterator, BTreeIterator *btreeIterator );

extern void		UpdateCatalogIterator( const BTreeIterator *btreeIterator, CatalogIterator *catalogIterator );


#endif //__CATALOGPRIVATE__
