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
#import "libsaio.h"

extern BOOL in_linear_mode;
extern unsigned char  *frame_buffer;
extern unsigned short screen_width;
extern unsigned short screen_height;
extern unsigned char  bits_per_pixel;

#define CHAR_W		8
#define CHAR_W_SHIFT	3
#define CHAR_H		16
#define CHAR_H_SHIFT	4
#define BYTE_SHIFT	3
#define NCOLS		(screen_width / CHAR_W)
#define NROWS		(screen_height / CHAR_H)
#define SCREEN_W	(screen_width)
#define SCREEN_H	(screen_height)

#define TEXTBUFSIZE 1536
extern BOOL showText;
extern char *textBuf;
extern int bufIndex;
