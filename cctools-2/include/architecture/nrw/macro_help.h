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
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * 	09-23-91 mike	Created from Mach version
 * 
 */
/*
 *	File:	architecture/nrw/macro_help.h
 *
 *	Provide help in making lint-free macro routines
 *
 */

#ifndef	_NRW_MACRO_HELP_H_
#define	_NRW_MACRO_HELP_H_

#ifndef	MACRO_BEGIN
# define		MACRO_BEGIN	do {
#endif	MACRO_BEGIN

#ifndef	MACRO_END
# define		MACRO_END	} while (0)
#endif	MACRO_END

#ifndef	MACRO_RETURN
# define		MACRO_RETURN	if (1) return
#endif	MACRO_RETURN

#endif	_NRW_MACRO_HELP_H_

