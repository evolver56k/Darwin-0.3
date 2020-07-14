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
#import <bsd/sys/reboot.h>
#import <bsd/ppc/reboot.h>
#include <mach/machine/vm_types.h>


/* definitions */
extern char rootdevice[8];
extern int nbuf;
extern int srv;
extern int bufpages;
extern int ncl;
extern vm_offset_t mem_size;
extern int kdp_flag;
extern int default_preemption_rate;
extern int hash_table_factlog2;
extern int symbol_table_addr;

#define NBOOTFILE   64
char    boot_file[NBOOTFILE + 1] = "mach_kernel"; /* metalink */

struct kernargs {
	char *name;
	int *i_ptr;
} kernargs[] = {
	"nbuf", &nbuf,
	"bufpages", &bufpages,
	"rootdev", (int*) rootdevice,
	"rd", (int*) rootdevice,
	"maxmem", (int *) &mem_size,
	"kdp", &kdp_flag,
	"preempt", &default_preemption_rate,
	"htab", &hash_table_factlog2,
	"srv", &srv,
	"ncl", &ncl,
	"symtab", &symbol_table_addr,
	0,0,
};

#define	NUM	0
#define	STR	1

parse_boot_args(char *args)
{
	extern char init_args[];
	char		*cp, c;
	char		*namep;
	struct kernargs *kp;
	extern int boothowto;
	int i;
	int val;

	if (*args == 0) return 1;

	while(isargsep(*args)) args++;

	while (*args)
	{
		if (*args == '-')
		{
			char *ia = init_args;

			argstrcpy(args, init_args);
			do {
				switch (*ia) {
				    case 'a':
					boothowto |= RB_ASKNAME;
					break;
				    case 'b':
					boothowto |= RB_NOBOOTRC;
					break;
				    case 's':
					boothowto |= RB_SINGLE;
					break;
				    case 'i':
					boothowto |= RB_INITNAME;
					break;
				    case 'p':
					boothowto |= RB_DEBUG;
					break;
				    case 'd':
					boothowto |= RB_KDB;
					break;
				    case 'f':
#define RB_NOFP		0x00200000	/* don't use floating point */
				    	boothowto |= RB_NOFP;
					break;
			            case 'h':
				      /* break in prom for debugging */
				      boothowto |= RB_HALT;
				      break;
			     }
			} while (*ia && !isargsep(*ia++));
		}
		else
		{
			cp = args;
			while (!isargsep (*cp) && *cp != '=')
				cp++;
			if (*cp != '=')
				goto gotit;

			c = *cp;
			for(kp=kernargs;kp->name;kp++) {
				i = cp-args;
				if (strncmp(args, kp->name, i))
					continue;
				while (isargsep (*cp))
					cp++;
				if (*cp == '=' && c != '=') {
					args = cp+1;
					goto gotit;
				}

				switch (getval(cp, &val)) 
				{
					case NUM:
						*kp->i_ptr = val;
						break;
					case STR:
						argstrcpy(++cp, kp->i_ptr);
						break;
				}
				goto gotit;
			}
		}
gotit:
		/* Skip over current arg */
		while(!isargsep(*args)) args++;

		/* Skip leading white space (catch end of args) */
		while(*args && isargsep(*args)) args++;
	}

	return 0;
}

isargsep(c)
char c;
{
	if (c == ' ' || c == '\0' || c == '\t' || c == ',')
		return(1);
	else
		return(0);
}

argstrcpy(from, to)
char *from, *to;
{
	int i = 0;

	while (!isargsep(*from)) {
		i++;
		*to++ = *from++;
	}
	*to = 0;
	return(i);
}

getval(s, val)
register char *s;
int *val;
{
	register unsigned radix, intval;
	register unsigned char c;
	int sign = 1;

	if (*s == '=') {
		s++;
		if (*s == '-')
			sign = -1, s++;
		intval = *s++-'0';
		radix = 10;
		if (intval == 0)
			switch(*s) {

			case 'x':
				radix = 16;
				s++;
				break;

			case 'b':
				radix = 2;
				s++;
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				intval = *s-'0';
				s++;
				radix = 8;
				break;

			default:
				if (!isargsep(*s))
					return (STR);
			}
		for(;;) {
			if (((c = *s++) >= '0') && (c <= '9'))
				c -= '0';
			else if ((c >= 'a') && (c <= 'f'))
				c -= 'a' - 10;
			else if ((c >= 'A') && (c <= 'F'))
				c -= 'A' - 10;
			else if (isargsep(c))
				break;
			else
				return (STR);
			if (c >= radix)
				return (STR);
			intval *= radix;
			intval += c;
		}
		*val = intval * sign;
		return (NUM);
	}
	*val = 1;
	return (NUM);
}
