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
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#import "font.h"
#import "fontio.h"
#import "libsaio.h"

#define DKGRAY_PX 2
#define PIXELS_PER_BYTE 8

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define setPixel(x,y,c) 	clearRect((x),(y),1,1,(c))
#define flushbits()

const font_t *fontp;
//const font_c_t *fontp = &Helvetica_14;

//static int xmargin = 30;
//static int ymargin = 0;

#define DEFLEAD (fontp->bbx.height + (fontp->bbx.height + 9) / 10)
//#define DEFLEAD (Times_Italic_14_BBX_HEIGHT + (Times_Italic_14_BBX_HEIGHT + 9) / 10)
//#define FONT_HEIGHT Times_Italic_14_BBX_HEIGHT
#define FONT_HEIGHT (fontp->bbx.height)
//#define FONT_WIDTH Times_Italic_14_BBX_WIDTH
#define FONT_WIDTH (fontp->bbx.width)
//#define FONT_XOFF Times_Italic_14_BBX_XOFF
#define FONT_XOFF (fontp->bbx.xoff)
//#define FONT_YOFF Times_Italic_14_BBX_YOFF
#define FONT_YOFF (fontp->bbx.yoff)

static bitmap_t *
getbm(unsigned char c)
{
	if (c < ENCODEBASE || c > ENCODELAST)
		c = '*';
	return (bitmap_t *)&fontp->bitmaps[c - ENCODEBASE];
}

static inline int
bmbit(bitmap_t *bmp, int x, int y)
{
		int bitoffset;
		
		bitoffset = bmp->bitx + y * bmp->bbx.width + x;
		return (fontp->bits[bitoffset >> 3] >> (7 - (bitoffset & 0x7))) & 1;
}

/*
 * Assumes bitmap is in PS format, ie. origin is lower left corner.
 */
static void
blit_bm(bitmap_t *bmp, int xpos, int ypos, int color)
{
		int x,y;
		/*
		 * x and y are in fb coordinate system
		 * xoff and yoff are in ps coordinate system
		 */
		for (y = 0; y < bmp->bbx.height; y++)
		{
				for (x = 0; x < bmp->bbx.width; x++)
				{
						int theBit = bmbit(bmp, x, (bmp->bbx.height - y - 1));
						if (theBit)
								 setPixel(xpos + bmp->bbx.xoff + x, 
										ypos - bmp->bbx.yoff - y, color);
				}
		}
}

static int
strwidth_internal(
	register const char *str,
	int line_only
)
{
	register int c, width, maxwidth = 0;
	register bitmap_t *bmp;

	bmp = getbm(*str);
	width = - bmp->bbx.xoff;
	while (c = *str++) {
		if (c == '\n') {
			if (line_only)
				return width;
			maxwidth = max(width, maxwidth);
			width = 0;
		} else {
			bmp = getbm(c);
			width += bmp->dwidth + bmp->bbx.xoff;
		}
	}
	return max(width, maxwidth);
}

int
strwidth(
	register const char *str
)
{
	return strwidth_internal(str, 0);
}

static inline int
linewidth(
	register const char *str
)
{
	return strwidth_internal(str, 1);
}

int strheight(
	const char *str
)
{
	register int lines=1;
	register int c;
	
	while (c = *str++)
		if (c == '\n')
			lines++;
	return (FONT_HEIGHT +
		(FONT_HEIGHT + (FONT_HEIGHT + 9) / 10) * (lines - 1));
}

void
blit_clear(int maxwidth, int xpos, int ypos, int center, int color)
{
	if (center & CENTER_V)
		ypos -= FONT_HEIGHT / 2;
	else
		ypos = ypos - FONT_YOFF - FONT_HEIGHT;
	if (center & CENTER_H)
		xpos -= maxwidth / 2;
	clearRect( xpos, ypos, maxwidth, FONT_HEIGHT, color);
}

int
blit_string(const char *str, int xpos, int ypos, int color, int center)
{
		int c, line_xpos;
		bitmap_t *bmp;
//		int deflead;
//		static int first_line = TRUE;
		const unsigned char *line;
		
//		/* Round up origin points. */
//		xorg = (xorg + PIXELS_PER_BYTE - 1) & ~(PIXELS_PER_BYTE - 1);
//		/* Round down width and height */
//		width &= ~(PIXELS_PER_BYTE - 1);
		if (center & CENTER_V)
			ypos = ypos - strheight(str) / 2
				+ FONT_HEIGHT + FONT_YOFF;
		
//		deflead = DEFLEAD;
//		if (first_line) {
//				first_line = FALSE;
//				xpos = xmargin;
//				ypos = ymargin + FONT_HEIGHT;
//		}

		for(line = str, c = 1; c && *line; ) {
			line_xpos = xpos;
			if (center & CENTER_H)
				line_xpos -= linewidth(line) / 2;
			while ((c = *line++) && (c != '\n')) {
#if 0
					if (c == '\\') {
							switch (c = *line++) {
							case 'n':
									c = '\n';
									break;
							}
					}							
#endif
					bmp = getbm(c);
					blit_bm(bmp, line_xpos, ypos, color);
					line_xpos += bmp->dwidth;
			}
			ypos += DEFLEAD;
		}
//		if (autonl) {
//				xpos = xmargin;
//				ypos += lead_set ? lead : deflead;
//		}

		return strwidth(str);
}
