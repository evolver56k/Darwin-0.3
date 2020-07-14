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
 * Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 1994 NeXT Computer, Inc.  All rights reserved.
 *
 * machdep/ppc/fault_copy.c
 *
 * Machine dependent code for kernel user copy
 *
 * History :
 * 22-Jul-1998 Umesh Vaishampayan      (umeshv@apple.com)
 *     Added safe_bzero().
 *
 * 30-Mar-1998	Umesh Vaishampayan  (umeshv@apple.com)
 *	Made the copy routines more robust.
 *
 * 22-Sep-1997  Umesh Vaishampayan  (umeshv@apple.com)
 *	Fixed error return from ENOENT to ENAMETOOLONG.
 *
 * March, 1997	Umesh Vaishampayan [umeshv@NeXT.com]
 *	Created from sparc.
 *
 */

#include <diagnostic.h>
#include <mach/mach_types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <machine/setjmp.h>
#include <machine/label_t.h>

#undef NBSEG
#define NBSEG 0x10000000
#undef SEGMASK
#define SEGMASK (NBSEG-1)
#define PGOFFSET(x) ((unsigned)x & (NBPG-1))
#undef SEGOFFSET
#define SEGOFFSET(x) ((unsigned)(x) & SEGMASK)


caddr_t cioseg(space_t space, caddr_t addr);

int
safe_bcopy(caddr_t src, caddr_t dst, int len)
{
	int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("safe_bcopy() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	} else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		bcopy(src, dst, len);
	}
	current_thread()->recover = (vm_offset_t)tmpbuf;

	return(error);
}

int
safe_bzero(void *dst, unsigned long ulen)
{
	int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if (tmpbuf)
		kprintf("safe_bzero() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	} else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		bzero(dst, ulen);
	}
	current_thread()->recover = (vm_offset_t)tmpbuf;

	return(error);
}


int
lbcopytoz(from, to, maxlen)
    register caddr_t from, to;
    unsigned maxlen;
{
	register unsigned l;

	for (l = 0; l < maxlen; l++) {
		if (*from == '\0')
			return l;
		*to++ = *from++;
	}
	return maxlen;
}

/* 
 * copy a null terminated string from the user address space into
 * the kernel address space.
 *   - if the user is denied read access, return EFAULT
 *   - if the end of string isn't found before
 *     maxlen bytes are copied,  return ENAMETOOLONG,
 *     indicating an incomplete copy.
 *   - otherwise, return 0, indicating success.
 * the number of bytes copied is always returned in lencopied.
 */
int
copyinstr(from, to, maxlen, lencopied)
caddr_t from, to;
unsigned maxlen, *lencopied;
{
	unsigned segroom, room = maxlen;
	caddr_t oto = to;
	caddr_t mapped_from;
	unsigned l;
	int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("copyinstr() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	} else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		do {
			/* calc # of bytes to transfer from this seg. */
			segroom = MIN(room, NBSEG - SEGOFFSET(from));

			mapped_from = cioseg(current_task()->map->pmap->space,from);
			/* this *does not* copy a terminating null */
			l = lbcopytoz(mapped_from, to, segroom);

			room -= l;
			from += l;
			to   += l;
		} while (l == segroom && room > 0);
	}

	if (room == 0)
		error = ENAMETOOLONG;

	if (error == 0)
		*to++ = '\0';	// NULL terminate

	current_thread()->recover = (vm_offset_t)tmpbuf;

	if (lencopied)
		*lencopied = to - oto;

	return error;
}

/*
 * Copy count bytes from (src) to (dst)
 */
int
copywithin(src, dst, count)
caddr_t src,dst;
unsigned count;
{
	int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	}
	else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		bcopy(src,dst,count);
	}

	current_thread()->recover = (vm_offset_t)tmpbuf;
	return (error);
}

caddr_t view_user_address(caddr_t addr);
int kdp_map_segment(unsigned int segment);

/*
 * Copy count bytes from (src) to (dst) where one is user space
 */
int
copy_for_kdp_user(src, dst, count, dir)
caddr_t src,dst;
unsigned count;
int dir;
{
    caddr_t	msrc, mdst, ptr;
    unsigned mcount;
    int seg;
    int rtn_value;

#define SEG_NUM(x)	((((unsigned int)(x))>>28)&0xF)
#define SEG_ADDR(x, y)	((((unsigned int)(x)) << 28) | (((unsigned int)(y)) & 0x0FFFFFFF))

    rtn_value = 0;
    while (count > 0) {
	if (dir) {
	    ptr = dst;
	    seg = kdp_map_segment(SEG_NUM(ptr));
	    if (seg < 0) {
		return 1;
	    }
	    mdst = view_user_address(SEG_ADDR(seg, ptr));
	    msrc = src;
	} else {
	    ptr = src;
	    mdst = dst;
	    seg = kdp_map_segment(SEG_NUM(ptr));
	    if (seg < 0) {
		return 1;
	    }
	    msrc = view_user_address(SEG_ADDR(seg, ptr));
	}
	if (SEG_NUM(ptr) != SEG_NUM(ptr+count-1)) {
	    mcount = 0x10000000 - SEG_ADDR(0, ptr);
	    count -= mcount;
	    src += mcount;
	    dst += mcount;
	} else {
	    mcount = count;
	    count = 0;
	}
	rtn_value |= safe_bcopy(msrc,mdst,mcount);
    }
    return rtn_value;
}

/*
 * Copy count bytes from (src) to (dst)
 */
int
copy_for_kdp(src, dst, count, dir)
vm_offset_t src,dst;
unsigned count;
int dir;
{
	int rtn_value = 0;
 
	if (dir) {
		if (dst > VM_MAX_KERNEL_ADDRESS) {
			rtn_value = copy_for_kdp_user(src, dst, count, 1);
		} else if (dst >= VM_MIN_KERNEL_ADDRESS) {
			rtn_value = safe_bcopy(src, dst, count);
			kdp_flush_icache(dst, count);
		}
	} else {
		if (src > VM_MAX_KERNEL_ADDRESS) {
			rtn_value = copy_for_kdp_user(src, dst, count, 0);
		} else {
			rtn_value = safe_bcopy(src, dst, count);
		}
	}
	return rtn_value;
}

/* 
 * copy a null terminated string from one point to another in 
 * the kernel address space.
 *   - no access checks are performed.
 *   - if the end of string isn't found before
 *     maxlen bytes are copied,  return ENAMETOOLONG,
 *     indicating an incomplete copy.
 *   - otherwise, return 0, indicating success.
 * the number of bytes copied is always returned in lencopied.
 */
int
copystr(from, to, maxlen, lencopied)
    register caddr_t from, to;
    unsigned maxlen, *lencopied;
{
    register unsigned l;
    int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("copystr() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	} else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		for (l = 0; l < maxlen; l++)
			if ((*to++ = *from++) == '\0') {
				if (lencopied)
					*lencopied = l + 1;
				error = 0;
				goto out;
			}
		if (lencopied)
			*lencopied = maxlen;
		error = ENAMETOOLONG;
	}
out:
	current_thread()->recover = (vm_offset_t)tmpbuf;
	return (error);
}

/* 
 * copy a null terminated string from the kernel address space into
 * the user address space.
 *   - if the user is denied write access, return EFAULT.
 *   - if the end of string isn't found before
 *     maxlen bytes are copied,  return ENAMETOOLONG,
 *     indicating an incomplete copy.
 *   - otherwise, return 0, indicating success.
 * the number of bytes copied is always returned in lencopied.
 */
int
copyoutstr(from, to, maxlen, lencopied)
    caddr_t from, to;
    unsigned maxlen, *lencopied;
{
	unsigned segroom, room = maxlen;
	caddr_t oto = to;
	caddr_t mapped_to;
	unsigned l;
	int error = 0;
	label_t jmpbuf;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("copyoutstr() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	} else {
		current_thread()->recover = (vm_offset_t)&jmpbuf;
		do {
			/* calc # of bytes to transfer from this seg. */
			segroom = MIN(room, NBSEG - SEGOFFSET(to));

			mapped_to = cioseg(current_task()->map->pmap->space,to);

			/* this *does not* copy a terminating null */
			l = lbcopytoz(from, mapped_to, segroom);

			room -= l;
			from += l;
			to   += l;
			mapped_to += l;  // for NULL whenever
		} while (l == segroom && room > 0);

		/* set terminating NULL */
		*mapped_to = '\0';
		to++;
	}

	if (room == 0)
		error = ENAMETOOLONG;

	if (lencopied)
		*lencopied = to - oto;

	current_thread()->recover = (vm_offset_t)tmpbuf;
	return error;
}

#undef PGOFFSET

int
copyin(void *src, void *dst, size_t len)
{
	thread_t	self = current_thread();
	vm_offset_t	tmpbuf = self->recover;
	label_t		jmpbuf;
	int		error = 0;
	void		*tsrc;
	size_t		tlen;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("copyin() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	}
	else {
		self->recover = (vm_offset_t)&jmpbuf;
		while (len > 0) {
		    	tlen = ((SEGOFFSET(src) + len) > NBSEG) ?
			    		NBSEG - SEGOFFSET(src) : len;
			tsrc = cioseg(self->task->map->pmap->space, src);
			bcopy(tsrc, dst, tlen);
			src += tlen;
			dst += tlen;
			len -= tlen;
		}
	}
	self->recover = (vm_offset_t)tmpbuf;
	return(error);
}

int
copyout(void *src, void *dst, size_t len)
{
	thread_t	self = current_thread();
	vm_offset_t	tmpbuf = self->recover;
	label_t		jmpbuf;
	int		error = 0;
	void		*tdst;
	size_t		tlen;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("copyout() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

	if (setjmp(&jmpbuf)) {
		error = EFAULT;
	}
	else {
		self->recover = (vm_offset_t)&jmpbuf;
		while (len > 0) {
		    	tlen = ((SEGOFFSET(dst) + len) > NBSEG) ?
			    		NBSEG - SEGOFFSET(dst) : len;
			tdst = cioseg(self->task->map->pmap->space, dst);
			bcopy(src, tdst, tlen);
			src += tlen;
			dst += tlen;
			len -= tlen;
		}
	}
	self->recover = (vm_offset_t)tmpbuf;
	return(error);
}

int
copyinmsg(caddr_t src, caddr_t dst, int len)
{
	return(copyin(src,dst,len));
}

int
copyoutmsg(caddr_t src, caddr_t dst, int len)
{
	return(copyout(src, dst, len));
}
