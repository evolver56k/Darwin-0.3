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

/******************************************************************************
	event.h (PostScript side version)
	
	CONFIDENTIAL
	Copyright (c) 1992 NeXT Computer, Inc. as an unpublished work.
	All Rights Reserved.

	Created jpasqua 22 Sept 1992

	Modified:
******************************************************************************/

#ifndef _PPC_DEV_EVENT_
#define _PPC_DEV_EVENT_

#import <bsd/dev/event.h>	/* Portable event defs */
/*
 * Device-dependent bits within event.flags
 * (The bits probably won't change, but may not be supported by all hardware)
 *
 * Bletch!  This stuff is also defined in dpsclient/event.h!
 * That's totally inappropriate for such machine dependent
 * stuff.  I guarantee we can't support this for all possible
 * keyboards. (Hint: What do NX_NEXTLCMDKEYMASK and
 * NX_NEXTRCMDKEYMASK mean on the keyboards with a single
 * Command bar?)
 */

#ifndef NX_NEXTCTLKEYMASK
#define	NX_NEXTCTLKEYMASK	0x00000001
#endif

#ifndef NX_NEXTLSHIFTKEYMASK
#define	NX_NEXTLSHIFTKEYMASK	0x00000002
#endif

#ifndef NX_NEXTRSHIFTKEYMASK
#define	NX_NEXTRSHIFTKEYMASK	0x00000004
#endif

#ifndef NX_NEXTLCMDKEYMASK
#define	NX_NEXTLCMDKEYMASK	0x00000008
#endif

#ifndef NX_NEXTRCMDKEYMASK
#define	NX_NEXTRCMDKEYMASK	0x00000010
#endif

#ifndef NX_NEXTLALTKEYMASK
#define	NX_NEXTLALTKEYMASK	0x00000020
#endif

#ifndef NX_NEXTRALTKEYMASK
#define	NX_NEXTRALTKEYMASK	0x00000040
#endif

/* NX_STYLUSPROXIMITYMASK	0x00000080	RESERVED */
/* NX_NONCOALSESCEDMASK		0x00000100	RESERVED */


#endif /* _PPC_DEV_EVENT_ */

