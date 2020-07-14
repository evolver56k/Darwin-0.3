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
 * Initialize the VGA console.
 */
 
#import <bsd/i386/param.h>
#import <bsd/dev/i386/BasicConsole.h>
#import <bsd/dev/i386/VGAConsole.h>
#import <driverkit/displayDefs.h>
#import <machdep/i386/kernBootStruct.h>
#import <machdep/i386/io_inline.h>


#define VGA_MODE_S12		0	// 640x480x4 VGA mode
#define	N_SMODES		1	// Total number of modes


#define VGA_SEQ_ADDR		0x3c4
#define VGA_SEQ_DATA		0x3c5

#define VGA_CLK_MODE_INDEX	0x01
#define VGA_SCRN_OFF		0x20


#define VGA_CRTC_ADDR   		0x3d4
#define VGA_CRTC_DATA   		0x3d5

#define	VGA_ADDR		((char *)0xA0000)
#define	VGA_LENGTH		0x10000

// Generic VGA Stuff
#define	INPUT_STATUS_1	0x3da

// Misc Output Register values
#define	MISC_OUTPUT_PORT	0x3C2
#define	FEAT_CNTRL_PORT		0x3da
#define VGA_MISC_CNT		2
static const unsigned char miscOutData[VGA_MISC_CNT] = {
	0xe3,0,	/* VGA_MODE_S12 */
};

// Sequencer Data
#define	VGA_SEQ_CNT	5

static const unsigned char sequencerData[VGA_SEQ_CNT] = {
     0x03, 0x21, 0x0f, 0x00, 0x06	// VGA_MODE_S12
};

#define	VGA_CRT_CNT	25

static const unsigned char crtData[VGA_CRT_CNT] = {
	 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0xea, 0x8c, 0xdf, 0x28,
	 0x00, 0xe7, 0x04, 0xe3, 0xff	// VGA_MODE_S12
};

#define	VGA_ATR_CNT	21
#define ATTR_INDEX	0x3C0
#define ATTR_DATA	0x3C0
static const unsigned char attrData[VGA_ATR_CNT] = {
	 0x00, 0x01, 0x02, 0x03,
	 0x00, 0x01, 0x02, 0x03,
	 0x00, 0x01, 0x02, 0x03,
	 0x00, 0x01, 0x02, 0x03,
	 0x01, 0x00,
	 0x03, 0x00, 0
	 //VGA_MODE_S12
};

#define	VGA_GFX_CNT	9
#define GC_INDEX	0x3CE
#define GC_DATA		0x3CF
static const unsigned char gfxData[VGA_GFX_CNT] = {
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff,	// VGA_MODE_S12
};

#define VGA_PALETTE_REG		0x3C8
#define VGA_PALETTE_DATA	0x3C9
#define VGA_NUM_COLORS		16
static const unsigned char paletteVals[VGA_NUM_COLORS][3] = {
	{0,0,0},	// 0 black
	{21, 21, 21},	// 1 dark gray
	{42, 42, 42},	// 2 light gray
	{63, 63, 63},	// 3 white
	{0,0,0},	// 0 black
	{21, 21, 21},	// 1 dark gray
	{42, 42, 42},	// 2 light gray
	{63, 63, 63},	// 3 white
	{0,0,0},	// 0 black
	{21, 21, 21},	// 1 dark gray
	{42, 42, 42},	// 2 light gray
	{63, 63, 63},	// 3 white
	{0,0,0},	// 0 black
	{21, 21, 21},	// 1 dark gray
	{42, 42, 42},	// 2 light gray
	{63, 63, 63},	// 3 white
};

static inline void setColorReg(int reg, int red, int green, int blue)
{
	outb(VGA_PALETTE_REG, reg);	// VGA select color palette reg
	DELAY(10);
	outb(VGA_PALETTE_DATA, red);	// VGA now prepared to accept each color
	DELAY(10);
	outb(VGA_PALETTE_DATA, green);
	DELAY(10);
	outb(VGA_PALETTE_DATA, blue);
	DELAY(10);
}

static void setVGAcolors()
{
	int i, index;
	// set the palette registers to something nice
	for (i = 0; i < VGA_NUM_COLORS; i++)
	{
	    index = i % 4;
	    setColorReg(i, paletteVals[index][0],
			   paletteVals[index][1],
			   paletteVals[index][2]);
	}
}

extern void bzero(void *, int);
extern void memset(void *, unsigned char, int);

int VGASetGraphicsMode(void)
// Description:	Sets the standard VGA registers based on the supplied mode.
//		This code is pretty generic, maybe we should break it out
//		as a standard utility routine.
{
    unsigned int i;
    
    // NOTE: The attribute registers are a little wierd. Normally there is a
    // separate index and data port. The attribute register set has just
    // one port that gets used for both. You write an index to the port then
    // use the same port for data. The VGA automatically toggles the sense
    // of the port (between index and data) with an internal flip-flop.
    // You set the state of the flip-flop by doing an in() on the input
    // status 1 port - I know its hokey, but thats the way it works...
    // MORE: The other weird thing is that the attribute index register also
    // contains a palette access bit. This bit determines whether the CPU or
    // the VGA has control of the palette. While the CPU owns the palette, the
    // display is effectively off.

    // Turn the video off while we are doing this...
    outb(VGA_SEQ_ADDR, 1);
    outb(VGA_SEQ_DATA, sequencerData[1]);
    
    inb(INPUT_STATUS_1);	// Set the attribute flip-flop to "index"
    outb(ATTR_INDEX, 0x00);	// Gives palette to CPU, turning off video
//    outb(ATTR_DATA, attrData[0]);

    // Set the misc. output register
    outb(MISC_OUTPUT_PORT, miscOutData[0]);
    
    // Set the feature control register
    outb(FEAT_CNTRL_PORT, miscOutData[1]);

    // Load the sequencer registers
    for (i = 0; i < VGA_SEQ_CNT; i++)
    {
	outb(VGA_SEQ_ADDR, i);
	outb(VGA_SEQ_DATA, sequencerData[i]);
    }

    outb(VGA_SEQ_ADDR, 0x00);
    outb(VGA_SEQ_DATA, 0x03);	// Low order two bits are reset bits

    // Load the CRTC registers
    // CRTC registers 0-7 are locked by a bit in register 0x11. We need
    // to unlock these registers before we can start setting them.
    outb(VGA_CRTC_ADDR, 0x11);
    outb(VGA_CRTC_DATA, 0x00);	// Unlocks registers 0-7
    for (i = 0; i < VGA_CRT_CNT; i++)
    {
	outb(VGA_CRTC_ADDR, i);
	outb(VGA_CRTC_DATA, crtData[i]);
    }


    inb(INPUT_STATUS_1);	// Set the attribute flip-flop to "index"
    // Load the attribute registers
    for (i = 0; i < VGA_ATR_CNT; i++)
    {
	outb(ATTR_INDEX, i);
	outb(ATTR_DATA, attrData[i]);
    }


    // Load graphics registers
    for (i = 0; i < VGA_GFX_CNT; i++)
    {
	outb(GC_INDEX, i);
	outb(GC_DATA, gfxData[i]);
    }    

    setVGAcolors();

    // Clear display memory
    // FIXME -- should ensure memory mapping here
    // (this is relying on a straight physical-to-virtual mapping)
    memset(VGA_ADDR, 0x01, VGA_LENGTH);
    
    // Re-enable video
    inb(INPUT_STATUS_1);	// Set the attribute flip-flop to "index"
    outb(ATTR_INDEX, 0x20);	// Give the palette back to the VGA
//    outb(ATTR_DATA, attrData[0]);

    // Really re-enable video.
    outb(VGA_SEQ_ADDR, 1);
    outb(VGA_SEQ_DATA, ((sequencerData[1]) & (~0x20)));

    return 0;
}


IOConsoleInfo *BasicAllocateConsole()
{
    IODisplayInfo di;
    KERNBOOTSTRUCT *kernbootstruct = KERNSTRUCT_ADDR;
    
    bzero(&di, sizeof(di));
    di.width = 640;
    di.height = 480;
    return( VGAAllocateConsole(&di) );
}

