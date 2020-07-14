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
 	File:		FileMgrInternal.h
 
 	Contains:	IPI for File Manager (HFS Plus)
 
 	Version:	HFS Plus 1.0
 
 	DRI:		Don Brady
 
 	Copyright:	© 1996-1997 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			
 				With Interfacer:	3.0d2   (PowerPC native)
 				From:				FileMgrInternal.i
 					Revision:		CS29
 					Dated:			12/10/97
 					Last change by:	DSH
 					Last comment:	2201501, Overload the NodeData valence field for over 2 Gig file
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __FILEMGRINTERNAL__
#define __FILEMGRINTERNAL__

#if TARGET_OS_MAC
#ifndef __TYPES__
#include <Types.h>
#endif
#ifndef __FILES__
#include <Files.h>
#endif
#ifndef __FSM__
#include <FSM.h>
#endif
#ifndef __LOWMEM__
#include <LowMem.h>
#endif
#ifndef __OSUTILS__
#include <OSUtils.h>
#endif
#ifndef __PROCESSES__
#include <Processes.h>
#endif
#ifndef __TEXTCOMMON__
#include <TextCommon.h>
#endif
#ifndef __UNICODECONVERTER__
#include <UnicodeConverter.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */
#ifndef __HFSVOLUMESPRIV__
#include <HFSVolumesPriv.h>
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

/* internal error codes*/

enum {
																/* FXM errors*/
	fxRangeErr					= 16,							/* file position beyond mapped range*/
	fxOvFlErr					= 17,							/* extents file overflow*/
																/* Unicode errors*/
	uniTooLongErr				= 24,							/* Unicode string too long to convert to Str31*/
	uniBufferTooSmallErr		= 25,							/* Unicode output buffer too small*/
	uniNotMappableErr			= 26,							/* Unicode string can't be mapped to given script*/
																/* BTree Manager errors*/
	btNotFound					= 32,							/* record not found*/
	btExists					= 33,							/* record already exists*/
	btNoSpaceAvail				= 34,							/* no available space*/
	btNoFit						= 35,							/* record doesn't fit in node */
	btBadNode					= 36,							/* bad node detected*/
	btBadHdr					= 37,							/* bad BTree header record detected*/
	dsBadRotate					= 64,							/* bad BTree rotate*/
																/* Catalog Manager errors*/
	cmNotFound					= 48,							/* CNode not found*/
	cmExists					= 49,							/* CNode already exists*/
	cmNotEmpty					= 50,							/* directory CNode not empty (valence = 0)*/
	cmRootCN					= 51,							/* invalid reference to root CNode*/
	cmBadNews					= 52,							/* detected bad catalog structure*/
	cmFThdDirErr				= 53,							/* thread belongs to a directory not a file*/
	cmFThdGone					= 54,							/* file thread doesn't exist*/
	cmParentNotFound			= 55,							/* CNode for parent ID does not exist*/
																/* TFS internal errors*/
	fsDSIntErr					= -127							/* Internal file system error*/
};


/* internal flags*/


enum {
																/* File System busy flag:*/
																/* Bit zero of FSBusy (lomem $360) is true when the file system is running.*/
																/* The word at $360 is cleared when the file system is exited. The*/
																/* bits defined here are for additional flags in the FSBusy word that are*/
																/* valid only when the file system is running.*/
	fsBusyBit					= 0,							/* file system is running; other FSBusy bits are valid*/
	fsSCSIDefer					= 1,							/* file system is waiting for SCSI transaction to complete*/
	fsIntMaskDefer				= 2,							/* file system is waiting until the interrupt mask is lowered*/
																/* Flag bits in HFSFlags byte:*/
	hfsReq						= 0,							/* Set if request is specific to HFS*/
	dirCN						= 1,							/* Set if a CNode is a directory*/
	reportMissingParent			= 4,							/* tell Catalog to report missing parents (used by MakeFSSpec)*/
	skipPMSP					= 5,							/* Set to skip PMSP setup (one-shot)*/
	noPMSP						= 6,							/* Set to disable PMSP completely (status flag)*/
	hfsContd					= 7,							/* Set if Async trap is continued*/
																/* fsFlags values*/
	fsNoAllocate				= 0,
	fsNoAllocateMask			= 0x01,							/* true when allocating memory is a very bad idea*/
	fsNeedFCBs					= 1,
	fsNeedFCBsMask				= 0x02,							/* true when a local FCB couldn't be found	*/
	fsNoFCBExpansion			= 2,
	fsNoFCBExpansionMask		= 0x04,							/* true if no FCB expansion logic is desired*/
																/*	ExtendFile option flags*/
																/*	extendFileAllBit			= 0,				|* allocate all requested bytes or none *|*/
																/*	extendFileAllMask			= 0x0001,*/
																/*	*/
																/*	extendFileContigBit			= 1,				|* force contiguous allocation *|*/
																/*	extendFileContigMask		= 0x0002*/
	kEFContigBit				= 1,							/*	force contiguous allocation*/
	kEFContigMask				= 0x02,
	kEFAllBit					= 0,							/*	allocate all requested bytes or none*/
	kEFAllMask					= 0x01,							/*	TruncateFile option flags*/
	kTFTrunExtBit				= 0,							/*	truncate to the extent containing new PEOF*/
	kTFTrunExtMask				= 1
};



enum {
	HFSStkLen					= 1792,							/* old stack size (pre HFS Plus)*/
	kFileSystemStackSlop		= 16,							/* additional temporary space*/
	kFileSystemStackSize		= 16384,						/* give us more breathing room*/
	kFileSystemVersion			= FOUR_CHAR_CODE('2.0A'),		/* current file system version*/
																/*	31744 = $7C00, a nice round number close to*/
																/*	(32767*1000)/1024, which is about the largest */
																/*	free space unsuspecting, decimal-K minded apps*/
																/*	might be expected to handle.*/
																/*	AlBlkLim*/
	kMaxHFSAllocationBlocks		= 31744,
	WDRfnMin					= -32767,						/* lowest assigned WD RefNum*/
	WDRfnMax					= -4096,						/* largest possible WDrefnum*/
	kFirstFileRefnum			= 2,							/* smallest FCB refnum*/
	kNoHint						= 0
};


/* Internal LowMem pointers*/

/*€€ The following should really be in LowMemPriv.i*/

enum {
	FSCallAsync					= 0x0342,						/*	ONE BYTE FREE*/
	NoEject						= 0x034B,						/* used by Eject and Offline*/
	CacheFlag					= 0x0377,
	SysBMCPtr					= 0x0378,						/* System-wide bitmap cache pointer*/
	SysCtlCPtr					= 0x0380,						/* System-wide control cache pointer*/
	HFSDSErr					= 0x0392,						/* Final gasp - error that caused IOErr.*/
	LMParamBlock				= 0x03A4,						/* LMGetParams() just gives us a copy of it*/
	FSVarsPtr					= 0x0BB8,						/* lomem that points to file system variable block*/
	CacheVars					= 0x0394,
	HFSStkPtr					= 0x036E,						/* Temporary location of HFS Stack pointer*/
	FSIOErr						= 0x03DE,						/* last I/O error (NEXT WORD FREE)*/
																/* file manager vectors not found in LowMemPriv.i*/
	JUpdAltMDB					= (0xED) * 4 + 0x0400,			/* ($A0ED) $0400 is n/OSTable*/
	JCkExtFS					= (0xEE) * 4 + 0x0400,			/* ($A0EE) $0400 is n/OSTable*/
	JBMChk						= (0xF0) * 4 + 0x0400,			/* ($A0F0) $0400 is n/OSTable*/
	JTstMod						= (0xF1) * 4 + 0x0400,			/* ($A0F1) $0400 is n/OSTable*/
	JLocCRec					= (0xF2) * 4 + 0x0400,			/* ($A0F2) $0400 is n/OSTable*/
	JTreeSearch					= (0xF3) * 4 + 0x0400,			/* ($A0F3) $0400 is n/OSTable*/
	JMapFBlock					= (0xF4) * 4 + 0x0400,			/* ($A0F4) $0400 is n/OSTable*/
	JXFSearch					= (0xF5) * 4 + 0x0400,			/* ($A0F5) $0400 is n/OSTable*/
	JReadBM						= (0xF6) * 4 + 0x0400			/* ($A0F6) $0400 is n/OSTable*/
};


/* Poor Man's Search Path*/

struct SearchPathHeader {
	Ptr 							PMSPHook;					/* Hook for PMSP modification*/
	short 							PMSPIndx;					/* Index to PMSP index from start of PMSP*/
};
typedef struct SearchPathHeader SearchPathHeader;

struct SearchPathEntry {
	short 							spVRefNum;					/* VRefNum in PMSP entry*/
	UInt32 							spDirID;					/* Directory ID in PMSP entry*/
};
typedef struct SearchPathEntry SearchPathEntry;


enum {
	kPoorMansSearchIndex		= -2,
	MaxDVCnt					= 8,							/* Leave room for 8 default VRefNums*/
	PMSPSize					= MaxDVCnt * sizeof(SearchPathEntry) + sizeof(SearchPathHeader) + 2
};



enum {
	fsWDCBExtendCount			= 8,							/* # of WDCB's to add when we run out*/
																/*	FileIDs variables*/
	kNumExtentsToCache			= 4								/*	just guessing for ExchangeFiles*/
};


enum {
	kInvalidMRUCacheKey			= -1L,							/* flag to denote current MRU cache key is invalid*/
	kDefaultNumMRUCacheBlocks	= 16							/* default number of blocks in each cache*/
};




/* Internal Data structures*/

struct ExtendedVCB {
	QElemPtr 						qLink;
	SInt16 							qType;
	SInt16 							vcbFlags;
	UInt16 							vcbSigWord;
	UInt32 							vcbCrDate;
	UInt32 							vcbLsMod;
	SInt16 							vcbAtrb;
	UInt16 							vcbNmFls;
	SInt16 							vcbVBMSt;
	SInt16 							vcbAllocPtr;
	UInt16 							vcbNmAlBlks;
	SInt32 							vcbAlBlkSiz;
	SInt32 							vcbClpSiz;
	SInt16 							vcbAlBlSt;
	SInt32 							vcbNxtCNID;
	UInt16 							vcbFreeBks;
	Str27 							vcbVN;
	SInt16 							vcbDrvNum;
	SInt16 							vcbDRefNum;
	SInt16 							vcbFSID;
	SInt16 							vcbVRefNum;
	Ptr 							vcbMAdr;
	Ptr 							vcbBufAdr;
	SInt16 							vcbMLen;
	SInt16 							vcbDirIndex;
	SInt16 							vcbDirBlk;
	UInt32 							vcbVolBkUp;
	UInt16 							vcbVSeqNum;
	SInt32 							vcbWrCnt;
	SInt32 							vcbXTClpSiz;
	SInt32 							vcbCTClpSiz;
	UInt16 							vcbNmRtDirs;
	SInt32 							vcbFilCnt;
	SInt32 							vcbDirCnt;
	SInt32 							vcbFndrInfo[8];
	UInt16 							vcbEmbedSigWord;			/* embedded volume signature (formerly vcbVCSize)*/
	SmallExtentDescriptor 			vcbEmbedExtent;				/* embedded volume location and size (formerly vcbVBMCSiz and vcbCtlCSiz)*/
	UInt16 							vcbXTAlBlks;
	UInt16 							vcbCTAlBlks;
	SInt16 							extentsRefNum;
	SInt16 							catalogRefNum;
	Ptr 							vcbCtlBuf;
	SInt32 							vcbDirIDM;
	SInt16 							vcbOffsM;

																/*	Extended VCB information*/
	UInt16 							reserved;					/* align to 4-byte boundry*/
	UInt32 							hfsPlusIOPosOffset;			/*	Disk block where HFS+ starts	*/

	SInt16 							allocationsRefNum;
	SInt16 							attributesRefNum;			/*	volume's attributes file, if any*/
	UInt32 							allocationsClumpSize;		/*	clump size for allocations file (also in fcb)*/
	UInt32 							attributesClumpSize;		/*	clump size for attributes file (also in fcb)*/

	UInt32 							blockSize;					/*	size of allocation blocks - vcbAlBlkSiz*/
	UInt32 							totalBlocks;				/*	number of allocation blocks in volume (includes this header and VBM) - vcbNmAlBlks*/
	UInt32 							freeBlocks;					/*	number of unused allocation blocks - vcbFreeBks*/
	UInt32 							nextAllocation;				/*	start of next allocation search - vcbAllocPtr*/

	UInt32 							checkedDate;				/*	date and time of last disk check	*/

	LogicalAddress 					catalogDataCache;			/* cached catalog data //€€ this should go away!*/

	UInt64 							encodingsBitmap;			/* HFS Plus only*/
	TextEncoding 					volumeNameEncodingHint;		/* Text encoding used for volume name*/

	Ptr 							hintCachePtr;				/* points to this volumes heuristicHint cache*/
};
typedef struct ExtendedVCB ExtendedVCB;

struct FCB {
	UInt32 							fcbFlNm;					/* FCB file number. Non-zero marks FCB used */
	SignedByte 						fcbFlags;					/* FCB flags */
	SignedByte 						fcbTypByt;					/* File type byte */
	UInt16 							fcbSBlk;					/* File start block (in alloc size blks) */
	UInt32 							fcbEOF;						/* Logical length or EOF in bytes */
	UInt32 							fcbPLen;					/* Physical file length in bytes */
	UInt32 							fcbCrPs;					/* Current position within file */
	ExtendedVCB *					fcbVPtr;					/* Pointer to the corresponding ExtendedVCB */
	Ptr 							fcbBfAdr;					/* File's buffer address */
	unsigned short 					fcbFlPos;					/* Directory block this file is in */
	UInt32 							fcbClmpSize;				/* Number of bytes per clump */
	Ptr 							fcbBTCBPtr;					/* Pointer to B*-Tree control block for file */
	SmallExtentRecord 				fcbExtRec;					/* First 3 file extents */
	OSType 							fcbFType;					/* File's 4 Finder Type bytes */
	UInt32 							fcbCatPos;					/* Catalog hint for use on Close */
	UInt32 							fcbDirID;					/* Parent Directory ID */
	Str31 							fcbCName;					/* CName of open file */
};
typedef struct FCB FCB;

struct ExtendedFCB {
	UInt32 							processID1;					/*	these two fields are the Process Manager PID that*/
	UInt32 							processID2;					/*	opened the file (used to clean up at process death).*/
	LargeExtentRecord 				extents;					/*	extents for HFS+ volumes*/
};
typedef struct ExtendedFCB ExtendedFCB;

struct ParallelFCB {
	UInt16 							count;						/*	number of ExtendedFCB's*/
	UInt16 							unitSize;					/*	size of a single ExtendedFCB (should be sizeof(ExtendedFCB))*/
	ExtendedFCB 					extendedFCB[1];
};
typedef struct ParallelFCB ParallelFCB;

struct ExtendedWDCB {
	UInt32 							processID1;
	UInt32 							processID2;
};
typedef struct ExtendedWDCB ExtendedWDCB;


struct ParallelWDCB {
	UInt16 							count;						/*	number of ExtendedFCB's*/
	UInt16 							unitSize;					/*	size of a single ExtendedFCB (should be sizeof(ExtendedFCB))*/
	ExtendedWDCB 					extendedWDCB[1];
};
typedef struct ParallelWDCB ParallelWDCB;

struct WDCBArray {
	UInt16 							count;
	WDCBRec 						wdcb[1];
};
typedef struct WDCBArray WDCBArray;

struct FCBArray {
	UInt16 							length;						/*	first word is FCB part length*/
	FCB 							fcb[1];						/*	fcb array*/
};
typedef struct FCBArray FCBArray;

/*	Volumes*/


enum {
	vcbMaxNam					= 27,							/* volumes currently have a 27 byte max name length*/
																/* VCB flags*/
	vcbManualEjectMask			= 0x0001,						/* bit 0	manual-eject bit: set if volume is in a manual-eject drive*/
	vcbFlushCriticalInfoMask	= 0x0002,						/* bit 1	critical info bit: set if critical MDB information needs to flush*/
																/*	IoParam->ioVAtrb*/
	kDefaultVolumeMask			= 0x0020,
	kFilesOpenMask				= 0x0040
};


/* Catalog Node Data - universal data returned from the Catalog Manager*/


enum {
	kCatalogFolderNode			= FOUR_CHAR_CODE('fold'),
	kCatalogFileNode			= FOUR_CHAR_CODE('file'),		/* mask bits for afp's three inhibit bits which live in fdXFlags in xFndrInfo*/
	xFFFilAttrLockMask			= 0x70
};

/*	valence is overloaded for files and used as additional flags. 2201501*/

enum {
	kLargeDataForkMask			= 0x00000001,
	kLargeRsrcForkMask			= 0x00000002
};

struct CatalogNodeData {
	OSType 							nodeType;					/* 'file' or 'fold' */
	UInt32 							nodeFlags;					/* node flags */
	CatalogNodeID 					nodeID;						/* node ID */
	UInt32 							createDate;					/* date and time of creation */
	UInt32 							contentModDate;				/* date and time of last fork modification */
	UInt32 							backupDate;					/* date and time of last backup */
	UInt8 							finderInfo[16];				/* Finder information part 1 */
	UInt8 							extFinderInfo[16];			/* Finder information part 2 */

	UInt32 							valence;					/* valence fot folders, overloaded for files for additional flags (larger than 2 Gig flag) */
	UInt32 							textEncoding;				/* hint for name conversions */

	UInt32 							dataLogicalSize;			/* files only */
	UInt32 							dataPhysicalSize;			/* files only */
	UInt32 							rsrcLogicalSize;			/* files only */
	UInt32 							rsrcPhysicalSize;			/* files only */

	LargeExtentRecord 				dataExtents;				/* files only */
	LargeExtentRecord 				rsrcExtents;				/* files only */
};
typedef struct CatalogNodeData CatalogNodeData;

/* Universal catalog name*/

union CatalogName {
	Str31 							pstr;
	UniStr255 						ustr;
};
typedef union CatalogName CatalogName;

/* Unicode Conversion*/


enum {
	kMacBaseEncodingCount		= 50,
	kTextEncodingUndefined		= 0x00007FFF
};

#ifdef INVESTIGATE
struct ConversionContext {
	TextToUnicodeInfo 				toUnicode;
	UnicodeToTextInfo 				fromUnicode;
};
typedef struct ConversionContext ConversionContext;
#endif

struct CallProfile {
	UInt16 							refCount;
	UInt16 							errCount;
	UInt32 							callCount;
	UInt32 							minTime;
	UInt32 							maxTime;
	UInt64 							totalTime;
	UInt64 							startBase;					/* in nanoseconds*/
};
typedef struct CallProfile CallProfile;


struct FSVarsRec {
	short 							length;
	void *							gBlockCacheGlobals;
	long 							XFS;
	long 							BTMgr;
	long 							DTDBMgr;
	long 							FSMgr;
	long 							QMgr;
	ParallelFCB *					fcbPBuf;					/*	Parallel FCB array*/
	long 							wdcbPBuf;
	short 							reserved1;
	DeferredTask 					fsDefer;					/* <in OSUtils.h>*/
	long 							dtOwnCall;
	long 							FSMHook;
	short 							fsSelector;
	Byte 							fsFlags;
	Boolean 						gBlockCacheDirty;
	ExtendedVCB *					dsRecoverVCBPtr;			/* pointer to VCB of volume we're doing a disk switch for*/
	unsigned char *					dsRecoverNamePtr;			/* points to name of volume we're doing a disk switch for*/
	UInt16 							fsFCBBurst;					/* # of FCBs that we'd like to keep free*/
	UInt16 							fsFCBGrow;					/* # of FCBs to make free when we grow the array*/
	UInt16 							fsFCBMax;					/* max # of FCBs to allow*/
	UInt16 							fsFCBCounter;				/* counts down as files are opened*/
	UInt32 							later[4];					/* and a few more for later*/

	void *							userStackPtr;				/* user's A7 stack pointer*/
	FSSpec 							gCatalogFSSpec;				/* space to hold catalog manager's FSSpec output (70 bytes)*/
	Boolean 						gUseDynamicUnicodeConverters;
	Boolean 						gIsUnicodeInstalled;
	CatalogNodeData 				gCatalogData;				/* space to hold catalog manager's universal node data (212 bytes)*/

	UInt32 							gDefaultBaseEncoding;
	ItemCount 						gInstalledEncodings;
	ConversionContext 				gConversionContext[50];
	UniversalProcPtr 				uppCreateTextToUnicodeInfo;
	UniversalProcPtr 				uppCreateUnicodeToTextInfo;
	UniversalProcPtr 				uppConvertFromTextToUnicode;
	UniversalProcPtr 				uppConvertFromUnicodeToText;
	UniversalProcPtr 				uppUpgradeScriptInfoToTextEncoding;
	UniversalProcPtr 				uppRevertTextEncodingToScriptInfo;
	UniversalProcPtr 				uppLockMappingTable;

	UInt32 							gNativeCPUtype;				/* what processor are we running on*/
	UInt32 							gTimeBaseFactor;			/* adjustment for µsec*/
	CallProfile 					gCallProfile[32];			/* call profiles for unicode and b-tree calls*/

	Ptr 							gAttributesBuffer;			/* used by attributes code to read/construct keys and/or data*/
	UInt32 							gAttributesBufferSize;		/* size of block pointed to by gAttributesBuffer*/
	SInt32 							offsetToUTC;				/* added to local time to produce UTC (GMT)*/
	Ptr 							gBootPToUTable;				/* used by boot code to find Extensions folder*/
	UniversalProcPtr 				oldWriteXPRam;				/* previous WriteXPRam trap address*/
	Ptr 							gCatalogCacheGlobals;		/* private catalog cache (for iterators)*/
	CatalogKey 						gLastCatalogKey;			/* keep a copy for UpdateCatalogNode*/
#ifdef INVESTIGATE
	CatalogRecord 					gLastCatalogRecord;			/* keep a copy for UpdateCatalogNode*/
#endif
	StringPtr 						gTextEncodingFontName;		/* points to font name (only used when no HFS Plus volumes have been mounted)*/
};
typedef struct FSVarsRec FSVarsRec;


EXTERN_API_C( void )
LMSetCacheFlag					(Byte 					value);

/* cache usage count now used as cache flag*/
#define	LMSetCacheFlag( value )				(*(Ptr) 0x0377 = ( value ))

/* Internal macros*/
/* Disk First Aid constants*/

enum {
	kHFSStage					= 0,
	kVerifyStage				= 1,
	kRepairStage				= 2
};

#if FORDISKFIRSTAID
EXTERN_API_C( FSVarsRec *)
LMGetFSVars						(void);

#define 	LMGetFSVars()			LMGetFSMVars()
EXTERN_API_C( Ptr )
LMGetFCBTable					(void);

#define 	LMGetFCBTable()			LMGetFCBSPtr()
EXTERN_API_C( UInt32 )
GetDFAStage						(void);

#else
EXTERN_API_C( FSVarsRec *)
LMGetFSVars						(void);

#define 	LMGetFSVars()			(*(FSVarsRec**) 0x0BB8)
EXTERN_API_C( Ptr )
LMGetFCBTable					(void);

#define 	LMGetFCBTable()			(*(Ptr*) 0x034E)
EXTERN_API_C( UInt32 )
GetDFAStage						(void);

#define 	GetDFAStage()			(UInt32) kHFSStage
#endif  /* (FORDISKFIRSTAID) */

EXTERN_API_C( short )
GetFileRefNumFromFCB			(FCB *					fcb);

#define	GetFileRefNumFromFCB(fcb) 			((short) ( ((UInt32) (fcb)) - ((UInt32) LMGetFCBTable() ) ))
EXTERN_API_C( FCB *)
GetFileControlBlock				(short 					refNum);

#define	GetFileControlBlock(refNum) 		((FCB*) ( (Ptr)LMGetFCBTable() + refNum ) )
EXTERN_API_C( Boolean )
BlockCameFromDisk				(void);

#if !FORDISKFIRSTAID
#define	BlockCameFromDisk()					( *(*(UInt16 **)CacheVars + 29) == 0)
#endif  /* ( !FORDISKFIRSTAID) */

/*	The following macro marks a VCB as dirty by setting the upper 8 bits of the flags*/
EXTERN_API_C( void )
MarkVCBDirty					(ExtendedVCB *			vcb);

#define	MarkVCBDirty(vcb)	((void) (vcb->vcbFlags |= 0xFF00))
EXTERN_API_C( void )
MarkVCBClean					(ExtendedVCB *			vcb);

#define	MarkVCBClean(vcb)	((void) (vcb->vcbFlags &= 0x00FF))
EXTERN_API_C( Boolean )
IsVCBDirty						(ExtendedVCB *			vcb);

#define	IsVCBDirty(vcb)		((Boolean) ((vcb->vcbFlags & 0xFF00) != 0))
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

#define	LongToShort( l )	l <= (UInt32)0x0000FFFF ? ((UInt16) l) : ((UInt16) 0xFFFF)


/* Catalog Manager Routines (IPI)*/
EXTERN_API_C( OSErr )
CreateVolumeCatalogCache		(ExtendedVCB *			volume);

EXTERN_API_C( OSErr )
DisposeVolumeCatalogCache		(ExtendedVCB *			volume);

EXTERN_API_C( OSErr )
CreateCatalogNode				(ExtendedVCB *			volume,
								 CatalogNodeID 			parentID,
								 ConstStr31Param 		name,
								 SInt16 				nodeType,
								 CatalogNodeID *		catalogNodeID,
								 UInt32 *				catalogHint);

EXTERN_API_C( OSErr )
DeleteCatalogNode				(ExtendedVCB *			volume,
								 CatalogNodeID 			parentID,
								 ConstStr31Param 		name,
								 UInt32 				hint);

EXTERN_API_C( OSErr )
GetCatalogNode					(ExtendedVCB *			volume,
								 CatalogNodeID 			parentID,
								 ConstStr31Param 		name,
								 UInt32 				hint,
								 FSSpec *				nodeSpec,
								 CatalogNodeData *		nodeData,
								 UInt32 *				newHint);

EXTERN_API_C( OSErr )
GetCatalogOffspring				(ExtendedVCB *			volume,
								 CatalogNodeID 			folderID,
								 UInt16 				index,
								 FSSpec *				spec,
								 CatalogNodeData *		nodeData,
								 UInt32 *				hint);

EXTERN_API_C( OSErr )
MoveCatalogNode					(ExtendedVCB *			volume,
								 CatalogNodeID 			folderID,
								 ConstStr31Param 		name,
								 UInt32 				srcHint,
								 CatalogNodeID 			destFolderID,
								 UInt32 *				newHint);

EXTERN_API_C( OSErr )
RenameCatalogNode				(ExtendedVCB *			volume,
								 CatalogNodeID 			folderID,
								 ConstStr31Param 		oldName,
								 ConstStr31Param 		newName,
								 UInt32 				inHint,
								 UInt32 *				outHint);

EXTERN_API_C( OSErr )
RenameCatalogNodeUnicode		(ExtendedVCB *			volume,
								 CatalogNodeID 			folderID,
								 const CatalogName *	oldName,
								 const CatalogName *	newName,
								 TextEncoding 			newEncoding,
								 UInt32 				inHint,
								 UInt32 *				outHint);

EXTERN_API_C( OSErr )
UpdateCatalogNode				(ExtendedVCB *			volume,
								 const CatalogNodeData * nodeData,
								 UInt32 				catalogHint);

EXTERN_API_C( OSErr )
LinkCatalogNode					(ExtendedVCB *			volume,
								 CatalogNodeID 			parentID,
								 ConstStr31Param 		name,
								 CatalogNodeID 			linkParentID,
								 ConstStr31Param 		linkName);

EXTERN_API_C( SInt32 )
CompareCatalogKeys				(SmallCatalogKey *		searchKey,
								 SmallCatalogKey *		trialKey);

EXTERN_API_C( SInt32 )
CompareExtendedCatalogKeys		(LargeCatalogKey *		searchKey,
								 LargeCatalogKey *		trialKey);

EXTERN_API_C( OSErr )
InitCatalogCache				(Ptr *					catalogCacheGlobals);

EXTERN_API_C( void )
InvalidateCatalogCache			(ExtendedVCB *			volume);


/* GenericMRUCache Routines*/
EXTERN_API_C( OSErr )
InitMRUCache					(UInt32 				bufferSize,
								 UInt32 				numCacheBlocks,
								 Ptr *					cachePtr);

EXTERN_API_C( OSErr )
DisposeMRUCache					(Ptr 					cachePtr);

EXTERN_API_C( OSErr )
GetMRUCacheBlock				(UInt32 				key,
								 Ptr 					cachePtr,
								 Ptr *					buffer);

EXTERN_API_C( void )
InvalidateMRUCacheBlock			(Ptr 					cachePtr,
								 Ptr 					buffer);

EXTERN_API_C( void )
InsertMRUCacheBlock				(Ptr 					cachePtr,
								 UInt32 				key,
								 Ptr 					buffer);

/* MountCheck Routines*/
EXTERN_API_C( OSErr )
MountCheck						(ExtendedVCB *			vcb,
								 UInt32 *				consistencyStatus);

/* BTree Manager Routines*/

typedef CALLBACK_API_C( SInt32 , KeyCompareProcPtr )(void *a, void *b);
EXTERN_API_C( OSErr )
OpenBTree						(SInt16 				refNum,
								 KeyCompareProcPtr 		keyCompareProc);

EXTERN_API_C( OSErr )
CloseBTree						(SInt16 				refNum);

EXTERN_API_C( OSErr )
FlushBTree						(SInt16 				refNum);

EXTERN_API_C( OSErr )
SearchBTreeRecord				(SInt16 				refNum,
								 const void *			key,
								 UInt32 				hint,
								 void *					foundKey,
								 void *					data,
								 UInt16 *				dataSize,
								 UInt32 *				newHint);

EXTERN_API_C( OSErr )
GetBTreeRecord					(SInt16 				refNum,
								 SInt16 				selectionIndex,
								 void *					key,
								 void *					data,
								 UInt16 *				dataSize,
								 UInt32 *				newHint);

EXTERN_API_C( OSErr )
InsertBTreeRecord				(SInt16 				refNum,
								 void *					key,
								 void *					data,
								 UInt16 				dataSize,
								 UInt32 *				newHint);

EXTERN_API_C( OSErr )
DeleteBTreeRecord				(SInt16 				refNum,
								 void *					key);

EXTERN_API_C( OSErr )
ReplaceBTreeRecord				(SInt16 				refNum,
								 const void *			key,
								 UInt32 				hint,
								 void *					newData,
								 UInt16 				dataSize,
								 UInt32 *				newHint);

/*	From HFSVolumesInit.c*/
EXTERN_API_C( void )
InitBTreeHeader					(UInt32 				fileSize,
								 UInt32 				clumpSize,
								 UInt16 				nodeSize,
								 UInt16 				recordCount,
								 UInt16 				keySize,
								 UInt32 				attributes,
								 UInt32 *				mapNodes,
								 void *					buffer);

/*	Prototypes for big block cache*/

EXTERN_API_C( OSErr )
InitializeBlockCache			(UInt32 				blockSize,
								 UInt32 				blockCount);

EXTERN_API_C( OSErr )
FlushBlockCache					(void);

EXTERN_API_C( OSErr )
GetCacheBlock					(SInt16 				fileRefNum,
								 UInt32 				blockNumber,
								 UInt32 				blockSize,
								 UInt16 				options,
								 LogicalAddress *		buffer,
								 Boolean *				readFromDisk);

EXTERN_API_C( OSErr )
ReleaseCacheBlock				(LogicalAddress 		buffer,
								 UInt16 				options);

EXTERN_API_C( OSErr )
MarkCacheBlock					(LogicalAddress 		buffer);

EXTERN_API_C( OSErr )
TrashCacheBlocks				(SInt16 				fileRefNum);

/*	Prototypes for C->Asm glue*/
EXTERN_API_C( OSErr )
GetBlock_glue					(UInt16 				flags,
								 UInt32 				nodeNumber,
								 Ptr *					nodeBuffer,
								 UInt16 				refNum,
								 ExtendedVCB *			vcb);

EXTERN_API_C( OSErr )
RelBlock_glue					(Ptr 					nodeBuffer,
								 UInt16 				flags);

EXTERN_API_C( void )
MarkBlock_glue					(Ptr 					nodeBuffer);

EXTERN_API_C( OSErr )
C_FlushCache					(ExtendedVCB *			vcb,
								 UInt32 				flags,
								 UInt32 				refNum);

EXTERN_API_C( OSErr )
CacheReadInPlace				(ExtendedVCB *			volume,
								 HIOParam *				iopb,
								 UInt32 				currentPosition,
								 UInt32 				maximumBytes,
								 UInt32 *				actualBytes);


/*	Prototypes for exported routines in VolumeAllocation.c*/
EXTERN_API_C( OSErr )
BlockAllocate					(ExtendedVCB *			vcb,
								 UInt32 				startingBlock,
								 UInt32 				bytesRequested,
								 UInt32 				bytesMaximum,
								 Boolean 				forceContiguous,
								 UInt32 *				startBlock,
								 UInt32 *				actualBlocks);

EXTERN_API_C( OSErr )
BlockDeallocate					(ExtendedVCB *			vcb,
								 UInt32 				firstBlock,
								 UInt32 				numBlocks);

EXTERN_API_C( SInt32 )
BlockCheck						(ExtendedVCB *			vcb,
								 LargeExtentRecord 		extents);

EXTERN_API_C( OSErr )
UpdateFreeCount					(ExtendedVCB *			vcb);


EXTERN_API_C( OSErr )
AllocateFreeSpace				(ExtendedVCB *			vcb,
								 UInt32 *				startBlock,
								 UInt32 *				actualBlocks);

EXTERN_API_C( UInt32 )
DivideAndRoundUp				(UInt32 				numerator,
								 UInt32 				denominator);

EXTERN_API_C( OSErr )
BlockAllocateAny				(ExtendedVCB *			vcb,
								 UInt32 				startingBlock,
								 UInt32 				endingBlock,
								 UInt32 				maxBlocks,
								 UInt32 *				actualStartBlock,
								 UInt32 *				actualNumBlocks);

EXTERN_API_C( void )
UpdateVCBFreeBlks				(ExtendedVCB *			vcb);

/*	File Extent Mapping routines*/
EXTERN_API_C( OSErr )
FlushExtentFile					(ExtendedVCB *			vcb);

EXTERN_API_C( SInt32 )
CompareExtentKeys				(const SmallExtentKey *	searchKey,
								 const SmallExtentKey *	trialKey);

EXTERN_API_C( SInt32 )
CompareExtentKeysPlus			(const LargeExtentKey *	searchKey,
								 const LargeExtentKey *	trialKey);

EXTERN_API_C( OSErr )
DeallocateFile					(ExtendedVCB *			vcb,
								 CatalogNodeID 			parDirID,
								 ConstStr31Param 		catalogName);

EXTERN_API_C( OSErr )
TruncateFileC					(ExtendedVCB *			vcb,
								 FCB *					fcb,
								 UInt32 				peof,
								 Boolean 				truncateToExtent);

EXTERN_API_C( OSErr )
ExtendFileC						(ExtendedVCB *			vcb,
								 FCB *					fcb,
								 UInt32 				bytesToAdd,
								 UInt32 				flags,
								 UInt32 *				actualBytesAdded);

EXTERN_API_C( OSErr )
MapFileBlockC					(ExtendedVCB *			vcb,
								 FCB *					fcb,
								 UInt32 				numberOfBytes,
								 UInt32 				offset,
								 UInt32 *				startBlock,
								 UInt32 *				availableBytes);

EXTERN_API_C( OSErr )
GrowParallelFCBs				(void);

EXTERN_API_C( void )
AdjustEOF						(FCB *					sourceFCB);

EXTERN_API_C( OSErr )
MapLogicalToPhysical			(HIOParam *				pb);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBMapFilePosition(__A0)
																							#endif
EXTERN_API( OSErr )
PBMapFilePosition				(HIOParam *				pb)									TWOWORDINLINE(0x7051, 0xA260);

/*	Extended Attributes Manager routines*/
EXTERN_API_C( SInt32 )
CompareAttributeKeys			(const void *			searchKey,
								 const void *			trialKey);

EXTERN_API_C( OSErr )
AttributesOpenVolume			(ExtendedVCB *			vcb);

EXTERN_API_C( OSErr )
AttributesCloseVolume			(ExtendedVCB *			vcb);


/*	Utility routines*/

EXTERN_API( OSErr )
HFSCommunicationProc			(short 					message,
								 void *					paramBlock,
								 void *					globalsPtr);

EXTERN_API_C( void )
ClearMemory						(void *					start,
								 UInt32 				length);

EXTERN_API_C( Boolean )
UnicodeBinaryCompare			(ConstUniStr255Param 	ustr1,
								 ConstUniStr255Param 	ustr2);

EXTERN_API_C( Boolean )
PascalBinaryCompare				(ConstStr31Param 		pstr1,
								 ConstStr31Param 		pstr2);

EXTERN_API_C( OSErr )
VolumeWritable					(ExtendedVCB *			vcb);

EXTERN_API_C( void )
Get1stFileControlBlock			(UInt16 *				index,
								 Ptr *					fcbsH);

EXTERN_API_C( Boolean )
GetNextFileControlBlock			(UInt16 *				index,
								 Ptr 					fcbsPtr);


/*€€ Obsolete this ASM call*/
EXTERN_API_C( ExtendedFCB *)
ParallelFCBFromRefnum			(int 					refNum);

/*	Assembly glue routines should use ints instead of shorts to be usable with SC compiler.*/
EXTERN_API_C( ExtendedFCB *)
GetParallelFCB					(short 					fref);

struct FindFileNameGlueRec {
	UInt32 							nameLength;
	char *							nameBuffer;
	ExtendedVCB *					vcb;
	WDCBRec *						wdcb;
	Ptr 							volumeBuffer;
	CatalogNodeData *				data;
	CatalogNodeID 					id;
	UInt32 							hint;
};
typedef struct FindFileNameGlueRec FindFileNameGlueRec;

EXTERN_API_C( OSErr )
FindFileName					(ParamBlockRec *		filePB,
								 FindFileNameGlueRec *	ff);

/*	Get the current time in UTC (GMT)*/
EXTERN_API_C( UInt32 )
GetTimeUTC						(void);

/*	Get the current local time*/
EXTERN_API_C( UInt32 )
GetTimeLocal					(void);

EXTERN_API_C( UInt32 )
LocalToUTC						(UInt32 				localTime);

EXTERN_API_C( UInt32 )
UTCToLocal						(UInt32 				utcTime);

EXTERN_API_C( UInt64 )
U64Add							(UInt64 				x,
								 UInt64 				y);

EXTERN_API_C( UInt64 )
U64Subtract						(UInt64 				left,
								 UInt64 				right);


/*	Volumes routines*/
EXTERN_API_C( OSErr )
FlushVolumeControlBlock			(ExtendedVCB *			vcb);

EXTERN_API_C( OSErr )
CheckVolumeOffLine				(ExtendedVCB *			vcb);

EXTERN_API_C( OSErr )
CloseFile						(ExtendedVCB *			vcb,
								 SInt16 				index,
								 Ptr 					fcbs);

EXTERN_API_C( OSErr )
FlushAlternateVolumeControlBlock (ExtendedVCB *			vcb,
								 Boolean 				isHFSPlus);

EXTERN_API_C( OSErr )
FindDrive						(short *				driverRefNum,
								 DrvQEl **				dqe,
								 short 					driveNumber);

EXTERN_API_C( OSErr )
GetVCBDriveNum					(ExtendedVCB **			vcb,
								 short 					driveNumber);

EXTERN_API_C( OSErr )
ValidVolumeHeader				(VolumeHeader *			volumeHeader);

EXTERN_API_C( void )
FillHFSStack					(void);

EXTERN_API_C( OSErr )
UnMountVolume					(VolumeParam *			volume,
								 WDCBRecPtr *			wdcb);

EXTERN_API_C( OSErr )
MakeVCBsExtendedVCBs			(void);

EXTERN_API_C( OSErr )
FindFileControlBlock			(UInt16 *				index,
								 Ptr *					fcbsH);

EXTERN_API_C( FCB *)
SetupFCB						(ExtendedVCB *			vcb,
								 SInt16 				refNum,
								 UInt32 				fileID,
								 UInt32 				fileClumpSize);

EXTERN_API_C( OSErr )
AccessBTree						(ExtendedVCB *			vcb,
								 SInt16 				refNum,
								 UInt32 				fileID,
								 UInt32 				fileClumpSize,
								 void *					CompareRoutine);

EXTERN_API_C( void )
RemountWrappedVolumes			(void);

EXTERN_API_C( OSErr )
CheckVolumeConsistency			(ExtendedVCB *			vcb);

EXTERN_API_C( void )
HFSBlocksFromTotalSectors		(UInt32 				totalSectors,
								 UInt32 *				blockSize,
								 UInt16 *				blockCount);

/*	CreateEmbeddedVolume.c*/
EXTERN_API_C( OSErr )
CreateEmbeddedVolume			(ParmBlkPtr 			paramBlock);


/*	Private Files SPI*/

/*€€ move following to FilesPriv.i*/



enum {
																/* Private HFSDispatch calls (currently implemented only in AppleShare or FileShare)*/
	selectShare					= 0x42,
	selectUnshare				= 0x43,
	selectGetUGEntry			= 0x44,
	selectServerControl			= 0x45,
	selectServerStartup			= 0x46
};



enum {
																/* fields for _FSControl call:*/
	ioFSVrsn					= 0x20,							/* File system version*/
	ioHQElSize					= 52							/* size of a standard HFS call queue element*/
};


struct SearchPathParam {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	short 							ioRefNum;
	SInt8 							ioPMSPFlg;					/* Flag whether to enable the PMSP*/
	SInt8 							filler1;
	Ptr 							ioPMSPHook;					/* Pointer to PMSP hook proc*/
};
typedef struct SearchPathParam SearchPathParam;

typedef SearchPathParam *				SearchPathParamPtr;
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 FSControl(__A0)
																							#endif
EXTERN_API( OSErr )
FSControl						(ParmBlkPtr 			paramBlock)							TWOWORDINLINE(0x7000, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 MakeEmbeddedVolume(__A0)
																							#endif
EXTERN_API( OSErr )
MakeEmbeddedVolume				(ParmBlkPtr 			paramBlock)							TWOWORDINLINE(0x701C, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 SetPMSP(__A0)
																							#endif
EXTERN_API( OSErr )
SetPMSP							(ParmBlkPtr 			paramBlock)							TWOWORDINLINE(0x700C, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 GetParallelFCBFromRefnum(__A0)
																							#endif
EXTERN_API( OSErr )
GetParallelFCBFromRefnum		(ParmBlkPtr 			paramBlock)							TWOWORDINLINE(0x7050, 0xA260);

struct CreateLargeFileParam {
																/* The first several fields come from HFileInfo. */
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioFRefNum;
	SInt8 							ioFVersNum;
	SInt8 							filler1;
	short 							ioFDirIndex;
	SInt8 							ioFlAttrib;
	SInt8 							ioACUser;
	FInfo 							ioFlFndrInfo;
	long 							ioDirID;

																/* The rest of these fields are specific to the PBCreateLargeFile call. */
	UInt64 							dataLogicalEOF;
	UInt64 							dataPhysicalEOF;
	UInt64 							rsrcLogicalEOF;
	UInt64 							rsrcPhysicalEOF;
};
typedef struct CreateLargeFileParam CreateLargeFileParam;

EXTERN_API_C( OSErr )
CreateLargeFile					(CreateLargeFileParam *	pb);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateLargeFile(__A0)
																							#endif
EXTERN_API( OSErr )
PBCreateLargeFile				(CreateLargeFileParam *	paramBlock)							TWOWORDINLINE(0x7064, 0xA260);

EXTERN_API_C( OSErr )
LongRename						(CMovePBRec *			pb);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBLongRenameSync(__A0)
																							#endif
EXTERN_API( OSErr )
PBLongRenameSync				(CMovePBRec *			paramBlock)							TWOWORDINLINE(0x701E, 0xA260);

struct AttributeParam {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	ProcPtr 						ioCompletion;				/* --> A pointer to a completion routine */
	OSErr 							ioResult;					/* --> The result code of the function */
	StringPtr 						ioNamePtr;					/* --> Pointer to pathname to object */
	short 							ioVRefNum;					/* --> A volume specification */
	long 							filler1;
	long 							filler2;
	Ptr 							ioBuffer;
	unsigned long 					ioReqCount;
	unsigned long 					ioActCount;
	UniStr255 *						ioAttributeName;			/* --> A pointer to the attribute name buffer */
	long 							ioDirID;					/* --> A directory ID */
};
typedef struct AttributeParam AttributeParam;

EXTERN_API_C( OSErr )
CreateAttribute					(AttributeParam *		pb);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateAttributeSync(__A0)
																							#endif
EXTERN_API( OSErr )
PBCreateAttributeSync			(AttributeParam *		paramBlock)							TWOWORDINLINE(0x7065, 0xA260);


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

#endif /* __FILEMGRINTERNAL__ */

