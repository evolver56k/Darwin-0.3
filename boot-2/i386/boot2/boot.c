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
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * 			INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license  agreement or 
 *	nondisclosure agreement with Intel Corporation and may not be copied 
 *	nor disclosed except in accordance with the terms of that agreement.
 *
 *	Copyright 1988, 1989 by Intel Corporation
 */

/*
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */
 
/*
 * Completely reworked by Sam Streeper (sam_s@NeXT.com)
 * Reworked again by Curtis Galloway (galloway@NeXT.com)
 */
#import "libsa.h"
#import "io_inline.h"
#import "memory.h"
#import "saio.h"
#import "libsaio.h"
#import "kernBootStruct.h"
#import "sarld.h"
#import "graphics.h"
#import "console.h"
#import "boot.h"
#import "zalloc.h"
#import "drivers.h"
#import "rcz.h"

#import <mach-o/nlist.h>
#import <sys/reboot.h>

BOOL		useDefaultConfig;

/* Path name of kernel. */
#define NAME_LENGTH	BOOT_STRING_LEN
static char	name[NAME_LENGTH];

/* Families of device drivers to consider loading
 * during installation.  A space-separated list.
 */
char	*LoadableFamilies;

/*
 * The user asked for boot graphics.
 */
BOOL	wantBootGraphics;

extern char	*gFilename;
extern BOOL sysConfigValid;
extern char bootPrompt[];
extern BOOL errors;
extern BOOL verbose_mode;

#if MULTIPLE_DEFAULTS
char	*default_names[] = {
	"$LBL",
};
#define NUM_DEFAULT_NAMES	(sizeof(default_names)/sizeof(char *))
int	current_default = 0;
#else
#define DEFAULT_NAME	"$LBL"
#endif

#if NOTYET
static char *YesNo[] = {"Yes", "No"};
#endif
static int getBootString(char *name);
static void pickLanguage(void);


static inline void skipblank(char **cp) 
{
	while (**(cp) == ' ' || **(cp) == '\t')
		++(*cp);
}

static inline int isKernel(char *cp)
{
	register char c;
	
	skipblank(&cp);
	c = *cp | 0x20;
	if ((c < 'a' || c > 'z') && ( c != '/' )) return 0;
	while (*cp && (*cp != '=') && (*cp != ' ') && (*cp != '\t'))
	    cp++;

	if (*cp == '=') return 0;
	return 1;
}

static void usage(void)
{
	int fd;
	char *help = "/usr/standalone/i386/BootHelp.txt";

	if ((fd = open(help, 0)) >= 0)
	{
		char *buffer = malloc(file_size(fd));
		read(fd, buffer, file_size(fd) - 1);
		close(fd);
		printf("%s",buffer);
		free(buffer);
	}
	else error(" \nError loading %s\n\n",help);
}

static void
zeroBSS(void)
{
	extern char	_DATA__bss__begin, _DATA__bss__end;
	extern char	_DATA__common__begin, _DATA__common__end;

	bzero(&_DATA__bss__begin,
		(&_DATA__bss__end - &_DATA__bss__begin));
	
	bzero(&_DATA__common__begin, 
		(&_DATA__common__end - &_DATA__common__begin));
}


static int
execKernel(int fd, int installMode)
{
	register KERNBOOTSTRUCT *kbp = kernBootStruct;
	register char *src = gFilename;
	register char *dst = kbp->boot_file;
	char *val, *linkerPath;
	static struct mach_header head;
	entry_t kernelEntry;
	int ret, size;
	int loadDrivers;
	
	while (*src && (*src != ' ' && *src != '\t'))
		*dst++ = *src++;
	*dst = 0;

	verbose("Loading %s\n", kbp->boot_file);

	/* perform the actual load */
	kbp->kaddr = kbp->ksize = 0;
	ret = loadprog(kbp->kernDev, fd, &head,
		&kernelEntry,
		(char **)&kbp->kaddr,
		&kbp->ksize);
	close(fd);
	
	/* Clear memory that might be used for loaded drivers
	 * because the standalone linker doesn't zero
	 * memory that is used later for BSS in the drivers.
	 */
	{
	    long addr = kbp->kaddr +
			kbp->ksize;
	    bzero((char *)addr, RLD_MEM_ADDR - addr);
	}
	
	clearActivityIndicator();
	printf("\n");

	if ( ret != 0 ) {
		return ret;
	}
	if ((getValueForKey("Kernel Flags", &val, &size)) && size) {
		int oldlen, len1;
		char *cp = kbp->bootString;
		oldlen = len1 = strlen(cp);

		// move out the user string
		for(; len1 >= 0; len1--)
			cp[size + len1] = cp[len1 - 1];
		strncpy(cp,val,size);
		if (oldlen) cp[strlen(cp)] = ' ';
	}

	message("Reading Rhapsody configuration",0);
	
	if ((linkerPath = newStringForKey("Linker")) == 0) {
	    linkerPath = "/usr/standalone/i386/sarld";
	}
	
	ret = loadStandaloneLinker(
	    linkerPath,
	    (sa_rld_t **)&kbp->rld_entry);
	if (ret == -1) {
	    error("Couldn't load standalone linker; "
		    "unable to load boot drivers.\n");
	    loadDrivers = 0;
	}
#if	0
		printf("finsihed loading sarld\n");
		sleep(0);
#endif	1
	
	if (getBoolForKey(PROMPT_KEY)) {
	    int checkfd;
	    
insert_again:
	    setMode(TEXT_MODE);
	    clearActivityIndicator();
	    clearScreen();
	    /* The text for the following message is in Localizable.strings. */
	    localPrintf("Insert Driver Disk");
	    flushdev();
	    while(getc() != '\r');
	    printf("\n");
	    
	    /* Check to see that they really inserted the driver disk. */
	    if ((checkfd = open("fd()/mach_kernel", 0)) >= 0 ||
		(checkfd = open("fd()/mach_kernel" RCZ_EXTENSION, 0)) >= 0) {
		close(checkfd);
		goto insert_again;
	    }
	}
	
#if	0
		printf("loading other configs\n");
		sleep(1);
#endif	1
	loadOtherConfigs(useDefaultConfig);
#if	0
		printf("completed\n");
		sleep(1);
#endif	1

	if (getBoolForKey(ASK_KEY)) {
#ifdef NOTYET
	    char *selected = popupBrowser(
		YesNo, 2,
		"Do you want to load boot drivers?",
		BROWSER_NO_MESSAGE,
		"Continue",
		BROWSER_CURRENT_IS_SELECTED);
	    loadDrivers = selected[0];
	    free(selected);
#else NOTYET
	    setMode(TEXT_MODE);
	    loadDrivers = 1;
#endif NOTYET
	} else {
	    loadDrivers = 0;
	}
	
	if (loadDrivers || driverMissing) {
	    int prompts = 0;

	    setMode(TEXT_MODE);		/* For now */
	    /*
	     * If we prompted for the driver disk,
	     * go straight to scanning for drivers.
	     * (Don't ask whether you want to load boot drivers.)
	     */
	    getIntForKey(NUM_PROMPTS_KEY, &prompts);
	    loadBootDrivers(!getBoolForKey(PROMPT_KEY), prompts, installMode);
	}
	if (Dev(kbp->kernDev) == DEV_FLOPPY)
	{
		if ((!getValueForBootKey(kbp->bootString, 
			"rootdev", &val, &size)) || 
			!strncmp("fd",val,2))
		{
			clearActivityIndicator();
			printf("\n");
#if NOTYET
			popupPanel(
			    "Insert file system media"
			    " and press Return");
#else NOTYET
			localPrintf("Insert file system media"
				" and press Return");
			while(getc() != '\r');
			printf("\n");
#endif NOTYET
		}
	}

	if (errors)
	{
		setMode(TEXT_MODE);
localPrintf("Errors encountered while starting up the computer.\n");
		localPrintf("Pausing %d seconds...\n",BOOT_TIMEOUT);
		sleep(BOOT_TIMEOUT);
	}
	
	if (wantBootGraphics)
	    setMode(GRAPHICS_MODE);
	message("Starting Rhapsody",0);

	removeLinkEditSegment(
	    (struct mach_header *)kbp->kaddr );
	
	if (kbp->eisaConfigFunctions)
	    kbp->first_addr0 = EISA_CONFIG_ADDR + 
		(kbp->eisaConfigFunctions * sizeof(EISA_func_info_t));

	clearActivityIndicator();
	turnOffFloppy();
	if (getBoolForKey("APM")) {
	    if (APMPresent()) {
		APMConnect32();
	    }
	}
	startprog(kernelEntry);
	/* Not reached */
	return 0;
}

static void scanHardware()
{
    int slot, function, fn_count;
    KERNBOOTSTRUCT *kbp = KERNSTRUCT_ADDR;
    EISA_slot_info_t *esp = kbp->eisaSlotInfo;
    EISA_func_info_t *efp = (EISA_func_info_t *)EISA_CONFIG_ADDR;
    EISA_func_info_t funcBuf;
    extern int ReadEISASlotInfo(EISA_slot_info_t *, int);
    extern int ReadEISAFuncInfo(EISA_func_info_t *, int, int);
    extern int ReadPCIBusInfo(PCI_bus_info_t *);
    extern void PCI_Bus_Init(PCI_bus_info_t *);

    bzero((char *)esp, sizeof(*esp) * NUM_EISA_SLOTS);
    fn_count = 0;
#ifdef EISA_SUPPORT
    if (eisa_present()) {
	for (slot = 0; slot < NUM_EISA_SLOTS; slot++, esp++) {
	    ReadEISASlotInfo(esp, slot);
	}
	for (slot = function = fn_count = 0;
	     fn_count < (EISA_CONFIG_LEN / sizeof(EISA_func_info_t)); ) {
    
	    /*
	     * Must use a structure on the stack because
	     * the BIOS function can't write to an address greater
	     * than 64k.
	     */
	    bzero((char *)&funcBuf, sizeof(funcBuf));
	    if (ReadEISAFuncInfo(&funcBuf, slot, function) == 0) {
		bcopy((char *)&funcBuf, (char *)efp, sizeof(funcBuf));
		function++;
		efp++;
		fn_count++;
	    } else {
		if (function == 0) {
		    break;
		} else {
		    slot++;
		    function = 0;
		}
	    }
	}
    }
#endif
    kbp->eisaConfigFunctions = fn_count;
    
    (void)ReadPCIBusInfo(&kbp->pciInfo);
    PCI_Bus_Init(&kbp->pciInfo);
}

static void malloc_error(int code)
{
    stop("Out of memory\n");
}

void
boot(int bootdev)
{
	register KERNBOOTSTRUCT *kbp = kernBootStruct;
	int		fd, size;
	char		*val;
	int		installMode;

	zeroBSS();
	
	/* turn on gate A20 to be able to access memory above 1 MB */
	setA20();

	setMode(TEXT_MODE);

	/* initialize boot info structure */
	getKernBootStruct();
	
	/* initialize malloc arena to top of conventional memory */
	malloc_init((char *)ZALLOC_ADDR, 
	    (kbp->convmem * 1024) - ZALLOC_ADDR,
	    ZALLOC_NODES);

	/* Check for insufficient memory */
	if (kbp->extmem < MIN_EXT_MEM_KB) {
	    printf("\nThis computer has only %d MB of memory (RAM).\n",
		(kbp->extmem + 1024) / 1024);
	    printf("Rhapsody requires at least %d MB of memory,\n",
		(MIN_EXT_MEM_KB + 1024) / 1024);
	    printf("so it can't run on this computer.\n");
	    turnOffFloppy();
	    halt();
	}
	
	malloc_error_init(malloc_error);

	scanHardware();
#if 0
	printf("\n\nRhapsody on Intel boot:\n");
#endif	
	printf(bootPrompt, kbp->convmem, kbp->extmem);
	printf( "Rhapsody will start up in %d seconds, or you can:\n"
"  Type -v and press Return to start up Rhapsody with diagnostic messages\n"
"  Type ? and press Return to learn about advanced startup options\n"
"  Type any other character to stop Rhapsody from starting up automatically\n",
		BOOT_TIMEOUT);

	/*  parse args, start kernel */
	for (;;) {
	    sysConfigValid = 0;
	    useDefaultConfig = 0;
	    showText = 1;
	    errors = 0;

	    setMode(TEXT_MODE);

	    if (bootdev==0) {
		    if (kbp->numIDEs > 0) {
			    kbp->kernDev = DEV_HD;
		    } else {
			    kbp->kernDev = DEV_SD;
			}
	    } else {
		    kbp->kernDev = DEV_FLOPPY;
	    }
	    flushdev();
	    
#if MULTIPLE_DEFAULTS
	    strcpy(name, default_names[current_default]);

	    if (++current_default == NUM_DEFAULT_NAMES)
		    current_default = 0;
#else
	    strcpy(name, DEFAULT_NAME);
#endif

	    getBootString(name);

	    /* To force loading config file off same device as kernel,
	     * open kernel file to force device change if necessary.
	     */
	    fd = open(name, 0);
	    if (fd >= 0)
		close(fd);
		
	    if (!sysConfigValid) {
		    val = 0;
		    getValueForBootKey(kbp->bootString, 
				    "config", &val, &size);

#if 0
		printf("sys config was not valid trying alt\n");
		sleep(1);
#endif 1
		    useDefaultConfig = loadSystemConfig(val,size);
	    }
		
	    if (!sysConfigValid) {
#if 0
		printf("sys config is not valid\n");
		sleep(1);
#endif 1
		    continue;
		}

	    if (getBoolForKey("Boot Graphics"))
	    {
		wantBootGraphics = YES;
	    } 

	    if (getBoolForKey(INSTALL_KEY)) {
		installMode = 1;
		pickLanguage();
	    } else {
		char *lang;
		
		installMode = 0;
		if ((lang = newStringForKey("Language")) != 0) {
		    setLanguage(lang);
		}
		if (wantBootGraphics)
		    setMode(GRAPHICS_MODE);
	    }
	    
	    /* Load language localization file */
	    loadLanguageFiles();

	    if (installMode) {
                int retv = 0;
		char *msg = "Really Install?";

		/* Do you REALLY want to install?? */
                while(retv != 1) {
		    clearScreen();
		    if ((retv = chooseSimple(&msg, 1, 1, 2)) == 2) {
		        printf("\n");
		        /* Localized string */
		        localPrintf("Install Canceled");
		        stop("");
		    }
                }
	    }

	    message("Loading Rhapsody",0);
	    if ((fd = openfile(name, 0)) >= 0) {
		/* If this returns, kernel load failed. */
#if 0
		printf("calling exec kernel\n");
		sleep(1);
#endif 1
		execKernel(fd, installMode);
	    } else {
		error("Can't find %s\n", name);
		if (bootdev != 0) {
		    // floppy in drive, but failed to load kernel
message("Couldn't start up the computer using this floppy disk.",0);
		    bootdev = 0; errors = 0;
		}
	    }
	}
}




static int getBootString(char *name)
{
	char line[BOOT_STRING_LEN];
	char *cp, *val;
	register char *namep;
	int count, ret;
	static int timeout = BOOT_TIMEOUT;

top:	cp = line;
	line[0] = '\0';

	/* If there have been problems, don't go on. */
	if (errors)
	    timeout = 0;
	errors = 0;
	    
	/* Wait a few seconds for user to input string. */
	printf("\n");
	ret = Gets(line, sizeof(line), timeout, "boot: ", "");
	flushdev();

	/* If something was typed, don't use automatic boot again.
	 */
	if (ret)
	    timeout = 0;
	
	skipblank(&cp);

	if (*cp == '?')
	{
		usage();
		goto top;
	}

	if (!isKernel(cp))
	{
		printf("\n");

		val = 0;
		getValueForBootKey(cp, "config", &val, &count);
#if 0
		printf("calling load system config\n");
		//sleep(2);
#endif 1
		useDefaultConfig = loadSystemConfig(val,count);

#if 0
		printf("Got default config %d\n", useDefaultConfig);
		//sleep(2);
#endif 1
		
		if (!sysConfigValid)
			goto top;

		if (getValueForKey( "Kernel", 
			&val, &count))
		{
			strncpy(name,val,count);
#if 0
		printf("Got kernel name");
		//sleep(2);
#endif 1
		}

		// if nothing was typed,
		// and there were no errors,
		// and (Bootgraphics == Yes),
		// then do graphics mode
		if (*line == '\0' && (errors == 0) &&
		    getBoolForKey("Boot Graphics"))
		{
#if 1
		    wantBootGraphics = YES;
		    /*
		     * Ensure that we will be able to enter
		     * graphics mode later.
		     */
		    initMode(GRAPHICS_MODE);
#else
			printf("Graphic boot disabled\n");
		sleep(5);
#endif
		}
	}
	else
	{
#if 0
		printf("No kernel cp\n");
		sleep(5);
#endif 1
		// file name
		namep = name;
		while (*cp && !(*cp == ' ' || *cp == '\t'))
			*namep++ = *cp++;
		*namep = '\0';
	}
	verbose_mode = getValueForBootKey(cp, "-v", &val, &count);
#if 0
	verbose_mode = TRUE;	
	printf("Verbose mode set %d\n", verbose_mode);
		//sleep(5);
#endif 1

	strcpy(kernBootStruct->bootString, cp);
#if 0
	printf("Completed get boot string\n");
		//sleep(5);
#endif 1
	return 0;
}



static void pickLanguage(void)
{
    char *LanguageTable, *val;
    int count, size, answer;
    
    /* Load language choice table */
    loadConfigFile("/usr/standalone/i386/Language.table",
	&LanguageTable, 1);

    /* Get language choices. */
    if (LanguageTable &&
	getValueForStringTableKey(
	    LanguageTable, "Languages", &val, &count))
    {
	char *string, *languages[16], *language_strings[16];
	int nlang = 0;

	while ((string = (char *)newStringFromList(&val, &count))) {
	    languages[nlang] = string;
	    language_strings[nlang++] = (char *)
		newStringForStringTableKey(
		    LanguageTable, string);
	}

	for(answer = -1; answer == -1;) {
	    clearScreen();
//		        message("Rhapsody Installation",1);
//		        printf("\n");
	    answer = chooseSimple(language_strings, nlang,
		1, nlang);
	}
	setLanguage(languages[answer - 1]);
	while (nlang)
	    free(language_strings[nlang--]);
	/* Don't bother freeing the language names. */
	
	free(LanguageTable);
    } else {
	verbose("Could not load language choice file; "
	    "defaulting to English.\n");
	setLanguage("English");
    }

    /* Now save the language in the "Language" key,
	* which has cleverly been made large for us.
	*/
    if (getValueForKey("Language", &val, &size)) {
	char *Language = getLanguage();
	if ((strlen(Language)+2) < size) {
	    strcpy(val, Language);
	    val += strlen(Language);
	    *val++ = '\"';
	    *val++ = ';';
	    *val++ = '\n';
	    size -= strlen(Language) + 1;
	    while (size--)
		*val++ = ' ';
	}
    }
    
    /* Now limit the families we look for when considering
     * drivers during installation.
     */
    LoadableFamilies = newStringForKey("Installation Driver Families");
}




