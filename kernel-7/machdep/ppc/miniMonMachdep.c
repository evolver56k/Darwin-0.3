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
 * Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 1994 NeXT Computer, Inc.  All rights reserved.
 *
 * machdep/ppc/miniMonMachdep.c
 *
 * Machine-dependent code for Remote Debugging Protocol
 *
 * March, 1997	Created.	Umesh Vaishampayan [umeshv@NeXT.com]
 *
 */

#include <sys/reboot.h>

#include <mach/vm_param.h>

#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>

#include <kern/miniMonPrivate.h>


static boolean_t ishex(char c)
{
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
	(c >= 'A' && c <= 'F'))
	    return TRUE;
    return FALSE;
}

static int xx(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

static void
xtoi(char *str, unsigned *nump)
{
    for (*nump = 0; *str && ishex(*str); str++)
        *nump = 16*(*nump) + xx(*str);
}

static unsigned found_valu = 0;
static unsigned prev_valu = 0;

static boolean_t
miniMonFind(char *line)
{
    static unsigned *start = (unsigned *)0;
    static unsigned howmany = 0;
    static unsigned prev_howmany = 0;
    struct phys_entry *pp;
    struct mapping *mp;
    static unsigned found = 0;
    extern unsigned *find_phys();
    extern vm_size_t mem_size;
    extern vm_offset_t virtual_avail;

    /* Skip command name */
    while (*line && (*line != ' ' && *line != '\t'))
	line++;
    /* Skip whitespace */
    while (*line && (*line == ' ' || *line == '\t'))
	line++;

    xtoi(line, &found_valu);

    /* Skip found_valu */
    while (*line && (*line != ' ' && *line != '\t'))
	line++;
    /* Skip whitespace */
    while (*line && (*line == ' ' || *line == '\t'))
	line++;

    xtoi(line, &howmany);

    if (found_valu == 0 && howmany == 0) {
	found_valu = prev_valu;
	howmany = prev_howmany;
    } else {
    	start = (unsigned *)0;
	prev_valu = found_valu;
	prev_howmany = howmany;
    	found = 0;
    }

    if (howmany == 0)
	howmany = (unsigned)(-1);

    for (; howmany--; start++) {
	start = find_phys(found_valu, start, mem_size);
	if (start == (unsigned *)0)
	    break;
	if (start == &found_valu || start == &prev_valu)
	    continue;
        safe_prf("P: %08x V:", start);
	found++;
	if (start < (unsigned *)virtual_avail)
	    safe_prf(" 0/%08x", start);
	else if ((pp = pmap_find_physentry((vm_offset_t)start))) {
	    queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link)
            {
		safe_prf(" %x/%08x ", mp->pmap->space,
			 (mp->vm_info.bits.page << PPC_PGSHIFT) |
			 ((unsigned)start & (PPC_PGBYTES - 1)));
	    }
	}
        safe_prf("\n");
    }
    safe_prf("%d found.\n", found);
    return TRUE;
}

static boolean_t
miniMonDump(char *line)
{
    int addr, count;
    static unsigned int *ptr = (unsigned int *)0x10000;
    static unsigned int old_count = 8;
    int text;

    text = ((*line) == 't');

    /* Skip command name */
    while (*line && (*line != ' ' && *line != '\t'))
	line++;
    /* Skip whitespace */
    while (*line && (*line == ' ' || *line == '\t'))
	line++;

    xtoi(line, &addr);
    if (addr != 0)
	ptr = (unsigned int *)addr;

    /* Skip address */
    while (*line && (*line != ' ' && *line != '\t'))
	line++;
    /* Skip whitespace */
    while (*line && (*line == ' ' || *line == '\t'))
	line++;

    if (*line == 's') {
        safe_prf("%08x (as string) : %s\n", ptr, (char *) ptr);
	return TRUE;
    }

    xtoi(line, &count);
    if (count == 0)
	count = old_count;
    else
	old_count = count;
    if (count > 1024) count = 1024;
    safe_prf("%08x : ", ptr);
    while (count > 0)
	if( text) {
	   miniMonPutchar(*((char *)ptr)++);
	   count--;
	} else {
	   safe_prf("%08x ",*ptr++);
	   count -= 4;
	}
    safe_prf("\n");
    return TRUE;
}

miniMonCommand_t miniMonMDCommands[] =
{
    {"dump", miniMonDump, "Dump memory"},
    {"text", miniMonDump, "Dump text"},
    {"find", miniMonFind, "Find value in physical memory"},
    {0,0,0}
};


/*
 * Reboot without sync.
 */
boolean_t
miniMonReboot(char *line)
{
  boot(RB_BOOT, RB_AUTOBOOT|RB_NOSYNC, "");
  return FALSE;
}

/*
 * Halt with sync.
 */
boolean_t
miniMonHalt(char *line)
{
  reboot_mach(RB_HALT);
  return FALSE;
}

boolean_t
miniMonGdb(char *line)
{
    call_kdp();
  return FALSE;
}

int
miniMonTryGetchar(void)
{
  return kmtrygetc();
}

int
miniMonGetchar(void)
{
  register int c;
  do {
    c = kmtrygetc();
  } while (c == NO_CHAR);
  switch (c) {
  case '\177':
    c = '\b';
    break;
  case '\r':
    kmputc(0,'\r');
    c = '\n';
    break;
  case 'u'&037:
    c = '\025';
    break;
  default:
    break;
  }
  kmputc(0,c);
  return c;
}

void
miniMonPutchar(int ch)
{
    kmputc(0,ch);
};

