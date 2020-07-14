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
 	File:		MacOSStubs.h
 
 	Contains:	Definitions and routines from MacOS 
 
 	Version:	Rhapsody (Titan)
 
 	DRI:		Scott Roberts
 
 	Copyright:	© 1995-1997 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Scott Roberts
 
 	Bugs:		Report bugs to Radar component 
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __MACOSSTUBS__
#define __MACOSSTUBS__

#if TARGET_OS_RHAPSODY
#include <sys/time.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#endif /* TARGET_OS_RHAPSODY */

#ifndef __MACOSTYPES__
#include <MacOSTypes.h>
#endif


/*
	SizeTDef.h -- Common definitions
	
	size_t - this type is defined by several ANSI headers.
*/
#if ! defined (__size_t__)
	#define __size_t__
        #if defined (__xlc) || defined (__xlC) || defined (__xlC__) || defined (__MWERKS__)
		typedef unsigned long size_t;
        #else	/* __xlC */
		typedef unsigned int size_t;
	#endif	/* __xlC */
#endif	/* __size_t__ */



/*
	StdDef.h -- Common definitions
	
*/

#define offsetof(structure,field) ((size_t)&((structure *) 0)->field)


/************************************************************

	String.h
	String handling

************************************************************/


//int memcmp (const void *s1, const void *s2, size_t n);



/*
 	File:		Math64.h
 
*/

/* 
	Functions to convert between [Unsigned]Wide and [S|U]Int64 types.
	
	These functions are necessary if source code which uses both
	wide and SInt64 is to compile under a compiler that supports
	long long.
*/
#if TYPE_LONGLONG 
	#define SInt64ToWide(x) 		(*((wide*)(&x)))
	#define WideToSInt64(x) 		(*((SInt64*)(&x)))
	#define UInt64ToUnsignedWide(x) (*((UnsignedWide*)(&x)))
	#define UnsignedWideToUInt64(x) (*((UInt64*)(&x)))
#else
	#define SInt64ToWide(x) 		(x)
	#define WideToSInt64(x) 		(x)
	#define UInt64ToUnsignedWide(x) (x)
	#define UnsignedWideToUInt64(x) (x)
#endif



/*
 	File:		Errors.h
 
*/
enum {
	paramErr					= -50,							/*error in user parameter list*/
	noHardwareErr				= -200,							/*Sound Manager Error Returns*/
	notEnoughHardwareErr		= -201,							/*Sound Manager Error Returns*/
	userCanceledErr				= -128,
	qErr						= -1,							/*queue element not found during deletion*/
	vTypErr						= -2,							/*invalid queue element*/
	corErr						= -3,							/*core routine number out of range*/
	unimpErr					= -4,							/*unimplemented core routine*/
	SlpTypeErr					= -5,							/*invalid queue element*/
	seNoDB						= -8,							/*no debugger installed to handle debugger command*/
	controlErr					= -17,							/*I/O System Errors*/
	statusErr					= -18,							/*I/O System Errors*/
	readErr						= -19,							/*I/O System Errors*/
	writErr						= -20,							/*I/O System Errors*/
	badUnitErr					= -21,							/*I/O System Errors*/
	unitEmptyErr				= -22,							/*I/O System Errors*/
	openErr						= -23,							/*I/O System Errors*/
	closErr						= -24,							/*I/O System Errors*/
	dRemovErr					= -25,							/*tried to remove an open driver*/
	dInstErr					= -26							/*DrvrInstall couldn't find driver in resources*/
};

enum {																/* Printing Errors */
	iMemFullErr					= -108,
	iIOAbort					= -27,							/*Scrap Manager errors*/
	noScrapErr					= -100,							/*No scrap exists error*/
	noTypeErr					= -102,							/*No object of that type in scrap*/
	memROZWarn					= -99,							/*soft error in ROZ*/
	memROZError					= -99,							/*hard error in ROZ*/
	memROZErr					= -99,							/*hard error in ROZ*/
	memFullErr					= -108,							/*Not enough room in heap zone*/
	nilHandleErr				= -109,							/*Master Pointer was NIL in HandleZone or other*/
	memWZErr					= -111,							/*WhichZone failed (applied to free block)*/
	memPurErr					= -112,							/*trying to purge a locked or non-purgeable block*/
	memAdrErr					= -110							/*address was odd; or out of range*/
};



enum {
	abortErr					= -27,							/*IO call aborted by KillIO*/
	iIOAbortErr					= -27,							/*IO abort error (Printing Manager)*/
	notOpenErr					= -28,							/*Couldn't rd/wr/ctl/sts cause driver not opened*/
	unitTblFullErr				= -29,							/*unit table has no more entries*/
	dceExtErr					= -30,							/*dce extension error*/
	slotNumErr					= -360,							/*invalid slot # error*/
	gcrOnMFMErr					= -400,							/*gcr format on high density media error*/
	dirFulErr					= -33,							/*Directory full*/
	dskFulErr					= -34,							/*disk full*/
	nsvErr						= -35,							/*no such volume*/
	ioErr						= -36,							/*I/O error (bummers)*/
	bdNamErr					= -37,							/*there may be no bad names in the final system!*/
	fnOpnErr					= -38,							/*File not open*/
	eofErr						= -39,							/*End of file*/
	posErr						= -40,							/*tried to position to before start of file (r/w)*/
	mFulErr						= -41,							/*memory full (open) or file won't fit (load)*/
	tmfoErr						= -42,							/*too many files open*/
	fnfErr						= -43,							/*File not found*/
	wPrErr						= -44,							/*diskette is write protected.*/
	fLckdErr					= -45							/*file is locked*/
};


enum {
	vLckdErr					= -46,							/*volume is locked*/
	fBsyErr						= -47,							/*File is busy (delete)*/
	dupFNErr					= -48,							/*duplicate filename (rename)*/
	opWrErr						= -49,							/*file already open with with write permission*/
	rfNumErr					= -51,							/*refnum error*/
	gfpErr						= -52,							/*get file position error*/
	volOffLinErr				= -53,							/*volume not on line error (was Ejected)*/
	permErr						= -54,							/*permissions error (on file open)*/
	volOnLinErr					= -55,							/*drive volume already on-line at MountVol*/
	nsDrvErr					= -56,							/*no such drive (tried to mount a bad drive num)*/
	noMacDskErr					= -57,							/*not a mac diskette (sig bytes are wrong)*/
	extFSErr					= -58,							/*volume in question belongs to an external fs*/
	fsRnErr						= -59,							/*file system internal error:during rename the old entry was deleted but could not be restored.*/
	badMDBErr					= -60,							/*bad master directory block*/
	wrPermErr					= -61,							/*write permissions error*/
	dirNFErr					= -120,							/*Directory not found*/
	tmwdoErr					= -121,							/*No free WDCB available*/
	badMovErr					= -122,							/*Move into offspring error*/
	wrgVolTypErr				= -123,							/*Wrong volume type error [operation not supported for MFS]*/
	volGoneErr					= -124							/*Server volume has been disconnected.*/
};

enum {
																/*Dictionary Manager errors*/
	notBTree					= -410,							/*The file is not a dictionary.*/
	btNoSpace					= -413,							/*Can't allocate disk space.*/
	btDupRecErr					= -414,							/*Record already exists.*/
	btRecNotFnd					= -415,							/*Record cannot be found.*/
	btKeyLenErr					= -416,							/*Maximum key length is too long or equal to zero.*/
	btKeyAttrErr				= -417,							/*There is no such a key attribute.*/
	unknownInsertModeErr		= -20000,						/*There is no such an insert mode.*/
	recordDataTooBigErr			= -20001,						/*The record data is bigger than buffer size (1024 bytes).*/
	invalidIndexErr				= -20002						/*The recordIndex parameter is not valid.*/
};


enum {
	fidNotFound					= -1300,						/*no file thread exists.*/
	fidExists					= -1301,						/*file id already exists*/
	notAFileErr					= -1302,						/*directory specified*/
	diffVolErr					= -1303,						/*files on different volumes*/
	catChangedErr				= -1304,						/*the catalog has been modified*/
	desktopDamagedErr			= -1305,						/*desktop database files are corrupted*/
	sameFileErr					= -1306,						/*can't exchange a file with itself*/
	badFidErr					= -1307,						/*file id is dangling or doesn't match with the file number*/
	notARemountErr				= -1308,						/*when _Mount allows only remounts and doesn't get one*/
	fileBoundsErr				= -1309,						/*file's EOF, offset, mark or size is too big*/
	fsDataTooBigErr				= -1310,						/*file or volume is too big for system*/
	volVMBusyErr				= -1311,						/*can't eject because volume is in use by VM*/
	envNotPresent				= -5500,						/*returned by glue.*/
	envBadVers					= -5501,						/*Version non-positive*/
	envVersTooBig				= -5502,						/*Version bigger than call can handle*/
	fontDecError				= -64,							/*error during font declaration*/
	fontNotDeclared				= -65,							/*font not declared*/
	fontSubErr					= -66,							/*font substitution occured*/
	fontNotOutlineErr			= -32615,						/*bitmap font passed to routine that does outlines only*/
	firstDskErr					= -84,							/*I/O System Errors*/
	lastDskErr					= -64,							/*I/O System Errors*/
	noDriveErr					= -64,							/*drive not installed*/
	offLinErr					= -65,							/*r/w requested for an off-line drive*/
	noNybErr					= -66							/*couldn't find 5 nybbles in 200 tries*/
};

enum {
																/* general text errors*/
	kTextUnsupportedEncodingErr	= -8738,						/* specified encoding not supported for this operation*/
	kTextMalformedInputErr		= -8739,						/* in DBCS, for example, high byte followed by invalid low byte*/
	kTextUndefinedElementErr	= -8740,						/* text conversion errors*/
	kTECMissingTableErr			= -8745,
	kTECTableChecksumErr		= -8746,
	kTECTableFormatErr			= -8747,
	kTECCorruptConverterErr		= -8748,						/* invalid converter object reference*/
	kTECNoConversionPathErr		= -8749,
	kTECBufferBelowMinimumSizeErr = -8750,						/* output buffer too small to allow processing of first input text element*/
	kTECArrayFullErr			= -8751,						/* supplied name buffer or TextRun, TextEncoding, or UnicodeMapping array is too small*/
	kTECBadTextRunErr			= -8752,
	kTECPartialCharErr			= -8753,						/* input buffer ends in the middle of a multibyte character, conversion stopped*/
	kTECUnmappableElementErr	= -8754,
	kTECIncompleteElementErr	= -8755,						/* text element may be incomplete or is too long for internal buffers*/
	kTECDirectionErr			= -8756,						/* direction stack overflow, etc.*/
	kTECGlobalsUnavailableErr	= -8770,						/* globals have already been deallocated (premature TERM)*/
	kTECItemUnavailableErr		= -8771,						/* item (e.g. name) not available for specified region (& encoding if relevant)*/
																/* text conversion status codes*/
	kTECUsedFallbacksStatus		= -8783,
	kTECNeedFlushStatus			= -8784,
	kTECOutputBufferFullStatus	= -8785,						/* output buffer has no room for conversion of next input text element (partial conversion)*/
																/* deprecated error & status codes for low-level converter*/
	unicodeChecksumErr			= -8769,
	unicodeNoTableErr			= -8768,
	unicodeVariantErr			= -8767,
	unicodeFallbacksErr			= -8766,
	unicodePartConvertErr		= -8765,
	unicodeBufErr				= -8764,
	unicodeCharErr				= -8763,
	unicodeElementErr			= -8762,
	unicodeNotFoundErr			= -8761,
	unicodeTableFormatErr		= -8760,
	unicodeDirectionErr			= -8759,
	unicodeContextualErr		= -8758,
	unicodeTextEncodingDataErr	= -8757
};


/*
 	File:		MacMemory.h
 
 
*/


/*
 	File:		MixedMode.h
 
*/

/* Calling Conventions */
typedef unsigned short 					CallingConventionType;

enum {
	kPascalStackBased			= 0,
	kCStackBased				= 1,
	kRegisterBased				= 2,
	kD0DispatchedPascalStackBased = 8,
	kD1DispatchedPascalStackBased = 12,
	kD0DispatchedCStackBased	= 9,
	kStackDispatchedPascalStackBased = 14,
	kThinkCStackBased			= 5
};

#if 0
/* SizeCodes we use everywhere */

enum {
	kNoByteCode					= 0,
	kOneByteCode				= 1,
	kTwoByteCode				= 2,
	kFourByteCode				= 3
};

/* Mixed Mode Routine Records */
typedef unsigned long 					ProcInfoType;
/* Routine Flag Bits */
typedef unsigned short 					RoutineFlagsType;

/* * * * * * * * * * * * * * 
 *	SIZE_CODE - 	Return the size code for an object, given its size in bytes.
 *		size - size of an object in bytes
 */
#define SIZE_CODE(size) \
	(((size) == 4) ? kFourByteCode : (((size) == 2) ? kTwoByteCode : (((size) == 1) ? kOneByteCode : 0)))


/* * * * * * * * * * * * * * 
 *	RESULT_SIZE - 	Return the result field of a ProcInfo, given the return object¹s size.
 * 					This is the same for all ProcInfos
 *		sizeCode - size code
 */
#define RESULT_SIZE(sizeCode) \
	((ProcInfoType)(sizeCode) << kResultSizePhase)



/* * * * * * * * * * * * * * 
 *	STACK_ROUTINE_PARAMETER -	Return a parameter field of a ProcInfo, for a simple,
 *								non-dispatched, stack based routine.
 *		whichParam - which parameter
 *		sizeCode - size code
 */
#define STACK_ROUTINE_PARAMETER(whichParam, sizeCode) \
	((ProcInfoType)(sizeCode) << (kStackParameterPhase + (((whichParam) - 1) * kStackParameterWidth)))

#endif


	#define STACK_UPP_TYPE(name) 	name
	#define REGISTER_UPP_TYPE(name) name


/*
 	File:		OSUtils.h
 
*/
typedef struct QElem QElem;

typedef QElem *							QElemPtr;
struct QHdr {
	short 							qFlags;
	QElemPtr 						qHead;
	QElemPtr 						qTail;
};
typedef struct QHdr QHdr;

typedef QHdr *							QHdrPtr;

typedef CALLBACK_API( void , DeferredTaskProcPtr )(long dtParam);
/*
	WARNING: DeferredTaskProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
typedef REGISTER_UPP_TYPE(DeferredTaskProcPtr) 					DeferredTaskUPP;
enum { uppDeferredTaskProcInfo = 0x0000B802 }; 					/* register no_return_value Func(4_bytes:A1) */
#define NewDeferredTaskProc(userRoutine) 						(DeferredTaskUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppDeferredTaskProcInfo, GetCurrentArchitecture())
#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	#pragma parameter CallDeferredTaskProc(__A0, __A1)
	void CallDeferredTaskProc(DeferredTaskUPP routine, long dtParam) = 0x4E90;
#else
	#define CallDeferredTaskProc(userRoutine, dtParam) 			CALL_ONE_PARAMETER_UPP((userRoutine), uppDeferredTaskProcInfo, (dtParam))
#endif
struct DeferredTask {
	QElemPtr 						qLink;
	short 							qType;
	short 							dtFlags;
	DeferredTaskUPP 				dtAddr;
	long 							dtParam;
	long 							dtReserved;
};
typedef struct DeferredTask DeferredTask;

typedef DeferredTask *					DeferredTaskPtr;

/*
 	File:		Finder.h
 
 
*/

/*	
	The following declerations used to be in Files.‰, 
	but are Finder specific and were moved here.
*/

enum {
                                                                /* Finder Flags */
    kIsOnDesk					= 0x0001,
    kColor						= 0x000E,
    kIsShared					= 0x0040,						/* bit 0x0080 is hasNoINITS */
    kHasBeenInited				= 0x0100,						/* bit 0x0200 was the letter bit for AOCE, but is now reserved for future use */
    kHasCustomIcon				= 0x0400,
    kIsStationery				= 0x0800,
    kNameLocked					= 0x1000,
    kHasBundle					= 0x2000,
    kIsInvisible				= 0x4000,
    kIsAlias					= 0x8000
};


enum {
																/* Finder Constants */
	fOnDesk						= 1,
	fHasBundle					= 8192,
	fTrash						= -3,
	fDesktop					= -2,
	fDisk						= 0
};

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif


struct FInfo {
	OSType 							fdType;						/*the type of the file*/
	OSType 							fdCreator;					/*file's creator*/
	unsigned short 					fdFlags;					/*flags ex. hasbundle,invisible,locked, etc.*/
	Point 							fdLocation;					/*file's location in folder*/
	short 							fdFldr;						/*folder containing file*/
};
typedef struct FInfo FInfo;

struct FXInfo {
	short 							fdIconID;					/*Icon ID*/
	short 							fdUnused[3];				/*unused but reserved 6 bytes*/
	SInt8 							fdScript;					/*Script flag and number*/
	SInt8 							fdXFlags;					/*More flag bits*/
	short 							fdComment;					/*Comment ID*/
	long 							fdPutAway;					/*Home Dir ID*/
};
typedef struct FXInfo FXInfo;

struct DInfo {
	Rect 							frRect;						/*folder rect*/
	unsigned short 					frFlags;					/*Flags*/
	Point 							frLocation;					/*folder location*/
	short 							frView;						/*folder view*/
};
typedef struct DInfo DInfo;

struct DXInfo {
	Point 							frScroll;					/*scroll position*/
	long 							frOpenChain;				/*DirID chain of open folders*/
	SInt8 							frScript;					/*Script flag and number*/
	SInt8 							frXFlags;					/*More flag bits*/
	short 							frComment;					/*comment*/
	long 							frPutAway;					/*DirID*/
};
typedef struct DXInfo DXInfo;

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

/*
 	File:		Files.h
 
 
*/


enum {
	fsAtMark					= 0,
	fsCurPerm					= 0,
	fsRdPerm					= 1,
	fInvisible					= 16384,
	fsWrPerm					= 2,
	fsRdWrPerm					= 3,
	fsRdWrShPerm				= 4,
	fsFromStart					= 1,
	fsFromLEOF					= 2,
	fsFromMark					= 3,
	rdVerify					= 64,
	ioMapBuffer					= 4,
	ioModeReserved				= 8,
	ioDirFlg					= 4,							/* see IM IV-125 */
	ioDirMask					= 0x10,
	fsRtParID					= 1,
	fsRtDirID					= 2
};



enum {
																/* CatSearch SearchBits Constants */
	fsSBPartialName				= 1,
	fsSBFullName				= 2,
	fsSBFlAttrib				= 4,
	fsSBFlFndrInfo				= 8,
	fsSBFlLgLen					= 32,
	fsSBFlPyLen					= 64,
	fsSBFlRLgLen				= 128,
	fsSBFlRPyLen				= 256,
	fsSBFlCrDat					= 512,
	fsSBFlMdDat					= 1024,
	fsSBFlBkDat					= 2048,
	fsSBFlXFndrInfo				= 4096,
	fsSBFlParID					= 8192,
	fsSBNegate					= 16384,
	fsSBDrUsrWds				= 8,
	fsSBDrNmFls					= 16,
	fsSBDrCrDat					= 512,
	fsSBDrMdDat					= 1024,
	fsSBDrBkDat					= 2048,
	fsSBDrFndrInfo				= 4096,							/* bit values for the above */
	fsSBPartialNameBit			= 0,							/*ioFileName points to a substring*/
	fsSBFullNameBit				= 1,							/*ioFileName points to a match string*/
	fsSBFlAttribBit				= 2,							/*search includes file attributes*/
	fsSBFlFndrInfoBit			= 3,							/*search includes finder info*/
	fsSBFlLgLenBit				= 5,							/*search includes data logical length*/
	fsSBFlPyLenBit				= 6,							/*search includes data physical length*/
	fsSBFlRLgLenBit				= 7,							/*search includes resource logical length*/
	fsSBFlRPyLenBit				= 8,							/*search includes resource physical length*/
	fsSBFlCrDatBit				= 9,							/*search includes create date*/
	fsSBFlMdDatBit				= 10,							/*search includes modification date*/
	fsSBFlBkDatBit				= 11,							/*search includes backup date*/
	fsSBFlXFndrInfoBit			= 12,							/*search includes extended finder info*/
	fsSBFlParIDBit				= 13,							/*search includes file's parent ID*/
	fsSBNegateBit				= 14,							/*return all non-matches*/
	fsSBDrUsrWdsBit				= 3,							/*search includes directory finder info*/
	fsSBDrNmFlsBit				= 4,							/*search includes directory valence*/
	fsSBDrCrDatBit				= 9,							/*directory-named version of fsSBFlCrDatBit*/
	fsSBDrMdDatBit				= 10,							/*directory-named version of fsSBFlMdDatBit*/
	fsSBDrBkDatBit				= 11,							/*directory-named version of fsSBFlBkDatBit*/
	fsSBDrFndrInfoBit			= 12							/*directory-named version of fsSBFlXFndrInfoBit*/
};


enum {
	fsSBDrParID					= 8192,
	fsSBDrParIDBit				= 13,							/*directory-named version of fsSBFlParIDBit*/
																/* vMAttrib (GetVolParms) bit position constants */
	bLimitFCBs					= 31,
	bLocalWList					= 30,
	bNoMiniFndr					= 29,
	bNoVNEdit					= 28,
	bNoLclSync					= 27,
	bTrshOffLine				= 26,
	bNoSwitchTo					= 25,
	bNoDeskItems				= 20,
	bNoBootBlks					= 19,
	bAccessCntl					= 18,
	bNoSysDir					= 17,
	bHasExtFSVol				= 16,
	bHasOpenDeny				= 15,
	bHasCopyFile				= 14,
	bHasMoveRename				= 13,
	bHasDesktopMgr				= 12,
	bHasShortName				= 11,
	bHasFolderLock				= 10,
	bHasPersonalAccessPrivileges = 9
};


enum {
	bHasUserGroupList			= 8,
	bHasCatSearch				= 7,
	bHasFileIDs					= 6,
	bHasBTreeMgr				= 5,
	bHasBlankAccessPrivileges	= 4,
	bSupportsAsyncRequests		= 3								/* asynchronous requests to this volume are handled correctly at any time*/
};


enum {
																/* Desktop Database icon Constants */
	kLargeIcon					= 1,
	kLarge4BitIcon				= 2,
	kLarge8BitIcon				= 3,
	kSmallIcon					= 4,
	kSmall4BitIcon				= 5,
	kSmall8BitIcon				= 6,
	kLargeIconSize				= 256,
	kLarge4BitIconSize			= 512,
	kLarge8BitIconSize			= 1024,
	kSmallIconSize				= 64,
	kSmall4BitIconSize			= 128,
	kSmall8BitIconSize			= 256,							/* Foreign Privilege Model Identifiers */
	fsUnixPriv					= 1,							/* Authentication Constants */
	kNoUserAuthentication		= 1,
	kPassword					= 2,
	kEncryptPassword			= 3,
	kTwoWayEncryptPassword		= 6
};


/* mapping codes (ioObjType) for MapName & MapID */

enum {
	kOwnerID2Name				= 1,
	kGroupID2Name				= 2,
	kOwnerName2ID				= 3,
	kGroupName2ID				= 4,							/* types of oj object to be returned (ioObjType) for _GetUGEntry */
	kReturnNextUser				= 1,
	kReturnNextGroup			= 2,
	kReturnNextUG				= 3
};



/* Folder and File values of access privileges */

enum {
	kfullPrivileges				= 0x00070007,					/*			; all privileges for everybody and owner*/
	kownerPrivileges			= 0x00000007					/*			; all privileges for owner only*/
};

/* values of user IDs and group IDs */

enum {
	knoUser						= 0,
	kadministratorUser			= 1
};


enum {
	knoGroup					= 0
};
/* Catalog position record */
struct CatPositionRec {
	long 							initialize;
	short 							priv[6];
};
typedef struct CatPositionRec CatPositionRec;

struct FSSpec {
	short 							vRefNum;
	long 							parID;
	Str63 							name;
};
typedef struct FSSpec FSSpec;

typedef FSSpec *						FSSpecPtr;
typedef FSSpecPtr *						FSSpecHandle;
/* pointer to array of FSSpecs */
typedef FSSpecPtr 						FSSpecArrayPtr;
/* 
	The only difference between "const FSSpec*" and "ConstFSSpecPtr" is 
	that as a parameter, ConstFSSpecPtr is allowed to be NULL 
*/
typedef const FSSpec *					ConstFSSpecPtr;

/* 
	The following are structures to be filled out with the _GetVolMountInfo call
	and passed back into the _VolumeMount call for external file system mounts. 
*/
/* the "signature" of the file system */
typedef OSType 							VolumeType;

enum {
																/* the signature for AppleShare */
	AppleShareMediaType			= FOUR_CHAR_CODE('afpm')
};

/*
	VolMount stuff was once in FSM.‰
*/
struct VolMountInfoHeader {
	short 							length;						/* length of location data (including self) */
	VolumeType 						media;						/* type of media.  Variable length data follows */
};
typedef struct VolMountInfoHeader VolMountInfoHeader;

typedef VolMountInfoHeader *			VolMountInfoPtr;
/* The new volume mount info record.  The old one is included for compatibility. 
	the new record allows access by foriegn filesystems writers to the flags 
	portion of the record. This portion is now public.  
*/
struct VolumeMountInfoHeader {
	short 							length;						/* length of location data (including self) */
	VolumeType 						media;						/* type of media (must be registered with Apple) */
	short 							flags;						/* volume mount flags. Variable length data follows */
};
typedef struct VolumeMountInfoHeader VolumeMountInfoHeader;

typedef VolumeMountInfoHeader *			VolumeMountInfoHeaderPtr;
/*	additional volume mount flags */

enum {
	volMountInteractBit			= 15,							/* Input to VolumeMount: If set, it's OK for the file system */
	volMountInteractMask		= 0x8000,						/* to perform user interaction to mount the volume */
	volMountChangedBit			= 14,							/* Output from VoumeMount: If set, the volume was mounted, but */
	volMountChangedMask			= 0x4000,						/* the volume mounting information record needs to be updated. */
	volMountFSReservedMask		= 0x00FF,						/* bits 0-7 are defined by each file system for its own use */
	volMountSysReservedMask		= 0xFF00						/* bits 8-15 are reserved for Apple system use */
};

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

typedef union ParamBlockRec 			ParamBlockRec;
typedef ParamBlockRec *					ParmBlkPtr;
typedef CALLBACK_API( void , IOCompletionProcPtr )(ParmBlkPtr paramBlock);
/*
	WARNING: IOCompletionProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
typedef REGISTER_UPP_TYPE(IOCompletionProcPtr) 					IOCompletionUPP;
struct IOParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioRefNum;					/*refNum for I/O operation*/
	SInt8 							ioVersNum;					/*version number*/
	SInt8 							ioPermssn;					/*Open: permissions (byte)*/
	Ptr 							ioMisc;						/*Rename: new name (GetEOF,SetEOF: logical end of file) (Open: optional ptr to buffer) (SetFileType: new type)*/
	Ptr 							ioBuffer;					/*data buffer Ptr*/
	long 							ioReqCount;					/*requested byte count; also = ioNewDirID*/
	long 							ioActCount;					/*actual byte count completed*/
	short 							ioPosMode;					/*initial file positioning*/
	long 							ioPosOffset;				/*file position offset*/
};
typedef struct IOParam IOParam;

typedef IOParam *						IOParamPtr;
struct FileParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioFRefNum;					/*reference number*/
	SInt8 							ioFVersNum;					/*version number*/
	SInt8 							filler1;
	short 							ioFDirIndex;				/*GetFInfo directory index*/
	SInt8 							ioFlAttrib;					/*GetFInfo: in-use bit=7, lock bit=0*/
	SInt8 							ioFlVersNum;				/*file version number*/
	FInfo 							ioFlFndrInfo;				/*user info*/
	unsigned long 					ioFlNum;					/*GetFInfo: file number; TF- ioDirID*/
	unsigned short 					ioFlStBlk;					/*start file block (0 if none)*/
	long 							ioFlLgLen;					/*logical length (EOF)*/
	long 							ioFlPyLen;					/*physical length*/
	unsigned short 					ioFlRStBlk;					/*start block rsrc fork*/
	long 							ioFlRLgLen;					/*file logical length rsrc fork*/
	long 							ioFlRPyLen;					/*file physical length rsrc fork*/
	unsigned long 					ioFlCrDat;					/*file creation date& time (32 bits in secs)*/
	unsigned long 					ioFlMdDat;					/*last modified date and time*/
};
typedef struct FileParam FileParam;

typedef FileParam *						FileParamPtr;
struct VolumeParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	long 							filler2;
	short 							ioVolIndex;					/*volume index number*/
	unsigned long 					ioVCrDate;					/*creation date and time*/
	unsigned long 					ioVLsBkUp;					/*last backup date and time*/
	unsigned short 					ioVAtrb;					/*volume attrib*/
	unsigned short 					ioVNmFls;					/*number of files in directory*/
	unsigned short 					ioVDirSt;					/*start block of file directory*/
	short 							ioVBlLn;					/*GetVolInfo: length of dir in blocks*/
	unsigned short 					ioVNmAlBlks;				/*for compatibilty ioVNmAlBlks * ioVAlBlkSiz <= 2 GB*/
	unsigned long 					ioVAlBlkSiz;				/*for compatibilty ioVAlBlkSiz is <= $0000FE00 (65,024)*/
	unsigned long 					ioVClpSiz;					/*GetVolInfo: bytes to allocate at a time*/
	unsigned short 					ioAlBlSt;					/*starting disk(512-byte) block in block map*/
	unsigned long 					ioVNxtFNum;					/*GetVolInfo: next free file number*/
	unsigned short 					ioVFrBlk;					/*GetVolInfo: # free alloc blks for this vol*/
};
typedef struct VolumeParam VolumeParam;

typedef VolumeParam *					VolumeParamPtr;
struct CntrlParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioCRefNum;					/*refNum for I/O operation*/
	short 							csCode;						/*word for control status code*/
	short 							csParam[11];				/*operation-defined parameters*/
};
typedef struct CntrlParam CntrlParam;

typedef CntrlParam *					CntrlParamPtr;

union ParamBlockRec {
	IOParam 						ioParam;
	FileParam 						fileParam;
	VolumeParam 					volumeParam;
	CntrlParam 						cntrlParam;
};


struct HFileInfo {
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
	unsigned short 					ioFlStBlk;
	long 							ioFlLgLen;
	long 							ioFlPyLen;
	unsigned short 					ioFlRStBlk;
	long 							ioFlRLgLen;
	long 							ioFlRPyLen;
	unsigned long 					ioFlCrDat;
	unsigned long 					ioFlMdDat;
	unsigned long 					ioFlBkDat;
	FXInfo 							ioFlXFndrInfo;
	long 							ioFlParID;
	long 							ioFlClpSiz;
};
typedef struct HFileInfo HFileInfo;

struct DirInfo {
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
	DInfo 							ioDrUsrWds;
	long 							ioDrDirID;
	unsigned short 					ioDrNmFls;
	short 							filler3[9];
	unsigned long 					ioDrCrDat;
	unsigned long 					ioDrMdDat;
	unsigned long 					ioDrBkDat;
	DXInfo 							ioDrFndrInfo;
	long 							ioDrParID;
};
typedef struct DirInfo DirInfo;

union CInfoPBRec {
	HFileInfo 						hFileInfo;
	DirInfo 						dirInfo;
};
typedef union CInfoPBRec CInfoPBRec;

typedef CInfoPBRec *					CInfoPBPtr;

struct HIOParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioRefNum;
	SInt8 							ioVersNum;
	SInt8 							ioPermssn;
	Ptr 							ioMisc;
	Ptr 							ioBuffer;
	long 							ioReqCount;
	long 							ioActCount;
	short 							ioPosMode;
	long 							ioPosOffset;
};
typedef struct HIOParam HIOParam;

typedef HIOParam *						HIOParamPtr;
struct CMovePBRec {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	long 							filler1;
	StringPtr 						ioNewName;
	long 							filler2;
	long 							ioNewDirID;
	long 							filler3[2];
	long 							ioDirID;
};
typedef struct CMovePBRec CMovePBRec;

typedef CMovePBRec *					CMovePBPtr;
struct FIDParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	long 							filler14;
	StringPtr 						ioDestNamePtr;				/* dest file name */
	long 							filler15;
	long 							ioDestDirID;				/* dest file's directory id */
	long 							filler16;
	long 							filler17;
	long 							ioSrcDirID;					/* source file's directory id */
	short 							filler18;
	long 							ioFileID;					/* file ID */
};
typedef struct FIDParam FIDParam;

typedef FIDParam *						FIDParamPtr;
struct CSParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	FSSpecPtr 						ioMatchPtr;					/* match array */
	long 							ioReqMatchCount;			/* maximum allowable matches */
	long 							ioActMatchCount;			/* actual match count */
	long 							ioSearchBits;				/* search criteria selector */
	CInfoPBPtr 						ioSearchInfo1;				/* search values and range lower bounds */
	CInfoPBPtr 						ioSearchInfo2;				/* search values and range upper bounds */
	long 							ioSearchTime;				/* length of time to run search */
	CatPositionRec 					ioCatPosition;				/* current position in the catalog */
	Ptr 							ioOptBuffer;				/* optional performance enhancement buffer */
	long 							ioOptBufSize;				/* size of buffer pointed to by ioOptBuffer */
};
typedef struct CSParam CSParam;

typedef CSParam *						CSParamPtr;
struct HVolumeParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	long 							filler2;
	short 							ioVolIndex;
	unsigned long 					ioVCrDate;
	unsigned long 					ioVLsMod;
	short 							ioVAtrb;
	unsigned short 					ioVNmFls;
	unsigned short 					ioVBitMap;
	unsigned short 					ioAllocPtr;
	unsigned short 					ioVNmAlBlks;
	unsigned long 					ioVAlBlkSiz;
	unsigned long 					ioVClpSiz;
	unsigned short 					ioAlBlSt;
	unsigned long 					ioVNxtCNID;
	unsigned short 					ioVFrBlk;
	unsigned short 					ioVSigWord;
	short 							ioVDrvInfo;
	short 							ioVDRefNum;
	short 							ioVFSID;
	unsigned long 					ioVBkUp;
	short 							ioVSeqNum;
	unsigned long 					ioVWrCnt;
	unsigned long 					ioVFilCnt;
	unsigned long 					ioVDirCnt;
	long 							ioVFndrInfo[8];
};
typedef struct HVolumeParam HVolumeParam;

typedef HVolumeParam *					HVolumeParamPtr;

enum {
																/* Large Volume Constants */
	kWidePosOffsetBit			= 8,
	kMaximumBlocksIn4GB			= 0x007FFFFF
};

struct XIOParam {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	short 							ioRefNum;
	SInt8 							ioVersNum;
	SInt8 							ioPermssn;
	Ptr 							ioMisc;
	Ptr 							ioBuffer;
	long 							ioReqCount;
	long 							ioActCount;
	short 							ioPosMode;					/* must have kUseWidePositioning bit set */
	wide 							ioWPosOffset;				/* wide positioning offset */
};
typedef struct XIOParam XIOParam;

typedef XIOParam *						XIOParamPtr;
struct XVolumeParam {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	unsigned long 					ioXVersion;					/* this XVolumeParam version (0) */
	short 							ioVolIndex;
	unsigned long 					ioVCrDate;
	unsigned long 					ioVLsMod;
	short 							ioVAtrb;
	unsigned short 					ioVNmFls;
	unsigned short 					ioVBitMap;
	unsigned short 					ioAllocPtr;
	unsigned short 					ioVNmAlBlks;
	unsigned long 					ioVAlBlkSiz;
	unsigned long 					ioVClpSiz;
	unsigned short 					ioAlBlSt;
	unsigned long 					ioVNxtCNID;
	unsigned short 					ioVFrBlk;
	unsigned short 					ioVSigWord;
	short 							ioVDrvInfo;
	short 							ioVDRefNum;
	short 							ioVFSID;
	unsigned long 					ioVBkUp;
	short 							ioVSeqNum;
	unsigned long 					ioVWrCnt;
	unsigned long 					ioVFilCnt;
	unsigned long 					ioVDirCnt;
	long 							ioVFndrInfo[8];
	UnsignedWide 					ioVTotalBytes;				/* total number of bytes on volume */
	UnsignedWide 					ioVFreeBytes;				/* number of free bytes on volume */
};
typedef struct XVolumeParam XVolumeParam;

typedef XVolumeParam *					XVolumeParamPtr;

struct VCB {
	QElemPtr 						qLink;
	short 							qType;
	short 							vcbFlags;
	unsigned short 					vcbSigWord;
	unsigned long 					vcbCrDate;
	unsigned long 					vcbLsMod;
	short 							vcbAtrb;
	unsigned short 					vcbNmFls;
	short 							vcbVBMSt;
	short 							vcbAllocPtr;
	unsigned short 					vcbNmAlBlks;
	long 							vcbAlBlkSiz;
	long 							vcbClpSiz;
	short 							vcbAlBlSt;
	long 							vcbNxtCNID;
	unsigned short 					vcbFreeBks;
	Str27 							vcbVN;
	short 							vcbDrvNum;
	short 							vcbDRefNum;
	short 							vcbFSID;
	short 							vcbVRefNum;
	Ptr 							vcbMAdr;
	Ptr 							vcbBufAdr;
	short 							vcbMLen;
	short 							vcbDirIndex;
	short 							vcbDirBlk;
	unsigned long 					vcbVolBkUp;
	unsigned short 					vcbVSeqNum;
	long 							vcbWrCnt;
	long 							vcbXTClpSiz;
	long 							vcbCTClpSiz;
	unsigned short 					vcbNmRtDirs;
	long 							vcbFilCnt;
	long 							vcbDirCnt;
	long 							vcbFndrInfo[8];
	unsigned short 					vcbVCSize;
	unsigned short 					vcbVBMCSiz;
	unsigned short 					vcbCtlCSiz;
	unsigned short 					vcbXTAlBlks;
	unsigned short 					vcbCTAlBlks;
	short 							vcbXTRef;
	short 							vcbCTRef;
	Ptr 							vcbCtlBuf;
	long 							vcbDirIDM;
	short 							vcbOffsM;
};
typedef struct VCB VCB;

typedef VCB *							VCBPtr;
struct DrvQEl {
	QElemPtr 						qLink;
	short 							qType;
	short 							dQDrive;
	short 							dQRefNum;
	short 							dQFSID;
	unsigned short 					dQDrvSz;
	unsigned short 					dQDrvSz2;
};
typedef struct DrvQEl DrvQEl;

typedef DrvQEl *						DrvQElPtr;

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

/*
 	File:		FSM.h
 
 
*/

/*
 * Miscellaneous file system values not in Files.‰
 */

enum {
	fsUsrCNID					= 16,							/* First assignable directory or file number */
																/*	File system trap word attribute bits */
	kHFSBit						= 9,							/* HFS call: bit 9 */
	kHFSMask					= 0x0200,
	kAsyncBit					= 10,							/* Asynchronous call: bit 10 */
	kAsyncMask					= 0x0400
};



/*
 * UTCacheReadIP and UTCacheWriteIP cacheOption
 */

enum {
	noCacheBit					= 5,							/* don't cache this please */
	noCacheMask					= 0x0020,
	rdVerifyBit					= 6,							/* read verify */
	rdVerifyMask				= 0x0040
};

/*
 * Cache routine internal error codes
 */

enum {
	chNoBuf						= 1,							/* no free cache buffers (all in use) */
	chInUse						= 2,							/* requested block in use */
	chnotfound					= 3,							/* requested block not found */
	chNotInUse					= 4								/* block being released was not in use */
};


/*
 * UTGetBlock options
 */

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


/*
 * UTReleaseBlock options
 */

enum {
	rbDefault					= 0,							/* default value - just mark the buffer not in-use */
																/*	bits and masks */
	rbWriteBit					= 0,							/* force write buffer to disk */
	rbWriteMask					= 0x0001,
	rbTrashBit					= 1,							/* trash buffer contents after release */
	rbTrashMask					= 0x0002,
	rbDirtyBit					= 2,							/* mark buffer dirty */
	rbDirtyMask					= 0x0004,
	rbFreeBit					= 3,							/* free the buffer (save in the hash) */
	rbFreeMask					= 0x000A						/* rbFreeMask (rbFreeBit + rbTrashBit) works as rbTrash on < System 7.0 RamCache; on >= System 7.0, rbfreeMask overrides rbTrash */
};

/*
 * UTFlushCache options
 */

enum {
	fcDefault					= 0,							/* default value - pass this fcOption to just flush any dirty buffers */
																/*	bits and masks */
	fcTrashBit					= 0,							/* (don't pass this as fcOption, use only for testing bit) */
	fcTrashMask					= 0x0001,						/* pass this fcOption value to flush and trash cache blocks */
	fcFreeBit					= 1,							/* (don't pass this as fcOption, use only for testing bit) */
	fcFreeMask					= 0x0003						/* pass this fcOption to flush and free cache blocks (Note: both fcTrash and fcFree bits are set) */
};



/*
 * FCBRec.fcbFlags bits
 */

enum {
	fcbWriteBit					= 0,							/* Data can be written to this file */
	fcbWriteMask				= 0x01,
	fcbResourceBit				= 1,							/* This file is a resource fork */
	fcbResourceMask				= 0x02,
	fcbWriteLockedBit			= 2,							/* File has a locked byte range */
	fcbWriteLockedMask			= 0x04,
	fcbSharedWriteBit			= 4,							/* File is open for shared write access */
	fcbSharedWriteMask			= 0x10,
	fcbFileLockedBit			= 5,							/* File is locked (write-protected) */
	fcbFileLockedMask			= 0x20,
	fcbOwnClumpBit				= 6,							/* File has clump size specified in FCB */
	fcbOwnClumpMask				= 0x40,
	fcbModifiedBit				= 7,							/* File has changed since it was last flushed */
	fcbModifiedMask				= 0x80
};


/*
 *	HFS Utility routine records
 */

/*
 * record used by UTGetPathComponentName
 */
struct ParsePathRec {
	StringPtr 						namePtr;					/* pathname to parse */
	short 							startOffset;				/* where to start parsing */
	short 							componentLength;			/* the length of the pathname component parsed */
	SignedByte 						moreName;					/* non-zero if there are more components after this one */
	SignedByte 						foundDelimiter;				/* non-zero if parsing stopped because a colon (:) delimiter was found */
};
typedef struct ParsePathRec ParsePathRec;

typedef ParsePathRec *					ParsePathRecPtr;
struct WDCBRec {
	VCBPtr 							wdVCBPtr;					/* Pointer to VCB of this working directory */
	long 							wdDirID;					/* Directory ID number of this working directory */
	long 							wdCatHint;					/* Hint for finding this working directory */
	long 							wdProcID;					/* Process that created this working directory */
};
typedef struct WDCBRec WDCBRec;

typedef WDCBRec *						WDCBRecPtr;
struct FCBRec {
	unsigned long 					fcbFlNm;					/* FCB file number. Non-zero marks FCB used */
	SignedByte 						fcbFlags;					/* FCB flags */
	SignedByte 						fcbTypByt;					/* File type byte */
	unsigned short 					fcbSBlk;					/* File start block (in alloc size blks) */
	unsigned long 					fcbEOF;						/* Logical length or EOF in bytes */
	unsigned long 					fcbPLen;					/* Physical file length in bytes */
	unsigned long 					fcbCrPs;					/* Current position within file */
	VCBPtr 							fcbVPtr;					/* Pointer to the corresponding VCB */
	Ptr 							fcbBfAdr;					/* File's buffer address */
	unsigned short 					fcbFlPos;					/* Directory block this file is in */
																/* FCB Extensions for HFS */
	unsigned long 					fcbClmpSize;				/* Number of bytes per clump */
	Ptr 							fcbBTCBPtr;					/* Pointer to B*-Tree control block for file */
	unsigned long 					fcbExtRec[3];				/* First 3 file extents */
	OSType 							fcbFType;					/* File's 4 Finder Type bytes */
	unsigned long 					fcbCatPos;					/* Catalog hint for use on Close */
	unsigned long 					fcbDirID;					/* Parent Directory ID */
	Str31 							fcbCName;					/* CName of open file */
};
typedef struct FCBRec FCBRec;

typedef FCBRec *						FCBRecPtr;

/*
 * FormatListRec as returned by the .Sony disk driver's
 * Return Format List status call (csCode = 6).
 * If the status call to get this list for a drive is not
 * implemented by the driver, then a list with one entry
 * is contructed from the drive queue element for the drive.
 */
struct FormatListRec {
	unsigned long 					volSize;					/* disk capacity in SECTORs */
	SignedByte 						formatFlags;				/* flags */
	SignedByte 						sectorsPerTrack;			/* sectors per track side */
	unsigned short 					tracks;						/* number of tracks */
};
typedef struct FormatListRec FormatListRec;

typedef FormatListRec *					FormatListRecPtr;
/*
 * SizeListRec built from FormatListRecs as described above.
 */
struct SizeListRec {
	short 							sizeListFlags;				/* flags as set by external file system */
	FormatListRec 					sizeEntry;					/* disk driver format list record */
};
typedef struct SizeListRec SizeListRec;

typedef SizeListRec *					SizeListRecPtr;
/*
 * paramBlock for the diCIEvaluateSize call
 */
struct DICIEvaluateSizeRec {
	short 							defaultSizeIndex;			/* default size for this FS */
	short 							numSizeEntries;				/* number of size entries */
	short 							driveNumber;				/* drive number */
	SizeListRecPtr 					sizeListPtr;				/* ptr to size entry table */
	unsigned short 					sectorSize;					/* bytes per sector */
};
typedef struct DICIEvaluateSizeRec DICIEvaluateSizeRec;

typedef DICIEvaluateSizeRec *			DICIEvaluateSizeRecPtr;
/*
 * paramBlock for the diCIExtendedZero call
 */
struct DICIExtendedZeroRec {
	short 							driveNumber;				/* drive number */
	StringPtr 						volNamePtr;					/* ptr to volume name string */
	short 							fsid;						/* file system ID */
	short 							volTypeSelector;			/* volume type selector, if supports more than 1 type */
	unsigned short 					numDefectBlocks;			/* number of bad logical blocks */
	unsigned short 					defectListSize;				/* size of the defect list buffer in bytes */
	Ptr 							defectListPtr;				/* pointer to defect list buffer */
	unsigned long 					volSize;					/* size of volume in SECTORs */
	unsigned short 					sectorSize;					/* bytes per sector */
	Ptr 							extendedInfoPtr;			/* ptr to extended info */
};
typedef struct DICIExtendedZeroRec DICIExtendedZeroRec;

typedef DICIExtendedZeroRec *			DICIExtendedZeroRecPtr;
/*
 * paramBlock for the diCIValidateVolName call
 */
struct DICIValidateVolNameRec {
	char 							theChar;					/* the character to validate */
	Boolean 						hasMessageBuffer;			/* false if no message */
	short 							charOffset;					/* position of the current character (first char = 1) */
	StringPtr 						messageBufferPtr;			/* pointer to message buffer or nil */
	short 							charByteType;				/* theChar's byte type (smSingleByte, smFirstByte, or smLastByte) */
};
typedef struct DICIValidateVolNameRec DICIValidateVolNameRec;

typedef DICIValidateVolNameRec *		DICIValidateVolNameRecPtr;
/*
 * paramBlock for the diCIGetVolTypeInfo call
 */
struct DICIGetVolTypeInfoRec {
	unsigned long 					volSize;					/* size of volume in SECTORs */
	unsigned short 					sectorSize;					/* bytes per sector */
	short 							numVolTypes;				/* number of volume types supported */
	Str31 							volTypesBuffer[4];			/* 4 string buffers */
};
typedef struct DICIGetVolTypeInfoRec DICIGetVolTypeInfoRec;

typedef DICIGetVolTypeInfoRec *			DICIGetVolTypeInfoRecPtr;
/*
 * paramBlock for the diCIGetFormatString call
 */
struct DICIGetFormatStringRec {
	unsigned long 					volSize;					/* volume size in SECTORs */
	unsigned short 					sectorSize;					/* sector size */
	short 							volTypeSelector;			/* volume type selector */
	short 							stringKind;					/* sub-function = type of string */
	Str255 							stringBuffer;				/* string buffer */
};
typedef struct DICIGetFormatStringRec DICIGetFormatStringRec;

typedef DICIGetFormatStringRec *		DICIGetFormatStringRecPtr;
/*
 * paramBlock for the diCIGetExtendedFormatParams call
 */
struct DICIGetExtendedFormatRec {
	short 							driveNumber;				/* drive number */
	short 							volTypeSelector;			/* volume type selector or 0 */
	unsigned long 					volSize;					/* size of volume in SECTORs */
	unsigned short 					sectorSize;					/* bytes per sector */
	FSSpecPtr 						fileSystemSpecPtr;			/* pointer to the foreign file system's FSSpec */
	Ptr 							extendedInfoPtr;			/* pointer to extended parameter structure */
};
typedef struct DICIGetExtendedFormatRec DICIGetExtendedFormatRec;

typedef DICIGetExtendedFormatRec *		DICIGetExtendedFormatRecPtr;


/*
 	File:		Gestalt.h
 
*/
typedef CALLBACK_API( OSErr , SelectorFunctionProcPtr )(OSType selector, long *response);
typedef STACK_UPP_TYPE(SelectorFunctionProcPtr) 				SelectorFunctionUPP;


/*
 	File:		TextCommon.h
 
*/

/* LocaleIdentifier is an obsolete Copland typedef, will be removed soon*/
typedef UInt32 							LocaleIdentifier;
/* TextEncodingBase type & values */
/* (values 0-32 correspond to the Script Codes defined in Inside Macintosh: Text pages 6-52 and 6-53 */
typedef UInt32 							TextEncodingBase;

enum {
																/* Mac OS encodings*/
	kTextEncodingMacRoman		= 0L,
	kTextEncodingMacJapanese	= 1,
	kTextEncodingMacChineseTrad	= 2,
	kTextEncodingMacKorean		= 3,
	kTextEncodingMacArabic		= 4,
	kTextEncodingMacHebrew		= 5,
	kTextEncodingMacGreek		= 6,
	kTextEncodingMacCyrillic	= 7,
	kTextEncodingMacDevanagari	= 9,
	kTextEncodingMacGurmukhi	= 10,
	kTextEncodingMacGujarati	= 11,
	kTextEncodingMacOriya		= 12,
	kTextEncodingMacBengali		= 13,
	kTextEncodingMacTamil		= 14,
	kTextEncodingMacTelugu		= 15,
	kTextEncodingMacKannada		= 16,
	kTextEncodingMacMalayalam	= 17,
	kTextEncodingMacSinhalese	= 18,
	kTextEncodingMacBurmese		= 19,
	kTextEncodingMacKhmer		= 20,
	kTextEncodingMacThai		= 21,
	kTextEncodingMacLaotian		= 22,
	kTextEncodingMacGeorgian	= 23,
	kTextEncodingMacArmenian	= 24,
	kTextEncodingMacChineseSimp	= 25,
	kTextEncodingMacTibetan		= 26,
	kTextEncodingMacMongolian	= 27,
	kTextEncodingMacEthiopic	= 28,
	kTextEncodingMacCentralEurRoman = 29,
	kTextEncodingMacVietnamese	= 30,
	kTextEncodingMacExtArabic	= 31,							/* The following use script code 0, smRoman*/
	kTextEncodingMacSymbol		= 33,
	kTextEncodingMacDingbats	= 34,
	kTextEncodingMacTurkish		= 35,
	kTextEncodingMacCroatian	= 36,
	kTextEncodingMacIcelandic	= 37,
	kTextEncodingMacRomanian	= 38,							/* The following use script code 4, smArabic*/
	kTextEncodingMacFarsi		= 0x8C,							/* Like MacArabic but uses Farsi digits*/
																/* The following use script code 7, smCyrillic*/
	kTextEncodingMacUkrainian	= 0x98,							/* The following use script code 32, smUnimplemented*/
	kTextEncodingMacVT100		= 0xFC,							/* VT100/102 font from Comm Toolbox: Latin-1 repertoire + box drawing etc*/
																/* Special Mac OS encodings*/
	kTextEncodingMacHFS			= 0xFF,							/* Meta-value, should never appear in a table.*/
																/* Unicode & ISO UCS encodings begin at 0x100*/
	kTextEncodingUnicodeDefault	= 0x0100,						/* Meta-value, should never appear in a table.*/
	kTextEncodingUnicodeV1_1	= 0x0101,
	kTextEncodingISO10646_1993	= 0x0101,						/* Code points identical to Unicode 1.1*/
	kTextEncodingUnicodeV2_0	= 0x0103,						/* New location for Korean Hangul*/
																/* ISO 8-bit and 7-bit encodings begin at 0x200*/
	kTextEncodingISOLatin1		= 0x0201,						/* ISO 8859-1*/
	kTextEncodingISOLatin2		= 0x0202,						/* ISO 8859-2*/
	kTextEncodingISOLatinCyrillic = 0x0205,						/* ISO 8859-5*/
	kTextEncodingISOLatinArabic	= 0x0206,						/* ISO 8859-6, = ASMO 708, =DOS CP 708*/
	kTextEncodingISOLatinGreek	= 0x0207,						/* ISO 8859-7*/
	kTextEncodingISOLatinHebrew	= 0x0208,						/* ISO 8859-8*/
	kTextEncodingISOLatin5		= 0x0209,						/* ISO 8859-9*/
																/* MS-DOS & Windows encodings begin at 0x400*/
	kTextEncodingDOSLatinUS		= 0x0400,						/* code page 437*/
	kTextEncodingDOSGreek		= 0x0405,						/* code page 737 (formerly code page 437G)*/
	kTextEncodingDOSBalticRim	= 0x0406,						/* code page 775*/
	kTextEncodingDOSLatin1		= 0x0410,						/* code page 850, "Multilingual"*/
	kTextEncodingDOSGreek1		= 0x0411,						/* code page 851*/
	kTextEncodingDOSLatin2		= 0x0412,						/* code page 852, Slavic*/
	kTextEncodingDOSCyrillic	= 0x0413,						/* code page 855, IBM Cyrillic*/
	kTextEncodingDOSTurkish		= 0x0414,						/* code page 857, IBM Turkish*/
	kTextEncodingDOSPortuguese	= 0x0415,						/* code page 860*/
	kTextEncodingDOSIcelandic	= 0x0416,						/* code page 861*/
	kTextEncodingDOSHebrew		= 0x0417,						/* code page 862*/
	kTextEncodingDOSCanadianFrench = 0x0418,					/* code page 863*/
	kTextEncodingDOSArabic		= 0x0419,						/* code page 864*/
	kTextEncodingDOSNordic		= 0x041A,						/* code page 865*/
	kTextEncodingDOSRussian		= 0x041B,						/* code page 866*/
	kTextEncodingDOSGreek2		= 0x041C,						/* code page 869, IBM Modern Greek*/
	kTextEncodingDOSThai		= 0x041D,						/* code page 874, also for Windows*/
	kTextEncodingDOSJapanese	= 0x0420,						/* code page 932, also for Windows*/
	kTextEncodingDOSChineseSimplif = 0x0421,					/* code page 936, also for Windows*/
	kTextEncodingDOSKorean		= 0x0422,						/* code page 949, also for Windows; Unified Hangul Code*/
	kTextEncodingDOSChineseTrad	= 0x0423,						/* code page 950, also for Windows*/
	kTextEncodingWindowsLatin1	= 0x0500,						/* code page 1252*/
	kTextEncodingWindowsANSI	= 0x0500,						/* code page 1252 (alternate name)*/
	kTextEncodingWindowsLatin2	= 0x0501,						/* code page 1250, Central Europe*/
	kTextEncodingWindowsCyrillic = 0x0502,						/* code page 1251, Slavic Cyrillic*/
	kTextEncodingWindowsGreek	= 0x0503,						/* code page 1253*/
	kTextEncodingWindowsLatin5	= 0x0504,						/* code page 1254, Turkish*/
	kTextEncodingWindowsHebrew	= 0x0505,						/* code page 1255*/
	kTextEncodingWindowsArabic	= 0x0506,						/* code page 1256*/
	kTextEncodingWindowsBalticRim = 0x0507,						/* code page 1257*/
	kTextEncodingWindowsKoreanJohab = 0x0510,					/* code page 1361, for Windows NT*/
																/* Various national standards begin at 0x600*/
	kTextEncodingUS_ASCII		= 0x0600,
	kTextEncodingJIS_X0201_76	= 0x0620,
	kTextEncodingJIS_X0208_83	= 0x0621,
	kTextEncodingJIS_X0208_90	= 0x0622,
	kTextEncodingJIS_X0212_90	= 0x0623,
	kTextEncodingJIS_C6226_78	= 0x0624,
	kTextEncodingGB_2312_80		= 0x0630,
	kTextEncodingGBK_95			= 0x0631,						/* annex to GB 13000-93; for Windows 95*/
	kTextEncodingKSC_5601_87	= 0x0640,						/* same as KSC 5601-92 without Johab annex*/
	kTextEncodingKSC_5601_92_Johab = 0x0641,					/* KSC 5601-92 Johab annex*/
	kTextEncodingCNS_11643_92_P1 = 0x0651,						/* CNS 11643-1992 plane 1*/
	kTextEncodingCNS_11643_92_P2 = 0x0652,						/* CNS 11643-1992 plane 2*/
	kTextEncodingCNS_11643_92_P3 = 0x0653,						/* CNS 11643-1992 plane 3 (was plane 14 in 1986 version)*/
																/* ISO 2022 collections begin at 0x800*/
	kTextEncodingISO_2022_JP	= 0x0820,
	kTextEncodingISO_2022_JP_2	= 0x0821,
	kTextEncodingISO_2022_CN	= 0x0830,
	kTextEncodingISO_2022_CN_EXT = 0x0831,
	kTextEncodingISO_2022_KR	= 0x0840,						/* EUC collections begin at 0x900*/
	kTextEncodingEUC_JP			= 0x0920,						/* ISO 646, 1-byte katakana, JIS 208, JIS 212*/
	kTextEncodingEUC_CN			= 0x0930,						/* ISO 646, GB 2312-80*/
	kTextEncodingEUC_TW			= 0x0931,						/* ISO 646, CNS 11643-1992 Planes 1-16*/
	kTextEncodingEUC_KR			= 0x0940,						/* ISO 646, KS C 5601-1987*/
																/* Misc standards begin at 0xA00*/
	kTextEncodingShiftJIS		= 0x0A01,						/* plain Shift-JIS*/
	kTextEncodingKOI8_R			= 0x0A02,						/* Russian internet standard*/
	kTextEncodingBig5			= 0x0A03,						/* Big-5 (has variants)*/
	kTextEncodingMacRomanLatin1	= 0x0A04,						/* Mac OS Roman permuted to align with ISO Latin-1*/
	kTextEncodingHZ_GB_2312		= 0x0A05,						/* HZ (RFC 1842, for Chinese mail & news)*/
																/* Other platform encodings*/
	kTextEncodingNextStepLatin	= 0x0B01,						/* NextStep encoding*/
																/* EBCDIC & IBM host encodings begin at 0xC00*/
	kTextEncodingEBCDIC_US		= 0x0C01,						/* basic EBCDIC-US*/
	kTextEncodingEBCDIC_CP037	= 0x0C02,						/* code page 037, extended EBCDIC (Latin-1 set) for US,Canada...*/
																/* Special value*/
	kTextEncodingMultiRun		= 0x0FFF,						/* Multi-encoding text with external run info*/
																/* The following are older names for backward compatibility*/
	kTextEncodingMacTradChinese	= 2,
	kTextEncodingMacRSymbol		= 8,
	kTextEncodingMacSimpChinese	= 25,
	kTextEncodingMacGeez		= 28,
	kTextEncodingMacEastEurRoman = 29,
	kTextEncodingMacUninterp	= 32
};

/* TextEncodingVariant type & values */
typedef UInt32 							TextEncodingVariant;

enum {
																/* Default TextEncodingVariant, for any TextEncodingBase*/
	kTextEncodingDefaultVariant	= 0,							/* Variants of kTextEncodingMacIcelandic													*/
	kMacIcelandicStandardVariant = 0,							/* 0xBB & 0xBC are fem./masc. ordinal indicators*/
	kMacIcelandicTrueTypeVariant = 1,							/* 0xBB & 0xBC are fi/fl ligatures*/
																/* Variants of kTextEncodingMacJapanese*/
	kMacJapaneseStandardVariant	= 0,
	kMacJapaneseStdNoVerticalsVariant = 1,
	kMacJapaneseBasicVariant	= 2,
	kMacJapanesePostScriptScrnVariant = 3,
	kMacJapanesePostScriptPrintVariant = 4,
	kMacJapaneseVertAtKuPlusTenVariant = 5,						/* Variant options for most Japanese encodings (MacJapanese, ShiftJIS, EUC-JP, ISO 2022-JP)	*/
																/* These can be OR-ed into the variant value in any combination*/
	kJapaneseNoOneByteKanaOption = 0x20,
	kJapaneseUseAsciiBackslashOption = 0x40,					/* Variants of kTextEncodingMacArabic*/
	kMacArabicStandardVariant	= 0,							/* 0xC0 is 8-spoke asterisk, 0x2A & 0xAA are asterisk (e.g. Cairo)*/
	kMacArabicTrueTypeVariant	= 1,							/* 0xC0 is asterisk, 0x2A & 0xAA are multiply signs (e.g. Baghdad)*/
	kMacArabicThuluthVariant	= 2,							/* 0xC0 is Arabic five-point star, 0x2A & 0xAA are multiply signs*/
	kMacArabicAlBayanVariant	= 3,							/* 8-spoke asterisk, multiply sign, Koranic ligatures & parens*/
																/* Variants of kTextEncodingMacFarsi*/
	kMacFarsiStandardVariant	= 0,							/* 0xC0 is 8-spoke asterisk, 0x2A & 0xAA are asterisk (e.g. Tehran)*/
	kMacFarsiTrueTypeVariant	= 1,							/* asterisk, multiply signs, Koranic ligatures, geometric shapes*/
																/* Variants of kTextEncodingMacHebrew*/
	kMacHebrewStandardVariant	= 0,
	kMacHebrewFigureSpaceVariant = 1,							/* Variants of Unicode & ISO 10646 encodings*/
	kUnicodeNoSubset			= 0,
	kUnicodeNoCompatibilityVariant = 1,
	kUnicodeMaxDecomposedVariant = 2,
	kUnicodeNoComposedVariant	= 3,
	kUnicodeNoCorporateVariant	= 4,							/* Variants of Big-5 encoding*/
	kBig5_BasicVariant			= 0,
	kBig5_StandardVariant		= 1,							/* 0xC6A1-0xC7FC: kana, Cyrillic, enclosed numerics*/
	kBig5_ETenVariant			= 2,							/* adds kana, Cyrillic, radicals, etc with hi bytes C6-C8,F9*/
																/* The following are older names for backward compatibility*/
	kJapaneseStandardVariant	= 0,
	kJapaneseStdNoVerticalsVariant = 1,
	kJapaneseBasicVariant		= 2,
	kJapanesePostScriptScrnVariant = 3,
	kJapanesePostScriptPrintVariant = 4,
	kJapaneseVertAtKuPlusTenVariant = 5,						/* kJapaneseStdNoOneByteKanaVariant = 6,	// replaced by kJapaneseNoOneByteKanaOption*/
																/* kJapaneseBasicNoOneByteKanaVariant = 7,	// replaced by kJapaneseNoOneByteKanaOption	*/
	kHebrewStandardVariant		= 0,
	kHebrewFigureSpaceVariant	= 1
};

/* TextEncodingFormat type & values */
typedef UInt32 							TextEncodingFormat;

enum {
																/* Default TextEncodingFormat for any TextEncodingBase*/
	kTextEncodingDefaultFormat	= 0,							/* Formats for Unicode & ISO 10646*/
	kUnicode16BitFormat			= 0,
	kUnicodeUTF7Format			= 1,
	kUnicodeUTF8Format			= 2,
	kUnicode32BitFormat			= 3
};

/* TextEncoding type */
typedef UInt32 							TextEncoding;
/* name part selector for GetTextEncodingName*/
typedef UInt32 							TextEncodingNameSelector;

enum {
	kTextEncodingFullName		= 0,
	kTextEncodingBaseName		= 1,
	kTextEncodingVariantName	= 2,
	kTextEncodingFormatName		= 3
};

/* Types used in conversion */
struct TextEncodingRun {
	ByteOffset 						offset;
	TextEncoding 					textEncoding;
};
typedef struct TextEncodingRun TextEncodingRun;

typedef TextEncodingRun *				TextEncodingRunPtr;
typedef const TextEncodingRun *			ConstTextEncodingRunPtr;
struct ScriptCodeRun {
	ByteOffset 						offset;
	ScriptCode 						script;
};
typedef struct ScriptCodeRun ScriptCodeRun;

typedef ScriptCodeRun *					ScriptCodeRunPtr;
typedef const ScriptCodeRun *			ConstScriptCodeRunPtr;
typedef UInt8 *							TextPtr;
typedef const UInt8 *					ConstTextPtr;
/* Basic types for Unicode characters and strings: */
typedef UniChar *						UniCharArrayPtr;
typedef const UniChar *					ConstUniCharArrayPtr;
/* enums for TextEncoding Conversion routines*/

enum {
	kTextScriptDontCare			= -128,
	kTextLanguageDontCare		= -128,
	kTextRegionDontCare			= -128
};



/*
 	File:		UnicodeConverter.h
 
 
*/

/* Unicode conversion contexts: */
typedef struct OpaqueTextToUnicodeInfo*  TextToUnicodeInfo;
typedef struct OpaqueUnicodeToTextInfo*  UnicodeToTextInfo;
typedef struct OpaqueUnicodeToTextRunInfo*  UnicodeToTextRunInfo;
typedef const TextToUnicodeInfo 		ConstTextToUnicodeInfo;
typedef const UnicodeToTextInfo 		ConstUnicodeToTextInfo;
/* UnicodeMapVersion type & values */
typedef SInt32 							UnicodeMapVersion;

enum {
	kUnicodeUseLatestMapping	= -1,
	kUnicodeUseHFSPlusMapping	= 4
};

/* Types used in conversion */
struct UnicodeMapping {
	TextEncoding 					unicodeEncoding;
	TextEncoding 					otherEncoding;
	UnicodeMapVersion 				mappingVersion;
};
typedef struct UnicodeMapping UnicodeMapping;

typedef UnicodeMapping *				UnicodeMappingPtr;
typedef const UnicodeMapping *			ConstUnicodeMappingPtr;
/* Control flags for ConvertFromUnicodeToText and ConvertFromTextToUnicode */

enum {
	kUnicodeUseFallbacksBit		= 0,
	kUnicodeKeepInfoBit			= 1,
	kUnicodeDirectionalityBits	= 2,
	kUnicodeVerticalFormBit		= 4,
	kUnicodeLooseMappingsBit	= 5,
	kUnicodeStringUnterminatedBit = 6,
	kUnicodeTextRunBit			= 7,
	kUnicodeKeepSameEncodingBit	= 8
};


enum {
	kUnicodeUseFallbacksMask	= 1L << kUnicodeUseFallbacksBit,
	kUnicodeKeepInfoMask		= 1L << kUnicodeKeepInfoBit,
	kUnicodeDirectionalityMask	= 3L << kUnicodeDirectionalityBits,
	kUnicodeVerticalFormMask	= 1L << kUnicodeVerticalFormBit,
	kUnicodeLooseMappingsMask	= 1L << kUnicodeLooseMappingsBit,
	kUnicodeStringUnterminatedMask = 1L << kUnicodeStringUnterminatedBit,
	kUnicodeTextRunMask			= 1L << kUnicodeTextRunBit,
	kUnicodeKeepSameEncodingMask = 1L << kUnicodeKeepSameEncodingBit
};

/* Values for kUnicodeDirectionality field */

enum {
	kUnicodeDefaultDirection	= 0,
	kUnicodeLeftToRight			= 1,
	kUnicodeRightToLeft			= 2
};

/* Directionality masks for control flags */

enum {
	kUnicodeDefaultDirectionMask = kUnicodeDefaultDirection << kUnicodeDirectionalityBits,
	kUnicodeLeftToRightMask		= kUnicodeLeftToRight << kUnicodeDirectionalityBits,
	kUnicodeRightToLeftMask		= kUnicodeRightToLeft << kUnicodeDirectionalityBits
};

/* Control flags for TruncateForUnicodeToText: */
/*
   Now TruncateForUnicodeToText uses control flags from the same set as used by
   ConvertFromTextToUnicode, ConvertFromUnicodeToText, etc., but only
   kUnicodeStringUnterminatedMask is meaningful for TruncateForUnicodeToText.
   
   Previously two special control flags were defined for TruncateForUnicodeToText:
  		kUnicodeTextElementSafeBit = 0
  		kUnicodeRestartSafeBit = 1
   However, neither of these was implemented.
   Instead of implementing kUnicodeTextElementSafeBit, we now use
   kUnicodeStringUnterminatedMask since it accomplishes the same thing and avoids
   having special flags just for TruncateForUnicodeToText
   Also, kUnicodeRestartSafeBit is unnecessary, since restart-safeness is handled by
   setting kUnicodeKeepInfoBit with ConvertFromUnicodeToText.
   If TruncateForUnicodeToText is called with one or both of the old special control
   flags set (bits 0 or 1), it will not generate a paramErr, but the old bits have no
   effect on its operation.
*/

/* Filter bits for filter field in QueryUnicodeMappings and CountUnicodeMappings: */

enum {
	kUnicodeMatchUnicodeBaseBit	= 0,
	kUnicodeMatchUnicodeVariantBit = 1,
	kUnicodeMatchUnicodeFormatBit = 2,
	kUnicodeMatchOtherBaseBit	= 3,
	kUnicodeMatchOtherVariantBit = 4,
	kUnicodeMatchOtherFormatBit	= 5
};


enum {
	kUnicodeMatchUnicodeBaseMask = 1L << kUnicodeMatchUnicodeBaseBit,
	kUnicodeMatchUnicodeVariantMask = 1L << kUnicodeMatchUnicodeVariantBit,
	kUnicodeMatchUnicodeFormatMask = 1L << kUnicodeMatchUnicodeFormatBit,
	kUnicodeMatchOtherBaseMask	= 1L << kUnicodeMatchOtherBaseBit,
	kUnicodeMatchOtherVariantMask = 1L << kUnicodeMatchOtherVariantBit,
	kUnicodeMatchOtherFormatMask = 1L << kUnicodeMatchOtherFormatBit
};

/* Control flags for SetFallbackUnicodeToText */

enum {
	kUnicodeFallbackSequencingBits = 0
};


enum {
	kUnicodeFallbackSequencingMask = 3L << kUnicodeFallbackSequencingBits
};

/* values for kUnicodeFallbackSequencing field */

enum {
	kUnicodeFallbackDefaultOnly	= 0L,
	kUnicodeFallbackCustomOnly	= 1L,
	kUnicodeFallbackDefaultFirst = 2L,
	kUnicodeFallbackCustomFirst	= 3L
};



/*
 	File:		Timer.h
 
 
*/


enum {
																/* high bit of qType is set if task is active */
	kTMTaskActive				= (1L << 15)
};

typedef struct TMTask 					TMTask;
typedef TMTask *						TMTaskPtr;
typedef CALLBACK_API( void , TimerProcPtr )(TMTaskPtr tmTaskPtr);
/*
	WARNING: TimerProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
typedef REGISTER_UPP_TYPE(TimerProcPtr) 						TimerUPP;
struct TMTask {
	QElemPtr 						qLink;
	short 							qType;
	TimerUPP 						tmAddr;
	long 							tmCount;
	long 							tmWakeUp;
	long 							tmReserved;
};


/*
 	File:		TextCommonPriv.h
 
 
*/


/*
   -----------------------------------------------------------------------------------------------------------
   TextEncoding creation & extraction macros.
   Current packed format:
  		31 30 29    26 25                 16 15                             0
  		|pack| format |       variant       |              base              |
  		|vers|        |                     |                                |
  		|2bit| 4 bits |       10 bits       |            16 bits             |
   Unpacked elements
  		base                                 15                             0
  		|                0                  |            16 bits             |
  		variant                                              9              0
  		|                0                                  |      10 bits   |
  		format                                                       3      0
  		|                0                                          | 4 bits |
   -----------------------------------------------------------------------------------------------------------
*/

enum {
	kTextEncodingVersion		= 0
};


enum {
	kTextEncodingBaseShiftBits	= 0,							/*	<13>*/
	kTextEncodingVariantShiftBits = 16,							/*	<13>*/
	kTextEncodingFormatShiftBits = 26,							/*	<13><16>*/
	kTextEncodingVersionShiftBits = 30
};



enum {
	kTextEncodingBaseSourceMask	= 0x0000FFFF,					/*	16 bits <13>*/
	kTextEncodingVariantSourceMask = 0x000003FF,				/*	10 bits <13><16>*/
	kTextEncodingFormatSourceMask = 0x0000000F,					/*	 4 bits <13><16>*/
	kTextEncodingVersionSourceMask = 0x00000003					/*	 2 bits*/
};


enum {
	kTextEncodingBaseMask		= kTextEncodingBaseSourceMask << kTextEncodingBaseShiftBits,
	kTextEncodingVariantMask	= kTextEncodingVariantSourceMask << kTextEncodingVariantShiftBits,
	kTextEncodingFormatMask		= kTextEncodingFormatSourceMask << kTextEncodingFormatShiftBits,
	kTextEncodingVersionMask	= kTextEncodingVersionSourceMask << kTextEncodingVersionShiftBits
};


enum {
	kTextEncodingVersionShifted	= (kTextEncodingVersion & kTextEncodingVersionSourceMask) << kTextEncodingVersionShiftBits
};


#define CreateTextEncodingPriv(base,variant,format) \
				( ((base & kTextEncodingBaseSourceMask) << kTextEncodingBaseShiftBits) \
				| ((variant & kTextEncodingVariantSourceMask) << kTextEncodingVariantShiftBits) \
				| ((format & kTextEncodingFormatSourceMask) << kTextEncodingFormatShiftBits) \
				| (kTextEncodingVersionShifted) )
#define GetTextEncodingBasePriv(encoding) \
				((encoding & kTextEncodingBaseMask) >> kTextEncodingBaseShiftBits)
#define GetTextEncodingVariantPriv(encoding) \
				((encoding & kTextEncodingVariantMask) >> kTextEncodingVariantShiftBits)
#define GetTextEncodingFormatPriv(encoding) \
				((encoding & kTextEncodingFormatMask) >> kTextEncodingFormatShiftBits)
#define IsMacTextEncoding(encoding) ((encoding & 0x0000FF00L) == 0x00000000L)
#define IsUnicodeTextEncoding(encoding) ((encoding & 0x0000FF00L) == 0x00000100L)
/* TextEncoding used by HFS*/

enum {
	kMacHFSTextEncoding			= 0x000000FF
};


/*
	File:		Instrumentation.h


*/
/*******************************************************************/
/*                        Types				                       */
/*******************************************************************/
/* Reference to an instrumentation class */
typedef struct InstOpaqueClassRef* InstClassRef;

/* Aliases to the generic instrumentation class for each type of class */
typedef InstClassRef InstPathClassRef;
typedef InstClassRef InstTraceClassRef;
typedef InstClassRef InstHistogramClassRef;
typedef InstClassRef InstSplitHistogramClassRef;
typedef InstClassRef InstMagnitudeClassRef;
typedef InstClassRef InstGrowthClassRef;
typedef InstClassRef InstTallyClassRef;

/* Reference to a data descriptor */
typedef struct InstOpaqueDataDescriptorRef* InstDataDescriptorRef;


/*******************************************************************/
/*            Constant Definitions                                 */
/*******************************************************************/

/* Reference to the root of the class hierarchy */
#define kInstRootClassRef	 ( (InstClassRef) -1)

/* Options used for creating classes */
typedef OptionBits InstClassOptions;


enum {
	kInstDisableClassMask			= 0x00,			/* Create the class disabled */
	kInstEnableClassMask			= 0x01,			/* Create the class enabled */

	kInstSummaryTraceClassMask		= 0x20			/* Create a summary trace class instead of a regular one */
};


EXTERN_API( OSErr )
UTLocateVCBByRefNum				(short 					refNum,
								 short *				vRefNum,
								 VCBPtr *				volCtrlBlockPtr);
								
#if 0
EXTERN_API( Ptr ) LMGetFCBSPtr(void);					
EXTERN_API( void ) LMSetFCBSPtr(Ptr value);

EXTERN_API( SInt16 ) LMGetFSFCBLen(void);
EXTERN_API( void ) LMSetFSFCBLen(SInt16 value);

EXTERN_API( QHdrPtr ) LMGetVCBQHdr(void);
EXTERN_API( void ) LMSetVCBQHdr(QHdrPtr value);

EXTERN_API( Ptr ) LMGetDefVCBPtr(void);
EXTERN_API( void ) LMSetDefVCBPtr(Ptr value);
#endif

EXTERN_API( Boolean )
EqualString						(ConstStr255Param 		str1,
								 ConstStr255Param 		str2,
								 Boolean 				caseSensitive,
								 Boolean 				diacSensitive);

EXTERN_API( void )
UprText							(Ptr 					textPtr,
								 short 					len);
								 


/*
 	File:		LowMemPriv.h
 
 
*/

/* The following replace storage used in low-mem on MacOS: */
extern struct FSVarsRec * gFSMVars;
extern UInt8 gHFSFlags;
extern UInt8 gFlushOnlyFlag;
extern Ptr gReqstVol;			/* May be unnecessary: not referenced from HFS itself? */

#if TARGET_OS_RHAPSODY

#define LMGetFSMVars() gFSMVars

#define LMGetHFSFlags() gHFSFlags
#define LMSetHFSFlags(FLAGS) (gHFSFlags = (FLAGS))

//#define LMGetReqstVol() gReqstVol
#define LMSetReqstVol(PTR) (gReqstVol = (PTR))

#define LMGetFlushOnly() gFlushOnlyFlag
#define LMSetFlushOnly(VALUE) (gFlushOnlyFlag = (VALUE))

#endif

#if 0
EXTERN_API( void )
LMSetFSMVars					(Ptr 					value);

EXTERN_API( Ptr )
LMGetPMSPPtr					(void);
EXTERN_API( void )
LMSetPMSPPtr					(Ptr 					value);

EXTERN_API( Ptr )
LMGetWDCBsPtr					(void);
EXTERN_API( void )
LMSetWDCBsPtr					(Ptr 					value);
#endif

EXTERN_API( void )
InsTime							(QElemPtr 				tmTaskPtr);
EXTERN_API( void )
PrimeTime						(QElemPtr 				tmTaskPtr,
								 long 					count);
EXTERN_API( void )
RmvTime							(QElemPtr 				tmTaskPtr);



/*
 	File:		StringComparePriv.h
 
 
*/

EXTERN_API( Boolean )
CaseAndMarkSensitiveEqualString	(void *					str1,
								 void *					str2,
								 unsigned long 			firstStringLength,
								 unsigned long 			secondStringLength);
								 

/* PROTOTYPES */

EXTERN_API( void )
BlockMove						(const void *			srcPtr,
								 void *					destPtr,
								 Size 					byteCount);
EXTERN_API( void )
BlockMoveData					(const void *			srcPtr,
								 void *					destPtr,
								 Size 					byteCount);

EXTERN_API_C( void )
BlockMoveUncached				(const void *			srcPtr,
								 void *					destPtr,
								 Size 					byteCount);

EXTERN_API_C( void )
BlockMoveDataUncached			(const void *			srcPtr,
								 void *					destPtr,
								 Size 					byteCount);

EXTERN_API_C( void )
BlockZero						(void *					destPtr,
								 Size 					byteCount);

EXTERN_API_C( void )
BlockZeroUncached				(void *					destPtr,
								 Size 					byteCount);

EXTERN_API( Ptr )
NewPtr							(Size 					byteCount);

EXTERN_API( Ptr )
NewPtrSys						(Size 					byteCount);

EXTERN_API( Ptr )
NewPtrClear						(Size 					byteCount);

EXTERN_API( Ptr )
NewPtrSysClear					(Size 					byteCount);

EXTERN_API( OSErr )
MemError						(void);

EXTERN_API( void )
DisposePtr						(Ptr 					p);

EXTERN_API( Size )
GetPtrSize						(Ptr 					p);

EXTERN_API( void )
SetPtrSize						(Ptr 					p,
								 Size 					newSize);
								 
EXTERN_API( void )
DisposeHandle					(Handle 				h);

EXTERN_API( void )
SetHandleSize					(Handle 				h,
								 Size 					newSize);

/*
 	File:		DateTimeUtils.h
 
 
*/
EXTERN_API( void )
GetDateTime						(unsigned long *		secs);


/*
	PLStringFuncs.h -- C string conversion functions for pascal
		
	Copyright Apple Computer,Inc.  1989, 1990, 1995
	All rights reserved

*/

/* Conditional Macros:
 *	UsingStaticLibs	- for CFM-68K:  Insures that #pragma import is never used.
 *	<none>			- for CFM-68K:	Insures that all functions and data items are
 *									marked as library imports
 */

Ptr PLstrstr(ConstStr255Param str1, ConstStr255Param str2);




#if 0
/*************************** Creation routines ***************************/
/* Create a class that has no statistic or trace data. It is a path node in the class hierarchy */
extern pascal OSStatus InstCreatePathClass( InstPathClassRef parentClass, const char *className, 
											InstClassOptions options, InstPathClassRef *returnPathClass);


/*
 Create a class that can log events to a trace buffer.
*/
extern pascal OSStatus InstCreateTraceClass( InstPathClassRef parentClass, const char *className, 
												OSType component, InstClassOptions options, 
												InstTraceClassRef *returnTraceClass);

/*
 Create a descriptor to a piece of data
 DataDescriptors must be used when logging trace data so that generic browsers can display the data.
*/
extern pascal OSStatus InstCreateDataDescriptor( const char *formatString, InstDataDescriptorRef *returnDescriptor);



extern pascal OSStatus		InstInitialize68K( void);
#endif


#endif	/* __MACOSSTUBS__ */
							 
