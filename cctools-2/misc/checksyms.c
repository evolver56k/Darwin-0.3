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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <libc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>
#include "stuff/bool.h"
#include "stuff/ofile.h"
#include "stuff/errors.h"
#include "stuff/allocate.h"

/* used by error routines as the name of the program */
char *progname = NULL;

static int exit_status = EXIT_SUCCESS;

/* flags set from the command line arguments */
struct cmd_flags {
    unsigned long nfiles;
    enum bool rldtype;
    enum bool detail;
    enum bool trey;
    enum bool check_dynamic_binary;
};

/*
 * The table of known dynamic library names and addresses they are linked at.
 * This is loaded from the -dylib_table option or from the file
 * ~rc/Data/DylibTable by default.
 */
struct dylib_table {
    unsigned long seg1addr;
    char *name;
};
static struct dylib_table *dylib_table = NULL;
static char *new_dylib_table_name = NULL;

static void usage(
    void);

static void checksyms(
    struct ofile *ofile,
    char *arch_name,
    void *cookie);

static void check_dynamic_binary(
    struct ofile *ofile,
    char *arch_name,
    enum bool detail);

static void check_dylib(
    struct ofile *ofile,
    char *arch_name,
    enum bool detail);

static void parse_dylib_table(
    char *file_name,
    char *flag,
    char *argument);

int
main(
int argc,
char **argv,
char **envp)
{
    struct cmd_flags cmd_flags;
    unsigned long i, j;
    struct arch_flag *arch_flags;
    unsigned long narch_flags;
    enum bool all_archs;
    char **files;

	progname = argv[0];

	arch_flags = NULL;
	narch_flags = 0;
	all_archs = FALSE;

	cmd_flags.nfiles = 0;
	cmd_flags.rldtype = FALSE;
	cmd_flags.detail = FALSE;
	cmd_flags.trey = FALSE;
	cmd_flags.check_dynamic_binary = TRUE;

        files = allocate(sizeof(char *) * argc);
	for(i = 1; i < argc; i++){
	    if(argv[i][0] == '-'){
		if(argv[i][1] == '\0'){
		    for( ; i < argc; i++)
			files[cmd_flags.nfiles++] = argv[i];
		    break;
		}
		else if(strcmp(argv[i], "-arch") == 0){
		    if(i + 1 == argc){
			error("missing argument(s) to %s option", argv[i]);
			usage();
		    }
		    if(strcmp("all", argv[i+1]) == 0){
			all_archs = TRUE;
		    }
		    else{
			arch_flags = reallocate(arch_flags,
				(narch_flags + 1) * sizeof(struct arch_flag));
			if(get_arch_from_flag(argv[i+1],
					      arch_flags + narch_flags) == 0){
			    error("unknown architecture specification flag: "
				  "%s %s", argv[i], argv[i+1]);
			    arch_usage();
			    usage();
			}
			narch_flags++;
		    }
		    i++;
		}
		else if(strcmp(argv[i], "-dylib_table") == 0){
		    if(i + 1 == argc){
			error("missing argument(s) to %s option", argv[i]);
			usage();
		    }
		    if(new_dylib_table_name != NULL){
			error("more than one: %s option", argv[i]);
			usage();
		    }
		    parse_dylib_table(argv[i+1], argv[i], argv[i+1]);
		    i++;
		}
		else{
		    for(j = 1; argv[i][j] != '\0'; j++){
			switch(argv[i][j]){
			case 'r':
			    cmd_flags.rldtype = TRUE;
			    break;
			case 'd':
			    cmd_flags.detail = TRUE;
			    break;
			case 't':
			    cmd_flags.trey = TRUE;
			    break;
			case 'b':
			    cmd_flags.check_dynamic_binary = TRUE;
			    break;
			default:
			    error("invalid argument -%c", argv[i][j]);
			    usage();
			}
		    }
		}
		continue;
	    }
	    files[cmd_flags.nfiles++] = argv[i];
	}

	if(arch_flags == NULL)
	    all_archs = TRUE;

	if(cmd_flags.nfiles != 1)
	    usage();

	for(i = 0; i < cmd_flags.nfiles; i++)
	    ofile_process(files[i], arch_flags, narch_flags, all_archs, TRUE,
			  TRUE, checksyms, &cmd_flags);

	if(errors == 0)
	    return(exit_status);
	else
	    return(EXIT_FAILURE);
}

/*
 * usage() prints the current usage message and exits indicating failure.
 */
static
void
usage(
void)
{
	fprintf(stderr, "Usage: %s [-r] [-d] [-t] [-b] [-] [-dylib_table file] "
		"[[-arch <arch_flag>] ...] file\n", progname);
	exit(EXIT_FAILURE);
}

/*
 * checksyms() is the routine that gets called by ofile_process() to process
 * single object files.
 */
static
void
checksyms(
struct ofile *ofile,
char *arch_name,
void *cookie)
{
    struct cmd_flags *cmd_flags;
    unsigned long i;
    struct load_command *lc;
    struct symtab_command *st;
    struct nlist *symbols;
    unsigned long nsymbols;
    char *strings;
    unsigned long strsize;
    unsigned long nfiledefs, ncats, nlocal, nstabs, nfun;
    unsigned long filedef_strings, cat_strings, local_strings, stab_strings;

	if(ofile->mh == NULL)
	    return;

	cmd_flags = (struct cmd_flags *)cookie;

	if(cmd_flags->check_dynamic_binary == TRUE)
	    if((ofile->mh->flags & MH_DYLDLINK) == MH_DYLDLINK)
		check_dynamic_binary(ofile, arch_name, cmd_flags->detail);

	if(ofile->mh->filetype == MH_DYLIB)
	    check_dylib(ofile, arch_name, cmd_flags->detail);

	st = NULL;
	lc = ofile->load_commands;
	for(i = 0; i < ofile->mh->ncmds; i++){
	    if(st == NULL && lc->cmd == LC_SYMTAB){
		st = (struct symtab_command *)lc;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	if(st == NULL || st->nsyms == 0){
	    return;
	}

	if(cmd_flags->rldtype == FALSE &&
	   cmd_flags->trey == FALSE &&
	   (ofile->mh->flags & MH_DYLDLINK) == 0 &&
	    (ofile->file_type != OFILE_FAT ||
	     ofile->arch_type != OFILE_ARCHIVE) &&
	    ofile->file_type != OFILE_ARCHIVE &&
	    ofile->mh->filetype != MH_FVMLIB){
	    if(st->nsyms == 0)
		return;

	    if(cmd_flags->detail == TRUE){
		if(arch_name != NULL)
		    printf("(for architecture %s):", arch_name);
		if(ofile->member_ar_hdr != NULL){
		    printf("%s:%.*s:", ofile->file_name,
			   (int)ofile->member_name_size, ofile->member_name);
		}
		else
		    printf("%s:", ofile->file_name);
		printf(" has %lu symbols and %lu string bytes\n", st->nsyms,
		       st->strsize);
	    }
	    exit_status = EXIT_FAILURE;
	    return;
	}

	symbols = (struct nlist *)(ofile->object_addr + st->symoff);
	nsymbols = st->nsyms;
	if(ofile->object_byte_sex != get_host_byte_sex())
	    swap_nlist(symbols, nsymbols, get_host_byte_sex());

	strings = ofile->object_addr + st->stroff;
	strsize = st->strsize;
	for(i = 0; i < nsymbols; i++){
	    if(symbols[i].n_un.n_strx == 0)
		symbols[i].n_un.n_name = "";
	    else if(symbols[i].n_un.n_strx < 0 ||
		    symbols[i].n_un.n_strx > st->strsize)
		symbols[i].n_un.n_name = "bad string index";
	    else
		symbols[i].n_un.n_name = symbols[i].n_un.n_strx + strings;

	    if((symbols[i].n_type & N_TYPE) == N_INDR){
		if(symbols[i].n_value == 0)
		    symbols[i].n_value = (long)"";
		else if(symbols[i].n_value > st->strsize)
		    symbols[i].n_value = (long)"bad string index";
		else
		    symbols[i].n_value =
				(long)(symbols[i].n_value + strings);
	    }
	}

	nfiledefs = 0;
	ncats = 0;
	nlocal = 0;
	nstabs = 0;
	nfun = 0;
	filedef_strings = 0;
	cat_strings = 0;
	local_strings = 0;
	stab_strings = 0;
	for(i = 0; i < nsymbols; i++){
	    if(ofile->mh->filetype == MH_EXECUTE){
		if(symbols[i].n_type == (N_ABS | N_EXT) &&
		   symbols[i].n_value == 0){
		    if(strncmp(symbols[i].n_un.n_name, ".file_definition_",
			       sizeof(".file_definition_") - 1) == 0){
			nfiledefs++;
			filedef_strings += strlen(symbols[i].n_un.n_name);
		    }
		    if(strncmp(symbols[i].n_un.n_name, ".objc_category_name_",
			       sizeof(".objc_category_name_") - 1) == 0){
			ncats++;
			cat_strings += strlen(symbols[i].n_un.n_name);
		    }
		}
	    }
	    if((symbols[i].n_type & N_EXT) == 0){
		nlocal++;
		local_strings += strlen(symbols[i].n_un.n_name);
	    }
	    if(symbols[i].n_type & N_STAB){
		nstabs++;
		stab_strings += strlen(symbols[i].n_un.n_name);
		if(symbols[i].n_type == N_FUN)
		    nfun++;
	    }
	}

	if(nfiledefs == 0 && ncats == 0 && nlocal == 0 && nstabs == 0)
	    return;
	if(cmd_flags->rldtype == TRUE && nstabs == 0)
	    return;
	if((ofile->mh->flags & MH_DYLDLINK) == MH_DYLDLINK &&
	   (nstabs == 0 && nlocal == 0))
	    return;
	if(nstabs == 0 &&
	   ((ofile->file_type == OFILE_FAT &&
	     ofile->arch_type == OFILE_ARCHIVE) ||
	    ofile->file_type == OFILE_ARCHIVE ||
	    ofile->mh->filetype == MH_FVMLIB))
	    return;
	if((ofile->mh->filetype == MH_DYLIB ||
	    ofile->mh->filetype == MH_FVMLIB) &&
	    nfun == 0)
	    return;

	if(cmd_flags->detail == TRUE){
	    if(arch_name != NULL)
		printf("(for architecture %s):", arch_name);
	    if(ofile->member_ar_hdr != NULL){
		printf("%s:%.*s:", ofile->file_name,
		       (int)ofile->member_name_size, ofile->member_name);
	    }
	    else
		printf("%s:", ofile->file_name);
	    printf("\n");
	    if(nfiledefs != 0)
		printf(" has %lu .file_definition_ symbols and %lu string "
		       "bytes\n", nfiledefs, filedef_strings);
	    if(ncats != 0)
		printf(" has %lu .objc_category_name_ symbols and %lu string "
		       "bytes\n", ncats, cat_strings);
	    if(nlocal != 0)
		printf(" has %lu local symbols and %lu string "
		       "bytes\n", nlocal, local_strings);
	    if(nstabs != 0)
		printf(" has %lu debugging symbols and %lu string "
		       "bytes\n", nstabs, stab_strings);
	}
	if(cmd_flags->trey == TRUE && nstabs == 0)
	    return;

	exit_status = EXIT_FAILURE;
	return;
}

/*
 * check_dynamic_binary checks to see a dynamic is built correctly.  That is it
 * has no read-only-relocs, is prebound and has objcunique run on it.
 */
static
void
check_dynamic_binary(
struct ofile *ofile,
char *arch_name,
enum bool detail)
{
    unsigned long i, j, section_attributes;
    struct load_command *lc;
    struct segment_command *sg;
    struct section *s;
    enum bool objc, objcunique;

	/*
	 * First check for relocation entries in read only segments.
	 */
	objc = FALSE;
	objcunique = FALSE;
	lc = ofile->load_commands;
	for(i = 0; i < ofile->mh->ncmds; i++){
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		s = (struct section *)((char *)lc +
					sizeof(struct segment_command));
		if(strcmp(sg->segname, SEG_OBJC) == 0)
		    objc = TRUE;
		for(j = 0; j < sg->nsects; j++){
		    section_attributes = s->flags & SECTION_ATTRIBUTES;
		    if((sg->initprot & VM_PROT_WRITE) == 0 &&
		       ((section_attributes & S_ATTR_EXT_RELOC) != 0 ||
		        (section_attributes & S_ATTR_LOC_RELOC) != 0)){
			if(detail == TRUE){
			    if(arch_name != NULL)
				printf("(for architecture %s):", arch_name);
			    printf("%s: relocation entries in read-only section"
				   " (%.16s,%.16s)\n", ofile->file_name,
				   s->segname, s->sectname);
			}
			exit_status = EXIT_FAILURE;
		    }
		    /*
		     * See if the (__OBJC,__sel_backref) created by objcunique
		     * is present.
		     */
		    if(strcmp(s->segname, SEG_OBJC) == 0 &&
		       strcmp(s->sectname, "__sel_backref") == 0)
			objcunique = TRUE;
		    s++;
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}

	/*
	 * If the file is an executable or a dynamic library and has no
	 * undefined references it should be prebound.
	 */
	if((ofile->mh->filetype == MH_EXECUTE ||
	    ofile->mh->filetype == MH_DYLIB) &&
	   (ofile->mh->flags & MH_NOUNDEFS) == MH_NOUNDEFS){

	    if((ofile->mh->flags & MH_PREBOUND) != MH_PREBOUND){
		if(detail == TRUE){
		    if(arch_name != NULL)
			printf("(for architecture %s):", arch_name);
		    printf("%s: is not prebound\n", ofile->file_name);
		}
		exit_status = EXIT_FAILURE;
	    }
	    else{
		if(objc == TRUE && objcunique == FALSE){
		    if(detail == TRUE){
			if(arch_name != NULL)
			    printf("(for architecture %s):", arch_name);
			printf("%s: is not objcunique\n", ofile->file_name);
		    }
		    exit_status = EXIT_FAILURE;
		}
	    }
	}
}

/*
 * check_dylib() checks the dynamic library against the Apple conventions.
 * This includes the install name, the linked address and the setting of
 * compatibility and current versions.
 */
static
void
check_dylib(
struct ofile *ofile,
char *arch_name,
enum bool detail)
{
    unsigned long i, seg1addr;
    struct load_command *lc;
    struct segment_command *sg;
    struct dylib_command *dlid;
    char *install_name, *base_name, *dot, *name, *p, *suffix;
    enum bool framework;
    FILE *fp;
    char file_name[MAXPATHLEN+1];
    struct stat stat_buf;

	if(stat(ofile->file_name, &stat_buf) == -1)
	    system_fatal("Can't stat dynamic library: %s\n", ofile->file_name);
	if((stat_buf.st_mode & 0111) == 0){
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("no execute bits for dynamic library are set\n");
	    }
	    exit_status = EXIT_FAILURE;
	}

	/*
	 * First pick up the linked address and the dylib id command.
	 */
	seg1addr = ULONG_MAX;
	dlid = NULL;
	install_name = NULL;
	lc = ofile->load_commands;
	for(i = 0; i < ofile->mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(sg->vmaddr < seg1addr)
		    seg1addr = sg->vmaddr;
		break;
	    case LC_ID_DYLIB:
		if(dlid == NULL){
		    dlid = (struct dylib_command *)lc;
		    install_name = (char *)dlid + dlid->dylib.name.offset;
		}
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	if(dlid == NULL){
	    printf("%s: ", ofile->file_name);
	    if(arch_name != NULL)
		printf("(for architecture %s): ", arch_name);
	    printf("malformed dynamic library (no LC_ID_DYLIB command)\n");
	    exit_status = EXIT_FAILURE;
	    return;
	}

	/*
	 * Check for compatibility and current version being set (non-zero).
	 */
	if(dlid->dylib.compatibility_version == 0){
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("compatibility_version for dynamic library not set\n");
	    }
	    exit_status = EXIT_FAILURE;
	}
	if(dlid->dylib.current_version == 0){
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("current_version for dynamic library not set\n");
	    }
	    exit_status = EXIT_FAILURE;
	}

	/*
	 * Check the install names for these forms:
	 * #ifdef __OPENSTEP__
	 * /NextLibrary/Frameworks/AppKit.framework/Versions/A/Appkit
	 * /NextLibrary/PrivateFrameworks/DevKit.framework/Versions/A/DevKit
	 * /usr/lib/NextPrinter/libFaxD.A.dylib
	 * #else
	 * /System/Library/Frameworks/JavaVM.framework/Libraries/libjava.A.dylib
	 * /System/Library/Frameworks/AppKit.framework/Versions/A/Appkit
	 * /System/Library/PrivateFrameworks/DevKit.framework/Versions/A/DevKit
	 * /System/Library/Printers/libFaxD.A.dylib
	 * /Local/Library/Frameworks/Kerberos.framework/Versions/A/Kerberos
	 * #endif
	 * /lib/libsys_s.A.dylib
	 * /usr/lib/libsys_s.A.dylib
	 * /usr/lib/java/libsys_s.A.dylib
	 * /usr/local/lib/libsys_s.A.dylib
	 * /usr/canna/dylib/libRK.A.dylib
	 * and break out the name (Appkit or libsys_s for example).
	 */
	base_name = NULL;
	framework = FALSE;
#ifdef __OPENSTEP__
	if(strncmp(install_name, "/NextLibrary/Frameworks/",
		   sizeof("/NextLibrary/Frameworks/") - 1) == 0){
	    base_name = install_name + sizeof("/NextLibrary/Frameworks/") - 1;
	    framework = TRUE;
	}
	else if(strncmp(install_name, "/NextLibrary/PrivateFrameworks/",
		sizeof("/NextLibrary/PrivateFrameworks/") - 1) == 0){
	    base_name = install_name +
			sizeof("/NextLibrary/PrivateFrameworks/") - 1;
	    framework = TRUE;
	}
#else
	if(strncmp(install_name, "/System/Library/Frameworks"
		   		 "/JavaVM.framework/Libraries/",
		   sizeof("/System/Library/Frameworks"
			  "/JavaVM.framework/Libraries/") - 1) == 0){
	    base_name = install_name + sizeof("/System/Library/Frameworks"
					      "/JavaVM.framework/Libraries/")-1;
	}
	else if(strncmp(install_name, "/System/Library/Frameworks/",
		   sizeof("/System/Library/Frameworks/") - 1) == 0){
	    base_name = install_name + sizeof("/System/Library/Frameworks/") -1;
	    framework = TRUE;
	}
	else if(strncmp(install_name, "/System/Library/PrivateFrameworks/",
		sizeof("/System/Library/PrivateFrameworks/") - 1) == 0){
	    base_name = install_name +
			sizeof("/System/Library/PrivateFrameworks/") - 1;
	    framework = TRUE;
	}
	else if(strncmp(install_name, "/Local/Library/Frameworks/",
		   sizeof("/Local/Library/Frameworks/") - 1) == 0){
	    base_name = install_name + sizeof("/Local/Library/Frameworks/") -1;
	    framework = TRUE;
	}
#endif
	else if(strncmp(install_name, "/lib/",
		sizeof("/lib/") - 1) == 0){
	    base_name = install_name + sizeof("/lib/") - 1;
	}
#ifdef __OPENSTEP__
	else if(strncmp(install_name, "/usr/lib/NextPrinter/",
		sizeof("/usr/lib/NextPrinter/") - 1) == 0){
	    base_name = install_name + sizeof("/usr/lib/NextPrinter/") - 1;
	}
#else
	else if(strncmp(install_name, "/System/Library/Printers/",
		sizeof("/System/Library/Printers/") - 1) == 0){
	    base_name = install_name + sizeof("/System/Library/Printers/") - 1;
	}
#endif
	else if(strncmp(install_name, "/usr/lib/java/",
		sizeof("/usr/lib/java/") - 1) == 0){
	    base_name = install_name + sizeof("/usr/lib/java/") - 1;
	}
	else if(strncmp(install_name, "/usr/lib/",
		sizeof("/usr/lib/") - 1) == 0){
	    base_name = install_name + sizeof("/usr/lib/") - 1;
	}
	else if(strncmp(install_name, "/usr/local/lib/",
		sizeof("/usr/local/lib/") - 1) == 0){
	    base_name = install_name + sizeof("/usr/local/lib/") - 1;
	}
	else if(strncmp(install_name, "/usr/canna/dylib/",
		sizeof("/usr/canna/dylib/") - 1) == 0){
	    base_name = install_name + sizeof("/usr/canna/dylib/") - 1;
	}
	else{
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("dynamic library has non-conventional directory in its "
		       "install name (%s)\n", install_name);
	    }
	    exit_status = EXIT_FAILURE;
	    return;
	}
	if(framework == TRUE){
	    dot = strchr(base_name, '.');
	    if(dot == NULL)
		goto bad_name;
	    name = base_name;
	    *dot = '\0';
	    p = dot + 1;
	    if(strncmp(p, "framework/", sizeof("framework/") - 1) != 0){
		*dot = '.';
		goto bad_name;
	    }
	    p += sizeof("framework/") - 1;
	    if(strncmp(p, "Versions/", sizeof("Versions/") - 1) != 0){
		*dot = '.';
		if(detail == TRUE){
		    printf("%s: ", ofile->file_name);
		    if(arch_name != NULL)
			printf("(for architecture %s): ", arch_name);
		    printf("framework does not have Versions directory in "
			   "its install name (%s)\n", install_name);
		}
		exit_status = EXIT_FAILURE;
		*dot = '\0';
	    }
	    else{
		p += sizeof("Versions/") - 1;
		p = strchr(p, '/');
		if(p == NULL){
		    *dot = '.';
		    goto bad_name;
		}
		p++;
	    }
	    /*
	     * The name may have a suffix.  By convention all suffixes start
	     * with an underbar and the name and the suffix may not contain
	     * any other underbars.
	     */
	    suffix = strchr(p, '_');
	    if(suffix != NULL)
		*suffix = '\0';
	    if(strcmp(p, name) != 0){
		*dot = '.';
		goto bad_name;
	    }
	}
	else{
	    name = base_name;
	    dot = strchr(base_name, '.');
	    if(dot == NULL){
		if(detail == TRUE){
		    printf("%s: ", ofile->file_name);
		    if(arch_name != NULL)
			printf("(for architecture %s): ", arch_name);
		    printf("dynamic library does not have major version letter "
			   "in its install name (%s)\n", install_name);
		}
		exit_status = EXIT_FAILURE;
		return;
	    }
	    else{
		if(dot[1] == '\0'){
		    if(detail == TRUE){
			printf("%s: ", ofile->file_name);
			if(arch_name != NULL)
			    printf("(for architecture %s): ", arch_name);
			printf("dynamic library does not have major version "
			       "letter in its install name (%s)\n",
			       install_name);
		    }
		    exit_status = EXIT_FAILURE;
		    return;
		}
		if(strcmp(dot, ".dylib") == 0){
		    if(detail == TRUE){
			printf("%s: ", ofile->file_name);
			if(arch_name != NULL)
			    printf("(for architecture %s): ", arch_name);
			printf("dynamic library does not have major version "
			       "letter in its install name (%s)\n",
			       install_name);
		    }
		    exit_status = EXIT_FAILURE;
		    p = dot;
		}
		else
		    p = dot + 2;
	    }
	    if(strcmp(p, ".dylib") != 0){
		if(detail == TRUE){
		    printf("%s: ", ofile->file_name);
		    if(arch_name != NULL)
			printf("(for architecture %s): ", arch_name);
		    printf("dynamic library's install name (%s)  does not end "
			   "in .dylib\n", install_name);
		}
		exit_status = EXIT_FAILURE;
	    }
	    *dot = '\0';
	    /*
	     * The name may have a suffix.  By convention all suffixes start
	     * with an underbar and the name and the suffix may not contain any
	     * other underbars.
	     */
	    p = strchr(name, '_');
	    if(p != NULL)
		*p = '\0';
	}

	/*
	 * If there is no dylib table open ~rc/Data/DylibTable and use it.
	 */
	if(dylib_table == NULL){
	    fp = popen("/bin/echo ~rc/Data/DylibTable", "r");
	    if(fp == NULL)
		fatal("must use -dylib_table (popen failed on \"/bin/echo "
		      "~rc/Data/DylibTable\"");
	    if(fgets(file_name, MAXPATHLEN, fp) == NULL)
		fatal("must use -dylib_table (fgets failed from popen of "
		      "\"/bin/echo ~rc/Data/DylibTable\"");
	    i = strlen(file_name);
	    if(i == 0 || file_name[i-1] != '\n')
		fatal("must use -dylib_table (file name from popen of "
		      "\"/bin/echo ~rc/Data/DylibTable\" greater than "
		      "MAXPATHLEN");
	    file_name[i-1] = '\0';
	    pclose(fp);
	    parse_dylib_table(file_name, "default", "dylib table");
/*
	    parse_dylib_table("/Net/seaport/release/Data/DylibTable",
			      "default", "dylib table");
*/
	}
	/*
	 * Now name points to the name of the library.  This name must be found
	 * in the table and the seg1addr must match with what is in the table.
	 */
	for(i = 0; dylib_table[i].name != NULL; i++){
	    if(strcmp(dylib_table[i].name, name) == 0)
		break;
	}
	if(dylib_table[i].name == NULL){
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("dynamic library name (%s) unknown to %s and "
		    "checking of its seg1addr can't be done ",name,
		    progname);
		printf("(dylib table: %s must be updated to include this "
		       "library, contact release control to assign an address "
		       "to this library)\n", new_dylib_table_name);
	    }
	    exit_status = EXIT_FAILURE;
	}
	else if(dylib_table[i].seg1addr != seg1addr){
	    if(detail == TRUE){
		printf("%s: ", ofile->file_name);
		if(arch_name != NULL)
		    printf("(for architecture %s): ", arch_name);
		printf("dynamic library (%s) not linked at its expected "
		       "seg1addr (0x%x)\n", name,
		       (unsigned int)dylib_table[i].seg1addr);
	    }
	    exit_status = EXIT_FAILURE;
	}

	return;

bad_name:
	if(detail == TRUE){
	    printf("%s: ", ofile->file_name);
	    if(arch_name != NULL)
		printf("(for architecture %s): ", arch_name);
	    printf("framework has non-conventional install name (%s)\n",
		   install_name);
	}
	exit_status = EXIT_FAILURE;
	return;
}

static
void
parse_dylib_table(
char *file_name,/* file name of dylib table file */
char *flag,	/* "-dylib_file" or "default" */
char *argument) /* -dylib_file argument or "dylib table" */
{
    int fd;
    kern_return_t r;
    struct stat stat_buf;
    unsigned long j, k, file_size, new_dylib_table_size;
    char *file_addr, *endp;
    struct dylib_table *new_dylib_table;

	new_dylib_table_name = file_name;
	if((fd = open(file_name, O_RDONLY, 0)) == -1)
	    system_fatal("Can't open: %s for %s %s",
		    file_name, flag, argument);
	if(fstat(fd, &stat_buf) == -1)
	    system_fatal("Can't stat file: %s for %s %s",
		    file_name, flag, argument);
	/*
	 * For some reason mapping files with zero size fails
	 * so it has to be handled specially.
	 */
	if(stat_buf.st_size != 0){
	    if((r = map_fd((int)fd, (vm_offset_t)0,
		(vm_offset_t *)&file_addr, (boolean_t)TRUE,
		(vm_size_t)stat_buf.st_size)) != KERN_SUCCESS)
		mach_fatal(r, "can't map file: %s for %s %s",
		    file_name, flag, argument);
	}
	else
	    fatal("Empty file: %s for %s %s", file_name, flag, argument);
	close(fd);
	file_size = stat_buf.st_size;

	/*
	 * Got the file mapped now parse it.
	 */
	if(file_addr[file_size - 1] != '\n')
	    fatal("file: %s for %s %s does not end in new line",
		  file_name, flag, argument);
	new_dylib_table_size = 0;
	for(j = 1; j < file_size; j++){
	    if(file_addr[j] == '\n'){
		new_dylib_table_size++;
	    }
	}
	new_dylib_table_size++;
	new_dylib_table = allocate(sizeof(struct dylib_table) *
				   new_dylib_table_size);
	k = 0;
	for(j = 0; j < file_size; /* no increment expression */ ){
	    /* Skip blank lines */
	    while(file_addr[j] == ' ' || file_addr[j] == '\t')
		j++;
	    if(file_addr[j] == '\n'){
		j++;
		continue;
	    }
	    new_dylib_table[k].seg1addr =
		strtoul(file_addr + j, &endp, 16);
	    if(endp == NULL)
		fatal("improper hexadecimal number on line %lu in "
		      "file: %s for %s %s", j, file_name, flag, argument);
	    j = endp - file_addr;
	    if(j == file_size)
		fatal("missing library name on line %lu in file: "
		      "%s for %s %s", j, file_name, flag, argument);
	    /*
	     * Since we check to see the file ends in a '\n' we can
	     * be assured this won't run off the end of the file.
	     */
	    while(file_addr[j] == ' ' || file_addr[j] == '\t')
		j++;
	    if(file_addr[j] == '\n')
		fatal("missing library name on line %lu in file: "
		      "%s for %s %s", j, file_name, flag, argument);
	    new_dylib_table[k].name = file_addr + j;
	    k++;
	    while(file_addr[j] != '\n')
		j++;
	    file_addr[j] = '\0';
	    j++;
	}
	new_dylib_table[k].seg1addr = 0;
	new_dylib_table[k].name = NULL;
	dylib_table = new_dylib_table;
}
