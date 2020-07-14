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

/* Sym8xxExecute.m created by russb2 on Sat 30-May-1998 */

#import "Sym8xxController.h"

/*-----------------------------------------------------------------------------*
 * IOThread Routines
 *
 * This module contains routines that run on the driver's IOThread.
 *
 *-----------------------------------------------------------------------------*/

@implementation Sym8xxController(Execute)

/*-----------------------------------------------------------------------------*
 *  This is the main command processing loop for the driver's I/O thread.
 *  It removes SRBs from the driver's command queue and processes them according
 *  to the command code in srb->srbCmd.
 *
 *-----------------------------------------------------------------------------*/
- (void) commandRequestOccurred
{
    SRB				*srb = NULL;

    while ( 1 )
    {
       [srbPendingQLock lock];

       if ( queue_empty(&srbPendingQ) )
       {
          [srbPendingQLock unlock];
           break;
       }
       queue_remove_first( &srbPendingQ, srb, SRB *, srbQ );

       [srbPendingQLock unlock];

    
        /*
         * If we are in the quiet period after a SCSI Bus reset, then reject new SRBs. In
         * general the client thread processing will block new SRBs, however, some may have
         * been on the IOThread's SRB queue when the reset occurred.
         */
        if ( resetQuiesceTimer )
        {
            srb->srbSCSIResult = SR_IOST_RESET;
            [srb->srbCmdLock unlockWith: ksrbCmdComplete];  
            continue;
        }

        switch ( srb->srbCmd )
        {
            /*
             * For a SCSI CDB request, stuff the physical address of the SRB's  Nexus struct into a
             * mailbox and signal the Symbios script engine.
             */
            case ksrbCmdExecuteReq:
                srb->nexus.targetParms.scntl3Reg = adapter->targetClocks[srb->target].scntl3Reg;
                srb->nexus.targetParms.sxferReg  = adapter->targetClocks[srb->target].sxferReg;

                adapter->nexusPtrsVirt[srb->nexus.tag] = &srb->nexus;
                adapter->nexusPtrsPhys[srb->nexus.tag] = (Nexus *)EndianSwap32( (u_int32_t)&srb->srbPhys->nexus );
                adapter->schedMailBox[mailBoxIndex++]  = (Nexus *)EndianSwap32 ( (u_int32_t)&srb->srbPhys->nexus );

                [self Sym8xxSignalScript: srb];
                break;

            case ksrbCmdResetSCSIBus:
                [self Sym8xxSCSIBusReset: srb];
                break;
           
            case ksrbCmdAbortReq:
	    case ksrbCmdBusDevReset:
                [self Sym8xxAbortBdr: srb];
                break;

            default:
                ; 
        }         
    }
}


/*-----------------------------------------------------------------------------*
 * Interrupts from the Symbios chipset are dispatched here at task time under the
 * IOThread's context.
 *-----------------------------------------------------------------------------*/
- (void) interruptOccurred
{
    do
    {
        /* 
         * The chipset's ISTAT reg gives us the general interrupting condiditions,
         * with DSTAT and SIST providing more detailed information.
         */
        istatReg = Sym8xxReadRegs( chipBaseAddr, ISTAT, ISTAT_SIZE );

        /* The INTF bit in ISTAT indicates that the script is signalling the driver
         * that its IODone mailbox is full and that we should process a completed
         * request. The script continues to run after posting this interrupt unlike 
         * other chipset interrupts which require the driver to restart the script
         * engine.
         */
        if ( istatReg & INTF )
        {
            Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, istatReg );
            [self Sym8xxProcessIODone];
        }

        /*
         * Handle remaining interrupting conditions
         */  
        if ( istatReg & (SIP | DIP) )
        {
            [self Sym8xxProcessInterrupt];    
        }
    }
    while ( istatReg & (SIP | DIP | INTF) );

    [self enableAllInterrupts];

}

/*-----------------------------------------------------------------------------*
 * Process a request posted in the script's IODone mailbox.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxProcessIODone
{
    SRB				*srb;
    Nexus			*nexus;
    IODoneMailBox		*pMailBox;
    
 
    /*
     * The IODone mailbox contains an index into our Nexus pointer tables.
     *
     * The Nexus struct is part of the SRB so we can get our SRB address
     * by subtracting the offset of the Nexus struct in the SRB.
     */
    pMailBox = (IODoneMailBox *)&SCRIPT_VAR(R_ld_IOdone_mailbox);
    nexus = adapter->nexusPtrsVirt[pMailBox->nexus];        
    srb   = (SRB *)((u_int32_t)nexus - offsetof(SRB, nexus));    

    /*
     * If there was no request sense performed, then update the transfer
     * counts in the SRB.
     */
    if ( srb->srbState == ksrbStateCDBDone  )
    {
        [self Sym8xxUpdateXferOffset: srb];
    }

    /* 
     * Clear the completed Nexus pointer from our tables and clear the
     * IODone mailbox.
     */
    adapter->nexusPtrsVirt[pMailBox->nexus] = (Nexus *) -1;
    adapter->nexusPtrsPhys[pMailBox->nexus] = (Nexus *) -1;
    SCRIPT_VAR(R_ld_IOdone_mailbox)       = 0;

    /*
     * Don't ask why we need to do this -- we shouldn't if the Indirect SCSI
     * drivers worked properly!
     * 
     */
    if ( nexus->cdbData.cdb_opcode == C6OP_INQUIRY )
    {
        [self Sym8xxCheckInquiryData: srb];
    }

    /*
     * Wake up the client's thread to do post-processing
     */
    [srb->srbCmdLock unlockWith: ksrbCmdComplete];  
}

/*-----------------------------------------------------------------------------*
 * General script interrupt processing
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxProcessInterrupt
{
    SRB			*srb 		= NULL;
    Nexus		*nexus 		= NULL;
    u_int32_t		nexusIndex;
    u_int32_t		scriptPhase;
    u_int32_t		fifoCnt 	= 0;
    u_int32_t		dspsReg 	= 0;
    u_int32_t		dspReg  	= 0;


    /*
     * Read DSTAT/SIST regs to determine why the script stopped.
     */
    dstatReg = Sym8xxReadRegs( chipBaseAddr,  DSTAT, DSTAT_SIZE );
    IODelay(5);
    sistReg =  Sym8xxReadRegs( chipBaseAddr,  SIST,  SIST_SIZE );

//  kprintf( "SCSI(Symbios8xx): SIST = %04x DSTAT = %02x\n\r", sistReg, dstatReg  );

    /*
     * This Script var tells us what the script thinks it was doing when the interrupt occurred.
     */
    scriptPhase = EndianSwap32( SCRIPT_VAR(R_ld_phase_flag) );

    /*
     * SCSI Bus reset detected 
     *
     * Clean up the carnage.     
     * Note: This may be either an adapter or target initiated reset.
     */
    if ( sistReg & RSTI )
    {
        [self Sym8xxProcessSCSIBusReset];
        return;
    }

    /*
     * Calculate our current SRB/Nexus.
     *
     * Read a script var to determine the index of the nexus it was processing
     * when the interrupt occurred. The script will invalidate the index if there
     * is no target currently connected or the script cannot determine which target
     * has reconnected.
     */
    nexusIndex = EndianSwap32(SCRIPT_VAR(R_ld_nexus_index));
    if ( nexusIndex >= MAX_SCSI_TAG )
    {
        [self Sym8xxProcessNoNexus];
        return;
    }
    nexus  = adapter->nexusPtrsVirt[nexusIndex];        
    if ( nexus == (Nexus *) -1 )
    {
        [self Sym8xxProcessNoNexus];
        return;
    }
    srb = (SRB *)((u_int32_t)nexus - offsetof(SRB, nexus));  

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_phase_handler];
   
    /*   
     * Parity and SCSI Gross Errors.
     *
     * Abort the current connection. The abort completion will trigger
     * clean-up of the current SRB/Nexus.
     */
    if ( sistReg & PAR )
    {  
         srb->srbSCSIResult = SR_IOST_PARITY;
         [self Sym8xxAbortCurrent: srb];
    }

    else if ( sistReg & SGE )
    {
         srb->srbSCSIResult = SR_IOST_BV;
         [self Sym8xxAbortCurrent: srb];
    }
       
    /*
     * Unexpected disconnect. 
     *
     * If we were currently trying to abort this connection then mark the abort
     * as completed. For all cases clean-up and wake-up the client thread.
     */ 
    else if ( sistReg & UDC )
    {
        if ( srb->srbSCSIResult == SR_IOST_GOOD )
        {
            srb->srbSCSIResult = SR_IOST_BV;
        }
        adapter->nexusPtrsVirt[nexusIndex] = (Nexus *) -1;
        adapter->nexusPtrsPhys[nexusIndex] = (Nexus *) -1;

        if ( scriptPhase == A_kphase_ABORT_CURRENT )
        {
            abortCurrentSRB = NULL;
        }

        [srb->srbCmdLock unlockWith: ksrbCmdComplete];  

        scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];
    }

    /*
     * Phase Mis-match
     *
     * If we are in MsgOut phase then calculate how much of the message we sent. For
     * now, however, we dont handle the target rejecting messages, so the request is aborted.
     *
     * If we are in DataIn/DataOut phase. We update the SRB/Nexus with our current data 
     * pointers.
     */
    else if ( sistReg & MA )
    {
        if ( scriptPhase == A_kphase_MSG_OUT )
        {
            srb->srbMsgResid = [self Sym8xxCheckFifo:srb FifoCnt:&fifoCnt];
            nexus->msg.ppData   = EndianSwap32( EndianSwap32(nexus->msg.ppData) + EndianSwap32(nexus->msg.length) 
                                         - srb->srbMsgResid );
            nexus->msg.length   = EndianSwap32( srb->srbMsgResid );

            [self Sym8xxAbortCurrent: srb];
        }
        else if ( (scriptPhase == A_kphase_DATA_OUT) || (scriptPhase == A_kphase_DATA_IN) )
        {
            [self Sym8xxAdjustDataPtrs:srb Nexus:nexus];
        }
        else
        {
//          kprintf("SCSI(Symbios8xx): Unexpected phase mismatch - scriptPhase = %08x\n\r", scriptPhase);
            srb->srbSCSIResult = SR_IOST_BV;
            [self Sym8xxAbortCurrent: srb];
        }

        [self Sym8xxClearFifo];
    }
    
    /*
     * Selection Timeout.
     *
     * Clean-up the current request.
     */
    else if ( sistReg & STO )
    {
        srb->srbSCSIResult = SR_IOST_SELTO;

        adapter->nexusPtrsVirt[nexusIndex] = (Nexus *) -1;
        adapter->nexusPtrsPhys[nexusIndex] = (Nexus *) -1;
        SCRIPT_VAR(R_ld_IOdone_mailbox)    = 0;

        [srb->srbCmdLock unlockWith: ksrbCmdComplete];  

        scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];
    }
        
    /*
     * Handle script initiated interrupts
     */
    else if ( dstatReg & SIR )
    {
        dspsReg = Sym8xxReadRegs( chipBaseAddr, DSPS, DSPS_SIZE );

        switch ( dspsReg )
        {
            /* 
             * Non-zero SCSI status
             *
             * Send request sense CDB or complete request depending on SCSI status value
             */
            case A_status_error:
                if ( [self Sym8xxProcessStatus:srb] == NO )
                {
                    [self Sym8xxProcessIODone];
                }
                break;
             
            /*
             * Received SDTR/WDTR message from target.
             *
             * Prepare reply message if we requested negotiation. Otherwise reject
             * target initiated negotiation.
             */
	    case A_negotiateSDTR:
                [self Sym8xxNegotiateSDTR:srb Nexus: nexus];
                break;

	    case A_negotiateWDTR:
                [self Sym8xxNegotiateWDTR:srb Nexus: nexus];
                break;

            /*
             * Partial SG List completed.
             *
             * Refresh the list from the remaining addresses to be transfered and set the
             * script engine to branch into the list.
             */
            case A_sglist_complete:
                [self Sym8xxUpdateSGList:srb];
                scriptRestartAddr = (u_int32_t)&srb->srbPhys->nexus.sgListData[2];
                break;

            /*
             * Completed abort request
             *
             * Clean-up the aborted request.
             */
	    case A_abort_current:	
                if ( srb->srbSCSIResult == SR_IOST_GOOD )
                {
                    srb->srbSCSIResult = SR_IOST_BV;
                }

                adapter->nexusPtrsVirt[nexusIndex] = (Nexus *) -1;
                adapter->nexusPtrsPhys[nexusIndex] = (Nexus *) -1; 

                abortCurrentSRB = NULL;

                [srb->srbCmdLock unlockWith: ksrbCmdComplete];  

                scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];
                break;
    
            /*
             * Script detected protocol errors
             *
             * Abort the current request.
             */
            case A_unknown_msg_reject:
            case A_unknown_phase:
            case A_unexpected_msg:
            case A_unexpected_ext_msg:
                srb->srbSCSIResult = SR_IOST_TABT;
                [self Sym8xxAbortCurrent: srb];
                break;

            default:
                kprintf( "SCSI(Symbios8xx): Unknown Script Int = %08x\n\r", dspsReg );
                srb->srbSCSIResult = SR_IOST_INT;
                [self Sym8xxAbortCurrent: srb];
        }
    }

    /*
     * Illegal script instruction.
     *
     * We're toast! Abort the current request and hope for the best!
     */
    else if ( dstatReg & IID )
    {
        dspReg  = Sym8xxReadRegs( chipBaseAddr, DSP, DSP_SIZE );

        kprintf("SCSI(Symbios8xx): Illegal script instruction - dsp = %08x srb=%08x\n\r", dspReg, (u_int32_t)srb );

        srb->srbSCSIResult = SR_IOST_INT;
        [self Sym8xxAbortCurrent: srb];
    }

    if ( scriptRestartAddr )
    {
        Sym8xxWriteRegs( chipBaseAddr, DSP, DSP_SIZE, scriptRestartAddr );
    }
}    

/*-----------------------------------------------------------------------------*
 * Handle non-zero SCSI status
 *
 * Returns: 
 *    NO  - Clean-up request now.
 *    YES - Wait for request sense to complete.
 *
 * This routine filter's out BUSY and more arcane SCSI status conditions,
 * leaving CHECK_CONDITION, for which it set's up a request sense operation.
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxProcessStatus: (SRB *) srb;
{
    IODoneMailBox		*pMailBox;

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];
 
    pMailBox = (IODoneMailBox *)&SCRIPT_VAR(R_ld_IOdone_mailbox);

    /*
     * If a previous request sense failed, then clean-up the request now.
     */
    if ( srb->srbState != ksrbStateCDBDone )
    {
        if ( srb->srbSCSIResult == SR_IOST_GOOD )
        {
            srb->srbSCSIResult = SR_IOST_CHKSNV;
        }
        return NO;
    }

    /*
     * Update the SRB with our byte transferred count and SCSI status.
     * This information needs to be captured before we issue the request
     * sense.
     */
    srb->srbSCSIStatus = pMailBox->status;

    [self Sym8xxUpdateXferOffset: srb];

    if ( pMailBox->status != STAT_CHECK )
    {
        srb->srbSCSIResult = ST_IOST_BADST;
        return NO;
    }
    if ( srb->senseData == NULL )
    {
        srb->srbSCSIResult = SR_IOST_CHKSNV;
        return NO;
    }

    [self Sym8xxIssueRequestSense: srb];

    return YES;
}

/*-----------------------------------------------------------------------------*
 * Prepare request sense request.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxIssueRequestSense:(SRB *) srb
{
    IODoneMailBox		*pMailBox;
    u_int32_t			reqSenseMailBox;

    pMailBox = (IODoneMailBox *)&SCRIPT_VAR(R_ld_IOdone_mailbox);

    /*
     * We put the request sense Nexus in the last completed script mailbox and
     * back-up the script's mailbox pointer.
     */
    reqSenseMailBox = (u_int8_t)(EndianSwap32(SCRIPT_VAR(R_ld_counter)) - 1);
    SCRIPT_VAR(R_ld_counter) = EndianSwap32( reqSenseMailBox );

    srb->srbTimeout = kReqSenseTimeoutMS / kSCSITimerIntervalMS + 1; 

    srb->srbState      = ksrbStateReqSenseDone;
    srb->srbSCSIResult = SR_IOST_CHKSV;

    /*
     * Reuse the original Nexus struct. The original CDB is not preserved. The
     * original status and transfer counts and tag are kept in the SRB.
     */
    bzero( &srb->nexus.cdbData, 6 );
    srb->nexus.cdbData.cdb_c6.c6_opcode = C6OP_REQSENSE;
    srb->nexus.cdbData.cdb_c6.c6_lun    = srb->lun;
    srb->nexus.cdbData.cdb_c6.c6_len    = srb->senseDataLength;
    srb->nexus.cdb.length               = EndianSwap32( 6 );

    /* 
     * Force renegotiation on request sense.
     */
    targets[srb->target].flags &= ~(kTFXferSync | kTFXferWide16);
    srb->srbRequestFlags       &= ~(ksrbRFCmdQueueAllowed | ksrbRFDisconnectAllowed);
    [self Sym8xxCalcMsgs:srb];

    /*
     * Create a new SG List for the request sense data
     *
     */
    srb->xferOffset        = 0;
    srb->xferOffsetPrev    = 0;
    srb->xferClient        = IOVmTaskSelf();
    srb->xferBuffer        = srb->senseData;
    srb->xferCount         = srb->senseDataLength;
    srb->directionMask     = 0x01000000;
    srb->nexus.ppSGList    = (SGEntry *)EndianSwap32((u_int32_t)&srb->srbPhys->nexus.sgListData[2]);
    [self Sym8xxUpdateSGList: srb];

    /*
     * If the original request was using cmd-queuing, we clean-up the original tagged request
     * and convert it to a non-tagged request sense.
     */
    if ( srb->nexus.tag >= MIN_SCSI_TAG )
    {
        adapter->nexusPtrsVirt[pMailBox->nexus] = (Nexus *) -1;
        adapter->nexusPtrsPhys[pMailBox->nexus] = (Nexus *) -1;

        srb->nexus.tag  = (srb->target << 3) | srb->lun;
        adapter->nexusPtrsVirt[srb->nexus.tag] = &srb->nexus;
        adapter->nexusPtrsPhys[srb->nexus.tag] = (Nexus *)EndianSwap32( (u_int32_t)&srb->srbPhys->nexus );
    }

    adapter->schedMailBox[reqSenseMailBox]  = (Nexus *)EndianSwap32 ( (u_int32_t)&srb->srbPhys->nexus );

    [self Sym8xxSignalScript: srb];

    SCRIPT_VAR(R_ld_IOdone_mailbox) = 0;
}

/*-----------------------------------------------------------------------------*
 * Current Data Pointer calculations
 * 
 * To do data transfers the driver generates a list of script instructions 
 * in system storage to deliver data to the requested physical addresses. The
 * script branches to the list when the target enters data transfer phase.
 *
 * When the target changes phase during a data transfer, data is left trapped
 * inside the various script engine registers. This routine determines how much
 * data was not actually transfered to/from the target and generates a new
 * S/G List entry for the partial transfer and a branch back into the original
 * S/G list. These script instructions are stored in two reserved slots at the
 * top of the original S/G List.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxAdjustDataPtrs:(SRB *) srb Nexus:(Nexus *) nexus
{
    u_int32_t		i;
    u_int32_t		sgResid;
    u_int32_t		fifoCnt;
    u_int32_t		dspReg;
    u_int32_t		sgDone;
    u_int8_t		scntl2Reg;
    Nexus		*nexusPhys;

    /*
     * Determine SG element residual
     *
     * This routine returns how much of the current S/G List element the 
     * script was processing remains to be sent/received. All the information
     * required to do this is stored in the script engine's registers.
     */
    sgResid = [self Sym8xxCheckFifo:srb FifoCnt:&fifoCnt];

    /*
     * Determine which script instruction in our SGList we were executing when
     * the target changed phase.
     *
     * The script engine's dspReg tells us where the script thinks it was. Based
     * on the physical address of our current SRB/Nexus we can calculate
     * an index into our S/G List.  
     */
    dspReg  = Sym8xxReadRegs( chipBaseAddr, DSP, DSP_SIZE );

    i = ((dspReg - (u_int32_t)srb->srbPhys->nexus.sgListData) / sizeof(SGEntry)) - 1;
       
    if ( i > MAX_SGLIST_ENTRIES-1 )
    {
       kprintf("SCSI(Symbios8xx): Bad sgListIndex\n\r");
       [self Sym8xxAbortCurrent: srb];
       return;
    }

    /* 
     * Wide/odd-byte transfers.
     *     
     * When dealing with Wide data transfers, if a S/G List ends with an odd-transfer count, then a
     * valid received data byte is left in the script engine's SWIDE register. The least painful way
     * to recover this byte is to construct a small script thunk to transfer one additional byte. The
     * script will automatically draw this byte from the SWIDE register rather than the SCSI bus.
     * The script thunk then branches back to script's PhaseHandler entrypoint.
     * 
     */
    nexusPhys = &srb->srbPhys->nexus;

    scntl2Reg = Sym8xxReadRegs( chipBaseAddr, SCNTL2, SCNTL2_SIZE );
    if ( scntl2Reg & WSR )
    {
        adapter->xferSWideInst[0] = EndianSwap32( srb->directionMask | 1 );
        adapter->xferSWideInst[1] = nexus->sgListData[i].physAddr;
        adapter->xferSWideInst[2] = EndianSwap32( 0x80080000 );
        adapter->xferSWideInst[3] = EndianSwap32( (u_int32_t)&chipRamAddrPhys[Ent_phase_handler] );

        scriptRestartAddr = (u_int32_t) adapterPhys->xferSWideInst;
        
        /*
         * Note: There is an assumption here that the sgResid count will be > 1. It appears 
         *       that the script engine does not generate a phase-mismatch interrupt until 
         *       we attempt to move > 1 byte from the SCSI bus and the only byte available is
         *       in SWIDE. 
         */        
        sgResid--;
    }

    /*
     * Calculate partial S/G List instruction and branch
     *
     * Fill in slots 0/1 of the SGList based on the SGList index (i) and SGList residual count
     * (sgResid) calculated above.
     *
     */
    sgDone  = (EndianSwap32( nexus->sgListData[i].length ) & 0x00ffffff) - sgResid;

    nexus->sgListData[0].length   = EndianSwap32( sgResid | srb->directionMask );
    nexus->sgListData[0].physAddr = EndianSwap32( EndianSwap32(nexus->sgListData[i].physAddr) + sgDone );
    /*
     * If a previously calculated SGList 0 entry was interrupted again, we dont need to calculate
     * a new branch address since the previous one is still valid.
     */
    if ( i != 0 )
    {
        nexus->sgListData[1].length   = EndianSwap32( 0x80080000 );
        nexus->sgListData[1].physAddr = EndianSwap32( (u_int32_t)&nexusPhys->sgListData[i+1] );
        nexus->sgNextIndex            = i + 1;
    }
    nexus->ppSGList = (SGEntry *)EndianSwap32( (u_int32_t) &nexusPhys->sgListData[0] );
 
    /*
     * The script sets this Nexus variable to non-zero each time it calls the driver generated
     * S/G list. This allows the driver's completion routines to differentiate between a successful
     * transfer vs no data transfer at all.
     */
    nexus->dataXferCalled = 0;

    return;
}

/*-----------------------------------------------------------------------------*
 * Determine SG element residual
 *
 * This routine returns how much of the current S/G List element the 
 * script was processing remains to be sent/received. All the information
 * required to do this is stored in the script engine's registers.
 *
 *-----------------------------------------------------------------------------*/
- (u_int32_t) Sym8xxCheckFifo: (SRB *) srb FifoCnt:(u_int32_t *)pfifoCnt
{
    BOOL		fSCSISend;
    BOOL		fXferSync;
    u_int32_t		scriptPhase 	= 0;
    u_int32_t		dbcReg    	= 0;
    u_int32_t		dfifoReg  	= 0;
    u_int32_t		ctest5Reg 	= 0;
    u_int8_t		sstat0Reg 	= 0;
    u_int8_t		sstat1Reg 	= 0;
    u_int8_t		sstat2Reg 	= 0;
    u_int32_t		fifoCnt   	= 0;
    u_int32_t		sgResid	  	= 0;

    scriptPhase = EndianSwap32( SCRIPT_VAR(R_ld_phase_flag) );

    fSCSISend =  (scriptPhase == A_kphase_DATA_OUT) || (scriptPhase == A_kphase_MSG_OUT);
 
    fXferSync =  ((scriptPhase == A_kphase_DATA_OUT) || (scriptPhase == A_kphase_DATA_IN)) 
                         && (srb->nexus.targetParms.sxferReg & 0x1F);  

    dbcReg = Sym8xxReadRegs( chipBaseAddr, DBC, DBC_SIZE ) & 0x00ffffff;

    if ( !(dstatReg & DFE) )
    {
        ctest5Reg = Sym8xxReadRegs( chipBaseAddr, CTEST5, CTEST5_SIZE );
        dfifoReg  = Sym8xxReadRegs( chipBaseAddr, DFIFO,  DFIFO_SIZE );

        if ( ctest5Reg & DFS )
        {
            fifoCnt = ((((ctest5Reg & 0x03) << 8) | dfifoReg) - dbcReg) & 0x3ff;
        }
        else
        {
            fifoCnt = (dfifoReg - dbcReg) & 0x7f;
        }
    }

    sstat0Reg = Sym8xxReadRegs( chipBaseAddr, SSTAT0, SSTAT0_SIZE );
    sstat2Reg = Sym8xxReadRegs( chipBaseAddr, SSTAT2, SSTAT2_SIZE );
 
    if ( fSCSISend )
    {    
        fifoCnt += (sstat0Reg & OLF )  ? 1 : 0;
        fifoCnt += (sstat2Reg & OLF1)  ? 1 : 0;

        if ( fXferSync )
        {
            fifoCnt += (sstat0Reg & ORF )  ? 1 : 0;
            fifoCnt += (sstat2Reg & ORF1)  ? 1 : 0;
        }
    }
    else
    {
        if ( fXferSync )
        {
            sstat1Reg = Sym8xxReadRegs( chipBaseAddr, SSTAT0, SSTAT0_SIZE );
            fifoCnt +=  (sstat1Reg >> 4) | (sstat2Reg & FF4);  
        }
        else
        {
            fifoCnt += (sstat0Reg & ILF )  ? 1 : 0;
            fifoCnt += (sstat2Reg & ILF1)  ? 1 : 0;
        }
    }   
    
    sgResid   = dbcReg + fifoCnt;
    *pfifoCnt = fifoCnt;

    return sgResid;
}

/*-----------------------------------------------------------------------------*
 * Calculate transfer counts.
 *
 * This routine updates srb->xferDone with the amount of data transferred
 * by the last S/G List executed.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxUpdateXferOffset:(SRB *) srb
{
    u_int32_t		i;
    u_int32_t		xferOffset;

    /*
     * srb->xferOffset contains the client buffer offset INCLUDING the range
     * covered by the current SGList.
     */
    xferOffset = srb->xferOffset;

    /*
     * If script did not complete the current transfer list then we need to determine
     * how much of the list was completed.
     */
    if ( srb->nexus.dataXferCalled == 0 )
    {
        /* 
         * srb->xferOffsetPrev contains the client buffer offset EXCLUDING the
         * range covered by the current SGList.
         */
        xferOffset = srb->xferOffsetPrev;

        /*
         * Calculate bytes transferred for partially completed list.
         *
         * To calculate the amount of this list completed, we sum the residual amount
         * in SGList Slot 0 and the completed list elements 2 to sgNextIndex-1.
         */
        if ( srb->nexus.sgNextIndex != 0 )
        {
            xferOffset += EndianSwap32( srb->nexus.sgListData[srb->nexus.sgNextIndex-1].length )
                             - EndianSwap32( srb->nexus.sgListData[0].length );

            for ( i=2; i < srb->nexus.sgNextIndex-1; i++ )
            {
                xferOffset += EndianSwap32( srb->nexus.sgListData[i].length ) & 0x00ffffff;
            }
        }
    }
    
    /*
     * The script leaves the result of any Ignore Wide Residual message received from the target
     * during the transfer.
     */
    xferOffset -= srb->nexus.wideResidCount;


#if 0
    {
        u_int32_t	resid = srb->xferOffset - xferOffset;
        if ( resid )
        {
            kprintf( "SCSI(Symbios8xx): Incomplete transfer - Req Count = %08x Act Count = %08x - srb = %08x\n\r", 
                    srb->xferCount, xferOffset, (u_int32_t)srb );
        }   
    }
#endif

    srb->xferDone = xferOffset;
}

/*-----------------------------------------------------------------------------*
 * No SRB/Nexus Processing.
 *
 * In some cases (mainly Aborts) not having a SRB/Nexus is normal. In other
 * cases it indicates a problem such a reconnection from a target that we
 * have no record of.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxProcessNoNexus
{
    u_int32_t			dspsReg;
    u_int32_t			dspReg      = 0;
    u_int32_t			scriptPhase = -1 ;

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];

    dspsReg = Sym8xxReadRegs( chipBaseAddr, DSPS, DSPS_SIZE );

    scriptPhase = EndianSwap32( SCRIPT_VAR(R_ld_phase_flag) );

    /* 
     * If we were trying to abort or disconnect a target and the bus
     * is now free we consider the abort to have completed.
     */
    if ( sistReg & UDC ) 
    {
        if ( (scriptPhase == A_kphase_ABORT_MAILBOX) && abortSRB )
        {
            [abortSRB->srbCmdLock unlockWith: ksrbCmdComplete];
            abortSRB = (SRB *) NULL;  
            SCRIPT_VAR(R_ld_AbortBdr_mailbox) = 0;
        }         
        else if ( scriptPhase == A_kphase_ABORT_CURRENT )
        {
            abortCurrentSRB = NULL;
        }
    }
    /*
     * If we were trying to connect to a target to send it an abort message, and
     * we timed out, we consider the abort as completed.
     *
     * Note: In this case the target may be hung, but at least its not on the bus.
     */
    else if ( sistReg & STO )
    {
        if ( (scriptPhase == A_kphase_ABORT_MAILBOX) && abortSRB )
        {
            [abortSRB->srbCmdLock unlockWith: ksrbCmdComplete];
            abortSRB = (SRB *) NULL;  
            SCRIPT_VAR(R_ld_AbortBdr_mailbox) = 0;
        }         
    }     
    
    /*
     * If the script died, without a vaild nexusIndex, we abort anything that is currently
     * connected and hope for the best!
     */
    else if ( dstatReg & IID )
    {
        dspReg  = Sym8xxReadRegs( chipBaseAddr, DSP, DSP_SIZE );
        kprintf("SCSI(Symbios8xx): Illegal script instruction - dsp = %08x srb=0\n\r", dspReg );
        [self Sym8xxAbortCurrent: (SRB *) -1];
    }

    /*
     * Script signaled conditions
     */
    else if ( dstatReg & SIR )
    {
        switch ( dspsReg )
        {
            case A_abort_current:
                abortCurrentSRB = NULL;
                break;
              
            case A_abort_mailbox:
                [abortSRB->srbCmdLock unlockWith: ksrbCmdComplete];
                abortSRB = (SRB *) NULL;  
                SCRIPT_VAR(R_ld_AbortBdr_mailbox) = 0;
                break;
           
            default:
               [self Sym8xxAbortCurrent: (SRB *)-1];
        }
    }             
    else
    {
        [self Sym8xxAbortCurrent: (SRB *)-1];
    }

    if ( scriptRestartAddr )
    {
        Sym8xxWriteRegs( chipBaseAddr, DSP, DSP_SIZE, scriptRestartAddr );
    }
}
 

/*-----------------------------------------------------------------------------*
 * Abort currently connected target.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxAbortCurrent:(SRB *)srb
{
    if ( abortCurrentSRB )
    {
        if ( abortCurrentSRB != srb )
        {
//          kprintf("SCSI(Symbios8xx): Multiple abort immediate SRBs - resetting\n\r");
            [self Sym8xxSCSIBusReset: (SRB *)NULL];
        }
        return;
    }
   
    abortCurrentSRB        = srb;
    abortCurrentSRBTimeout = kAbortTimeoutMS / kSCSITimerIntervalMS + 1; 

    /*
     * Issue abort or abort tag depending on whether the is a tagged request
     */
    SCRIPT_VAR(R_ld_AbortCode) = EndianSwap32( ((srb != (SRB *)-1) && (srb->nexus.tag >= MIN_SCSI_TAG)) ? 0x0d : 0x06 );
    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_issueAbort_BDR];

    [self Sym8xxClearFifo];
}

/*-----------------------------------------------------------------------------*
 * This routine clears the script engine's SCSI and DMA fifos.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxClearFifo
{
    u_int8_t		ctest3Reg;
    u_int8_t		stest2Reg;
    u_int8_t		stest3Reg;

    stest2Reg  = Sym8xxReadRegs( chipBaseAddr, STEST2, STEST2_SIZE );
    if ( stest2Reg & ROF )
    {
        Sym8xxWriteRegs( chipBaseAddr, STEST2, STEST2_SIZE, stest2Reg );
    }

    ctest3Reg = Sym8xxReadRegs( chipBaseAddr, CTEST3, CTEST3_SIZE );
    ctest3Reg |= CLF;
    Sym8xxWriteRegs( chipBaseAddr, CTEST3, CTEST3_SIZE, ctest3Reg );

    stest3Reg = Sym8xxReadRegs( chipBaseAddr, STEST3, STEST3_SIZE );
    stest3Reg |= CSF;
    Sym8xxWriteRegs( chipBaseAddr,STEST3, STEST3_SIZE, stest3Reg );

    do
    {
        ctest3Reg = Sym8xxReadRegs( chipBaseAddr, CTEST3, CTEST3_SIZE );
        stest2Reg = Sym8xxReadRegs( chipBaseAddr, STEST3, STEST3_SIZE );
        stest3Reg = Sym8xxReadRegs( chipBaseAddr, STEST3, STEST3_SIZE );
    } 
    while( (ctest3Reg & CLF) || (stest3Reg & CSF) || (stest2Reg & ROF) );            
}

/*-----------------------------------------------------------------------------*
 * This routine processes the target's response to our SDTR message.
 * 
 * We calculate the values for the script engine's timing registers
 * for synchronous registers, and update our tables indicating that
 * requested data transfer mode is in-effect.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxNegotiateSDTR:(SRB *) srb Nexus:(Nexus *)nexus
{
    u_int32_t		x;
    u_int8_t		*pMsg;
    u_int32_t		syncPeriod;
    
    /*
     * If we were not negotiating, the send MsgReject to targets negotiation
     * attempt.
     */
    if ( !(srb->srbRequestFlags & ksrbRFNegotiateSync) )
    {
        [self Sym8xxSendMsgReject: srb];
        return;
    }

    /* 
     * Get pointer to negotiation message received from target.
     */
    pMsg = (u_int8_t *) &SCRIPT_VAR(R_ld_message);

    /*
     * The target's SDTR response contains the (transfer period / 4).
     *
     * We set our sync clock divisor to 1, 2, or 4 giving us a clock rates
     * of:
     *     80Mhz (Period = 12.5ns), 
     *     40Mhz (Period = 25.0ns)
     *     20Mhz (Period = 50.0ns) 
     *
     * This is further divided by the value in the sxfer reg to give us the final sync clock rate.
     *
     * The requested sync period is scaled up by 1000 and the clock periods are scaled up by 10
     * giving a result scaled up by 100. This is rounded-up and converted to sxfer reg values.
     */
    syncPeriod = (u_int32_t)pMsg[3] << 2;
    if ( syncPeriod < 100 )
    {
        nexus->targetParms.scntl3Reg |= SCNTL3_INIT_875_ULTRA;
        x = (syncPeriod * 1000) / 125;
    }
    else if ( syncPeriod < 200 )
    {
        nexus->targetParms.scntl3Reg  |= SCNTL3_INIT_875_FAST;
        x = (syncPeriod * 1000) / 250;
    }
    else 
    {
        nexus->targetParms.scntl3Reg  |= SCNTL3_INIT_875_SLOW;
        x = (syncPeriod * 1000) / 500;
    }
           
    if ( x % 100 ) x += 100;
    
    /*
     * sxferReg  Bits: 5-0 - Transfer offset
     *                 7-6 - Sync Clock Divisor (0 = sync clock / 4)
     */
    nexus->targetParms.sxferReg = ((x/100 - 4) << 5) | pMsg[4];

    /*
     * Update our per-target tables and set-up the hardware regs for this request.
     *
     * On reconnection attempts, the script will use our per-target tables to set-up
     * the scntl3 and sxfer registers in the script engine.
     */
    adapter->targetClocks[srb->target].sxferReg  = nexus->targetParms.sxferReg;
    adapter->targetClocks[srb->target].scntl3Reg = nexus->targetParms.scntl3Reg;

    Sym8xxWriteRegs( chipBaseAddr, SCNTL3, SCNTL3_SIZE, nexus->targetParms.scntl3Reg );
    Sym8xxWriteRegs( chipBaseAddr, SXFER,  SXFER_SIZE,  nexus->targetParms.sxferReg );

    targets[srb->target].flags |= kTFXferSync;

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_clearACK];
}
   
/*-----------------------------------------------------------------------------*
 * This routine processes the target's response to our WDTR message.
 *
 * In addition, if there is a pending SDTR message, this routine sends it
 * to the target.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxNegotiateWDTR:(SRB *) srb Nexus:(Nexus *)nexus
{
    u_int8_t		*pMsg;
    u_int32_t		msgBytesSent;
    u_int32_t           msgBytesLeft;

    /*
     * If we were not negotiating, the send MsgReject to targets negotiation
     * attempt.
     */
   if ( !(srb->srbRequestFlags & ksrbRFNegotiateWide) )
    {
        [self Sym8xxSendMsgReject: srb];
        return;
    }

    /* 
     * Set Wide (16-bit) vs Narrow (8-bit) data transfer mode based on target's response.
     */
    pMsg = (u_int8_t *) &SCRIPT_VAR(R_ld_message);

    if ( pMsg[3] == 1 )
    {
        nexus->targetParms.scntl3Reg |= EWS;
    }
    else
    {
        nexus->targetParms.scntl3Reg &= ~EWS;
    }

    /*
     * Update our per-target tables and set-up the hardware regs for this request.
     *
     * On reconnection attempts, the script will use our per-target tables to set-up
     * the scntl3 and sxfer registers in the script engine.
     */

    adapter->targetClocks[srb->target].scntl3Reg = nexus->targetParms.scntl3Reg;
    Sym8xxWriteRegs( chipBaseAddr, SCNTL3, SCNTL3_SIZE, nexus->targetParms.scntl3Reg );

    targets[srb->target].flags |= kTFXferWide16;

    /*
     * If there any pending messages left for the target, send them now, 
     */
    msgBytesSent = EndianSwap32( nexus->msg.length );
    msgBytesLeft = srb->srbMsgLength - msgBytesSent;
    if ( msgBytesLeft )
    {
        nexus->msg.length = EndianSwap32( msgBytesLeft );
        nexus->msg.ppData = EndianSwap32( EndianSwap32( nexus->msg.ppData ) + msgBytesSent );                
        scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_issueMessageOut];
    }

    /*
     * Otherwise, tell the script we're done with MsgOut phase.
     */
    else
    {
        scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_clearACK];
    }
}  

/*-----------------------------------------------------------------------------*
 * Reject message received from target.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxSendMsgReject:(SRB *) srb
{
    srb->nexus.msg.ppData = EndianSwap32((u_int32_t)&srb->srbPhys->nexus.msgData);
    srb->nexus.msg.length = EndianSwap32(0x01);
    srb->nexus.msgData[0] = 0x07;

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_issueMessageOut];
}
  
/*-----------------------------------------------------------------------------*
 * This routine snoops inquiry data.
 *
 * The indirect SCSI Disk driver in driverKit, does not bother to check the target's 
 * capabilities before enabling Synchronous Negotiation or Cmd Queueing. If things 
 * were left to themselves, targets that did not support tags would be broken since 
 * the request comming from driverKit always indicate that tags are allowed.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxCheckInquiryData: (SRB *)srb
{
    IOMemoryDescriptor			*mem;
    inquiry_reply_t			inqData;
    u_int32_t				inqSize;

    bzero( &inqData, sizeof(inqData) );

    inqSize = (srb->xferDone < sizeof(inqData)) ? srb->xferDone : sizeof(inqData);

    mem = [[ IOSimpleMemoryDescriptor alloc ] initWithAddress: (void *)srb->xferBuffer length: inqSize ];
    [mem setClient: srb->xferClient];

    do
    {
        if ( srb->srbSCSIResult != SR_IOST_GOOD )
        {
            continue;
        }
        if ( srb->xferDone < offsetof(inquiry_reply_t, ir_vendorid) )
        {
            continue;
        }
          
        if ( [mem readFromClient: (void *) &inqData count: inqSize] != inqSize )
        {
            continue;
        }
        if ( inqData.ir_qual != DEVQUAL_OK )
        {
            continue;
        }

        if ( inqData.ir_wbus16 )
        {
            targets[srb->target].flags |= kTFXferWide16Supported;
        }
        if ( inqData.ir_sync )
        {
           targets[srb->target].flags |= kTFXferSyncSupported;
        }
        if ( inqData.ir_cmdque )
        {
           targets[srb->target].flags |= kTFCmdQueueSupported;
        }
    }
    while ( 0 );
    [mem release];
}


/*-----------------------------------------------------------------------------*
 * This routine initiates a SCSI Bus Reset.
 *
 * This may be an internally generated request as part of error recovery or
 * a client's bus reset request.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxSCSIBusReset: (SRB *)srb
{
    if ( srb )
    {
        if ( resetSRB )
        {
            srb->srbSCSIResult = SR_IOST_CMDREJ;
            [srb->srbCmdLock unlockWith: ksrbCmdComplete];  
            return;
        }    
        resetSRB = srb;
    }

    Sym8xxWriteRegs( chipBaseAddr, SCNTL1, SCNTL1_SIZE, SCNTL1_SCSI_RST );
    IODelay( 25 );
    Sym8xxWriteRegs( chipBaseAddr, SCNTL1, SCNTL1_SIZE, SCNTL1_INIT );
}
    
/*-----------------------------------------------------------------------------*
 * This routine handles a SCSI Bus Reset interrupt.
 *
 * The SCSI Bus reset may be generated by a target on the bus, internally from
 * the driver's error recovery or from a client request.
 *
 * Once the reset is detected we establish a settle period where new client requests
 * are blocked in the client thread. In addition we flush all currently executing
 * scsi requests back to the client.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxProcessSCSIBusReset
{
    SRB			*srb   = 0;
    Nexus		*nexus = 0;
    u_int32_t		i;

    /*
     * If we got another bus reset event during the settle period we extend the settle
     * period accordingly.
     */
    if ( resetQuiesceTimer )
    {
        resetQuiesceTimer = kResetQuiesceDelayMS / kSCSITimerIntervalMS + 1;
        return;
    }

    resetSeqNum = srbSeqNum;
//  kprintf("SCSI(Symbios8xx): Reset Started - SRB Seq = %d\n\r", resetSeqNum);

    /*
     * We take the resetQuiesceSem lock which will block new client thread requests.
     * 
     * Note: The client thread checks resetQuiesceTimer for <> 0 before taking this lock.
     */
    [resetQuiesceSem lock];
    resetQuiesceTimer = kResetQuiesceDelayMS / kSCSITimerIntervalMS + 1;

    /*
     * We end any aborts currently in progress
     */
    abortCurrentSRB = (SRB *)NULL;

    if ( abortSRB )
    {
        [abortSRB->srbCmdLock unlockWith: ksrbCmdComplete];
        abortSRB = (SRB *) NULL;  
    }

   [self Sym8xxClearFifo];

   /* 
    * We return anything in our Nexus table back to the client
    */
   for ( i=0; i < MAX_SCSI_TAG; i++ )
    {
        nexus = adapter->nexusPtrsVirt[i];
        if ( nexus == (Nexus *) -1 )
        {
            continue;
        }

        srb = (SRB *)((u_int32_t)nexus - offsetof(SRB, nexus));
  
        srb->srbSCSIResult = SR_IOST_RESET;

        adapter->nexusPtrsVirt[i] = (Nexus *) -1;
        adapter->nexusPtrsPhys[i] = (Nexus *) -1;

        [srb->srbCmdLock unlockWith: ksrbCmdComplete];  
    }

    /*
     * We clear the script's request mailboxes. Any work in the script mailboxes is
     * already in the NexusPtr tables so we have already have handled the SRB/Nexus
     * cleanup.
     */
    for ( i=0; i < MAX_SCHED_MAILBOXES; i++ )
    {
        adapter->schedMailBox[i] = 0;
    }

    SCRIPT_VAR(R_ld_AbortBdr_mailbox) = 0;
    SCRIPT_VAR(R_ld_IOdone_mailbox)   = 0;
    SCRIPT_VAR(R_ld_counter)          = 0;
    mailBoxIndex                      = 0;

    /*
     * Reset the data transfer mode/clocks in our per-target tables back to Async/Narrow 8-bit
     */
    for ( i=0; i < MAX_SCSI_TARGETS; i++ )
    {
        targets[i].flags &= ~(kTFXferSync | kTFXferWide16);

        adapter->targetClocks[i].scntl3Reg = SCNTL3_INIT_875;
        adapter->targetClocks[i].sxferReg  = 0;
    }

    scriptRestartAddr = (u_int32_t) &chipRamAddrPhys[Ent_select_phase];
    Sym8xxWriteRegs( chipBaseAddr, DSP, DSP_SIZE, scriptRestartAddr );

}

/*-----------------------------------------------------------------------------*
 * This routine sets the SIGP bit in the script engine's ISTAT
 * register. This signals the script to wake-up for a WAIT for
 * reselection instruction. The script will then check the mailboxes
 * for work to do.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxSignalScript:(SRB *)srb
{
    Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, SIGP );
}

/*-----------------------------------------------------------------------------*
 * Timeout handler.
 *
 * This routine is scheduled and implements timeouts for the driver.
 *
 * The following items are timed:
 *
 * - Reset settle period.
 * - Aborts
 * - SRBs
 *
 *-----------------------------------------------------------------------------*/
- (void) timeoutOccurred
{
    SRB				*srb;
    Nexus			*nexus;
    u_int32_t			i;
    u_int32_t			nexusIndex;
    u_int32_t			mailboxNexusIndex = -1;

    if ( SCRIPT_VAR( R_ld_IOdone_mailbox ) )
    {
        mailboxNexusIndex = ((IODoneMailBox *)&SCRIPT_VAR(R_ld_IOdone_mailbox))->nexus;
    }

    nexusIndex = EndianSwap32( SCRIPT_VAR(R_ld_nexus_index) );

    /*
     * If we are in a reset settle period, suspend all other timing. 
     *
     * When the reset settle period completes, return the SRB if the
     * client requested the bus reset. Also unlock the reset semaphore.
     */
    if ( resetQuiesceTimer )
    {
        if ( --resetQuiesceTimer )
        {
            goto timeoutOccurred_Exit;
        }

        if ( resetSRB )
        {
            [resetSRB->srbCmdLock unlockWith: ksrbCmdComplete];  
            resetSRB = (SRB *) NULL;
        }
//      kprintf("SCSI(Symbios8xx): Reset Ended - SRB Seq = %d\n\r", srbSeqNum);
        [resetQuiesceSem unlock];
    }

    /*
     * Check whether an abort timed out. If it does, then its likely that a target is
     * hung on the bus. In this case the only recourse is to issue a SCSI Bus reset.
     */
    if ( abortCurrentSRB && abortCurrentSRBTimeout )
    {
        if ( !(--abortCurrentSRBTimeout) )
        {
//          kprintf("SCSI(Symbios8xx): Abort Current SRB failed - resetting\n\r");
            [self Sym8xxSCSIBusReset: (SRB *)NULL];
            goto timeoutOccurred_Exit;
        }
    }

    if ( abortSRB && abortSRBTimeout )
    {
        if ( !(--abortSRBTimeout) )
        {
//          kprintf("SCSI(Symbios8xx): MailBox abort failed - resetting\n\r");
            [self Sym8xxSCSIBusReset: (SRB *)NULL];
            goto timeoutOccurred_Exit;
        }
    }

    /*
     * Scan the Nexus pointer table looking for SRBs to timeout
     */
    for ( i=0; i < MAX_SCSI_TAG; i++ )
    {
         nexus = adapter->nexusPtrsVirt[i];
         if ( nexus == (Nexus *) -1 )
         {
             continue;
         }

         srb = (SRB *)((u_int32_t)nexus - offsetof(SRB, nexus));
         if ( srb->srbTimeout )
         {
             if ( !(--srb->srbTimeout) )
             {  
//               kprintf("SCSI(Symbios8xx): Timeout - Target = %d SRB = %08x SRB Seq = %d\n\r", 
//                            srb->target, (u_int32_t)srb, srb->srbSeqNum );

                 /* If the SRB we're timing out is in the script's IODone mailbox, then
                  * clear the mailbox in addition to timing out the request.
                  */
                 if ( i == mailboxNexusIndex )
                 {
                     SCRIPT_VAR(R_ld_IOdone_mailbox) = 0;
                 }
                 
                 /* 
                  * If the target for the SRB we're timing out is currently connected on 
                  * the SCSI Bus, then issue an abort. Since it is likely that the script
                  * is running in this scenario, we call a Sym8xxAbortScript to shutdown the
                  * script engine in an orderly fashion.
                  */
                 if ( i == nexusIndex )
                 {
                     [self Sym8xxAbortScript];
                     srb->srbSCSIResult = SR_IOST_IOTO;
                     [self Sym8xxAbortCurrent: srb];
                     Sym8xxWriteRegs( chipBaseAddr, DSP, DSP_SIZE, scriptRestartAddr );
                 }  
                
                 /* 
                  * If target for the SRB we're timing out is not on the SCSI bus, then
                  * we mark the request as requiring an Abort and return it to the client
                  * thread. The client thread will then schedule the abort.
                  */ 
                 else
                 {
                     adapter->nexusPtrsVirt[i] = (Nexus *) -1;
                     adapter->nexusPtrsPhys[i] = (Nexus *) -1;
                     
                     srb->srbSCSIResult = SR_IOST_IOTO;
                     srb->srbCmd        = ksrbCmdProcessTimeout;
                     [srb->srbCmdLock unlockWith: ksrbCmdComplete];  
                 }
             }
         }
    }

   
timeoutOccurred_Exit: ;
    /*
     * Reschedule the next timer interval.
     */
    ns_timeout((func) Sym8xxTimerReq, (void *) self, (ns_time_t) kSCSITimerIntervalMS * 1000 * 1000, (int)CALLOUT_PRI_THREAD);
}  
  
/*-----------------------------------------------------------------------------*
 * This routine does a mailbox abort.
 *
 * This type of abort is used for targets not currently connected to the SCSI Bus.
 *
 * The script will select the target and send a tag (if required) followed by the
 * appropriate abort message (abort/abort-tag)
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxAbortBdr:(SRB *) srb
{
    IOAbortBdrMailBox 			abortMailBox;

    abortSRB        = srb;
    abortSRBTimeout = kAbortTimeoutMS / kSCSITimerIntervalMS + 1; 

    /*
     * Setup a script variable containing the abort information.
     */
    abortMailBox.message   = ( srb->nexus.tag < MIN_SCSI_TAG) ? 0x06 : 0x0d;
    abortMailBox.identify  = srb->lun | 0xC0 ;
    abortMailBox.scsi_id   = srb->target;
    abortMailBox.tag       = ( srb->nexus.tag < MIN_SCSI_TAG) ? 0 : srb->nexus.tag;

    SCRIPT_VAR(R_ld_AbortBdr_mailbox) = *(u_int32_t *) &abortMailBox;

    [self Sym8xxSignalScript: srb];
}

/*-----------------------------------------------------------------------------*
 * This routine is used to shutdown the script engine in an orderly fashion.
 *
 * Normally the script engine automatically stops when an interrupt is generated. However,
 * in the case of timeouts we need to change the script engine's dsp reg (instruction pointer).
 * to issue an abort.
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxAbortScript
{
    ns_time_t		currentTime;
    ns_time_t		endTime;

    [self disableAllInterrupts];
    
    /*
     * We set the ABRT bit in ISTAT and spin until the script engine acknowledges the
     * abort or we timeout.
     */
    Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, ABRT );
    
    IOGetTimestamp( &endTime );

    endTime += (kAbortScriptTimeoutMS * 1000 * 1000);

    do
    {
        IOGetTimestamp( &currentTime );

        istatReg = Sym8xxReadRegs( chipBaseAddr, ISTAT, ISTAT_SIZE );

        if ( istatReg & SIP )
        {
            Sym8xxReadRegs( chipBaseAddr, SIST, SIST_SIZE );
            continue;
        }
    
        if ( istatReg & DIP )
        {
            Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, 0x00 );
            Sym8xxReadRegs( chipBaseAddr, DSTAT, DSTAT_SIZE );
            break;
        }
    }
    while ( currentTime < endTime );
    
    istatReg = SIGP;
    Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, istatReg );

    [self enableAllInterrupts];

    if ( currentTime >= endTime )
    {
//      kprintf( "SCSI(Symbios8xx): Abort script failed - resetting bus\n\r" );
        [self Sym8xxSCSIBusReset: NULL];
    }  

  }
    
@end
