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


/* test of standalone rld
 * usage: rldtest <sarld> <kernel> <driver>
 *
 */

#include <stdio.h>
#include <streams/streams.h>
#include <mach-o/loader.h>

extern int errno;

typedef unsigned int entry_t;
struct mach_header head;

#define zalloc(a) malloc(a)
#define zfree(a) free(a)
#define xread read
#define b_lseek lseek

#define min(a,b) ((a) < (b) ? (a) : (b))

extern void *malloc(int size);
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
	int ncmds, cc;
	unsigned int cmds, cp;
	struct xxx_thread_command {
		unsigned long	cmd;
		unsigned long	cmdsize;
		unsigned long	flavor;
		unsigned long	count;
		// i386_thread_state_t state;
	} *th;
	unsigned int	entry;
	int size, vmsize = 0;
	unsigned int vmaddr = ~0;

	// XXX should check cputype
	printf("loadmacho: errno=%d\n",errno);
	if ((cmds = (unsigned int) zalloc(head->sizeofcmds)) == 0)
	{
		printf("zalloc: %s bytes: failed\n",head->sizeofcmds);
	} else {
		printf("zalloc: 0x%x\n",cmds);
	}

	cc = b_lseek(io, (off_t)(sizeof (struct mach_header) + file_offset), 0);
	printf("lseek=%d errno=%d file_offset=%d\n",cc,errno,file_offset);

	if ((cc = read(io, (char *)cmds, head->sizeofcmds)) != head->sizeofcmds) {
		printf("error reading commands: ret=%d errno=%d\n",cc,errno);
		printf("error reading commands: len=%d \n",head->sizeofcmds);
		printf("error reading commands: cmd=%d io=%d\n",cmds,io);
	    goto shread;
	}
    
	printf("num of cmds %d, sizeof %d \n",head->ncmds,head->sizeofcmds);
	for (ncmds = head->ncmds, cp = cmds; ncmds > 0; ncmds--)
	{
		unsigned int	*addr;
		extern char *mem;

//		putchar('.');

#define lcp	((struct load_command *)cp)		
		switch (lcp->cmd)
		{
	    
		case LC_SEGMENT:
#define scp	((struct segment_command *)cp)

			printf("LC_SEGMENT: %s\n",scp->segname);
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
			    
			    // Zero any space at the end of the segment.
			    bzero((char *)(addr + scp->filesize),
				scp->vmsize - scp->filesize);
				
			    // FIXME:  check to see if we overflow
			    // the available space (should be passed in
			    // as the size argument).
			    
			    cc = b_lseek(io, (off_t)(scp->fileoff + file_offset), 0);
			    printf("lseek ret=0x%x errno=%d\n",cc,errno);
			    printf("read addr=0x%x size=0x%x %d\n",addr,scp->filesize,scp->filesize);
				if ((addr < mem) || ((addr + scp->filesize) > (mem + 16 * 1024 * 1024)))
				{
					printf("read %x not in malloc buf %x\n",addr,mem);
					break;
				}
			    vmaddr = min(vmaddr, addr);
			    if ((cc = xread(io, (char *)addr, scp->filesize))
							!= scp->filesize) {
					printf("Error loading section, ret=0x%x errno=%d\n",cc,errno);
					printf("File size =0x%x; fileoff_set=0x%x; addr=0x%x\n",
						scp->filesize, scp->fileoff, addr);
					goto shread;
			    }
			    printf("LC_SEGMENT: %x %x\n",addr,(unsigned int *)*addr);
			}
			break;
		    
		case LC_THREAD:
		case LC_UNIXTHREAD:
			printf("LC_THREAD: %s\n",scp->segname);
			th = (struct xxx_thread_command *)cp;
			// entry = th->state.eip;
			entry = 0x0070d1cc;
			break;
		}
	    
		cp += lcp->cmdsize;
	}

//	kernBootStruct->rootdev = (dev & 0xffffff00) | devMajor[Dev(dev)];

	printf("zfree: 0x%x\n",cmds);
	zfree((char *)cmds);
	*rentry = (entry_t)( (int) entry & 0x3fffffff );
	*rsize = vmsize;
	*raddr = (char *)vmaddr;
	return 0;

shread:
	printf("zfree: 0x%x\n",cmds);
	zfree((char *)cmds);
	printf("Read error\n");
	*rentry = (entry_t)( (int) entry & 0x3fffffff );
	*rsize = vmsize;
	*raddr = (char *)vmaddr;
	return -1;
}

loadprog(
	int		dev,
	int		fd,
	entry_t		*entry,		// entry point
	char		**addr,		// load address
	int		*size		// size of loaded program
)
{
    /* get file header */
    read(fd, &head, sizeof(head));

    if (head.magic == MH_MAGIC) {
	return loadmacho((struct mach_header *) &head,
		dev, fd, entry, addr, size,0);
    }

    else if (head.magic == 0xbebafeca)
    {
	printf("no fat binary support yet\n");
	return -1;
    }

    printf("unrecognized binary format\n");
    return -1;
}

#define DRIVER "/usr/Devices/EtherExpress16.config/EtherExpress16_reloc"

void usage(void)
{
    fprintf(stderr,"usage: rldtest <sarld> <kernel> <driver>\n");
    exit(1);
}

char *mem;

main(int argc, char **argv)
{
    int fd;
    char *workmem, *ebuf;
    char *laddr, *kaddr, *daddr;
    NXStream *str;
    struct mach_header *mh;
    int ret, size, ksize, dsize, count;
    entry_t entry;
    int (*fn)();
    struct section *sp;
    char *kernel, *sarld, *driver;
    
    if (argc != 4)
	usage();
    sarld = argv[1];
    kernel = argv[2];
    driver = argv[3];
    mem = malloc(1024 * 1024 * 16);
    printf("mem = 0x%x errno=%d end=0x%x\n",mem,errno,mem+1024*1024*16);
    laddr = (char *)0x0;
    fd = open(sarld,0);
    if (fd < 0) {
	fprintf(stderr, "couldn't open sarld %s, error %d\n",sarld,errno);
	exit(1);
    }
    printf("fd = %d\n",fd);
    printf("load program: %s\n",sarld);
    loadprog(0, fd, &entry, &laddr, &size);
    close(fd);
    printf("entry = 0x%x, laddr = 0x%x, size = %d\n",entry, laddr, size);
    fn = (int (*)())entry;
    fd = open(kernel,0);
    if (fd < 0) {
	fprintf(stderr, "couldn't open kernel %s, error %d\n",kernel,errno);
	exit(1);
    }
    kaddr = 0;
    printf("load program: %s\n",kernel);
    loadprog(0, fd, &entry, &kaddr, &ksize);
    printf("entry = 0x%x, kaddr = 0x%x, ksize = %d\n",entry, kaddr, ksize);
    close(fd);
    daddr = (char *)0x380000;
    fd = open(driver,0);
    printf("load program: %s\n",driver);
    if (fd < 0) {
	fprintf(stderr, "couldn't open driver %s, error %d\n",driver,errno);
	exit(1);
    }
    size = 0;
    do {
	count = read(fd, daddr, 65536);
	daddr += count;
	size += count;
    } while (count > 0);
    daddr = (char *)0x380000;
    printf("entry = 0x%x, daddr = 0x%x, size = %d\n",entry, daddr, size);
    close(fd);
    workmem = malloc(300 * 1024);
    ebuf = malloc(64 * 1024);
    ebuf[0] = 0;
    dsize = 16 * 1024 * 1024 - (int)kaddr - (int)ksize;
    printf("about to call 0x%x dsize=%d\n",fn,dsize);
    ret = (*fn)( "mach_kernel", kaddr,
	"driver", daddr, size,
	kaddr + ksize, &dsize,
	ebuf, 64 * 1024,
	workmem, 300 * 1024
    );
    if (ret == 1) printf("rld is ok\n");
    printf("rld return: %d '%s' dsize=%d\n",ret, ebuf,dsize);
    printf("rld return: start=0x%x %d\n",kaddr+ksize,kaddr+ksize);
    printf("rld return: kaddr=0x%x ksize=0x%x\n",kaddr,ksize);
	{
		int i;
		int *p = kaddr + ksize;
		for(i = 0 ; i < dsize/4 ; i++) {
		//	printf("0x%x %x\n",i+kaddr+ksize,*p);
			p++;
		}
	}
#define SEG_OBJC "__OBJC"
    sp = getsectbyname ("__TEXT", "__text");
    printf("text section: %s\n",sp->sectname);
    sp = getsectbynamefromheader (kaddr+ksize, "__OBJC", "__module_info");
    printf("objc section: %s\n",sp->sectname);
    sp = getsectdatafromheader (kaddr+ksize,
		    SEG_OBJC, "__module_info", &size);
    printf("objc section: %s\n",sp->sectname);

    printf("getsectdata ret = 0x%x %d\n",sp,sp);
    free(mem);
}
