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

#include <powermac.h>
#include <mach/machine/vm_types.h>
#include <mach/machine.h>
#include <mach/vm_param.h>
#include <mach/thread_status.h>
#include <kern/thread.h>
#include <kern/mach_header.h>
#include <kern/assert.h>
#include <mach-o/loader.h>

#include <vm/pmap.h>
#include <machdep/ppc/cpu_data.h>
#include <machdep/ppc/kernBootStruct.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/boot.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>
#include <machdep/ppc/mem.h>
#include <machdep/ppc/DeviceTree.h>
#include <machdep/ppc/exception.h>
#include <mach/ppc/exception.h>
#include <bsd/sys/reboot.h>
#include <bsd/sys/msgbuf.h>
#import <sys/buf.h>
#import <stdlib.h>

/* External references */
extern thread_t setup_main();
extern void	DTInit(void *base);
extern void	identify_machine1(void);
extern void	identify_machine2(void);
extern void 	initialize_interrupts(void);
extern void 	initialize_serial(void);
extern void	initialize_screen(void *);

/* Local references */
void initialize_vm();
void initialize_debugger();
void configure_platform();
void initialize_rtclock();
void initialize_processors();
void initialize_keyboard();
void initialize_network();
void display_kernel_version(void);
void display_machine_class(void);
void display_machine_info(void);
void copy_boot_args(boot_args *our_args, boot_args *passed_args);
void pause(int seconds);

/* Globals */
vm_offset_t		istackptr = 0;	  /* mark stack as occupied initially */
boot_args		our_boot_args;    /* The boot args passed in from the booter (SecondaryLoader) */
powermac_info_t		powermac_info;    /* powermac specific info */
powermac_machine_info_t powermac_machine_info; /* for host_machine_info */
powermac_io_info_t	powermac_io_info; /* powermac io specific info */
powermac_init_t		*powermac_init_p; /* powermac initialization routines */
int			master_cpu;	  /* used by the kernel to specify which cpu is the master */
struct msgbuf		temp_msgbuf;	  /* EV 1997.02.25 Needs to be dynamically allocated */
powermac_dbdma_channels_t *powermac_dbdma_channels; /* Global storage for dbdma channel numbers */

// bit 0 => debugger break at start
// bit 1 => enable modem port kprintf()
int kdp_flag = 0;

/*
 * EV 1997.03.06
 *
 * This is the first function to be called. (See start.s)
 */
void ppc_init(boot_args *args)
{
	unsigned int mem_size;
	thread_t t;
	volatile unsigned char *dataReg;
	int i;

	copy_boot_args(&our_boot_args, args);
	/*
	 * The booter doesn't pass the total
	 * memory size yet, so fake it. (EV 1997.02.21)
	 */
	mem_size = 0;

	/* 
	 * parse_boot_args:
	 *   Parse arguments.  Depending on boot arguments, can set globals:
	 *      boothowto, rootdevice, kdp_flag, mem_size, bufpages, and nbuf
	 */
	parse_boot_args(our_boot_args.CommandLine);

	/* EV 1997.02.25 Needs to be dynamically allocated */
	msgbufp = &temp_msgbuf;

	if (our_boot_args.Revision != 0) {
	    DTInit(our_boot_args.deviceTreeP);
	}

	/* Figure out which machine class this is. */
	identify_machine1();

	/* Set up BATs and other processor related items */
	initialize_processors();

	// Translations are now on, and the system is running off of BATs.

	/* Setup powermac_info and powermac_machine_info structures */
	identify_machine2();

	/* Get kprintf's working */
	initialize_serial();

	/* Get printf's working */
	initialize_screen((void *) &our_boot_args.Video);

	display_kernel_version();
	display_machine_class();

	display_machine_info();

	/* Startup network for debugging purposes */
	initialize_network();

	initialize_vm();

	initialize_debugger();

	/* 
	 * configure_platform:
	 *   Give the PExpert a chance to set up anything specific to the platform.
	 */
	configure_platform();

	initialize_interrupts();

	if (kdp_flag & 1)  {
		call_kdp();
		//miniMonGdb("testing"); 
	}

	/*
	 * Start the system.
	 */
	t = setup_main();
	start_initial_context(t);

	/* Should never return */
	panic("return from start_initial_context\n");
}


void initialize_network()
{
#if GDB
	(*(*powermac_init_p).machine_initialize_network)();
        /* for use of kgdb, it's still too early to call kdp_raise_exception() */
#endif
}

void configure_platform()
{
	cpu_subtype_t  t;


	/*
	 * global: master_cpu.  Used by the kernel.
	 * Determines which cpu will keep time, etc.
	 */
	master_cpu = 0;

	/* XXX - we assume 1 cpu */
	machine_slot[0].is_cpu = TRUE;
	machine_slot[0].running = TRUE;

	/*
	 * We return the processor info, nothing on the machine
	 */

	machine_slot[0].cpu_type = CPU_TYPE_POWERPC;
	switch (PROCESSOR_VERSION) {
	case PROCESSOR_VERSION_601:		/* 601 */
		t = CPU_SUBTYPE_POWERPC_601; break;
	case PROCESSOR_VERSION_603:		/* 603 - Wart */
		t = CPU_SUBTYPE_POWERPC_603; break;
	case PROCESSOR_VERSION_604:		/* 604 - Zephyr */
		t = CPU_SUBTYPE_POWERPC_604; break;
	case 5:					/* 602 - Galahad */
		t = CPU_SUBTYPE_POWERPC_602; break;
	case PROCESSOR_VERSION_603e:		/* 603e - Stretch */
		t = CPU_SUBTYPE_POWERPC_603e; break;
	case 7:					/* 603ev - Valiant */
		t = CPU_SUBTYPE_POWERPC_603ev; break;
	case PROCESSOR_VERSION_604e:		/* 604e - Sirocco, Helmwind */
	case PROCESSOR_VERSION_604ev:           /* 604ev - MachV too! */
		t = CPU_SUBTYPE_POWERPC_604e; break;
	case PROCESSOR_VERSION_750:
		t = CPU_SUBTYPE_POWERPC_750; break;
	default:
		t = CPU_SUBTYPE_POWERPC_ALL; break;
	}
	machine_slot[0].cpu_subtype = t;

	machine_info.max_cpus = NCPUS;
	machine_info.avail_cpus = 1;
	machine_info.memory_size = mem_size;

	/* Call the platform specific configure function. */
	(*(*powermac_init_p).configure_machine)();
}

/*
 * initialize_rtclock:
 *    Do and platform wide rtclock initialization.  Then call machine
 *    specific initialization.
void initialize_rtclock()
{
        (*(*powermac_init_p).machine_initialize_rtclock)();
}
 */

/*
 * initialize_processors:
 *    Do any platform wide processor initialization.
 *    Call machine specific processor initialization function.
 */
void initialize_processors()
{
	// Setup platform bats
	initialize_bats( &our_boot_args);

	// Setup machine specific processor items, if any
	(*(*powermac_init_p).machine_initialize_processors)( &our_boot_args);
}

void initialize_keyboard()
{
}

void initialize_vm()
{
    /*
     * Set the boot-time vm PAGE_SIZE before using
     * any funny macros.
     */
    if (!page_size)
        page_size = /* 2* */PPC_PGBYTES;
    vm_set_page_size();

    /*
     * VM initialization, after this we're using page tables...
     */
    ppc_vm_init(mem_size, &our_boot_args);
}

void initialize_debugger()
{
#if     MACH_KDB

    /*
     * Initialize the kernel debugger.
     */
    ddb_init();

#endif /* MACH_KDB */

#if     MACH_KDB || MACH_KGDB
    /*
     * Cause a breakpoint trap to the debugger before proceeding
     * any further if the proper option bit was specified in
     * the boot flags.
     *
     * XXX use -a switch to invoke kdb, since there's no
     *     boot-program switch to turn on RB_HALT!
     */
    if (halt_in_debugger) {
            printf("inline call to debugger(machine_startup)\n");
            Debugger("");
    }
#endif /* MACH_KDB || MACH_KGDB */
}

extern const char version[];

void display_kernel_version(void)
{
	printf(version);
}


void display_machine_class(void)
{
	char *str;

    switch (powermac_info.class) {
    case    POWERMAC_CLASS_PDM:
        str = "NuBus";
        break;

    case    POWERMAC_CLASS_POWERSURGE:
        str = "POWERSURGE";
        break;

    case    POWERMAC_CLASS_POWEREXPRESS:
        str = "PowerExpress";
        break;

    case    POWERMAC_CLASS_GOSSAMER:
        str = "Gossamer";
        break;

    case    POWERMAC_CLASS_POWERSTAR:
        str = "PowerStar";
        break;

    case    POWERMAC_CLASS_PERFORMA:
        str = "Performa (unsupported)";
        break;

    case    POWERMAC_CLASS_POWERBOOK:
        str = "PowerBook (unsupported)";
        break;

    case    POWERMAC_CLASS_YOSEMITE:
        str = "Yosemite";
        break;

    default:
        str = "unsupported";
        break;
    }
    printf("MACH microkernel is booting on a "
           "Power Macintosh %s class machine\n", str );
}

void display_machine_info(void)
{
  printf("CPU Frequency: %9d Hz\n", powermac_machine_info.cpu_clock_rate_hz);
  printf("BUS Frequency: %9d Hz\n", powermac_machine_info.bus_clock_rate_hz);

  if (powermac_machine_info.l2_clock_rate_hz != 0)
    printf("L2 Frequency:  %9d Hz\n", powermac_machine_info.l2_clock_rate_hz);

#if 0
  kprintf("g_raw_cpu:         %9d\n", g_raw_cpu);
  kprintf("g_raw_bus:         %9d\n", g_raw_bus);

  kprintf("cpu_clock_rate_hz: %9d\n", powermac_machine_info.cpu_clock_rate_hz);
  kprintf("l2_clock_rate_hz:  %9d\n", powermac_machine_info.l2_clock_rate_hz);
  kprintf("bus_clock_rate_hz: %9d\n", powermac_machine_info.bus_clock_rate_hz);
  kprintf("dec_clock_rate_hz: %9d\n", powermac_machine_info.dec_clock_rate_hz);
  
  kprintf("cpu_pll: %d\n", powermac_machine_info.cpu_pll);
  kprintf("l2_pll:  %d\n", powermac_machine_info.l2_pll);

  kprintf("l2_cache_type: %d\n", powermac_machine_info.l2_cache_type);
  kprintf("l2_cache_size: %d\n", powermac_machine_info.l2_cache_size);
#endif
}


void
copy_boot_args(boot_args *our_args, boot_args *passed_args)
{
	int i;

	/* 
	 * Copy boot_args argument to safe place
	 */
	if (passed_args->Version == 1) {
		our_args->Version = passed_args->Version;
		if (passed_args->Revision == 0) {
			our_args->Revision = passed_args->machineType;
		} else {
			our_args->Revision = passed_args->Revision;
		}
		for (i = 0; i < BOOT_LINE_LENGTH; i++) {
			our_args->CommandLine[i] = passed_args->CommandLine[i];
		}
		for (i = 0; i < kMaxDRAMBanks; i++) {
			our_args->PhysicalDRAM[i].base =
					passed_args->PhysicalDRAM[i].base;
			our_args->PhysicalDRAM[i].size =
					passed_args->PhysicalDRAM[i].size;
		}
		our_args->Video.v_baseAddr = passed_args->Video.v_baseAddr;
		our_args->Video.v_display = passed_args->Video.v_display;
		our_args->Video.v_rowBytes = passed_args->Video.v_rowBytes;
		our_args->Video.v_width = passed_args->Video.v_width;
		our_args->Video.v_height = passed_args->Video.v_height;
		our_args->Video.v_depth = passed_args->Video.v_depth;
		our_args->machineType = 0;	// not used
		if (our_args->Revision == 1) {
		    our_args->deviceTreeP = passed_args->deviceTreeP;
		    our_args->deviceTreeLength = passed_args->deviceTreeLength;
		    our_args->topOfKernelData = passed_args->topOfKernelData;
		} else {
		    our_args->deviceTreeP = 0;
		    our_args->deviceTreeLength = 0;
		    our_args->topOfKernelData = 0;
		}
	} else {
		printf("Infinite Loop in ppc_init()\n");
		for (;;) {
		}
	}
}

void NO_ENTRY() { }

void pause(int seconds)
{
    int x, y, z;

    for (x = 0; x < (500000*seconds); x++)
	for (y = 0; y < 100; y++)
	    z = x ^ y;
}


#include <bsd/sys/errno.h>

/* PPC specific sysctl calls */
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
