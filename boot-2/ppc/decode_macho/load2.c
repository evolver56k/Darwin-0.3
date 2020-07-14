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
#include <mach/machine.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <libc.h>

#define DEBUG printf
static int skip(int fd, int len);

load_segment(char *filename, int fd, int offset)
{
    struct segment_command seg;
    struct section sect;
    char *tmp;
    int i;

    tmp = (char *)&seg + offset;
    read(fd,tmp,sizeof(struct segment_command) - offset);

    DEBUG("load_segment: segname = %s\n",seg.segname);
    DEBUG("load_segment: vmaddr = 0x%x\n",seg.vmaddr);
    DEBUG("load_segment: vmsize = 0x%x\n",seg.vmsize);
    DEBUG("load_segment: fileoff = %d\n",seg.fileoff);
    DEBUG("load_segment: filesize = %d\n",seg.filesize);
    DEBUG("load_segment: nsects = %d\n",seg.nsects);
    DEBUG("load_segment: flags = %d\n",seg.flags);

    if (seg.nsects == 0) return;

    for (i = 0 ; i < seg.nsects ; i++)
    {
        read(fd,&sect,sizeof(struct section));
        load_sect(filename,sect);
    }
}

load_sect(char *filename,struct section sect)
{
    DEBUG("\nload_sect: sect sectname = %s\n",sect.sectname);
    DEBUG("load_sect: sect segname = %s\n",sect.segname);
    DEBUG("load_sect: sect addr = 0x%x\n",sect.addr);
    DEBUG("load_sect: sect size = 0x%x\n",sect.size);
    DEBUG("load_sect: sect offset = 0x%x\n",sect.offset);
    DEBUG("load_sect: sect align = 0x%x\n",sect.align);
    DEBUG("load_sect: sect reloff = 0x%x\n",sect.reloff);
    DEBUG("load_sect: sect nreloc = 0x%x\n",sect.nreloc);
    DEBUG("load_sect: sect flags = 0x%x\n",sect.flags);
    DEBUG("load_sect: sect reserved1 = 0x%x\n",sect.reserved1);
    DEBUG("load_sect: sect reserved2 = 0x%x\n",sect.reserved2);

    load_reloc(filename,sect.reloff,sect.nreloc);
    {
        int fd,cc,i = 0;
        unsigned int n;

        fd = open(filename,O_RDONLY);
        skip(fd,sect.offset);
        cc = sect.size;

        while (cc > 0)
        {
            read(fd,&n,4); 
            if (sect.flags == 1) n = 0;
            printf("%4d %4x: %x\n",i,i,n);
            cc -= 4;
            i += 4;
        }
        close(fd);
    }
}

load_sym(char *name, int n, struct nlist nl)
{
    DEBUG("load_sym: %d name: %s; ",n,name);
    DEBUG("n_value: %ld 0x%x; ",nl.n_value,nl.n_value);
    DEBUG("n_type: %d; ",nl.n_type);
    DEBUG("n_sect: %d; ",nl.n_sect);
    DEBUG("n_desc: %d; ",nl.n_desc);
    DEBUG("\nload_sym: ");

    if (nl.n_type == N_UNDF) DEBUG("N_UNDF: undefined; ");
    if ((nl.n_type&N_ABS) == N_ABS) DEBUG("N_ABS: absolute; ");
    if ((nl.n_type&N_SECT) == N_SECT) DEBUG("N_SECT: def in sect; ");
    //if (nl.n_type == N_PBUD) DEBUG("N_PBUD: prebound\n");
    if ((nl.n_type&N_INDR) == N_INDR) DEBUG("N_INDR: indirect; ");
    if ((nl.n_type&N_STAB) == N_STAB) DEBUG("N_STAB: symbolic debug; ");
    if ((nl.n_type&N_PEXT) == N_PEXT) DEBUG("N_PEXT: private extern; ");
    if ((nl.n_type&N_EXT) == N_EXT) DEBUG("N_EXT: extern; ");
    DEBUG("\n\n");
}

static int skip(int fd, int len)
{
    char buf[1024];
    int ret,cc;

    while (len) {
        cc = (len > 1024) ? 1024 : len;
        ret = read(fd,buf,cc);
        len -= ret;
    }
}

