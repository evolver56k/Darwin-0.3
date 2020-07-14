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

/* Sym8xxSRB.h created by russb2 on Sat 30-May-1998 */

/*
 * The SRB is the main per-request data structure used by the driver.
 *
 * It contains an embedded Nexus structure which is used as a per-request
 * communication area between the script and the driver.
 */ 

typedef struct SRB 		SRB;

struct	SRB
{
    queue_chain_t		srbQ;

    SRB				*srbPhys;

    NXConditionLock		*srbCmdLock;

    u_int32_t			srbTimeout;

    u_int32_t			srbSeqNum;

    u_int8_t			srbCmd;
    u_int8_t			srbState;
    u_int8_t			srbRequestFlags;

    u_int8_t			srbSCSIResult;
    u_int8_t			srbSCSIStatus;

    u_int8_t			srbMsgResid;
    u_int8_t			srbMsgLength;

    u_int8_t			target;
    u_int8_t			lun;
    u_int8_t			tag;

    u_int32_t			directionMask;

    vm_task_t			xferClient;
    vm_offset_t        		xferBuffer;
    u_int32_t			xferOffset;
    u_int32_t			xferOffsetPrev;
    u_int32_t       		xferCount;
    u_int32_t			xferDone;

    vm_offset_t			senseData;
    u_int32_t			senseDataLength;

    Nexus			nexus;  

};

enum srbCmdLock
{
    ksrbCmdPending = 1,
    ksrbCmdComplete 
};

enum srbState
{
    ksrbStateCDBDone = 1,
    ksrbStateReqSenseDone,
};

enum srbQCmd
{
    ksrbCmdExecuteReq		= 0x01,
    ksrbCmdResetSCSIBus         = 0x02,
    ksrbCmdAbortReq		= 0x03,
    ksrbCmdBusDevReset		= 0x04,
    ksrbCmdProcessTimeout       = 0x05,
};

enum srbRequestFlags
{
    ksrbRFCmdQueueAllowed	= 0x01,
    ksrbRFDisconnectAllowed	= 0x02,
    ksrbRFXferSyncAllowed	= 0x04,

    ksrbRFNegotiateWide		= 0x40,
    ksrbRFNegotiateSync         = 0x80,
};


typedef struct SRBPool
{
    queue_chain_t		nextPage;
    u_int32_t			pagePhysAddr;
    queue_head_t		freeSRBList;
    u_int32_t			srbInUseCount;
    u_int32_t			reserved[2];
} SRBPool;

enum
{
    kSRBGrowPoolRunning		= 1,
    kSRBGrowPoolIdle,
};

enum
{
    kSRBPoolMaxFreePages	= 2,
};

typedef struct Target
{
    u_int32_t			flags;
    NXLock			*targetTagSem;
} Target;

enum 
{
     kTFCmdQueueSupported	= 0x00000001,
     kTFCmdQueueAllowed		= 0x00000002,

     kTFXferSyncSupported	= 0x00000004,
     kTFXferSyncAllowed		= 0x00000008,
     kTFXferSync		= 0x00000010,
  
     kTFXferWide16Supported	= 0x00000020,
     kTFXferWide16Allowed	= 0x00000040,
     kTFXferWide16		= 0x00000080,
};