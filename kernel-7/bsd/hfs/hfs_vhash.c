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

/* Copyright (c) 1998 Apple Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)hfs_vhash.c
 *	derived from @(#)ufs_ihash.c	8.7 (Berkeley) 5/17/95
 */

#include <sys/param.h>
#include <mach/machine/vm_types.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include "hfs.h"

#include <libkern/libkern.h>
#include <mach/machine/simple_lock.h>

/*
 * Structures associated with hfsnode cacheing.
 */
LIST_HEAD(vhashhead, hfsnode) *vhashtbl;
u_long	vhash;		/* size of hash table - 1 */
#define	HFSNODEHASH(device, nodeID, forkType)	(&vhashtbl[((device) + ((nodeID)<<2) + (forkType)) & vhash])
struct slock hfs_vhash_slock;

/*
 * Initialize hfsnode hash table.
 */
void
hfs_vhashinit()
{

    vhashtbl = hashinit(desiredvnodes, M_HFSMNT, &vhash);
	simple_lock_init(&hfs_vhash_slock);
}

/*
 * Use the device/dirID pair to find the incore hfsnode, and return a pointer
 * to it. If it is in core, return it, even if it is locked.
 */
struct vnode *
hfs_vhashlookup(dev, nodeID, forkType)
	dev_t 		dev;
    UInt32 		nodeID;
UInt8	forkType;
{
	struct hfsnode *hp;

	simple_lock(&hfs_vhash_slock);
    for (hp = HFSNODEHASH(dev, nodeID, forkType)->lh_first; hp; hp = hp->h_hash.le_next)
    	if ((H_FILEID(hp) == nodeID) && (hp->h_dev == dev) && (H_FORKTYPE(hp) == forkType))
			break;
	simple_unlock(&hfs_vhash_slock);

	if (hp)
		return (HTOV(hp));
	return (NULLVP);
}

/*
 * Use the device/dirID pair to find the incore hfsnode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
hfs_vhashget(dev, nodeID, forkType)
    dev_t dev;
    UInt32 nodeID;
	UInt8	forkType;
{
	struct proc *p = CURRENT_PROC;	/* XXX */
	struct hfsnode *hp;
	struct vnode *vp;

loop:
	simple_lock(&hfs_vhash_slock);
    for (hp = HFSNODEHASH(dev, nodeID, forkType)->lh_first; hp; hp = hp->h_hash.le_next) {
    	if ((H_FILEID(hp) == nodeID) && (hp->h_dev == dev) && (H_FORKTYPE(hp) == forkType)) {
			vp = HTOV(hp);
			simple_lock(&vp->v_interlock);
			simple_unlock(&hfs_vhash_slock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK, p))
				goto loop;
			return (vp);
		}
	}
	simple_unlock(&hfs_vhash_slock);
	return (NULL);
}

/*
* Lock the hfsnode and insert the hfsnode into the hash table, and return it locked.
 */
void
hfs_vhashins(hp)
	struct hfsnode *hp;
{
	struct vhashhead *ipp;

    lockmgr(&hp->h_lock, LK_EXCLUSIVE, (struct slock *)0, CURRENT_PROC);

	simple_lock(&hfs_vhash_slock);
    ipp = HFSNODEHASH(hp->h_dev, H_FILEID(hp), H_FORKTYPE(hp));
	LIST_INSERT_HEAD(ipp, hp, h_hash);
	simple_unlock(&hfs_vhash_slock);
}

/*
 * Insert a locked hfsnode into the hash table.
 */
void
hfs_vhashinslocked(hp)
	struct hfsnode *hp;
{
	struct vhashhead *ipp;

	simple_lock(&hfs_vhash_slock);
    ipp = HFSNODEHASH(hp->h_dev, H_FILEID(hp), H_FORKTYPE(hp));
	LIST_INSERT_HEAD(ipp, hp, h_hash);
	simple_unlock(&hfs_vhash_slock);
}

/*
 * Remove the hfsnode from the hash table.
 */
void
hfs_vhashrem(hp)
	struct hfsnode *hp;
{

	simple_lock(&hfs_vhash_slock);
    LIST_REMOVE(hp, h_hash);
#if DIAGNOSTIC
	hp->h_hash.le_next = NULL;
	hp->h_hash.le_prev = NULL;
#endif
	simple_unlock(&hfs_vhash_slock);
}
