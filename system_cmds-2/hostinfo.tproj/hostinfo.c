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
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log: hostinfo.c,v $
 * Revision 1.1.1.1.54.2  1999/03/16 17:53:26  wsanchez
 * Substitute License
 *
 * Revision 1.1.1.1.54.1  1999/03/16 04:42:07  wsanchez
 * Add APL
 *
 * Revision 1.1.1.1  1997/10/13 19:00:18  wsanchez
 * Import of system_cmds-28
 *
 * Revision 2.2  90/01/24  22:57:33  mrt
 * 	Add information about default processor set.
 * 	[89/02/20            dlb]
 * 
 *  1-Dec-88  David Black (dlb) at Carnegie-Mellon University
 *	Complete rewrite for host port and new host_info.
 *
 * 28-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */
/*
 *	File:	hostinfo.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1987, Avadis Tevanian, Jr.
 *
 *	Display information about the host this program is
 *	execting on.
 */

#include <mach/mach.h>

struct host_basic_info	hi;
kernel_version_t	version;
int 			slots[1024];

#ifdef	NeXT

#ifdef	m68k
#define ARCH_PRIVATE
#define KERNEL_PRIVATE
#import	<sys/types.h>
#import	<architecture/m68k/scr.h>
#import	<bsd/sys/table.h>

extern int table();
#endif	m68k

#endif	NeXT


main(argc, argv)
	int	argc;
	char	*argv[];
{
	kern_return_t		ret;
	int			size;
	char			*cpu_name, *cpu_subname;
	int			i, num_slots, count;
	processor_set_t		default_pset;
	host_t			host;
	struct processor_set_basic_info	info;
	unsigned char		cpu_rev, cpu_clk;

	host = host_self();
	ret = host_kernel_version(host, version);
	if (ret != KERN_SUCCESS) {
		mach_error(argv[0], ret);
		exit(1);
	}
	printf("Mach kernel version:\n\t %s\n", version);
	size = sizeof(hi)/sizeof(int);
	ret = host_info(host, HOST_BASIC_INFO, &hi, &size);
	if (ret != KERN_SUCCESS) {
		mach_error(argv[0], ret);
		exit(1);
	}
	num_slots = sizeof(slots)/sizeof(int);
	ret = host_info(host, HOST_PROCESSOR_SLOTS, slots, &num_slots);
	if (ret != KERN_SUCCESS) {
		mach_error(argv[0], ret);
		exit(1);
	}
	ret = processor_set_default(host_self(), &default_pset);
	if (ret != KERN_SUCCESS) {
		mach_error(argv[0], ret);
		exit(1);
	}
	count = PROCESSOR_SET_BASIC_INFO_COUNT;
	ret = processor_set_info(default_pset, PROCESSOR_SET_BASIC_INFO,
		&host, &info, &count);
	if (ret != KERN_SUCCESS) {
		mach_error(argv[0], ret);
		exit(1);
	}
	if (hi.max_cpus > 1)
		printf("Kernel configured for up to %d processors.\n",
			hi.max_cpus);
	else
		printf("Kernel configured for a single processor only.\n");
	printf("%d processor%s physically available.\n", hi.avail_cpus,
		(hi.avail_cpus > 1) ? "s are" : " is");

	printf("Processor type:");
	slot_name(hi.cpu_type, hi.cpu_subtype, &cpu_name, &cpu_subname);
	printf(" %s (%s)\n", cpu_name, cpu_subname);

#ifdef	NeXT
#ifdef	m68k
	ret = table(TBL_NeXT_CPU_REV2, 0, (char *)&cpu_clk, 1, sizeof(cpu_clk));
	if (ret == -1) {
		perror(argv[0]);
		exit(1);
	}
	printf("Processor speed: %d MHz\n", cpu_clk);
#endif	m68k
#endif	NeXT

	printf("Processor%s active:", (num_slots > 1) ? "s" : "");
	for (i = 0; i < num_slots; i++)
		printf(" %d", slots[i]);
	printf("\n");

#ifdef	NeXT
#ifdef	m68k
        ret = table(TBL_NeXT_CPU_REV, 0, (char *)&cpu_rev, 1, sizeof(cpu_rev));
	if (ret == -1) {
		perror(argv[0]);
		exit(1);
	}

	printf("System type: %d\n", MACHINE_TYPE(cpu_rev));
	printf("Board revision: 0x%x\n", BOARD_REV(cpu_rev));
#endif	m68k
#endif	NeXT

	printf("Primary memory available: %.2f megabytes.\n",
			(float)hi.memory_size/(1024.0*1024.0));
	printf("Default processor set: %d tasks, %d threads, %d processors\n",
		info.task_count, info.thread_count, info.processor_count);
	printf("Load average: %d.%02d, Mach factor: %d.%02d\n",
		info.load_average/LOAD_SCALE,
		(info.load_average%LOAD_SCALE)/10,
		info.mach_factor/LOAD_SCALE,
		(info.mach_factor%LOAD_SCALE)/10);
}
