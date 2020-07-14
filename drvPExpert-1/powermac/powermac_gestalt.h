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

#ifndef _POWERMAC_GESTALT_H_
#define _POWERMAC_GESTALT_H_

/* Machine gestalt's */

enum {
	gestaltClassic				= 1,
	gestaltMacXL				= 2,
	gestaltMac512KE				= 3,
	gestaltMacPlus				= 4,
	gestaltMacSE				= 5,
	gestaltMacII				= 6,
	gestaltMacIIx				= 7,
	gestaltMacIIcx				= 8,
	gestaltMacSE030				= 9,
	gestaltPortable				= 10,
	gestaltMacIIci				= 11,
	gestaltMacIIfx				= 13,
	gestaltMacClassic			= 17,
	gestaltMacIIsi				= 18,
	gestaltMacLC				= 19,
	gestaltQuadra900			= 20,
	gestaltPowerBook170			= 21,
	gestaltQuadra700			= 22,
	gestaltClassicII			= 23,
	gestaltPowerBook100			= 24,
	gestaltPowerBook140			= 25,
	gestaltQuadra950			= 26,
	gestaltMacLCIII				= 27,
	gestaltPerforma450			= gestaltMacLCIII,
	gestaltPowerBookDuo210		= 29,
	gestaltMacCentris650		= 30,
	gestaltPowerBookDuo230		= 32,
	gestaltPowerBook180			= 33,
	gestaltPowerBook160			= 34,
	gestaltMacQuadra800			= 35,
	gestaltMacQuadra650			= 36,
	gestaltMacLCII				= 37,
	gestaltPowerBookDuo250		= 38,
	gestaltAWS9150_80			= 39,
	gestaltPowerMac8100_110		= 40,
	gestaltAWS8150_110			= gestaltPowerMac8100_110,
	gestaltPowerMac5200			= 41,
	gestaltPowerMac6200			= 42,
	gestaltMacIIvi				= 44,
	gestaltMacIIvm				= 45,
	gestaltPerforma600			= gestaltMacIIvm,
	gestaltPowerMac7100_80		= 47,
	gestaltMacIIvx				= 48,
	gestaltMacColorClassic		= 49,
	gestaltPerforma250			= gestaltMacColorClassic,
	gestaltPowerBook165c		= 50,
	gestaltMacCentris610		= 52,
	gestaltMacQuadra610			= 53,
	gestaltPowerBook145			= 54,
	gestaltPowerMac8100_100		= 55,
	gestaltMacLC520				= 56,
	gestaltAWS9150_120			= 57,
	gestaltMacCentris660AV		= 60,
	gestaltPerforma46x			= 62,
	gestaltPowerMac8100_80		= 65,
	gestaltAWS8150_80			= gestaltPowerMac8100_80,
	gestaltPowerMac9500			= 67,
	gestaltPowerMac9600			= gestaltPowerMac9500,
	gestaltPowerMac7500			= 68,
	gestaltPowerMac7600			= gestaltPowerMac7500,
	gestaltPowerMac8500			= 69,
	gestaltPowerMac8600			= gestaltPowerMac8500,
	gestaltPowerBook180c		= 71,
	gestaltPowerBook520			= 72,
	gestaltPowerBook520c		= gestaltPowerBook520,
	gestaltPowerBook540			= gestaltPowerBook520,
	gestaltPowerBook540c		= gestaltPowerBook520,
	gestaltPowerMac5400		= 74,
	gestaltPowerMac6100_60		= 75,
	gestaltAWS6150_60			= gestaltPowerMac6100_60,
	gestaltPowerBookDuo270c		= 77,
	gestaltMacQuadra840AV		= 78,
	gestaltPerforma550			= 80,
	gestaltPowerBook165			= 84,
	gestaltPowerBook190			= 85,
	gestaltMacTV				= 88,
	gestaltMacLC475				= 89,
	gestaltPerforma47x			= gestaltMacLC475,
	gestaltMacLC575				= 92,
	gestaltMacQuadra605			= 94,
	gestaltQuadra630			= 98,
	gestaltPowerMac6100_66		= 100,
	gestaltAWS6150_66		= gestaltPowerMac6100_66,
	gestaltPowerBookDuo280		= 102,
	gestaltPowerBookDuo280c		= 103,
	gestaltPowerMac7200		= 108,
	gestaltPowerMac7300		= 109,
	gestaltPowerMac7100_66		= 112,							/* Power Macintosh 7100/66 */
	gestaltPowerBook150		= 115,
	gestaltPowerBookDuo2300		= 124,
	gestaltPowerBook500PPCUpgrade	= 126,
	gestaltPowerBook5300		= 128,

// RESERVE 306-405 Powerbook products
	gestaltPowerBook3400		= 306,  // gestaltHooper
	gestaltPowerBook2400		= 307,  // gestaltComet
	gestaltPowerBook1400            = 310,  // gestaltEpic
	gestaltMustang			= 311,
	gestaltWallstreet		= 312,
	gestaltKanga			= 313,
	
// RESERVE 406-505 Hi-End Macs
	gestaltCHRP_Version1       	= 406,	// Common Hardware Reference Platform (CHRP), version 1.0
	gestaltWhiteSands		= 407,	// Power Express three-slot MacRISC, Video-In, DEAD
	gestaltPowerExpress		= 408,	// Power Express six-slot MacRISC
	gestaltOakRidgeAV		= 409,	// Power Express six-slot MacRISC, Video-In/Out

// RESERVE 506-605 Low-End Macs
	gestaltTrailBlazerSB1		= 506,	// Trailblazer speed bump 1
	gestaltTrailBlazerSB2		= 507,	// Trailblazer speed bump 2
	gestaltClipper1			= 508,
	gestaltClipper2			= 509,
	gestaltGossamer			= 510,
	gestaltZanzibar			= 511,
	gestaltPowerMac5500             = 512,  // gestaltAvalon
	gestaltPowerMac6500             = 513,  // gestaltGazelleTower
	gestaltPowerMac4400             = 515,
	gestalt20thAnniversary          = gestaltPowerMac5500,
};

#endif /* !defined _POWERMAC_GESTALT_H_ */
