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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	ipc/ipc_port.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Implementation specific complement to mach/port.h.
 */

#ifndef	_IPC_PORT_H_
#define _IPC_PORT_H_

#include <mach/port.h>

/*
 *	mach_port_t must be an unsigned type.  Port values
 *	have two parts, a generation number and an index.
 *	These macros encapsulate all knowledge of how
 *	a mach_port_t is layed out.  However, ipc/ipc_entry.c
 *	implicitly assumes when it uses the splay tree functions
 *	that the generation number is in the low bits, so that
 *	names are ordered first by index and then by generation.
 *
 *	If the size of generation numbers changes,
 *	be sure to update IE_BITS_GEN_MASK and friends
 *	in ipc/ipc_entry.h.
 */

#define	MACH_PORT_INDEX(name)		((name) >> 8)
#define	MACH_PORT_GEN(name)		(((name) & 0xff) << 24)
#define	MACH_PORT_MAKE(index, gen)	(((index) << 8) | ((gen) >> 24))

#define	MACH_PORT_NGEN(name)		MACH_PORT_MAKE(0, MACH_PORT_GEN(name))
#define	MACH_PORT_MAKEB(index, bits)	\
		MACH_PORT_MAKE(index, IE_BITS_GEN(bits))

/*
 *	Typedefs for code cleanliness.  These must all have
 *	the same (unsigned) type as mach_port_t.
 */

typedef mach_port_t mach_port_index_t;		/* index values */
typedef mach_port_t mach_port_gen_t;		/* generation numbers */


#define	MACH_PORT_UREFS_MAX	((mach_port_urefs_t) ((1 << 16) - 1))

#define	MACH_PORT_UREFS_OVERFLOW(urefs, delta)				\
		(((delta) > 0) &&					\
		 ((((urefs) + (delta)) <= (urefs)) ||			\
		  (((urefs) + (delta)) > MACH_PORT_UREFS_MAX)))

#define	MACH_PORT_UREFS_UNDERFLOW(urefs, delta)				\
		(((delta) < 0) && (-(delta) > (urefs)))

#endif	/* _IPC_PORT_H_ */
