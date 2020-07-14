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
 * Copyright 1993 NeXT, Inc.
 * All rights reserved.
 */

#import "libsaio.h"
#import "io_inline.h"

#import "boot.h"
#import "console.h"
#import "memory.h"
#import "bitmap.h"
#import "font.h"
#import "fontio.h"
#import "graphics.h"
#import "language.h"
#import "bitmap_list.h"
#import "spin_cursor.h"
#import "vbe.h"
#import "kernBootStruct.h"

struct bitmap_list bitmapList[] = {
    {"Panel.image",0},
    {0,0}
};

const struct bitmap *panel;

static BOOL loadAllBitmaps( void );
static font_t *loadFont(char *fontname);
int convert_vbe_mode(char *mode_name, int *mode);

void
message(
    char *str,
    int centered
)
{
	register int x;
	//register int y;
	BOOL tshow = showText;
	char *val = 0;
	
	if (LanguageConfig) {
	    if (val = newStringForStringTableKey(LanguageConfig, str)) {
		str = val;
	    }
	}
	x = (NCOLS - strlen(str)) >> 1;
 	if (kernBootStruct->graphicsMode == GRAPHICS_MODE) {
	    strwidth ("9");  // stupid!! must have this or it won't link
	    /*
	     *  Do nothing for now since it messes up the image
	     *
	    y = MESSAGE_Y;
	    blit_clear(BOX_W - 48, BOX_C_X, y, CENTER_V | CENTER_H, TEXT_BG);
	    blit_string(str, BOX_C_X, y, TEXT_FG, CENTER_V | CENTER_H);
	     */
	} else {
	    showText = 1;
	    if (centered)
		while(x--)
		    printf(" ");
	    printf("%s\n",str);
	    showText = tshow;
	}
	if (val)
	    free(val);
}



#if NOTYET
void
popupBox( int x, int y, int width, int height, int bgcolor, int inout)
{
    int topcolor, bot1color, bot2color;
    
    if (inout == POPUP_IN) {
	topcolor = COLOR_DK_GREY;
	bot1color = COLOR_LT_GREY;
	bot2color = COLOR_WHITE;
    } else {
	topcolor = COLOR_WHITE;
	bot1color = COLOR_DK_GREY;
	bot2color = COLOR_BLACK;
    }
    clearRect(x, y, width, height, bgcolor);
    
    clearRect(x, y, width, 2,      topcolor);
    clearRect(x ,y, 2,     height, topcolor);
    
    clearRect(x+width-1, y,   1, height, bot2color);
    clearRect(x+width-2, y+1, 1, height-2, bot1color);
    
    clearRect(x,   y+height-1, width,   1, bot2color);
    clearRect(x+1, y+height-2, width-2, 1, bot1color);
}

void clearBox(
    int x,
    int y,
    int w,
    int h
)
{
    if (x < BOX_X)
	clearRect(x, y, BOX_X - x, h, SCREEN_BG);
    if (x+w > BOX_X + BOX_W)
	clearRect(BOX_X + BOX_W, y, (x + w) - (BOX_X + BOX_W), h, SCREEN_BG);
    if (y < BOX_Y)
	clearRect(x, y, x+w, (BOX_Y - y), SCREEN_BG);
    if (y+h > BOX_Y + BOX_H)
	clearRect(x, BOX_Y + BOX_H, w, (y + h) - (BOX_Y + BOX_H), SCREEN_BG);
    copyImage(bitmapList[PANEL_BITMAP].bitmap, BOX_X, BOX_Y);
}
#endif NOTYET


// for spinning disk
static int currentIndicator = 0;

/*
 * Load all files needed to go into selected mode.
 */
BOOL
initMode(int mode)
{
    static BOOL fontsLoaded;
    static BOOL bitmapsLoaded;
    
    if (mode != GRAPHICS_MODE)
	return YES;
	
    if (!bitmapsLoaded) {
	if (loadAllBitmaps() == 0) {
	    /* If bitmaps couldn't be loaded, stay in text mode. */
	    return NO;
	}
	bitmapsLoaded = YES;
    }
    if (!fontsLoaded) {
	fontp = loadFont("Default.font");
	/* If font couldn't be loaded, stay in text mode. */
	if (fontp == NULL)
	    return NO;
	fontsLoaded = YES;
    }
    return YES;
}

void
setMode(int mode)
{
	unsigned short vmode;
	char *vmode_name;

	if (currentMode() == mode)
	    return;

	if (initMode(mode) == NO) {
	    /* Couldn't go into requested mode. */
	    return;
	}

	if (mode == GRAPHICS_MODE &&
		(vmode_name = newStringForKey(G_MODE_KEY)) != 0)
	{
	    textBuf = malloc(TEXTBUFSIZE);
	    bufIndex = showText = 0;

	    if (!convert_vbe_mode(vmode_name, &vmode))
		vmode = mode1024x768x256;   /* default mode */

	    set_linear_video_mode(vmode);

	    clearRect(0, 0, SCREEN_W, SCREEN_H, SCREEN_BG);
	    copyImage(bitmapList[PANEL_BITMAP].bitmap, BOX_X, BOX_Y);

	    kernBootStruct->graphicsMode = GRAPHICS_MODE;
	    kernBootStruct->video.v_baseAddr    = (unsigned long)frame_buffer;
	    kernBootStruct->video.v_width       = SCREEN_W;
	    kernBootStruct->video.v_height      = SCREEN_H;
	    kernBootStruct->video.v_depth       = bits_per_pixel;
	    kernBootStruct->video.v_rowBytes    = (SCREEN_W * bits_per_pixel) >> BYTE_SHIFT;
	} else {
	    showText = 1;
	    set_video_mode(2);
	    if (textBuf) {
		int i;
		for (i = 0; bufIndex; bufIndex--)
			putchar(textBuf[i++]);
		free(textBuf);
		textBuf = (char *)0;
	    }
	}	
	currentIndicator = 0;
}

int currentMode(void)
{
    return kernBootStruct->graphicsMode;
}

typedef struct {
    char mode_name[15];
    int  mode_val;
} mode_table_t;

mode_table_t mode_table[] = {           
{ "640x400x256",   mode640x400x256 },
{ "640x480x256",   mode640x480x256 },
{ "800x600x16",    mode800x600x16 },
{ "800x600x256",   mode800x600x256 },
{ "1024x768x16",   mode1024x768x16 },
{ "1024x768x256",  mode1024x768x256 },
{ "1280x1024x16",  mode1280x1024x16 },
{ "1280x1024x256", mode1280x1024x256 },
{ "640x480x555",   mode640x480x555 },
{ "640x480x888",   mode640x480x888 },
{ "800x600x555",   mode800x600x555 },
{ "800x600x888",   mode800x600x888 },
{ "1024x768x555",  mode1024x768x555 },
{ "1024x768x888",  mode1024x768x888 },
{ "1280x1024x555", mode1280x1024x555 },
{ "1280x1024x888", mode1280x1024x888 },
{ "", 0 }};

int convert_vbe_mode(char *mode_name, int *mode)
{
    mode_table_t *mtp = mode_table;

    if (mode_name == 0 || *mode_name == 0)
	return 0;

    while (*mtp->mode_name)
    {
	if (strcmp(mtp->mode_name, mode_name) == 0)
	{
	    *mode =  mtp->mode_val;
	    return 1;
	}
	mtp++;
    }
    return 0;

}


static char indicator[] = {'-', '\\', '|', '/', '-', '\\', '|', '/', '\0'};
static const struct bitmap *indicator_bitmap[4] = {
    &wait1_bitmap,
    &wait2_bitmap,
    &wait3_bitmap,
    0
};

// To prevent a ridiculously fast-spinning indicator,
// ensure a minimum of 1/9 sec between animation frames.
#define MIN_TICKS 2

void
spinActivityIndicator( void )
{
    static unsigned long lastTickTime = 0;
    unsigned long currentTickTime = time18();
    static char string[3] = {'\0', '\b', '\0'};
    
    if (currentTickTime < lastTickTime + MIN_TICKS)
	return;
    else
	lastTickTime = currentTickTime;
	
    if (kernBootStruct->graphicsMode == GRAPHICS_MODE) {
	copyImage(indicator_bitmap[currentIndicator], CURSOR_X, CURSOR_Y);
	if (indicator_bitmap[++currentIndicator] == 0)
	    currentIndicator = 0;
    } else {
	string[0] = indicator[currentIndicator];
	reallyPrint(string);
	if (indicator[++currentIndicator] == 0)
	    currentIndicator = 0;
    }
}

void
clearActivityIndicator( void )
{
    if (showText) {
	reallyPrint(" \b");
    } else {
	/*
	 * Turn this off since it messes up the panel image. (the
	 *  panel image is not necessarily TEXT_BG)
	 */
	//clearRect(CURSOR_X, CURSOR_Y, CURSOR_W, CURSOR_H, TEXT_BG);
    }
}

BOOL
loadAllBitmaps( void )
{
    static BOOL bitmapsLoaded;
    struct bitmap_list *bp;
    char buf[128];
    
    if (bitmapsLoaded)
	return YES;
    for (bp = bitmapList; bp->name; bp++) {
	if (bp->bitmap)
	    continue;
	sprintf(buf, "/usr/standalone/i386/%s", bp->name);
	if ((bp->bitmap = (struct bitmap *)loadBitmap(buf)) == 0) {
	    printf("Could not load all bitmaps; using text mode.\n");
	    return NO;
	}
    }
    /* Indicator bitmaps are statically declared above. */
    panel = bitmapList[PANEL_BITMAP].bitmap;

    bitmapsLoaded = YES;
    return YES;
}

font_t *loadFont(char *fontname)
{
    char buf[128];
    int cc, fd, numbytes;
    extern char *Language;
    font_t *font;
        
    sprintf(buf, "/usr/standalone/i386/%s/%s", Language, fontname);
    fd = open(buf, 0);
    if (fd < 0) {
	sprintf(buf, "/usr/standalone/i386/%s/%s", "English.lproj", fontname);
	fd = open(buf,0);
	if (fd < 0) {
	    error("Couldn't open font file %s\n",buf);
	    return 0;
	}
    }
    numbytes = file_size(fd);
    font = (font_t *)malloc(numbytes);
    cc = read(fd, (char *)font, numbytes);
    close(fd);
    if (cc < numbytes) {
	error("Short read on font file %s\n",buf);
	free(font);
	return 0;
    }
    return font;
}
