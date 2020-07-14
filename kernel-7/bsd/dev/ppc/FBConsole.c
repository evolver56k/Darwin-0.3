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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * FBConsole.c - FrameBuffer based console implementation.
 *
 *
 * HISTORY
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 * 25 Mar 97    Simon Douglas
 *      Rhapsodized.
 */

// TO DO:
// * Implement other cases of FlipCursor, HorizLine, VertLine, ClearWindow.
//   Erase
// * Find the #defines for np and del are, then get rid of our #defines.
//
// Notes:
// * This module implements the console functionality required by the
//   ConsoleSupport protocol. If you have a device which is a framebuffer
//   with a depth supported by this module, then you can use this code. 
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.

#define BRINGUP 1

#import <sys/syslog.h>
#import	<bsd/dev/ppc/FBConsole.h>
#import	<bsd/dev/ppc/FBConsPriv.h>
#import <architecture/ascii_codes.h>
#import "BootImages.h"

#if defined(NOTINKERNEL)
#define panic(str)
#define copyin(src, dst, size) bcopy(src, dst, size)
#define kalloc(size) malloc(size)
#define kfree(ptr, size) free(ptr)
#endif

#ifdef	DRIVERKIT
#else
#define IOMalloc(size) kalloc(size)
#define IOFree(ptr,size) kfree(ptr,size)
#define IOLog(s,a) printf(s,a)
#endif	// DRIVERKIT

#define	TEXTURE_BACKGND	1

/*
 * Standard font.
 */
extern char ohlfs12[][];

#import "ohlfs12.h"

#define	NIL	(0)

/* Window margins */
#define BG_MARGIN	2	// pixels of background to leave as margin
#define FG_MARGIN	1	// pixels of foreground to leave as margin
#define TOTAL_MARGIN	(BG_MARGIN + FG_MARGIN)
#define DROP_SHADOW	1	// pixels of dark_grey, bottom & right
#define TS_MARGIN	1	// 

#define HORIZ_BORDER	32	// 40 on VGA
#define VERT_BORDER	22	// 30 on VGA

/*
 * Ansi sequence state.
 */
typedef enum {
	AS_NORMAL,
	AS_ESCAPE,
	AS_BRACKET,
	AS_R
} ansi_state_t;

#define ANSI_STACK_SIZE	3

typedef struct _t_Console {
    ScreenMode		window_type;

    IODisplayInfo	display;

    //
    // The remaining fields are window parameters. These are only 
    // meaningful for text-type windows (window_type == SCM_TEXT or 
    // SCM_ALERT).
    //
	
    //
    // Current window origin in pixels, relative to origin of screen.
    //
    int			window_origin_x;
    int			window_origin_y;

    //
    // Window size information, in pixels and in characters.
    //
    int			chars_per_row;	
    int			pixels_per_row;	
    int			chars_per_column;
    int 		pixels_per_column;

    //
    // Cursor location, in characters. Origin is top 
    // left corner of window = {0,0}. 
    //
    int			curr_row;		// in characters
    int 		curr_column;		// in characters
    
    //
    // Misc. video parameters. The background and foreground fields
    // are slightly dependent on the hardware implementation; they'll
    // work as long as bits per pixel <= 32.
    //

    unsigned int	background;		// bits for background pixel
    unsigned int	light_grey;		// bits for light grey
    unsigned int	dark_grey;		// bits for dark grey
    unsigned int	foreground;		// bits for foreground pixel
    unsigned int	baseground;		// bits for base color
    unsigned int	grayTable[4];		// bits for drawing bitmaps

    boolean_t		has_title;
    boolean_t		prettyPanelUp;
    unsigned int	bolding;		// bold text pixel count

    //
    // Storage for saved region under an alert panel.
    //
    unsigned char *saveBits;
    int saveHeight;
    int saveRowBytes;
    int saveBytes;
    unsigned char *saveLocation;
    ansi_state_t ansi_state;	/* track ansi escape sequence state */
    u_char ansi_stack[ANSI_STACK_SIZE];
    u_char *ansi_stack_p;

} ConsoleRep, *ConsolePtr;

//
// BEGIN:	Private utility routines
//

static void videoMemMove( char * dst, char * src, unsigned int len )
{
#if 1
    bcopy_nc( src, dst, len );
#else
    memmove( dst, src, len );
#endif
}

static unsigned int BPPToPPW(int bpp)
{
  if (bpp == IO_2BitsPerPixel) return(16);
  if (bpp == IO_8BitsPerPixel) return(4);
  if (bpp == IO_12BitsPerPixel) return(2);
  if (bpp == IO_15BitsPerPixel) return(2);
  if (bpp == IO_24BitsPerPixel) return(1);
  else return(-1);
}

static unsigned char *PixelAddress(ConsolePtr console, int xPix, int yPix)
{
    switch (console->display.bitsPerPixel) {
        case IO_8BitsPerPixel:
	    return ((unsigned char *)console->display.frameBuffer) +
		    yPix * console->display.rowBytes + xPix;
        case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    return ((unsigned char *)console->display.frameBuffer) +
		    yPix * console->display.rowBytes + xPix * 2;
	case IO_24BitsPerPixel:
	    return ((unsigned char *)console->display.frameBuffer) +
		    yPix * console->display.rowBytes + xPix * 4;
	default:
	    panic("FBConsole/PixelAddress: bogus bitsPerPixel");
    }
}

static void Fill(ConsolePtr console,
    unsigned char *dst,		// Where to start filling
    unsigned pixel, 		// pattern to fill
    unsigned len)		// in pixels
// Descritpion:	Fill the framebuffer with 'pixel' starting at dst
//		and for 'len' pixels.
{
    switch (console->display.bitsPerPixel) {
        case IO_8BitsPerPixel:
	    {
	    unsigned char *dst8 = (unsigned char *)dst;
	    for (; len--; dst8++)
	        *dst8 = pixel;
	    break;
	    }
        case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    {
	    unsigned short *dst16 = (unsigned short *)dst;
	    for (; len--; dst16++)
	        *dst16 = pixel;
	    break;
	    }
	case IO_24BitsPerPixel:
	    {
	    unsigned int *dst32 = (unsigned int *)dst;
	    for (; len--; dst32++)
	        *dst32 = pixel;
	    break;
	    }
	default:
	    panic("FBConsole/Fill: bogus bitsPerPixel");
    }
}

static void FlipCursor(ConsolePtr console)
// Description: Turn cursor on/off. This just inverts every pixel at the
// character at (curr_rown, curr_column).
{
    int y;
    unsigned char *dst8;
    unsigned short *dst16;
    unsigned int *dst32;
    unsigned pixel_num;
    
    // Screen-relative pixel coordinates of bounds of current  character.
    int start_y = console->window_origin_y +
        console->curr_row * CHAR_H;
    int end_y   = start_y + CHAR_H;
    int start_x = console->window_origin_x +
        console->curr_column * CHAR_W;
    
    for (y=start_y; y < end_y; y++) {
	switch (console->display.bitsPerPixel) {
	    case IO_8BitsPerPixel:
		dst8 = (unsigned char *)PixelAddress(console, start_x, y);
		for (pixel_num = 0; pixel_num < CHAR_W; pixel_num++) {
			if ((*dst8 & PIXEL_MASK_16) ==
			    (console->background & PIXEL_MASK_16)) {
				*dst8 = console->foreground;
			}
			else {
				*dst8 = console->background;
			}
			dst8++;
		}
		break;
	    case IO_12BitsPerPixel:
	    case IO_15BitsPerPixel:
		dst16 = (unsigned short *)PixelAddress(console, start_x, y);
		for (pixel_num = 0; pixel_num < CHAR_W; pixel_num++) {
			if ((*dst16 & PIXEL_MASK_16) ==
			    (console->background & PIXEL_MASK_16)) {
				*dst16 = console->foreground;
			}
			else {
				*dst16 = console->background;
			}
			dst16++;
		}
		break;
	    case IO_24BitsPerPixel:
		dst32 = (unsigned int *)PixelAddress(console, start_x, y);
		for (pixel_num = 0; pixel_num < CHAR_W; pixel_num++) {
			if ((*dst32 & PIXEL_MASK_32) ==
			    (console->background & PIXEL_MASK_32)) {
				*dst32 = console->foreground;
			}
			else {
				*dst32 = console->background;
			}
			dst32++;
		}
		break;
		
	    default:
		panic("FBConsole/FlipCursor: bogus bits per pixel");
	}	
    }
}

static void SwapForegroundAndBackground(ConsolePtr console)
{
    int temp_color;
    
    temp_color = console->foreground;
    console->foreground = console->background;
    console->background = temp_color;
}

static void Rect(
    ConsolePtr console,
    int origin_x, int origin_y,
    int width, int height,
    unsigned pixel)
// Description:	Draws a single pixel wide horizontal line
{
    while (height--)
	Fill(console,
	    PixelAddress(console, origin_x, origin_y++),
	    pixel, width);
}

#ifdef notdef
static void HorizLine(
    ConsolePtr console,
    int origin_x, int origin_y, int length,
    unsigned pixel)
// Description:	Draws a single pixel wide horizontal line
{
    Fill(console, PixelAddress(console, origin_x, origin_y), pixel, length);
}

static void VertLine(
    ConsolePtr console,
    int origin_x, int origin_y, int length,
    unsigned pixel)
// Description:	Draws a single pixel wide vertical line
{
    unsigned char *dstPixel = PixelAddress(console, origin_x, origin_y);

    switch(console->display.bitsPerPixel) {
	case IO_8BitsPerPixel:
	    {
		unsigned char *dst = (unsigned char *)dstPixel;
		while (length--) {
		    *dst = pixel;
		    dst += console->display.totalWidth;
		}
	    }
	    break;
	case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    {
		unsigned short *dst = (unsigned short *)dstPixel;
		while (length--) {
		    *dst = pixel;
		    dst += console->display.totalWidth;
		}
	    }
	    break;
	case IO_24BitsPerPixel:
	    {
		unsigned long *dst = (unsigned long *)dstPixel;
		while (length--) {
		    *dst = pixel;
		    dst += console->display.totalWidth;
		}
	    }
	    break;
	    
	default:
	    panic("FBConsole/VertLine: bogus bits per pixel");
    }
}
#endif

static void ClearWindow(ConsolePtr console)
// Description:	 Set contents of entire current window to background.
// Preconditions:
// *	Cursor is off on entry and will remain so on exit.
{
    int line;
    unsigned char *dstPixel = PixelAddress(
        console, console->window_origin_x, console->window_origin_y);

    for (line=0; line < console->pixels_per_column; line++) {
	// One loop per scan line in the window.
	Fill(console, dstPixel, console->background,
	     console->pixels_per_row);
	dstPixel += console->display.rowBytes;
    }
}

static void WipeScreen(ConsolePtr console, const unsigned pixel)
// Description:	Fill the screen with specific pixel value.  
{
    static int horstArray[5] = { 5,3,2,1,1 };
    int i,horst;

    Rect( console, 0, 0, console->display.width, console->display.height, pixel );

    // Round rect
    for( i=0; i < 5; i++) {
	horst = horstArray[i];
	// top left
	Fill(console, PixelAddress(console, 0, i), console->foreground, horst);
	// bottom left
	Fill(console, PixelAddress(console, 0, console->display.height - i - 1), console->foreground, horst);
	// top right
	Fill(console, PixelAddress(console, console->display.width - horst, i), console->foreground, horst);
	// bottom right
	Fill(console, PixelAddress(console, console->display.width - horst, console->display.height - i - 1),
		console->foreground, horst);
    }
}

static void ClearToEOL(ConsolePtr console)
// Description:	Clear to end of line (within window), including current pos.
// Preconditions:
// *	Cursor is 'off' on entry and will remain so on exit.
{
    unsigned pixel_count;
    int starting_x;			// window-relative 'x' coordinate
    int y;
    int line;	

    starting_x = console->window_origin_x + 
	(console->curr_column * CHAR_W);
    y = console->window_origin_y + (console->curr_row * CHAR_H);

    pixel_count = console->pixels_per_row -
        (starting_x - console->window_origin_x);

    for (line=0; line < CHAR_H; line++) {
	// One loop per scan line in the row.
	Fill(
	    console, PixelAddress(console, starting_x, y++),
	    console->background, pixel_count);
    }
}

static void Erase(ConsolePtr console)
// Description: Erase current cursor location.
{
    int y;
    unsigned char *dst8;
    unsigned short *dst16;
    unsigned int *dst32;
    int x;
    // Screen-relative pixel coordinates of bounds of current character.
    int start_y = console->window_origin_y +
        console->curr_row * CHAR_H;
    int end_y   = start_y + CHAR_H;
    int start_x = console->window_origin_x +
        console->curr_column * CHAR_W;
    
    for (y=start_y; y<end_y; y++) {
	// I think it's quicker not to use Fill() here...
	switch (console->display.bitsPerPixel) {
	    case IO_8BitsPerPixel:
		dst8 = (unsigned char *)PixelAddress(console, start_x, y);
		for (x=0; x < CHAR_W; x++) {
		    *dst8++ = console->background;
		}
		break;
	    case IO_12BitsPerPixel:
	    case IO_15BitsPerPixel:
		dst16 = (unsigned short *)PixelAddress(console, start_x, y);
		for (x=0; x < CHAR_W; x++) {
		    *dst16++ = console->background;
		}
		break;
	    case IO_24BitsPerPixel:
		dst32 = (unsigned int *)PixelAddress(console, start_x, y);
		for (x=0; x < CHAR_W; x++) {
		    *dst32++ = console->background;
		}
		break;
		
	    default:
		panic("FBConsole/Erase: bogus bits per pixel");	
	}	
    }
}

extern unsigned char appleClut8[ 768 ];

static void Expand4Or8ToN( ConsolePtr console, int x, int y, int width, int height,
                        unsigned char * dataPtr, unsigned char * map16)
{
    unsigned char *dst;
    int line, col, bit = 4;
    unsigned int data, data8;

    dst = (unsigned char *) PixelAddress(console, x, y);
    for( line = 0; line < height; line++) {
        for( col = 0; col < width; col++, bit ^= 4) {
	    if( map16) {
	        if( bit)
                    data8 = *dataPtr++;
                data = map16[ (data8 >> bit) & 15];
            } else {
                data = data8 = *dataPtr++;
            }
            switch (console->display.bitsPerPixel) {
                case IO_8BitsPerPixel:
                    if (console->display.colorSpace != IO_OneIsWhiteColorSpace)
                        *(dst + col) = data;
		    else {
                        data *= 3;
                        *(dst + col) = ((19595 * appleClut8[data] +
                                         38470 * appleClut8[data + 1] +
                                         7471  * appleClut8[data + 2] ) / 65536);

		    }
                    break;
                case IO_24BitsPerPixel:
                    data *= 3;
                    *(((unsigned int *)dst) + col) = (appleClut8[data] << 16)
                                                    | (appleClut8[data + 1] << 8)
                                                    |  appleClut8[data + 2];
                    break;
                case IO_15BitsPerPixel:
                    data *= 3;
                    *(((unsigned short *)dst) + col) = ( (0xf8 & (appleClut8[data])) << 7)
                                                            | ( (0xf8 & (appleClut8[data + 1])) << 2)
                                                            | ( (0xf8 & (appleClut8[data + 2])) >> 3);
                    break;
                default:
                    break;
            }
        }
        dst = (unsigned char *) ((int)dst + console->display.rowBytes);
    }
}

static void Expand4ToN( ConsolePtr console, int x, int y, int width, int height,
                        unsigned char * dataPtr, unsigned char * map16)
{
    Expand4Or8ToN(console, x, y, width, height, dataPtr, map16);
}

static void Expand8ToN( ConsolePtr console, int x, int y, int width, int height,
                        unsigned char * dataPtr)
{
    Expand4Or8ToN(console, x, y, width, height, dataPtr, 0);
}

static void DrawColorRect( ConsolePtr console, int x, int y, int width, int height, unsigned char * dataPtr)
{
    unsigned char *dst8;
    int line,col;
    unsigned int data;

    dst8 = (unsigned char *) PixelAddress(console, x, y);
    for( line = 0; line < height; line++) {
	for( col = 0; col < width; col++) {

	    data = *dataPtr++;
	    if( data != 0x01) {
		switch (console->display.bitsPerPixel) {
		    case IO_8BitsPerPixel:
                        if (console->display.colorSpace == IO_OneIsWhiteColorSpace) {
                            data *= 3;
                            *(dst8 + col) = ((19595 * appleClut8[data] +
                                              38470 * appleClut8[data + 1] +
                                              7471  * appleClut8[data + 2] ) / 65536);
                        } else {
                            *(dst8 + col) = data;
			}
                        break;
		    case IO_24BitsPerPixel:
			data *= 3;
			*(((unsigned int *)dst8) + col) = (appleClut8[data] << 16)
							| (appleClut8[data + 1] << 8)
							|  appleClut8[data + 2];
			break;
		    case IO_15BitsPerPixel:
			data *= 3;
			*(((unsigned short *)dst8) + col) = ( (0xf8 & (appleClut8[data])) << 7)
							  | ( (0xf8 & (appleClut8[data + 1])) << 2)
							  | ( (0xf8 & (appleClut8[data + 2])) >> 3);
			break;
		    default:
			break;
		}
	    }
	}
	dst8 = (unsigned char *) ((int)dst8 + console->display.rowBytes);
    }
}

static void DrawIcon( ConsolePtr console)
{

#define ICON_WIDTH 11
#define ICON_HEIGHT 14

    static unsigned char iconData[ICON_WIDTH*ICON_HEIGHT] = {
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xE3,0xE5,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0xE3,0xE5,0xE8,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0xE5,0xE8,0x01,0x01,0x01,
	0x01,0x01,0xE3,0xE3,0xE5,0x01,0x01,0xE3,0xE3,0xE8,0x01,
	0x01,0xE5,0x6F,0x26,0xE3,0xE3,0xE3,0xE3,0xE5,0xE6,0xE8,
	0x10,0x05,0x01,0x05,0x05,0x0A,0x11,0x10,0x17,0x5E,0x01,
	0x10,0x00,0x05,0x05,0x0B,0x0A,0x11,0x10,0x5F,0x01,0x01,
	0x3B,0x00,0x08,0x11,0x11,0x17,0x3B,0x3B,0x65,0x01,0x01,
	0x3B,0x00,0x08,0x11,0x17,0x17,0x17,0x3B,0x65,0x65,0x01,
	0xD8,0x07,0x14,0xD8,0xD8,0xD8,0xD8,0xD9,0xD9,0xDB,0x8F,
	0x14,0x3E,0x06,0x14,0x20,0x20,0x20,0x3E,0x3E,0x62,0x8D,
	0x01,0x3E,0x3E,0x14,0x20,0x20,0x3E,0x3E,0x62,0x8D,0x8D,
	0x01,0x01,0xEE,0xA3,0xA3,0xEF,0xCE,0xA3,0xCE,0xEF,0x01,
	0x01,0x01,0x01,0xEF,0xEF,0x01,0x01,0xEF,0xEF,0x01,0x01
    };
    int x,y;			// location of upper left corner of glyph

    x = console->window_origin_x + console->curr_column * CHAR_W - ICON_WIDTH - 7; // left: BG_MARGIN + FG_MARGIN + 2;
    y = console->window_origin_y + console->curr_row * CHAR_H - 4;

    DrawColorRect( console, x, y, ICON_WIDTH, ICON_HEIGHT, &iconData[0]);
}

/*
 * Expand a 1-bit PS bitmap, origin is lower left corner.
 */
#import "kmFontPriv.h"

void
BltPSBitmap(IOConsoleInfo *cso, int xpos, int ypos, bitmap_t *bmp,
	const unsigned char *bits, int grayValue)
{
        ConsolePtr console = (ConsolePtr)cso->priv;
        int x, y;
	int bitoffset;
        unsigned char *pixAddr;
	int fg;

        pixAddr = PixelAddress(console, (console->display.width / 2) + xpos + bmp->bbx.xoff,
                    ((console->display.height - 480) / 2) + ypos - bmp->bbx.yoff - bmp->bbx.height + 1);
        fg = console->grayTable[ grayValue & 3 ];

        /*
         * x and y are in fb coordinate system
         * xoff and yoff are in ps coordinate system
         */
        for (y = 0; y < bmp->bbx.height; y++)
        {
            for (x = 0; x < bmp->bbx.width; x++)
            {
                bitoffset = bmp->bitx + y * bmp->bbx.width + x;
                if( bits[bitoffset >> 3] & (0x80 >> (bitoffset & 0x7)))

                    switch (console->display.bitsPerPixel)
		    {
                        case IO_8BitsPerPixel:
                            ((unsigned char *)pixAddr)[ x ] = fg;
                            break;
                        case IO_12BitsPerPixel:
                        case IO_15BitsPerPixel:
                            ((unsigned short *)pixAddr)[ x ] = fg;
                            break;

                        case IO_24BitsPerPixel:
                            ((unsigned int *)pixAddr)[ x ] = fg;
                            break;
                    }
            }
            pixAddr += console->display.rowBytes;
        }
}

static void BltChar(ConsolePtr console, const char c)
// Description: Paint one character at (curr_column, curr_row).
{
    unsigned char *glyphbase;	// the glyph we're rendering
    unsigned char byte;
    unsigned char *pixAddr;
    int x,y;			// location of upper left corner of glyph
    int source_y, bold;
    unsigned char *dst8, fg8;
    unsigned short *dst16, fg16;
    unsigned int *dst32, fg32;


    // Erase current contents of {curr_row, curr_column}.
    Erase(console);
    
    // Skip non-printing characters.
    if (c < ENCODEBASE) {
	return;
    }
    bold = console->bolding;
    x = console->window_origin_x + console->curr_column * CHAR_W;
    y = console->window_origin_y + console->curr_row * CHAR_H;

    do {
	glyphbase = ohlfs12[c - ENCODEBASE];
	pixAddr = PixelAddress(console, x + bold, y);
	switch (console->display.bitsPerPixel) {
	case IO_8BitsPerPixel:
	    fg8 = (unsigned char) console->foreground;
		dst8 = (unsigned char *) pixAddr;
	    for (source_y = CHAR_H; source_y != 0; source_y--) {
		byte = *glyphbase++;
		if (byte & 0x80) dst8[0] = fg8;
		if (byte & 0x40) dst8[1] = fg8;
		if (byte & 0x20) dst8[2] = fg8;
		if (byte & 0x10) dst8[3] = fg8;
		if (byte & 0x08) dst8[4] = fg8;
		if (byte & 0x04) dst8[5] = fg8;
		if (byte & 0x02) dst8[6] = fg8;
		if (byte & 0x01) dst8[7] = fg8;
		dst8 = (unsigned char *)
				((int)dst8 + console->display.rowBytes);
	    }
	    break;
	case IO_12BitsPerPixel:
	case IO_15BitsPerPixel:
	    fg16 = (unsigned short) console->foreground;
		dst16 = (unsigned short *) pixAddr;
	    for (source_y = CHAR_H; source_y != 0; source_y--) {
		byte = *glyphbase++;
		if (byte & 0x80) dst16[0] = fg16;
		if (byte & 0x40) dst16[1] = fg16;
		if (byte & 0x20) dst16[2] = fg16;
		if (byte & 0x10) dst16[3] = fg16;
		if (byte & 0x08) dst16[4] = fg16;
		if (byte & 0x04) dst16[5] = fg16;
		if (byte & 0x02) dst16[6] = fg16;
		if (byte & 0x01) dst16[7] = fg16;
		dst16 = (unsigned short *)
				((int)dst16 + console->display.rowBytes);
	    }
	    break;
	case IO_24BitsPerPixel:
	    fg32 = (unsigned int) console->foreground;
		dst32 = (unsigned int *) pixAddr;
	    for (source_y = CHAR_H; source_y != 0; source_y--) {
		byte = *glyphbase++;
		if (byte & 0x80) dst32[0] = fg32;
		if (byte & 0x40) dst32[1] = fg32;
		if (byte & 0x20) dst32[2] = fg32;
		if (byte & 0x10) dst32[3] = fg32;
		if (byte & 0x08) dst32[4] = fg32;
		if (byte & 0x04) dst32[5] = fg32;
		if (byte & 0x02) dst32[6] = fg32;
		if (byte & 0x01) dst32[7] = fg32;
		dst32 = (unsigned int *)
				((int)dst32 + console->display.rowBytes);
	    }
	    break;
	}
    } while( bold--);
    console->curr_column++;
    return;
}

extern void video_scroll_up(unsigned char * start, unsigned char * end, unsigned char * dest);

static void FBPutC(ConsolePtr console, char c)
// Write one character to screen. Cursor is 'on' on entry and exit.
{
int	repeat;
int	i;

	/*
	 * First deal with ANSI escape sequences.
	 * This is a very bizarre implementation, copied from the m68k
	 * version. 
	 */
	switch(console->ansi_state) {
	    case AS_NORMAL:
	    	if(c == esc) {
			console->ansi_state = AS_ESCAPE;
			return;
		}
		else {
			break;				// continue
		}
		
	    case AS_ESCAPE:
	        switch(c) {
	  	    case '[':
			console->ansi_state = AS_BRACKET;
			return;
		    default:
			console->ansi_state = AS_NORMAL;
			break;				// continue
		}
	
	    case AS_BRACKET:
		if(c >= '0' && c <= '9') {
			*console->ansi_stack_p = 
				*console->ansi_stack_p * 10 + (c - '0');
			return;
		} 
		else if (c == ';') {
			if(console->ansi_stack_p < 
			    &console->ansi_stack[ANSI_STACK_SIZE]) {
				console->ansi_stack_p++;
			}
			return;
		} 
		else {
			for(i=0; i<ANSI_STACK_SIZE; i++) {
				if (console->ansi_stack[i] == 0) {
					console->ansi_stack[i] = 1; 
				}
			}
			repeat = *console->ansi_stack_p;
			FlipCursor(console);		// cursor off
			switch (c) {

			    case 'A':
				while (repeat--) {
					if(console->curr_row) {
						console->curr_row--;
					}
				}
				break;

			    case 'B':
				while (repeat--) {
					console->curr_row++;
				}
				break;

			    case 'C':		// non destructive space
				while (repeat--) {
					console->curr_column++;
				}
				break;

			    case 'D':		// not in termcap
				while (repeat--) {
					if(console->curr_column) {
						console->curr_column--;
					}
				}
				break;

			    case 'E':		// not in termcap
				console->curr_column = 0;
				while (repeat--) {
					console->curr_row++;
				}
				break;

			    case 'H':		// should be home
			    case 'f':		// not in termcap
				console->curr_column = 
					*console->ansi_stack_p - 1;
				console->ansi_stack_p--;
				console->curr_row = 
					*console->ansi_stack_p - 1;
				console->ansi_stack_p--;
				break;

			    case 'K':
				ClearToEOL(console);
				break;
				
			    case 'm': /* FIXME */
				console->ansi_stack_p--;
				console->ansi_stack_p--;
				break;
			}
		}
		console->ansi_stack_p = &console->ansi_stack[1];
		for(i=0; i<ANSI_STACK_SIZE; i++)
			console->ansi_stack[i] = 0;
		console->ansi_state = AS_NORMAL;
		goto proceed;
	}

    FlipCursor(console);		// cursor off
    switch(c) {
	case '\r':			// Carriage return
	    console->curr_column = 0;
	    break;    	
	    
	case '\n':			// line feed
	    console->curr_column = 0;
	    console->curr_row++;
	    break;
	    
	case '\b':			// backspace
	    if (console->curr_column == 0)
		    break;
	    console->curr_column--;
	    break;
	    
	case '\t':
	    // tab. Erase all characters up to and including the 
	    // next tab stop.
	    {
	    int col, num_cols;
	    
	    num_cols = TAB_SIZE - (console->curr_column % TAB_SIZE);
	    FlipCursor(console);			// cursor on
	    for (col = 0; col<num_cols; col++) {
	    	FBPutC(console, ' ');			// cursor on at return
	    }
	    FlipCursor(console);			// cursor off
	    break;
	    }
	    
	case np:				// aka ff, form feed
	    console->curr_row = console->curr_column = 0;
	    ClearWindow(console);
	    break;

	case del:
	    console->curr_column++;
	    break;
	    
	default:
	    // Normal case.
	    BltChar(console, c);
	    break;
    }
    
    // Cursor is off.
proceed:	
    if (console->curr_column >= console->chars_per_row) {
	// End-of-line wrap. 
	console->curr_column = 0;
	console->curr_row++;
    }
    
    if (console->curr_row >= console->chars_per_column) {
	//
	// Screen scroll. Copy the whole window down in memory 
	// (pixels_per_row * CHAR_H) pixels, starting at 
	// the first character in row 1.
	//
	unsigned char *src;	
	unsigned char *dst;
	int source_y;
	unsigned windowRowBytes;
	
	console->curr_row = console->chars_per_column - 1;
	switch (console->display.bitsPerPixel) {
	    case IO_8BitsPerPixel:
	        windowRowBytes = console->pixels_per_row;
		break;
	    case IO_12BitsPerPixel:
	    case IO_15BitsPerPixel:
	        windowRowBytes = 2*console->pixels_per_row;
		break;
	    case IO_24BitsPerPixel:
		windowRowBytes = 4*console->pixels_per_row;
		break;
	    default:
		panic("FBConsole/FBPutC: bogus bitsPerPixel");	
	}

	// Copy one row at a time, moving each one up
	// CHAR_H rows. Within the loop, just increment
	// pixel pointers by one row's worth of pixels.
	// Source starts at the left top of row 1; destination 
	// starts at left top of row 0.
	//
	src = PixelAddress(console, 
	    console->window_origin_x,
	    console->window_origin_y + CHAR_H);
	dst  = PixelAddress(console,
		console->window_origin_x,
		console->window_origin_y);
	for (source_y = CHAR_H;		// top, row 1
	    source_y < console->pixels_per_column;	// bottom
	    source_y++) {
#ifndef	notDefUseFPU
	    video_scroll_up( src, src + windowRowBytes, dst);
#else
	    videoMemMove(dst, src, windowRowBytes);
#endif
	    src += console->display.rowBytes;
	    dst += console->display.rowBytes;
	}

	console->curr_column = 0;
	ClearToEOL(console);
}
    
    // Cursor back on.
    FlipCursor(console);
}

static void SetTitle(ConsolePtr console, const char *title)
// Description:	Draw title bar if none exists, write specified title into it.
//		Drawing a title bar involves snarfing up row 0 of the window
//		and decreasing the usable size of the window by one row.
// Preconditions:
// *	Cursor is 'on' on entry and will remain so on exit.
{
    int saved_curr_row = console->curr_row;
    int saved_curr_column = console->curr_column;
    int saved_origin_y;
    int title_len = strlen(title);
    int pixel_num;
    
    if ((title_len == 0) || (title_len > console->chars_per_row)) {
	    IOLog("console: Illegal title length (%d)\n", title_len);
	    return;
    }
    FlipCursor(console);			// old cursor off
    if (console->has_title) {
	// First grow window by two rows - this absorbs the existing 
	// title bar.
	console->window_origin_y -= CHAR_H * 2;
	console->chars_per_column += 2;
	console->pixels_per_column += CHAR_H * 2;
	saved_curr_row += 2;
    }

    // Clear row 0, center new title in it.
    console->curr_row = 0;
    Rect(console,
	console->window_origin_x - (BG_MARGIN - TS_MARGIN),
	console->window_origin_y - (BG_MARGIN - TS_MARGIN),
	CHAR_W * console->chars_per_row + 2 * (BG_MARGIN - TS_MARGIN),
	CHAR_H * 2 - TS_MARGIN,
	console->light_grey);
    saved_origin_y = console->window_origin_y;
    console->window_origin_y += CHAR_H / 2;
    console->curr_column = (console->chars_per_row - title_len) / 2;
    DrawIcon(console);
//	SwapForegroundAndBackground(console);
    console->bolding = 1;
    console->background = console->light_grey;
    FlipCursor(console);				// new cursor on	
    while (*title) {
	FBPutC(console, *title++);
    }
    FlipCursor(console);			// new cursor off	
    console->background = console->grayTable[0];
    console->bolding = 0;
//    SwapForegroundAndBackground(console);
    console->window_origin_y = saved_origin_y;
    
    // Draw grey lines one pixel away from border of title bar.
    // Origin is currently at first pixel in top line of title bar.

    Rect(console,				// inside top
	console->window_origin_x - BG_MARGIN,
	console->window_origin_y - BG_MARGIN,
	console->pixels_per_row + (BG_MARGIN * 2),
	TS_MARGIN,
	console->background);
    Rect(console,				// inside bottom
	console->window_origin_x - BG_MARGIN,
	console->window_origin_y + CHAR_H * 2 - TOTAL_MARGIN - TS_MARGIN,
	console->pixels_per_row + BG_MARGIN * 2,
	TS_MARGIN,
	console->dark_grey);
    for (pixel_num = 0; pixel_num < TS_MARGIN; pixel_num++) {
        Rect(console,				// left
	    console->window_origin_x - BG_MARGIN + pixel_num,
	    console->window_origin_y - BG_MARGIN + pixel_num,
	    1,
	    CHAR_H * 2 - 1 - (pixel_num * 2),
	    console->background);
    }
    for (pixel_num = 1 + BG_MARGIN - TS_MARGIN; pixel_num <= BG_MARGIN; pixel_num++) {
        Rect(console,				// right
	    console->window_origin_x + console->pixels_per_row + pixel_num - 1,
	    console->window_origin_y - pixel_num,
	    1,
	    CHAR_H * 2 - 1 - (BG_MARGIN - pixel_num) * 2,
	    console->dark_grey);
    }
    Rect(console,				// outside bottom
	console->window_origin_x - TOTAL_MARGIN,
	console->window_origin_y + CHAR_H * 2 - FG_MARGIN - BG_MARGIN,
	console->pixels_per_row + TOTAL_MARGIN * 2,
	FG_MARGIN,
	console->foreground);
	    
    // Diminish window size by two rows.
    console->window_origin_y += CHAR_H * 2;
    console->chars_per_column -= 2;
    console->pixels_per_column -= CHAR_H * 2;
    

    // Restore old cursor. Careful, the y axis just changed...
    console->curr_column = saved_curr_column;
    if (saved_curr_row > 0) {
	console->curr_row = saved_curr_row - 2;
    } 
    else {
	// New row 0 - clear it.
	ClearToEOL(console);
    }
    FlipCursor(console);			// old cursor on
    console->has_title = TRUE;	
}


static void UpdateWindowChars(ConsolePtr console)
// Description:	Calculate window size in characters. Used when either window
//		size or font changes.
{
    console->chars_per_row = console->pixels_per_row / CHAR_W;
    console->chars_per_column =
        console->pixels_per_column / CHAR_H;
}



static void InitWindow(ConsolePtr console,
	int		width,
	int 		height,
	const char 	*title,
	boolean_t 	initWindow,
	boolean_t	saveUnder)
// Description:	Initialize window parameters. If initWindow is TRUE, an empty
//		window is drawn in the screen, else we assume that a window
//		already  exists. Screen and font parameters must be valid on
//		entry.
{
    // Truncate specified size to integral mutliple of character size
    // in both dimensions.
    width  /= CHAR_W;		// now in chars
    height /= CHAR_H;
    width  *= CHAR_W;		// now in pixels
    height *= CHAR_H;
    
    // Ensure there's enough room for a border.
    if (width > (console->display.width - HORIZ_BORDER - TOTAL_MARGIN * 2 - DROP_SHADOW))
	width = console->display.width - HORIZ_BORDER - TOTAL_MARGIN * 2 - DROP_SHADOW;
    if (height > (console->display.height - VERT_BORDER - TOTAL_MARGIN * 2 - DROP_SHADOW))
	height = console->display.height - VERT_BORDER - TOTAL_MARGIN * 2 - DROP_SHADOW;
    width &= 0xfffffffe;
	    
    //
    // Init console window variables. The window is centered in the 
    // ! screen, minus a pixel or two to guarantee double word alignment
    // ! for the first pixel of a row (until further developments...when 
    // ! we do alert panels, we can enumerate some cases here...).
    //
    console->window_origin_x   = (console->display.width - width) / 2;
    console->window_origin_y   = (console->display.height - height) / 2;
    console->pixels_per_row    = width;
    console->pixels_per_column = height;
    (unsigned)console->window_origin_x &= 0xfffffffe;

    UpdateWindowChars(console);
    
    console->curr_column = 0;
    console->has_title = FALSE;
    
    if (initWindow) {
	// Before we start drawing anything, save under if needed
	if (saveUnder) {
	    int i, bytesPerPixel = 0;
	    unsigned char *save, *sp;

	    // Compute size of save under region in bytes and allocate
	    switch (console->display.bitsPerPixel) {
		case IO_8BitsPerPixel:  bytesPerPixel = 1; break;
		case IO_12BitsPerPixel:
		case IO_15BitsPerPixel: bytesPerPixel = 2; break;
		case IO_24BitsPerPixel: bytesPerPixel = 4; break;
	    }
	    console->saveHeight = height + TOTAL_MARGIN * 2 + DROP_SHADOW;
	    console->saveRowBytes = (width + TOTAL_MARGIN * 2 + DROP_SHADOW)*bytesPerPixel;
	    console->saveBytes = console->saveHeight*console->saveRowBytes;
	    save = console->saveBits = (unsigned char *)
                                        kalloc_noblock(console->saveBytes);
	    sp = console->saveLocation = 
		PixelAddress(console, console->window_origin_x - TOTAL_MARGIN, 
			      console->window_origin_y - TOTAL_MARGIN);

	    // Copy the window contents into the save under
	    if( save)
                for (i = console->saveHeight; i != 0; i--) {
                    videoMemMove(save, sp, console->saveRowBytes);
                    sp += console->display.rowBytes;
                    save += console->saveRowBytes;
                }
	}
    
	// New window - cursor at origin.
	console->curr_row = 0;
	console->has_title = FALSE;
	
	// Draw the border. The border goes outside of the specified
	// window. Outside lines are black; inside lines are 
	// half-tone.
	Rect(console,				// outside top
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    width + TOTAL_MARGIN * 2,
	    FG_MARGIN,
	    console->foreground);
	Rect(console,				// inside top
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y - BG_MARGIN,
	    width + BG_MARGIN * 2,
	    BG_MARGIN,
	    console->background);
	Rect(console,				// inside bottom
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y + height,
	    width + BG_MARGIN * 2,
	    BG_MARGIN,
	    console->background);
	Rect(console,				// outside bottom
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y + height + BG_MARGIN,
	    width + TOTAL_MARGIN * 2,
	    FG_MARGIN,
	    console->foreground);
	if( DROP_SHADOW) Rect(console,				// outside bottom shadow
	    console->window_origin_x - TOTAL_MARGIN + DROP_SHADOW,
	    console->window_origin_y + height + BG_MARGIN + DROP_SHADOW,
	    width + TOTAL_MARGIN * 2,
	    DROP_SHADOW,
	    console->dark_grey);
	Rect(console,				// outside left
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    FG_MARGIN,
	    height + TOTAL_MARGIN * 2,
	    console->foreground);
	Rect(console,				// inside left
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y - BG_MARGIN,
	    BG_MARGIN,
	    height + BG_MARGIN * 2,
	    console->background);
	Rect(console,				// inside right
	    console->window_origin_x + width,
	    console->window_origin_y - BG_MARGIN,
	    BG_MARGIN,
	    height + BG_MARGIN * 2,
	    console->background);
	Rect(console,				// outside right
	    console->window_origin_x + width + BG_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    FG_MARGIN,
	    height + TOTAL_MARGIN * 2,
	    console->foreground);
	if( DROP_SHADOW) Rect(console,				// outside right shadow
	    console->window_origin_x + width + BG_MARGIN + DROP_SHADOW,
	    console->window_origin_y - TOTAL_MARGIN + DROP_SHADOW,
	    DROP_SHADOW,
	    height + TOTAL_MARGIN * 2,
	    console->dark_grey);
		
	// Clear the contents of the window and draw a cursor.
	ClearWindow(console);
	FlipCursor(console);			// cursor on
	SetTitle(console, title);
    }
    else {
	    /* Leave current window contents, just updating title. 
	     * We'll do a newline to make sure we have room for 
	     * our font and proceed.Initial cursor will be at 
	     * lower left corner.
	     */
	    console->curr_row = console->chars_per_column - 1;
	    SetTitle(console, title);
	    FBPutC(console, '\n');
    }
    return;
}

//
// END:		Private utility routines
//

//
// BEGIN:	Implementation of IOConsoleInfo functions
//
static void Free(IOConsoleInfo *cso)
// Description: Free a FB Console object
{
    IOFree(cso->priv, sizeof(ConsoleRep));
    IOFree(cso, sizeof(IOConsoleInfo));
}

static void Init(
    IOConsoleInfo *cso,
    ScreenMode mode,
    boolean_t initScreenOrSaveUnder,
    boolean_t initWindow,
    const char *title)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    int i;
	
    switch (console->display.bitsPerPixel) {
        case IO_8BitsPerPixel:
	    if (console->display.colorSpace == IO_OneIsWhiteColorSpace) {
		console->baseground = (TRUE || (mode == SCM_GRAPHIC)) ? 107 : 0x55;
		console->background = 0xff;
		console->foreground = 0x00;
		console->dark_grey  = 85;
		console->light_grey = 187;
	    } else {
		console->baseground = (TRUE || (mode == SCM_GRAPHIC)) ? 0x80 : 0x7e;
		console->background = 0x00;
		console->foreground = 0xff;
		console->dark_grey  = 0xfb;
		console->light_grey = 0xf7;
	    }
	    break;
	case IO_15BitsPerPixel:
	    console->baseground = (TRUE || (mode == SCM_GRAPHIC)) ? 0x3193 : 0x295f;
	    console->background = 0x7fff;
	    console->foreground = 0x0000;
	    console->dark_grey  = 0x294a;
	    console->light_grey = 0x5ef7;
	    break;
	case IO_24BitsPerPixel:
	    console->baseground = (TRUE || (mode == SCM_GRAPHIC)) ? 0xff666699 : 0xff5555ff;
	    console->background = 0xffffffff;
	    console->foreground = 0xff000000;
	    console->dark_grey  = 0xff555555;
	    console->light_grey = 0xffbbbbbb;
	    break;
    }

    console->grayTable[0] = console->background;
    console->grayTable[1] = console->light_grey;
    console->grayTable[2] = console->dark_grey;
    console->grayTable[3] = console->foreground;

    console->ansi_state = AS_NORMAL;
    for(i=0; i<ANSI_STACK_SIZE; i++) {
		console->ansi_stack[i] = 0;
	}
    console->ansi_stack_p = &console->ansi_stack[1];	// FIXME - why not 0?

    // Initialize the screen, if appropriate.
    if (initScreenOrSaveUnder && (mode != SCM_ALERT)) {
	WipeScreen(console, console->baseground);
    }

    console->window_type = mode;
    switch(console->window_type) {
	case SCM_TEXT:
	    InitWindow(console,	TEXT_WIN_WIDTH, TEXT_WIN_HEIGHT,
		title, initWindow, 0 /* Don't save under */);
	    break;
	case SCM_ALERT:
	    InitWindow(console,	ALERT_WIN_WIDTH, ALERT_WIN_HEIGHT,
		title, initWindow, initScreenOrSaveUnder );
	    break;
	case SCM_GRAPHIC:
	    if( initScreenOrSaveUnder) {
		int cx, cy;

                cx = console->display.width / 2;
                cy = console->display.height / 2;
#if 0
                Expand4ToN( console, cx + RIGHT_DX, cy + RIGHT_DY,
			RIGHT_WIDTH, RIGHT_HEIGHT, rightData, rightBotColors );
                Expand4ToN( console, cx + CENTER_DX, cy + CENTER_DY,
			CENTER_WIDTH, CENTER_HEIGHT, centerData, centerColors );
                Expand4ToN( console, cx + BOTTOM_DX, cy + BOTTOM_DY,
			BOTTOM_WIDTH, BOTTOM_HEIGHT, bottomData, rightBotColors );
#else
                Expand8ToN( console, cx + CENTER_DX, cy + CENTER_DY,
			CENTER_WIDTH, CENTER_HEIGHT, centerData );
#endif
	    }
	    break;

	default:
	    panic("FBConsole/FBInitConsole: can't init");
	    break;
    }
    return;
}

static int Restore(IOConsoleInfo *cso)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    unsigned char *save, *dp;
    int i;

    if (console->window_type != SCM_ALERT) {
	IOLog("frameBuffer: bogus restore, mode %d\n", console->window_type);
	return -1;
    }

    // Copy save under bits back onto screen
    save = console->saveBits;
    dp = console->saveLocation;
    if( save) {
        for (i = console->saveHeight; i != 0; i--) {
            videoMemMove(dp, save, console->saveRowBytes);
            dp += console->display.rowBytes;
            save += console->saveRowBytes;
        }

        // Free bits
        kfree(console->saveBits, console->saveBytes);
	console->saveBits = 0;
    }
    return 0;
}

#define	VIDEO_W	640
#define	VIDEO_H	480

static int DrawRect(IOConsoleInfo *cso,  const struct km_drawrect *km_rect)
// Description:	Given a 2 bit raster, convert it to the format appropriate for
//		the display. The co-ordinates passed in by these ioctls assume
//		an (VIDEO_W x VIDEO_H) screen. All screen sizes are
//		assumed to share a common center point. We scale the X and Y
//		values to maintain a constant offset from the center point,
//		so that boot animations, popup windows, and related goodies
//		don't break with screens larger or smaller than the 'default'
//		screen.
// Preconditions:
// * The rectangle that we are to draw must start on a 4 pixel boundary
//   and should be a multiple of 4 pixels in width.
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    int pixel;		/* Starting pixel in screen */
    int i, x, y;
    unsigned char *data;
    unsigned char value;
    unsigned int * table;
    struct km_drawrect *rect = (struct km_drawrect *) km_rect;
    int bgX, bgY, bgH;
    unsigned char * bg;

    table = console->grayTable;

    /* copyin is done in km */
    data = rect->data.bits;

    x = (rect->x & 0x1ffc) + (console->display.width - VIDEO_W)/2;
    y = rect->y + (console->display.height - VIDEO_H)/2;

    /* Correct the x and y values for screen size */
    if( (rect->x & 3) == 3) {
	DrawColorRect( console, x, y, rect->width, rect->height, data);
	return 0;
    }

#if TEXTURE_BACKGND
    bgX = console->display.width / 2 + CENTER_DX;
    bgY = console->display.height / 2 + CENTER_DY;

    bgH = (y - bgY) & -2;
    if( bgH > 0) {
        bgY += bgH;
        bg = centerData + (bgH * CENTER_WIDTH);
        bgH = CENTER_HEIGHT - bgH;
    } else
	bg = (unsigned char *)0;
#endif

    rect->x &= 0x1ffc;

    /* Sanity check */
    if ((x + rect->width > console->display.width)
    || (y + rect->height) > console->display.height)
	return( -1 );


    pixel = x + (y * console->display.totalWidth);

    switch ( console->display.bitsPerPixel )
    {
	case IO_2BitsPerPixel:	/* 2 bit pixels */
	{
	    unsigned char *dst = (unsigned char *)console->display.frameBuffer;
	    unsigned char *fb;
	    unsigned char final_value;
	    
	    dst += pixel >> 2;
	    for ( y = 0; y < rect->height; ++y ) {
		fb = dst;
		for ( x = 0; x < rect->width; x += 4) {
			value = *data++;
			final_value = 0;
			for ( i = 0; i < 8; i += 2 ) {
			    final_value |= ((table[(value>>i)&3] & 3) << i);
			}
			*fb++ = final_value;
		}
		dst += console->display.rowBytes;
	    }
	    break;
	}
	case IO_8BitsPerPixel:	/* 8 bit pixels */
	{
	    unsigned char *dst = (unsigned char *)console->display.frameBuffer;
	    unsigned char *fb;

	    dst += pixel;
	    for ( y = 0; y < rect->height; ++y ) {
		fb = dst;
#if TEXTURE_BACKGND
		Expand8ToN( console, bgX, bgY,
			CENTER_WIDTH, 1, bg );
		bgY += 1;
		bg += CENTER_WIDTH;
#endif
		for ( x = 0; x < rect->width; x += 4) {
                    value = *data++;
                    if( value) {
                    int pix;
                        pix = (value >> 6) & 3;
                        if( pix)
                            fb[0] = table[pix];
                        pix = (value >> 4) & 3;
                        if( pix)
                            fb[1] = table[pix];
                        pix = (value >> 2) & 3;
                        if( pix)
                            fb[2] = table[pix];
                        pix = value & 3;
                        if( pix)
                            fb[3] = table[pix];
                    }
                    fb += 4;
		}
		dst += console->display.rowBytes;
	    }	    	
	    break;
	}
	case IO_12BitsPerPixel:	/* 16 bit pixels */
	case IO_15BitsPerPixel:	/* 16 bit pixels */
	{

	    unsigned short *dst = (unsigned short *)console->display.frameBuffer;
	    unsigned short *fb;
	    dst += pixel;

	    for ( y = 0; y < rect->height; ++y ) {
		fb = dst;
#if TEXTURE_BACKGND
		Expand8ToN( console, bgX, bgY,
			CENTER_WIDTH, 1, bg );
		bgY += 1;
		bg += CENTER_WIDTH;
#endif
		for ( x = 0; x < rect->width; x += 4) {
                    value = *data++;
                    if( value) {
                    int pix;
                        pix = (value >> 6) & 3;
                        if( pix)
                            fb[0] = table[pix];
                        pix = (value >> 4) & 3;
                        if( pix)
                            fb[1] = table[pix];
                        pix = (value >> 2) & 3;
                        if( pix)
                            fb[2] = table[pix];
                        pix = value & 3;
                        if( pix)
                            fb[3] = table[pix];
                    }
                    fb += 4;
		}
		dst = (unsigned short *)(((char*)dst) +
		    console->display.rowBytes);
	    }	    	
	    break;
	}
	case IO_24BitsPerPixel:	/* 32 bit pixels */
	{
	    unsigned int *dst = (unsigned int *) console->display.frameBuffer;
	    unsigned int *fb;

	    dst += pixel;
	    for ( y = 0; y < rect->height; ++y ) {
		fb = dst;
#if TEXTURE_BACKGND
		Expand8ToN( console, bgX, bgY,
			CENTER_WIDTH, 1, bg );
		bgY += 1;
		bg += CENTER_WIDTH;
#endif
		for ( x = 0; x < rect->width; x += 4) {
                    value = *data++;
                    if( value) {
                    int pix;
                        pix = (value >> 6) & 3;
                        if( pix)
                            fb[0] = table[pix];
                        pix = (value >> 4) & 3;
                        if( pix)
                            fb[1] = table[pix];
                        pix = (value >> 2) & 3;
                        if( pix)
                            fb[2] = table[pix];
                        pix = value & 3;
                        if( pix)
                            fb[3] = table[pix];
                    }
                    fb += 4;
		}
		dst = (unsigned int *)(((char*)dst) +
		    console->display.rowBytes);
	    }	    	
	    break;
	}
    }
    return 0;
}

static int EraseRect(IOConsoleInfo *cso,  const struct km_drawrect *km_rect)
// Description:	Erase the specified rectangle to the color specified
//		in the km_rect structure.
// Preconditions:
// * The rectangle that we are to erase must start on a 4 pixel boundary
//   and should be a multiple of 4 pixels in width.
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    int pixel;		// Starting pixel in screen
    int x, y, w, h;
    unsigned int value;
    struct km_drawrect *rect = (struct km_drawrect *) km_rect;
    int bgX, bgY, bgH;
    unsigned char * bg;

    value = console->grayTable[ rect->data.fill & 3 ];

    /* Correct the x and y values for screen size */
    x = rect->x + (console->display.width - VIDEO_W)/2;
    y = rect->y + (console->display.height - VIDEO_H)/2;

    x &= ~3;				// Trunc to 4 pixel boundary
    w = (rect->width) & ~3;		// Round dn to 4 pixel boundary
    h = rect->height;

    /* Sanity check */
    /* Sanity check */
    if (((x + w) > console->display.width)
    ||  ((y + h) > console->display.height) )
	return( -1 );


#if TEXTURE_BACKGND
    bgX = console->display.width / 2 + CENTER_DX;
    bgY = console->display.height / 2 + CENTER_DY;

    bgH = (y - bgY) & -2;
    if( bgH > 0) {
        bgY += bgH;
        bg = centerData + (bgH * CENTER_WIDTH);
        bgH = CENTER_HEIGHT - bgH;
        Expand4ToN( console, bgX, bgY,
                CENTER_WIDTH, bgH, bg, centerColors );
    }

#else
    Rect( console, x, y, w, h, value);
#endif

    return 0;
}

//extern void czputc( char c);

static void PutC(IOConsoleInfo *cso, char c)
// Write one character to screen. Cursor is 'on' on entry and exit.
{
    FBPutC((ConsolePtr)cso->priv, c);
//    czputc(c);
}

static void GetSize(IOConsoleInfo *cso, struct ConsoleSize *s)
// Return the size of the screen in pixels
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    s->cols = console->chars_per_row;
    s->rows = console->chars_per_column;
    s->pixel_width = console->display.width;
    s->pixel_height = console->display.height;
}

//
// END:		Implementation of IOConsoleInfo functions
//

static IOConsoleInfo staticConsoleInfo;
static ConsoleRep staticConsoleRep;

static void InitializeConsole( IOConsoleInfo *cso, IODisplayInfo *display )
{
    cso->Init = Init;
    cso->Restore = Restore;
    cso->DrawRect = DrawRect;
    cso->EraseRect = EraseRect;
    cso->PutC = PutC;
    cso->GetSize = GetSize;
    ((ConsolePtr)cso->priv)->display = *display;
    ((ConsolePtr)cso->priv)->window_type = SCM_UNINIT;
    return;
}

//
// BEGIN:	Exported routines
//

static void DoNothing(IOConsoleInfo *cso)
{
}

IOConsoleInfo *BasicAllocateConsole(IODisplayInfo *display)
{
// No memory allocation. Assume non backed window.
    IOConsoleInfo *cso = &staticConsoleInfo;
    cso->priv = (void *)&staticConsoleRep;
    InitializeConsole(cso, display);
    cso->Free = DoNothing;
#if 0
    printf("base:%x rowBytes:%x width:%x height:%x\n",display->frameBuffer,
		display->rowBytes, display->width, display->height);
#endif
    return cso;
}


IOConsoleInfo *FBAllocateConsole(IODisplayInfo *display)
{
   IOConsoleInfo *cso = (IOConsoleInfo *)IOMalloc(sizeof(IOConsoleInfo));
    
    if (cso == NIL)
        return NIL;
    cso->priv = (void *)IOMalloc(sizeof(ConsoleRep));
    if (cso->priv == NIL)
    {
        IOFree(cso, sizeof(IOConsoleInfo));
	return NIL;
    }
    InitializeConsole(cso, display);
    cso->Free = Free;
   return cso;
}


//
// END:		Exported routines
//
