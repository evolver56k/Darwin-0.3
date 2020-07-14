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
 * ConsoleSupport.h - Definition of the ConsoleSupport system.
 *
 *
 * HISTORY
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 */

// Notes:
// * Every display object needs to be able to support console output. Also,
//   there is always VGA style console output available. Each display class
//   will need to implement the functions listed in the object defined below.
// * The km driver will get one of these objects ffrom the display be calling
//   a method in the display. The km driver can get a console object for the
//   VGA by calling VGAAllocateConsole().
//

#ifdef	DRIVER_PRIVATE

#import <bsd/dev/kmreg_com.h>

// Screen mode and window type.
typedef enum {
    SCM_UNINIT,			// uninitialized
    SCM_TEXT,			// verbose boot mode or runnning
				//   with no window server
    SCM_GRAPHIC,		// boot w/icons
    SCM_ALERT,			// temporary alert mode
    SCM_OTHER,			// screen owned by someone else
} ScreenMode;

/*
 * Arguments to kmDrawGraphicPanel().
 */
typedef enum {
    KM_PANEL_ERASE,		/* erase all panels */
    KM_PANEL_NS,		/* NEXTSTEP panel */
    KM_PANEL_ALERT		/* alert panel */
} GraphicPanelType;

struct ConsoleSize {
    unsigned short	rows;
    unsigned short	cols;
    unsigned short	pixel_width;
    unsigned short	pixel_height;
};

typedef struct _t_IOConsoleInfo {
    void (*Free)(struct _t_IOConsoleInfo *console);
    void (*Init)(
	struct _t_IOConsoleInfo *console,
	ScreenMode mode,
	boolean_t initScreen,
	boolean_t initWindow,
	const char *title);
    int (*Restore)(struct _t_IOConsoleInfo * console);
    int (*DrawRect)(
        struct _t_IOConsoleInfo * console, const struct km_drawrect *r);
    int (*EraseRect)(
        struct _t_IOConsoleInfo * console, const struct km_drawrect *r);
    void (*PutC)(struct _t_IOConsoleInfo *console, char c);
    void (*GetSize)(
	struct _t_IOConsoleInfo * console,
	struct ConsoleSize * size);
    void *priv;
} IOConsoleInfo;

#endif	/* DRIVER_PRIVATE */
