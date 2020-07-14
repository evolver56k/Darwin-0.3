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
/* Copyright (c) 1993 by NeXT Computer, Inc.
 * All rights reserved.
 *
 * IOVPCodeDisplay.m -- vpcode video display driver.
 *
 * 22 July 1993		Derek B Clegg
 * 	Created.
 */

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <driverkit/i386/ioPorts.h>
#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/i386/IOVPCodeDisplay.h>

@interface IOVPCodeDisplay (Private)
- jumpTo:(int)entryPoint withInitialSRegs:(VPInstruction *)initialSRegs;
- setGammaTable;
- _setDefaultGammaTable:(VPInstruction *)transferTable;
- getDisplayInfo;
@end

@implementation IOVPCodeDisplay

- getPixelEncoding
{
    int n, k;
    unsigned char *p;
    IODisplayInfo *displayInfo;
    VPInstruction reg[8];

    if (_debug)
	IOLog("%s: Getting pixel encoding.\n", [self name]);

    if (![self runVPCode:VP_GET_PIXEL_ENCODING withRegs:reg]) {
	IOLog("%s: Can't obtain pixel encoding.\n", [self name]);
	return nil;
    }

    displayInfo = [self displayInfo];
    memset(displayInfo->pixelEncoding, 0, sizeof(displayInfo->pixelEncoding));

    switch (displayInfo->colorSpace) {
    case IO_OneIsBlackColorSpace:
    case IO_OneIsWhiteColorSpace:
	switch (displayInfo->bitsPerPixel) {
	case IO_2BitsPerPixel:
	    for (k = 0; k < 2; k++)
		displayInfo->pixelEncoding[k] = reg[0];
	    break;
	case IO_8BitsPerPixel:
	    for (k = 0; k < 8; k++)
		displayInfo->pixelEncoding[k] = reg[0];
	    break;
	default:
	    IOLog("%s: invalid `bitsPerPixel' (%d) for color space %d.\n", 
		  [self name], displayInfo->bitsPerPixel,
		  displayInfo->colorSpace);
	    return nil;
	}
	break;

    case IO_RGBColorSpace:
	switch (displayInfo->bitsPerPixel) {
	case IO_12BitsPerPixel:
	    if (reg[0] != 'R' || reg[1] != 'G' || reg[2] != 'B'
		|| reg[3] != '-') {
		IOLog("%s: Sorry, only `RRRRGGGGBBBB----' supported for "
		      "`IO_12BitsPerPixel'.\n", [self name]);
		return nil;
	    }
	    strcpy(displayInfo->pixelEncoding, "RRRRGGGGBBBB----");
	    break;
	case IO_15BitsPerPixel:
	    if (reg[0] != '-' || reg[1] != 'R' || reg[2] != 'G'
		|| reg[3] != 'B') {
		IOLog("%s: Sorry, only `-RRRRRGGGGGBBBBB' supported for "
		      "`IO_15BitsPerPixel'.\n", [self name]);
		return nil;
	    }
	    strcpy(displayInfo->pixelEncoding, "-RRRRRGGGGGBBBBB");
	    break;

	case IO_24BitsPerPixel:
	    p = displayInfo->pixelEncoding;
	    for (n = 0; n < 4; n++) {
		for (k = 0; k < 8; k++) {
		    *p++ = reg[n];
		}
	    }
	    break;
	default:
	    IOLog("%s: invalid `bitsPerPixel' (%d) for RGB color space.\n", 
		  [self name], displayInfo->bitsPerPixel);
	    break;
	}
	break;

    default:
	IOLog("%s: Sorry, color space %d is not supported.\n", 
	      [self name], displayInfo->colorSpace);
	return nil;
    }

    if (_debug)
	IOLog("%s: pixelEncoding = `%s'.\n", [self name], 
	      displayInfo->pixelEncoding);

    return self;
}

- getDisplayInfo
{
    IODisplayInfo *displayInfo;
    VPInstruction reg[8];

    /* Verify that the selected mode is valid. */

    if (_debug)
	IOLog("%s: verifying selected mode.\n", [self name]);

    if (![self runVPCode:VP_VERIFY_MODE withRegs:reg]) {
	IOLog("%s: Failed to verify mode.\n", [self name]);
	return nil;
    }
    if (reg[0] != 0) {
	IOLog("%s: Selected mode is invalid.\n", [self name]);
	if (![self runVPCode:VP_SET_DEFAULT_MODE withRegs:0]) {
	    IOLog("%s: Failed to set default mode.\n", [self name]);
	    return nil;
	}
    }

    if (_debug)
	IOLog("%s: Getting display info.\n", [self name]);

    if (![self runVPCode:VP_GET_DISPLAY_INFO withRegs:reg]) {
	IOLog("%s: Failed to obtain display info.\n", [self name]);
	return nil;
    }
    displayInfo = [self displayInfo];
    displayInfo->width = reg[0];
    displayInfo->height = reg[1];
    displayInfo->totalWidth = reg[2];
    displayInfo->rowBytes = reg[3];
    displayInfo->refreshRate = reg[4];
    displayInfo->bitsPerPixel = reg[5];
    displayInfo->colorSpace = reg[6];
    displayInfo->flags = reg[7];

    if ([self getPixelEncoding] == nil)
	return nil;

    IOLog("%s: IOVPCodeDisplay: Initialized.\n", [self name]);
    return self;
}

/* Put the display into linear framebuffer mode. This typically happens
 * when the window server starts running.
 */
- (void)enterLinearMode
{
    IODisplayInfo *displayInfo;
    VPInstruction reg[8];

    /* Set up the chip to use the selected mode. */

    if (_debug)
	IOLog("%s: Initializing video mode.\n", [self name]);

    if (![self runVPCode:VP_INITIALIZE_MODE withRegs:0]) {
	IOLog("%s: Failed to initialize mode.\n", [self name]);
	return;
    }

    /* Set the gamma-corrected gray-scale palette if necessary. */
    [self setGammaTable];

    /* Enter linear mode. */
    if (_debug)
	IOLog("%s: Enabling linear framebuffer: 0x%08x\n", [self name], 
	      _videoRamAddress);

    reg[0] = _videoRamAddress;
    reg[1] = _videoRamSize;
    if (![self runVPCode:VP_ENABLE_LINEAR_FRAMEBUFFER withRegs:reg])
	return;

    /* Clear the screen. */
    displayInfo = [self displayInfo];
    memset(displayInfo->frameBuffer, 0, 
	   displayInfo->rowBytes * displayInfo->height);
}

/* Get the device out of whatever advanced linear mode it was using and back
 * into a state where it can be used as a standard VGA device.
 */
- (void)revertToVGAMode
{
    /* Reset the VGA parameters. */
    if (_debug)
	IOLog("%s: Resetting VGA parameters.\n", [self name]);

    if (![self runVPCode:VP_RESET_VGA withRegs:0])
	return;

    /* Let the superclass do whatever work it needs to do. */
    [super revertToVGAMode];
}

/* Set the brightness to `level'.
 */
- setBrightness:(int)level token:(int)t
{
    if (level < EV_SCREEN_MIN_BRIGHTNESS || level > EV_SCREEN_MAX_BRIGHTNESS) {
	IOLog("QVision: Invalid brightness level `%d'.\n", level);
	return nil;
    }
    brightnessLevel = level;
    [self setGammaTable];
    return self;
}

/* Set the transfer tables.
 */
- setTransferTable:(const unsigned int *)table count:(int)count
{
    int k;

    if (redTransferTable != 0)
	IOFree(redTransferTable, 3 * transferTableCount);

    transferTableCount = count;

    redTransferTable = IOMalloc(3 * count);
    greenTransferTable = redTransferTable + count;
    blueTransferTable = greenTransferTable + count;

    switch ([self displayInfo]->bitsPerPixel) {
    case IO_2BitsPerPixel:
    case IO_8BitsPerPixel:
	for (k = 0; k < count; k++) {
	    redTransferTable[k] = greenTransferTable[k] =
		blueTransferTable[k] = table[k] & 0xFF;
	}
	break;

    case IO_12BitsPerPixel:
    case IO_15BitsPerPixel:
    case IO_24BitsPerPixel:
	for (k = 0; k < count; k++) {
	    redTransferTable[k] = (table[k] >> 24) & 0xFF;
	    greenTransferTable[k] = (table[k] >> 16) & 0xFF;
	    blueTransferTable[k] = (table[k] >> 8) & 0xFF;
	}
	break;

    default:
	IOFree(redTransferTable, 3 * count);
	redTransferTable = 0;
	break;
    }
    [self setGammaTable];
    return self;
}

/* Default gamma precompensation table for color displays.
 * Gamma 2.2 LUT for P22 phosphor displays (Hitachi, NEC, generic VGA) */

static unsigned char gamma2[4] = {
    0, 155, 212, 255,
};

static unsigned char gamma4[16] = {
      0,  74, 102, 123, 140, 155, 168, 180,
    192, 202, 212, 221, 230, 239, 247, 255,
};

static unsigned char gamma5[32] = {
      0,  54,  73,  88, 101, 111, 121, 130,
    138, 145, 152, 159, 166, 172, 178, 183,
    189, 194, 199, 204, 209, 214, 218, 223,
    227, 231, 235, 239, 243, 247, 251, 255,
};

static const unsigned char gamma8[256] = {
      0,  15,  22,  27,  31,  35,  39,  42,  45,  47,  50,  52,
     55,  57,  59,  61,  63,  65,  67,  69,  71,  73,  74,  76,
     78,  79,  81,  82,  84,  85,  87,  88,  90,  91,  93,  94,
     95,  97,  98,  99, 100, 102, 103, 104, 105, 107, 108, 109,
    110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 141, 142, 143, 144, 145,
    146, 147, 148, 148, 149, 150, 151, 152, 153, 153, 154, 155, 
    156, 157, 158, 158, 159, 160, 161, 162, 162, 163, 164, 165,
    165, 166, 167, 168, 168, 169, 170, 171, 171, 172, 173, 174,
    174, 175, 176, 177, 177, 178, 179, 179, 180, 181, 182, 182,
    183, 184, 184, 185, 186, 186, 187, 188, 188, 189, 190, 190, 
    191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 198, 198,
    199, 200, 200, 201, 201, 202, 203, 203, 204, 205, 205, 206, 
    206, 207, 208, 208, 209, 210, 210, 211, 211, 212, 213, 213,
    214, 214, 215, 216, 216, 217, 217, 218, 218, 219, 220, 220, 
    221, 221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227,
    228, 228, 229, 229, 230, 230, 231, 231, 232, 233, 233, 234, 
    234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 240, 240,
    241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 
    247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252,
    253, 253, 254, 255, 
};

- _setDefaultGammaTable:(VPInstruction *)transferTable
{
    unsigned int k, g, v;
    const IODisplayInfo *displayInfo;

    displayInfo = [self displayInfo];

    switch (displayInfo->bitsPerPixel) {
    case IO_2BitsPerPixel:
	for (g = 0; g < 4; g++) {
	    v = EV_SCALE_BRIGHTNESS(brightnessLevel, gamma2[g]);
	    v = (v << 16) | (v << 8) | v;
	    for (k = 0; k < 64; k++) {
		*transferTable++ = v;
	    }
	}
	break;

    case IO_12BitsPerPixel:
	for (g = 0; g < 16; g++) {
	    v = EV_SCALE_BRIGHTNESS(brightnessLevel, gamma4[g]);
	    v = (v << 16) | (v << 8) | v;
	    for (k = 0; k < 16; k++) {
		*transferTable++ = v;
	    }
	}
	break;

    case IO_15BitsPerPixel:
	for (g = 0; g < 32; g++) {
	    v = EV_SCALE_BRIGHTNESS(brightnessLevel, gamma5[g]);
	    v = (v << 16) | (v << 8) | v;
	    for (k = 0; k < 8; k++)
		*transferTable++ = v;
	}
	break;

    case IO_8BitsPerPixel:
    case IO_24BitsPerPixel:
	for (g = 0; g < 256; g++) {
	    v = EV_SCALE_BRIGHTNESS(brightnessLevel, gamma8[g]);
	    *transferTable++ = (v << 16) | (v << 8) | v;
	}
	break;

    default:
	return self;
    }
    if (![self runVPCode:VP_SET_TRANSFER_TABLE withRegs:0])
	return nil;
    return self;
}

- setGammaTable
{
    unsigned int i, j, r, g, b, v, *transferTable;

    if (_debug)
	IOLog("%s: setting transfer table.\n", [self name]);

    if (_vpCode == 0 || _vpCodeCount <= 0 
	|| VP_TRANSFER_TABLE >= _vpCodeCount) {
	IOLog("%s: Can't set transfer table: no vpcode present.\n",
	      [self name]);
	return nil;
    }
    v = _vpCode[VP_TRANSFER_TABLE];
    if (v >= _vpCodeCount) {
	IOLog("%s: transfer table address is out of range: 0x%x.\n",
	      [self name], v);
	return nil;
    }
    if (v == 0)
	return self;

    if (_debug)
	IOLog("%s: using transfer table at 0x%08x.\n", [self name], v);

    transferTable = &_vpCode[v];

    if (redTransferTable == 0)
	return [self _setDefaultGammaTable:transferTable];

    for (i = 0; i < transferTableCount; i++) {
	for (j = 0; j < 256/transferTableCount; j++) {
	    r = EV_SCALE_BRIGHTNESS(brightnessLevel, redTransferTable[i]);
	    g = EV_SCALE_BRIGHTNESS(brightnessLevel, greenTransferTable[i]);
	    b = EV_SCALE_BRIGHTNESS(brightnessLevel, blueTransferTable[i]);
	    *transferTable++ = (r << 16) | (g << 8) | b;
	}
    }

    if (_debug) {
	transferTable = &_vpCode[v];
	IOLog("%s: Transfer table:\n", [self name]);
	for (i = 0; i < 64; i++) {
	    IOLog("%s: ", [self name]);
	    for (j = 0; j < 4; j++) {
		IOLog("%08x ", *transferTable++);
	    }
	    IOLog("\n");
	}
    }

    if (![self runVPCode:VP_SET_TRANSFER_TABLE withRegs:0])
	return nil;
    return self;
}

- initFromDeviceDescription:deviceDescription
{
    const IORange *range;
    IODisplayInfo *displayInfo;

    if ([super initFromDeviceDescription:deviceDescription] == nil)
	return [super free];

    _debug = NO;

    range = [deviceDescription memoryRangeList];
    if (range == 0) {
	IOLog("%s: No memory range set.\n", [self name]);
	return [super free];
    }
    _videoRamAddress = range[0].start;
    _videoRamSize = range[0].size;

    redTransferTable = greenTransferTable = blueTransferTable = 0;
    transferTableCount = 0;
    brightnessLevel = EV_SCREEN_MAX_BRIGHTNESS;

    displayInfo = [self displayInfo];
    displayInfo->frameBuffer =
	(void *)[self mapFrameBufferAtPhysicalAddress:0 length:0];
    if (displayInfo->frameBuffer == 0)
	return [self free];

    if (_debug) {
	IOLog("Video Ram Address = 0x%08x\n", _videoRamAddress);
	IOLog("Framebuffer Address = 0x%08x\n",
	      (unsigned int)displayInfo->frameBuffer);
    }
    return self;
}

- jumpTo:(int)entryPoint withInitialSRegs:(VPInstruction *)initialSRegs
{
    VPInstruction reg[16];	/* P-machine register set */
    VPInstruction immediate;	/* Immediate value for some instructions. */
    VPInstruction instruction;	/* The current instruction */
    unsigned int port;		/* The port for ins and outs. */
    BOOL zero;			/* Zero condition code. */
    BOOL positive;		/* Positive condition code. */
    BOOL negative;		/* Negative condition code. */
    int pc;			/* The program counter. */

    if (_vpCode == 0 || _vpCodeCount <= 0) {
	IOLog("%s: No vpcode to run.\n", [self name]);
	return nil;
    }

    /* Set up the initial registers. */

    if (initialSRegs == 0) {
	memset(&reg[0], 0, 16 * sizeof(reg[0]));
    } else {
	memset(&reg[0], 0, 8 * sizeof(reg[0]));
	memcpy(&reg[8], initialSRegs, 8 * sizeof(reg[0]));
    }

    immediate = 0;
    zero = positive = negative = NO;
    pc = entryPoint;

    while (1) {
	if (pc < 0 || pc >= _vpCodeCount) {
	    IOLog("%s: program counter is out of range: 0x%x.\n",
		  [self name], (unsigned)pc);
	    return nil;
	}
	instruction = _vpCode[pc++];
	if (VP_IMMEDIATE_FOLLOWS_OPCODE(instruction))
	    immediate = _vpCode[pc++];

	switch (VP_OPCODE(instruction)) {
	case VP_DEBUG:
	    _debug = YES;
	    break;

	case VP_DELAY:
	    IODelay(immediate);
	    break;

	case VP_LOAD_CR:	/* load constant, reg2 */
	case VP_LOAD_TR:	/* load label, reg2 */
	    reg[VP_REG2(instruction)] = immediate;
	    break;

	case VP_LOAD_AR:	/* load @address, reg2 */
	    if (immediate >= _vpCodeCount) {
		IOLog("%s: load address is out of range: 0x%x.\n",
		      [self name], immediate);
		return nil;
	    }
	    reg[VP_REG2(instruction)] = _vpCode[immediate];
	    break;

	case VP_LOAD_IR:	/* load @reg1, reg2 */
	    immediate = reg[VP_REG1(instruction)];
	    if (immediate >= _vpCodeCount) {
		IOLog("%s: load address is out of range: 0x%x.\n",
		      [self name], immediate);
		return nil;
	    }
	    reg[VP_REG2(instruction)] = _vpCode[immediate];
	    break;

	case VP_VLOAD_AR:	/* vload @address, reg2 */
	    if (_videoRamAddress == 0 || immediate >= _videoRamSize) {
		IOLog("%s: invalid video ram address: 0x%x.\n",
		      [self name], _videoRamAddress + immediate);
		return nil;
	    }
	    reg[VP_REG2(instruction)] =
		((VPInstruction *)_videoRamAddress)[immediate];
	    break;

	case VP_VLOAD_IR:	/* vload @reg1, reg2 */
	    immediate = reg[VP_REG1(instruction)];
	    if (_videoRamAddress == 0 || immediate >= _videoRamSize) {
		IOLog("%s: invalid video ram address: 0x%x.\n",
		      [self name], _videoRamAddress + immediate);
		return nil;
	    }
	    reg[VP_REG2(instruction)] =
		((VPInstruction *)_videoRamAddress)[immediate];
	    break;

	case VP_STORE_AR:	/* store reg1, @address */
	    if (immediate >= _vpCodeCount) {
		IOLog("%s: store address is out of range: 0x%x.\n",
		      [self name], immediate);
		return nil;
	    }
	    _vpCode[immediate] = reg[VP_REG1(instruction)];
	    break;

	case VP_STORE_IR:	/* store  reg1, @reg2 */
	    immediate = reg[VP_REG2(instruction)];
	    if (immediate >= _vpCodeCount) {
		IOLog("%s: store address is out of range: 0x%x.\n",
		      [self name], immediate);
		return nil;
	    }
	    _vpCode[immediate] = reg[VP_REG1(instruction)];
	    break;

	case VP_VSTORE_AR:	/* vstore reg1, @address */
	    if (_videoRamAddress == 0
		|| immediate * sizeof(VPInstruction) >= _videoRamSize) {
		IOLog("%s: invalid video ram address: 0x%x.\n",
		      [self name], _videoRamAddress + immediate);
		return nil;
	    }
	    ((VPInstruction *)_videoRamAddress)[immediate] =
		reg[VP_REG1(instruction)];
	    break;

	case VP_VSTORE_IR:	/* vstore  reg1, @reg2 */
	    immediate = reg[VP_REG2(instruction)];
	    if (_videoRamAddress == 0
		|| immediate * sizeof(VPInstruction) >= _videoRamSize) {
		IOLog("%s: invalid video ram address: 0x%x.\n",
		      [self name], _videoRamAddress + immediate);
		return nil;
	    }
	    ((VPInstruction *)_videoRamAddress)[immediate] =
		reg[VP_REG1(instruction)];
	    break;

	case VP_ADD_CRR:	/* add constant, reg1, reg2 */
	    reg[VP_REG2(instruction)] = immediate + reg[VP_REG1(instruction)];
	    break;

	case VP_ADD_RRR:	/* add reg1, reg2, reg3 */
	    reg[VP_REG3(instruction)] =
		reg[VP_REG1(instruction)] + reg[VP_REG2(instruction)];
	    break;

	case VP_SUB_RCR:	/* sub reg1, constant, reg2 */
	    reg[VP_REG2(instruction)] = reg[VP_REG1(instruction)] - immediate;
	    break;

	case VP_SUB_CRR:	/* sub constant, reg1, reg2 */
	    reg[VP_REG2(instruction)] = immediate - reg[VP_REG1(instruction)];
	    break;

	case VP_SUB_RRR:	/* sub reg1, reg2, reg3 */
	    reg[VP_REG3(instruction)] =
		reg[VP_REG1(instruction)] - reg[VP_REG2(instruction)];
	    break;

	case VP_AND_CRR:	/* and constant, reg1, reg2 */
	    reg[VP_REG2(instruction)] = immediate & reg[VP_REG1(instruction)];
	    break;

	case VP_AND_RRR:	/* and reg1, reg2, reg3 */
	    reg[VP_REG3(instruction)] = 
		reg[VP_REG1(instruction)] & reg[VP_REG2(instruction)];
	    break;

	case VP_OR_CRR:		/* or constant, reg1, reg2 */
	    reg[VP_REG2(instruction)] = immediate | reg[VP_REG1(instruction)];
	    break;

	case VP_OR_RRR:		/* or reg1, reg2, reg3 */
	    reg[VP_REG3(instruction)] =
		reg[VP_REG1(instruction)] | reg[VP_REG2(instruction)];
	    break;

	case VP_XOR_CRR:	/* xor constant, reg1, reg2 */
	    reg[VP_REG2(instruction)] = immediate ^ reg[VP_REG1(instruction)];
	    break;

	case VP_XOR_RRR:	/* xor reg1, reg2, reg3 */
	    reg[VP_REG3(instruction)] =
		reg[VP_REG1(instruction)] ^ reg[VP_REG2(instruction)];
	    break;

	case VP_LSL_RCR:	/* lsl reg1, constant, reg3 */
	    reg[VP_REG3(instruction)] =
		reg[VP_REG1(instruction)] << VP_FIELD2(instruction);
	    break;

	case VP_LSR_RCR:	/* lsr reg1, constant, reg3 */
	    reg[VP_REG3(instruction)] = 
		reg[VP_REG1(instruction)] >> VP_FIELD2(instruction);
	    break;

	case VP_MOVE_RR:	/* move reg1, reg2 */
	    reg[VP_REG2(instruction)] = reg[VP_REG1(instruction)];
	    break;

	case VP_TEST_R:		/* test reg1 */
	    zero = positive = negative = NO;
	    immediate = reg[VP_REG1(instruction)];
	set_condition_codes:
	    if (immediate == 0) {
		zero = YES;
	    } else if ((int)immediate > 0) {
		positive = YES;
	    } else {
		negative = YES;
	    }
	    break;

	case VP_CMP_RC:		/* cmp reg1, constant */
	    zero = positive = negative = 0;
	    immediate = reg[VP_REG1(instruction)] - immediate;
	    goto set_condition_codes;
	    break;

	case VP_CMP_CR:		/* cmp constant, reg1 */
	    zero = positive = negative = 0;
	    immediate = immediate - reg[VP_REG1(instruction)];
	    goto set_condition_codes;
	    break;

	case VP_CMP_RR:		/* cmp reg1, reg2 */
	    zero = positive = negative = 0;
	    immediate = reg[VP_REG1(instruction)] - reg[VP_REG2(instruction)];
	    goto set_condition_codes;
	    break;

	case VP_BR:
	    pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BPOS:
	    if (positive)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BNEG:
	    if (negative)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BZERO:
	    if (zero)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BNPOS:
	    if (!positive)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BNNEG:
	    if (!negative)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_BNZERO:
	    if (!zero)
		pc = VP_BRANCH_DEST(instruction);
	    break;

	case VP_INB_CR:
	    port = VP_PORT(instruction);
	    reg[VP_REG2(instruction)] = 0xFF & inb(port);
	    break;

	case VP_OUTB_CR:
	    port = VP_PORT(instruction);
	    outb(port, 0xFF & reg[VP_REG2(instruction)]);
	    break;

	case VP_OUTB_CC:
	    port = VP_PORT(instruction);
	    outb(port, 0xFF & immediate);
	    break;

	case VP_INW_CR:
	    port = VP_PORT(instruction);
	    reg[VP_REG2(instruction)] = 0xFFFF & inw(port);
	    break;

	case VP_OUTW_CR:
	    port = VP_PORT(instruction);
	    outw(port, 0xFFFF & reg[VP_REG2(instruction)]);
	    break;

	case VP_OUTW_CC:
	    port = VP_PORT(instruction);
	    outw(port, 0xFFFF & immediate);
	    break;

	case VP_CALL:
	    [self jumpTo:VP_BRANCH_DEST(instruction) withInitialSRegs:&reg[8]];
	    break;

	case VP_RETURN:
	    /* Store the S-registers in the caller's initial S-register set. */
	    if (initialSRegs != 0)
		memcpy(&initialSRegs[0], &reg[8], 8 * sizeof(reg[0]));
	    return self;

	default:
	    IOLog("%s: Unrecognized opcode: 0x%08x; pc: 0x%x\n", [self name],
		  instruction, pc);
	    return nil;
	}
    }
}

- runVPCode:(unsigned int)entryPoint withRegs:(VPInstruction *)initialRegs
{
    VPInstruction reg[8];

    if (_vpCode == 0 || _vpCodeCount <= 0) {
	IOLog("%s: No vpcode to run.\n", [self name]);
	return nil;
    }
    if (entryPoint >= _vpCodeCount) {
	IOLog("%s: entry point is out of range: 0x%x.\n", [self name],
	      (unsigned)entryPoint);
	return nil;
    }
    if (_vpCode[entryPoint] == 0)
	return self;
    if (_debug) {
	IOLog("%s: Running vpcode at 0x%x\n", [self name],
	      (unsigned)_vpCode[entryPoint]);
    }
    if (initialRegs == 0) {
	memset(reg, 0, sizeof(reg));
	initialRegs = reg;
    }
    return [self jumpTo:_vpCode[entryPoint] withInitialSRegs:initialRegs];
}

static const char *
find_parameter(const char *parameter, const char *string)
{
    int c;
    size_t length;

    length = strlen(parameter);
    while (*string != 0) {
	if (strncmp(string, parameter, length) == 0) {
	    string += length;
	    while ((c = *string) != '\0' && (c == ' ' || c == '\t'))
		string++;
	    return (c != 0) ? string : 0;
	}
	string++;
    }
    return 0;
}

- getVPCodeFilename:(char *)parameterArray count:(unsigned int *)count
{
    char buffer[256];
    IOConfigTable *configTable;
    const char *s, *displayMode, *color, *filename;
    int width, height, refreshRate, length;

    /* Get the string describing the display mode. */
    configTable = [[self deviceDescription] configTable];
    if (configTable == nil)
	return nil;

    displayMode = [configTable valueForStringKey:"Display Mode"];

    s = find_parameter("Width:", displayMode);
    if (s == 0)
	return nil;
    width = strtol(s, 0, 10);

    s = find_parameter("Height:", displayMode);
    if (s == 0)
	return nil;
    height = strtol(s, 0, 10);

    s = find_parameter("Refresh:", displayMode);
    if (s == 0)
	return nil;
    refreshRate = strtol(s, 0, 10);

    s = find_parameter("ColorSpace:", displayMode);
    if (s == 0)
	return nil;
    if (strncmp(s, "BW:2", 4) == 0) {
	color = "BW:2";
    } else if (strncmp(s, "BW:8", 4) == 0) {
	color = "BW:8";
    } else if (strncmp(s, "RGB:444/16", 10) == 0) {
	color = "RGB:444/16";
    } else if (strncmp(s, "RGB:555/16", 10) == 0) {
	color = "RGB:555/16";
    } else if (strncmp(s, "RGB:888/32", 10) == 0) {
	color = "RGB:888/32";
    } else {
	return nil;
    }
    sprintf(buffer, "[%d x %d x %s @ %d]", width, height, color, refreshRate);

    if (_debug)
	IOLog("%s: Searching for key `%s'.\n", [self name], buffer);

    filename = [configTable valueForStringKey:buffer];
    if (filename == 0)
	return nil;
    length = strlen(filename) + 1;
    if (*count < length)
	return nil;

    if (_debug)
	IOLog("%s: Using vpcode from `%s'.\n", [self name], filename);
    *count = length;
    strcpy(parameterArray, filename);
    return self;
}

- (IOReturn)getCharValues:(unsigned char *)parameterArray
		forParameter:(IOParameterName)parameterName
		count:(unsigned int *)count
{
    if (_debug)
	IOLog("%s: received parameter `%s'.\n", [self name], parameterName);

    if (strcmp(parameterName, VP_GET_VPCODE_FILENAME) == 0) {
	if (![self getVPCodeFilename:parameterArray count:count])
	    return IO_R_UNSUPPORTED;
	return IO_R_SUCCESS;
    } else {
	return [super getCharValues:parameterArray 
	    forParameter:parameterName
	    count:count];
    }
}

/* Set parameters for the display object.
 */
- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
{
    if (strcmp(parameterName, VP_SET_VPCODE_SIZE) == 0) {
	if (count != 1)
	    return IO_R_INVALID_ARG;
	if (_vpCode != 0)
	    IOFree(_vpCode, _vpCodeCount * sizeof(_vpCode[0]));
	_vpCodeCount = parameterArray[0];
	_vpCodeReceived = 0;
	_vpCode = IOMalloc(_vpCodeCount * sizeof(_vpCode[0]));
	if (_vpCode == 0)
	    return IO_R_NO_MEMORY;
	IOLog("About to receive %d bytes of VPCode!\n", _vpCodeCount);
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, VP_SET_VPCODE) == 0) {
	if (_vpCode == 0)
	    return IO_R_NO_MEMORY;
	if (_vpCodeReceived + count > _vpCodeCount)
	    return IO_R_INVALID_ARG;
	memcpy(&_vpCode[_vpCodeReceived], parameterArray,
	       count * sizeof(_vpCode[0]));
	_vpCodeReceived += count;
	IOLog("Received %d bytes of VPCode!\n", count);
	return IO_R_SUCCESS;

    } else if (strcmp(parameterName, VP_END_VPCODE) == 0) {
	if (![self getDisplayInfo])
	    return IO_R_UNSUPPORTED;
	return IO_R_SUCCESS;
    } else {
	return [super setIntValues:parameterArray
	    forParameter:parameterName
	    count:count];
    }
}
@end
