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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in.
 */


#ifndef __NDRVINTERFACE__
#define __NDRVINTERFACE__

#import <driverkit/ppc/IOMacOSTypes.h>

#pragma options align=mac68k


typedef void * RegEntryID[4];

struct DriverInitInfo {
    UInt16                          refNum;
    RegEntryID                      deviceEntry;
};

#define MAKE_REG_ENTRY(regEntryID,obj) 				\
	((void **)regEntryID)[ 0 ] = (void *) obj;		\
	((void **)regEntryID)[ 1 ] = (void *) ~(UInt32)obj;	\
	((void **)regEntryID)[ 2 ] = (void *) 0x53696d65;	\
	((void **)regEntryID)[ 3 ] = (void *) 0x52756c7a;

#define REG_ENTRY_TO_ID(regEntryID,obj) 			\
	if( (UInt32)((obj = ((id *)regEntryID)[ 0 ])) 		\
	 != ~((UInt32 *)regEntryID)[ 1 ] )			\
	    return( -2538);

struct CntrlParam {
    void *                          qLink;
    short                           qType;
    short                           ioTrap;
    void *                          ioCmdAddr;
    void *                          ioCompletion;
    short                           ioResult;
    char *                          ioNamePtr;
    short                           ioVRefNum;
    short                           ioCRefNum;
    short                           csCode;
    void *                          csParams;
    short                           csParam[9];
};
typedef struct CntrlParam CntrlParam, *CntrlParamPtr;

#pragma options align=reset

enum {
    kOpenCommand                = 0,
    kCloseCommand               = 1,
    kReadCommand                = 2,
    kWriteCommand               = 3,
    kControlCommand             = 4,
    kStatusCommand              = 5,
    kKillIOCommand              = 6,
    kInitializeCommand          = 7,                            /* init driver and device*/
    kFinalizeCommand            = 8,                            /* shutdown driver and device*/
    kReplaceCommand             = 9,                            /* replace an old driver*/
    kSupersededCommand          = 10                            /* prepare to be replaced by a new driver*/
};
enum {
    kSynchronousIOCommandKind   = 0x00000001,
    kAsynchronousIOCommandKind  = 0x00000002,
    kImmediateIOCommandKind     = 0x00000004
};

typedef void * NDRVInstance;

OSStatus    NDRVLoad( LogicalAddress container, ByteCount containerSize, NDRVInstance * instance );
OSStatus    NDRVGetSymbol( NDRVInstance instance, const char * symbolName, LogicalAddress * address );
OSStatus    NDRVGetShimClass( id ioDevice, NDRVInstance instance, UInt32 serviceIndex, char * className );
OSStatus    NDRVUnload( NDRVInstance * instance );
OSStatus    NDRVTest( void * ioDevice, LogicalAddress entry );
OSStatus    NDRVDoDriverIO( LogicalAddress entry, UInt32 commandID, void * contents, UInt32 commandCode, UInt32 commandKind );

extern OSStatus CallTVectorWithStack( 
	    void * p1, void * p2, void * p3, void * p4, void * p5, void * p6,
	    LogicalAddress entry, LogicalAddress stack );

extern OSStatus    CallTVector( 
	    void * p1, void * p2, void * p3, void * p4, void * p5, void * p6,
	    LogicalAddress entry );

struct InterruptSetMember {
	id 		setID;
	UInt32	 	member;
};
typedef struct InterruptSetMember InterruptSetMember;

typedef SInt32	(*InterruptHandler)( InterruptSetMember setMember, void *refCon, UInt32 theIntCount);
typedef void    (*InterruptEnabler)( InterruptSetMember setMember, void *refCon);
typedef Boolean (*InterruptDisabler)( InterruptSetMember setMember, void *refCon);

enum {
    kFirstMemberNumber	= 1,
    kIsrIsComplete	= 0,
    kIsrIsNotComplete	= -1,
    kMemberNumberParent	= -2
};

#endif /* __NDRVINTERFACE__ */

