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
 *	Copyright (C) 1998  Apple Computer, Inc.
 *
 *	File:	machdep/ppc/kern_machdep.c
 *
 *	Machine-specific kernel routines.
 *
 * HISTORY
 *  8-jun-1998	Umesh Vaishampayan  (umeshv@apple.com)
 *		Replaced grade_cpu_subtype() and check_cpu_subtype() with
 *		the implementation by Kevin Enderby.
 *		Deleted obsolete history.
 *
 */

#import <cpus.h>
#import	<sys/types.h>
#import	<mach/machine.h>
#import	<machine/cpu.h>

/*
 * Routine: grade_cpu_subtype()
 *
 * Function:
 *	Return a relative preference for cpu_subtypes in fat executable files.
 *	The higher the grade, the higher the preference.
 *	A grade of 0 means not acceptable.
 */

int
grade_cpu_subtype (cpu_subtype)
cpu_subtype_t	cpu_subtype;
{
	struct machine_slot *ms = &machine_slot[cpu_number()];

	/*
	 * This code should match cpusubtype_findbestarch() in best_arch.c in the
	 * cctools project.  As of 2/16/98 this is what has been agreed upon for
	 * the PowerPC subtypes.  If an exact match is not found the subtype will
	 * be picked from the following order:
	 *		750, 604e, 604, 603ev, 603e, 603, ALL
	 * Note the 601 is NOT in the list above.  It is only picked via an exact
	 * match. For details see Radar 2213821.
	 *
	 * To implement this function to follow what was agreed upon above, we use
	 * the fact there are currently 8 different subtypes.  Exact matches return
	 * the value 8, the value 0 is returned for 601 that is not an exact match,
	 * and the values 7 thru 1 are returned for the subtypes listed in the order
	 * above.
	 */
	if (ms->cpu_subtype == cpu_subtype)
		return 8;
	if (cpu_subtype == CPU_SUBTYPE_POWERPC_601)
		return 0;
	switch (cpu_subtype) {
		case CPU_SUBTYPE_POWERPC_750:
			return 7;
		case CPU_SUBTYPE_POWERPC_604e:
			return 6;
		case CPU_SUBTYPE_POWERPC_604:
			return 5;
		case CPU_SUBTYPE_POWERPC_603ev:
			return 4;
		case CPU_SUBTYPE_POWERPC_603e:
			return 3;
		case CPU_SUBTYPE_POWERPC_603:
			return 2;
		case CPU_SUBTYPE_POWERPC_ALL:
			return 1;
	}
	/*
	 * If we get here it is because it is a cpusubtype we don't support (602 and
	 * 620) or new cpusubtype that was added since this code was written.  Both
	 * will be considered unacceptable.
	 */
	return 0;
}

int
check_cpu_subtype (cpu_subtype)
cpu_subtype_t	cpu_subtype;
{
	struct machine_slot *ms = &machine_slot[cpu_number()];

	if (cpu_subtype == ms->cpu_subtype)
		return (TRUE);

	if (cpu_subtype == CPU_SUBTYPE_POWERPC_601)
		return (FALSE);

	switch (cpu_subtype) {
		case CPU_SUBTYPE_POWERPC_750:
		case CPU_SUBTYPE_POWERPC_604e:
		case CPU_SUBTYPE_POWERPC_604:
		case CPU_SUBTYPE_POWERPC_603ev:
		case CPU_SUBTYPE_POWERPC_603e:
		case CPU_SUBTYPE_POWERPC_603:
		case CPU_SUBTYPE_POWERPC_ALL:
			return (TRUE);
	}

	return (FALSE);
}

