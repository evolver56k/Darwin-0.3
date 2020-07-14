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
#import "kmFontPriv.h"
#import "ConsoleSupport.h"
#import "kmDevice.h"
#import "TimesItalic14.h"

#define TEXT_COLOR		KM_COLOR_BLACK

// in psuedoVGA 640 x 480 coords
#define XMARGIN			0

#define PANELWIDTH	352
#define PANELHEIGHT	264
#define TEXTYPOS	(156 + 21)

#define PANELTOP	((480 - PANELHEIGHT) / 2)
#define PANELLEFT	((640 - PANELWIDTH) / 2)

#define TEXTBASELINE		(PANELTOP + TEXTYPOS)
// rect to erase
#define TEXTTOP			(PANELTOP + TEXTYPOS - 20)
#define TEXTLEFT		(PANELLEFT + 16)
#define TEXTWIDTH		(PANELWIDTH - 32)
#define TEXTHEIGHT		(PANELHEIGHT - 16 - (TEXTYPOS - 20))


void
kmDrawString( IOConsoleInfo *console, char * str )
{
	int lines;
	char lastchar;
	bitmap_t *bmp;
	int deflead;
	const unsigned char *string;
        font_c_t *fontp = &Times_Italic_14;
        int xpos, ypos;

	str = kmLocalizeString(str);
	
	for(lines = 0, string = str, lastchar = '\0'; *string; string++) {
	    lastchar = *string;
	    if (lastchar == '\n')
		lines++;
	}
	if (lastchar != '\n')
	    lines++;
	
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;

        ypos = TEXTBASELINE; // - ((deflead * (lines - 1)) / 2);

	for (string = str; *string; ) {
	    const char *line;
	    int len;
	    
	    /* Center this line */
	    for (line = string, len = 0; *line && (*line != '\n'); line++) {
                bmp = &fontp->bitmaps[(*line) - ENCODEBASE];
		len += bmp->dwidth;
	    }
            xpos = XMARGIN - len / 2;

	    for (; *string; string++) {
		if (*string == '\n') {
		    string++;
		    break;
		}
                bmp = &fontp->bitmaps[(*string) - ENCODEBASE];
                BltPSBitmap( console, xpos, ypos, bmp, fontp->bits, TEXT_COLOR);
                xpos += bmp->dwidth;
	    }
	    ypos += deflead;
	}
}

@implementation kmDevice (KmGraphics)

- (void)graphicPanelString:(char *)str
{
    struct km_drawrect rect;

    if (fbMode != SCM_GRAPHIC)
        return;

    rect.width = TEXTWIDTH;
    rect.height = TEXTHEIGHT;
    rect.x = TEXTLEFT;
    rect.y = TEXTTOP;
    rect.data.fill = (void *) KM_COLOR_LTGRAY;
    (void)(*fbp[FB_DEV_NORMAL]->EraseRect)(fbp[FB_DEV_NORMAL], &rect);

    kmDrawString( fbp[FB_DEV_NORMAL], str);
}

- (void)drawGraphicPanel:(GraphicPanelType) panelType
{
}

@end
