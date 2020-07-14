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
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * 			INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license  agreement or 
 *	nondisclosure agreement with Intel Corporation and may not be copied 
 *	nor disclosed except in accordance with the terms of that agreement.
 *
 *	Copyright 1988, 1989 Intel Corporation
 */

/*
 * Copyright 1993 NeXT, Inc.
 * All rights reserved.
 */

#import "io_inline.h"
#import "libsaio.h"
#import "kernBootStruct.h"
#import "memory.h"
#import "bitmap.h"
#import "console.h"
#import "vga.h"
#import "vbe.h"

extern int PackBitsDecode(
	TIFF *tif,
	register unsigned char *op,
	register int occ,
	unsigned int s
);

BOOL showText = 1;

/* these should be setup when put in graphics mode */
BOOL in_linear_mode;
unsigned char  *frame_buffer;
unsigned short screen_width;
unsigned short screen_height;
unsigned char  bits_per_pixel;

char *textBuf;
int bufIndex;

/*
 * write one character to console
 */
void
putchar(
    int c
)
{
	// if not showing text, buffer it in case of errors
	if (!showText)
	{
		if (textBuf && (bufIndex < TEXTBUFSIZE))
			textBuf[bufIndex++] = c;
		return;
	}

	if (c == '\t')
	{
		for (c=0; c<8; c++) putc(' ');
		return;
	}

	if (c == '\n')
		putc('\r');
	putc(c);
}

int
getc( void )
{
    int c = bgetc();
    if ((c & 0xff) == 0)
	return c;
    else
	return (c & 0xff);
}

// Read and echo a character from console.  This doesn't echo backspace
// since that screws up higher level handling

int
getchar( void )
{
	register int c;

	c = getc();
	if (c == '\r') c = '\n';

	if (c >= ' ' && c < 0x7f) putchar(c);
	
	return(c);
}

/* use outw to send index and data together */
static inline void rwrite(int port, int index, int value)
{
    outw(port, index | (value << 8));
}

static inline int rread(int port, int index)
{
    outb(port, index);
    return(inb(port + 1));
}

static unsigned char savedGCRegisters[VGA_NUM_GC_REGS];
static unsigned char savedSEQRegisters[VGA_NUM_SEQ_REGS];

static inline void SaveVGARegs()
{
    register int i;
    
    for (i = 0; i < VGA_NUM_GC_REGS; i++)
	savedGCRegisters[i] = rread(VGA_GC_ADDR,i);
#if	defined(SAVE_ALL_SEQ_REGS)
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++)
	savedSEQRegisters[i] = rread(VGA_SEQ_ADDR,i);
#else	/* defined(SAVE_ALL_SEQ_REGS) */
    savedSEQRegisters[2] = rread(VGA_SEQ_ADDR,2);
#endif	/* defined(SAVE_ALL_SEQ_REGS) */
}

static inline void RestoreVGARegs()
{
    register int i;
    
    for (i = 0; i < VGA_NUM_GC_REGS; i++)
	rwrite(VGA_GC_ADDR,i, savedGCRegisters[i]);
#if	defined(SAVE_ALL_SEQ_REGS)
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++)
	rwrite(VGA_SEQ_ADDR,i, savedSEQRegisters[i]);
#else	/* defined(SAVE_ALL_SEQ_REGS) */
    rwrite(VGA_SEQ_ADDR, 2, savedSEQRegisters[2]);
#endif	/* defined(SAVE_ALL_SEQ_REGS) */
}

static unsigned char leftMaskArray[] =
  {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
  
//static unsigned char rightMaskArray[] = 
//  {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

static void
blitRow(
    int x,
    int y,
    int w,
    unsigned char *rowData,
    unsigned char constantData
)
{
    register unsigned char *src_p, *dst_p, prev_byte;
    unsigned char *last_byte, lmask, rmask;
    int lshift, rshift, last_w;
    volatile unsigned char xx;
    
    if (w == 0) return;
    dst_p = (unsigned char *)
		VGA_BUF_ADDR + (x >> CHAR_W_SHIFT) + y * NCOLS;
    last_byte = (unsigned char *)
		VGA_BUF_ADDR + ((x + w) >> CHAR_W_SHIFT) + y * NCOLS;
    rshift = x & 7;
    lshift = (8 - rshift) & 7;
    last_w = (x + w) & 7;
    lmask = leftMaskArray[rshift];
    rmask = ~leftMaskArray[last_w];

    src_p = rowData ? rowData : &constantData;
    
    outb(VGA_GC_ADDR, 8);			/* point to pixel mask reg */

    if (dst_p == last_byte) {
	outb(VGA_GC_ADDR + 1, lmask & rmask);
	xx = *dst_p;				/* latch data in VGA reg. */
	*dst_p = *src_p >> rshift;
    } else {
	outb(VGA_GC_ADDR + 1, lmask);
	xx = *dst_p;				/* latch data in VGA reg. */
	*dst_p = *src_p >> rshift;
	prev_byte = lshift ? *src_p : 0;
	outb(VGA_GC_ADDR + 1, 0xff);
    
	for ( dst_p++, src_p++;
	      dst_p < last_byte;
	      dst_p++, src_p++) {
		if (lshift)
		    *dst_p = (prev_byte << lshift) | (*src_p >> rshift);
		else
		    *dst_p = *src_p >> rshift;
		prev_byte = *src_p;
	}
	outb(VGA_GC_ADDR + 1, rmask);
	xx = *dst_p;
	if (lshift)
	    *dst_p = (prev_byte << lshift) | (*src_p >> rshift);
	else
	    *dst_p = *src_p >> rshift;
    }
    outb(VGA_GC_ADDR + 1, 0xff);
}

#define NPLANES 2

/* This depends on bitmaps being a multiple of 8 pixels wide.
 */ 
void
copyImage(
    const struct bitmap *bitmap,
    int x,
    int y
)
{
    if (!in_linear_mode)
    {
	int i,plane, row_bytes;
	unsigned char rowbuf[NCOLS];
	TIFF tif;

    row_bytes = (bitmap->width + 7) >> CHAR_W_SHIFT;
    for (plane = 0; plane < NPLANES; plane++) {
	tif.tif_rawcp = tif.tif_rawdata = bitmap->plane_data[plane];
	tif.tif_rawcc = bitmap->plane_len[plane];
	/* enable write to correct plane */
	rwrite(VGA_SEQ_ADDR, 0x2, 1 << plane);
	for (i=0; i < bitmap->height; i++) {
	    if (bitmap->packed) {
		PackBitsDecode(&tif, rowbuf, row_bytes, 0);
		blitRow(x, y+i, bitmap->width, rowbuf, 0);
	    } else {
		blitRow(x, y+i, bitmap->width, tif.tif_rawcp, 0);
		tif.tif_rawcp += row_bytes;
	    }
	}
    }
    }
    else
    {
	unsigned char *fb = frame_buffer;
	unsigned int bytes_per_pixel, row_bytes, skip;
	register int j, k;
	unsigned char *data;

	bytes_per_pixel = bits_per_pixel >> BYTE_SHIFT;
	row_bytes = bytes_per_pixel * SCREEN_W;
	data = bitmap->plane_data[0];
	skip = bytes_per_pixel * (SCREEN_W - bitmap->width);

	fb += (row_bytes * y) + (bytes_per_pixel * x);
        for (j=0; j < bitmap->height; j++) {
            for (k=0; k < bitmap->width; k++) {
	        *fb++ = *data++;
	    }
	    fb += skip;
        }
	free(data);
    }
}

/* Clear a rectangle on the screen;
 * arguments are in pixels.
 */

void
clearRect(
    int x,
    int y,
    int w,
    int h,
    int c
)
{
    register int j, k;

    if (!in_linear_mode)
    {
        SaveVGARegs();
        rwrite(VGA_SEQ_ADDR, 2, 0x0f); /* enable all planes in map mask register */
        rwrite(VGA_GC_ADDR, 1, 0x0f);  /* enable planes for set/reset write mode */
        rwrite(VGA_GC_ADDR, 0, c);			/* set color */
        rwrite(VGA_GC_ADDR, 5, 0x00);		/* set default mode */

        for (k=0; k < h; k++) {
	    blitRow(x, y+k, w, 0, 0xff);
        }
        RestoreVGARegs();   
    }
    else
    {
	unsigned char *fb = frame_buffer;
	unsigned int bytes_per_pixel, row_bytes, skip;

	bytes_per_pixel = bits_per_pixel >> BYTE_SHIFT;
	row_bytes = bytes_per_pixel * SCREEN_W;
	skip = bytes_per_pixel * (SCREEN_W - w);

	fb += (row_bytes * y) + (bytes_per_pixel * x);
        for (j=0; j < h; j++) {
            for (k=0; k < w; k++) {
	        *fb++ = c;
	    }
	    fb += skip;
        }
    }
}

