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
 * Copyright (c) 1992, 1993, 1994 NeXT Computer, Inc.
 *
 * Early machine initialization.
 *
 * HISTORY
 *
 * 20 May 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <kern/mach_header.h>

#import <vm/vm_page.h>

#import <machdep/i386/cpu_inline.h>

#import	<machdep/i386/pmap.h>
#import <machdep/i386/gdt.h>
#import <machdep/i386/idt.h>
#import <machdep/i386/configure.h>
#import <machdep/i386/kernBootStruct.h>
#include <sys/reboot.h>
#include <sys/errno.h>
#import "uxpr.h"
#import "xpr_debug.h"

extern char rootdevice[];
extern int nbuf;
extern int srv;
extern int ncl;
static unsigned int maxmem;
static int subtype = 0;

struct kernargs {
	char *name;
	int *i_ptr;
} kernargs[] = {
	"nbuf", &nbuf,
	"rootdev", (int*) rootdevice,
	"maxmem", &maxmem,
	"subtype", &subtype,
	"srv", &srv,
	"ncl", &ncl,
	0,0,
};

#define	NBOOTFILE	64
char	boot_file[NBOOTFILE + 1];

int			master_cpu;
cpu_conf_t		cpu_config;

static vm_offset_t	first_addr, last_addr;
static vm_offset_t	first_addr0, last_addr0;

vm_offset_t		virtual_avail, virtual_end;

struct mem_region	mem_region[2];
int			num_regions;

vm_size_t		mem_size;

/* parameters passed from bootstrap loader */
int cnvmem = 0;		/* must be in .data section */
int extmem = 0;

#import <sys/msgbuf.h>

static void	zero_fill_data(void), size_memory(void),
		machine_configure(void), idt_page_protect(void);

vm_offset_t	alloc_cnvmem(
			vm_size_t	size,
			vm_offset_t	align);

thread_saved_state_t	*___xxx_state;  /* Just a placeholder */

/*
 * Called from Protected mode,
 * flat 3G segmentation, paging disabled,
 * interrupts disabled.
 */
i386_init(void)
{
    register mem_region_t	rp;
	KERNBOOTSTRUCT *kernBootStruct = KERNSTRUCT_ADDR;

    cnvmem = kernBootStruct->convmem;
    extmem = kernBootStruct->extmem;

    // Clear "zero fill" data sections.

    zero_fill_data();
    
    getargs(kernBootStruct->bootString);

    // Figure out what kind of cpu is present
    
    machine_configure();
	
    // Initialize the interrupt system.

    intr_initialize();
    
    // Calibrate the constant for us_spin().

    us_spin_calibrate();

    // Set the MI system page size	

    page_size = 2*I386_PGBYTES;
    vm_set_page_size();

    // Enable DDM tracing
#if	UXPR && XPR_DEBUG
	IOInitDDM(512);
#endif	UXPR && XPR_DEBUG

    // Figure out how much extended memory the system contains.

    size_memory();

    // Configure memory regions.

    num_regions			= 1;

    rp				= (mem_region_t) &mem_region[0];
    rp->base_phys_addr =
	rp->first_phys_addr	= first_addr;
    rp->last_phys_addr		= last_addr;

    /*
     * Place the idt on its own page to
     * enable working around the 'f00f' bug.
     */
    idt_copy(alloc_cnvmem(I386_PGBYTES, I386_PGBYTES), I386_PGBYTES);

    // Initialize kernel physical map.

    pmap_bootstrap(mem_region, num_regions, &virtual_avail, &virtual_end);

    //
    // Relocate the GDT & IDT
    // to reside in the kernel
    // address space.

    locate_gdt((vm_offset_t) gdt + KERNEL_LINEAR_BASE);
    locate_idt((vm_offset_t) idt + KERNEL_LINEAR_BASE);
    
    //
    // Set up the handler
    // for double bus faults
    
    dbf_init();

    /*
     * Write protect the page containing
     * the IDT as part of the 'f00f'
     * hack/workaround.
     */
    idt_page_protect();

    // Specify address of console message buffer.

    mem_region[0].last_phys_addr -= round_page(sizeof (struct msgbuf));
    msgbufp =
	(struct msgbuf *)pmap_phys_to_kern(mem_region[0].last_phys_addr);
}

static void
idt_page_protect(void)
{
    pt_entry_t	*pte = pmap_pt_entry(kernel_pmap, (vm_offset_t) idt);

    pte->prot = PT_PROT_KR;
    flush_cache(); flush_tlb();
}

static
void
zero_fill_data(void)
{
    struct segment_command	*sgp;
    struct section		*sp;
    
    sgp = getsegbyname("__DATA");
    if (!sgp)
	return;
	
    sp = firstsect(sgp);
    if (!sp)
	return;
	
    do {
	if (sp->flags & S_ZEROFILL)
	    bzero(sp->addr, sp->size);
    } while (sp = nextsect(sgp, sp));
}

static inline
boolean_t
is486_or_higher(void)
{
    unsigned int	efl;
    
    efl = eflags();

    efl |= EFL_AC;
    set_eflags(efl);
    
    if ((efl & EFL_AC) == 0)
    	return (FALSE);
	
    efl &= ~EFL_AC;
    set_eflags(efl);

    return (TRUE);
}

/*
 * This stuff belongs elsewhere
 * (.h files), but I can't put
 * it there due to the confidentiality
 * of P5 information.
 * 	- 17 September 92
 */
typedef struct _cpuid {
    unsigned int	step	:4,
    			model	:4,
			family	:4,
			type	:2,
				:18;
} cpuid_t;

static
cpuid_t
cpuid(void)
{
    cpuid_t	value;
    
    asm volatile(
	"movl $1,%%eax; cpuid"
	    : "=a" (value)
	    :
	    : "ebx", "ecx", "edx");
	    
    return (value);
}

#define EFL_ID		0x200000	/* P5 specific */

static
cpuid_t
get_cpuid(void)
{
    unsigned int	efl;
    cpuid_t		pid = { 0 };

    if (subtype != 0) {
       /*
	* This is a hack, intended only
	* for testing purposes.
	*/
	pid.family = CPU_SUBTYPE_INTEL_FAMILY(subtype);
	pid.model = CPU_SUBTYPE_INTEL_MODEL(subtype);

	return pid;
    }
    
    efl = eflags();

    efl |= EFL_ID;
    set_eflags(efl);
    
    efl = eflags();
    
    if ((efl & EFL_ID) == 0)
    	return pid;
	
    pid = cpuid();
	
    efl &= ~EFL_ID;
    set_eflags(efl);

    return pid;
}

char cpu_model[65];
char machine[65];

static
void
machine_configure(void)
{
    cpuid_t	pid;

    while (!is486_or_higher())
	asm volatile("hlt");
			
    enable_cache();
    
    fp_configure();

    machine_slot[0].is_cpu = TRUE;
    machine_slot[0].running = TRUE;

    machine_slot[0].cpu_type = CPU_TYPE_I386;
    (void) strcpy(machine, "i386");

    pid = get_cpuid();
    switch (pid.family) {
    
    case 0:
	if (cpu_config.fpu_type == FPU_HDW) {
	    machine_slot[0].cpu_subtype = CPU_SUBTYPE_486;
	    (void) strcpy(cpu_model, "486");
	}
	else {
	    machine_slot[0].cpu_subtype = CPU_SUBTYPE_486SX;
	    (void) strcpy(cpu_model, "486SX");
	}
	break;

    case 5:
	machine_slot[0].cpu_subtype = CPU_SUBTYPE_586;
	(void) strcpy(cpu_model, "586");
	break;

    default:
	machine_slot[0].cpu_subtype = CPU_SUBTYPE_INTEL(pid.family, pid.model);
	(void) sprintf(cpu_model, "i86 family %d, model %d",
		       pid.family, pid.model);
	break;
    }
}

static
void
size_memory(void)
{
    KERNBOOTSTRUCT	*kernBootStruct = (KERNBOOTSTRUCT *)KERNSTRUCT_ADDR;
    vm_offset_t		end_of_image, end_of_memory;
    int			i;
#define KB(x)		((x)*1024)

    end_of_image = getlastaddr();

    for (i=0; i < kernBootStruct->numBootDrivers; i++)
	end_of_image += kernBootStruct->driverConfig[i].size;
    
    if (maxmem)
	end_of_memory = KB(maxmem);
    else
	end_of_memory = KB(extmem);

    /*
     * This is the Mach notion of
     * how much physical memory the
     * machine contains.  This value
     * is only used for informational
     * purposes.
     */

    mem_size = end_of_memory;
    
    first_addr0 = round_page(kernBootStruct->first_addr0);
    last_addr0 = trunc_page(KB(cnvmem));

    first_addr = round_page(end_of_image);
    last_addr = trunc_page(end_of_memory);
#undef	KB
}

vm_offset_t
bios_extdata_addr(void)
{
    return (cnvmem * 1024);
}

vm_offset_t
alloc_cnvmem(
    vm_size_t	size,
    vm_offset_t	align
)
{
    static vm_offset_t	free_ptr, end_ptr;
    vm_offset_t		p;
    
    if (!free_ptr) {
	free_ptr = pmap_phys_to_kern(first_addr0);
	end_ptr = pmap_phys_to_kern(last_addr0);
    }
	
    p = (free_ptr + (align - 1)) & ~(align - 1);
    if ((p + size) > end_ptr)
	return ((vm_offset_t) 0);
	
    free_ptr = (p + size);
    
    return (p);
}

vm_offset_t
alloc_pages(
    vm_size_t	size
)
{
    vm_offset_t		p;

    {	/* XXX */
	extern boolean_t	pmap_initialized;
    
	if (pmap_initialized)
	    panic("alloc_pages");

	/* XXX */
    }
    
    size = round_page(size);
    p = pmap_phys_to_kern(mem_region[0].first_phys_addr);
    mem_region[0].first_phys_addr += size;
    
    return (p);
}

#define	NUM	0
#define	STR	1

getargs(char *args)
{
	extern char init_args[];
	char		*cp, c;
	char		*namep;
	struct kernargs *kp;
	extern int boothowto;
	int i;
	int val;

	KERNBOOTSTRUCT *kernBootStruct = (KERNBOOTSTRUCT *)KERNSTRUCT_ADDR;
	strncpy(boot_file, kernBootStruct->boot_file, NBOOTFILE);

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
				    case 's':
					boothowto |= RB_SINGLE;
					break;
				    case 'd':
					boothowto |= RB_KDB;
					break;
				    case 'f':
#define RB_NOFP		0x00200000	/* don't use floating point */
				    	boothowto |= RB_NOFP;
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

/* i386 specific sysctl calls */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/*
	 * nothing defined yet
	 */
	return (EOPNOTSUPP);
}

