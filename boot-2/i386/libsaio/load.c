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
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#import "libsaio.h"
#import "memory.h"
#import "kernBootStruct.h"
#import "load.h"
#import "sarld.h"
#import "language.h"
#import "stringConstants.h"
#import "rcz.h"

#import <ufs/ufs/dir.h>
#import <mach-o/fat.h>

#if 1
int ram_debug_sarld=0;
#endif 1
int devMajor[3] = { 6, 3, 1 };  	// sd, hd, fd major dev #'s


/* Open a file for reading.  If the file doesn't exist,
 * try opening the compressed version.
 */
int
openfile(char *filename, int ignored)
{
    unsigned char *buf;
    int fd, size, ret;
    unsigned char *addr;
    
    if ((fd = open(filename, 0)) < 0) {
	buf = malloc(256);
	sprintf(buf, "%s%s", filename, RCZ_EXTENSION);
	if ((fd = open(buf, 0)) >= 0) {
	    size = rcz_file_size(fd);
	    addr = (unsigned char *)((KERNEL_ADDR + KERNEL_LEN) - size);
	    ret = rcz_decompress_file(fd, addr);
	    close(fd);
	    if (ret < 0)
		fd = -1;
	    else
		fd = openmem(addr, size);
	}
	free(buf);
    }
    return fd;
}

int
loadprog(
	int		dev,
	int		fd,
	struct mach_header *headOut,
	entry_t		*entry,		// entry point
	char		**addr,		// load address
	int		*size		// size of loaded program
)
{
    struct mach_header head;
    int file_offset = 0;
    
read_again:
    /* get file header */
    read(fd, (char *)&head, sizeof(head));
    if (headOut)
	bcopy((char *)&head, (char *)headOut, sizeof(head));

    if (head.magic == MH_MAGIC) {
#if	0
		printf("oneway fat binary found\n");
		sleep(1);
#endif	1
	return loadmacho(&head, dev, fd, entry, addr, size, file_offset);
    }
    else if (file_offset == 0 && 
	    ((head.magic == FAT_CIGAM) || (head.magic == FAT_MAGIC)))
    {
	int swap = (head.magic == FAT_CIGAM) ? 1 : 0;
	struct fat_header *fhp = (struct fat_header *)&head;
	struct fat_arch *fap;
	int i, narch = swap ? NXSwapLong(fhp->nfat_arch) : fhp->nfat_arch;
	int cpu, size;
	char *buf;
	
	size = sizeof(struct fat_arch) * narch;
	buf = malloc(size);
	b_lseek(fd, 0, 0);
	read(fd, buf, size);
	
	for (i = 0, fap = (struct fat_arch *)(buf+sizeof(struct fat_header));
	     i < narch;
	     i++, fap++) {
	    cpu = swap ? NXSwapLong(fap->cputype) : fap->cputype;
	    if (cpu == CPU_TYPE_I386) {
		/* that's specific enough */
		free(buf);
		file_offset = swap ? NXSwapLong(fap->offset) : fap->offset;
		b_lseek(fd, file_offset, 0);
		goto read_again;
	    }
	}
	free(buf);
	error("Fat binary file doesn't contain i386 code\n");
	return -1;
    }
    error("Unrecognized binary format: %08x\n", head.magic);
    return -1;
}

/* read from file descriptor.
 * addr is a physical address.
 */

int xread(
	int		fd,
	char		* addr,
	int		size
)
{
	char		* orgaddr = addr;
	long		offset;
	unsigned	count;
	long		max;
#define BUFSIZ 8192
	char		*buf;
	int		bufsize = BUFSIZ;

#if 0
	if (ram_debug_sarld) {
		printf("In xread addr=%x, size=%x\n", addr, size);
		sleep(1);
	}
#endif 1
	buf = malloc(BUFSIZ);
	
	// align your read to increase speed
	offset = tell(fd) & 4095;
	if ( offset != 0)
		max = 4096 - offset;
	else
		max = bufsize;

	while (size > 0)
	{
		if (size > max) count = max;
		else count = size;
#if 0
	if (ram_debug_sarld) {
		printf("In xread loop size=%x; count=%x\n", size, count );
		sleep(1);
	}
#endif 1

		if ( read(fd, buf, count) != count) break;

		bcopy(buf, ptov(addr), count);
		size -= count;
		addr += count;

		max = bufsize;

#if notdef
		tick += count;
		if (tick > (50*1024))
		{
			putchar('+');
			tick = 0;
		}
#endif
	}

	free(buf);
	return addr-orgaddr;
}

int
loadmacho(
    struct mach_header *head,
    int dev,
    int io,
    entry_t *rentry,
    char **raddr,
    int *rsize,
    int file_offset
)
{
	int ncmds;
	unsigned  cmds, cp;
	struct xxx_thread_command {
		unsigned long	cmd;
		unsigned long	cmdsize;
		unsigned long	flavor;
		unsigned long	count;
		i386_thread_state_t state;
	} *th;
	unsigned int	entry = 0;
	int vmsize = 0;
	unsigned int vmaddr = ~0;

	// XXX should check cputype
	cmds = (unsigned int) malloc(head->sizeofcmds);
	b_lseek(io, sizeof (struct mach_header) + file_offset, 0);

	if (read(io, (char *)cmds, head->sizeofcmds) != head->sizeofcmds) {
	    error("Error reading commands\n");
	    goto shread;
	}
    
	for (ncmds = head->ncmds, cp = cmds; ncmds > 0; ncmds--)
	{
		unsigned int	addr;

#define lcp	((struct load_command *)cp)		
		switch (lcp->cmd)
		{
	    
		case LC_SEGMENT:
#define scp	((struct segment_command *)cp)

			addr = (scp->vmaddr & 0x3fffffff) + (int)*raddr;
			if (scp->filesize) {
			    // Is this an OK assumption?
			    // if the filesize is zero, it doesn't
			    // take up any virtual space...
			    // (Hopefully this only excludes PAGEZERO.)
			    // Also, ignore linkedit segment when
			    // computing size, because we will erase
			    // the linkedit segment later.
			    if(strncmp(scp->segname, SEG_LINKEDIT,
					    sizeof(scp->segname)) != 0)
				vmsize += scp->vmsize;
			    vmaddr = min(vmaddr, addr);
			    
			    // Zero any space at the end of the segment.
			    bzero((char *)(addr + scp->filesize),
				scp->vmsize - scp->filesize);
				
			    // FIXME:  check to see if we overflow
			    // the available space (should be passed in
			    // as the size argument).
			    
#if 0
	if (ram_debug_sarld) {
		printf("In LC_SEGMENT\n");
		printf("LS;file_off %x; fos %x; fsize %x ; addr %x \n",scp->fileoff, file_offset,scp->filesize, addr);
		sleep(1);
	}
#endif 1
			    b_lseek(io, scp->fileoff + file_offset, 0);
			    if (xread(io, (char *)addr, scp->filesize)
							!= scp->filesize) {
				error("Error loading section\n");
				goto shread;
			    }
			}
			break;
		    
		case LC_THREAD:
		case LC_UNIXTHREAD:
			th = (struct xxx_thread_command *)cp;
			entry = th->state.eip;

			break;
		}
		cp += lcp->cmdsize;
	}

	kernBootStruct->rootdev = (dev & 0xffffff00) | devMajor[Dev(dev)];

	free((char *)cmds);
	*rentry = (entry_t)( (int) entry & 0x3fffffff );
	*rsize = vmsize;
	*raddr = (char *)vmaddr;
#if 0
		printf("suceesful load;vmaddr=%x; vmsize=%x;entry=%x\n", vmaddr, vmsize,entry);
		sleep(1);
#endif 1
	return 0;

shread:
	free((char *)cmds);
	error("Read error\n");
	return -1;
}




int
loadStandaloneLinker(
    char *linkerPath,
    sa_rld_t **rld_entry_p
)
{
	int fd, size, ret;
	char *addr;

#if 1
	ram_debug_sarld	= 0;
#endif 1
	if ((fd = openfile(linkerPath, 0)) >= 0) {	    
	    verbose("Loading %s\n",linkerPath);
	    addr = 0;
	    // FIXME:  need to see if it overflows available space
	    ret = loadprog(kernBootStruct->kernDev, fd, 0,
		    (entry_t *)rld_entry_p, &addr, &size);
	    close(fd);
	    if (ret != 0) {
		error("Error in standalone linker executable\n");
		goto linker_error;
	    }
#if	0
		printf("load linker ok\n");
		sleep(2);
#endif	1

	} else {
linker_error:
	    error("Error loading %s\n", linkerPath);
	    *rld_entry_p = 0;
	    return -1;
	}
#if 1
	ram_debug_sarld	= 0;
#endif 1
	return 0;
}

void
removeLinkEditSegment(struct mach_header *mhp)
{
	struct segment_command *sgp;

	sgp = getsegbynamefromheader(mhp, SEG_LINKEDIT);
	if (sgp) {
	    sgp->cmd = 0;	/* destroy segment */
	}
}

/* returns a file descriptor for the driver's
 * relocatable file if present, or returns -1.
 * 'name' is the driver name, e.g. "SerialPort".
 */
 
int
openDriverReloc(
    struct driver_load_data *data
)
{
    int fd;
    char *buf = malloc(256);
    char *name = data->name;
    
    // Check for name_reloc in config dir.
    data->compressed = NO;
    sprintf(buf, ARCH_DEVICES "/%s.config/%s_reloc", name, name);
    if ((fd = open(buf,0)) < 0) {
	strcat(buf, RCZ_EXTENSION);
	if ((fd = open(buf, 0)) >= 0) {
	    data->compressed = YES;
	} else {
	    sprintf(buf, USR_DEVICES "/%s.config/%s_reloc", name, name);
	    if ((fd = open(buf,0)) < 0) {
		strcat(buf, RCZ_EXTENSION);
		fd = open(buf, 0);
		data->compressed = YES;
	    }
	}
    }
    free(buf);
    data->fd = fd;
    data->buf = 0;
    return fd;
}

int
loadDriver(
    struct driver_load_data *data
)
{
    char *vaddr;
    int vsize;
    int length;
    int ret = 0;
    
    if (data->compressed) {
	length = rcz_file_size(data->fd);
	if (length < 0) {
	    ret = -1; goto out;
	}
    } else {
	length = file_size(data->fd);
    }
    vsize = (KERNEL_ADDR + KERNEL_LEN) - (kernBootStruct->kaddr + kernBootStruct->ksize);
    if (length > vsize) {
	error("Driver %s is larger than %d bytes and can't be loaded.\n",
	    data->name, vsize);
	ret = -1; goto out;
    }
    data->buf = vaddr = (char *)((KERNEL_ADDR + KERNEL_LEN) - length - 1);
    if (data->compressed) {
	vsize = rcz_decompress_file(data->fd, vaddr);
    } else {
	vsize = read(data->fd, vaddr, vsize);
    }
    if (vsize < 0) {
	error("Error occurred while loading driver %s", data->name);
	ret = -1; goto out;
    }
    data->len = vsize;
out:
    close(data->fd);
    return ret;
}

int
linkDriver(
    struct driver_load_data *data
)
{
    char *vaddr, *daddr, *buf;
    unsigned int vsize, dsize;
    sa_rld_t *rld_entry = (sa_rld_t *)kernBootStruct->rld_entry;
    int ret = -1;
    
#if	0
		printf("in link driver\n");
		sleep(2);
#endif	1
   if (rld_entry == 0) {
	error("Can't link driver %s without sarld\n",data->name);
	return -1;
    }
    vaddr = data->buf;
    vsize = data->len;
    daddr = (char *)(kernBootStruct->kaddr + kernBootStruct->ksize);
    // remaining memory available for driver
    dsize = (KERNEL_ADDR + KERNEL_LEN) - (unsigned int)daddr;

    if (vsize > 0) {
	driver_config_t *dcp;
	
	/* The area we are using to write the linked drivers
	 * was zeroed after loading the kernel.  This ensures
	 * that the BSS area of the linked driver will be zero filled.
	 */
	buf = malloc(256);
	buf[0] = '\0';
#if	0
		printf("calling  (*rld_entry) and it is %x\n",*rld_entry);
		printf("args are mach_kernel; %s\n %x \n %x ; %x \n%x ; %x \n",
			data->name, kernBootStruct->kaddr,vaddr, vsize,
			daddr, dsize);
		sleep(10);
#endif	1
#if 1 
		//{ unsigned int ram_dummy=0;
		//ram_dummy = *(unsigned int *)rld_entry;
		//printf("rld_entry val is %x\n", ram_dummy);
		//sleep(5);
		//}
	ret = (*rld_entry)("mach_kernel",
	    (struct mach_header *)kernBootStruct->kaddr,
	    data->name, vaddr, vsize,
	    daddr, &dsize,
	    buf, 256,
	    (char *)RLD_MEM_ADDR,
	    RLD_MEM_LEN);
#else
		ret=0;
		{ unsigned int ram_dummy=0;
		ram_dummy = *(unsigned int *)rld_entry;
		printf("rld_entry val is %x\n", ram_dummy);
		sleep(5);
		}
#endif
	if (ret == 1) {
#if	0
		printf("call (*rld_entry) ok\n");
		sleep(1);
#endif	1
	    dcp = &kernBootStruct->driverConfig[
		kernBootStruct->numBootDrivers++];
	    dcp->address = daddr;
	    dcp->size = dsize;
	    kernBootStruct->ksize += dsize;
	    ret = 0;
	} else {
#if	0
		printf("call (*rld_entry) nok\n");
		printf("error buf is %s\n",buf);
		sleep(2);
#endif	1
	    error("Error occurred while linking driver %s:\n%s", data->name, buf);
	    ret = -1;
	}
	free(buf);
    }
    return ret;
}

