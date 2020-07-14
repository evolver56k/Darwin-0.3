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

/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 * 
 *	File:	driverkit/objc_support.m
 *
 *	Run-time support for kernel ObjC library.
 *
 * HISTORY
 * 31-Jan-92	Doug Mitchell at NeXT
 *	Created.
 */

#import <streams/streams.h>
#import <bsd/sys/syslog.h>
#import <bsd/stdarg.h>
#import <bsd/machine/spl.h>

/*
 * FIXME - what the heck is this?
 */
char **NXArgv;

int NXFlush(NXStream *s)
{
	/*
	 * Nothing to do for now.
	 */
	return 0;
}

void NXPrintf(NXStream *s, const char *format, ...)
{
	register sp = splhigh();
	va_list ap;

	va_start(ap, format);
	log(LOG_ERR, format, ap);
	va_end(ap);
	splx(sp);
}

/*
 * This should never be called...
 */
void abort()
{
	panic("objc: fatal error\n");
}

#import <objc/zone.h>

#import <kern/kalloc.h>

#define	MDECL(reqlen)					\
union {							\
	struct	_mhead hdr;				\
	char	_m[(reqlen) + sizeof (struct _mhead)];	\
}

struct _mhead {
	size_t	mlen;
	char	dat[0];
};

static __inline__
void *kern_malloc(
	struct _NXZone	*zonep,
	size_t		size)
{
	MDECL(size)	*mem;
	size_t		memsize = sizeof (*mem);

	if (size == 0)
		return (0);

	mem = (void *)kalloc(memsize);
	if (!mem)
		return (0);

	mem->hdr.mlen = memsize;
	(void) memset(mem->hdr.dat, 0, size);

	return  (mem->hdr.dat);
}

static __inline__
void kern_free(
	struct _NXZone	*zonep,
	void		*addr)
{
	struct _mhead	*hdr;

	if (!addr)
		return;

	hdr = addr; hdr--;
	kfree((vm_offset_t)hdr, hdr->mlen);
}

static
void *kern_realloc(
        struct _NXZone	*zonep,
	void		*addr,
	size_t		nsize)
{
	struct _mhead	*ohdr;
	MDECL(nsize)	*nmem;
	size_t		nmemsize, osize;
	
	if (!addr)
		return (kern_malloc(zonep, nsize));

	ohdr = addr; ohdr--;
	osize = ohdr->mlen - sizeof (*ohdr);
	if (nsize == osize)
		return (addr);

	if (nsize == 0) {
		kfree((vm_offset_t)ohdr, ohdr->mlen);
		return (0);
	}

	nmemsize = sizeof (*nmem);
	nmem = (void *)kalloc(nmemsize);
	if (!nmem)
		return (0);

	nmem->hdr.mlen = nmemsize;
	if (nsize > osize)
		(void) memset(&nmem->hdr.dat[osize], 0, nsize - osize);
	(void) memcpy(nmem->hdr.dat, ohdr->dat,
					(nsize > osize) ? osize : nsize);
	kfree((vm_offset_t)ohdr, ohdr->mlen);

	return (nmem->hdr.dat);
}

static void kern_destroy(struct _NXZone *zonep) 
{
}

NXZone	KernelZone = {
	kern_realloc,
	kern_malloc,
	kern_free,
	kern_destroy,
};

NXZone *NXDefaultMallocZone()
{
	return &KernelZone; 
}

NXZone *NXZoneFromPtr(void *ptr)
{
	return(&KernelZone);
}


NXZone *NXCreateZone(size_t startsize, size_t granularity, int canfree)
{
	return(&KernelZone);
}

void NXNameZone(NXZone *z, const char *name)
{
	/* EMPTY */
}

void *
NXZoneCalloc(NXZone *zonep, size_t numElems, size_t byteSize)
{
	return kern_malloc(zonep, numElems*byteSize);
}

void *malloc(
	size_t		size)
{
	return kern_malloc(&KernelZone, size);
}

void free(
	void		*addr)
{
	kern_free(&KernelZone, addr);
}

void *calloc(
	size_t		num,
	size_t		size)
{
	return kern_malloc(&KernelZone, num*size);
}

void *realloc(
	void		*addr,
	size_t		size)
{
	return kern_realloc(&KernelZone, addr, size);
}
