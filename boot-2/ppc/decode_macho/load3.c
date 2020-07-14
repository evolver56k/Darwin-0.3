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

#include <mach-o/reloc.h>
#include <mach-o/ppc/reloc.h>

static int skip(int fd, int len);
#define DEBUG printf

load_reloc(char *filename, int offset,int n)
{
    int fd;
    struct relocation_info reloc;

    fd = open(filename,O_RDONLY);
    skip(fd,offset);

    while (n)
    {
        read(fd,&reloc,sizeof(struct relocation_info));
        fix_reloc(reloc);
        n--;
    }
    close(fd);
}

fix_reloc(struct relocation_info reloc)
{
    DEBUG("\nreloc: address: 0x%x\n",reloc.r_address);
    DEBUG("reloc: symbolnum: %d\n",reloc.r_symbolnum);
    DEBUG("reloc: pcrel: %d\n",reloc.r_pcrel);
    DEBUG("reloc: length: %d\n",reloc.r_length);
    DEBUG("reloc: extern: %d\n",reloc.r_extern);
    DEBUG("reloc: type: %d\n",reloc.r_type);

    if (reloc.r_type == PPC_RELOC_VANILLA)
        DEBUG("reloc: generic entry\n");
    if (reloc.r_type == PPC_RELOC_BR14)
        DEBUG("reloc: 14bit branch\n");
    if (reloc.r_type == PPC_RELOC_BR24)
        DEBUG("reloc: 24bit branch\n");

    if (reloc.r_type == PPC_RELOC_PAIR)
        DEBUG("reloc: second entry\n");
    if (reloc.r_type == PPC_RELOC_HI16)
        DEBUG("reloc: high half\n");
    if (reloc.r_type == PPC_RELOC_LO16)
        DEBUG("reloc: low half\n");
    if (reloc.r_type == PPC_RELOC_HA16)
         DEBUG("reloc: high half, low extend\n");
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
