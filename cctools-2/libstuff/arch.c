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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <mach/mach.h>
#include "stuff/openstep_mach.h"
#include "stuff/arch.h"

/*
 * The array of all currently know architecture flags (terminated with an entry
 * with all zeros).  Pointer to this returned with get_arch_flags().
 */
#ifdef __DYNAMIC__
static struct arch_flag arch_flags[] = {
#else
static const struct arch_flag arch_flags[] = {
#endif
    { "any",	CPU_TYPE_ANY,	  CPU_SUBTYPE_MULTIPLE },
    { "little",	CPU_TYPE_ANY,	  CPU_SUBTYPE_LITTLE_ENDIAN },
    { "big",	CPU_TYPE_ANY,	  CPU_SUBTYPE_BIG_ENDIAN },
    /* architecture families */
    { "ppc",    CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL },
    { "i386",   CPU_TYPE_I386,    CPU_SUBTYPE_I386_ALL },
    { "m68k",   CPU_TYPE_MC680x0, CPU_SUBTYPE_MC680x0_ALL },
    { "hppa",   CPU_TYPE_HPPA,    CPU_SUBTYPE_HPPA_ALL },
    { "sparc",	CPU_TYPE_SPARC,   CPU_SUBTYPE_SPARC_ALL },
    { "m88k",   CPU_TYPE_MC88000, CPU_SUBTYPE_MC88000_ALL },
    { "i860",   CPU_TYPE_I860,    CPU_SUBTYPE_I860_ALL },
    /* specific architecture implementations */
    { "ppc601", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_601 },
    { "ppc603", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603 },
    { "ppc603e",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603e },
    { "ppc603ev",CPU_TYPE_POWERPC,CPU_SUBTYPE_POWERPC_603ev },
    { "ppc604", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604 },
    { "ppc604e",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604e },
    { "ppc750", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_750 },
    { "i486",   CPU_TYPE_I386,    CPU_SUBTYPE_486 },
    { "i486SX", CPU_TYPE_I386,    CPU_SUBTYPE_486SX },
    { "pentium",CPU_TYPE_I386,    CPU_SUBTYPE_PENT }, /* same as i586 */
    { "i586",   CPU_TYPE_I386,    CPU_SUBTYPE_586 },
    { "pentpro", CPU_TYPE_I386, CPU_SUBTYPE_PENTPRO },
    { "pentIIm3",CPU_TYPE_I386, CPU_SUBTYPE_PENTII_M3 },
    { "pentIIm5",CPU_TYPE_I386, CPU_SUBTYPE_PENTII_M5 },
    { "m68030", CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68030_ONLY },
    { "m68040", CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68040 },
    { "hppa7100LC", CPU_TYPE_HPPA,  CPU_SUBTYPE_HPPA_7100LC },
    { NULL,	0,		  0 }
};

/*
 * get_arch_from_flag() is passed a name of an architecture flag and returns
 * zero if that flag is not known and non-zero if the flag is known.
 * If the pointer to the arch_flag is not NULL it is filled in with the
 * arch_flag struct that matches the name.
 */
__private_extern__
int
get_arch_from_flag(
char *name,
struct arch_flag *arch_flag)
{
    unsigned long i;

	for(i = 0; arch_flags[i].name != NULL; i++){
	    if(strcmp(arch_flags[i].name, name) == 0){
		if(arch_flag != NULL)
		    *arch_flag = arch_flags[i];
		return(1);
	    }
	}
	if(arch_flag != NULL)
	    memset(arch_flag, '\0', sizeof(struct arch_flag));
	return(0);
}

/*
 * get_arch_from_host() gets the architecture from the host this is running on
 * and returns zero if the architecture is not known and zero if the
 * architecture is known.  If the parameters family_arch_flag and
 * specific_arch_flag are not NULL they get fill in with the family
 * architecture and specific architecure for the host.  If the architecture
 * is unknown and the parameters are not NULL then all fields are set to zero.
 */
__private_extern__
int
get_arch_from_host(
struct arch_flag *family_arch_flag,
struct arch_flag *specific_arch_flag)
{
    struct host_basic_info host_basic_info;
    unsigned int count;
    kern_return_t r;

	if(family_arch_flag != NULL)
	    memset(family_arch_flag, '\0', sizeof(struct arch_flag));
	if(specific_arch_flag != NULL)
	    memset(specific_arch_flag, '\0', sizeof(struct arch_flag));

	count = HOST_BASIC_INFO_COUNT;
	if((r = host_info(mach_host_self(), HOST_BASIC_INFO,
			  (host_info_t)(&host_basic_info),
			  &count)) != KERN_SUCCESS)
	    return(0);

	if(family_arch_flag != NULL){
	    family_arch_flag->cputype = host_basic_info.cpu_type;
	}
	if(specific_arch_flag != NULL){
	    specific_arch_flag->cputype = host_basic_info.cpu_type;
	    specific_arch_flag->cpusubtype = host_basic_info.cpu_subtype;
	}
	switch(host_basic_info.cpu_type){
	case CPU_TYPE_MC680x0:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_MC680x0_ALL:
	    case CPU_SUBTYPE_MC68030_ONLY:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "m68k";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_MC680x0_ALL;
		}
		if(specific_arch_flag != NULL){
		    specific_arch_flag->name = "m68030";
		    /* 
		     * There is a "bug" in the kernel for compatiblity that on
		     * an 030 machine host_info() returns cpusubtype
		     * CPU_SUBTYPE_MC680x0_ALL and not CPU_SUBTYPE_MC68030_ONLY.
		     */
		    specific_arch_flag->cpusubtype = CPU_SUBTYPE_MC68030_ONLY;
		}
		return(1);
	    case CPU_SUBTYPE_MC68040:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "m68k";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_MC680x0_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "m68040";
		return(1);
	    }
	    break;
	case CPU_TYPE_POWERPC:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_POWERPC_ALL:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc";
		return(1);
	    case CPU_SUBTYPE_POWERPC_601:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc601";
		return(1);
	    case CPU_SUBTYPE_POWERPC_603:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc603";
		return(1);
	    case CPU_SUBTYPE_POWERPC_603e:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc603e";
		return(1);
	    case CPU_SUBTYPE_POWERPC_603ev:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc603ev";
		return(1);
	    case CPU_SUBTYPE_POWERPC_604:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc604";
		return(1);
	    case CPU_SUBTYPE_POWERPC_604e:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc604e";
		return(1);
	    case CPU_SUBTYPE_POWERPC_750:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "ppc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "ppc750";
		return(1);
	    default:
		if(family_arch_flag != NULL){
                    family_arch_flag->name = "ppc";
                    family_arch_flag->cpusubtype = CPU_SUBTYPE_POWERPC_ALL;
                }
                if(specific_arch_flag != NULL){
                    specific_arch_flag->name =
			malloc(sizeof("PowerPC cpusubtype ") + 10);
                    if(specific_arch_flag->name != NULL)
			sprintf(specific_arch_flag->name,
				"PowerPC cpusubtype %u", 
				host_basic_info.cpu_subtype);
		}
                return(1);
	    }
	    break;
	case CPU_TYPE_MC88000:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_MC88000_ALL:
	    case CPU_SUBTYPE_MC88110:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "m88k";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_MC88000_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "m88k";
		return(1);
	    }
	    break;
	case CPU_TYPE_I386:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_I386_ALL:
	    /* case CPU_SUBTYPE_386: same value as above */
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "i386";
		return(1);
	    case CPU_SUBTYPE_486:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "i486";
		return(1);
	    case CPU_SUBTYPE_486SX:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "i486SX";
		return(1);
	    case CPU_SUBTYPE_PENT: /* same as CPU_SUBTYPE_586 */
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "pentium";
		return(1);
	    case CPU_SUBTYPE_PENTPRO:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "pentpro";
		return(1);
	    case CPU_SUBTYPE_PENTII_M3:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "pentIIm3";
		return(1);
	    case CPU_SUBTYPE_PENTII_M5:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "pentIIm5";
		return(1);
	    default:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i386";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I386_ALL;
                }
                if(specific_arch_flag != NULL){
                    specific_arch_flag->name =
			malloc(sizeof("Intel family  model ") + 2 + 8);
		    if(specific_arch_flag->name != NULL)
			sprintf(specific_arch_flag->name,
			    "Intel family %u model %u", 
			CPU_SUBTYPE_INTEL_FAMILY(host_basic_info.cpu_subtype),
			CPU_SUBTYPE_INTEL_MODEL(host_basic_info.cpu_subtype));
		}
                return(1);
	    }
	    break;
	case CPU_TYPE_I860:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_I860_ALL:
	    case CPU_SUBTYPE_I860_860:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "i860";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_I860_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "i860";
		return(1);
	    }
	    break;
	case CPU_TYPE_HPPA:
	    switch(host_basic_info.cpu_subtype){
	    case CPU_SUBTYPE_HPPA_ALL:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "hppa";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_HPPA_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "hppa";
		return(1);
	    case CPU_SUBTYPE_HPPA_7100LC:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "hppa";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_HPPA_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "hppa7100LC";
		return(1);
	      
	    }
	    break;
	case CPU_TYPE_SPARC:
	    switch(host_basic_info.cpu_subtype){
	    case /*CPU_SUBTYPE_SPARC_ALL*/0:
		if(family_arch_flag != NULL){
		    family_arch_flag->name = "sparc";
		    family_arch_flag->cpusubtype = CPU_SUBTYPE_SPARC_ALL;
		}
		if(specific_arch_flag != NULL)
		    specific_arch_flag->name = "sparc";
		return(1);
	    }
	    break;
	}
	return(0);
}

/*
 * get_arch_flags() returns a pointer to an array of all currently know
 * architecture flags (terminated with an entry with all zeros).
 */
__private_extern__
const struct arch_flag *
get_arch_flags(
void)
{
	return(arch_flags);
}

/*
 * get_arch_name_from_types() returns the name of the architecture for the
 * specified cputype and cpusubtype if known.  If unknown it returns a pointer
 * to the an allocated string "cputype X cpusubtype Y" where X and Y are decimal
 * values.
 */
__private_extern__
const char *
get_arch_name_from_types(
cpu_type_t cputype,
cpu_subtype_t cpusubtype)
{
    unsigned long i;
    char *p;

	for(i = 0; arch_flags[i].name != NULL; i++){
	    if(arch_flags[i].cputype == cputype &&
	       arch_flags[i].cpusubtype == cpusubtype)
		return(arch_flags[i].name);
	}
	p = malloc(sizeof("cputype  cpusubtype ") + 10 + 10);
	if(p != NULL)
	    sprintf(p, "cputype %u cpusubtype %u", cputype, cpusubtype);
	return(p);
}

/*
 * get_arch_family_from_cputype() returns the family architecture for the
 * specified cputype if known.  If unknown it returns NULL.
 */
__private_extern__
const struct arch_flag *
get_arch_family_from_cputype(
cpu_type_t cputype)
{
    unsigned long i;

	for(i = 0; arch_flags[i].name != NULL; i++){
	    if(arch_flags[i].cputype == cputype)
		return(arch_flags + i);
	}
	return(NULL);
}

/*
 * get_byte_sex_from_flag() returns the byte sex of the architecture for the
 * specified cputype and cpusubtype if known.  If unknown it returns
 * UNKNOWN_BYTE_SEX.  If the bytesex can be determined directly as in the case
 * of reading a magic number from a file that should be done and this routine
 * should not be used as it could be out of date.
 */
__private_extern__
enum byte_sex
get_byte_sex_from_flag(
const struct arch_flag *flag)
{
   if(flag->cputype == CPU_TYPE_MC680x0 ||
      flag->cputype == CPU_TYPE_MC88000 ||
      flag->cputype == CPU_TYPE_POWERPC ||
      flag->cputype == CPU_TYPE_HPPA ||
      flag->cputype == CPU_TYPE_SPARC ||
      flag->cputype == CPU_TYPE_I860)
        return BIG_ENDIAN_BYTE_SEX;
    else if(flag->cputype == CPU_TYPE_I386)
        return LITTLE_ENDIAN_BYTE_SEX;
    else
        return UNKNOWN_BYTE_SEX;
}

/*
 * get_stack_direction_from_flag() returns the direction the stack grows as
 * either positive (+1) or negative (-1) of the architecture for the
 * specified cputype and cpusubtype if known.  If unknown it returns 0.
 */
__private_extern__
long
get_stack_direction_from_flag(
const struct arch_flag *flag)
{
   if(flag->cputype == CPU_TYPE_MC680x0 ||
      flag->cputype == CPU_TYPE_MC88000 ||
      flag->cputype == CPU_TYPE_POWERPC ||
      flag->cputype == CPU_TYPE_I386 ||
      flag->cputype == CPU_TYPE_SPARC ||
      flag->cputype == CPU_TYPE_I860)
        return(-1);
    else if(flag->cputype == CPU_TYPE_HPPA)
        return(+1);
    else
        return(0);
}

/*
 * get_stack_addr_from_flag() returns the default starting address of the user
 * stack.  This should be in the header file <bsd/XXX/vmparam.h> as USRSTACK.
 * Since some architectures have come and gone and come back and because you
 * can't include all of these headers in one source the constants have been
 * copied here.
 */
__private_extern__
unsigned long
get_stack_addr_from_flag(
const struct arch_flag *flag)
{
    switch(flag->cputype){
    case CPU_TYPE_MC680x0:
	return(0x04000000);
    case CPU_TYPE_MC88000:
	return(0xffffe000);
    case CPU_TYPE_POWERPC:
	return(0xc0000000);
    case CPU_TYPE_I386:
	return(0xc0000000);
    case CPU_TYPE_SPARC:
	return(0xf0000000);
    case CPU_TYPE_I860:
	return(0);
    case CPU_TYPE_HPPA:
	return(0xc0000000-0x04000000);
    default:
	return(0);
    }
}

/*
 * get_stack_size_from_flag() returns the default size of the userstack.  This
 * should be in the header file <bsd/XXX/vmparam.h> as MAXSSIZ. Since some
 * architectures have come and gone and come back, you can't include all of
 * these headers in one source and some of the constants covered the whole
 * address space the common value of 64meg was chosen.
 */
__private_extern__
unsigned long
get_stack_size_from_flag(
const struct arch_flag *flag)
{
#ifdef __MWERKS__
    const struct arch_flag *dummy;
	dummy = flag;
#endif

    return(64*1024*1024);
}

/*
 * get_segalign_from_flag() returns the default segment alignment (page size).
 */
__private_extern__
unsigned long
get_segalign_from_flag(
const struct arch_flag *flag)
{
	if(flag->cputype == CPU_TYPE_POWERPC)
	    return(0x1000); /* 4K */
	else
	    return(0x2000); /* 8K */
}
