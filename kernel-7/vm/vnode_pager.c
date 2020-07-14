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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 *	File:	vnode_pager.c
 *
 *	"Swap" pager that pages to/from vnodes.  Also
 *	handles demand paging from files.
 *
 * 12-Mar-86  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 */

#import <mach_nbc.h>

#import <mach/boolean.h>
#import <sys/param.h>
#import <sys/systm.h>
#import <kern/lock.h>
#import <sys/proc.h>
#import <sys/buf.h>
#import <sys/uio.h>
#import <sys/vnode.h>
#import <ufs/ufs/quota.h>
#import <ufs/ufs/inode.h>
#import <sys/namei.h>
#import <sys/mach_swapon.h>
#import <ufs/ffs/fs.h>
#import <sys/mount.h>
#import <net/if.h>
#import <netinet/in.h>
#import <nfs/rpcv2.h>
#import <nfs/nfsproto.h>
#import <nfs/nfs.h>
#undef	fs_fsok
#undef	fs_tsize
#undef	fs_bsize
#undef	fs_blocks
#undef	fs_bfree
#undef	fs_bavail

#import <mach/mach_types.h>
#import <vm/vm_page.h>
#if defined(ppc)
#import <vm/pmap.h>
#endif /* ppc */
#import <vm/vm_map.h>
#import <vm/vm_kern.h>
#import <kern/parallel.h>
#import <kern/zalloc.h>
#import <kern/kalloc.h>

#import <vm/vnode_pager.h>
#import <kern/mapfs.h>

#import <kern/assert.h>

extern struct vnodeops nfs_vnodeops;
extern struct vnodeops spec_vnodeops;

#if	NBBY == 8
#define BYTEMASK 0xff
#else	NBBY
Define a byte mask for this machine.
#endif	NBBY


#define PAGEMAP_THRESHOLD	64 /* Integral of sizeof(vm_offset_t) */
#define	PAGEMAP_ENTRIES		(PAGEMAP_THRESHOLD/sizeof(vm_offset_t))
#define	PAGEMAP_SIZE(npgs)	(npgs*sizeof(long))

#define	INDIR_PAGEMAP_ENTRIES(npgs) \
	(((npgs-1)/PAGEMAP_ENTRIES) + 1)
#define INDIR_PAGEMAP_SIZE(npgs) \
	(INDIR_PAGEMAP_ENTRIES(npgs) * sizeof(caddr_t))
#define INDIR_PAGEMAP(size) \
	(PAGEMAP_SIZE(size) > PAGEMAP_THRESHOLD)

#define RMAPSIZE(blocks) \
	(howmany(blocks,NBBY))

/*
 *	Sigh... with NFS/vnodes it is highly likely that we will need
 *	to allocate memory at page-out time, so use the XP hack to reserve
 *	pages and always use kalloc/zalloc instead of kget/zget.
 *	This must be fixed!!!  FIXME - XXX.
 */

#define kget(size)	kalloc_noblock(size)
#define zget(zone)	zalloc_noblock(zone)

/*
 *	Basic vnode pager data structures
 */
zone_t			vstruct_zone;
simple_lock_data_t	vstruct_lock;

static queue_head_t	pager_files;
static int		pager_file_count;
static pager_file_t	pager_file_list[MAXPAGERFILES];

static pf_entry		seen_files[MAXPAGERFILES];
static int		seen_files_max = 0;


/*
 *	Routine:	vnode_pager_vput
 *	Function:
 *		Release one use of this vnode_pager_t
 */
void
vnode_pager_vput(vs)
	register vnode_pager_t	vs;
{

	simple_lock(&vstruct_lock);
	vs->vs_count--;
	simple_unlock(&vstruct_lock);
}

/*
 *	vnode_pager_vget:
 *
 *	Return a vnode corresponding to the specified paging space
 *	and guarantee that it will remain in memory (until furthur action
 *	is taken).
 *
 *	The vnode is returned unlocked.
 */
struct vnode *
vnode_pager_vget(vs)
	vnode_pager_t	vs;
{
	register struct vnode *vp;

	simple_lock(&vstruct_lock);
	vs->vs_count++;
	simple_unlock(&vstruct_lock);
	vp = vs->vs_vp;
	return(vp);
}


/*
 * vnode_pager_allocpage - allocate a page in a paging file
 */
daddr_t
vnode_pager_allocpage(pf)
     register struct pager_file *pf;
{
	int bp;			/* byte counter */
	int i;			/* bit counter */
	daddr_t page;		/* page number */

	lock_write(&pf->pf_lock);

	if (pf->pf_pfree == 0) {
		lock_done(&pf->pf_lock);
		return(-1);
	}

	/*
	 *  Start at hint page and work up.
	 */
	i = 0;
	for (bp = pf->pf_hint / NBBY; bp < howmany(pf->pf_npgs, NBBY); bp++) {
		if (*(pf->pf_bmap + bp) != BYTEMASK) {
			for (i = 0; i < NBBY; i++) {
				if (isclr((pf->pf_bmap + bp), i))
					break;
			}
			break;
		}
	}
	page = bp*NBBY+i;
	if (page >= pf->pf_npgs) {
		panic("vnode_pager_allocpage");
	}
	if (page > pf->pf_hipage) {
		pf->pf_hipage = page;
	}
	setbit(pf->pf_bmap,page);
	--pf->pf_pfree;
	pf->pf_hint = page;

	lock_done(&pf->pf_lock);
	return(page);
}


/*
 * vnode_pager_findpage - find an available page in some paging file, using the
 * argument as a preference.  If the pager_file argument is NULL, any file will
 * do.  Return the designated page and file in entry.
 */
kern_return_t
vnode_pager_findpage(struct pager_file *preferPf, pf_entry *entry)
{
	daddr_t result;
	pager_file_t pf;
	
	if (preferPf == PAGER_FILE_NULL) {
		if (!queue_empty(&pager_files))
		    preferPf = (pager_file_t) queue_first(&pager_files);
		else
		    return KERN_FAILURE;
	}
		
	pf = preferPf;		     
	do {
		result = vnode_pager_allocpage(pf);
		if (result != -1) {
			entry->index = pf->pf_index;
			entry->offset = result;
			return KERN_SUCCESS;
		}
		
		if (queue_end(&pager_files, &pf->pf_chain))
			pf = (pager_file_t) queue_first(&pager_files);
		else
			pf = (pager_file_t) queue_next(&pf->pf_chain);
			
	} while (preferPf != pf);
	
	return KERN_FAILURE;
}

static void
vnode_pager_deallocpage(pf_entry entry)
{
	register struct pager_file *pf;
	daddr_t		page;

	if (entry.index == INDEX_NULL)
		return;

	assert(entry.index <= pager_file_count);
	
	pf = pager_file_list[entry.index];
	page = entry.offset;
	
	lock_write(&pf->pf_lock);

	if (page >= (daddr_t) pf->pf_npgs)
		panic("vnode_pager_deallocpage");
	if (page < pf->pf_hint)
		pf->pf_hint = page;
	clrbit(pf->pf_bmap, page);
	++pf->pf_pfree;
	
	lock_done(&pf->pf_lock);
}


/*
 *	pagerfile_pager_create
 *
 *	Create an vstruct corresponding to the given pagerfile.
 *
 */
vnode_pager_t
pagerfile_pager_create(pf, size)
	register pager_file_t	pf;
	vm_size_t		size;
{
	register vnode_pager_t	vs;
	register int i;

	/*
	 *	XXX This can still livelock -- if the
	 *	pageout daemon needs an vnode_pager record
	 *	it won't get one until someone else
	 *	refills the zone.
	 */

	vs = (struct vstruct *) zget(vstruct_zone);

	if (vs == VNODE_PAGER_NULL)
		return(vs);

	vs->vs_size = atop(round_page(size));
	
	if (vs->vs_size == 0)
		vs->vs_pmap = (pf_entry **) 0;
	else {
		if (INDIR_PAGEMAP(vs->vs_size)) {
			vs->vs_pmap = (pf_entry **)
				kget(INDIR_PAGEMAP_SIZE(vs->vs_size));
		} else {
			vs->vs_pmap = (pf_entry **)
				kget(PAGEMAP_SIZE(vs->vs_size));
		}
		if (vs->vs_pmap == (pf_entry **) 0) {
			/*
			 * We can't sleep here, so if there are no free pages, then
			 * just return nothing.
			 */
			zfree(vstruct_zone, (vm_offset_t) vs);
			return(VNODE_PAGER_NULL);
		}
	
		if (INDIR_PAGEMAP(vs->vs_size)) {
			bzero((caddr_t)vs->vs_pmap,
				INDIR_PAGEMAP_SIZE(vs->vs_size));
		} else {
			for (i = 0; i < vs->vs_size; i++)
			    ((pf_entry *) &vs->vs_pmap[i])->index = INDEX_NULL;
		}
	}
	
	vs->is_device = FALSE;
	vs->vs_count = 1;
	vs->vs_vp = pf->pf_vp;
	vs->vs_swapfile = TRUE;
	vs->vs_pf = pf;
	pf->pf_count++;

	vnode_pager_vput(vs);

	return(vs);
}


/*
 *  pagerfile_lookup
 *
 *  Look for an entry at the specified offset in vstruct.  If it's there,
 *  fill in the entry and return TRUE, otherwise return FALSE.
 */
static boolean_t
pagerfile_lookup(struct vstruct *vs, vm_offset_t f_offset, pf_entry *entry)
{
	vm_offset_t	f_page  = atop(f_offset);
	int		indirBlock;
	int		blockOffset;

        if (f_page >= vs->vs_size)
                return FALSE;

	/*
	 * Now look for the entry in the map.
	 */
	if (INDIR_PAGEMAP(vs->vs_size)) {
		indirBlock = f_page / PAGEMAP_ENTRIES;
		blockOffset = f_page % PAGEMAP_ENTRIES;
    
		if (vs->vs_pmap[indirBlock] == NULL ||
		    vs->vs_pmap[indirBlock][blockOffset].index == INDEX_NULL)
			return FALSE;
		else
			*entry = vs->vs_pmap[indirBlock][blockOffset];

	} else {	/* direct map */
	    
		if (((pf_entry *) &vs->vs_pmap[f_page])->index == INDEX_NULL)
			return FALSE;
		else
			*entry = *((pf_entry *) &vs->vs_pmap[f_page]);
	}
	return TRUE;
}


/*
 *	pagerfile_bmap
 *
 *	Fill in the map entry (pager file, offset) for a given f_offset into an
 *	object backed this pager map.
 *
 *	Returns: KERN_FAILURE if page not in map or no room left
 */
static kern_return_t
pagerfile_bmap(vs, f_offset, flag, entry)
	struct vstruct *vs;
	vm_offset_t f_offset;
	int flag;
	pf_entry *entry;
{
    vm_offset_t	f_page = atop(f_offset);
    boolean_t	found;
    int 	i;

    /*
     *  Check the map to see if we can find it.  If we can't, then we'll
     *  make room below.
     */
    found = pagerfile_lookup(vs, f_offset, entry);

	if ((found) && (entry->index > pager_file_count))
		panic("pagerfile_bmap: bad index %d", entry->index);

    if (flag == B_READ)
	return found ? KERN_SUCCESS : KERN_FAILURE;
    else if (found) {
   	/*
	 *  Deallocate the page here, if the hint says that there's free
	 *  space earlier in the file.  We do this to keep the swapfile
	 *  as small as possible, and to enable the swapfile compactor.
	 */
	struct pager_file *pf;

	if (entry->index == 0)
		panic("pagerfile_bmap: 0 index");

	pf = pager_file_list[entry->index];
	if (pf->pf_hint < entry->offset)
	    vnode_pager_deallocpage(*entry);
	else
	    return KERN_SUCCESS;
    }

    /*
     * If the object has grown, expand the page map.
     */
    if (f_page + 1 > vs->vs_size) {
	pf_entry	**new_pmap;
	int		new_size;

	new_size = f_page + 1;
	assert(new_size > 0);
	
	if (INDIR_PAGEMAP(new_size)) {	/* new map is indirect */

	    if (vs->vs_size == 0) {
		/*
		 * Nothing to copy, just get a new
		 * map and zero it.
		 */
		new_pmap = (pf_entry **) kget(INDIR_PAGEMAP_SIZE(new_size));
		if (new_pmap == NULL)
			return (KERN_FAILURE);
		bzero((caddr_t)new_pmap, INDIR_PAGEMAP_SIZE(new_size));	    	
	    }
	    else if (INDIR_PAGEMAP(vs->vs_size)) {
	    
		if (INDIR_PAGEMAP_SIZE(new_size) == 
		    INDIR_PAGEMAP_SIZE(vs->vs_size)) {
			goto leavemapalone;
		}
		
		/* Get a new indirect map */
		new_pmap = (pf_entry **) kget(INDIR_PAGEMAP_SIZE(new_size));
		if (new_pmap == NULL)
			return KERN_FAILURE;

		bzero((caddr_t)new_pmap, INDIR_PAGEMAP_SIZE(new_size));

		/* Old map is indirect, copy the entries */
		for (i = 0; i < INDIR_PAGEMAP_ENTRIES(vs->vs_size); i++)
			new_pmap[i] = vs->vs_pmap[i];
			
		/* And free the old map */
		kfree(vs->vs_pmap, INDIR_PAGEMAP_SIZE(vs->vs_size));
				
	    } else {		/* old map was direct, new map is indirect */
	    
		/* Get a new indirect map */
		new_pmap = (pf_entry **) kget(INDIR_PAGEMAP_SIZE(new_size));
		if (new_pmap == NULL)
			return KERN_FAILURE;

		bzero((caddr_t)new_pmap, INDIR_PAGEMAP_SIZE(new_size));

		/*
		 * Old map is direct, move it to the first indirect block.
		 */
		new_pmap[0] = (pf_entry *) kget(PAGEMAP_THRESHOLD);
		if (new_pmap[0] == NULL) {
			kfree(new_pmap, INDIR_PAGEMAP_SIZE(new_size));
			return KERN_FAILURE;
		}
		for (i = 0; i < vs->vs_size; i++)
			new_pmap[0][i] = *((pf_entry *) &vs->vs_pmap[i]);
			
		/* Initialize the remainder of the block */
		for (i = vs->vs_size; i < PAGEMAP_ENTRIES; i++)
			new_pmap[0][i].index = INDEX_NULL;
		    
		/* And free the old map */
		kfree(vs->vs_pmap, PAGEMAP_SIZE(vs->vs_size));
	    }
	    
	} else {	/* The new map is a direct one */
	
	    new_pmap = (pf_entry **) kget(PAGEMAP_SIZE(new_size));
	    if (new_pmap == NULL)
		    return KERN_FAILURE;

	    /* Copy info from the old map */
	    for (i = 0; i < vs->vs_size; i++)
		    new_pmap[i] = vs->vs_pmap[i];

	    /* Initialize the rest of the new map */
	    for (i = vs->vs_size; i < new_size; i++)
		    ((pf_entry *) &new_pmap[i])->index = INDEX_NULL;
		    
	    if (vs->vs_size > 0)
	    	kfree(vs->vs_pmap, PAGEMAP_SIZE(vs->vs_size));
	}
	
	vs->vs_pmap = new_pmap;
leavemapalone:
	vs->vs_size = new_size;
    }

    /*
     * Now allocate the spot for the new page.
     */
    if (INDIR_PAGEMAP(vs->vs_size)) {

	int indirBlock = f_page / PAGEMAP_ENTRIES;   /* the indirect block */
	int blockOffset = f_page % PAGEMAP_ENTRIES;  /* offset into block */

	/*
	 *  In an indirect map, we may need to allocate space for the
	 *  indirect block itself.
	 */
	if (vs->vs_pmap[indirBlock] == NULL) {
		vs->vs_pmap[indirBlock]=(pf_entry *) kget(PAGEMAP_THRESHOLD);

		if (vs->vs_pmap[indirBlock] == NULL)
			return KERN_FAILURE;

		for (i = 0; i < PAGEMAP_ENTRIES; i++)
		    vs->vs_pmap[indirBlock][i].index = INDEX_NULL;
	}

	if (vnode_pager_findpage(vs->vs_pf, entry) == KERN_FAILURE)
		return KERN_FAILURE;
	else
		vs->vs_pmap[indirBlock][blockOffset] = *entry;

    } else {	/* direct map */

	if (vnode_pager_findpage(vs->vs_pf, entry) == KERN_FAILURE)
		return KERN_FAILURE;

	*(pf_entry *) &vs->vs_pmap[f_page] = *entry;
    }
    
    return KERN_SUCCESS;
}

/*
 *	vnode_pager_create
 *
 *	Create an vstruct corresponding to the given vp.
 *
 */
vnode_pager_t
vnode_pager_create(vp)
	register struct vnode	*vp;
{
	vnode_pager_t	vs;
	struct vattr	vattr;
	vm_size_t	size;
	struct proc	*p = current_proc();
	int		error;

	/*
	 *	XXX This can still livelock -- if the
	 *	pageout daemon needs a vnode_pager record
	 *	it won't get one until someone else
	 *	refills the zone.
	 */

	vs = (struct vstruct *) zalloc(vstruct_zone);

	if (vs == VNODE_PAGER_NULL)
		return(vs);

	bzero((caddr_t)vs, sizeof(struct vstruct));

	vs->is_device = FALSE;
	vs->vs_count = 1;
	vp->v_vm_info->pager = (vm_pager_t) vs;
	vs->vs_vp = vp;
	vs->vs_swapfile = FALSE;

	error = VOP_GETATTR(vp, &vattr, p->p_ucred, p);
	if (!error) {
	    size = vattr.va_size;
	    vp->v_vm_info->vnode_size = size;
	}
	else
	    vp->v_vm_info->vnode_size = 0;
	
	VREF(vp);

	vnode_pager_vput(vs);

	return(vs);
}

/*
 *	vnode_pager_setup
 *
 *	Set up a vstruct for a given vnode.  This is an exported routine.
 */
vm_pager_t
vnode_pager_setup(vp, is_text, can_cache)
	struct vnode	*vp;
	boolean_t	is_text;
	boolean_t	can_cache;
{
	register pager_file_t	pf;

	unix_master();

	if (is_text)
		vp->v_flag |= VTEXT;
#if !MACH_NBC
	if(!vp->v_vm_info)
		vm_info_init(vp);
#endif /* MACH_NBC */

	if (vp->v_vm_info->pager == vm_pager_null) {
		/*
		 * Check to make sure this isn't in use as a pager file.
		 */
		for (pf = (pager_file_t) queue_first(&pager_files);
		     !queue_end(&pager_files, &pf->pf_chain);
		     pf = (pager_file_t) queue_next(&pf->pf_chain)) {
			if (pf->pf_vp == vp) {
				return(vm_pager_null);
			}
		}
		(void) vnode_pager_create(vp);
		if (can_cache)
			vm_object_cache_object(
				vm_object_lookup(vp->v_vm_info->pager), TRUE);
	} 

	unix_release();
	/*
	 * Try to keep something in the vstruct zone since we can sleep
	 * here if necessary.
	 */
	zfree(vstruct_zone, zalloc(vstruct_zone));
	return(vp->v_vm_info->pager);
}

#ifdef	i386

static __inline__ vm_offset_t
vnode_pageio_setup(
    vm_page_t		m
)
{
    return (VM_PAGE_TO_PHYS(m));
}

static __inline__ void
vnode_pageio_complete(
    vm_page_t		m,
    vm_offset_t		addr
)
{
    /* do nothing */
}

#else	/* notdef i386 */

static vm_offset_t
vnode_pageio_setup(
    vm_page_t		m
)
{
    kern_return_t	result;
    vm_offset_t		addr = vm_map_min(kernel_map);

    result = vm_map_find(kernel_map, VM_OBJECT_NULL, 0,
			 &addr, PAGE_SIZE, TRUE);
    if (result != KERN_SUCCESS)
	return (0);

    pmap_enter_phys_page(VM_PAGE_TO_PHYS(m), addr);

    return (addr);
}

static void
vnode_pageio_complete(
    vm_page_t		m,
    vm_offset_t		addr
)
{
    vm_offset_t		taddr = trunc_page(addr);

    (void) vm_map_remove(kernel_map, taddr, taddr + PAGE_SIZE);
}

#endif	/* notdef i386 */

pager_return_t
vnode_pagein(
    vm_page_t		m,
    int			*errorp
)
{
	struct vnode	*vp;
	vnode_pager_t	vs;
	pager_return_t	result = PAGER_SUCCESS;
	vm_offset_t	f_offset;
	pf_entry	entry;
	struct proc	*p = current_proc();
	int		error = 0;

	unix_master();

	vs = (vnode_pager_t) m->object->pager;
	vp = vnode_pager_vget(vs);
	f_offset = m->offset + m->object->paging_offset;

	if (vs->vs_swapfile) {
	    if (pagerfile_bmap(vs, f_offset, B_READ, &entry) == KERN_FAILURE)
		result = PAGER_ABSENT;
	    else {
		f_offset = ptoa(entry.offset);
		vp = pager_file_list[entry.index]->pf_vp;
	    }
	}

	if (result != PAGER_ABSENT) {
	    vm_offset_t		ioaddr = vnode_pageio_setup(m);
	    struct uio		auio;
	    struct iovec	aiov;

	    if (ioaddr) {
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = f_offset;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_resid = PAGE_SIZE;
		auio.uio_procp = NULL;

		aiov.iov_len = PAGE_SIZE;
		aiov.iov_base = (caddr_t)ioaddr;

		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY | LK_CANRECURSE, p);

		m->nfspagereq=TRUE;
		error = VOP_PAGEIN(vp, &auio, 0, p->p_ucred);
		m->nfspagereq=FALSE;
		if (error)
		    result = PAGER_ERROR;
		vp->v_vm_info->error = error;

		VOP_UNLOCK(vp, 0, p);

		if (!error && auio.uio_resid > 0)
		    (void) memset((void *)(ioaddr + PAGE_SIZE -
					   auio.uio_resid), 0, auio.uio_resid);

		vnode_pageio_complete(m, ioaddr);
#ifdef	ppc
		/*
		 * After a pagein, we must synchronize the processor caches.
		 * On PPC, the i-cache is not coherent in all models, thus
		 * it needs to be invalidated.
		 */
		flush_cache(VM_PAGE_TO_PHYS(m), PAGE_SIZE);
#endif /* ppc */
	    }
	    else
		result = PAGER_ERROR;
	}

	vnode_pager_vput(vs);

	if (errorp)
	    *errorp = error;

	unix_release();

	return (result);
}

pager_return_t
vnode_pageout(
    vm_page_t		m
)
{
	struct vnode	*vp;
	vnode_pager_t	vs;
	pager_return_t	result = PAGER_SUCCESS;
	vm_offset_t	f_offset;
	vm_size_t	size = PAGE_SIZE;
	pf_entry	entry;
	struct proc	*p = current_proc();
	int		error = 0;

	unix_master();

	vs = (vnode_pager_t) m->object->pager;
	vp = vnode_pager_vget(vs);
	f_offset = m->offset + m->object->paging_offset;

#if	MACH_NBC
	if (!vs->vs_swapfile) {
	    /*
	     *	Be sure that a paging operation doesn't
	     *	accidently extend the size of "mapped" file.
	     *
	     *	However, we do extend the size up to the current
	     *	size kept in the vm_info structure.
	     */
	    if (f_offset + size > vp->v_vm_info->vnode_size) {
		if (f_offset > vp->v_vm_info->vnode_size)
		    size = 0;
		else
		    size = vp->v_vm_info->vnode_size - f_offset;
	    }
	}
#endif	MACH_NBC

	if (vs->vs_swapfile) {
	    if (pagerfile_bmap(vs, f_offset, B_WRITE, &entry) == KERN_FAILURE)
		result = PAGER_ERROR;
	    else {
		/*
		 *	If the paging operation extends the size of the
		 *	pagerfile, update the information in the vm_info
		 *	structure
		 */
		f_offset = ptoa(entry.offset);
		vp = pager_file_list[entry.index]->pf_vp;
		if (f_offset + size > vp->v_vm_info->vnode_size)
		    vp->v_vm_info->vnode_size = f_offset + size;
	    }
	}

	if (result != PAGER_ERROR && size > 0) {
	    vm_offset_t		ioaddr = vnode_pageio_setup(m);
	    struct uio		auio;
	    struct iovec	aiov;

	    if (ioaddr) {
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = f_offset;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_WRITE;
		auio.uio_procp = NULL;
		aiov.iov_base = (caddr_t)ioaddr;
#if MACH_NBC
		auio.uio_resid = size;
		aiov.iov_len = size;
#else
		auio.uio_resid = PAGE_SIZE;
		aiov.iov_len = PAGE_SIZE;
#endif /* MACH_NBC */

#if MACH_NBC
		{
#define	VNODE_LOCK_RETRY_COUNT	1
#define	VNODE_LOCK_RETRY_TICKS	2

			int retry = VNODE_LOCK_RETRY_COUNT;
vnode_lock_retry:
			error = vn_lock(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_CANRECURSE, p);
			if (error) {
				/*
				 * Retry the lock after yielding to other threads.
				 * Not doing this makes the pageout thread compute bound.
				 * Yielding lets I/O threads to run
				 * and make forward progress.
				 */
				if (retry-- > 0) {
					thread_will_wait(current_thread());
					thread_set_timeout(VNODE_LOCK_RETRY_TICKS);
					thread_block();
					goto vnode_lock_retry;
				}

				result = PAGER_ERROR;
				vnode_pageio_complete(m, ioaddr);
				goto out;
			}
		}
#else /* MACH_NBC */
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
#endif /* MACH_NBC */

		error = VOP_PAGEOUT(vp, &auio, 0, p->p_ucred);
		if (error)
		    result = PAGER_ERROR;
		vp->v_vm_info->error = error;
			
		VOP_UNLOCK(vp, 0, p);

		vnode_pageio_complete(m, ioaddr);
	    }
	    else
		result = PAGER_ERROR;
	}

	if (result == PAGER_SUCCESS) {
	    m->clean = TRUE;		 		/* XXX - wrong place */
	    pmap_clear_modify(VM_PAGE_TO_PHYS(m));	/* XXX - wrong place */
	}

out:
	vnode_pager_vput(vs);

	unix_release();

	return (result);
}

/*
 *	vnode_has_page:
 *
 *	Parameters:
 *		pager
 *		id		paging object
 *		offset		Offset in paging object to test
 *
 *	Assumptions:
 *		This is only used on shadowing (copy) objects.
 *		Since copy objects are always backed by a swapfile, we just
 *		test the map for that swapfile to see if the page is present.
 */
boolean_t
vnode_has_page(pager, offset)
	vm_pager_t	pager;
	vm_offset_t	offset;
{
	vnode_pager_t	vs = (vnode_pager_t) pager;
	pf_entry	entry;

	/*
	 * For now, we do all inode hacking on the master cpu.
	 */
	unix_master();

	if (vs == VNODE_PAGER_NULL)
		panic("vnode_has_page: failed lookup");

	if (vs->vs_swapfile) {
		unix_release();
		if (pagerfile_bmap(vs, offset, B_READ, &entry) == KERN_FAILURE)
			return FALSE;
		else
			return TRUE;
	}
	else {
		 panic("vnode_has_page called on non-default pager");
	}
	/*NOTREACHED*/

	return FALSE;
}


/*
 * 	Routine:	vnode_pager_file_init
 *	Function:
 *		Create a pager_file structure for a new pager file.
 *	Arguments:
 *		The file in question is specified by vnode pointer.
 *		lowat and hiwat are the low water and high water marks
 *		that the size of pager file will float between.  If
 *		the low water mark is zero, then the file will not
 *		shrink after paging space is freed.  If the high water
 *		mark is zero, the file will grow without bounds.
 *
 *	The vp is locked on entry to and exit from this function.
 */
int
vnode_pager_file_init(pfp, vp, lowat, hiwat)
	pager_file_t	*pfp;
	struct vnode	*vp;
	long lowat;
	long hiwat;
{
	struct vattr	vattr;
	register pager_file_t	pf;
	int	error;
	long	i;
	struct proc	*p = current_proc();
	struct ucred	*cred;
	vm_size_t	size;

	*pfp = PAGER_FILE_NULL;

	/*
	 * Make sure no other object paging to this file?
	 */
#if	MACH_NBC
	mapfs_uncache(vp);
#endif	/* MACH_NBC */

 	if(!vp->v_vm_info) {
 		vm_info_init(vp);
 	}
 	
	if (vp->v_vm_info->mapped) {
		return(EBUSY);
	}
	
	/*
	 * Clean up the file blocks on a pager file by
	 * truncating to length "lowat".
	 */
	error = VOP_GETATTR(vp, &vattr, p->p_ucred, p);
	size = vattr.va_size;
	if (size > lowat) {
		vattr_null(&vattr);
		vattr.va_size = size = lowat;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
		if (error) {
			return(error);
		}
	}

	/*
	 * Initialize the vnode_size field
	 */
	vp->v_vm_info->vnode_size = size;

	pf = (pager_file_t) kalloc(sizeof(struct pager_file));
	VREF(vp);
	pf->pf_vp = vp;
	cred = p->p_ucred;
	crhold(cred);
	vp->v_vm_info->cred = cred;
	pf->pf_count = 0;
	pf->pf_hint = 0;
	pf->pf_lowat = atop(round_page(lowat));
	/*
	 * If no maximum space is specified, then we should make a map that
	 * can cover the entire disk, otherwise the block map need only
	 * cover the maximum space allowed.
	 */
	if (!hiwat)
		hiwat = vp->v_mount->mnt_stat.f_blocks * 
					vp->v_mount->mnt_stat.f_bsize;
	pf->pf_pfree = pf->pf_npgs = atop(hiwat);
	pf->pf_bmap = (u_char *) kalloc(RMAPSIZE(pf->pf_npgs));
	for (i = 0; i < pf->pf_npgs; i++) {
		clrbit(pf->pf_bmap, i);
	}
	pf->pf_hipage = -1;
	pf->pf_prefer = FALSE;
	lock_init(&pf->pf_lock, TRUE);

	/*
	 * Put the new pager file in the list.
	 */
	queue_enter(&pager_files, pf, pager_file_t, pf_chain);
	pager_file_count++;
	pf->pf_index = pager_file_count;
	pager_file_list[pager_file_count] = pf;
	*pfp = pf;
	return (0);
}

void
vnode_pager_shutdown()
{
	pager_file_t	pf;

	while (!queue_empty(&pager_files)) {
		pf = (pager_file_t) queue_first(&pager_files);
		vrele(pf->pf_vp);
		queue_remove(&pager_files, pf, pager_file_t, pf_chain);
		pager_file_count--;
	}
}


/*
 *	Routine:	mach_swapon
 *	Function:
 *		Syscall interface to mach_swapon.
 */
int
mach_swapon(filename, flags, lowat, hiwat)
	char 	*filename;
	int	flags;
	long	lowat;
	long	hiwat;
{
	struct vnode		*vp;
	struct nameidata 	nd, *ndp;
	struct proc		*p = current_proc();
	pager_file_t		pf;
	register int		error;

	ndp = &nd;

	if ((error = suser(p->p_ucred, &p->p_acflag)))
		return (error);

	unix_master();

	/*
	 * Get a vnode for the paging area.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    filename, p);
	if ((error = namei(ndp)))
		return (error);
	vp = ndp->ni_vp;

	if (vp->v_type != VREG) {
		error = EINVAL;
		goto bailout;
	}

	/*
	 * Look to see if we are already paging to this file.
	 */
	for (pf = (pager_file_t) queue_first(&pager_files);
	     !queue_end(&pager_files, &pf->pf_chain);
	     pf = (pager_file_t) queue_next(&pf->pf_chain)) {
		if (pf->pf_vp == vp)
			break;
	}
	if (!queue_end(&pager_files, &pf->pf_chain)) {
		error = EBUSY;
		goto bailout;
	}

	error = vnode_pager_file_init(&pf, vp, lowat, hiwat);
	if (error) {
		goto bailout;
	}
	pf->pf_prefer = ((flags & MS_PREFER) != 0);

        /*
         * Create dummy symbol file for current mach_kernel executable.
         * See bsd/kern/kern_symfile.c
         */
	output_kernel_symbols(p);

	error = 0;
bailout:
	if (vp) {
		VOP_UNLOCK(vp, 0, p);
		vrele(vp);
	}
	unix_release();
	return(error);
}

/*
 *	Routine:	vswap_allocate
 *	Function:
 *		Allocate a place for paging out a kernel-created
 *		memory object.
 *
 *	Implementation: 
 *		Looks through the paging files for the one with the
 *		most free space.  First, only "preferred" paging files
 *		are considered, then local paging files, and then
 *		remote paging files.  In each case, the pager file
 *		the most free blocks will be chosen.
 *
 *	In/out conditions:
 *		If the paging area is on a local disk, the inode is
 *		returned locked.
 */
pager_file_t
vswap_allocate()
{
	int		pass;
	int 		mostspace;
	pager_file_t	pf, mostpf;
	
		extern int (**ffs_vnodeop_p)();

	mostpf = PAGER_FILE_NULL;
	mostspace = 0;

	if (pager_file_count > 1) {
		for (pass = 0; pass < 4; pass++) {
			for (pf = (pager_file_t)queue_first(&pager_files);
			     !queue_end(&pager_files, &pf->pf_chain);
			     pf = (pager_file_t)queue_next(&pf->pf_chain)) {

				if ((pass < 2) && !pf->pf_prefer)
					continue;
				if ((!(pass &1) &&
				     (pf->pf_vp->v_op != ffs_vnodeop_p)))
					continue;

				if (pf->pf_pfree > mostspace) {
					mostspace = pf->pf_pfree;
					mostpf = pf;
				}
			}
			/*
			 * If we found space, then break out of loop.
			 */
			if (mostpf != PAGER_FILE_NULL)
				break;
		}
	} else if (pager_file_count == 1) {
		mostpf = (pager_file_t) queue_first(&pager_files);
	}

	return(mostpf);
}

vm_pager_t
vnode_alloc(size)
	vm_size_t	size;
{
	pager_file_t	pf;
	vnode_pager_t 	vs = (vnode_pager_t) vm_pager_null;

#ifdef	lint
	size++;
#endif	lint

	unix_master();

	/*
	 *	Get a pager_file, then turn it into a paging space.
	 */

	if ((pf = vswap_allocate()) == PAGER_FILE_NULL) {
		goto out;
	}
	if ((vs = pagerfile_pager_create(pf, size)) ==
	    VNODE_PAGER_NULL) {
		vs = (vnode_pager_t) vm_pager_null;
		goto out;
	}
out:
	unix_release();
	return((vm_pager_t) vs);
}


/*
 *  Try to truncate the paging files.
 */
void
vnode_pager_truncate(pf_entry entry)
{
	struct pager_file *pf = pager_file_list[entry.index];
	struct vnode	*vp = pf->pf_vp;
	struct vattr	vattr;
	int		error;
	struct proc	*p = current_proc();
	long		truncpage;
	int		i;

	/*
	 *  If this is not the last page in the file, return now.
	 *  If the swaptimizer is enabled we cannot free blocks out from
	 *  underneath it.
	 */
	assert(entry.offset <= pf->pf_hipage);
	if (entry.offset < pf->pf_hipage)
		return;

	lock_write(&pf->pf_lock);

	/*
	 * Find a new high page
	 */
	for (i = entry.offset - 1; i >= 0; i--) {
		if (isset(pf->pf_bmap, i)) {
			pf->pf_hipage = i;
			break;
		}
	}

	/*
	 *  If we are higher than the low water mark, truncate
	 *  the file.
	 */
	truncpage = pf->pf_hipage + 1;
	if (pf->pf_lowat == 0 || truncpage <= pf->pf_lowat ||
	    vp->v_vm_info->vnode_size < ptoa(truncpage)) {
		lock_done(&pf->pf_lock);
		return;
	}

	vattr_null(&vattr);
	vattr.va_size = ptoa(truncpage);
	ASSERT( (int) vattr.va_size >= 0 );

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if ((error = VOP_SETATTR(vp, &vattr, vp->v_vm_info->cred, p))) {
		printf("vnode_deallocpage: error truncating %s,"
			" error = %d\n", pf->pf_name, error);
	}
	VOP_UNLOCK(vp, 0, p);


	lock_done(&pf->pf_lock);
}


static void
visit_file(pf_entry entry)
{
	int	i;

	if (entry.index == INDEX_NULL)
		return;

	for (i = 0;  i < seen_files_max;  i++)
	    if (seen_files[i].index == entry.index) {
		seen_files[i].offset = max(seen_files[i].offset,entry.offset);
		return;
	    }

	seen_files[seen_files_max++] = entry;
}


void
vnode_dealloc(pager)
	vm_pager_t	pager;
{
	struct vnode	*vp;
	vnode_pager_t	vs = (vnode_pager_t) pager;
	int		i;

	unix_master();

	vp = vnode_pager_vget(vs);

	ASSERT(vs->vs_count == 1);

	seen_files_max = 0;
	if (vs->vs_swapfile) {
	    pager_file_t	pf;
	    int		i,j;

	    ASSERT(vs->vs_pf);

	    pf = vs->vs_pf;
	    if (INDIR_PAGEMAP(vs->vs_size)) {
		for (i = 0; i < INDIR_PAGEMAP_ENTRIES(vs->vs_size); i++) {
		    if (vs->vs_pmap[i] != NULL) {
			for(j = 0; j < PAGEMAP_ENTRIES; j++) {
			    vnode_pager_deallocpage(vs->vs_pmap[i][j]);
			    visit_file(vs->vs_pmap[i][j]);
			}
			kfree(vs->vs_pmap[i], PAGEMAP_THRESHOLD);
		    }
		}
		kfree(vs->vs_pmap, INDIR_PAGEMAP_SIZE(vs->vs_size));
	    } else {
		for (i = 0; i < vs->vs_size; i++) {
		    vnode_pager_deallocpage(*(pf_entry *)&vs->vs_pmap[i]);
		    visit_file(*(pf_entry *)&vs->vs_pmap[i]);
		}
		if (vs->vs_size > 0)
			kfree(vs->vs_pmap, PAGEMAP_SIZE(vs->vs_size));
	    }
	    pf->pf_count--;
	} else {
	    vp->v_flag &= ~VTEXT;
	    vp->v_vm_info->pager = vm_pager_null; /* so vrele will free */
    
	    vp->v_flag |= VAGE; /* put this vnode at the head of freelist */
	    vrele(vp);
	}

	for (i=0; i < seen_files_max; i++)
		vnode_pager_truncate(seen_files[i]);
	zfree(vstruct_zone, (vm_offset_t) vs);
	unix_release();
}

/*
 * Remove vnode associated object from the object cache.
 *
 * XXX unlock the vnode if it is currently locked.
 * We must do this since uncaching the object may result in its
 * destruction which may initiate paging activity which may necessitate
 * re-locking the vnode.
 */
int
vnode_uncache(vp)
	register struct vnode	*vp;
{
	struct proc *p = current_proc();

	if (vp->v_type != VREG)
		return (1);

	if (vp->v_vm_info == 0 || vp->v_vm_info->pager == vm_pager_null)
		return (1);

#ifdef DEBUG
    if (!VOP_ISLOCKED(vp)) {
	extern int (**nfsv2_vnodeop_p)();

		if (vp->v_op != nfsv2_vnodeop_p)
			panic("vnode_uncache: vnode not locked!");
	}
#endif

	/*
	 * The act of uncaching may cause an object to be deallocated
	 * which may need to wait for the pageout daemon which in turn
	 * may be waiting for this inode's lock, so be sure to unlock
	 * and relock later if necessary.  (This of course means that
	 * code calling this routine must be able to handle the fact
	 * that the inode has been unlocked temporarily).  This code, of
	 * course depends on the Unix master restriction for proper
	 * synchronization.
	 */
#if	MACH_NBC
	mapfs_uncache(vp);
#endif	/* MACH_NBC */

	VOP_UNLOCK(vp, 0, p);
	vm_object_uncache(vp->v_vm_info->pager);
#if MACH_NBC
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY | LK_CANRECURSE, p);
#else
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
#endif
	return (1);
}

void
vnode_pager_init()
{
	register vm_size_t	size;

	/*
	 *	Initialize zone of paging structures.
	 */

	size = (vm_size_t) sizeof(struct vstruct);
	vstruct_zone = zinit(size,
			(vm_size_t) 10000*size,	/* XXX */
			PAGE_SIZE,
			FALSE, "vnode pager structures");
	simple_lock_init(&vstruct_lock);
	queue_init(&pager_files);
}

void
vnode_pager_setsize(vp, nsize)
struct vnode *vp;
u_long nsize;
{
	if (vp->v_vm_info) {
		if (vp->v_type != VREG)
			panic("vnode_pager_setsize not VREG");
		vp->v_vm_info->vnode_size = nsize;
	}
}

void
vnode_pager_umount(mp)
    register struct mount *mp;
{
	struct proc *p = current_proc();
	struct vnode *vp, *nvp;

loop:
	for (vp = mp->mnt_vnodelist.lh_first; vp; vp = nvp) {
		if (vp->v_mount != mp)
			goto loop;
		nvp = vp->v_mntvnodes.le_next;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		(void) vnode_uncache(vp);
		VOP_UNLOCK(vp, 0, p);
	}
}
