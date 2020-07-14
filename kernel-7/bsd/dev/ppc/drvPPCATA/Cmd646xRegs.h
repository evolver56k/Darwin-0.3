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
 *	PCI Control registers for Cmd646X chipset 
 *
 */
#define	kCmd646xCFR				0x50		/* Configuration */
#define kCmd646xCFR_DSA1			0x40
#define kCmd646xCFR_IDEIntPRI			0x04

#define kCmd646xCNTRL				0x51		/* Drive 0/1 Control Register */
#define kCmd646xCNTRL_Drive1ReadAhead		0x80
#define kCmd646xCNTRL_Drive0ReadAhead		0x40
#define kCmd646xCNTRL_EnableSDY			0x08
#define kCmd646xCNTRL_EnablePRI			0x04

#define kCmd646xCMDTIM				0x52		/* Task file timing (all drives) */
#define kCmd646xCMDTIM_Drive01CmdActive		0xF0
#define kCmd646xCMDTIM_Drive01CmdRecovery	0x0F

#define kCmd646xARTTIM0				0x53		/* Drive 0 Address Setup */
#define kCmd646xARTTIM0_Drive0AddrSetup		0xC0

#define kCmd646xDRWTIM0				0x54		/* Drive 0 Data Read/Write - DACK Time	*/
#define kCmd646xDRWTIM0_Drive0DataActive	0xF0
#define kCmd646xDRWTIM0_Drive0DataRecovery	0x0F

#define kCmd646xARTTIM1				0x55		/* Drive 1 Address Setup */
#define kCmd646xARTTIM1_Drive1AddrSetup		0xC0

#define kCmd646xDRWTIM1				0x56		/* Drive 1 Data Read/Write - DACK Time */
#define kCmd646xDRWTIM1_Drive1DataActive	0xF0
#define kCmd646xDRWTIM1_Drive1DataRecover	0x0F

#define kCmd646xARTTIM23			0x57		/* Drive 2/3 Control/Status */
#define kCmd646xARTTIM23_AddrSetup		0xC0
#define kCmd646xARTTIM23_IDEIntSDY		0x10
#define kCmd646xARTTIM23_Drive3ReadAhead	0x08
#define kCmd646xARTTIM23_Drive2ReadAhead	0x04

#define kCmd646xDRWTIM2				0x58		/* Drive 2 Read/Write - DACK Time */
#define kCmd646xDRWTIM2_Drive2DataActive	0xF0	
#define kCmd646xDRWTIM2_Drive2DataRecovery	0x0F

#define kCmd646xBRST				0x59		/* Read Ahead Count */

#define kCmd646xDRWTIM3				0x5B		/* Drive 3 Read/Write - DACK Time */
#define kCmd646xDRWTIM3_Drive3DataActive	0xF0
#define kCmd646xDRWTIM3_Drive3DataRecover	0x0F

#define kCmd646xBMIDECR0			0x70		/* BusMaster Command Register - Primary */
#define kCmd646xBMIDECR0_PCIWritePRI		0x08
#define kCmd646xBMIDECR0_StartDMAPRI		0x01

#define kCmd646xMRDMODE				0x71		/* DMA Master Read Mode Select */
#define kCmd646xMRDMODE_PCIReadMask		0x03
#define kCmd646xMRDMODE_PCIRead			0x00
#define kCmd646xMRDMODE_PCIReadMultiple		0x01
#define kCmd646xMRDMODE_IDEIntPRI		0x04
#define kCmd646xMRDMODE_IDEIntSDY		0x08
#define kCmd646xMRDMODE_IntEnablePRI		0x10
#define kCmd646xMRDMODE_IntEnableSDY		0x20
#define kCmd646xMRDMODE_ResetAll		0x40

#define kCmd646xBMIDESR0			0x72		/* BusMaster Status Register - Primary */
#define kCmd646xBMIDESR0_Simplex		0x80
#define kCmd646xBMIDESR0_Drive1DMACap		0x40
#define kCmd646xBMIDESR0_Drive0DMACap		0x20
#define kCmd646xBMIDESR0_DMAIntPRI		0x04
#define kCmd646xBMIDESR0_DMAErrorPRI		0x02
#define kCmd646xBMIDESR0_DMAActivePRI		0x01

#define kCmd646xUDIDETCR0			0x73		/* Ultra DMA Timing Control Register - Primary */
#define kCmd646xUDIDETCR0_Drive1UDMACycleTime	0xC0
#define kCmd646xUDIDETCR0_Drive0UDMACycleTime	0x30
#define kCmd646xUDIDETCR0_Drive1UDMAEnable	0x02
#define kCmd646xUDIDETCR0_Drive0UDMAEnable	0x01

#define kCmd646xDTPR0				0x74		/* Descriptor Table Pointer - Primary */

#define kCmd646xBMIDECR1			0x78		/* BusMaster Command Register - Secondary */
#define kCmd646xBMIDECR1_PCIWriteSDY		0x08
#define kCmd646xBMIDECR1_StartDMASDY		0x01

#define kCmd646xBMIDESR1			0x7A		/* BusMaster Status Register - Secondary */
#define kCmd646xBMIDESR1_Simplex		0x80
#define kCmd646xBMIDESR1_Drive3DMACap		0x40
#define kCmd646xBMIDESR1_Drive2DMACap		0x20
#define kCmd646xBMIDESR1_DMAIntSDY		0x04
#define kCmd646xBMIDESR1_DMAErrorSDY		0x02
#define kCmd646xBMIDESR1_DMAActiveSDY		0x01

#define kCmd646xUDIDETCR1			0x7B		/* Ultra DMA Timing Control Register - Secondary */
#define kCmd646xUDIDETCR1_Drive3UDMACycleTime	0xC0
#define kCmd646xUDIDETCR1_Drive2UDMACycleTime	0x30
#define kCmd646xUDIDETCR1_Drive3UDMAEnable	0x02
#define kCmd646xUDIDETCR1_Drive2UDMAEnable	0x01

#define kCmd646xDTPR1				0x7C		/* Descriptor Table Pointer - Primary */

