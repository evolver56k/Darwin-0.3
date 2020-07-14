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
 * Copyright (C) 1990 by NeXT, Inc., All Rights Reserved
 *
 */

/* 
 * Network Buffer API (for kernel use only)
 *
 * HISTORY
 * 09-Apr-90  Bradley Taylor (btaylor) at NeXT, Inc.
 *	Created. 
 */
#ifndef _NETBUF_
#define _NETBUF_

typedef struct { char opaque[1]; } *netbuf_t;

#ifdef KERNEL

extern char *nb_map(netbuf_t nb);
extern netbuf_t nb_alloc(unsigned size);
extern netbuf_t nb_alloc_wrapper(void *data, unsigned size,
				 void freefunc(void *), void *freefunc_arg);

extern void nb_free(netbuf_t nb);
extern void nb_free_wrapper(netbuf_t nb);
extern unsigned nb_size(netbuf_t nb);
extern int nb_read(netbuf_t nb, unsigned offset, unsigned size, void *target);
extern int nb_write(netbuf_t nb, unsigned offset, unsigned size, void *source);
extern int nb_shrink_top(netbuf_t nb, unsigned size);
extern int nb_grow_top(netbuf_t nb, unsigned size);
extern int nb_shrink_bot(netbuf_t nb, unsigned size);
extern int nb_grow_bot(netbuf_t nb, unsigned size);

#endif /* KERNEL */
#endif /* _NETBUF_ */
