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



SInt8	cmdLengthTable[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0x00 - 0x0F
									// 0x10 - 0x1F
	1,							// 0x10 Subsystem Power/Clock Control
	1,							// 0x11 Subsystem Power/Clock Control (yet more)
	-1,-1,-1,-1,-1,-1,					// 0x12 - 0x17
	0,							// 0x18 Read Power/Clock Status
	0,							// 0x19 Read Power/Clock Status (yet more)
	-1,-1,-1,-1,-1,						// 0x1A
	0,							// 0x1F RESERVED FOR MSC/PG&E EMULATION
									// 0x20 - 0x2F
	-1,							// 0x20 Set New Apple Desktop Bus Command
	0,							// 0x21 ADB Autopoll Abort
	2,							// 0x22 ADB Set Keyboard Addresses
	1,							// 0x23 ADB Set Hang Threshold
	1,							// 0x24 ADB Enable/Disable Programmers Key
	-1,-1,-1,						// 0x25
	0,							// 0x28 ADB Transaction Read
	-1,-1,-1,-1,-1,-1,-1,					// 0x29
				      					// 0x30 - 0x3F
	4,							// 0x30 Set Realtime Clock.
	20,							// 0x31 Write Parameter RAM
	-1,							// 0x32 Write Extended Parameter RAM.
	3,							// 0x33 Write NVRAM
	-1,-1,-1,-1,						// 0x34
	0,							// 0x38 Read Realtime Clock.
	0,							// 0x39 Read Parameter RAM
	2,							// 0x3A Read Extended Parameter RAM.
	2,							// 0x3B Read NVRAM
	-1,-1,-1,-1,						// 0x3C
									// 0x40 - 0x4F
	1,							// 0x40 Set Screen Contrast
	1,							// 0x41 Set Screen Brightness
	-1,-1,-1,-1,-1,-1,					// 0x42
	0,							// 0x48 Read Screen Contrast
	0,							// 0x49 Read Screen Brightness
	-1,-1,							// 0x4A
	1,							// 0x4C PCMCIA card eject
	-1,-1,-1,						// 0x4D
									// 0x50 - 0x5F
	1,							// 0x50 Set Internal Modem Control Bits
	0,							// 0x51 Clear FIFOs
	2,							// 0x52 Set FIFO Interrupt Marks
	2,							// 0x53 Set FIFO Sizes
	-1,							// 0x54 Write Data to Modem
	1,							// 0x55 Set Data Mode
	3,							// 0x56 Set Flow Control Mode
	1,							// 0x57 Set DAA control lines
	0,							// 0x58 Read Internal Modem Status
	1,							// 0x59 Get DAA Identification
	0,							// 0x5A Get FIFO Counts
	0,							// 0x5B Get Maximum FIFO Sizes
	0,							// 0x5C Read Data From Modem
	-1,							// 0x5D General Purpose modem command (modem dependent)
	-1,-1,							// 0x5E
									// 0x60 - 0x6F
	2,							// 0x60 Set low power warning and cutoff levels
	-1,							// 0x61
	2,							// 0x62 Set low power first dialog and 10 second warning levels
	0,							// 0x63 Get low power first dialog and 10 second warning levels
	-1,-1,-1,-1,						// 0x64
        0,							// 0x68 Read Charger State, Battery Voltage, Temperature
	0,							// 0x69 Read Instantaneous Charger, Battery, Temperature
  	0,							// 0x6A Read low power warning and cutoff levels
	0,							// 0x6B Read Extended Battery Status
	0,							// 0x6C Read Battery ID
	0,							// 0x6D Battery Parameters
	-1,-1,							// 0x6E
									// 0x70 - 0x7F
	1,							// 0x70 Set One-Second Interrupt
	1,							// 0x71 Modem Interrupt Control
	1,							// 0x72 Set Modem Interrupt
	-1,-1,-1,-1,-1,						// 0x73
	0,							// 0x78 Read Interrupt Flag Register.
	0,							// 0x79 Read Modem Interrupt Data
	-1,-1,-1,-1,						// 0x7A
	4,							// 0x7E Enter Shutdown Mode
	4,							// 0x7F Enter Sleep Mode
									// 0x80 - 0x8F
	4,							// 0x80 Set Wakeup Timer
	-1,							// 0x81
	0,							// 0x82 Disable Wakeup Timer
	-1,-1,-1,-1,-1,						// 0x83
	0,							// 0x88 Read Wakeup Timer
	-1,-1,-1,-1,-1,-1,-1,// 0x89
 									// 0x90 - 0x9F
	1,							// 0x90 Set Sound Control Bits
	2,							// 0x91 Set DFAC Control Register
	-1,-1,-1,-1,-1,-1,					// 0x92
	0,							// 0x98 Read Sound Control Status
	0,							// 0x99 Read DFAC Control Register
	-1,-1,-1,-1,-1,-1,					// 0x9A
									// 0xA0 - 0xAF
	2,							// 0xA0 Write Modem Register
	2,							// 0xA1 Clear Modem Register Bits
	2,							// 0xA2 Set Modem Register Bits
	4,							// 0xA3 Write DSP RAM
	-1,							// 0xA4 Set Filter Coefficients
	0,							// 0xA5 Reset Modem
	-1,-1,							// 0xA6
	1,							// 0xA8 Read Modem Register
	1,							// 0xA9 Send Break
	3,							// 0xAA Dial Digit
	2,							// 0xAB Read DSP RAM
	-1,-1,-1,-1,						// 0xAC
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xB0 - 0xBF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xC0 - 0xCF
									// 0xD0 - 0xDF
	0,							// 0xD0 Reset CPU
	-1,-1,-1,-1,-1,-1,-1,					// 0xD1
	1,							// 0xD8 Read A/D Status
	1,							// 0xD9 Read User Input
	-1,-1,							// 0xDA
	0,							// 0xDC read external switches
	0,							// 0xDD -
	-1,-1,							// 0xDE
									// 0xE0 - 0xEF
	-1,							// 0xE0 Write to internal PMGR memory
	4,							// 0xE1 Download Flash EEPROM Code
	0,							// 0xE2 Get Flash EEPROM Status
	-1,-1,-1,-1,-1,						// 0xE3
	3,							// 0xE8 Read PMGR internal memory
	-1,							// 0xE9 -
	0,							// 0xEA Read PMGR firmware version number
	-1,							// 0xEB -
	0,							// 0xEC Execute self test
	-1,							// 0xED PMGR diagnostics (selector-based)
	-1,							// 0xEE -
	0,							// 0xEF PMGR soft reset
									// 0xF0 - 0xFF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};



//  This table is used to determine how to handle the reply:

//=0:no reply should be expected.
//=1: only a reply byte will be sent (this is a special case for a couple of commands)
//<0:a reply is expected and the PMGR will send a count byte.
//>1:a reply is expected and the PMGR will not send a count byte,
//but the count will be (value-1).
//
//Unused commands in the range $x8 to $xF will be marked as expecting a reply (with count)
//so that commands may be added without having to change the ROM.

SInt8	rspLengthTable[256] = {
									// 0x00 - 0x0F
	0,0,0,0,0,0,0,0,				 	// 0x00 -
	-1,-1,-1,-1,-1,-1,-1,-1,				// 0x08 -
									// 0x10 - 0x1F
	0,							// 0x10 Subsystem Power/Clock Control
	0,							// 0x11 Subsystem Power/Clock Control (yet more)
	0,0,0,0,0,0,						// 0x12 -
	1+1,							// 0x18 Read Power/Clock Status
	1+1,							// 0x19 Read Power/Clock Status (yet more)
	-1,-1,-1,-1,-1,						// 0x1A -
	0,							// 0x1F RESERVED FOR MSC/PG&E EMULATION
									// 0x20 - 0x2F
	0,							// 0x20 Set New Apple Desktop Bus Command
	0,							// 0x21 ADB Autopoll Abort
	0,							// 0x22 ADB Set Keyboard Addresses
	0,							// 0x23 ADB Set Hang Threshold
	0,							// 0x24 ADB Enable/Disable Programmers Key
	0,0,0,							// 0x25 -
	-1,							// 0x28 ADB Transaction Read
	-1,-1,-1,-1,-1,-1,-1,					// 0x29 -
									// 0x30 - 0x3F
	0,							// 0x30 Set Realtime Clock.
	0,							// 0x31 Write Parameter RAM
	0,							// 0x32 Write Extended Parameter RAM.
	0,							// 0x33 Write NVRAM
	0,0,0,0,						// 0x34 -
	4+1,							// 0x38 Read Realtime Clock.
	20+1,							// 0x39 Read Parameter RAM
	-1,							// 0x3A Read Extended Parameter RAM.
	1+1,							// 0x3B Read NVRAM
	-1,-1,-1,-1,						// 0x3C -
									// 0x40 - 0x4F
	0,							// 0x40 Set Screen Contrast
	0,							// 0x41 Set Screen Brightness
	0,0,0,0,0,0,						// 0x42 -
	1+1,							// 0x48 Read Screen Contrast
	1+1,							// 0x49 Read Screen Brightness
	-1,-1,							// 0x4A -
	0,							// 0x4C PCMCIA card eject
	-1,-1,-1,						// 0x4D -
									// 0x50 - 0x5F
	0,							// 0x50 Set Internal Modem Control Bits
	0,							// 0x51 Clear FIFOs
	0,							// 0x52 Set FIFO Interrupt Marks
	0,							// 0x53 Set FIFO Sizes
	0,							// 0x54 Write Data to Modem
	0,							// 0x55 Set Data Mode
	0,							// 0x56 Set Flow Control Mode
	0,							// 0x57 Set DAA control lines
	1+1,							// 0x58 Read Internal Modem Status
	0,							// 0x59 Get DAA Identification
	2+1,							// 0x5A Get FIFO Counts
	2+1,							// 0x5B Get Maximum FIFO Sizes
	-1,							// 0x5C Read Data From Modem
	-1,							// 0x5D General Purpose modem command (modem dependent)
	-1,-1,							// 0x5E -
									// 0x60 - 0x6F
	0,							// 0x60 Set low power warning and cutoff levels
	0,							// 0x61 -
	0,							// 0x62 Set low power first dialog and 10 second warning levels 
	2+1,							// 0x63 Get low power first dialog and 10 second warning levels
	0,0,0,0,						// 0x64 -
	3+1,							// 0x68 Read Charger State, Battery Voltage, Temperature
	3+1,							// 0x69 Read Instantaneous Charger, Battery, Temperature
	2+1,							// 0x6A Read low power warning and cutoff levels
	8+1,							// 0x6B Read Extended Battery Status
	-1,							// 0x6C Read Battery ID
	-1,							// 0x6D Battery Parameters (10+1 for AJ, 22+1 for Malcolm)
	-1,-1,							// 0x6E -
									// 0x70 - 0x7F
	0,							// 0x70 Set One-Second Interrupt
	0,							// 0x71 Modem Interrupt Control
	0,							// 0x72 Set Modem Interrupt
	0,0,0,0,0,						// 0x73 -
	-1,							// 0x78 Read Interrupt Flag Register.
	-1,							// 0x79 Read Modem Interrupt Data
	-1,-1,-1,-1,						// 0x7A -
	0+1,							// 0x7E Enter Shutdown Mode
	0+1,							// 0x7F Enter Sleep Mode
									// 0x80 - 0x8F
	0,							// 0x80 Set Wakeup Timer
	0,							// 0x81 -
	0,							// 0x82 Disable Wakeup Timer
	0,0,0,0,0,						// 0x83 -
	5+1,							// 0x88 Read Wakeup Timer
	-1,-1,-1,-1,-1,-1,-1,					// 0x89 -
									// 0x90 - 0x9F
	0,							// 0x90 Set Sound Control Bits
	0,							// 0x91 Set DFAC Control Register
	0,0,0,0,0,0,						// 0x92 -
	1+1,							// 0x98 Read Sound Control Status
	1+1,							// 0x99 Read DFAC Control Register
	1,-1,-1,-1,-1,-1,					// 0x9A -
									// 0xA0 - 0xAF
	0,							// 0xA0 Write Modem Register
	0,							// 0xA1 Clear Modem Register Bits
	0,							// 0xA2 Set Modem Register Bits
	0,							// 0xA3 Write DSP RAM
	0,							// 0xA4 Set Filter Coefficients
	0,							// 0xA5 Reset Modem
	0,0,							// 0xA6 -
	1+1,							// 0xA8 Read Modem Register
	0,							// 0xA9 Send Break
	0,							// 0xAA Dial Digit
	0,							// 0xAB Read DSP RAM
	-1,-1,-1,-1,						// 0xAC -
									// 0xB0 - 0xBF
	0,0,0,0,0,0,0,0,					// 0xB0 -
	-1,-1,-1,-1,-1,-1,-1,-1,				// 0xB8 -
									// 0xC0 - 0xCF
	0,0,0,0,0,0,0,0,					// 0xC0 -
	-1,-1,-1,-1,-1,-1,-1,-1,				// 0xC8 -
									// 0xD0 - 0xDF
	0,0,0,0,0,0,0,0,					// 0xD0 Reset CPU
	1+1,							// 0xD8 Read A/D Status
	1+1,							// 0xD9 Read User Input
	-1,-1,							// 0xDA -
	1+1,							// 0xDC read external switches
	-1,-1,-1,						// 0xDD -
									// 0xE0 - 0xEF
	0,							// 0xE0 Write to internal PMGR memory
	0,							// 0xE1 Download Flash EEPROM Code
	0+1,							// 0xE2 Get Flash EEPROM Status
	0,0,0,0,0,						// 0xE3 -
	-1,							// 0xE8 Read PMGR internal memory
	-1,							// 0xE9 -
	1+1,							// 0xEA Read PMGR firmware version number
	-1,							// 0xEB -
	-1,							// 0xEC Execute self test
	-1,							// 0xED PMGR diagnostics (selector-based)
	-1,							// 0xEE -
	0,							// 0xEF PMGR soft reset
									// 0xF0 - 0xFF
	0,0,0,0,0,0,0,0,					// 0xF0 -
	-1,-1,-1,-1,-1,-1,-1,-1					// 0xF8 -
};


