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
 	File:		Files.h
 
 	Contains:	File Manager (HFS and MFS) Interfaces.
 
 	Version:	System 7.5
 
 	DRI:		Don Brady
 
 	Copyright:	© 1985-1998 by Apple Computer, Inc., all rights reserved
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				Files.i
 					Revision:		76
 					Dated:			7/13/98
 					Last change by:	JL
 					Last comment:	Removed APPLE_ONLY_UNTIL_ALLEGRO conditional section for
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __FILES__
#define __FILES__

#ifndef __MACTYPES__
#include <MacTypes.h>
#endif
#ifndef __MIXEDMODE__
#include <MixedMode.h>
#endif
#ifndef __OSUTILS__
#include <OSUtils.h>
#endif

/* Finder constants where moved to Finder.Å */
#ifndef __FINDER__
#include <Finder.h>
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


enum {
	fsCurPerm					= 0x00,							/* open access permissions in ioPermssn */
	fsRdPerm					= 0x01,
	fsWrPerm					= 0x02,
	fsRdWrPerm					= 0x03,
	fsRdWrShPerm				= 0x04,
	fsRdDenyPerm				= 0x10,							/* for use with OpenDeny and OpenRFDeny */
	fsWrDenyPerm				= 0x20							/* for use with OpenDeny and OpenRFDeny */
};


enum {
	ioDirFlg					= 4,							/* directory bit in ioFlAttrib */
	ioDirMask					= 0x10
};


enum {
	fsRtParID					= 1,
	fsRtDirID					= 2
};


enum {
	fsAtMark					= 0,							/* positioning modes in ioPosMode */
	fsFromStart					= 1,
	fsFromLEOF					= 2,
	fsFromMark					= 3
};


enum {
																/* ioPosMode flags */
	pleaseCacheBit				= 4,							/* please cache this request */
	pleaseCacheMask				= 0x0010,
	noCacheBit					= 5,							/* please don't cache this request */
	noCacheMask					= 0x0020,
	rdVerifyBit					= 6,							/* read verify mode */
	rdVerifyMask				= 0x0040,
	rdVerify					= 64,							/* old name of rdVerifyMask */
	newLineBit					= 7,							/* newline mode */
	newLineMask					= 0x0080,
	newLineCharMask				= 0xFF00						/* newline character */
};



enum {
																/* CatSearch Search bitmask Constants */
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
	fsSBDrFndrInfo				= 4096,
	fsSBDrParID					= 8192
};


enum {
																/* CatSearch Search bit value Constants */
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
	fsSBDrFndrInfoBit			= 12,							/*directory-named version of fsSBFlXFndrInfoBit*/
	fsSBDrParIDBit				= 13							/*directory-named version of fsSBFlParIDBit*/
};


enum {
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
	bHasPersonalAccessPrivileges = 9,
	bHasUserGroupList			= 8,
	bHasCatSearch				= 7,
	bHasFileIDs					= 6,
	bHasBTreeMgr				= 5,
	bHasBlankAccessPrivileges	= 4,
	bSupportsAsyncRequests		= 3,							/* asynchronous requests to this volume are handled correctly at any time*/
	bSupportsTrashVolumeCache	= 2
};



enum {
																/* Desktop Database icon Constants */
	kLargeIcon					= 1,
	kLarge4BitIcon				= 2,
	kLarge8BitIcon				= 3,
	kSmallIcon					= 4,
	kSmall4BitIcon				= 5,
	kSmall8BitIcon				= 6
};


enum {
	kLargeIconSize				= 256,
	kLarge4BitIconSize			= 512,
	kLarge8BitIconSize			= 1024,
	kSmallIconSize				= 64,
	kSmall4BitIconSize			= 128,
	kSmall8BitIconSize			= 256
};


enum {
																/* Foreign Privilege Model Identifiers */
	fsUnixPriv					= 1
};


enum {
																/* Authentication Constants */
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



struct GetVolParmsInfoBuffer {
	short 							vMVersion;					/*version number*/
	long 							vMAttrib;					/*bit vector of attributes (see vMAttrib constants)*/
	Handle 							vMLocalHand;				/*handle to private data*/
	long 							vMServerAdr;				/*AppleTalk server address or zero*/
	long 							vMVolumeGrade;				/*approx. speed rating or zero if unrated*/
	short 							vMForeignPrivID;			/*foreign privilege model supported or zero if none*/
};
typedef struct GetVolParmsInfoBuffer	GetVolParmsInfoBuffer;
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
typedef struct IOParam					IOParam;
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
typedef struct FileParam				FileParam;
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
typedef struct VolumeParam				VolumeParam;
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
typedef struct CntrlParam				CntrlParam;
typedef CntrlParam *					CntrlParamPtr;

struct SlotDevParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioSRefNum;
	SInt8 							ioSVersNum;
	SInt8 							ioSPermssn;
	Ptr 							ioSMix;
	short 							ioSFlags;
	SInt8 							ioSlot;
	SInt8 							ioID;
};
typedef struct SlotDevParam				SlotDevParam;
typedef SlotDevParam *					SlotDevParamPtr;

struct MultiDevParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioMRefNum;
	SInt8 							ioMVersNum;
	SInt8 							ioMPermssn;
	Ptr 							ioMMix;
	short 							ioMFlags;
	Ptr 							ioSEBlkPtr;
};
typedef struct MultiDevParam			MultiDevParam;
typedef MultiDevParam *					MultiDevParamPtr;

union ParamBlockRec {
	IOParam 						ioParam;
	FileParam 						fileParam;
	VolumeParam 					volumeParam;
	CntrlParam 						cntrlParam;
	SlotDevParam 					slotDevParam;
	MultiDevParam 					multiDevParam;
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
typedef struct HFileInfo				HFileInfo;

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
typedef struct DirInfo					DirInfo;

union CInfoPBRec {
	HFileInfo 						hFileInfo;
	DirInfo 						dirInfo;
};
typedef union CInfoPBRec				CInfoPBRec;

typedef CInfoPBRec *					CInfoPBPtr;

struct XCInfoPBRec {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	ProcPtr 						ioCompletion;				/* --> A pointer to a completion routine */
	OSErr 							ioResult;					/* --> The result code of the function */
	StringPtr 						ioNamePtr;					/* --> Pointer to pathname to object */
	short 							ioVRefNum;					/* --> A volume specification */
	long 							filler1;
	StringPtr 						ioShortNamePtr;				/* <-> A pointer to the short name string buffer - required! */
	short 							filler2;
	short 							ioPDType;					/* <-- The ProDOS file type */
	long 							ioPDAuxType;				/* <-- The ProDOS aux type */
	long 							filler3[2];
	long 							ioDirID;					/* --> A directory ID */
};
typedef struct XCInfoPBRec				XCInfoPBRec;

typedef XCInfoPBRec *					XCInfoPBPtr;
/* Catalog position record */

struct CatPositionRec {
	long 							initialize;
	short 							priv[6];
};
typedef struct CatPositionRec			CatPositionRec;

struct FSSpec {
	short 							vRefNum;
	long 							parID;
	StrFileName 					name;						/* a Str63 on MacOS*/
};
typedef struct FSSpec					FSSpec;
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
	The following are structures to be filled out with the _PBGetVolMountInfo call
	and passed back into the _PBVolumeMount call for external file system mounts. 
*/
/* the "signature" of the file system */
typedef OSType 							VolumeType;

enum {
																/* the signature for AppleShare */
	AppleShareMediaType			= FOUR_CHAR_CODE('afpm')
};

/*
	VolMount stuff was once in FSM.Å
*/

struct VolMountInfoHeader {
	short 							length;						/* length of location data (including self) */
	VolumeType 						media;						/* type of media.  Variable length data follows */
};
typedef struct VolMountInfoHeader		VolMountInfoHeader;

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
typedef struct VolumeMountInfoHeader	VolumeMountInfoHeader;
typedef VolumeMountInfoHeader *			VolumeMountInfoHeaderPtr;
/* volume mount flags */

enum {
	volMountNoLoginMsgFlagBit	= 0,							/* Input to VolumeMount: If set, the file system */
	volMountNoLoginMsgFlagMask	= 0x0001,						/*  should suppresss any log-in message/greeting dialog */
	volMountExtendedFlagsBit	= 7,							/* Input to VolumeMount: If set, the mount info is a */
	volMountExtendedFlagsMask	= 0x0080,						/*  AFPXVolMountInfo record for 3.7 AppleShare Client */
	volMountInteractBit			= 15,							/* Input to VolumeMount: If set, it's OK for the file system */
	volMountInteractMask		= 0x8000,						/*  to perform user interaction to mount the volume */
	volMountChangedBit			= 14,							/* Output from VoumeMount: If set, the volume was mounted, but */
	volMountChangedMask			= 0x4000,						/*  the volume mounting information record needs to be updated. */
	volMountFSReservedMask		= 0x00FF,						/* bits 0-7 are defined by each file system for its own use */
	volMountSysReservedMask		= 0xFF00						/* bits 8-15 are reserved for Apple system use */
};




struct AFPVolMountInfo {
	short 							length;						/* length of location data (including self) */
	VolumeType 						media;						/* type of media */
	short 							flags;						/* bits for no messages, no reconnect */
	SInt8 							nbpInterval;				/* NBP Interval parameter (IM2, p.322) */
	SInt8 							nbpCount;					/* NBP Interval parameter (IM2, p.322) */
	short 							uamType;					/* User Authentication Method */
	short 							zoneNameOffset;				/* short positive offset from start of struct to Zone Name */
	short 							serverNameOffset;			/* offset to pascal Server Name string */
	short 							volNameOffset;				/* offset to pascal Volume Name string */
	short 							userNameOffset;				/* offset to pascal User Name string */
	short 							userPasswordOffset;			/* offset to pascal User Password string */
	short 							volPasswordOffset;			/* offset to pascal Volume Password string */
	char 							AFPData[144];				/* variable length data may follow */
};
typedef struct AFPVolMountInfo			AFPVolMountInfo;
typedef AFPVolMountInfo *				AFPVolMountInfoPtr;


/* AFPXVolMountInfo is the new AFP volume mount info record, requires the 3.7 AppleShare Client */

struct AFPXVolMountInfo {
	short 							length;						/* length of location data (including self) */
	VolumeType 						media;						/* type of media */
	short 							flags;						/* bits for no messages, no reconnect */
	SInt8 							nbpInterval;				/* NBP Interval parameter (IM2, p.322) */
	SInt8 							nbpCount;					/* NBP Interval parameter (IM2, p.322) */
	short 							uamType;					/* User Authentication Method type */
	short 							zoneNameOffset;				/* short positive offset from start of struct to Zone Name */
	short 							serverNameOffset;			/* offset to pascal Server Name string */
	short 							volNameOffset;				/* offset to pascal Volume Name string */
	short 							userNameOffset;				/* offset to pascal User Name string */
	short 							userPasswordOffset;			/* offset to pascal User Password string */
	short 							volPasswordOffset;			/* offset to pascal Volume Password string */
	short 							extendedFlags;				/* extended flags word */
	short 							uamNameOffset;				/* offset to a pascal UAM name string */
	short 							alternateAddressOffset;		/* offset to Alternate Addresses in tagged format */
	char 							AFPData[176];				/* variable length data may follow */
};
typedef struct AFPXVolMountInfo			AFPXVolMountInfo;
typedef AFPXVolMountInfo *				AFPXVolMountInfoPtr;

enum {
	kAFPExtendedFlagsAlternateAddressMask = 1					/*  bit in AFPXVolMountInfo.extendedFlags that means alternateAddressOffset is used*/
};



enum {
																/* constants for use in AFPTagData.fType field*/
	kAFPTagTypeIP				= 0x01,
	kAFPTagTypeIPPort			= 0x02,
	kAFPTagTypeDDP				= 0x03							/* Currently unused*/
};



enum {
																/* constants for use in AFPTagData.fLength field*/
	kAFPTagLengthIP				= 0x06,
	kAFPTagLengthIPPort			= 0x08,
	kAFPTagLengthDDP			= 0x06
};


struct AFPTagData {
	UInt8 							fLength;					/* length of this data tag including the fLength field */
	UInt8 							fType;
	UInt8 							fData[1];					/* variable length data */
};
typedef struct AFPTagData				AFPTagData;

struct AFPAlternateAddress {
	UInt8 							fAddressCount;
	UInt8 							fAddressList[1];			/* actually variable length packed set of AFPTagData */
};
typedef struct AFPAlternateAddress		AFPAlternateAddress;



struct DTPBRec {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioDTRefNum;					/* desktop refnum */
	short 							ioIndex;
	long 							ioTagInfo;
	Ptr 							ioDTBuffer;
	long 							ioDTReqCount;
	long 							ioDTActCount;
	SInt8 							ioFiller1;
	SInt8 							ioIconType;
	short 							ioFiller2;
	long 							ioDirID;
	OSType 							ioFileCreator;
	OSType 							ioFileType;
	long 							ioFiller3;
	long 							ioDTLgLen;
	long 							ioDTPyLen;
	short 							ioFiller4[14];
	long 							ioAPPLParID;
};
typedef struct DTPBRec					DTPBRec;

typedef DTPBRec *						DTPBPtr;

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
typedef struct HIOParam					HIOParam;
typedef HIOParam *						HIOParamPtr;

struct HFileParam {
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
	SInt8 							ioFlVersNum;
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
};
typedef struct HFileParam				HFileParam;
typedef HFileParam *					HFileParamPtr;

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
typedef struct HVolumeParam				HVolumeParam;
typedef HVolumeParam *					HVolumeParamPtr;

enum {
																/* Large Volume Constants */
	kWidePosOffsetBit			= 8,
	kUseWidePositioning			= (1 << kWidePosOffsetBit),
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
typedef struct XIOParam					XIOParam;
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
typedef struct XVolumeParam				XVolumeParam;
typedef XVolumeParam *					XVolumeParamPtr;

struct AccessParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							filler3;
	short 							ioDenyModes;				/*access rights data*/
	short 							filler4;
	SInt8 							filler5;
	SInt8 							ioACUser;					/*access rights for directory only*/
	long 							filler6;
	long 							ioACOwnerID;				/*owner ID*/
	long 							ioACGroupID;				/*group ID*/
	long 							ioACAccess;					/*access rights*/
	long 							ioDirID;
};
typedef struct AccessParam				AccessParam;
typedef AccessParam *					AccessParamPtr;

struct ObjParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							filler7;
	short 							ioObjType;					/*function code*/
	StringPtr 						ioObjNamePtr;				/*ptr to returned creator/group name*/
	long 							ioObjID;					/*creator/group ID*/
};
typedef struct ObjParam					ObjParam;
typedef ObjParam *						ObjParamPtr;

struct CopyParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							ioDstVRefNum;				/*destination vol identifier*/
	short 							filler8;
	StringPtr 						ioNewName;					/*ptr to destination pathname*/
	StringPtr 						ioCopyName;					/*ptr to optional name*/
	long 							ioNewDirID;					/*destination directory ID*/
	long 							filler14;
	long 							filler15;
	long 							ioDirID;
};
typedef struct CopyParam				CopyParam;
typedef CopyParam *						CopyParamPtr;

struct WDParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	short 							filler9;
	short 							ioWDIndex;
	long 							ioWDProcID;
	short 							ioWDVRefNum;
	short 							filler10;
	long 							filler11;
	long 							filler12;
	long 							filler13;
	long 							ioWDDirID;
};
typedef struct WDParam					WDParam;
typedef WDParam *						WDParamPtr;

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
typedef struct FIDParam					FIDParam;
typedef FIDParam *						FIDParamPtr;

struct ForeignPrivParam {
	QElemPtr 						qLink;						/*queue link in header*/
	short 							qType;						/*type byte for safety check*/
	short 							ioTrap;						/*FS: the Trap*/
	Ptr 							ioCmdAddr;					/*FS: address to dispatch to*/
	IOCompletionUPP 				ioCompletion;				/*completion routine addr (0 for synch calls)*/
	OSErr 							ioResult;					/*result code*/
	StringPtr 						ioNamePtr;					/*ptr to Vol:FileName string*/
	short 							ioVRefNum;					/*volume refnum (DrvNum for Eject and MountVol)*/
	long 							ioFiller21;
	long 							ioFiller22;
	Ptr 							ioForeignPrivBuffer;
	long 							ioForeignPrivActCount;
	long 							ioForeignPrivReqCount;
	long 							ioFiller23;
	long 							ioForeignPrivDirID;
	long 							ioForeignPrivInfo1;
	long 							ioForeignPrivInfo2;
	long 							ioForeignPrivInfo3;
	long 							ioForeignPrivInfo4;
};
typedef struct ForeignPrivParam			ForeignPrivParam;
typedef ForeignPrivParam *				ForeignPrivParamPtr;

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
typedef struct CSParam					CSParam;
typedef CSParam *						CSParamPtr;



union HParamBlockRec {
	HIOParam 						ioParam;
	HFileParam 						fileParam;
	HVolumeParam 					volumeParam;
	AccessParam 					accessParam;
	ObjParam 						objParam;
	CopyParam 						copyParam;
	WDParam 						wdParam;
	FIDParam 						fidParam;
	CSParam 						csParam;
	ForeignPrivParam 				foreignPrivParam;
};
typedef union HParamBlockRec			HParamBlockRec;

typedef HParamBlockRec *				HParmBlkPtr;


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
typedef struct CMovePBRec				CMovePBRec;

typedef CMovePBRec *					CMovePBPtr;

struct WDPBRec {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	short 							filler1;
	short 							ioWDIndex;
	long 							ioWDProcID;
	short 							ioWDVRefNum;
	short 							filler2[7];
	long 							ioWDDirID;
};
typedef struct WDPBRec					WDPBRec;

typedef WDPBRec *						WDPBPtr;

struct FCBPBRec {
	QElemPtr 						qLink;
	short 							qType;
	short 							ioTrap;
	Ptr 							ioCmdAddr;
	IOCompletionUPP 				ioCompletion;
	OSErr 							ioResult;
	StringPtr 						ioNamePtr;
	short 							ioVRefNum;
	short 							ioRefNum;
	short 							filler;
	short 							ioFCBIndx;
	short 							filler1;
	long 							ioFCBFlNm;
	short 							ioFCBFlags;
	unsigned short 					ioFCBStBlk;
	long 							ioFCBEOF;
	long 							ioFCBPLen;
	long 							ioFCBCrPs;
	short 							ioFCBVRefNum;
	long 							ioFCBClpSiz;
	long 							ioFCBParID;
};
typedef struct FCBPBRec					FCBPBRec;

typedef FCBPBRec *						FCBPBPtr;

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
typedef struct VCB						VCB;
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
typedef struct DrvQEl					DrvQEl;
typedef DrvQEl *						DrvQElPtr;
enum { uppIOCompletionProcInfo = 0x00009802 }; 					/* register no_return_value Func(4_bytes:A0) */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(IOCompletionUPP)
NewIOCompletionProc			   (IOCompletionProcPtr		userRoutine);
EXTERN_API(void)
CallIOCompletionProc		   (IOCompletionUPP			userRoutine,
								ParmBlkPtr				paramBlock);
#else
#define NewIOCompletionProc(userRoutine) 						(IOCompletionUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppIOCompletionProcInfo, GetCurrentArchitecture())
#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	#pragma parameter CallIOCompletionProc(__A1, __A0)
	void CallIOCompletionProc(IOCompletionUPP routine, ParmBlkPtr paramBlock) = 0x4E91;
#else
	#define CallIOCompletionProc(userRoutine, paramBlock) 		CALL_ONE_PARAMETER_UPP((userRoutine), uppIOCompletionProcInfo, (paramBlock))
#endif
#endif
#if OLDROUTINELOCATIONS
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA000);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenAsync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA400);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenImmed(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenImmed(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA200);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCloseSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCloseSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA001);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCloseAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCloseAsync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA401);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCloseImmed(__A0)
																							#endif
EXTERN_API( OSErr ) PBCloseImmed(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA201);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBReadSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBReadSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA002);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBReadAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBReadAsync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA402);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBReadImmed(__A0)
																							#endif
EXTERN_API( OSErr ) PBReadImmed(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA202);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBWriteSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBWriteSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA003);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBWriteAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBWriteAsync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA403);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBWriteImmed(__A0)
																							#endif
EXTERN_API( OSErr ) PBWriteImmed(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA203);

#endif  /* OLDROUTINELOCATIONS */

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVInfoSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA007);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVInfoAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA407);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBXGetVolInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBXGetVolInfoSync(XVolumeParamPtr paramBlock)							TWOWORDINLINE(0x7012, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBXGetVolInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBXGetVolInfoAsync(XVolumeParamPtr paramBlock)							TWOWORDINLINE(0x7012, 0xA460);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVolSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVolSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA014);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVolAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVolAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA414);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetVolSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetVolSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA015);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetVolAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetVolAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA415);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBFlushVolSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBFlushVolSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA013);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBFlushVolAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBFlushVolAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA413);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHTrashVolumeCachesSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHTrashVolumeCachesSync(ParmBlkPtr paramBlock)							ONEWORDINLINE(0xA213);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCreateSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA008);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCreateAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA408);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDeleteSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDeleteSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA009);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDeleteAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDeleteAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA409);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenDFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenDFSync(ParmBlkPtr paramBlock)										TWOWORDINLINE(0x701A, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenDFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenDFAsync(ParmBlkPtr paramBlock)									TWOWORDINLINE(0x701A, 0xA460);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenRFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenRFSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA00A);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenRFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenRFAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA40A);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBRenameSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBRenameSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA00B);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBRenameAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBRenameAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA40B);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFInfoSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA00C);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFInfoAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA40C);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFInfoSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA00D);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFInfoAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA40D);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFLockSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFLockSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA041);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFLockAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFLockAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA441);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBRstFLockSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBRstFLockSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA042);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBRstFLockAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBRstFLockAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA442);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFVersSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFVersSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA043);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFVersAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFVersAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA443);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBAllocateSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBAllocateSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA010);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBAllocateAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBAllocateAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA410);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetEOFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetEOFSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA011);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetEOFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetEOFAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA411);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetEOFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetEOFSync(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA012);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetEOFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetEOFAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA412);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFPosSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFPosSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA018);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFPosAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFPosAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA418);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFPosSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFPosSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA044);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetFPosAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetFPosAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA444);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBFlushFileSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBFlushFileSync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA045);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBFlushFileAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBFlushFileAsync(ParmBlkPtr paramBlock)									ONEWORDINLINE(0xA445);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBMountVol(__A0)
																							#endif
EXTERN_API( OSErr ) PBMountVol(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA00F);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnmountVol(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnmountVol(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA00E);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnmountVolImmed(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnmountVolImmed(ParmBlkPtr paramBlock)								ONEWORDINLINE(0xA20E);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBEject(__A0)
																							#endif
EXTERN_API( OSErr ) PBEject(ParmBlkPtr paramBlock)											ONEWORDINLINE(0xA017);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOffLine(__A0)
																							#endif
EXTERN_API( OSErr ) PBOffLine(ParmBlkPtr paramBlock)										ONEWORDINLINE(0xA035);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCatSearchSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCatSearchSync(CSParamPtr paramBlock)									TWOWORDINLINE(0x7018, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCatSearchAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCatSearchAsync(CSParamPtr paramBlock)									TWOWORDINLINE(0x7018, 0xA660);

EXTERN_API( OSErr )
SetVol							(ConstStr63Param 		volName,
								 short 					vRefNum);

EXTERN_API( OSErr )
UnmountVol						(ConstStr63Param 		volName,
								 short 					vRefNum);

EXTERN_API( OSErr )
Eject							(ConstStr63Param 		volName,
								 short 					vRefNum);

EXTERN_API( OSErr )
FlushVol						(ConstStr63Param 		volName,
								 short 					vRefNum);

EXTERN_API( OSErr )
HSetVol							(ConstStr63Param 		volName,
								 short 					vRefNum,
								 long 					dirID);

#if OLDROUTINELOCATIONS
EXTERN_API( void )
AddDrive						(short 					drvrRefNum,
								 short 					drvNum,
								 DrvQElPtr 				qEl);

#endif  /* OLDROUTINELOCATIONS */

EXTERN_API( OSErr )
FSOpen							(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 short *				refNum);

EXTERN_API( OSErr )
OpenDF							(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 short *				refNum);

EXTERN_API( OSErr )
FSClose							(short 					refNum);

EXTERN_API( OSErr )
FSRead							(short 					refNum,
								 long *					count,
								 void *					buffPtr);

EXTERN_API( OSErr )
FSWrite							(short 					refNum,
								 long *					count,
								 const void *			buffPtr);

EXTERN_API( OSErr )
GetVInfo						(short 					drvNum,
								 StringPtr 				volName,
								 short *				vRefNum,
								 long *					freeBytes);

EXTERN_API( OSErr )
GetFInfo						(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 FInfo *				fndrInfo);

EXTERN_API( OSErr )
GetVol							(StringPtr 				volName,
								 short *				vRefNum);

EXTERN_API( OSErr )
Create							(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 OSType 				creator,
								 OSType 				fileType);

EXTERN_API( OSErr )
FSDelete						(ConstStr255Param 		fileName,
								 short 					vRefNum);

EXTERN_API( OSErr )
OpenRF							(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 short *				refNum);

EXTERN_API( OSErr )
Rename							(ConstStr255Param 		oldName,
								 short 					vRefNum,
								 ConstStr255Param 		newName);

EXTERN_API( OSErr )
SetFInfo						(ConstStr255Param 		fileName,
								 short 					vRefNum,
								 const FInfo *			fndrInfo);

EXTERN_API( OSErr )
SetFLock						(ConstStr255Param 		fileName,
								 short 					vRefNum);

EXTERN_API( OSErr )
RstFLock						(ConstStr255Param 		fileName,
								 short 					vRefNum);

EXTERN_API( OSErr )
Allocate						(short 					refNum,
								 long *					count);

EXTERN_API( OSErr )
GetEOF							(short 					refNum,
								 long *					logEOF);

EXTERN_API( OSErr )
SetEOF							(short 					refNum,
								 long 					logEOF);

EXTERN_API( OSErr )
GetFPos							(short 					refNum,
								 long *					filePos);

EXTERN_API( OSErr )
SetFPos							(short 					refNum,
								 short 					posMode,
								 long 					posOff);

EXTERN_API( OSErr )
GetVRefNum						(short 					fileRefNum,
								 short *				vRefNum);

#if CGLUESUPPORTED
EXTERN_API_C( OSErr )
fsopen							(const char *			fileName,
								 short 					vRefNum,
								 short *				refNum);

EXTERN_API_C( OSErr )
getvinfo						(short 					drvNum,
								 char *					volName,
								 short *				vRefNum,
								 long *					freeBytes);

EXTERN_API_C( OSErr )
getfinfo						(const char *			fileName,
								 short 					vRefNum,
								 FInfo *				fndrInfo);

EXTERN_API_C( OSErr )
getvol							(char *					volName,
								 short *				vRefNum);

EXTERN_API_C( OSErr )
setvol							(const char *			volName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
unmountvol						(const char *			volName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
eject							(const char *			volName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
flushvol						(const char *			volName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
create							(const char *			fileName,
								 short 					vRefNum,
								 OSType 				creator,
								 OSType 				fileType);

EXTERN_API_C( OSErr )
fsdelete						(const char *			fileName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
openrf							(const char *			fileName,
								 short 					vRefNum,
								 short *				refNum);

EXTERN_API_C( OSErr )
fsrename						(const char *			oldName,
								 short 					vRefNum,
								 const char *			newName);

EXTERN_API_C( OSErr )
setfinfo						(const char *			fileName,
								 short 					vRefNum,
								 const FInfo *			fndrInfo);

EXTERN_API_C( OSErr )
setflock						(const char *			fileName,
								 short 					vRefNum);

EXTERN_API_C( OSErr )
rstflock						(const char *			fileName,
								 short 					vRefNum);

#endif  /* CGLUESUPPORTED */

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenWDSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenWDSync(WDPBPtr paramBlock)										TWOWORDINLINE(0x7001, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBOpenWDAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBOpenWDAsync(WDPBPtr paramBlock)										TWOWORDINLINE(0x7001, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCloseWDSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCloseWDSync(WDPBPtr paramBlock)										TWOWORDINLINE(0x7002, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCloseWDAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCloseWDAsync(WDPBPtr paramBlock)										TWOWORDINLINE(0x7002, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetVolSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetVolSync(WDPBPtr paramBlock)										ONEWORDINLINE(0xA215);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetVolAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetVolAsync(WDPBPtr paramBlock)										ONEWORDINLINE(0xA615);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVolSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVolSync(WDPBPtr paramBlock)										ONEWORDINLINE(0xA214);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVolAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVolAsync(WDPBPtr paramBlock)										ONEWORDINLINE(0xA614);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCatMoveSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCatMoveSync(CMovePBPtr paramBlock)									TWOWORDINLINE(0x7005, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCatMoveAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCatMoveAsync(CMovePBPtr paramBlock)									TWOWORDINLINE(0x7005, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDirCreateSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDirCreateSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7006, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDirCreateAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDirCreateAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7006, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetWDInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetWDInfoSync(WDPBPtr paramBlock)										TWOWORDINLINE(0x7007, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetWDInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetWDInfoAsync(WDPBPtr paramBlock)									TWOWORDINLINE(0x7007, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFCBInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFCBInfoSync(FCBPBPtr paramBlock)									TWOWORDINLINE(0x7008, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetFCBInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetFCBInfoAsync(FCBPBPtr paramBlock)									TWOWORDINLINE(0x7008, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetCatInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetCatInfoSync(CInfoPBPtr paramBlock)									TWOWORDINLINE(0x7009, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetCatInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetCatInfoAsync(CInfoPBPtr paramBlock)								TWOWORDINLINE(0x7009, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetCatInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetCatInfoSync(CInfoPBPtr paramBlock)									TWOWORDINLINE(0x700A, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetCatInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetCatInfoAsync(CInfoPBPtr paramBlock)								TWOWORDINLINE(0x700A, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBAllocContigSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBAllocContigSync(ParmBlkPtr paramBlock)								ONEWORDINLINE(0xA210);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBAllocContigAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBAllocContigAsync(ParmBlkPtr paramBlock)								ONEWORDINLINE(0xA610);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBLockRangeSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBLockRangeSync(ParmBlkPtr paramBlock)									TWOWORDINLINE(0x7010, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBLockRangeAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBLockRangeAsync(ParmBlkPtr paramBlock)									TWOWORDINLINE(0x7010, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnlockRangeSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnlockRangeSync(ParmBlkPtr paramBlock)								TWOWORDINLINE(0x7011, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnlockRangeAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnlockRangeAsync(ParmBlkPtr paramBlock)								TWOWORDINLINE(0x7011, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetVInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetVInfoSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x700B, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetVInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetVInfoAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x700B, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVInfoSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA207);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVInfoAsync(HParmBlkPtr paramBlock)								ONEWORDINLINE(0xA607);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenSync(HParmBlkPtr paramBlock)										ONEWORDINLINE(0xA200);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenAsync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA600);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenRFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenRFSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA20A);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenRFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenRFAsync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA60A);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenDFSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenDFSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x701A, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenDFAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenDFAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x701A, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHCreateSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHCreateSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA208);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHCreateAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHCreateAsync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA608);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHDeleteSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHDeleteSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA209);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHDeleteAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHDeleteAsync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA609);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHRenameSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHRenameSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA20B);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHRenameAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHRenameAsync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA60B);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHRstFLockSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHRstFLockSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA242);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHRstFLockAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHRstFLockAsync(HParmBlkPtr paramBlock)								ONEWORDINLINE(0xA642);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetFLockSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetFLockSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA241);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetFLockAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetFLockAsync(HParmBlkPtr paramBlock)								ONEWORDINLINE(0xA641);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetFInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetFInfoSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA20C);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetFInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetFInfoAsync(HParmBlkPtr paramBlock)								ONEWORDINLINE(0xA60C);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetFInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetFInfoSync(HParmBlkPtr paramBlock)									ONEWORDINLINE(0xA20D);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetFInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetFInfoAsync(HParmBlkPtr paramBlock)								ONEWORDINLINE(0xA60D);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBMakeFSSpecSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBMakeFSSpecSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x701B, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBMakeFSSpecAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBMakeFSSpecAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x701B, 0xA660);

EXTERN_API( void ) FInitQueue(void )																ONEWORDINLINE(0xA016);


EXTERN_API( QHdrPtr )
GetFSQHdr						(void)														THREEWORDINLINE(0x2EBC, 0x0000, 0x0360);

EXTERN_API( QHdrPtr )
GetVCBQHdr						(void)														THREEWORDINLINE(0x2EBC, 0x0000, 0x0356);

#if OLDROUTINELOCATIONS
EXTERN_API( QHdrPtr )
GetDrvQHdr						(void)														THREEWORDINLINE(0x2EBC, 0x0000, 0x0308);

#endif  /* OLDROUTINELOCATIONS */

EXTERN_API( OSErr )
HGetVol							(StringPtr 				volName,
								 short *				vRefNum,
								 long *					dirID);

EXTERN_API( OSErr )
HOpen							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 SInt8 					permission,
								 short *				refNum);

EXTERN_API( OSErr )
HOpenDF							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 SInt8 					permission,
								 short *				refNum);

EXTERN_API( OSErr )
HOpenRF							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 SInt8 					permission,
								 short *				refNum);

EXTERN_API( OSErr )
AllocContig						(short 					refNum,
								 long *					count);

EXTERN_API( OSErr )
HCreate							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 OSType 				creator,
								 OSType 				fileType);

EXTERN_API( OSErr )
DirCreate						(short 					vRefNum,
								 long 					parentDirID,
								 ConstStr255Param 		directoryName,
								 long *					createdDirID);

EXTERN_API( OSErr )
HDelete							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName);

EXTERN_API( OSErr )
HGetFInfo						(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 FInfo *				fndrInfo);

EXTERN_API( OSErr )
HSetFInfo						(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 const FInfo *			fndrInfo);

EXTERN_API( OSErr )
HSetFLock						(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName);

EXTERN_API( OSErr )
HRstFLock						(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName);

EXTERN_API( OSErr )
HRename							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		oldName,
								 ConstStr255Param 		newName);

EXTERN_API( OSErr )
CatMove							(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		oldName,
								 long 					newDirID,
								 ConstStr255Param 		newName);

EXTERN_API( OSErr )
OpenWD							(short 					vRefNum,
								 long 					dirID,
								 long 					procID,
								 short *				wdRefNum);

EXTERN_API( OSErr )
CloseWD							(short 					wdRefNum);

EXTERN_API( OSErr )
GetWDInfo						(short 					wdRefNum,
								 short *				vRefNum,
								 long *					dirID,
								 long *					procID);

/*  shared environment  */
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVolParmsSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVolParmsSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7030, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetVolParmsAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetVolParmsAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7030, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetLogInInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetLogInInfoSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7031, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetLogInInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetLogInInfoAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7031, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetDirAccessSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetDirAccessSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7032, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHGetDirAccessAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHGetDirAccessAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7032, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetDirAccessSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetDirAccessSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7033, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHSetDirAccessAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHSetDirAccessAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7033, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMapIDSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMapIDSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7034, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMapIDAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMapIDAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7034, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMapNameSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMapNameSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7035, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMapNameAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMapNameAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7035, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHCopyFileSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHCopyFileSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7036, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHCopyFileAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHCopyFileAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7036, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMoveRenameSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMoveRenameSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7037, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHMoveRenameAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHMoveRenameAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7037, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenDenySync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenDenySync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7038, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenDenyAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenDenyAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7038, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenRFDenySync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenRFDenySync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7039, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBHOpenRFDenyAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBHOpenRFDenyAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7039, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetXCatInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetXCatInfoSync(XCInfoPBPtr paramBlock)								TWOWORDINLINE(0x703A, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetXCatInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetXCatInfoAsync(XCInfoPBPtr paramBlock)								TWOWORDINLINE(0x703A, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBExchangeFilesSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBExchangeFilesSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7017, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBExchangeFilesAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBExchangeFilesAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7017, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateFileIDRefSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCreateFileIDRefSync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7014, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBCreateFileIDRefAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBCreateFileIDRefAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7014, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBResolveFileIDRefSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBResolveFileIDRefSync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7016, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBResolveFileIDRefAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBResolveFileIDRefAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7016, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDeleteFileIDRefSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDeleteFileIDRefSync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7015, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDeleteFileIDRefAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDeleteFileIDRefAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7015, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetForeignPrivsSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetForeignPrivsSync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7060, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetForeignPrivsAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetForeignPrivsAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7060, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetForeignPrivsSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetForeignPrivsSync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7061, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetForeignPrivsAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetForeignPrivsAsync(HParmBlkPtr paramBlock)							TWOWORDINLINE(0x7061, 0xA660);

/*  Desktop Manager  */
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetPath(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetPath(DTPBPtr paramBlock)											TWOWORDINLINE(0x7020, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTCloseDown(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTCloseDown(DTPBPtr paramBlock)										TWOWORDINLINE(0x7021, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTAddIconSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTAddIconSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x7022, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTAddIconAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTAddIconAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7022, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetIconSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetIconSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x7023, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetIconAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetIconAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7023, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetIconInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetIconInfoSync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7024, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetIconInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetIconInfoAsync(DTPBPtr paramBlock)								TWOWORDINLINE(0x7024, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTAddAPPLSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTAddAPPLSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x7025, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTAddAPPLAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTAddAPPLAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7025, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTRemoveAPPLSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTRemoveAPPLSync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7026, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTRemoveAPPLAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTRemoveAPPLAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7026, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetAPPLSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetAPPLSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x7027, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetAPPLAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetAPPLAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7027, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTSetCommentSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTSetCommentSync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7028, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTSetCommentAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTSetCommentAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x7028, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTRemoveCommentSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTRemoveCommentSync(DTPBPtr paramBlock)								TWOWORDINLINE(0x7029, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTRemoveCommentAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTRemoveCommentAsync(DTPBPtr paramBlock)								TWOWORDINLINE(0x7029, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetCommentSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetCommentSync(DTPBPtr paramBlock)									TWOWORDINLINE(0x702A, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetCommentAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetCommentAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x702A, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTFlushSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTFlushSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702B, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTFlushAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTFlushAsync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702B, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTResetSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTResetSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702C, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTResetAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTResetAsync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702C, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetInfoSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetInfoSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702D, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTGetInfoAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTGetInfoAsync(DTPBPtr paramBlock)									TWOWORDINLINE(0x702D, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTOpenInform(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTOpenInform(DTPBPtr paramBlock)										TWOWORDINLINE(0x702E, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTDeleteSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTDeleteSync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702F, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBDTDeleteAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBDTDeleteAsync(DTPBPtr paramBlock)										TWOWORDINLINE(0x702F, 0xA460);

/*  VolumeMount traps  */
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVolMountInfoSize(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVolMountInfoSize(ParmBlkPtr paramBlock)							TWOWORDINLINE(0x703F, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetVolMountInfo(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetVolMountInfo(ParmBlkPtr paramBlock)								TWOWORDINLINE(0x7040, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBVolumeMount(__A0)
																							#endif
EXTERN_API( OSErr ) PBVolumeMount(ParmBlkPtr paramBlock)									TWOWORDINLINE(0x7041, 0xA260);

/*  FSp traps  */
EXTERN_API( OSErr )
FSMakeFSSpec					(short 					vRefNum,
								 long 					dirID,
								 ConstStr255Param 		fileName,
								 FSSpec *				spec)								TWOWORDINLINE(0x7001, 0xAA52);

EXTERN_API( OSErr )
FSpOpenDF						(const FSSpec *			spec,
								 SInt8 					permission,
								 short *				refNum)								TWOWORDINLINE(0x7002, 0xAA52);

EXTERN_API( OSErr )
FSpOpenRF						(const FSSpec *			spec,
								 SInt8 					permission,
								 short *				refNum)								TWOWORDINLINE(0x7003, 0xAA52);

EXTERN_API( OSErr )
FSpCreate						(const FSSpec *			spec,
								 OSType 				creator,
								 OSType 				fileType,
								 ScriptCode 			scriptTag)							TWOWORDINLINE(0x7004, 0xAA52);

EXTERN_API( OSErr )
FSpDirCreate					(const FSSpec *			spec,
								 ScriptCode 			scriptTag,
								 long *					createdDirID)						TWOWORDINLINE(0x7005, 0xAA52);

EXTERN_API( OSErr )
FSpDelete						(const FSSpec *			spec)								TWOWORDINLINE(0x7006, 0xAA52);

EXTERN_API( OSErr )
FSpGetFInfo						(const FSSpec *			spec,
								 FInfo *				fndrInfo)							TWOWORDINLINE(0x7007, 0xAA52);

EXTERN_API( OSErr )
FSpSetFInfo						(const FSSpec *			spec,
								 const FInfo *			fndrInfo)							TWOWORDINLINE(0x7008, 0xAA52);

EXTERN_API( OSErr )
FSpSetFLock						(const FSSpec *			spec)								TWOWORDINLINE(0x7009, 0xAA52);

EXTERN_API( OSErr )
FSpRstFLock						(const FSSpec *			spec)								TWOWORDINLINE(0x700A, 0xAA52);

EXTERN_API( OSErr )
FSpRename						(const FSSpec *			spec,
								 ConstStr255Param 		newName)							TWOWORDINLINE(0x700B, 0xAA52);

EXTERN_API( OSErr )
FSpCatMove						(const FSSpec *			source,
								 const FSSpec *			dest)								TWOWORDINLINE(0x700C, 0xAA52);

EXTERN_API( OSErr )
FSpExchangeFiles				(const FSSpec *			source,
								 const FSSpec *			dest)								TWOWORDINLINE(0x700F, 0xAA52);


																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBShareSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBShareSync(HParmBlkPtr paramBlock)										TWOWORDINLINE(0x7042, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBShareAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBShareAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7042, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnshareSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnshareSync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7043, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBUnshareAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBUnshareAsync(HParmBlkPtr paramBlock)									TWOWORDINLINE(0x7043, 0xA660);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetUGEntrySync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetUGEntrySync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7044, 0xA260);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetUGEntryAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetUGEntryAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7044, 0xA660);




#if TARGET_CPU_68K
/*
	PBGetAltAccess and PBSetAltAccess are obsolete and will not be supported 
	on PowerPC. Equivalent functionality is provided by the routines 
	PBGetForeignPrivs and PBSetForeignPrivs.
*/
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetAltAccessSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetAltAccessSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7060, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBGetAltAccessAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBGetAltAccessAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7060, 0xA460);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetAltAccessSync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetAltAccessSync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7061, 0xA060);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PBSetAltAccessAsync(__A0)
																							#endif
EXTERN_API( OSErr ) PBSetAltAccessAsync(HParmBlkPtr paramBlock)								TWOWORDINLINE(0x7061, 0xA460);

#define PBSetAltAccess(pb, async) ((async) ? PBSetAltAccessAsync(pb) : PBSetAltAccessSync(pb))
#define PBGetAltAccess(pb, async) ((async) ? PBGetAltAccessAsync(pb) : PBGetAltAccessSync(pb))
#endif  /* TARGET_CPU_68K */


/*
	The PBxxx() routines are obsolete.  
	
	Use the PBxxxSync() or PBxxxAsync() version instead.
*/
#define PBGetVInfo(pb, async) ((async) ? PBGetVInfoAsync(pb) : PBGetVInfoSync(pb))
#define PBXGetVolInfo(pb, async) ((async) ? PBXGetVolInfoAsync(pb) : PBXGetVolInfoSync(pb))
#define PBGetVol(pb, async) ((async) ? PBGetVolAsync(pb) : PBGetVolSync(pb))
#define PBSetVol(pb, async) ((async) ? PBSetVolAsync(pb) : PBSetVolSync(pb))
#define PBFlushVol(pb, async) ((async) ? PBFlushVolAsync(pb) : PBFlushVolSync(pb))
#define PBCreate(pb, async) ((async) ? PBCreateAsync(pb) : PBCreateSync(pb))
#define PBDelete(pb, async) ((async) ? PBDeleteAsync(pb) : PBDeleteSync(pb))
#define PBOpenDF(pb, async) ((async) ? PBOpenDFAsync(pb) : PBOpenDFSync(pb))
#define PBOpenRF(pb, async) ((async) ? PBOpenRFAsync(pb) : PBOpenRFSync(pb))
#define PBRename(pb, async) ((async) ? PBRenameAsync(pb) : PBRenameSync(pb))
#define PBGetFInfo(pb, async) ((async) ? PBGetFInfoAsync(pb) : PBGetFInfoSync(pb))
#define PBSetFInfo(pb, async) ((async) ? PBSetFInfoAsync(pb) : PBSetFInfoSync(pb))
#define PBSetFLock(pb, async) ((async) ? PBSetFLockAsync(pb) : PBSetFLockSync(pb))
#define PBRstFLock(pb, async) ((async) ? PBRstFLockAsync(pb) : PBRstFLockSync(pb))
#define PBSetFVers(pb, async) ((async) ? PBSetFVersAsync(pb) : PBSetFVersSync(pb))
#define PBAllocate(pb, async) ((async) ? PBAllocateAsync(pb) : PBAllocateSync(pb))
#define PBGetEOF(pb, async) ((async) ? PBGetEOFAsync(pb) : PBGetEOFSync(pb))
#define PBSetEOF(pb, async) ((async) ? PBSetEOFAsync(pb) : PBSetEOFSync(pb))
#define PBGetFPos(pb, async) ((async) ? PBGetFPosAsync(pb) : PBGetFPosSync(pb))
#define PBSetFPos(pb, async) ((async) ? PBSetFPosAsync(pb) : PBSetFPosSync(pb))
#define PBFlushFile(pb, async) ((async) ? PBFlushFileAsync(pb) : PBFlushFileSync(pb))
#define PBCatSearch(pb, async) ((async) ? PBCatSearchAsync(pb) : PBCatSearchSync(pb))
#define PBOpenWD(pb, async) ((async) ? PBOpenWDAsync(pb) : PBOpenWDSync(pb))
#define PBCloseWD(pb, async) ((async) ? PBCloseWDAsync(pb) : PBCloseWDSync(pb))
#define PBHSetVol(pb, async) ((async) ? PBHSetVolAsync(pb) : PBHSetVolSync(pb))
#define PBHGetVol(pb, async) ((async) ? PBHGetVolAsync(pb) : PBHGetVolSync(pb))
#define PBCatMove(pb, async) ((async) ? PBCatMoveAsync(pb) : PBCatMoveSync(pb))
#define PBDirCreate(pb, async) ((async) ? PBDirCreateAsync(pb) : PBDirCreateSync(pb))
#define PBGetWDInfo(pb, async) ((async) ? PBGetWDInfoAsync(pb) : PBGetWDInfoSync(pb))
#define PBGetFCBInfo(pb, async) ((async) ? PBGetFCBInfoAsync(pb) : PBGetFCBInfoSync(pb))
#define PBGetCatInfo(pb, async) ((async) ? PBGetCatInfoAsync(pb) : PBGetCatInfoSync(pb))
#define PBSetCatInfo(pb, async) ((async) ? PBSetCatInfoAsync(pb) : PBSetCatInfoSync(pb))
#define PBAllocContig(pb, async) ((async) ? PBAllocContigAsync(pb) : PBAllocContigSync(pb))
#define PBLockRange(pb, async) ((async) ? PBLockRangeAsync(pb) : PBLockRangeSync(pb))
#define PBUnlockRange(pb, async) ((async) ? PBUnlockRangeAsync(pb) : PBUnlockRangeSync(pb))
#define PBSetVInfo(pb, async) ((async) ? PBSetVInfoAsync(pb) : PBSetVInfoSync(pb))
#define PBHGetVInfo(pb, async) ((async) ? PBHGetVInfoAsync(pb) : PBHGetVInfoSync(pb))
#define PBHOpen(pb, async) ((async) ? PBHOpenAsync(pb) : PBHOpenSync(pb))
#define PBHOpenRF(pb, async) ((async) ? PBHOpenRFAsync(pb) : PBHOpenRFSync(pb))
#define PBHOpenDF(pb, async) ((async) ? PBHOpenDFAsync(pb) : PBHOpenDFSync(pb))
#define PBHCreate(pb, async) ((async) ? PBHCreateAsync(pb) : PBHCreateSync(pb))
#define PBHDelete(pb, async) ((async) ? PBHDeleteAsync(pb) : PBHDeleteSync(pb))
#define PBHRename(pb, async) ((async) ? PBHRenameAsync(pb) : PBHRenameSync(pb))
#define PBHRstFLock(pb, async) ((async) ? PBHRstFLockAsync(pb) : PBHRstFLockSync(pb))
#define PBHSetFLock(pb, async) ((async) ? PBHSetFLockAsync(pb) : PBHSetFLockSync(pb))
#define PBHGetFInfo(pb, async) ((async) ? PBHGetFInfoAsync(pb) : PBHGetFInfoSync(pb))
#define PBHSetFInfo(pb, async) ((async) ? PBHSetFInfoAsync(pb) : PBHSetFInfoSync(pb))
#define PBMakeFSSpec(pb, async) ((async) ? PBMakeFSSpecAsync(pb) : PBMakeFSSpecSync(pb))
#define PBHGetVolParms(pb, async) ((async) ? PBHGetVolParmsAsync(pb) : PBHGetVolParmsSync(pb))
#define PBHGetLogInInfo(pb, async) ((async) ? PBHGetLogInInfoAsync(pb) : PBHGetLogInInfoSync(pb))
#define PBHGetDirAccess(pb, async) ((async) ? PBHGetDirAccessAsync(pb) : PBHGetDirAccessSync(pb))
#define PBHSetDirAccess(pb, async) ((async) ? PBHSetDirAccessAsync(pb) : PBHSetDirAccessSync(pb))
#define PBHMapID(pb, async) ((async) ? PBHMapIDAsync(pb) : PBHMapIDSync(pb))
#define PBHMapName(pb, async) ((async) ? PBHMapNameAsync(pb) : PBHMapNameSync(pb))
#define PBHCopyFile(pb, async) ((async) ? PBHCopyFileAsync(pb) : PBHCopyFileSync(pb))
#define PBHMoveRename(pb, async) ((async) ? PBHMoveRenameAsync(pb) : PBHMoveRenameSync(pb))
#define PBHOpenDeny(pb, async) ((async) ? PBHOpenDenyAsync(pb) : PBHOpenDenySync(pb))
#define PBHOpenRFDeny(pb, async) ((async) ? PBHOpenRFDenyAsync(pb) : PBHOpenRFDenySync(pb))
#define PBExchangeFiles(pb, async) ((async) ? PBExchangeFilesAsync(pb) : PBExchangeFilesSync(pb))
#define PBCreateFileIDRef(pb, async) ((async) ? PBCreateFileIDRefAsync(pb) : PBCreateFileIDRefSync(pb))
#define PBResolveFileIDRef(pb, async) ((async) ? PBResolveFileIDRefAsync(pb) : PBResolveFileIDRefSync(pb))
#define PBDeleteFileIDRef(pb, async) ((async) ? PBDeleteFileIDRefAsync(pb) : PBDeleteFileIDRefSync(pb))
#define PBGetForeignPrivs(pb, async) ((async) ? PBGetForeignPrivsAsync(pb) : PBGetForeignPrivsSync(pb))
#define PBSetForeignPrivs(pb, async) ((async) ? PBSetForeignPrivsAsync(pb) : PBSetForeignPrivsSync(pb))
#define PBDTAddIcon(pb, async) ((async) ? PBDTAddIconAsync(pb) : PBDTAddIconSync(pb))
#define PBDTGetIcon(pb, async) ((async) ? PBDTGetIconAsync(pb) : PBDTGetIconSync(pb))
#define PBDTGetIconInfo(pb, async) ((async) ? PBDTGetIconInfoAsync(pb) : PBDTGetIconInfoSync(pb))
#define PBDTAddAPPL(pb, async) ((async) ? PBDTAddAPPLAsync(pb) : PBDTAddAPPLSync(pb))
#define PBDTRemoveAPPL(pb, async) ((async) ? PBDTRemoveAPPLAsync(pb) : PBDTRemoveAPPLSync(pb))
#define PBDTGetAPPL(pb, async) ((async) ? PBDTGetAPPLAsync(pb) : PBDTGetAPPLSync(pb))
#define PBDTSetComment(pb, async) ((async) ? PBDTSetCommentAsync(pb) : PBDTSetCommentSync(pb))
#define PBDTRemoveComment(pb, async) ((async) ? PBDTRemoveCommentAsync(pb) : PBDTRemoveCommentSync(pb))
#define PBDTGetComment(pb, async) ((async) ? PBDTGetCommentAsync(pb) : PBDTGetCommentSync(pb))
#define PBDTFlush(pb, async) ((async) ? PBDTFlushAsync(pb) : PBDTFlushSync(pb))
#define PBDTReset(pb, async) ((async) ? PBDTResetAsync(pb) : PBDTResetSync(pb))
#define PBDTGetInfo(pb, async) ((async) ? PBDTGetInfoAsync(pb) : PBDTGetInfoSync(pb))
#define PBDTDelete(pb, async) ((async) ? PBDTDeleteAsync(pb) : PBDTDeleteSync(pb))
#if OLDROUTINELOCATIONS
#define PBOpen(pb, async) ((async) ? PBOpenAsync(pb) : PBOpenSync(pb))
#define PBClose(pb, async) ((async) ? PBCloseAsync(pb) : PBCloseSync(pb))
#define PBRead(pb, async) ((async) ? PBReadAsync(pb) : PBReadSync(pb))
#define PBWrite(pb, async) ((async) ? PBWriteAsync(pb) : PBWriteSync(pb))
#endif  /* OLDROUTINELOCATIONS */


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

#endif /* __FILES__ */

