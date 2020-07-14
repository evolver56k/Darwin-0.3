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

/**
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.12	Initial conversion from AMDPCSCSIDriver sources.
 *
 * There is no code in this module. It contains documentation of the 53C96 SCSI state
 * machine.
 *
 * Note: we must use a comment here as some C compilers parse the text inside an #if 0
 * and post errors if there are unbalanced quotes or similar.
 */

/*

Primary Bus Operation State Machine

** ** ** NOTE: This is slightly out of date. See the code for the actual logic. ** ** **

from				to				  	where			when
-----------------	-----------------	--------------	-------------------------
unknown				SCS_UNINITIALIZED	-archInit		Initial probe
xxx					SCS_DISCONNECTED	-hwReset		Any hardware reset
xxx					SCS_DISCONNECTED	-hardwareInterrupt	iDisconnect interrupt
-----------------
SCS_DISCONNECTED	SCS_SELECTING		-hwStart		Start of select sequence
SCS_DISCONNECTED	SCS_ACCEPTINGMSG	-fsmDisconnected Valid Reselect detected
-----------------
SCS_SELECTING		SCS_DISCONNECTED	-fsmSelecting	Select Timeout
SCS_SELECTING		SCS_INITIATOR		-fsmSelecting	Select seq. interrupt (maybe incomplete)
SCS_SELECTING		SCS_DISCONNECTED	-fsmSelecting	Reselect during select attempt
-----------------
SCS_INITIATOR		SCS_SENDINGCMD		-fsmPhaseChange	Phase change to CMDO
SCS_INITIATOR		SCS_COMPLETING		-fsmPhaseChange	Phase change to STAT
SCS_INITIATOR		SCS_GETTINGMSG		-fsmPhaseChange	Phase change to MSGI
SCS_INITIATOR		SCS_SENDINGMSG		-fsmPhaseChange	Phase change to MSGO, msg bytes in FIFO
SCS_INITIATOR		SCS_DMACOMPLETE			-dmaStart		DMA setup in DATI/DATO phase
-----------------
SCS_SENDINGCMD		SCS_INITIATOR		-fsmSendingCmd	Good interrupt after sending cmd
-----------------
SCS_COMPLETING		SCS_ACCEPTINGMSG	-fsmCompleting	Good SCMD_INIT_CMD_CMPLT 
SCS_COMPLETING		SCS_INITIATOR		-fsmCompleting	SCMD_INIT_CMD_CMPLT, but only STS byte.
-----------------
SCS_GETTINGMSG		SCS_ACCEPTINGMSG	-fsmGettingMsg	SCMD_MSG_ACCEPTED cmd sent
-----------------
SCS_ACCEPTINGMSG	SCS_GETTINGMSG		-fsmAcceptingMsg	Phase still MSGI
SCS_ACCEPTINGMSG	SCS_INITIATOR		-fsmAcceptingMsg	Phase != MSGI
SCS_ACCEPTINGMSG	SCS_DISCONNECTED	-fsmAcceptingMsg	Msg = Cmd complete
SCS_ACCEPTINGMSG	SCS_DISCONNECTED	-fsmAcceptingMsg	Msg = disconnect
SCS_ACCEPTINGMSG	SCS_INITIATOR		-fsmAcceptingMsg	Any other msg
-----------------
SCS_DMACOMPLETE			SCS_INITIATOR		-fsmDMAComplete		DMA complete
-----------------
SCS_SENDINGMSG		SCS_INITIATOR		-fsmSendingMsg	Always


Message Out State

The purpose of this is to provide a means to send a message out to the target. Basically,
to do a message out:

  -- Set ATN true
  -- Place message byte(s) in currMsgOut[]
  -- Set currMsgOutCnt to length of message 
  -- Set msgOutState = MOS_WAITING

All of these are done in -messageOut: for a single-byte message. For extended messages,
context-specific code must perform these manually. 

When we see a phase change to message out, -fsmPhaseChange sends the message byte(s) from
currMsgOut[] to the FIFO, does a SCMD_TRANSFER_INFO, and sets msgOutState to MOS_SAWMSGOUT.
We go back to MOS_NONE when we see any other phase. 

from				to				  	where			when
-----------------	-----------------	--------------	-------------------------
xxx					MOS_NONE			-hwReset		Hardware reset
xxx					MOS_NONE			-hwStart		Start of select sequence
MOS_NONE			MOS_WAITING			-fsmSelecting	Select done, set ATN, sending SDTR msg
MOS_NONE			MOS_WAITING			-fsmAcceptingMsg Target-init SDTR, we set ATN for our SDTR
MOS_NONE			MOS_WAITING			-messageOut		Set ATN to do a single-byte msg out 
MOS_WAITING			MOS_SAWMSGOUT		-fsmPhaseChange	Phase change to MSGO, our bytes in FIFO.
MOS_SAWMSGOUT		MOS_NONE			-fsmPhaseChange	Phase != msg in or msg out


This state machine keeps track of SDTR negotiations, both target- and host-initiated.

from				to				  	where			when
-----------------	-----------------	--------------	-------------------------
xxx				  SNS_NONE				-hwReset		Hardware reset
xxx				  SNS_HOST_INIT_NEEDED	-hwStart		We want to do SDTR 
xxx				  SNS_NONE				-hwStart		We don't want to do SDTR
SNS_HOST_INIT_NEEDED	SNS_NONE		-fsmSelecting	No message phase after Sel complete
SNS_HOST_INIT_NEEDED	SNS_HOST_INIT	-fsmPhaseChange	Phase = MSGO after selection
SNS_HOST_INIT		SNS_NONE			-fsmAcceptingMsg Target rejected our SDTR.
SNS_HOST_INIT		SNS_NONE			-fsmAcceptingMsg Target's SDTR response recv'd
SNS_HOST_INIT		SNS_NONE			-fsmPhaseChange  No MSGIn after our SDTR sent.
SNS_NONE			SNS_TARGET_INIT		-fsmGettingMsg	Saw target-init'd SDTR
SNS_TARGET_INIT	  SNS_NONE				-fsmSendingMsg	Completed target-init'd SDTR

*/