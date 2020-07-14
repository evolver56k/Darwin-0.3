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
/*	@(#)disk.c	1.0	08/28/87	(c) 1987 NeXT	*/

/* 
 **********************************************************************
 * HISTORY
 * 22-Jun-98 Wilfredo Sanchez at Apple
 *	Change NEXTSTEP to Rhapsody
 *
 * 11-Sep-97 Daniel Wade at Apple
 *	Replaced gets statements with fgets statements.
 *
 * 7-Aug-97  Dieter Siegmund at Apple
 *	Removed direct knowledge of Apple partition maps, fork/exec
 *      pdisk to get information instead (hopefully this will be in a library someday soon).
 *
 * 10-Jun-97 Dieter Siegmund at Apple
 *  	Added Apple partition code.
 *
 * 5-May-97 A Ramesh at Apple
 *  	Modified for intel Rhapsody
 *
 * 2-Apr-94 Bob Vadnais at NeXT
 *  	Added hppa boot support.
 * 
 * 27-Oct-91 Matt Watson at NeXT
 *	Added IDE module
 *
 * 1-Oct-91 Matt Watson at NeXT
 *	Added FDISK capabilities
 *
 * 4-Aug-92 Matt Watson at NeXT
 *	Added 486 specifics
 *	
 * 16-Sep-91  Garth Snyder (gsnyder) at NeXT
 *	Make init() check to be sure the disk isn't mounted before formatting.
 *	Remove dependencies on DEV_BSIZE.
 *
 *  2-Aug-91  Blaine (blaine) at NeXT
 *	drop -T from usage message
 *
 * 13-Jul-90  John Seamons (jks) at NeXT
 *	Remove some debugging code that allowed operating beyond the legal
 *	end of the media.
 *
 * 12-Mar-90  Doug Mitchell at NeXT
 *	Added floppy support.
 *
 *  3-Mar-90  John Seamons (jks) at NeXT
 *	Added "scan" command that searches for superblocks and prints
 *	their locations.  Used to discover what superblock backup number
 *	to give to fsck (i.e. "fsck -bnnn /dev/rod0a").
 *
 *  1-Mar-90  John Seamons (jks) at NeXT
 *	Removed commands which were judged to be of marginal interest to
 *	customers or were only intended for use during internal development.
 *
 * 15-Aug-89  Gregg Kellogg (gk) at NeXT
 *	In main loop, if gets returns NULL disk is aborted.
 *
 * 27-Oct-88  Mike DeMoney (mike) at NeXT
 *	Added support for SCSI disk types that may be configured with
 *	different sector sizes.
 *
 * 16-Mar-88  John Seamons (jks) at NeXT
 *	Cleaned up to support standard disk label definitions.
 *
 *  5-Mar-88  Mike DeMoney (mike) at NeXT
 *	Completed SCSI support.
 *
 * 16-Nov-87  Mike DeMoney (mike) at NeXT
 *	Modified for SCSI support.
 *
 * 28-Aug-87  John Seamons (jks) at NeXT
 *	Created.
 *
 **********************************************************************
 */

#define	A_OUT_COMPAT	1

/*
 *	TODO
 *
 *	- remove hardwired constants -- always use label info
 *	- cmp mode in r/w
 */

#include <mach/mach.h>
#include <stdio.h>
#import <unistd.h>
#import <stdlib.h>
#import <libc.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
//#include <mntent.h>
#include <math.h>
#include <strings.h>
#include <syslog.h>
#include <mach/features.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include <mach-o/loader.h>
#if	A_OUT_COMPAT
#include <machine/exec.h>
#endif	A_OUT_COMPAT
#include <dev/disk.h>
#import <architecture/byte_order.h>
#include "disk.h"
#include <bsd/dev/i386/disk.h>
#if defined(hppa)
#include <bsd/dev/hppa/disk.h>
#endif
#if defined(sparc)
#include <bsd/dev/sparc/disk.h>
#endif


#ifdef i386
#include <machdep/i386/kernBootStruct.h>
#endif

struct	disk_label disk_label;
struct	bad_block bad_block;
struct	disk_req req;
struct	dtype_sw *dsp;
struct	disktab disktab, *dt;
struct	disk_stats stats;
struct 	timeval exec_time;
int	*bit_map, lfd, fd, d_size, devblklen, dosdisk;
unsigned long dosbase, dossize;

/*
 * Default place to find block 0 boot
 * FIXME: maybe this should go in disktab?
 */
 
struct arch_boot_info {
    cpu_type_t arch;
    char *boot;
    int maxsize;
};

#define sizeofA(a) ((sizeof(a)) / (sizeof(a[0])))

struct arch_boot_info arch_boot_table[] = {
    { CPU_TYPE_MC680x0,	"/usr/standalone/boot",			64 * 1024  }, 
    { CPU_TYPE_I386,	"/usr/standalone/i386/boot",		64 * 1024  },
    { CPU_TYPE_HPPA,	"/usr/standalone/hppa/boot",		256 * 1024 },
    { CPU_TYPE_SPARC,	"/usr/standalone/sparc/bootblk",	15 * 512   }
 };

/* The largest MAXBOOTSIZE from arch_boot_table[] */
#define MAXBOOTSIZE (256 * 1024)

#if defined(m68k) || defined(i386) || defined(hppa) || defined(sparc) || defined(ppc)
/* We know about these architechtures. */
#else
#error "Need architecture specific arch_boot_table[] entry."
#endif

#define BOOT0		"/usr/standalone/i386/boot0"
#define BOOT1		"/usr/standalone/i386/boot1"
#define BOOT1F		"/usr/standalone/i386/boot1f"
#define	DISKNAME	"Disk"

/*
 * Device types
 */
#define	TY_SCSI		0x0001
#define	TY_OPTICAL	0x0002
#define	TY_REMOVABLE	0x0004
#define TY_FLOPPY	0x0008
#define	TY_ALL		0xffff
#define TY_IDE		0x0010

#define	TY_OD		(TY_OPTICAL|TY_REMOVABLE)

#ifdef m68k
int	od_conf(), od_init(), od_req(), od_geterr(), od_wlabel(), od_cyl(),
	od_pstats(), od_ecc(), od_glabel(), od_format();
#endif
int	sd_conf(), sd_init(), sd_req(), sd_geterr(), sd_wlabel(), sd_cyl(),
	sd_pstats(), sd_ecc(), sd_glabel(), sd_format(),
	fd_conf(), fd_init(), fd_req(), fd_geterr(), fd_wlabel(), fd_cyl(),
	fd_pstats(), fd_ecc(), fd_glabel(), fd_format(),
	hd_conf(), hd_init(), hd_req(), hd_geterr(), hd_wlabel(), hd_cyl(),
	hd_pstats(), hd_ecc(), hd_glabel(), hd_format();

struct dtype_sw dtypes[] = {
#ifdef m68k
	{
		"removable_rw_optical",	TY_OPTICAL | TY_REMOVABLE,
		od_conf, od_init, od_req, od_geterr, od_wlabel, od_cyl,
		od_pstats, od_ecc, od_glabel, od_format,
		4149, 4149*16, 20000, 1000
	},
#endif
	{
		"fixed_rw_scsi",	TY_SCSI,
		sd_conf, sd_init, sd_req, sd_geterr, sd_wlabel, sd_cyl,
		sd_pstats, sd_ecc, sd_glabel, sd_format,
		0, 0, 2000, 100
	},
	{
		/* SCSI opticals */
		"removable_rw_scsi",	TY_SCSI | TY_REMOVABLE,						
		sd_conf, sd_init, sd_req, sd_geterr, sd_wlabel, sd_cyl,
		sd_pstats, sd_ecc, sd_glabel, sd_format,
		0, 0, 20000, 1000
	},
	{
		"removable_rw_floppy",	TY_FLOPPY  | TY_REMOVABLE,
		fd_conf, fd_init, fd_req, fd_geterr, fd_wlabel, fd_cyl,
		fd_pstats, fd_ecc, fd_glabel, fd_format,
		0, 0, 20000, 1000
	},
	{
		"fixed_rw_ide",	TY_IDE,
		hd_conf, hd_init, hd_req, hd_geterr, hd_wlabel, hd_cyl,
		hd_pstats, hd_ecc, hd_glabel, hd_format,
		0, 0, 20000, 1000
	},
	{
		NULL,			0
	}
};

int	help(), quit(), init(), seek(), Read(), Write(), test(), all(), set(),
	label(), bad(), bitmap(), eject(), tdbug(), rw(), scan(),
	look(), pstats(), zstats(), rwr(), Format(),
	verify(), boot(), tabort(),
	tvers(), newhost(), newlbl(), defboot();

char	*MountPoint(char *dev);

struct cmds {
	int	(*c_func)();
	int	c_typemask;
	char	*c_name;
	char	*c_desc;
} cmds[] = {
	init,	TY_ALL,		"init",		"initialize disk",
	eject,	TY_ALL,		"eject",	"eject disk",
	boot,	TY_ALL,		"boot",		"write boot block(s)",
	label,	TY_ALL,		"label",	"edit label information",
	newhost,TY_ALL,		"host",		"change hostname on label",
	newlbl,	TY_ALL,		"name",		"change disk label name",
	defboot, TY_ALL,	"defaultboot",	"change default bootfile",
	Format, TY_SCSI | TY_IDE | TY_FLOPPY, "Format",  "Format Disk",
	bitmap,	TY_OPTICAL,	"bitmap",	"edit status bitmap",
	bad,	TY_OPTICAL,	"bad",		"edit bad block table",
	scan,	TY_ALL,		"scan",		"scan for superblocks",
	Read,	TY_ALL,		"read",		"read from disk",
	Write,	TY_ALL,		"write",	"write to disk",
	verify,	TY_ALL,		"verify",	"verify data on disk",
	rw,	TY_ALL,		"rw",		"read-after-write",
	rwr,	TY_ALL,		"rwr",		"read-after-write random",
	look,	TY_ALL,		"look",		"look at read/write buffer",
	set,	TY_ALL,		"set",		"set read/write buffer",
	pstats,	TY_OPTICAL,	"stats",	"print drive statistics",
	zstats,	TY_OPTICAL,	"zero",		"zero drive statistics",
	tabort,	TY_ALL,		"abort",	"toggle abort on error mode",
	tvers,	TY_ALL,		"vers",		"toggle label version",
	help,	TY_ALL,		"help",		"print this list",
	help,	TY_ALL,		"?",		"print this list",
	quit,	TY_ALL,		"quit",		"quit program",
};
int ncmds = sizeof (cmds) / sizeof (cmds[0]);

#define	MAXSECT		16
#define	MAXSSIZE	8192	/* Doesn't matter, blksize corrected */ 
#define	TBSIZE		(MAXSECT * MAXSSIZE)
#define	ALIGN		64
#define ri(l, h) \
	((random()/(2147483647/((h)-(l)+1)))+(l))

u_char	test_rbuf[TBSIZE+ALIGN], test_wbuf[TBSIZE+ALIGN],
	cmp_rbuf[TBSIZE+ALIGN];
int	abort_flag = 1, version = DL_VERSION;
int	f_init, f_stat, f_eject, f_test, f_boot, do_boot, do_boot0, do_boot1, f_query, f_bulk, f_newhost, f_kernel, named_boot1;
int	f_label, f_format;
int	interactive, no_prompt, bad_modified, resp;
int	force_blocksize = 1;	/* Coerce devs to at least DEV_BSIZE? */ 
int	repartition = 0;
char	*prog, *fn, *name, line[BUFSIZ], *hostname = 0, *defbootfile = 0,
	*labelname = 0;
char 	dev[100];
char	namebuf[BUFSIZ];
jmp_buf	env;
extern	int errno;
struct	drive_info drive_info, *di = &drive_info;
struct	timeval start, stop;
struct	dtype_sw *drive_type();
extern struct disktab *sd_inferdisktab(int fd, int partsize, int bfdsize);
extern struct disktab *fd_inferdisktab(int fd, char *devname, int init_flag, 
	int density);
extern struct disktab *hd_inferdisktab(int fd, int partsize, int bfdsize);
int 	density;		/* for formatting floppies */
int	partition_size;		/* size requested for first partition */
int	bfd_partition_size;	/* size of boot file domain partition */
int 	no_format;		/* inhibit floppy format */

const char *bootfile = NULL;
const char *bootfile0 = BOOT0;
const char *bootfile1 = BOOT1;

int useAllSectors;
#ifdef i386
int biosAccessibleBlocks;
KERNBOOTSTRUCT kernbootstruct;
#endif

volatile void bomb (int status, char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vprintf (fmt, ap);
	va_end(ap);
	printf ("\n");
	if (interactive)
		longjmp (env, 1);
	else {
		exit (status);
		closelog();
	}
}

/* like strncpy, but guarantees null terminated */
void strnzcpy (char *d, char *s, int len)
{
	strncpy (d, s, len-1);
	d[len-1] = 0;
}

int getrmsg (char *msg)
{
	printf (msg);
	if ( fgets(line, sizeof(line), stdin) == NULL )
		err (1, NULL);
	line[strlen(line)-1] = '\0';
	return ((int) line[0]);
}

int getn() 
{
	char resp[16];

	if ( fgets(resp, sizeof(resp), stdin) == NULL )
		err (1, NULL);
	resp[strlen(resp)-1] = '\0';
	if (resp[0] == 0)
		return (-1);
	return (atoi (resp));
}

int getnrmsg (msg, lo, hi)
	register char *msg;
	register int lo, hi;
{
	register int i;

retry:
	printf (msg);
 	if ((i = getn()) == -1)
		return (-1);
	if (i < lo || i > hi) {
		printf ("must be between %d and %d\n", lo, hi);
		goto retry;
	}
	return i;
}

int confirm (msg)
	register char *msg;
{
	char resp[16];

	if (interactive == 0)
		return (1);
	printf (msg);
	if ( fgets(resp, sizeof(resp), stdin) == NULL )
		err (1, NULL);	
	resp[strlen(resp)-1] = '\0';
	if (resp[0] == 'y')
		return (1);
	return (0);
}

#define	MAX_DEV_BSIZE	(8 * 1024)

int uses_fdisk()
{
	char *bufp, buf[DISK_BLK0SZ+16]; /* for alignment */
	struct disk_blk0 *blk0p;
	struct fdisk_part *fdiskp;
	int i;

	if (devblklen != DISK_BLK0SZ) return 0;
	bufp = (char *)((((int)buf + 15) >> 4) << 4);
        /* If disk is not formatted, don't read block 0. */
        if ((ioctl(fd, DKIOCGFORMAT, &i) < 0) || (i == 0))
            return 0;
	if ((lseek(lfd, 0, L_SET) < 0) || (read(lfd, bufp, DISK_BLK0SZ) < 0))
		bomb(S_NEVER, "Read of sector 0 failed\n", 0, 0, 0, 0);
	blk0p = (struct disk_blk0 *)bufp;
	if (NXSwapLittleShortToHost(blk0p->signature) != DISK_SIGNATURE) 
		return(0);
	fdiskp = (struct fdisk_part *)blk0p->parts;

	for(i=0; i < FDISK_NPART; i++, fdiskp++) {
		if(fdiskp->systid == FDISK_NEXTNAME){
			printf("Rhapsody partition base = %ld\t", dosbase = NXSwapLittleLongToHost(fdiskp->relsect));
			printf("Rhapsody partition size = %ld\n", dossize = NXSwapLittleLongToHost(fdiskp->numsect));
			if (f_init) force_blocksize = 0;
			return(FDISK_NEXTNAME);
		}
	}
	return(0);
}

void do_cmd(cmd)
char *cmd;
{
	register struct cmds *cp;

	for (cp = cmds; cp < &cmds[ncmds]; cp++)
		if (strncmp (cmd, cp->c_name, strlen (cmd)) == 0) {
			if (cp->c_typemask & dsp->ds_typemask) {
				(*cp->c_func)();
				return;
			}
			bomb(S_NEVER, "%s: invalid request for type %s", cmd,
			    dsp->ds_type);
		}
	bomb (S_NEVER, "%s: unknown command -- 'help' lists them", cmd);
}

void sigint(int foo) {
	longjmp (env, 1);
}

static cpu_type_t host_cpu_type;

void init_arch_info(void)
{
    kern_return_t ret;
    struct host_basic_info basic_info;
    unsigned int count = HOST_BASIC_INFO_COUNT;
    
    ret = host_info(host_self(), HOST_BASIC_INFO, 
	(host_info_t)&basic_info, &count);
    if (ret != KERN_SUCCESS)
	bomb(S_NEVER, "host_info() call failed");
 
    host_cpu_type = basic_info.cpu_type;
}

struct arch_boot_info *boot_info_for_cputype(cpu_type_t arch)
{
    int i;
    
    for (i = 0; i < sizeofA(arch_boot_table); i++)
	if (arch_boot_table[i].arch == arch)
	    return (&arch_boot_table[i]);

    return (struct arch_boot_info *) 0;
}

int main (int argc, char *argv[])
{
	char *fp, cmd[64];
	int isSCSI = 0;

	prog = *argv++;
	while (--argc > 0) {
		if (*argv[0] == '-') {
			fp = *argv;
			while (*(++fp)) switch (*fp) {
				case 'i':
					f_init = 1;
					do_boot = 1;
#if defined(i386)
					do_boot1 = 1;
					do_boot0 = 1;
#endif
					no_prompt=1;
					break;
				case 'p':
					if (--argc <= 0)
						goto usage;
					partition_size = atoi(*(++argv));
					repartition = 1;
					break;
				case 'n':
					if (--argc <= 0)
						goto usage;
					bfd_partition_size = atoi(*(++argv));
					repartition = 1;
					break;
				case 't':
					if (--argc <= 0)
						goto usage;
					name = *(++argv);
					break;
				case 'F':
					f_format = 1;
					no_prompt=1;
					break;
				case 'f':
					force_blocksize = 0;
					break;
				case 'd':
					if (--argc <= 0)
						goto usage;
					density = atoi(*(++argv));
					break;
				case 's':
					f_stat = 1;
					no_prompt=1;
					break;
				case 'e':
					f_eject = 1;
					no_prompt=1;
					break;
				case 'T':
					f_test = 1;
					no_prompt=1;
					break;
				case 'b':
					do_boot = 1;
// Only need these (by default) on Intel
#if defined(i386)
					do_boot1 = 1;
					do_boot0 = 1;
#endif
					f_boot = 1;
					no_prompt=1;
					break;
				case 'u':
					useAllSectors = 1;
					break;
				case 'h':
					if (--argc <= 0)
						goto usage;
					hostname = *(++argv);
					break;
				case 'l':
					if (--argc <= 0)
						goto usage;
					labelname = *(++argv);
					break;
				case 'k':
					if (--argc <= 0)
						goto usage;
					defbootfile = *(++argv);
					f_kernel = 1;
					no_prompt=1;
					break;
				case 'q':
					f_query = 1;
					no_prompt=1;
					break;
				case 'B':
					if (--argc <= 0)
						goto usage;
					switch (*(++fp)) {
					case '0':
						do_boot0 = 1;
						bootfile0 = *(++argv);
						break;
					case '1':
						do_boot1 = 1;
						named_boot1 = 1;
						bootfile1 = *(++argv);
						break;
					case '\0':
						fp--;
						do_boot = 1;
						bootfile = *(++argv);
						break;
					default:
						printf ("bad flag: -B%c\n", *fp);
						goto usage;
					}
					f_boot = 1;
					no_prompt=1;
					break;
				case 'H':
					if (--argc <= 0)
						goto usage;
					hostname = *(++argv);
					f_newhost = 1;
					no_prompt=1;
					break;
				case 'L':
					if (--argc <= 0)
						goto usage;
					labelname = *(++argv);
					f_label = 1;
					no_prompt=1;
					break;
				case 'N':
					no_format = 1;
					break;
				default:
					printf ("bad flag: %c\n", *fp);
					goto usage;
			}
		} else
			fn = *argv;
		argv++;
	}
	if (fn == 0 || *fn == 0 || argc < 0) {
usage:
		printf ("usage: %s [option flags] [action flags] "
			"raw-device\n", prog);
		printf ("option flags:\n");
		printf ("\t-h hostname\tspecify host name\n");
		printf ("\t-l labelname\tspecify label name\n");
		printf ("\t-k kernelname\tspecify default file to boot\n");
		printf ("\t-t disk_type\tspecify disk type name\n");
		printf ("\t-p part_size\tspecify partition \'a\' size\n");
		printf ("\t-n part_size\tspecify bfd partition size\n");
		printf ("\t-d density\tspecify format density (in KBytes)\n");
		printf ("action flags:\n");
		printf ("\t-b\t\twrite boot block\n");
		printf ("\t-e\t\teject disk\n");
		printf ("\t-i\t\tinitialize disk\n");
		printf ("\t-q\t\tquery disk name and print it\n");
		printf ("\t-s\t\tprint disk statistics\n");
		printf ("\t-B[n] bootfile\twrite boot block [#n] from file\n");
		printf ("\t-F\t\tFormat Disk\n");
		printf ("\t-H hostname\tchange host name on label\n");
		printf ("\t-L labelname\tchange disk label name\n");
		printf ("\t-N do not format disk during initialization\n");
		printf ("\t-T\t\trun test patterns\n");
		printf ("\t-f\t\tDon't force blksize to be >= DEV_BSIZE\n");
		printf ("\t-u\t\tUse all sectors, not just those bios-accessible\n");
		printf ("interactive mode if no action flags specified\n");
		exit (-1);
	}
	
	init_arch_info();
	
	dt = &disktab;
	openlog ("disk", LOG_USER, LOG_NOWAIT);
	if ((fd = open (fn, O_RDWR)) < 0)
		if (errno == ENODEV)
			bomb (S_EMPTY, "no disk cartridge inserted in drive");
		else
			dpanic (S_NEVER, fn);

	if(strncmp(fn, "/dev/rhd", 8) == 0)
	{
		fn[strlen(fn)-1] = 'h';
	}
	else if(strncmp(fn, "/dev/rsd", 8) == 0)
	{
		isSCSI = 1;
		fn[strlen(fn)-1] = 'h';
	}
	else useAllSectors = 1;

#ifdef i386
	{
	int ndx = 0, heads, spt, spc, cylinders, kmfd, diskInfo;

	if ((kmfd = open ("/dev/kmem", O_RDONLY)) < 0)
	{
		bomb(-1,"can't get kernel boot structure");
	}

	lseek(kmfd, (off_t)KERNSTRUCT_ADDR, L_SET);
	read(kmfd, (char *)&kernbootstruct, sizeof(KERNBOOTSTRUCT)-CONFIG_SIZE);
	close(kmfd);
	if (kernbootstruct.magicCookie != KERNBOOTMAGIC)
	{
		useAllSectors = 1;
		goto cont;
	}

	// if device is SCSI bios index must be bumped by number of IDE drives
	if (isSCSI) ndx += kernbootstruct.numIDEs;
	ndx += fn[8] - '0';
	if (ndx < 0 || ndx > 3 || (fn[9] >= '0' && fn[9] <= '9'))
	{
		useAllSectors = 1;
		goto cont;
	}

	diskInfo = kernbootstruct.diskInfo[ndx];
	heads = ((diskInfo >> 8) & 0xff) + 1;
	spt = diskInfo & 0xff;
	spc = spt * heads;
	cylinders = (diskInfo >> 16) + 1;

	if (heads == 1 || spt == 0)
	{
		useAllSectors = 1;
		goto cont;
	}

	biosAccessibleBlocks = (cylinders * spc);

cont:;
	}
#endif i386

	if(strncmp(fn, "/dev/rfd", 8) == 0) {
		// If we're writing to a floppy, and we didn't specify an
		// alternate boot file, use the floppy booter
		if (!named_boot1) {
		    bootfile1 = BOOT1F;
		}
		fn[strlen(fn)-1] = 'b';
	}
	if ((lfd = open (fn, O_RDWR)) < 0)
		if (errno == ENODEV)
			bomb (S_EMPTY, "no disk cartridge inserted in drive");
		else
			dpanic (S_NEVER, fn);
	strcpy(dev, fn);	/* save the device filename that was opened */
	fn[strlen(fn)-1] = 0;	/* delete partition letter */

	/*
	 * If all we want to do is to eject the disk, then we should do it
	 * before any furthur testing. 
	 */
	if (f_eject)	{
	    eject();
	    closelog();
	    exit(0);
	}

	if (ioctl (fd, DKIOCINFO, di) < 0)
		dpanic (S_NEVER, "get info");
	devblklen = di->di_devblklen;
	
	if ((devblklen > DEV_BSIZE) && force_blocksize) {
	    printf("NOTE: This device's block size (%d) is too big to "
		"coerce to DEV_BSIZE (%d).\nYou won't be able to use it "
		"with software versions earlier than Release 3.0.\n",
		devblklen, DEV_BSIZE);
	    force_blocksize = 0;
	}


	if (name == 0) {
		if (devblklen) {
			sprintf(namebuf, "%s-%d", di->di_name, devblklen);
			name = namebuf;
		} else
			name = di->di_name;
	}
again:
	if ((dt = getdiskbyname(name)) == 0) {		
		if (name == namebuf) {
			name = di->di_name;
			goto again;
		}
	}

	if (no_prompt && !f_init && !f_eject && !f_format) {
		if (ioctl (lfd, DKIOCGLABEL, &disk_label) < 0) {
			if (errno == ENXIO)
				bomb (S_NEVER, "no label on disk");
			dpanic (S_NEVER, "get label");
		}
		dt = &disk_label.dl_dt;
		dosdisk = uses_fdisk();
		apple_disk = has_apple_partitions();
	}
	/*
	 * No entry in disktab or user wishes to override partition info;
	 * let's see if device-specific code can figure it out...
	 */
	if ((dt == 0) || repartition)
	{
		if(strncmp(fn, "/dev/rsd", 8) == 0) {
			dt = sd_inferdisktab(fd, partition_size,
				bfd_partition_size);
		}
		else if(strncmp(fn, "/dev/rhd", 8) == 0) {
			dt = hd_inferdisktab(fd, partition_size,
				bfd_partition_size);
		}
		else if(strncmp(fn, "/dev/rfd", 8) == 0)
			dt = fd_inferdisktab(fd, fn, 
				(f_init || f_format) && !no_format,
				density);
	}
	if(dt == 0)
		bomb (S_NEVER, "%s: unknown disk name", name);

	if (f_query) {
		printf("%s\n", name);
		exit(0);
	}
	printf ("disk name: %s\n", name);
	
	/* 
	** This warning is no longer appropriate since we support 
	** this stuff now.
	**
	** if (dt->d_secsize != DEV_BSIZE)
	** 	printf("WARNING: disktab sector size (%d)"
	**	    "!= DEV_BSIZE (%d)\n", dt->d_secsize, DEV_BSIZE);
	*/
	
	if ((dt->d_secsize < devblklen) || 
		(devblklen && (dt->d_secsize % devblklen))) {
  		printf("WARNING: device sector size (%d) "
  		    "incompatable with disktab sector size %d\n", devblklen,
  		    dt->d_secsize);
	}
	if (dsp == NULL && (dsp = drive_type(dt->d_type)) == NULL)
		bomb (S_NEVER, "%s: unknown disk type", dt->d_type);
	printf ("disk type: %s\n", dt->d_type);

	if (f_init || f_stat || f_eject || f_test || f_boot ||
	    f_newhost || f_label || f_format || f_kernel) {
		if (f_format)
			Format();
		if (f_init)
			do_cmd("init");
		if (f_boot)
			do_cmd("boot");
		if (f_test)
			do_cmd("test");
		if (f_stat)
			do_cmd("stats");
		if (f_newhost)
			newhost();
		if (f_label)
			newlbl();
		if (f_eject)
			do_cmd("eject");
		if (f_kernel)
			do_cmd("defaultboot");
		closelog();
		exit(0);
	}

	/* interactive mode */
	printf ("Disk utility\n\n");
	interactive = 1;
	signal (SIGINT, sigint);
	while (1) {
		setjmp (env);
		printf ("disk> ");
		if ( fgets(cmd, sizeof(cmd), stdin) == NULL )
			quit();
		cmd[strlen(cmd)-1] = '\0';
		if (cmd[0] == 0)
			continue;
		do_cmd(cmd);
	}
}


int help() {
	register struct cmds *cp;

	printf ("commands are:\n");
	for (cp = cmds; cp < &cmds[ncmds]; cp++)
		if (cp->c_typemask & dsp->ds_typemask)
			printf ("\t%-8s%s\n", cp->c_name, cp->c_desc);
	return 0;
}

int scan()
{
	int blk = 0;
	char buf[SBSIZE];
	struct fs *fs = (struct fs*) buf;
	
	printf ("Backup superblocks at:\n");
	lseek (fd, 0, L_SET);
	while (1) {
		if (read (fd, buf, sizeof buf) < 0 && errno != EIO)
			break;
		if (fs->fs_magic == NXSwapBigLongToHost(FS_MAGIC)) {
			printf ("%d ", (blk * SBSIZE) / dt->d_secsize);
			fflush (stdout);
		}
		blk++;
	}
	return 0;
}

void make_new_label()
{
	struct disk_label *l = &disk_label;
	
	bzero (l, sizeof (struct disk_label));
	l->dl_version = version;
	l->dl_dt = *dt;
	if (hostname) {
		strnzcpy(l->dl_hostname, hostname, MAXHNLEN);
	} else {
		if (interactive) {
			getrmsg("enter host name: ");
			strnzcpy(l->dl_hostname, line, MAXHNLEN);
		} else
			strnzcpy(l->dl_hostname, "localhost", MAXHNLEN);
	}
	if (labelname) {
		strnzcpy(l->dl_label, labelname, MAXLBLLEN);
	} else {
		if (!interactive) {
			strnzcpy(l->dl_label, DISKNAME, MAXLBLLEN);
		} else {
			getrmsg("enter disk label: ");
			strnzcpy(l->dl_label, line, MAXLBLLEN);
		}
	}
	if (defbootfile) {
		strnzcpy(l->dl_bootfile, defbootfile, MAXBFLEN);
	}
}

int Format() {
	if((*dsp->ds_format)(fd, fn, TRUE)) {
		bomb(S_NEVER, "Disk Format Failed\n");
	}
	return 0;	
}

int init() {
	register int i, spbe;
	int status, nbad, *bbt;
	register struct disk_label *l = &disk_label;
	register struct partition *pp;
	char cmd[256];
	char buff[256];
	char *baseName;
	char partition;
	
	/* Generate cooked device names and test to see if any mounted. */
	
	if (!(baseName = rindex(fn, '/'))) {
	    sprintf(buff, "The pathname for the device, \"%s\", is too strange"
		"to be trusted.\nUse something like \"/dev/rsd0a\".\n", fn);
	    bomb(S_NEVER, buff);
	}
	
	/* NB: Cheesy test for raw device... */
	
	if (*(++baseName) != 'r') {
	    sprintf(buff, "The given pathname, \"%s\", doesn't look like a "
		"raw device.\nUse something like \"/dev/rsd0a\".\n", fn);
	    bomb(S_NEVER, buff);
	}
	
	*(baseName - 1) = '\0';	/* Slash temporarily overwritten. */
	
	for (partition = 'a'; partition < 'h'; partition++) {
	    sprintf(buff, "%s/%s%c", fn, baseName + 1, partition);
	    if (MountPoint(buff)) {
		bomb(S_NEVER, "The device is mounted as a filesystem and "
		    "can't be initialized.\n");
	    }
	}
	
	*(baseName - 1) = '/'; /* Slash restored */

	if (!confirm ("DESTROYS ALL EXISTING DISK DATA -- really initialize? "))
		return 0;

	make_new_label();
	d_size = dt->d_ncylinders * dt->d_ntracks * dt->d_nsectors;
	if (l->dl_version == DL_V1)
		spbe = l->dl_nsect >> 1;
	else
		spbe = 1;
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
		bzero (&bad_block, sizeof (bad_block));
	}
	i = (d_size - l->dl_front - l->dl_back -
		l->dl_ngroups * l->dl_ag_size) / spbe +
		l->dl_ngroups * (l->dl_ag_alts / spbe);
	i = (i < (nbad - 1))? i : nbad - 1;
	bbt[i] = -1;

	/* format volume if necessary */
	if(!no_format) {
		if((*dsp->ds_format)(fd, fn, FALSE)) {
			bomb(S_NEVER, "Disk Format Failed\n");
		}
	}

	/* do device mode selections */
	(*dsp->ds_config)();
	(*dsp->ds_devinit)();
	printf ("writing disk label\n");
	(*dsp->ds_wlabel) (bbt);
	boot();
	for (i = 0; i < NPART; i++) {
		pp = &dt->d_partitions[i];
		if (pp->p_newfs == 0)
			continue;
		sprintf (cmd, "/sbin/newfs %s%c", fn, 'a'+i);
		printf ("creating new filesystem on %s%c\n", fn, 'a'+i);
		printf("%s\n", cmd);
		if ((status = system (cmd)))
		    bomb (S_NEVER, "/sbin/newfs %s%c failed (status %d)",
			fn, 'a'+i, status >> 8);
	}
	printf ("initialization complete\n");
	return 0;
}

/*
 * If path could not be read but error is non-fatal, i.e. arch is
 * non-native, returns NULL.  Otherwise, returns a pointer to a 
 * malloc()'ed buffer filled with the contents of path.
 */
 
char *read_booter(const char *path, cpu_type_t arch, int *size)
{
    int bfd;
    char *buf;
    struct arch_boot_info *abi;
    struct stat statbuf;
   
    if ((bfd = open(path, 0)) < 0)
	if (arch == host_cpu_type)
	    dpanic(S_NEVER, path);
	else
	    return NULL;

    if (! (abi = boot_info_for_cputype(arch)))
	bomb(S_NEVER, "%s: unknown requirements", path);

    if (fstat(bfd, &statbuf) < 0)
	dpanic(S_NEVER, path);

    buf = malloc(statbuf.st_size);

    if ((*size = read(bfd, buf, statbuf.st_size)) < 0)
	dpanic(S_NEVER, path);

    if (*size > abi->maxsize)
    	bomb(S_NEVER, "Boot image size %d bytes exceeds maximum of %d bytes", *size, abi->maxsize);

    close(bfd);

    return(buf);
}


#if defined(hppa)
void hppa_write_booter(void *block0, const char *ipl_file)
{
    int i, j;
    char *buf;
    int size;
    struct mach_header *mh;
    struct segment_command *sc;
    struct section *sectptr;
    struct disk_label *l = &disk_label;
    struct ipl_header *ipl_header_p = (struct ipl_header *)block0;

    /* Read the IPL and write it into the front porch */

    if (!(buf = read_booter(ipl_file, CPU_TYPE_HPPA, &size))) 
    	return;
	
    printf("Writing %s\n", ipl_file);
    (*dsp->ds_glabel) (l, &bad_block, 0);
    if ((*dsp->ds_req) (CMD_WRITE, l->dl_boot0_blkno[0] * l->dl_secsize / devblklen, buf, size) < 0)
	bomb(S_NEVER, "Write of boot block failed\n");


    /* Now fill in block 0 information */

    /* Find the IPL entry point by examining the segment commands
     * in the boot image.  The entry point will be the offset into
     * the file to the __text section in the __TEXT segment.
     */
     
    ipl_header_p->ipl_entry_point = 0;
    mh = (struct mach_header *)buf;
    sc = (struct segment_command *)
		    ((char *)buf + sizeof(struct mach_header));
    for (i = 0; i < mh->ncmds; i++) {
	    if (sc->cmd == LC_SEGMENT && !strcmp(sc->segname, "__TEXT")) {
		    sectptr = (struct section *) ((char *)sc + sizeof(*sc));
		    for (j = 0; j < sc->nsects; j++) {
			    if (!strcmp(sectptr->sectname, "__text")) {
				    ipl_header_p->ipl_entry_point = sectptr->offset;
				    goto found_entry_point;
			    }
			    sectptr++;
		    }
	    }
	    sc = (struct segment_command *)((int)sc + sc->cmdsize);
    }
    
found_entry_point:

    if (ipl_header_p->ipl_entry_point == 0)
	bomb(S_NEVER, "entry point not found in %s", ipl_file);

    ipl_header_p->lif_magic = LIF_MAGIC;
    ipl_header_p->ipl_address = l->dl_boot0_blkno[0] * l->dl_secsize;
    ipl_header_p->ipl_size = roundup(size, IPL_ALIGNMENT);
    
    if ((ipl_header_p->ipl_address % IPL_ALIGNMENT) != 0)
	bomb(S_NEVER, "boot0_blkno is not a multiple of IPL_ALIGNMENT");

    free(buf);
}
#endif hppa


#if defined(sparc)
void sparc_write_booter(void *block0, const char *bootblk_file)
{
    char *buf;
    unsigned short *usp;
    int size;
    unsigned short dkl_cksum = 0;
    struct dk_label *dk_label_p = block0;

    /* Read and write the bootblk. */

    if (!(buf = read_booter(bootblk_file, CPU_TYPE_SPARC, &size))) 
    	return;

    printf("Writing %s\n", bootblk_file);
    if ((*dsp->ds_req) (CMD_WRITE, 1, buf, size) < 0)
	bomb(S_NEVER, "Write of bootblk failed\n");
    free(buf);


    /* Now fill in block 0 information */
    
    dk_label_p->dkl_magic = DKL_MAGIC;
    
    /* Compute the XOR checksum of block zero.  OBP wants block 0 to XOR checksum
     * to zero.  Since dkl_cksum is the last 2 bytes in block 0, compute the XOR
     * "sum" of all the unsigned shorts before it, and use that value for the checksum.
     */
    for (usp = (unsigned short *)dk_label_p; 
	    usp < (unsigned short *)(&dk_label_p->dkl_cksum); 
	    usp++) {
		dkl_cksum ^= *usp;
#if defined(DEBUG) && 0		
		printf("%d\t%04x\t%04x\n", (int)usp - (int)block0, *usp, dkl_cksum);
#endif		
    }
    dk_label_p->dkl_cksum = dkl_cksum;
}
#endif sparc

#if defined(ppc)
int boot()
{
	return 0;
}
#else
int boot()
{
	const char *blk0 = bootfile;
	int ok, bfd, size = 0;
	char buf[1024];
#if	A_OUT_COMPAT
	struct exec *a = (struct exec*) &buf;
#endif	A_OUT_COMPAT
	struct mach_header *mh = (struct mach_header*) &buf;
	char *blk0buf, blk0dup[DISK_BLK0SZ];
	char *memBuff;	/* Space malloced for blk0buf */
	struct disk_label *l = &disk_label;
	int i, nblk, success, lblblk;

#if defined(hppa) || defined(sparc)
	/* Don't install booters for hppa and sparc floppies */
	if (!strcmp(dsp->ds_type, "removable_rw_floppy"))
	    return 0;
#endif

	if (!bootfile) {
	    struct arch_boot_info *abi = boot_info_for_cputype(host_cpu_type);
	    if (!abi)
		dpanic(S_NEVER, "Unable to determine booter information");
	    bootfile = abi->boot;
	}
	blk0 = bootfile;

#if defined(m68k) || defined(i386)
	
	if ((memBuff = (char*) malloc(MAXBOOTSIZE + 16)) == NULL)
		bomb(S_NEVER, "Size (%d) of %s to big for memory",
			MAXBOOTSIZE + 16, blk0);
	/* 
	** Find a good alignment w/in memBuff, but keep the original ptr
	** for free.
	*/
	blk0buf = (char*)((((int)memBuff + 15) >> 4) << 4);
#endif m68k||i386

	/* write out boot blocks */
	if (interactive) {
		do {
			printf("Boot block is \"%s\", ", blk0);
			if (!(ok = confirm("ok? "))) {
				getrmsg("Boot block: ");
				blk0 = line;
			}
			do_boot = 1;
#if defined(i386)
			do_boot1 = 1;
#endif
		} while (!ok);
	}
	if (do_boot) {

#if defined(m68k) || defined(i386)
	
		if ((bfd = open(blk0, 0)) < 0)
			dpanic(S_NEVER, blk0);
			
		if (read(bfd, &buf, sizeof(buf)) != sizeof(buf))
			dpanic (S_NEVER, blk0);
#if	A_OUT_COMPAT
		if (a->a_magic == OMAGIC) {
			size = sizeof(a) + a->a_text + a->a_data;
		} else
#endif	A_OUT_COMPAT

		if (mh->magic == MH_MAGIC && mh->filetype == MH_PRELOAD) {
			struct segment_command *sc;
			int first_seg, cmd;
			
			sc = (struct segment_command*)
				(buf + sizeof (struct mach_header));
			first_seg = 1;
			size = 0;
			for (cmd = 0; cmd < mh->ncmds; cmd++) {
				switch (sc->cmd) {
				
				case LC_SEGMENT:
					if (first_seg) {
						size += sc->fileoff;
						first_seg = 0;
					}
					size += sc->filesize;
					break;
				}
				sc = (struct segment_command*)
					((int)sc + sc->cmdsize);
			}
		}
	/*	Commenting this out since /usr/standalone/boot on a 486 isn't a Mach-O: MGW
		else
			bomb(S_NEVER, "%s: unknown binary format", blk0);
	*/
		lseek(bfd, 0, 0);
		if (size) {
			if ((i = read(bfd, blk0buf, size)) < 0)
				dpanic (S_NEVER, blk0);
			if (i != size)
				bomb(S_NEVER, "%s: corrupted image size (%d != %d)",
					blk0, i, size);
		}
		else {
			if ((size = read(bfd, blk0buf, MAXBOOTSIZE)) < 0)
				dpanic(S_NEVER, blk0);
		}
	
		close(bfd);
		(*dsp->ds_glabel) (l, &bad_block, 0);
		/*
		 * Make sure boot blocks don't:
		 *	- overwrite labels
		 *	- overwrite each other
		 *	- extend beyond front porch
		 */
	
		/* FIXME: od: account for sparse labels and bitmap! */
		lblblk = NLABELS * howmany(sizeof(struct disk_label), l->dl_secsize) + dosbase;
		nblk = howmany(size, l->dl_secsize);
	
		success = 0;
		for (i = 0; i < NBOOTS; i++) {
			if (l->dl_boot0_blkno[i] < 0)
				continue;
			if (l->dl_boot0_blkno[i] < lblblk)
				bomb(S_NEVER, "boot block overlays labels");
			if (l->dl_boot0_blkno[i] + nblk > l->dl_front + dosbase)
				bomb(S_NEVER, "boot block extends beyond front porch");
			if (i < NBOOTS-1
			    && l->dl_boot0_blkno[i] != l->dl_boot0_blkno[i+1]) {
				if ((l->dl_boot0_blkno[i+1] > 0) &&
					(l->dl_boot0_blkno[i] > l->dl_boot0_blkno[i+1]))
					bomb(S_NEVER, "boot blocks out of order");
				if ((l->dl_boot0_blkno[i+1] > 0) &&
					(l->dl_boot0_blkno[i]+nblk > l->dl_boot0_blkno[i+1]))
					bomb(S_NEVER, "boot blocks overlay each other");
			}
			success++;
		}
		if (! success)
			bomb(S_NEVER, "No boot blocks specified in label");
		success = 0;
		for (i = 0; i < NBOOTS; i++) {
			if (l->dl_boot0_blkno[i] < 0)
				continue;
			if ((*dsp->ds_req) (CMD_WRITE, (int)((unsigned long long)l->dl_boot0_blkno[i] * l->dl_secsize / devblklen), blk0buf, size) < 0)
				printf("Write of boot block %d failed\n", i);
			else {
				success++;
			}
		}
		if (! success)
			bomb(S_NEVER, "No boot blocks on disk");
		printf("Writing %s\n", blk0);

#endif m68k||i386

#if defined(hppa) || defined(sparc)
	{
	    char block0[512];
	    
	    bzero(block0, sizeof(block0));
	    hppa_write_booter(block0, host_cpu_type == CPU_TYPE_HPPA 
		? blk0 : (boot_info_for_cputype(CPU_TYPE_HPPA))->boot);
	    sparc_write_booter(block0, host_cpu_type == CPU_TYPE_SPARC
		? blk0 : (boot_info_for_cputype(CPU_TYPE_SPARC))->boot);
	    
	    if ((*dsp->ds_req) (CMD_WRITE, 0, block0, sizeof(block0)) < 0)
		    bomb(S_NEVER, "Write of boot block header failed\n");

    }
#endif sparc||hppa

	}

	if (dosdisk && do_boot0) {
		/** FIXME: Save the original boot sector somewhere... **/
		if ((bfd = open(bootfile0, 0)) < 0)
			dpanic(S_NEVER, bootfile0);
		size = read(bfd, blk0buf, DISK_BLK0SZ);
		if ((*dsp->ds_req) (CMD_READ, 0, blk0dup, DISK_BLK0SZ) < 0)
			bomb(S_NEVER, "Read of sector 0 failed\n");
		bcopy(blk0dup+DISK_BOOTSZ,blk0buf+DISK_BOOTSZ,sizeof(struct fdisk_part)*FDISK_NPART);
		if ((*dsp->ds_req) (CMD_WRITE, 0, blk0buf, DISK_BLK0SZ) < 0)
			bomb(S_NEVER, "Write of boot0 failed\n");
		printf("Writing %s\n", bootfile0);
	} 

		
	if (do_boot1) {
		if ((bfd = open(bootfile1, 0)) >= 0) {
			if ((size = read(bfd, blk0buf, DISK_BLK0SZ)) < 0) 
				dpanic(S_NEVER, bootfile1);
			if ((*dsp->ds_req) (CMD_WRITE, dosbase, blk0buf, DISK_BLK0SZ) < 0)
				bomb(S_NEVER, "Write of boot1 failed\n");
			printf("Writing %s\n", bootfile1);
		}
	}

#if defined(m68k) || defined(i386)
	free(memBuff);
#endif m68k||i386

	return 0;
} /* boot */
#endif /* defined(ppc) */

int write_label()
{
	register struct disk_label *l = &disk_label;
	struct disk_label old_disk_label;
	register struct disk_label *ol = &old_disk_label;
	int spbe, nbad, *bbt, *o_bbt, d_size;
	register int i, j;
	struct bad_block o_bad_block;

	make_new_label();
	
	/*
	 *  Merge in bad block info from old label.
	 *  This is required if we're just changing a value of
	 *  the label that doesn't affect disk geometry and
	 *  want to preserve the old bad block structure.
	 */
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		o_bbt = ol->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		o_bbt = o_bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	if (l->dl_version == DL_V1)
		spbe = l->dl_nsect >> 1;
	else
		spbe = 1;
	d_size = l->dl_ncyl * l->dl_ntrack * l->dl_nsect;
	i = (d_size - l->dl_front - l->dl_back -
		l->dl_ngroups * l->dl_ag_size) / spbe +
		l->dl_ngroups * (l->dl_ag_alts / spbe);
	if ((*dsp->ds_glabel) (ol, &o_bad_block, 1) >= 0) {
		if (!confirm (
"WARNING: using information from /etc/disktab to construct new disk label.\n"
"If all you want to do is change the host name or label name, then use the\n"
"\"host\" or \"name\" commands.  If the information in /etc/disktab doesn't\n"
"match the disk geometry specified by the current disk label then the disk\n"
"contents may be unreadable.\n"
"OK to construct new disk label? "))
			return 0;
		if (ol->dl_version == DL_V1)
			spbe = ol->dl_nsect >> 1;
		else
			spbe = 1;
		d_size = ol->dl_ncyl * ol->dl_ntrack * ol->dl_nsect;
		j = (d_size - ol->dl_front - ol->dl_back -
			ol->dl_ngroups * ol->dl_ag_size) / spbe +
			ol->dl_ngroups * (ol->dl_ag_alts / spbe);
		if (l->dl_version == ol->dl_version && i == j) {
			printf ("merging in bad block info from old label\n");
			for (j = 0; j < nbad; j++)
				bbt[j] = o_bbt[j];
		}
	}

	/* mark end of bad block table */
	i = (i < (nbad - 1))? i : nbad - 1;
	bbt[i] = -1;
	printf ("writing disk label\n");
	(*dsp->ds_wlabel) (bbt);
	return 0;
}

int label() {
	register int i;
	register char c;
	register struct disk_label *l = &disk_label;
	register struct partition *p;

	c = getrmsg ("label information: print, write? ");
	if (c == 'p') {
	(*dsp->ds_glabel) (l, &bad_block, 0);
	printf ("current label information on disk:\n");
	printf ("disk label version #%d\n",
		l->dl_version == DL_V1? 1 : l->dl_version == DL_V2? 2 : 3);
	printf ("disk label: %s\ndisk name: %s\ndisk type: %s\n",
		l->dl_label, l->dl_name, l->dl_type);
	printf ("ncyls %d ntrack %d nsect %d rpm %d\n",
		l->dl_ncyl, l->dl_ntrack, l->dl_nsect, l->dl_rpm);
	printf ("sector_size %d front_porch %d back_porch %d\n",
		l->dl_secsize, l->dl_front, l->dl_back);
	printf ("ngroups %d ag_size %d ag_alts %d ag_off %d\n",
		l->dl_ngroups, l->dl_ag_size, l->dl_ag_alts, l->dl_ag_off);
	printf ("boot blocks: ");
	for (i = 0; i < NBOOTS; i++)
		printf ("#%d at %d ", i+1, l->dl_boot0_blkno[i]);
	printf ("\n");
	if (l->dl_bootfile[0])
		printf ("bootfile: %s\n", l->dl_bootfile);
	if (l->dl_hostname[0])
		printf ("host name: %s\n", l->dl_hostname);
	if (l->dl_rootpartition)
		printf ("root partition: %c\n", l->dl_rootpartition);
	if (l->dl_rwpartition)
		printf ("read/write partition: %c\n", l->dl_rwpartition);
printf (
"part   base   size bsize fsize cpg density minfree newfs optim automount type\n");
/*
 x    xxxxxx xxxxxx  xxxx  xxxx xxx   xxxxx     xx%   xxx xxxxx       xxx ...
*/
	for (i = 0; i < NPART; i++) {
		p = &l->dl_part[i];
		if (p->p_base == -1)
			continue;
printf ("%c    %6d %6d  %4d  %4d %3d   %5d     %2d%%   %s %s       %s %s\n",
			i+'a', p->p_base, p->p_size, p->p_bsize, p->p_fsize,
			p->p_cpg, p->p_density, p->p_minfree,
			p->p_newfs? "yes" : " no",
			p->p_opt == 's'? "space" : " time",
			p->p_automnt? "yes" : " no", p->p_type);
		if (p->p_mountpt[0])
			printf ("mount point: %s\n", p->p_mountpt);
	}
	} else
	if (c == 'w') {
		write_label();
	} else
		printf ("invalid label information command\n");
	return 0;
}

int newhost() {
	register struct disk_label *l = &disk_label;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (interactive) {
		getrmsg("enter host name: ");
		strnzcpy(l->dl_hostname, line, MAXHNLEN);
	} else {
		strnzcpy(l->dl_hostname, hostname, MAXHNLEN);
		printf ("changing hostname to %s\n", hostname);
	}
	(*dsp->ds_wlabel) (&bad_block);
	return 0;
}

int defboot() {
	register struct disk_label *l = &disk_label;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (interactive) {
		getrmsg("enter default bootfile: ");
		strnzcpy(l->dl_bootfile, line, MAXBFLEN);
	} else {
		strnzcpy(l->dl_bootfile, defbootfile, MAXBFLEN);
		printf ("changing default bootfile to %s\n", defbootfile);
	}
	(*dsp->ds_wlabel) (&bad_block);
	return 0;
}

int newlbl() {
	register struct disk_label *l = &disk_label;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (interactive) {
		getrmsg("enter disk label name: ");
		strnzcpy(l->dl_label, line, MAXLBLLEN);
	} else {
		strnzcpy(l->dl_label, labelname, MAXLBLLEN);
		printf ("changing disk label name to \"%s\"\n", labelname);
	}
	(*dsp->ds_wlabel) (&bad_block);
	return 0;
}

int bad_block_stats()
{
	register int i;
	register struct disk_label *l = &disk_label;
	int total, numbad, nbad, *bbt;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	total = numbad = 0;
	for (i = 0; i < nbad && bbt[i] != -1; i++) {
		total++;
		if (bbt[i])
			numbad++;
	}
	printf ("%d/%d (%4.1f%%) alternate blocks used\n",
		numbad, total,
		(float) numbad * 100.0 / (float) total);
	return 0;
}

int write_bitmap() {
	register int e;

	if ((e = ioctl (fd, DKIOCSBITMAP, bit_map)) < 0)
		dpanic (S_NEVER, "set bitmap");
	return 0;
}

int bitmap()
{
	register int i, j, k, first, num, nht, offset, shift, from, to;
	register struct disk_label *l = &disk_label;
	register int *b;
	register char c;
	float U = 0, B = 0, E = 0, W = 0, T;

	c = getrmsg ("status bitmap: read, print, edit, change, write, stats? ");
	if (c == 'r') {
		(*dsp->ds_glabel) (l, &bad_block, 0);
		d_size = l->dl_ncyl * l->dl_ntrack * l->dl_nsect;
		if (bit_map)
			free (bit_map);
		if (l->dl_version == DL_V1)
			bit_map = (int*) malloc ((d_size / l->dl_nsect) >> 1);
		else
			bit_map = (int*) malloc (d_size >> 2);
		if (ioctl (fd, DKIOCGBITMAP, bit_map) < 0)
			dpanic (S_NEVER, "get bitmap");
	} else
	if (c == 'p') {
		if ((b = bit_map) == 0) {
			printf ("no bitmap read in yet\n");
			return 0;
		}
		if (l->dl_version == DL_V1) {
			nht = (d_size / l->dl_nsect) << 1;
			first = getnrmsg ("first half track? ", 0, nht - 1);
			if (first == -1)
				return 0;
			num = getnrmsg ("# of half tracks? ", 0, nht - first);
		} else {
			nht = d_size;
			first = getnrmsg ("first sector? ", 0, nht - 1);
			if (first == -1)
				return 0;
			num = getnrmsg ("# of sectors? ", 0, nht - first);
		}
		if (num == -1)
			return 0;
	printf ("#\tstatus: (U=untested, B=bad, e=erased, w=written)\n");
		for (i = first, j = 0; i < first + num; i++, j++) {
			if ((j % 64) == 0)
				printf ("\n%6d\t", i);
			switch ((b[i>>4] >> ((i&0xf) << 1)) & 3) {
				case SB_UNTESTED:	c = 'U';  break;
				case SB_BAD:		c = 'B';  break;
				case SB_ERASED:		c = 'e';  break;
				case SB_WRITTEN:	c = 'w';  break;
			}
			printf ("%c", c);
		}
		printf ("\n");
	} else
	if (c == 'e') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return 0;
		}
		printf ("edit bitmap -- <return> when finished\n");
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		while (1) {
			if ((num = getnrmsg ("entry? ", 0, nht)) == -1)
				break;
again:
			switch (getrmsg (
				"u=untested, b=bad, e=erased, w=written? ")) {
				case 'u':	j = SB_UNTESTED;  break;
				case 'b':	j = SB_BAD;  break;
				case 'e':	j = SB_ERASED;  break;
				case 'w':	j = SB_WRITTEN;  break;
				default:	printf ("huh?\n");  goto again;
			}

			offset = num >> 4;
			shift = (num & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (j << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'E') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return 0;
		}
		printf ("edit bitmap range\n");
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		if ((from = getnrmsg ("from? ", 0, nht)) == -1)
			return 0;
		if ((to = getnrmsg ("to? ", 0, nht)) == -1)
			return 0;
again2:
		switch (getrmsg (
			"u=untested, b=bad, e=erased, w=written? ")) {
			case 'u':	j = SB_UNTESTED;  break;
			case 'b':	j = SB_BAD;  break;
			case 'e':	j = SB_ERASED;  break;
			case 'w':	j = SB_WRITTEN;  break;
			default:	printf ("huh?\n");  goto again2;
		}

		for (num = from; num < to; num++) {
			offset = num >> 4;
			shift = (num & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (j << shift);
		}
		bad_modified = 1;
	} else
	if (c == 'c') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return 0;
		}
		printf ("change a range of bitmap entries\n");
		if (l->dl_version == DL_V1) {
			nht = (d_size / l->dl_nsect) << 1;
			first = getnrmsg ("first half track? ", 0, nht - 1);
			if (first == -1)
				return 0;
			num = getnrmsg ("# of half tracks? ", 0, nht - first);
		} else {
			nht = d_size;
			first = getnrmsg ("first sector? ", 0, nht - 1);
			if (first == -1)
				return 0;
			num = getnrmsg ("# of sectors? ", 0, nht - first);
		}
		if (num == -1)
			return 0;
huh:
		switch (getrmsg (
			"u=untested, b=bad, e=erased, w=written? ")) {
			case 'u':	k = SB_UNTESTED;  break;
			case 'b':	k = SB_BAD;  break;
			case 'e':	k = SB_ERASED;  break;
			case 'w':	k = SB_WRITTEN;  break;
			default:	printf ("huh?\n");  goto huh;
		}
		for (i = first; i < first + num; i++) {
			offset = i >> 4;
			shift = (i & 0xf) << 1;
			j = bit_map[offset];
			j &= ~(3 << shift);
			bit_map[offset] = j | (k << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'w') {
		if (bit_map) {
			printf ("writing bitmap\n");
			write_bitmap();
		} else
			printf ("no bitmap read in yet\n");
	} else
	if (c == 's') {
		if ((b = bit_map) == 0) {
			printf ("no bitmap read in yet\n");
			return 0;
		}
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		for (i = 0; i < nht; i++) {
			switch ((b[i>>4] >> ((i&0xf) << 1)) & 3) {
				case SB_UNTESTED:	U++;  break;
				case SB_BAD:		B++;  break;
				case SB_ERASED:		E++;  break;
				case SB_WRITTEN:	W++;  break;
			}
		}
		T = (U + B + E + W) / 100.0;
		printf ("%4.1f%% untested, %4.1f%% bad, %4.1f%% erased, "
			"%4.1f%% written\n", U/T, B/T, E/T, W/T);
	}
	else
		printf ("invalid status bitmap command\n");
	return 0;
}

void timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

void timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

int Read()
{
	register int blk, len, inc;
	volatile int i, incr;
	int ms, bytes, secnt, blkno, bcount;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	bcount = dt->d_secsize * secnt;
	gettimeofday (&start, 0);
	bytes = 0;
	for (i = 0; i < len && setjmp (env) == 0; i++) {
		if ((*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "read");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
		bytes += bcount;
	}
	gettimeofday (&stop, 0);
	timevalsub (&stop, &start);
	ms = stop.tv_sec * 1000 + stop.tv_usec / 1000;
	if (ms == 0)
		ms = 1;
	printf ("%d bytes in %d ms = %u bytes/s\n",
		bytes, ms, (unsigned) ((bytes * 100) / (ms / 10)));
	return 0;
}

int Write()
{
	register int blk, len, inc, j;
	volatile int i, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int ms, bytes, rand = 0, secnt, blkno, bcount;
	volatile u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	rand = confirm ("random data? ");
	bcount = dt->d_secsize * secnt;
	gettimeofday (&start, 0);
	bytes = 0;
	for (i = 0; i < len && setjmp (env) == 0; i++) {
		if (rand)
			for (j = 0; j < dt->d_secsize; j++)
				wb[i] = ri(0, 255);
		if ((*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "write");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
		bytes += bcount;
	}
	gettimeofday (&stop, 0);
	timevalsub (&stop, &start);
	ms = stop.tv_sec * 1000 + stop.tv_usec / 1000;
	if (ms == 0)
		ms = 1;
	printf ("%d bytes in %d ms = %u bytes/s\n",
		bytes, ms, (unsigned) ((bytes * 100) / (ms / 10)));
	return 0;
}

int verify()
{
	register int blk, len, inc, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	int blkno, bcount, secnt;

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	bcount = dt->d_secsize * secnt;
	while (blkno < blk + len) {
		if ((*dsp->ds_req) (CMD_VERIFY, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "verify");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
	}
    return 0;
}

void log_msg (char *msg, ...)
{
    va_list args;
    char buf[4096];

    va_start(args, msg); 
    vsprintf(buf, msg, args);
    va_end(args);
    
    printf(buf);
    syslog(LOG_ERR, buf);
}

int rw()
{
	register int blk, len, inc, i, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int rand = 0, blkno, bcount, cmp, secnt, err;
	register u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	register u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);
	register u_char *bp, *bp2;

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	rand = confirm ("random data? ");
	cmp = confirm ("compare? ");
	bcount = dt->d_secsize * secnt;
	while (blkno < blk + len) {
		if (rand)
			for (i = 0; i < bcount; i++)
				wb[i] = ri(0, 255);
		if ((*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "write");
		if ((*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "read");
		if (cmp) {
			i = 0;  err = 0;
			for (bp = rb, bp2 = wb; bp < &rb[bcount];
			    bp++, bp2++, i++)
				if (*bp != *bp2) {
					log_msg ("1: R%02xW%02xX%02x@%d|%d ",
						*bp, *bp2, *bp ^ *bp2,
						blkno, i);
					err = 1;
				}
			if (err) {
				i = 0;
				for (bp = rb, bp2 = wb; bp < &rb[bcount];
				    bp++, bp2++, i++)
					if (*bp != *bp2) {
						log_msg ("2: R%02xW%02xX%02x@%d|%d ",
							*bp, *bp2, *bp ^ *bp2,
							blkno, i);
					}
			}
			if (err && abort_flag)
				return 0;
		}
		blkno += incr;
		if (inc < 0)
			incr = -incr;
	}
    return 0;
}

int rwr()
{
	register int blk;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int delta = 0, blkno, bcount, secnt, r = 0, w = 0;
	register u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	register u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	delta = getnrmsg ("sector delta? ", 0, size);
	r = confirm ("read? ");
	w = confirm ("write? ");
	bcount = dt->d_secsize * secnt;
	while (1) {
		if (w && (*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "write");
		if (r && (*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			dpanic (S_NEVER, "read");
		blkno += (ri (0, 2 * delta) - delta) * secnt;
		if (blkno > blk + delta * secnt ||
		    blkno < blk - delta * secnt)
			blkno = blk;
	}
}

#define	LOOK_W	24

int look()
{
	register int i, j, off, len, stop;
	register u_char b;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16),
		*wb = (u_char*)(((int)test_wbuf+16)/16*16), *buf;

	buf = getrmsg ("read or write buffer? ") == 'r'? rb : wb;
	off = getnrmsg ("offset? ", 0, TBSIZE - 1);
	len = getnrmsg ("length? ", 1, TBSIZE - off);
	stop = (off + len + LOOK_W) / LOOK_W * LOOK_W;
	for (i = off; i < stop; i += LOOK_W) {
		printf ("\n%4d\t", i);
		for (j = i; j < i + LOOK_W; j++)
			printf ("%02x ", buf[j]);
		printf ("    ");
		for (j = i; j < i + LOOK_W; j++) {
			b = buf[j];
			printf ("%c", (b < ' ' || b > 0x7e)? '.' : b);
		}
	}
	printf ("\n");
	return 0;
}

int set()
{
	register int i, off, len, rand = 0, val = 0;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16),
		*wb = (u_char*)(((int)test_wbuf+16)/16*16), *buf;

	buf = getrmsg ("read or write buffer? ") == 'r'? rb : wb;
	off = getnrmsg ("offset? ", 0, TBSIZE - 1);
	len = getnrmsg ("length? ", 1, TBSIZE - off);
	if (getrmsg ("random, <constant>? ") == 'r')
		rand = 1;
	else
		val = atoi (line);
	for (i = off; i < off + len; i++)
		buf[i] = rand? ri(0, 255) : val;
	return 0;
}

int eject() {
	if(ioctl(fd, DKIOCEJECT, 0) < 0)
		dpanic (S_NEVER, "eject");
	return 0;
}

int tabort() {
	abort_flag ^= 1;
	printf ("abort on error mode %s\n", abort_flag? "on" : "off");
    	return 0;
}

int tvers() {
	version = (version == DL_V2)? DL_V3 : DL_V2;
	printf ("label version #%d\n", version == DL_V2? 2 : 3);
	return 0;
}

int pstats() {
	register struct disk_stats *s = &stats;

	if (ioctl (fd, DKIOCGSTATS, s) < 0)
		dpanic (S_NEVER, "getstats");
	printf ("disk statistics:\n");
	printf ("\t%7d average ECC corrections per transfer\n", s->s_ecccnt);
	printf ("\t%7d maximum ECC corrections per transfer\n", s->s_maxecc);
	(*dsp->ds_pstats)();
	bad_block_stats();
	return 0;
}

int zstats() {
	if (ioctl (fd, DKIOCZSTATS) < 0)
		dpanic (S_NEVER, "zerostats");
	return 0;
}

int quit() {
	exit (0);
	closelog();
	return 0;
}

void dpanic (int status, const char *msg)
{
	perror (msg);
	if (interactive)
		longjmp (env, 1);
	else {
		exit (status);
		closelog();
	}
}


struct dtype_sw *
drive_type(type)
char *type;
{
	struct dtype_sw *dsp;

	for (dsp = dtypes; dsp->ds_type; dsp++)
		if (strcmp(type, dsp->ds_type) == 0)
			return(dsp);
	return(NULL);
}

/* 
** Return mount point of device (e.g. "/dev/sd0a") as recorded in
** /etc/mtab (which sometimes lies).  - GRS 9/16/91
*/

char *
MountPoint(char *dev)
{
#if 0
    FILE		*mtab;
    struct mntent	*ment;
    
    if ((mtab = setmntent("/etc/mtab", "r"))) {
	while ((ment = getmntent(mtab))) {
	    if (!strcmp(ment->mnt_fsname, dev)) {
		endmntent(mtab);
		return ment->mnt_dir;
	    }
	}
	endmntent(mtab);
    }
#endif
    return NULL;
}

int bad() {
	register int i, spbe, apag, alt, ag, offset, shift;
	register char c;
	register struct disk_label *l = &disk_label;
	int nbad, *bbt;

	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	c = getrmsg ("bad block table: print, edit, write, stats? ");
	if (c == 'p') {
		if (!bad_modified) {
			(*dsp->ds_glabel) (l, &bad_block, 0);
			if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
				bbt = l->dl_bad;
				nbad = NBAD;
			} else {
				bbt = bad_block.bad_blk;
				nbad = NBAD_BLK;
			}
		}
		if (l->dl_version == DL_V1)
			spbe = l->dl_nsect >> 1;
		else
			spbe = 1;
		apag = l->dl_ag_alts / spbe;
		printf ("entry(ag,#): bad_block->alternate\n");
		printf ("entries not listed are available\n");
		for (i = 0; i < nbad && bbt[i] != -1; i++) {
			if (bbt[i] == 0)
				continue;
			ag = i / apag;
			if (ag < l->dl_ngroups) {
				alt = l->dl_front + ag*l->dl_ag_size +
					l->dl_ag_off + (i % apag) * spbe;
				printf ("%d(%d,%d): %d->%d  ", i, ag,
					i % apag, bbt[i], alt);
			} else {
				alt = l->dl_front + (i - apag*l->dl_ngroups)
					* spbe + l->dl_ngroups*l->dl_ag_size;
				printf ("%d(ovfl): %d->%d  ", i,
					bbt[i], alt);
			}
			if ((i % 6) == 0)
				printf ("\n");
		}
		printf ("\n");
	} else
	if (c == 'e') {
		printf ("edit bad block table -- <return> when finished\n");
		while (1) {
			if ((i = getnrmsg ("entry? ", 0, nbad)) == -1)
				break;
			if ((alt = getnrmsg ("bad block? ", 0, d_size)) ==
			    -1)
				break;
			bbt[i] = alt;

			/* mark bitmap entry as bad */
			i = alt;
			if (l->dl_version == DL_V1) {
				i = (alt / l->dl_nsect) << 1;
				if ((alt % l->dl_nsect) >= (l->dl_nsect >> 1))
					i |= 1;
			}
			offset = i >> 4;
			shift = (i & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (SB_BAD << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'w') {
		if (!confirm ("really write bad block table? "))
			return 0;
		(*dsp->ds_wlabel) (bbt);
		write_bitmap();
		bad_modified = 0;
	} else
	if (c == 's') {
		bad_block_stats();
	} else
		printf ("invalid bad block command\n");
	return 0;
}

