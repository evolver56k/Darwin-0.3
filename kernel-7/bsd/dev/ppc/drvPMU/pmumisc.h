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

@protocol MiscService

- (PMUStatus)sendMiscCommand    :(UInt32)Command
				:(UInt32)SLength
				:(UInt8 *)SBuffer
				:(UInt8 *)RBuffer
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback;

@end


enum {
        kPMUpowerCntl           = 0x10, // power plane/clock control
        kPMUpower1Cntl          = 0x11, // more power control (DBLite)
        kPMUpowerRead           = 0x18, // power plane/clock status
        kPMUpower1Read          = 0x19, // more power status (DBLite)

        kPMUpMgrADB             = 0x20,  // send ADB command
        kPMUpMgrADBoff          = 0x21,  // turn ADB auto-poll off
        kPMUreadADB             = 0x28,  // Apple Desktop Bus
        kPMUpMgrADBInt          = 0x2F,  // get ADB interrupt data (Portable only)

        kPMUtimeWrite           = 0x30,  // write the time to the clock chip
        kPMUpramWrite           = 0x31,  // write the original 20 bytes of PRAM (Portable only)
        kPMUxPramWrite          = 0x32,  // write extended PRAM byte(s)
	kPMUNVRAMWrite		= 0x33,	 // write NVRAM byte
        kPMUtimeRead            = 0x38,  // read the time from the clock chip
        kPMUpramRead            = 0x39,  // read the original 20 bytes of PRAM (Portable only)
        kPMUxPramRead           = 0x3A,  // read extended PRAM byte(s)
	kPMUNVRAMRead		= 0x3B,  // read NVRAM byte
        kPMUSetContrast         = 0x40,  // set screen contrast
        kPMUReadContrast        = 0x48,  // read the contrast value
        kPMUReadBrightness      = 0x49,  // read the brightness value
        kPMUDoPCMCIAEject       = 0x4C,  // eject PCMCIA card(s)
        kPMUDoMediaBayDisp      = 0x4D,  // (MS 5/17/96) Get Media bay device status
        kPMUDisplayDisp         = 0x4F,  // Get raw Contrast numbers
        kPMUmodemSet            = 0x50,  // internal modem control
        kPMUmodemClrFIFO        = 0x51,  // clear modem fifo's
        kPMUmodemSetFIFOIntMask = 0x52,  // set the mask for fifo interrupts
        kPMUmodemWriteData      = 0x54,  // write data to modem
        kPMUmodemSetDataMode    = 0x55,  //
        kPMUmodemSetFloCtlMode  = 0x56,  //
        kPMUmodemDAACnt         = 0x57,  //
        kPMUmodemRead           = 0x58,  // internal modem status
        kPMUmodemDAAID          = 0x59,  //
        kPMUmodemGetFIFOCnt     = 0x5A,  //
        kPMUmodemSetMaxFIFOSize = 0x5B,  //
        kPMUmodemReadFIFOData   = 0x5C,  //
        kPMUmodemExtend         = 0x5D,  //

        kPMUsetBattWarning      = 0x60,  // set low power warning and cutoff battery levels (PowerBook 140/170, DBLite)
        kPMUsetCutoff           = 0x61,  // set hardware cutoff voltage<H44>
        kPMUnewSetBattWarn      = 0x62,  // set low power warning and 10 second battery levels (Epic/Mustang)
        kPMUnewGetBattWarn      = 0x63,  // get low power warning and 10 second battery levels (Epic/Mustang)
        kPMUbatteryRead         = 0x68,  // read battery/charger level and status
        kPMUbatteryNow          = 0x69,  // read battery/charger instantaneous level and status
        kPMUreadBattWarning     = 0x6A,  // read low power warning and cutoff battery levels (PowerBook 140/170, DBLite)
        kPMUreadExtBatt         = 0x6B,  // read extended battery/charger level and status (DBLite)
        kPMUreadBatteryID       = 0x6C,  // read the battery ID
        kPMUreadBatteryInfo     = 0x6D,  // return battery parameters
        kPMUGetSOB              = 0x6F,  // Get Smarts of Battery

        kPMUSetModem1SecInt     = 0x70,  //
        kPMUSetModemInts        = 0x71,  // turn modem interrupts on/off
        kPMUreadINT             = 0x78,  // get PMGR interrupt data
        kPMUReadModemInts       = 0x79,  // read modem interrupt status
        kPMUPmgrPWRoff          = 0x7E,  // turn system power off
        kPMUsleepReq            = 0x7F,  // put the system to sleep (sleepSig='MATT')
        kPMUsleepAck            = 0x70,  // sleep acknowledge

        kPMUtimerSet            = 0x80,  // set the wakeup timer
        kPMUtimerRead           = 0x88,  // read the wakeup timer setting

        kPMUsoundSet            = 0x90,  // sound power control
        kPMUSetDFAC             = 0x91,  // set DFAC register (DBLite)
        kPMUsoundRead           = 0x98,  // read sound power state
        kPMUReadDFAC            = 0x99,  // read DFAC register (DBLite)

        kPMUmodemWriteReg       = 0xA0,  // Write Modem Register
        kPMUmodemClrRegBits     = 0xA1,  // Clear Modem Register Bits
        kPMUmodemSetRegBits     = 0xA2,  // Set Modem Register Bits
        kPMUmodemWriteDSPRam    = 0xA3,  // Write DSP RAM
        kPMUmodemSetFilterCoeff = 0xA4,  // Set Filter Coefficients
        kPMUmodemReset          = 0xA5,  // Reset Modem
        kPMUmodemUNKNOWN        = 0xA6,  // <filler for now>
        kPMUmodemReadReg        = 0xA8,  // Read Modem Register
        kPMUmodemReadDSPRam     = 0xAB,  // Read DSP RAM

        kPMUresetCPU            = 0xD0,  // reset the CPU
        kPMUreadAtoD            = 0xD8,  // read A/D channel
        kPMUreadButton          = 0xD9,  // read button values on Channel 0 = Brightness, Channel 1 = Contrast 0-31
        kPMUreadExtSwitches     = 0xDC,  // read external switch status (DBLite)

        kPMUwritePmgrRAM        = 0xE0,  // write to internal PMGR RAM
        kPMUdownloadFlash       = 0xE1,  // download Flash memory
        kPMUsetMachineAttr      = 0xE3,  // set machine id
        kPMUreadPmgrRAM         = 0xE8,  // read from internal PMGR RAM
        kPMUreadPmgrVers        = 0xEA,  // read the PMGR version number
        kPMUreadMachineAttr     = 0xEB,  // read the machine id
        kPMUPmgrSelfTest        = 0xEC,  // run the PMGR selftest
        kPMUDBPMgrTest          = 0xED,  // DON'T USE THIS!!
        kPMUFactoryTest         = 0xEE,  // hook for factory requests
        kPMUPmgrSoftReset       = 0xEF   // soft reset of the PMGR
};
