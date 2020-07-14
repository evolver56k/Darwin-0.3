/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 8-Jun-92  Matthew Self (mself) at NeXT
 *	obj_vm_allocate changed to always map the mach header of the loaded
 *	module.  This is required to support Objective-C.
 *
 * 21-Apr-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#import "server.h"
#import "obj.h"
#import "log.h"
#import "misc.h"
#import <stdlib.h>
#import <fcntl.h>
#import <stdio.h>
#import <string.h>
#import <libc.h>
#import <streams/streams.h>
#import <mach/mach_error.h>
#import <mach/mach.h>
#import <mach-o/reloc.h>
#import <mach-o/loader.h>
#import <mach-o/rld.h>
#import <mach-o/fat.h>
#import <mach/vm_param.h>
#import <sys/stat.h>
#import <architecture/byte_order.h>

extern boolean_t debug;

const char *libloadserv = "/usr/lib/libloadserv.a";
const char *libloadserv_g = "/usr/local/lib/libloadserv_g.a";

mutex_t rld_mutex;
int rld_lock_taken;
condition_t rld_lock_wait;


#define rld_lock()      mutex_lock(rld_mutex);                             \
                        while (rld_lock_taken){                           \
                                condition_wait(rld_lock_wait, rld_mutex); \
                        }                                                  \
                        rld_lock_taken = 1;                               \
                        mutex_unlock(rld_mutex);

#define rld_unlock()    mutex_lock(rld_mutex);                             \
                        rld_lock_taken = 0;                               \
                        condition_signal(rld_lock_wait);                  \
                        mutex_unlock(rld_mutex);



static server_t *rld_server;
static const char *rld_executable;

#if !defined(page_round)
#define page_trunc(p) ((int)(p)&~(vm_page_size-1))
#define page_round(p) page_trunc((int)(p)+vm_page_size-1)
#endif

/*
 * Allocate space in ourselves and the executable and allocate virtual
 * memory starting with the address addr.
 */
static vm_address_t obj_vm_allocate (
	vm_size_t	file_size,
	vm_size_t	header_size)
{
	vm_address_t	exec_addr, my_addr, addr;
	vm_size_t	exec_size, my_size, vm_size;
	vm_prot_t	prot, max_prot;
	vm_inherit_t	inheritance;
	boolean_t	shared;
	port_t		obj_name;
	vm_offset_t	offset;
	kern_return_t	r;

	/*
	 * Always map the mach header, since this is required for Objective-C.
	 */
	/* header_size = 0; */

	rld_server->rld_size = file_size;
	vm_size = page_round(file_size);
	exec_addr = my_addr = 0;

	/*
	 * Allocate independent memory regions in kernel and loader
	 */
	r = vm_allocate(rld_server->executable_task, &exec_addr, vm_size,
		TRUE);
	switch (r) {
	case KERN_SUCCESS:
		rld_server->alloc_in_exec = TRUE;
		if (vm_protect(rld_server->executable_task,
			(vm_address_t)exec_addr, vm_size,
	    	FALSE, VM_PROT_ALL) != KERN_SUCCESS) {
				kllog(LOG_ERR, "Server %s couldn't set exec bits in kern_loader: "
				"%s (%d)\n", rld_server->name, mach_error_string(r),
				r);
			r = vm_deallocate(rld_server->executable_task, 
				  exec_addr, vm_size);
			if (r != KERN_SUCCESS)
				kllog(LOG_ERR, "vm_deallocate in exec failed: "
					"%s (%d)\n",
					mach_error_string(r), r);
			kllog(LOG_DEBUG, "Server %s deallocated %#x bytes "
				"at address %#x in executable \"%s\"\n",
				rld_server->name, vm_size, exec_addr,
				rld_server->executable);
			rld_server->alloc_in_exec = FALSE;
			return 0;
		}
		break;
	case KERN_NO_SPACE:
	case KERN_INVALID_ADDRESS:
		kllog(LOG_ERR, "couldn't allocate in executable\n");
		return 0;
	default:
		return 0;
	}

	r = vm_allocate(task_self(), &my_addr, vm_size, TRUE);
	switch (r) {
	case KERN_SUCCESS:
		rld_server->alloc_in_self = TRUE;
		break;
	case KERN_NO_SPACE:
	case KERN_INVALID_ADDRESS:
		kllog(LOG_ERR, "Server %s couldn't allocate in kern_loader: "
			"%s (%d)\n", rld_server->name, mach_error_string(r),
			r);
		r = vm_deallocate(rld_server->executable_task, 
				  exec_addr, vm_size);
		if (r != KERN_SUCCESS)
			kllog(LOG_ERR, "vm_deallocate in exec failed: "
				"%s (%d)\n",
				mach_error_string(r), r);
		kllog(LOG_DEBUG, "Server %s deallocated %#x bytes "
			"at address %#x in executable \"%s\"\n",
			rld_server->name, vm_size, exec_addr,
			rld_server->executable);
		rld_server->alloc_in_exec = FALSE;
		return 0;
	default:
		kllog(LOG_ERR, "Server %s couldn't allocate in kern_loader: "
			"%s (%d)\n", rld_server->name, mach_error_string(r),
			r);
		r = vm_deallocate(rld_server->executable_task, 
				  exec_addr, vm_size);
		if (r != KERN_SUCCESS)
			kllog(LOG_ERR, "vm_deallocate in exec failed: "
				"%s (%d)\n",
				mach_error_string(r), r);
		kllog(LOG_DEBUG, "Server %s deallocated %#x bytes "
			"at address %#x in executable \"%s\"\n",
			rld_server->name, vm_size, exec_addr,
			rld_server->executable);
			rld_server->alloc_in_exec = FALSE;
			return 0;
	}

	kllog(LOG_DEBUG, "Server %s allocated %#x bytes at address %#x "
		"in executable \"%s\" and %#x bytes at address %#x in "
		"kern_loader\n",
		rld_server->name, vm_size, exec_addr, rld_server->executable,
		vm_size, my_addr);

	rld_server->vm_addr = exec_addr;
	rld_server->vm_size = vm_size;
	/* here we return the region in the loader task */
	rld_server->mheader = (struct mach_header *)(my_addr);

	return exec_addr;
}

/*
 * Find the modification date of the specified file.
 */
boolean_t obj_date(const char *reloc, u_int *date)
{
	struct	stat	statb;

	if (stat(reloc, &statb) < 0) {
		kllog(LOG_ERR, "stat relocatable failed: %s(%d)\n",
			strerror(cthread_errno()), cthread_errno());
		return FALSE;
	}

	*date = (u_int)statb.st_mtime;
	return TRUE;
}

/*
 * Return the address of the symbol in the relocatable with the
 * given name (or NULL).
 */
vm_address_t obj_symbol_value(const char *filename, const char *name)
{
	vm_address_t symvalue;
	struct nlist nl[3];

	if (sym_value(filename, name, &symvalue) == KERN_SUCCESS) {
		kllog(LOG_DEBUG, "symbol \"%s\" in file %s "
			"cached value is %#x\n",
			name, filename, symvalue);
		return symvalue;
	}

	nl[0].n_un.n_name = (char *)name;
	nl[1].n_un.n_name = (char *)malloc(strlen(name)+2);
	sprintf(nl[1].n_un.n_name, "_%s", name);
	nl[2].n_un.n_name = NULL;
	unix_lock();
	nlist(filename, nl);
	unix_unlock();
	free(nl[1].n_un.n_name);
	symvalue = nl[0].n_value;

	kllog(LOG_DEBUG, "symbol \"%s\" in file %s value is %#x\n",
		name, filename, symvalue);

	if (!symvalue)
		symvalue = nl[1].n_value;

	kllog(LOG_DEBUG, "symbol \"%s\" in file %s value is %#x\n",
		name, filename, symvalue);

	if (symvalue)
		save_symvalue(filename, name, symvalue);
	
	return symvalue;
}

/*
 * Allocate space for and map in the Mach-O file on a page-aligned boundary.
 */
boolean_t obj_map_reloc (server_t *server)
{
	int fd;
	kern_return_t r;
	boolean_t rtn;
	
	if ((fd = open(server->reloc, O_RDONLY, 0)) < 0) {
		kllog(LOG_ERR, "Server %s can't open relocatable %s: %s(%d)\n",
			server->name, server->reloc,
			strerror(cthread_errno()), cthread_errno());
		return FALSE;
	}

	r = map_fd(fd, 0, (vm_address_t *)&server->fheader, TRUE,
		server->reloc_size);
	if (r != KERN_SUCCESS) {
		kllog(LOG_ERR, "Server %s can't map relocatable %s: %s(%d)\n",
			server->name, server->reloc, mach_error_string(r), r);
		return FALSE;
	}
	server->map_in_self = TRUE;
	
	/*
	 * I think these two fields are meaningless until we call 
	 * obj_vm_allocate()...
	 */
#if	0
	server->vm_addr = (vm_offset_t)server->header
			+ server->header->sizeofcmds
			+ sizeof(*server->header);
	server->vm_size = server->reloc_size
			- ((vm_offset_t)server->header - server->vm_addr);
#endif	0

	/*
	 * Deal with possible fat file. 
	 */
	if(((struct mach_header *)server->fheader)->magic == MH_MAGIC) {
		
		/*
		 * Simple case, thin file. The mach header, which we'll
		 * be using from now on, is at the start of the file.
		 */
		kllog(LOG_INFO, "%s is thin mach-o\n", server->reloc);
		server->mheader = (struct mach_header *)server->fheader;
		rtn = TRUE;
	}
	else if((server->fheader->magic == FAT_MAGIC) ||
		(server->fheader->magic == FAT_CIGAM)) {

		unsigned i;
		struct host_basic_info hbi;
		struct fat_arch *fap;

		/*
		 * Convert fat_arch structs to host byte ordering 
		 * (a constraint of NXFindBestFatArch())
		 */
		server->fheader->nfat_arch = 
			NXSwapBigLongToHost(server->fheader->nfat_arch);
		fap = (struct fat_arch *)((unsigned)server->fheader +
			sizeof(struct fat_header));
		for (i = 0; i < server->fheader->nfat_arch; i++) {
			fap[i].cputype =
				NXSwapBigLongToHost(fap[i].cputype);
			fap[i].cpusubtype =
			      	NXSwapBigLongToHost(fap[i].cpusubtype);
			fap[i].offset =
				NXSwapBigLongToHost(fap[i].offset);
			fap[i].size =
				NXSwapBigLongToHost(fap[i].size);
			fap[i].align =
				NXSwapBigLongToHost(fap[i].align);
		}
		
		/* 
		 * Get our host info.
		 */
		i = HOST_BASIC_INFO_COUNT;
		if (r = host_info(host_self(), HOST_BASIC_INFO,
			      (host_info_t)(&hbi), &i) != KERN_SUCCESS) {
			kllog(LOG_ERR, "Can\'t get host info (%d)\n", r);
			rtn = FALSE;
			goto out;
		}
		
		/*
		 * Find the fat_arch struct appropriate for this machine.
		 */
		fap = NXFindBestFatArch(hbi.cpu_type, hbi.cpu_subtype,
			fap, server->fheader->nfat_arch);
		if (!fap) {
			kllog(LOG_ERR, "%s does not contain appropriate"
				" architecture\n", server->reloc);
			rtn = FALSE;
		}
		else {
			server->mheader = (struct mach_header *)
				((char *)server->fheader + fap->offset);
			kllog(LOG_INFO, "%s: fat file; using arch at "
				"offset %d\n", server->reloc, fap->offset);
			rtn = TRUE;
		}
	}
	else {
		kllog(LOG_ERR, "%s is not a Mach object file\n",
			server->reloc);
		rtn = FALSE;
	}
out:
	close(fd);
	return rtn;
}

/*
 * Link the .o file against the executable.
 */
boolean_t obj_link(server_t *server)
{
	int			ok;
	struct stat		statb;
	NXStream		*rld_stream;
	const char		*rld_files[3];
	const char		**syms;
	struct mach_header	*rld_header;
	kern_return_t		r;

	if (!rld_mutex) {
		rld_mutex = mutex_alloc();
		mutex_init(rld_mutex);
		rld_lock_wait = condition_alloc();
		condition_init(rld_lock_wait);
	}

	kllog(LOG_INFO, "Server %s linking %s against %s\n",
		server->name, server->reloc, server->executable);

	delete_symfile(server->name);

	r = vm_deallocate(task_self(), (vm_address_t)server->fheader,
		server->reloc_size);
	if (r != KERN_SUCCESS)
		kllog(LOG_ERR, "vm_deallocate in self failed: "
			"%s (%d)\n",
			mach_error_string(r), r);
	kllog(LOG_DEBUG, "Server %s deallocated %#x bytes "
		"at address %#x in self (map_fd\'d)\n",
		server->name, server->reloc_size,
		(vm_address_t)server->fheader);
	server->map_in_self = FALSE;

	/*
	 * Link the relocatable against the executable and libraries to
	 * create the loadable.  We need to hold the rld lock until we've
	 * unloaded the loadable from the rld package.
	 */
	rld_stream = NXOpenMemory(NULL, 0, NX_READWRITE);
	rld_files[0] = server->reloc;
	if (debug && stat(libloadserv_g, &statb) == 0)
		rld_files[1] = libloadserv_g;
	else
		rld_files[1] = libloadserv;
	rld_files[2] = NULL;

	rld_lock();
	rld_server = server;
	rld_address_func((u_long (*)(u_long, u_long))obj_vm_allocate);
	if (rld_executable != server->executable) {
		kllog(LOG_INFO, "Server %s loading executable \"%s\" for "
			"linkage base file\n", server->name,
			server->executable);
		if (rld_executable)
			rld_unload_all(NULL, TRUE);
		rld_executable = server->executable;
		ok = rld_load_basefile(rld_stream, rld_executable);
		if (!ok) {
			kllog(LOG_ERR, "Basefile load failed\n");
			kllog_stream(LOG_WARNING, rld_stream);
			NXCloseMemory(rld_stream, NX_FREEBUFFER);
			rld_executable = NULL;
			rld_unload_all(NULL, TRUE);
			rld_unlock();
			kllog(LOG_ERR, "Server %s couldn't load "
				"executable \"%s\" for linking\n",
				server->name, server->executable);
			return FALSE;
		}
	}

	if (server->loadable)
		kllog(LOG_NOTICE, "Server %s linking relocatable \"%s\" "
			"into loadable \"%s\"\n",
			server->name, server->reloc, server->loadable);
	else
		kllog(LOG_NOTICE, "Server %s linking relocatable \"%s\"\n",
			server->name, server->reloc);

	ok = rld_load(rld_stream, &rld_header, rld_files,
			server->loadable);
	if (!ok) {
		kllog(LOG_ERR, "Link failed\n");
		kllog_stream(LOG_ERR, rld_stream);
		NXCloseMemory(rld_stream, NX_FREEBUFFER);
		rld_unlock();
		return FALSE;
	}

	/*
	 * Copy the data from the location returned by rld to where
	 * we really want it.
	 */
	memcpy((char *)server->mheader, (char *)rld_header, server->rld_size);

	
	/*
	 * Look up needed symbols.
	 */
	for (syms = server->symbols; *syms; syms++) {
		vm_address_t symvalue;
		
		ok = rld_lookup(rld_stream, *syms, (u_long *)&symvalue);
		if (!ok) {
			char *s = malloc(strlen(*syms)+2);
			sprintf(s, "_%s", *syms);
			ok = rld_lookup(rld_stream, s, (u_long *)&symvalue);
			free(s);
		}
		if (!ok) {
			kllog(LOG_ERR, "Server %s can't find symbol \"%s\" "
				"in loadable\n", server->name, *syms);
			kllog_stream(LOG_WARNING, rld_stream);
			NXCloseMemory(rld_stream, NX_FREEBUFFER);
			rld_unload(NULL);
			rld_unlock();
			return FALSE;
		}
		save_symvalue(server->name, *syms, symvalue);
	}

	NXCloseMemory(rld_stream, NX_FREEBUFFER);

	rld_unload(NULL);
	rld_unlock();
	return TRUE;
}
