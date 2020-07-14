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
 * km.h - kernel keyboard/monitor module, public procedural interface.
 *
 * HISTORY
 * 20-Jan-92    Doug Mitchell at NeXT 
 *      Created. 
 */

#ifdef	DRIVER_PRIVATE

#import <bsd/dev/ppc/ConsoleSupport.h>

/*
 * Write message to console; create an alert panel if no text-type window
 * currently exists. Caller must call alert_done() when finished.
 * The height and width arguments are not used; they are provided for 
 * compatibility with the 68k version of alert().
 */
int alert(int width, 
	int height, 
	const char *title, 
	const char *msg, 
	int p1, 
	int p2, 
	int p3, 
	int p4, 
	int p5, 
	int p6, 
	int p7, 
	int p8);
int alert_done(void);

void DoAlert(const char *title, const char *msg);
int DoRestore(void);

/*
 * printf() a message to an an alert panel. Can be used any time after calling
 * alert().
 */
void aprint(const char *msg, 
	int p1, 
	int p2, 
	int p3, 
	int p4, 
	int p5, 
	int p6, 
	int p7, 
	int p8);

/*
 * Dump message log.
 */
int kmdumplog(void);

/*
 * Initialize the console enough for printf's to work. No text window will 
 * be created if booting in icon mode. Keyboard is not enabled. 
 */
void initialize_screen(void * args);

void kmEnableAnimation(void);
void kmDisableAnimation(void);
void kmDrawGraphicPanel(GraphicPanelType panelType);
void kmGraphicPanelString(char *string);
const char *kmLocalizeString(const char *string);

int kmtrygetc(void);

#endif	/* DRIVER_PRIVATE */

