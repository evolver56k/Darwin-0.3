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
 * Copyright (c) 1990 NeXT, Inc.
 *
 * HISTORY
 * 29-Jun-90  Morris Meyer (mmeyer) at NeXT
 *	Created.
 */

#import <mallocdebug.h>

#if	MALLOCDEBUG

#import <sys/types.h>
#import <mach/boolean.h>
#import <kern/malloc_debug.h>
#import <sys/uio.h>
#import <sys/user.h>
#import <sys/vfs.h>
#import <sys/vnode.h>
#import <vm/vm_kern.h>
#import <vm/vm_pager.h>
#import <kern/lock.h>
#ifdef	m68k
#import <machdep/m68k/eventc.h>
#endif	m68k

int mallocdebug = FALSE;
int malloc_dropped = 0;
int malloc_curbuf = 0;
int malloc_buflim = 0;
char *malloctypes[] = { "MTYPE_KALLOC", "MTYPE_ZALLOC", "MTYPE_KMEM_ALLOC" };
struct vnode *mallocvp;
int mallocoffset;
struct mallocbuf {
	int numstamps;
	struct malloc_info mallocbufs[2048];
} mallocbuf [2];
static void malloc_debug_syncbufs (int nstamps, int whichbuf);

lock_data_t			malloc_lock_data;

#define	malloc_lock()		lock_write(&malloc_lock_data)
#define	malloc_unlock()		lock_write_done(&malloc_lock_data)

mallocdebuginit()
{
	malloc_buflim = 2048;
	lock_init(&malloc_lock_data, TRUE);
}

void malloc_debug (void *addr, void *pc, int size, int which, int type)
{
	struct malloc_info mi, *mip;
	int write_buf;
	int numstamps = mallocbuf[malloc_curbuf].numstamps;
	
	if (mallocdebug) {
		mip = &mallocbuf[malloc_curbuf].mallocbufs[numstamps++];
		mi.type = type;
		mi.which = which;
		mi.addr = addr;
		mi.pc = pc;
		mi.size = size;
#if	m68k
		event_set_ts(&mi.time);
#endif	m68k
		*(struct malloc_info *)mip = mi;

		if (numstamps >= malloc_buflim) {
			write_buf = malloc_curbuf;
			malloc_curbuf ^= 1;
			malloc_debug_syncbufs(numstamps, write_buf);
			numstamps = 0;
		}
		mallocbuf[malloc_curbuf].numstamps = numstamps;
	}
}

static void malloc_debug_syncbufs (int nstamps, int whichbuf)
{
	extern struct ucred *rootcred;
	struct vattr	vattr;
	struct uio auio;
	struct iovec aiov;
	int error;

	if (mallocvp == (struct vnode *)0)  {
		u.u_error = 0;
		vattr_null (&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = 0644;
		vattr.va_size = 0;
		error = vn_create ("/mallocinfo", UIO_SYSSPACE, 
				&vattr, NONEXCL, VWRITE, &mallocvp, rootcred);
		if (error)
			panic ("vn_create: malloc_debug_syncbufs");
		(void) VOP_SETATTR(mallocvp, &vattr, rootcred);
	}
	
	error = vn_rdwr(UIO_WRITE, mallocvp, 
			&mallocbuf[whichbuf].mallocbufs, 
			nstamps * sizeof (struct malloc_info), mallocoffset,
			UIO_SYSSPACE, IO_UNIT | IO_SYNC, (int *) 0);

	malloc_lock();
	mallocoffset += nstamps * sizeof (struct malloc_info);
	malloc_unlock();
}

/*
 * FIXME...
 */
#if	m88k
void *get_return_pc(void)
{
	return 0;
}
#endif	m88k
#endif	MALLOCDEBUG
