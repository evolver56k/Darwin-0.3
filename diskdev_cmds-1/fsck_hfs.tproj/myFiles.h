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
#ifndef __MYFILES__
#define __MYFILES__

enum {
																/* Large Volume Constants */
	kWidePosOffsetBit			= 8,
	kMaximumBlocksIn4GB			= 0x007FFFFF
};

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

struct FSSpec {
	short 							vRefNum;
	long 							parID;
	Str63 							name;
};
typedef struct FSSpec FSSpec;

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

struct ParamBlockRec {
	int dummy;
};
typedef struct ParamBlockRec ParamBlockRec;
typedef ParamBlockRec *ParmBlkPtr;

struct DrvQEl {
	int fd;
};
typedef struct DrvQEl DrvQEl;
typedef DrvQEl *DrvQElPtr;

struct VolumeParam {
	int dummy;
};
typedef struct VolumeParam VolumeParam;

struct FIDParam {
	int dummy;
};
typedef struct FIDParam FIDParam;
#endif