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

#import "kernBootStruct.h"

/* graphics definitions */

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define BOX_W		(panel->width)
#define BOX_H		(panel->height)
//#define BOX_W_OFFSET	(panel->width % 16)
#define BOX_W_OFFSET 0
#define BOX_X		((SCREEN_W - BOX_W) / 2)
#define BOX_Y		((SCREEN_H - BOX_H) / 2)
#define BOX_C_X		((SCREEN_W / 2) + 4)

#define POPUP_FRAME_MARGIN 3
#define POPUP_W		(BOX_W - 32)
//#define POPUP_H		(BOX_H / 2)
//#define POPUP_X		(BOX_X + (BOX_W - POPUP_W)/2)
#define POPUP_Y		(BOX_Y + 14)
#define POPUP_T_XMARGIN	4
#define POPUP_T_YMARGIN 4
#define POPUP_XMARGIN (2 * POPUP_T_XMARGIN)	// must be mulitple of 8
#define POPUP_YMARGIN (2 * POPUP_T_YMARGIN)
//#define POPUP_T_W	(fontp->bbx.width - fontp->bbx.xoff + \
//			    2 * POPUP_T_XMARGIN)
//#define POPUP_T_H	(fontp->bbx.height + 2 * POPUP_T_YMARGIN)
//#define POPUP_T_X	(POPUP_X + (POPUP_W - POPUP_T_W) / 2)
//#define POPUP_T_Y	(POPUP_Y + POPUP_H - POPUP_T_H - 8)

#define CURSOR_W	16
#define CURSOR_H	16
#define CURSOR_X	(BOX_X + (BOX_W - CURSOR_W) / 2)
#define CURSOR_Y	(BOX_Y + (148))

#define MESSAGE_Y	(BOX_Y + (182))

/* We must scramble the palette in order
 * to be able to write text with the correct
 * background and foreground colors.
 */
 
#define COLOR_BLACK	0x00
#define COLOR_DK_GREY	0xFE
#define COLOR_LT_GREY	0xFA
#define COLOR_WHITE	0xFF
#define COLOR_PLATNUM	0x80

#define TEXT_BG		COLOR_LT_GREY
#define TEXT_FG		COLOR_DK_GREY
#define SCREEN_BG	COLOR_PLATNUM

#define POPUP_IN	0
#define POPUP_OUT	1

typedef struct rect {
    short x, y;
    short w, h;
} rect_t;

extern void popupBox(
    int x, int y, int width, int height, int bgcolor, int inout
);
extern void message(char *str, int centered);
extern BOOL initMode(int mode);
extern void setMode(int mode);
extern int currentMode(void);
extern void spinActivityIndicator( void );
extern void clearActivityIndicator( void );





