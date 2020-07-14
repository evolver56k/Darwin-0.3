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

#define DEBUG  printf

/* forward declaractions */
static int decode_header(struct mach_header header);
static int decode_lcmds(char *filename, int fd, int ncmds);
static int decode_fvmlib(int fd, int len);
static int decode_symtab(char *filename, int fd, int offset);
static int skip(int fd, int len);
static int readsyms(char *filename, int symoff,int nsysm,int stroff,int strsize);

int load_file(char *filename)
{
    int fd;
    struct mach_header header;

    fd = open(filename,O_RDONLY);
    if (fd == -1) return;
    read(fd,&header,sizeof(struct mach_header));
    decode_header(header);
    decode_lcmds(filename,fd,header.ncmds);
    close(fd);
}

static int decode_header(struct mach_header header)
{
    if (header.magic == MH_MAGIC)
        DEBUG("decode_header: correct magic %x\n",header.magic);
    else
        DEBUG("decode_header: incorrect magic %x\n",header.magic);

    switch(header.cputype) {
        case CPU_TYPE_VAX:
            DEBUG("decode_header: cpu type VAX\n");
        break;
        case CPU_TYPE_MC680x0:
            DEBUG("decode_header: cpu type 68k\n");
        break;
        case CPU_TYPE_I386:
            DEBUG("decode_header: cpu type x386\n");
        break;
        case CPU_TYPE_MC98000:
            DEBUG("decode_header: cpu type PPC\n");
        break;
        case CPU_TYPE_HPPA:
            DEBUG("decode_header: cpu type PA\n");
        break;
        case CPU_TYPE_MC88000:
            DEBUG("decode_header: cpu type 88k\n");
        break;
        case CPU_TYPE_SPARC:
            DEBUG("decode_header: cpu type SPARC\n");
        break;
        case CPU_TYPE_I860:
            DEBUG("decode_header: cpu type I860\n");
        break;
        case CPU_TYPE_POWERPC:
            DEBUG("decode_header: cpu type PowerPC\n");
        break;
        default:
            DEBUG("decode_header: unknown cpu type\n");
        break;
    }

    DEBUG("decode_header: cpusubtype %x\n",header.cpusubtype);

    switch(header.filetype) {
        case MH_OBJECT:
           DEBUG("decode_header: relocatable object file\n");
        break;
        case MH_EXECUTE:
           DEBUG("decode_header: demand paged executable file\n");
        break;
        case MH_FVMLIB:
           DEBUG("decode_header: fixed VM shared library file\n");
        break;
        case MH_CORE:
           DEBUG("decode_header: core file\n");
        break;
        case MH_PRELOAD:
           DEBUG("decode_header: preloaded executable file\n");
        break;
        case MH_DYLIB:
           DEBUG("decode_header: dynamically bound shared library file\n");
        break;
        case MH_DYLINKER:
           DEBUG("decode_header: dynamic link editor\n");
        break;
        case MH_BUNDLE:
           DEBUG("decode_header: dynamically bound bundle file\n");
        break;
        default:
           DEBUG("decode_header: unknown filetype\n");
        break;
    }
 
    DEBUG("decode_header: ncmds %x\n",header.ncmds);
    DEBUG("decode_header: sizeofcmds %x\n",header.sizeofcmds);

    if (header.flags & MH_NOUNDEFS)
        DEBUG("decode_header: no undefined references\n");

    if (header.flags & MH_INCRLINK)
        DEBUG("decode_header: output of an incremental link\n");

    if (header.flags & MH_DYLDLINK)
        DEBUG("decode_header: input for the dynamic linker\n");
/*
    if (header.flags & MH_BINDATLOAD)
        DEBUG("decode_header: undefines are bound at load\n");

    if (header.flags & MH_PREBOUND)
        DEBUG("decode_header: undefines are prebound\n");
*/
}

static int decode_lcmds(char *filename, int fd, int ncmds)
{
    int i;
    struct load_command cmd;

    DEBUG("\ndecode_lcmds: ncmds %d\n",ncmds);
    for (i = 0 ; i < ncmds ; i++) {

        read(fd,&cmd,sizeof(struct load_command));

        DEBUG("\ndecode_lcmds: no %d cmd %d\n",i,cmd.cmd);
        DEBUG("decode_lcmds: no %d len %d\n",i,cmd.cmdsize);

        switch(cmd.cmd) {
            case LC_SEGMENT:
                DEBUG("decode_lcmds: LC_SEGMENT: segment of this file to be mapped\n");
                load_segment(filename,fd,sizeof(struct load_command));
            break;
            case LC_SYMTAB:
                DEBUG("decode_lcmds: LC_SYMTAB: link-edit stab symbol table info \n");
                decode_symtab(filename,fd,sizeof(struct load_command));
            break;
            case LC_SYMSEG:
                DEBUG("decode_lcmds: LC_SYMSEG: link-edit gdb symbol info (obsolete)\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_THREAD:
                DEBUG("decode_lcmds: LC_THREAD: thread\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_UNIXTHREAD:
                DEBUG("decode_lcmds: LC_UNIXTHREAD: unix thread (includes stack)\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_LOADFVMLIB:
                DEBUG("decode_lcmds: LC_LOADFVMLIB: load a specified fixed VM shared lib\n");
                decode_fvmlib(fd,cmd.cmdsize);
            break;
            case LC_IDFVMLIB:
                DEBUG("decode_lcmds: LC_IDFVMLIB: fixed VM shared library id\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_IDENT:
                DEBUG("decode_lcmds: LC_IDENT: object identification info (obsolete)\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_FVMFILE:
                DEBUG("decode_lcmds: LC_FVMFILE: fixed VM file inclusion\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_PREPAGE:
                DEBUG("decode_lcmds: LC_PREPAGE: prepage command\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_DYSYMTAB:
                DEBUG("decode_lcmds: LC_DYSYMTAB: dynamic link-edit symbol table info\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_LOAD_DYLIB:
                DEBUG("decode_lcmds: LC_LOAD_DYLIB: load dynamically linked shared lib\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_ID_DYLIB:
                DEBUG("decode_lcmds: LC_ID_DYLIB: dynamically linked shared lib id\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_LOAD_DYLINKER:
                DEBUG("decode_lcmds: LC_LOAD_DYLINKER: load dynamic linker\n");
                skip(fd,cmd.cmdsize-8);
            break;
            case LC_ID_DYLINKER:
                DEBUG("decode_lcmds: LC_ID_DYLINKER: dynamic linker id\n");
                skip(fd,cmd.cmdsize-8);
            break;
        }
    }
}


static int decode_fvmlib(int fd, int len)
{
    struct fvmlib fvm;
    char buf[80];
    int left;

    read(fd,&fvm,sizeof(struct fvmlib));
    left = len - sizeof(struct fvmlib) - 8;
    if (left > 0) read(fd,buf,left);

    DEBUG("decode_lcmds: fvmlib minor version = %d\n",fvm.minor_version);
    DEBUG("decode_lcmds: fvmlib name = %d\n",fvm.header_addr);
}

static int decode_symtab(char *filename, int fd, int offset)
{
    struct symtab_command symtab;
    char *tmp;

    tmp = (char *)&symtab + offset;
    read(fd,tmp,sizeof(struct symtab_command) - offset);

    DEBUG("decode_lcmds: symtab symoff = %d\n",symtab.symoff);
    DEBUG("decode_lcmds: symtab nsyms = %d\n",symtab.nsyms);
    DEBUG("decode_lcmds: symtab stroff = %d\n",symtab.stroff);
    DEBUG("decode_lcmds: symtab strsize = %d\n",symtab.strsize);

    readsyms(filename,symtab.symoff,symtab.nsyms,symtab.stroff,symtab.strsize);
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

static int readsyms(char *filename, int symoff,int nsysm,int stroff,int strsize)
{
    struct nlist nl;
    int fd, i;
    char *buf;

    fd = open(filename,O_RDONLY);
    skip(fd,stroff);
    buf = (char *) malloc(strsize);
    read(fd,buf,strsize);
    close(fd);

    fd = open(filename,O_RDONLY);
    skip(fd,symoff);

    for (i = 0 ; i < nsysm ; i++)
    {
        read(fd,&nl,sizeof(struct nlist));
        load_sym(&buf[(int)nl.n_un.n_name],i,nl);
    }
    free(buf);
    close(fd);
}
