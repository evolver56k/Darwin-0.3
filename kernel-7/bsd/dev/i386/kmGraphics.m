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
 * Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved.
 *
 * miniMon.c -- Machine-independent mini-monitor.
 *
 * HISTORY
 * 04-Oct-1993		Curtis Galloway at NeXT
 *	Created.
 *
 */

#import <mach/boolean.h>
#import <bsd/dev/kmreg_com.h>
#import <bsd/dev/i386/kmFontPriv.h>
#import "ConsoleSupport.h"
#import <bsd/dev/i386/kmDevice.h>

#import "NSPanel.h"
#import "TimesItalic14.h"

#define BACKGROUND_BYTE (KM_COLOR_LTGRAY | \
                   (KM_COLOR_LTGRAY << 2) | \
                   (KM_COLOR_LTGRAY << 4) | \
                   (KM_COLOR_LTGRAY << 6))

#define TEXT_COLOR		KM_COLOR_DKGRAY

#define	BITSPIXEL		2
#define PIXELS_PER_BYTE		(8/BITSPIXEL)

#define SCREEN_W	640
#define SCREEN_H	480

static char 
    kmTextBitmap[(NS_PANEL_TEXT_W * NS_PANEL_TEXT_H * BITSPIXEL + 7) / 8];
static char *fb = kmTextBitmap;
static font_c_t *fontp = &Times_Italic_14;
static int text_width, text_height;
static int panel_xorg, panel_yorg;

#define XMARGIN		0
#define YMARGIN		2		// empirically derived!

/*
 * The current position
 */
static int xpos;
static int ypos;

static void blit_bm(bitmap_t *bmp);
static void image_bitmap(IOConsoleInfo *console);

#if notdef
static char *bm_malloc(int size, int clear)
{
    return (char *)kalloc(size);
}
#endif

static inline bitmap_t *
getbm(unsigned char c)
{
	return &fontp->bitmaps[c - ENCODEBASE];
}

static void eraserect()
{
    int i;
    for (i = 0; i < sizeof(kmTextBitmap); i++) {
        kmTextBitmap[i] = BACKGROUND_BYTE;
    }
}



static inline int
bmbit(bitmap_t *bmp, int x, int y)
{
	int bitoffset;
	
	bitoffset = bmp->bitx + y * bmp->bbx.width + x;
	return (fontp->bits[bitoffset >> 3] >> (7 - (bitoffset & 0x7))) & 1;
}

static unsigned char *lastfbaddr;
static unsigned char fbbyte;

/*
 * Set a bit in the 'frame buffer'.  Assumes that left-most displayed pixel
 * is most significant in the frame buffer byte.  Also that 0,0 is at lowest
 * address of frame buffer.
 */
static inline void
setfbbit(int x, int y, int color)
{
	unsigned char *fbaddr;
	int bitoffset;
	int mask;
	int bitshift;
	
	bitoffset = ((y * text_width) + x) * BITSPIXEL;
	fbaddr = fb + (bitoffset >> 3);
	if (fbaddr != lastfbaddr) {
		if (lastfbaddr)
			*lastfbaddr = fbbyte;
		fbbyte = *fbaddr;
		lastfbaddr = fbaddr;
	}
	bitshift = (8 - BITSPIXEL) - (bitoffset & 0x7);
	color <<= bitshift;
	mask = ~(-1 << BITSPIXEL) << bitshift;
	fbbyte = (fbbyte & ~mask) | (color & mask);
}

static inline void
flushbits(void)
{
	if (lastfbaddr)
		*lastfbaddr = fbbyte;
	lastfbaddr = 0;
}

/*
 * Assumes bitmap is in PS format, ie. origin is lower left corner.
 */
static void
blit_bm(bitmap_t *bmp)
{
	int x, y;
	/*
	 * x and y are in fb coordinate system
	 * xoff and yoff are in ps coordinate system
	 */
	for (y = 0; y < bmp->bbx.height; y++)
	{
		for (x = 0; x < bmp->bbx.width; x++)
		{
			int theBit = bmbit(bmp, x, (bmp->bbx.height - y - 1));
			if (theBit) setfbbit(xpos + bmp->bbx.xoff + x, 
					ypos - bmp->bbx.yoff - y,
					TEXT_COLOR);
		}
	}
	flushbits();
	xpos += bmp->dwidth;
}

static void
image_bitmap(IOConsoleInfo *console)
{
	struct km_drawrect rect;
	
	rect.width = text_width;
	rect.height = text_height;
	rect.x = panel_xorg + NS_PANEL_TEXT_X;
	rect.y = panel_yorg + NS_PANEL_TEXT_Y;
	rect.data.bits = (void *)fb;
	
	(void)(*console->DrawRect)(console, &rect);
}

@implementation kmDevice (KmGraphics)

- (void)graphicPanelString:(char *)str
{
	int lines;
	char lastchar;
	bitmap_t *bmp;
	int deflead;
	const unsigned char *string;
	
	if (fbMode != SCM_GRAPHIC)
	    return;
	
	str = kmLocalizeString(str);
	
	lastfbaddr = 0;
	text_width = NS_PANEL_TEXT_W;
	text_height = NS_PANEL_TEXT_H;
	eraserect();
	
	for(lines = 0, string = str, lastchar = '\0'; *string; string++) {
	    lastchar = *string;
	    if (lastchar == '\n')
		lines++;
	}
	if (lastchar != '\n')
	    lines++;
	
	/* Round up origin points. */
	panel_xorg = (panel_xorg + PIXELS_PER_BYTE - 1) &
		     ~(PIXELS_PER_BYTE - 1);
	/* Round down width and height */
	text_width &= ~(PIXELS_PER_BYTE - 1);
	
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;

        ypos = TEXTBASELINE;

	for (string = str; *string; ) {
	    const char *line;
	    int len;
	    
	    /* Center this line */
	    for (line = string, len = 0; *line && (*line != '\n'); line++) {
		bmp = getbm(*line);
		len += bmp->dwidth;
	    }
	    xpos = XMARGIN + (text_width - len) / 2;

	    for (; *string; string++) {
		if (*string == '\n') {
		    string++;
		    break;
		}
		bmp = getbm(*string);
		blit_bm(bmp);
	    }
	    ypos += deflead;
	}
	(void) image_bitmap(fbp[FB_DEV_NORMAL]);
}

- (void)drawGraphicPanel:(GraphicPanelType) panelType
{
	struct km_drawrect rect;
	
	if (fbMode != SCM_GRAPHIC)
	    return;
	rect.width = NS_PANEL_WIDTH;
	rect.height = NS_PANEL_HEIGHT;
	rect.x = panel_xorg = (SCREEN_W - rect.width) / 2;
        rect.y = panel_yorg = (SCREEN_H - rect.height) / 2 + NS_PANEL_DY;
	rect.data.bits = (void *)(NSPanel);
	(void) (*fbp[FB_DEV_NORMAL]->DrawRect)(fbp[FB_DEV_NORMAL], &rect);
}

@end
