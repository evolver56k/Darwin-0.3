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
 * VGAConsole.c - VGA Mode 0x12 console implementation.
 *
 *
 * HISTORY
 * 02 Feb 93	Peter Graffagnino
 *      Created from FBConsole.c
 */

//
// Notes:
// * Considerations for supporting other VGA modes:
//	This code uses off screen memory for the save behind area.  This
//	permits the use of write mode 1 to save the data resulting in
//	a flicker-free restore, and makes the coding somewhat simpler. Care
//	should be taken that there is enough memory there in other modes,
//	or allocated storage.
//	
//	Since mode 12 fits comfortably in 64K/plane, no bank switching is
//	currently done in this code.  Higher res modes will need this.
//
// * This module implements the console functionality required by the
//   ConsoleSupport protocol. 
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.

#import <sys/syslog.h>
#import	<bsd/dev/i386/VGAConsole.h>
#import	<bsd/dev/i386/VGAConsPriv.h>
#import <driverkit/i386/displayRegisters.h>
#import <architecture/ascii_codes.h>

#define	NIL	(0)

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif

/* Window margins */
#define BG_MARGIN	2	// pixels of background to leave as margin
#define FG_MARGIN	1	// pixels of foreground to leave as margin
#define TOTAL_MARGIN	(BG_MARGIN + FG_MARGIN)

#define SVGA_SAVE_BITS	(2 * SVGA_ROWBYTES * SVGA_HEIGHT)

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
#define ANSI_STACK_MAX 2

typedef struct _t_VGARegisters {
    unsigned char GC[VGA_NUM_GC_REGS];
    unsigned char SEQ[VGA_NUM_SEQ_REGS];
} VGARegisters;

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
    unsigned int	baseground;	// bits for base color
    unsigned int	background;	// bits for background pixel
    unsigned int	foreground;	// bits for foreground pixel
    unsigned int	dark_grey;	// bits for dark grey
    unsigned int	light_grey;	// bits for light grey
    boolean_t		has_title;

    //
    // Storage for saved region under an alert panel.
    //
    unsigned char 	*saveBits;
    int 		saveHeight;
    int 		saveRowBytes;
    int 		saveBytes;
    unsigned char 	*saveLocation;
    VGARegisters 	savedRegisters;	// save for Restore
    VGARegisters	tempRegisters;	// save between operations
    
    ansi_state_t	ansi_state;	// track ansi escape sequence state
    u_char 		ansi_stack[ANSI_STACK_SIZE];
    u_char 		*ansi_stack_p;
    
    boolean_t		isSVGA;		// use SVGA methods
} ConsoleRep, *ConsolePtr;

#define PLANE_MASK	0x0F		// By default, use all planes
#define PIXEL_MASK	0xFF		// By default, all pixels
#define RW_MODE		0x00		// Default VGA read/write mode

//
// BEGIN:	Private utility routines
//

static inline int isTextMode(ConsolePtr console)
{
    return ((console->window_type == SCM_TEXT) ||
	    (console->window_type == SCM_ALERT));
}

static void SwapForegroundAndBackground(ConsolePtr console)
{
    int temp_color;
    
    temp_color = console->foreground;
    console->foreground = console->background;
    console->background = temp_color;
}

static inline int rread(int port, int index)
{
    outb(port, index);
    return(inb(port + 1));
}


static inline void SaveVGARegs(VGARegisters *saved)
{
    int i;
    
    for (i = 0; i < VGA_NUM_GC_REGS; i++)
	saved->GC[i] = rread(VGA_GC_ADDR,i);
#if	defined(SAVE_ALL_SEQ_REGS)
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++)
	saved->SEQ[i] = rread(VGA_SEQ_ADDR,i);
#else	/* defined(SAVE_ALL_SEQ_REGS) */
    saved->SEQ[2] = rread(VGA_SEQ_ADDR,2);
#endif	/* defined(SAVE_ALL_SEQ_REGS) */
}

static inline void RestoreVGARegs(VGARegisters *saved)
{
    int i;
    
    for (i = 0; i < VGA_NUM_GC_REGS; i++)
	IOWriteRegister(VGA_GC_ADDR,i, saved->GC[i]);
#if	defined(SAVE_ALL_SEQ_REGS)
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++)
	IOWriteRegister(VGA_SEQ_ADDR,i, saved->SEQ[i]);
#else	/* defined(SAVE_ALL_SEQ_REGS) */
    IOWriteRegister(VGA_SEQ_ADDR, 2, saved->SEQ[2]);
#endif	/* defined(SAVE_ALL_SEQ_REGS) */
}

static void FlipCursor(ConsolePtr console)
// Description: Turn cursor on/off. This just inverts every pixel at the
// character at (curr_rown, curr_column).
{

    int y;
    // Screen-relative pixel coordinates of bounds of current  character.
    int start_y = console->window_origin_y +
        console->curr_row * CHAR_H;
    int end_y   = start_y + CHAR_H;
    int start_x = console->window_origin_x +
        console->curr_column * CHAR_W;
    unsigned char *cp;
    volatile char mask;
    
    IOWriteRegister(VGA_GC_ADDR, 7, 0x0f);		/* enable cdc planes */
    IOWriteRegister(VGA_GC_ADDR, 5, 0x08);		/* set read mode 1 */
    IOWriteRegister(VGA_GC_ADDR, 2, console->background); /* setup color compare */
    cp = (unsigned char *)console->display.frameBuffer + 
          start_y*console->display.rowBytes + (start_x >> 3);
    for (y=start_y; y < end_y; y++)  {
        mask = *cp;
	IOWriteRegister(VGA_GC_ADDR, 0, console->background); /* set color */
	IOWriteRegister(VGA_GC_ADDR, 8, ~mask);		     /* set pixel mask */
	*cp = 0xff;
	IOWriteRegister(VGA_GC_ADDR, 0, console->foreground); /* set color */
	IOWriteRegister(VGA_GC_ADDR, 8, mask);		     /* set pixel mask */
	mask = *cp;
	*cp = 0xff;
	 cp += console->display.rowBytes;
    }

    IOWriteRegister(VGA_GC_ADDR, 5, RW_MODE);		/* set read mode 0 */
    IOWriteRegister(VGA_GC_ADDR, 8, PIXEL_MASK);		/* set pixel mask */

}

/*
 * Bank switching is currently not supported.
 */
 
static inline char SVGASegmentForAddress(register void *addr)
{
#if NOTYET
    register char segment;
    register int offset;

    offset = ((int)addr) - ((int)SVGA_FRAMEBUFFER_ADDRESS);
    segment = offset % SVGABANKBYTES;
    return(segment);
#else NOTYET
    return 0;
#endif NOTYET
}

static inline void SVGASelectSegment(char segment)
{
#if NOTYET
    outb(VGA_GCR_SEGS, segment << 4 | segment);
#endif NOTYET
}

static inline void SVGASelectSegmentForAddressIfNecessary(
    unsigned char *addr,
    char *currentSegmentPtr
)
{
#if NOTYET
    char segment;

    segment = SVGASegmentForAddress(addr);

    if (segment != *currentSegmentPtr) {
	SVGASelectSegment(segment);
	*currentSegmentPtr = segment;
    }
#endif NOTYET
}


static unsigned char leftMaskArray[] =
  {0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1};
  
static unsigned char rightMaskArray[] = 
  {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};



static void rect(ConsolePtr console, int x, int y, int w, int h, int c)
{
    int first_byte, last_byte;
    unsigned char lmask, rmask;
    int k, midcnt;
    int x2 = x + w;
    volatile unsigned char *rowptr;
    int rowBytes = console->display.rowBytes;

    IOWriteRegister(VGA_GC_ADDR, 0, c);			/* set color */
    outb(VGA_GC_ADDR, 8);			/* point to pixel mask reg */
    first_byte = x >> 3;
    last_byte = x2 >> 3;
    lmask = leftMaskArray[x & 7];
    rmask = rightMaskArray[x2 & 7];

    rowptr = (unsigned char *)console->display.frameBuffer + 
	y*rowBytes  + first_byte;

    midcnt = last_byte - first_byte - 1;
    if (midcnt == -1) {  /* i.e. first_word == last_word */
	outb(VGA_GC_ADDR + 1, rmask & lmask);
	for(; --h >= 0;rowptr += rowBytes) {
	    volatile char forceReadByte = *rowptr;
	    *rowptr = 0xff;
	}
    } else {
	for(; --h >= 0; rowptr += rowBytes) {
	    volatile unsigned char *p =  rowptr;
	    volatile char foo = *p;
	    outb(VGA_GC_ADDR + 1, lmask);
	    *p++ = 0xff;
	    outb(VGA_GC_ADDR + 1, 0xff);
	    for(k = midcnt; --k >= 0;)
		*p++ = 0xff; 		// no need to latch data.
	    outb(VGA_GC_ADDR + 1, rmask);
	    foo = *p;
	    *p = 0xff;
	}
    }
    outb(VGA_GC_ADDR + 1, PIXEL_MASK);
    
}


static void ClearWindow(ConsolePtr console)
// Description:	 Set contents of entire current window to background.
// Preconditions:
// *	Cursor is off on entry and will remain so on exit.
{
    rect(console, console->window_origin_x, console->window_origin_y,
	 console->pixels_per_row, console->pixels_per_column,
	 console->background);
}

static void WipeScreen(ConsolePtr console, const unsigned pixel)
// Description:	Fill the screen with specific pixel value.  
{
	rect(console, 0, 0, console->display.totalWidth,
	     console->display.height, pixel);
}

static void ClearToEOL(ConsolePtr console)
// Description:	Clear to end of line (within window), including current pos.
// Preconditions:
// *	Cursor is 'off' on entry and will remain so on exit.
{
    unsigned pixel_count;
    int starting_x;			// window-relative 'x' coordinate
    int y;

    starting_x = console->window_origin_x + 
	(console->curr_column * CHAR_W);
    y = console->window_origin_y + (console->curr_row * CHAR_H);

    pixel_count = console->pixels_per_row -
        (starting_x - console->window_origin_x);

    rect(console, starting_x, y, pixel_count, CHAR_H , console->background);

}

static void Erase(ConsolePtr console)
// Description: Erase current cursor location.
{
    // Screen-relative pixel coordinates of bounds of current character.
    int start_y = console->window_origin_y +
        console->curr_row * CHAR_H;
    int start_x = console->window_origin_x +
        console->curr_column * CHAR_W;

    rect(console, start_x, start_y, CHAR_W, CHAR_H, console->background);
}

#define ENCODEBASE ' '

static void BltChar(ConsolePtr console, const char c)
// Description: Paint one character at (curr_column, curr_row).
{
    unsigned char *glyphbase;	// the glyph we're rendering
    int x,y;			// location of upper left corner of glyph
    int source_y;
    volatile int tmp;
    unsigned char *cp;

    // Erase current contents of {curr_row, curr_column}.
    Erase(console);
    
    // Skip non-printing characters.
    if (c < ENCODEBASE) {
	return;
    }
	    
    glyphbase = ohlfs12[c - ENCODEBASE];
    x = console->window_origin_x + console->curr_column * CHAR_W;
    y = console->window_origin_y + console->curr_row * CHAR_H;

    IOWriteRegister(VGA_GC_ADDR, 0, console->foreground);/* set color */
    cp  = (unsigned char *)console->display.frameBuffer + 
	    y*console->display.rowBytes + (x >> 3);
    outb(VGA_GC_ADDR, 8);	/* point to pixel mask reg */

    for (source_y = CHAR_H; source_y != 0; source_y--) {
	outb(VGA_GC_ADDR + 1, *glyphbase++);
        tmp = *cp;
	*cp = 0xff;
	 cp += console->display.rowBytes;
    }
    outb(VGA_GC_ADDR + 1, PIXEL_MASK);  /* restore to default */

    console->curr_column++;
    return;
}

/*
 * Use bytemove as opposed to memmove.  32bit bus cycles seem to confuse
 * the VGA.
 */
static inline void bytemove(char *dst, char *src, int cnt)
{
#if DONT_USE_ASM
    while(cnt--) 
	*dst++ = *src++;
#else DONT_USE_ASM
    asm("rep; movsb"
	: /* no outputs */
	: "&c" (cnt), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
#endif DONT_USE_ASM
}

static void FBPutC(ConsolePtr console, char c)
// Write one character to screen. Cursor is 'on' on entry and exit.
{
	int	repeat;
	int	i;

	if (!isTextMode(console))
	    return;

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
			    &console->ansi_stack[ANSI_STACK_MAX]) {
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
				
				/* prevent washout */
				if(console->background ==
					console->foreground) {
					console->background = VGA_WHITE;
					console->foreground = VGA_BLACK;
				}
				break;
			}
		}
		console->ansi_stack_p = &console->ansi_stack[1];
		for(i=0; i<ANSI_STACK_SIZE; i++)
			console->ansi_stack[i] = 0;
		console->ansi_state = AS_NORMAL;
		goto proceed;
	    case AS_R:
	    	// FIXME
		break;
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
	
	console->curr_row = console->chars_per_column - 1;

	/*
	 * for screen-to-screen scrolling in graphics mode, we use
	 * write mode 1, and pretend its a 1 bit display.  We also
	 * exploit the fact the the scrolling area begins and ends
	 * on a 0 mod 8 pixel address.
	 */
	
	IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x1);	/* write mode 1 */
	src = (unsigned char *)console->display.frameBuffer + 
	    (console->window_origin_y + CHAR_H)*console->display.rowBytes +
		(console->window_origin_x >> 3);
	dst = src - CHAR_H*console->display.rowBytes;
	for (source_y = CHAR_H;		// top, row 1
	     source_y < console->pixels_per_column;	// bottom
	     source_y++) {

	    bytemove(dst, src, console->pixels_per_row>>3);
	    src += console->display.rowBytes;
	    dst += console->display.rowBytes;
	}
	IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x0); // restore write mode 

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
    rect(console,
	console->window_origin_x,
	console->window_origin_y,
	CHAR_W * console->chars_per_row,
	CHAR_H * 2 - BG_MARGIN,
	console->foreground);
    saved_origin_y = console->window_origin_y;
    console->window_origin_y += CHAR_H / 2;
    console->curr_column = (console->chars_per_row - title_len) / 2;
    FlipCursor(console);				// new cursor on	
    SwapForegroundAndBackground(console);
    while (*title) {
	FBPutC(console, *title++);
    }
    SwapForegroundAndBackground(console);
    FlipCursor(console);			// new cursor off	
    console->window_origin_y = saved_origin_y;
    
    // Draw grey lines one pixel away from border of title bar.
    // Origin is currently at first pixel in top line of title bar.

    rect(console,				// inside top
	console->window_origin_x - BG_MARGIN,
	console->window_origin_y - BG_MARGIN,
	console->pixels_per_row + (BG_MARGIN * 2),
	BG_MARGIN,
	console->light_grey);
    rect(console,				// inside bottom
	console->window_origin_x - BG_MARGIN,
	console->window_origin_y + CHAR_H * 2 - TOTAL_MARGIN - BG_MARGIN,
	console->pixels_per_row + BG_MARGIN * 2,
	BG_MARGIN,
	console->dark_grey);
    for (pixel_num = 0; pixel_num < BG_MARGIN; pixel_num++) {
        rect(console,				// left
	    console->window_origin_x - BG_MARGIN + pixel_num,
	    console->window_origin_y - BG_MARGIN + pixel_num,
	    1,
	    CHAR_H * 2 - 1 - (pixel_num * 2),
	    console->light_grey);
    }
    for (pixel_num = 1; pixel_num <= BG_MARGIN; pixel_num++) {
        rect(console,				// right
	    console->window_origin_x + console->pixels_per_row + pixel_num - 1,
	    console->window_origin_y - pixel_num,
	    1,
	    CHAR_H * 2 - 1 - (BG_MARGIN - pixel_num) * 2,
	    console->dark_grey);
    }
    rect(console,				// outside bottom
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
    if (width > (console->display.width - TOTAL_MARGIN * 2))
	width = console->display.width - TOTAL_MARGIN * 2;
    if (height > (console->display.height - TOTAL_MARGIN * 2))
	height = console->display.height - TOTAL_MARGIN * 2;
	    
    //
    // Init console window variables. The window is centered in the 
    // screen, minus a pixel or two to guarantee double word alignment
    // for the first pixel of a row (until further developments...when 
    // we do alert panels, we can enumerate some cases here...).
    //
    console->window_origin_x   = (console->display.width - width) / 2;
    console->window_origin_y   = (console->display.height - height) / 2;
    console->pixels_per_row    = width;
    console->pixels_per_column = height;
    (unsigned)console->window_origin_x &= ~0x07;

    UpdateWindowChars(console);
    
    console->curr_column = 0;
    console->has_title = FALSE;
    
    if (initWindow) {
	// Before we start drawing anything, save under if needed
	if (saveUnder) {
	    int i;
	    unsigned char *save, *sp;
	    /*
	     * in saveUnder mode, we take care to preserve the VGA
	     * state we'll mess with during console operations.  This
	     * consists of the 9 graphics controller registers
	     * and 5 sequencer registers in the VGA.
	     * We already did this in Init().
	     */
		    
	    /*
	     * load a default set of registers for mode 12
	     */	    
	    for(i = 0; i < VGA_NUM_GC_REGS; i++)
		IOWriteRegister(VGA_GC_ADDR,i, defaultGCRegisters[i]);

	    if (console->isSVGA) {
		char currentSegment = 0;

		SaveVGARegs(&console->tempRegisters);
		/*
		 * for SVGA, we use read mode 0 and copy the data to 
		 * a global array, not off-screen display memory as with VGA
		 * mode 0x12.
		 */ 
		console->saveHeight = height + TOTAL_MARGIN * 2;
		console->saveRowBytes = width/8 + ((TOTAL_MARGIN + 7) / 8) * 2;
		console->saveBytes = console->saveHeight*console->saveRowBytes;
		/* saveBits was allocated when we allocated the console */
		save = console->saveBits;
			    
		// Read mode 0
		IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x8, 0x0);
    
		// Copy the window contents into the save under.
    
		// Plane 0.
		IOReadModifyWriteRegister(VGA_GC_ADDR, 4, ~0x3, 0x0);
		
		sp = console->saveLocation =
		    (unsigned char *)console->display.frameBuffer +
			    (console->window_origin_y - TOTAL_MARGIN) *
				console->display.rowBytes + 
				    (console->window_origin_x>>3) - 1;
		SVGASelectSegment(currentSegment);
		for (i = console->saveHeight; i != 0; i--) {
		    SVGASelectSegmentForAddressIfNecessary(sp,
			   &currentSegment);
		    bytemove(save, sp, console->saveRowBytes);
		    sp += console->display.rowBytes;
		    save += console->saveRowBytes;
		}
    
		// Plane 1.
		IOReadModifyWriteRegister(VGA_GC_ADDR, 4, ~0x3, 0x1);
		sp = console->saveLocation =
		    (unsigned char *)console->display.frameBuffer +
			    (console->window_origin_y - TOTAL_MARGIN) *
				console->display.rowBytes + 
				    (console->window_origin_x>>3) - 1;
		for (i = console->saveHeight; i != 0; i--) {
		    SVGASelectSegmentForAddressIfNecessary(sp,
			   &currentSegment);
		    bytemove(save, sp, console->saveRowBytes);
		    sp += console->display.rowBytes;
		    save += console->saveRowBytes;
		}
		SVGASelectSegment(currentSegment = 0);
		// restore wr. mode
		IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x0);

		RestoreVGARegs(&console->tempRegisters);
	    } else {  
		/*
		 * for vga, we use write mode 1 and copy the data to 
		 * off-screen memory.  We save and extra byte on either side
		 * of the popup.
		 */ 
		console->saveHeight = height + TOTAL_MARGIN * 2;
		console->saveRowBytes = width/8 + ((TOTAL_MARGIN + 7) / 8) * 2;
		console->saveBytes = console->saveHeight*console->saveRowBytes;
		save = console->saveBits;
	    
		sp = console->saveLocation =
		    (unsigned char *)console->display.frameBuffer +
			    (console->window_origin_y - TOTAL_MARGIN) *
				console->display.rowBytes + 
				    (console->window_origin_x>>3) - 1;
    
		// Copy the window contents into the save under 
		IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x1);		/* write mode 1 */
		for (i = console->saveHeight; i != 0; i--) {
		    bytemove(save, sp, console->saveRowBytes);
		    sp += console->display.rowBytes;
		    save += console->saveRowBytes;
		}
		IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x0);		/* restore wr. mode*/
	    }
	}
    
	// New window - cursor at origin.
	console->curr_row = 0;
	console->has_title = FALSE;
	
	// Draw the border. The border goes outside of the specified
	// window. Outside lines are black; inside lines are 
	// half-tone.

	rect(console,				// outside top
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    width + TOTAL_MARGIN * 2,
	    FG_MARGIN,
	    console->foreground);
	rect(console,				// inside top
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y - BG_MARGIN,
	    width + BG_MARGIN * 2,
	    BG_MARGIN,
	    console->background);
	rect(console,				// inside bottom
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y + height,
	    width + BG_MARGIN * 2,
	    BG_MARGIN,
	    console->background);
	rect(console,				// outside bottom
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y + height + BG_MARGIN,
	    width + TOTAL_MARGIN * 2,
	    FG_MARGIN,
	    console->foreground);
	rect(console,				// outside left
	    console->window_origin_x - TOTAL_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    FG_MARGIN,
	    height + TOTAL_MARGIN * 2,
	    console->foreground);
	rect(console,				// inside left
	    console->window_origin_x - BG_MARGIN,
	    console->window_origin_y - BG_MARGIN,
	    BG_MARGIN,
	    height + BG_MARGIN * 2,
	    console->background);
	rect(console,				// inside right
	    console->window_origin_x + width,
	    console->window_origin_y - BG_MARGIN,
	    BG_MARGIN,
	    height + BG_MARGIN * 2,
	    console->background);
	rect(console,				// outside right
	    console->window_origin_x + width + BG_MARGIN,
	    console->window_origin_y - TOTAL_MARGIN,
	    FG_MARGIN,
	    height + TOTAL_MARGIN * 2,
	    console->foreground);
		
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
    ConsolePtr console = (ConsolePtr)cso->priv;
    if (console->isSVGA)
	IOFree(console->saveBits, SVGA_SAVE_BITS);
    IOFree(console, sizeof(ConsoleRep));
    IOFree(cso, sizeof(IOConsoleInfo));
}

static void Init(
    IOConsoleInfo *cso,
    ScreenMode mode,
    boolean_t initScreen,
    boolean_t initWindow,
    const char *title)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    int i;
    
    
    console->background = VGA_WHITE;		
    console->light_grey = VGA_LTGRAY;
    console->dark_grey  = VGA_DKGRAY;
    console->foreground = VGA_BLACK;
    console->baseground = console->dark_grey;

    console->ansi_state = AS_NORMAL;
    for(i=0; i<ANSI_STACK_SIZE; i++) {
	console->ansi_stack[i] = 0;
    }
    console->ansi_stack_p = &console->ansi_stack[1];	// FIXME - why not 0?

    // Save these for later in case we need them for Restore().
    SaveVGARegs(&console->savedRegisters);

    // In case the display is not in the mode we want...
    if ((mode == SCM_TEXT || mode == SCM_GRAPHIC) && initScreen)
	VGASetGraphicsMode();
    
    // Set up VGA registers the way we want them.
    // Enable all planes in mask register.
    IOWriteRegister(VGA_SEQ_ADDR, 2, PLANE_MASK);
    for(i = 0; i < VGA_NUM_GC_REGS; i++)
	IOWriteRegister(VGA_GC_ADDR,i, defaultGCRegisters[i]);

    console->window_type = mode;

    // Initialize the screen, if appropriate.
    if (initScreen && (console->window_type != SCM_ALERT)) {
        WipeScreen(console, console->baseground);
    }

    switch(console->window_type) {
	case SCM_TEXT:
	    InitWindow(console,	TEXT_WIN_WIDTH, TEXT_WIN_HEIGHT,
		title, initWindow, 0 /* Don't save under */);
	    break;
	case SCM_ALERT:
	    InitWindow(console,	ALERT_WIN_WIDTH, ALERT_WIN_HEIGHT,
		title, initWindow, 1 /* Save under */);
	    break;
	case SCM_GRAPHIC:
	    /* do nothing */
	    break;
	default:
	    panic("VGAConsole/VGAInitConsole: can't init");
    }
    return;
}

static int Restore(IOConsoleInfo *cso)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    unsigned char *save, *dp;
    int i;

    if (console->window_type != SCM_ALERT) {
	IOLog("VGAConsole: bogus restore, mode %d\n", console->window_type);
	return -1;
    }

    if (console->isSVGA) {
	char currentSegment;
	
        // Copy save under bits back onto screen
	save = console->saveBits;
    
	IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x0); /* write mode 0 */
	IOWriteRegister(VGA_GC_ADDR, 1, 0x0); /* disable set/reset write */
	IOWriteRegister(VGA_GC_ADDR, 8, 0xff); /* set pixel mask */
    
	// Plane 0.
	IOWriteRegister(VGA_SEQ_ADDR, 2, 0x1); /* map mask for plane 0 */
	dp = console->saveLocation;
	SVGASelectSegment(currentSegment = 0);
	for (i = console->saveHeight; i != 0; i--) {
	    SVGASelectSegmentForAddressIfNecessary(dp, &currentSegment);
	    bytemove(dp, save, console->saveRowBytes);
	    dp += console->display.rowBytes;
	    save += console->saveRowBytes;
	}
    
	// Plane 1.
	IOWriteRegister(VGA_SEQ_ADDR, 2, 0x2); /* map mask for plane 1 */
	dp = console->saveLocation;
	for (i = console->saveHeight; i != 0; i--) {
	    SVGASelectSegmentForAddressIfNecessary(dp, &currentSegment);
	    bytemove(dp, save, console->saveRowBytes);
	    dp += console->display.rowBytes;
	    save += console->saveRowBytes;
	}
	SVGASelectSegment(currentSegment = 0);

    } else {
	// Copy save under bits back onto screen
	save = console->saveBits;
	dp = console->saveLocation;
	IOReadModifyWriteRegister(VGA_GC_ADDR, 5, ~0x3, 0x1);		/* write mode 1 */
	for (i = console->saveHeight; i != 0; i--) {
	    bytemove(dp, save, console->saveRowBytes);
	    dp += console->display.rowBytes;
	    save += console->saveRowBytes;
	}
    }
    
    RestoreVGARegs(&console->savedRegisters);
	    
    console->window_type = SCM_UNINIT;
    
    return 0;
}


static void
vga_write_bpp2packd32_to_bpp4planar(unsigned int *dst, unsigned short *fb)
{
    unsigned int i, dstvalue;
    unsigned short fbvalue;
    unsigned char fblo, fbhi;

    IOWriteRegister(VGA_SEQ_ADDR, 2, 0x02);	// select plane 1

    dstvalue = *dst;
    i = dstvalue & 0xAAAA;

    fblo = 0xff  ^
	(char) (((i & 0x8000) >> 15) | ((i & 0x2000) >> 12) |
		((i & 0x0800) >> 9) | ((i & 0x0200) >> 6) |
		((i & 0x0080) >> 3) | ((i & 0x0020)      ) |
		((i & 0x0008) << 3) | ((i & 0x0002) << 6));
    i = (dstvalue >> 16) & 0xAAAA;
    fbhi = 0xff  ^
	(char) (((i & 0x8000) >> 15) | ((i & 0x2000) >> 12) |
		((i & 0x0800) >> 9) | ((i & 0x0200) >> 6) |
		((i & 0x0080) >> 3) | ((i & 0x0020)      ) |
		((i & 0x0008) << 3) | ((i & 0x0002) << 6));
    fbvalue = (((unsigned short)fbhi) << 8) | fblo;
    *fb = fbvalue;

    IOWriteRegister(VGA_SEQ_ADDR, 2, 0x01);	// select plane 0

    dstvalue = *dst;
    i = dstvalue & 0x5555;
    fblo = 0xff  ^ 
	(char) (((i & 0x4000) >> 14) | ((i & 0x1000) >> 11) |
		((i & 0x0400) >> 8) | ((i & 0x0100) >> 5) |
		((i & 0x0040) >> 2) | ((i & 0x0010) << 1) |
		((i & 0x0004) << 4) | ((i & 0x0001) << 7));
    i = (dstvalue >> 16) & 0x5555;
    fbhi = 0xff  ^ 
	(char) (((i & 0x4000) >> 14) | ((i & 0x1000) >> 11) |
		((i & 0x0400) >> 8) | ((i & 0x0100) >> 5) |
		((i & 0x0040) >> 2) | ((i & 0x0010) << 1) |
		((i & 0x0004) << 4) | ((i & 0x0001) << 7));
    fbvalue = (((unsigned short)fbhi) << 8) | fblo;
    *fb = fbvalue;
    
    // restore default plane mask
    IOWriteRegister(VGA_SEQ_ADDR, 2, PLANE_MASK);
}


// VGABlitRect blits a rect onto the VGA mode 0x12 screen.
// No error checking is done.  The screen must be in the proper video
// mode, the data must be in 2 bit format, on a 4 pixel boundary, and
// a multiple of 4 pixels wide.  Anything other than 16 pixel boundaries
// is padded with light gray.  -sam
static int VGABlitRect(
    IOConsoleInfo *cso,
    unsigned char *data,
    struct km_drawrect *rect
)
{
	ConsolePtr console = (ConsolePtr)cso->priv;
	int pixel;		/* Starting pixel in screen */
	int i, x, y;
	unsigned char value;
	unsigned char *dst = (unsigned char *)console->display.frameBuffer;
	unsigned short *tfb;
	unsigned int final_value;
	int extraLeftBytes;

	pixel = rect->x + (rect->y * console->display.width);

	extraLeftBytes = (rect->x % 16) / 4;
	pixel -= extraLeftBytes * 4;

	dst += pixel / 8;

	for ( y = 0; y < rect->height; ++y )
	{
		int bitShift = 30;
		final_value = 0;

		tfb = (unsigned short *) dst;
		for ( x = 0; x < rect->width; )
		{
			if (x == 0)
			{
				for (i=0; i<extraLeftBytes; i++)
				{
					final_value = final_value << 8;
					final_value |= 0x55;
					bitShift -= 8;
				}
			}

			value = *data++;
			
			for ( i = 0; i < 8; i += 2 )
			{
				final_value |= (((value>>(6-i)) & 3) << (30-bitShift));
				bitShift -= 2;
			}

			x += 4;

			if (bitShift < 0 || x  >= rect->width)
			{
				for (; bitShift >= 0; bitShift -= 2)
					final_value |= (0x01 << (30-bitShift));
				vga_write_bpp2packd32_to_bpp4planar(&final_value, tfb++);
				bitShift = 30;
				final_value = 0;
			}

		}
		dst += 80;
	}

	return(0);
}

/*
 * DrawRect assumes that the caller has copied in any data from user space,
 * so this function is safe to call directly during kernel shutdown.
 */

static int DrawRect(IOConsoleInfo *cso,  const struct km_drawrect *km_rect)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    long size;		// Size in bytes corresponding to a given 2bpp area
    struct km_drawrect *rect = (struct km_drawrect *) km_rect;
    int ret = 0;

    if (console->window_type != SCM_GRAPHIC) {
	return( -1 );
    }

    /* Sanity check */
    if ((rect->width > console->display.width) ||
        (rect->height) > console->display.height) {
	    return( -1 );
    }

    rect->x &= ~3;				// Trunc to 4 pixel bound
    rect->width = (rect->width) & ~3;		// Round down for safety

    size = (rect->width >> 2) * rect->height;	/* size in bytes. */
    if ( size <= 0 )
	return( -1 );

    IOWriteRegister(VGA_GC_ADDR, 1, 0x00);
    VGABlitRect(cso, rect->data.bits, rect);
    IOWriteRegister(VGA_GC_ADDR, 1, PLANE_MASK);

    return(ret);
}

static int EraseRect(IOConsoleInfo *cso,  const struct km_drawrect *km_rect)
{
    ConsolePtr console = (ConsolePtr)cso->priv;
    if (km_rect->x + km_rect->width > console->display.width ||
	km_rect->y + km_rect->height > console->display.height)
	    return -1;
    rect(console, km_rect->x, km_rect->y,
	   km_rect->width, km_rect->height, 3 - (km_rect->data.fill & 3));
    return 0;
}

static void PutC(IOConsoleInfo *cso, char c)
// Write one character to screen. Cursor is 'on' on entry and exit.
{
    FBPutC((ConsolePtr)cso->priv, c);
}

static void GetSize(IOConsoleInfo *cso, struct ConsoleSize *s)
// Return the screen size in pixels.
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


//
// BEGIN:	Exported routines
//

IOConsoleInfo *_VGAAllocateConsole(IODisplayInfo *display, boolean_t isSVGA)
{
    IOConsoleInfo *cso = (IOConsoleInfo *)IOMalloc(sizeof(IOConsoleInfo));
    ConsolePtr console = (ConsolePtr)IOMalloc(sizeof(ConsoleRep));
    
    if (cso == NIL)
        return NIL;
    cso->priv = console;
    if (cso->priv == NIL) {
	IOFree(cso, sizeof(IOConsoleInfo));
	return NIL;
    }
    cso->Free = Free;
    cso->Init = Init;
    cso->Restore = Restore;
    cso->DrawRect = DrawRect;
    cso->EraseRect = EraseRect;
    cso->PutC = PutC;
    cso->GetSize = GetSize;
    console->display = *display;
    console->isSVGA = isSVGA;
    
    // Fill in the rest of the display structure...
    if (isSVGA) {
	// Until the implementation of bank switching support is complete,
	// use only top LINESPERBANK lines and thus avoid bank switching.
	console->display.height = SVGA_LINESPERBANK;
	console->display.frameBuffer = (void *)FRAMEBUFFER_ADDRESS;
	console->display.totalWidth = SVGA_TOTALWIDTH;
	console->display.rowBytes = SVGA_ROWBYTES;
	console->saveBits = (unsigned char *)IOMalloc(SVGA_SAVE_BITS);
	if (console->saveBits == NIL) {
	    IOFree(cso, sizeof(IOConsoleInfo));
	    IOFree(console, sizeof(ConsoleRep));
	    return NIL;
	}
    } else {
	console->display.frameBuffer = (void *)FRAMEBUFFER_ADDRESS;
	console->display.totalWidth = VGA_TOTALWIDTH;
	console->display.rowBytes = VGA_ROWBYTES;
	console->saveBits =
		(unsigned char *)console->display.frameBuffer +
		console->display.rowBytes * console->display.height;
    }

    console->window_type = SCM_UNINIT;
    return cso;
}

IOConsoleInfo *SVGAAllocateConsole(IODisplayInfo *display)
{
    return _VGAAllocateConsole(display, TRUE);
}
IOConsoleInfo *VGAAllocateConsole(IODisplayInfo *display)
{
    return _VGAAllocateConsole(display, FALSE);
}

//
// END:		Exported routines
//
