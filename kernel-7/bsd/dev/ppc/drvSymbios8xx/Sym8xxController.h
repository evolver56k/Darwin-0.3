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

/* Sym8xxController.h created by russb2 on Sat 30-May-1998 */

#import <bsd/sys/systm.h>
#import <bsd/include/string.h>
#import <mach/vm_param.h>
#import <machkit/NXLock.h>
#import <kernserv/ns_timer.h>
#import <kern/queue.h>
#import <driverkit/driverTypes.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/ppc/IOPCIDevice.h>

#import <driverkit/IOSimpleMemoryDescriptor.h>

#import <bsd/dev/scsireg.h>
#import <driverkit/IOSCSIController.h>

#import "Sym8xxRegs.h"
#import "Sym8xxInterface.h"
#import "Sym8xxSRB.h"

#import "Sym8xxScript.h"

@interface Sym8xxController : IOSCSIController
{
    AdapterInterface            *adapter;
    AdapterInterface		*adapterPhys;

    Target 			targets[MAX_SCSI_TARGETS];
    u_int32_t              	tags[MAX_SCSI_TAG/32];
    u_int32_t			nexusArrayVirt[MAX_SCSI_TAG];

    u_int8_t			mailBoxIndex;

    u_int32_t			initiatorID;

    u_int8_t			istatReg;
    u_int8_t			dstatReg;
    u_int16_t			sistReg;

    u_int32_t			scriptRestartAddr;

    NXLock			*srbPendingQLock;
    queue_head_t		srbPendingQ;

    NXConditionLock		*srbPoolGrowLock;
    u_int32_t			srbPoolGrow;

    NXLock			*srbPoolLock;
    queue_head_t		srbPool;
    u_int32_t			srbSeqNum;
    u_int32_t			resetSeqNum;
    
    NXLock			*resetQuiesceSem;
    u_int32_t			resetQuiesceTimer;

    NXLock			*cmdQTagSem;
    NXLock                      *abortBdrSem;

    port_t			interruptPortKern;

    SRB				*resetSRB;
    SRB				*abortSRB;
    u_int32_t			abortSRBTimeout;
    SRB				*abortCurrentSRB;
    u_int32_t			abortCurrentSRBTimeout;

    u_int32_t			chipType;
    u_int32_t			chipClockRate;

    volatile u_int8_t		*chipBaseAddr;
    u_int8_t			*chipBaseAddrPhys;
    
    volatile u_int8_t		*chipRamAddr;
    u_int8_t			*chipRamAddrPhys;

}
@end

@interface Sym8xxController(Init)
+ (BOOL)  	probe:(IOPCIDevice *)deviceDescription;
- 		initFromDeviceDescription:(IOPCIDevice *) deviceDescription;
- (BOOL)  	Sym8xxInit:(IOPCIDevice *)deviceDescription;
- (BOOL)  	Sym8xxInitPCI:(IOPCIDevice *)deviceDescription;
- (BOOL)  	Sym8xxInitVars;
- (BOOL)  	Sym8xxInitScript;
- (void)  	Sym8xxLoadScript:(u_int32_t *)scriptPgm count:(u_int32_t)scriptWords;
- (BOOL)  	Sym8xxInitChip;

@end

@interface Sym8xxController(Client)
- (sc_status_t) executeRequest:(IOSCSIRequest *)scsiReq  buffer:(void *)buffer  client:(vm_task_t)client;
- (sc_status_t) executeRequest:(IOSCSIRequest *)scsiReq  ioMemoryDescriptor: (IOMemoryDescriptor *) ioMemoryDescriptor;
- (sc_status_t) resetSCSIBus;
- (void)	getDMAAlignment:(IODMAAlignment *)alignment;
- (int) 	numberOfTargets;
- (void) 	Sym8xxCalcMsgs: (SRB *)srb;
- (void) 	Sym8xxSendCommand: (SRB *) srb;
- (u_int32_t) 	Sym8xxAllocTag:(SRB *)srb CmdQueue:(BOOL)fCmdQueue;
- (void) 	Sym8xxFreeTag:(SRB *)srb;
- (SRB *) 	Sym8xxAllocSRB;
- (void) 	Sym8xxFreeSRB:(SRB *)srb;
- (void)  	Sym8xxGrowSRBPool;
- (BOOL) 	Sym8xxUpdateSGList:(SRB *)srb;
- (BOOL) 	Sym8xxUpdateSGListVirt: (SRB *) srb;
- (BOOL) 	Sym8xxUpdateSGListDesc: (SRB *) srb;


IOThreadFunc 	Sym8xxTimerReq( Sym8xxController *device );
IOThreadFunc	Sym8xxGrowSRBPool( Sym8xxController *controller );

@end

@interface Sym8xxController(Execute)
- (void) 	commandRequestOccurred;
- (void) 	Sym8xxSignalScript:(SRB *)srb;
- (void) 	interruptOccurred;
- (void)        timeoutOccurred;
- (void) 	Sym8xxProcessIODone;
- (void) 	Sym8xxProcessInterrupt;
- (BOOL) 	Sym8xxProcessStatus:(SRB *)srb;
- (void) 	Sym8xxIssueRequestSense:(SRB *) srb;
- (void) 	Sym8xxAdjustDataPtrs:(SRB *) srb Nexus:(Nexus *) nexus;
- (u_int32_t) 	Sym8xxCheckFifo:(SRB *)srb FifoCnt:(u_int32_t *)pfifoCnt;
- (void) 	Sym8xxUpdateXferOffset:(SRB *) srb;
- (void)        Sym8xxProcessNoNexus;
- (void) 	Sym8xxAbortCurrent:(SRB *)srb;
- (void)	Sym8xxClearFifo;
- (void) 	Sym8xxNegotiateSDTR:(SRB *) srb Nexus:(Nexus *)nexus;
- (void) 	Sym8xxNegotiateWDTR:(SRB *) srb Nexus:(Nexus *)nexus;
- (void) 	Sym8xxSendMsgReject:(SRB *) srb;
- (void) 	Sym8xxCheckInquiryData: (SRB *)srb;
- (void) 	Sym8xxSCSIBusReset:(SRB *)srb;
- (void) 	Sym8xxProcessSCSIBusReset;
- (void) 	Sym8xxAbortBdr: (SRB *) srb;
- (void) 	Sym8xxAbortScript;



@end

u_int32_t Sym8xxReadRegs( volatile u_int8_t *chipRegs, u_int32_t regOffset, u_int32_t regSize );
void      Sym8xxWriteRegs( volatile u_int8_t *chipRegs, u_int32_t regOffset, u_int32_t regSize, u_int32_t regValue );

extern kern_return_t	kmem_alloc_wired();
extern kern_return_t	kmem_free();
extern kern_return_t 	msg_send_from_kernel(msg_header_t *,int,int);
//extern void 		call_kdp();

extern u_int32_t	page_size;

#if 0
extern u_int32_t	gRegs875, gRegs875Phys, gRam875, gRam875Phys;
#endif


static __inline__ u_int16_t EndianSwap16(volatile u_int16_t y)
{
    u_int16_t	 	result;
    volatile u_int16_t  x;

    x = y;   
    __asm__ volatile("lhbrx %0, 0, %1" : "=r" (result) : "r" (&x) : "r0");
    return result;
}

static __inline__ u_int32_t EndianSwap32(u_int32_t y)
{
    u_int32_t		 result;
    volatile u_int32_t   x;

    x = y;
    __asm__ volatile("lwbrx %0, 0, %1" : "=r" (result) : "r" (&x) : "r0");
    return result;
}

#ifndef eieio
#define eieio() \
        __asm__ volatile("eieio")
#endif
