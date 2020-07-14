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

/* Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *
 *	File:	bsd/kern/kern_symfile.c
 *
 *	This file contains creates a dummy symbol file for mach_kernel based on
 *      the symbol table information passed by the SecondaryLoader/PlatformExpert.
 *      This allows us to correctly link other executables (drivers, etc) against the 
 *      the kernel in cases where the kernel image on the root device does not match
 *      the live kernel. This can occur during net-booting where the actual kernel
 *      image is obtained from the network via tftp rather than the root
 *      device.
 *
 *      If a symbol table is available, then the file /mach.sym will be created
 *      containing a Mach Header and a LC_SYMTAB load command followed by the
 *      the symbol table data for mach_kernel.
 *
 * HISTORY
 * 
 *	.
 */
#import <cputypes.h>
#import <mach_host.h>

#import <mach/vm_param.h>

#import <machine/cpu.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>

#import <mach-o/loader.h>
#import <mach-o/nlist.h>
#import <vm/vm_kern.h>

void 			*symbol_table_addr;			// Initialized by PlatformExpert.
extern unsigned char 	rootdevice[];

/*
 * 
 */
int output_kernel_symbols( register struct proc *p )
{
	register struct vnode		*vp;
	register struct pcred 		*pcred = p->p_cred;
	register struct ucred 		*cred = pcred->pc_ucred;
	struct nameidata 		nd;
	struct vattr			vattr;
	int				command_size, header_size;
	int				hoffset, foffset;
	vm_offset_t			header;
	struct machine_slot		*ms;
	struct mach_header		*mh;
	struct symtab_command 		*sc, *sc0;
        struct nlist                    *nl;
	vm_size_t			size;
	int				error, error1;
        int				i;

        if ( symbol_table_addr == NULL )
                return (EFAULT);

	if ( pcred->p_svuid != pcred->p_ruid || pcred->p_svgid != pcred->p_rgid )
		return (EFAULT);

        if ( rootdevice[0] == 'e' || rootdevice[1] == 'n' )
                return (EROFS);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, "mach.sym", p);
	if( (error = vn_open(&nd, O_CREAT | FWRITE, S_IRUSR | S_IRGRP | S_IROTH )) != 0 )
		return (error);
	vp = nd.ni_vp;
	
	/* Don't dump to non-regular files or files with links. */
	if (vp->v_type != VREG ||
	    VOP_GETATTR(vp, &vattr, cred, p) || vattr.va_nlink != 1) {
		error = EFAULT;
		goto out;
	}

	VATTR_NULL(&vattr);
	vattr.va_size = 0;
	VOP_LEASE(vp, p, cred, LEASE_WRITE);
	VOP_SETATTR(vp, &vattr, cred, p);
	p->p_acflag |= ACORE;

	command_size = sizeof(struct symtab_command);
	header_size = command_size + sizeof(struct mach_header);

	(void) kmem_alloc_wired(kernel_map,
				    (vm_offset_t *)&header,
				    (vm_size_t)header_size);

	/*
	 *	Set up Mach-O header.
	 */
	mh = (struct mach_header *) header;
	ms = &machine_slot[cpu_number()];
	mh->magic = MH_MAGIC;
	mh->cputype = ms->cpu_type;
	mh->cpusubtype = ms->cpu_subtype;
	mh->filetype = MH_CORE;
	mh->ncmds = 1;
	mh->sizeofcmds = command_size;

	hoffset = sizeof(struct mach_header);	/* offset into header */
	foffset = round_page(header_size);	/* offset into file */

	/*
	 *	Fill in segment command structure.
	 */
        sc0         = (struct symtab_command *)  symbol_table_addr;
	sc          = (struct symtab_command *) (header + hoffset);
	sc->cmd     = LC_SYMTAB;
	sc->cmdsize = sizeof(struct symtab_command);
        sc->symoff  = foffset;
        sc->nsyms   = sc0->nsyms;
        sc->strsize = sc0->strsize;
        sc->stroff =  foffset + sc->nsyms * sizeof(struct nlist);    

        size = sc->nsyms * sizeof(struct nlist) + sc->strsize;

        nl = (struct nlist *)(sc0+1);
        for (i = 0; i < sc->nsyms; i++, nl++ )
        {
            if ( (nl->n_type & N_TYPE) == N_SECT )
            {
                nl->n_sect = NO_SECT;
                nl->n_type = (nl->n_type & ~N_TYPE) | N_ABS;
            }
        }

	/*
	 *	Write out the Mach header at the beginning of the
	 *	file.
	 */
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)header, header_size, (off_t)0,
			UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) 0, p);

        /*
         * 	Write out kernel symbols
         */
        if ( !error )
        {
	    error = vn_rdwr(UIO_WRITE, vp, (caddr_t)(sc0+1), size, foffset,
	    		    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) 0, p);

        }

	kmem_free(kernel_map, header, header_size);
out:
	VOP_UNLOCK(vp, 0, p);
	error1 = vn_close(vp, FWRITE, cred, p);
	if (error == 0)
		error = error1;
	return(error);
}
