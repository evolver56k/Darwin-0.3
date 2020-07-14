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
#import <libc.h>
#ifndef __OPENSTEP__
extern int add_profil(char *, int, int, int);
#endif
#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <errno.h>
#import <limits.h>
#import <sys/file.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <mach/mach.h>
#import "stuff/openstep_mach.h"
#import <mach-o/fat.h>
#import <mach-o/loader.h>
#import <mach-o/dyld_debug.h>
#ifdef hppa
#import <mach-o/hppa/reloc.h>
#endif
#ifdef sparc
#import <mach-o/sparc/reloc.h>
#endif
#ifdef __ppc__
#import <mach-o/ppc/reloc.h>
#endif
#import <sys/mman.h>
#ifdef __OPENSTEP__
/* This should be in mman.h */
extern int mmap(char *, int, int, int, int, int);
#import <mach-o/gmon.h>
#else
#import <sys/gmon.h>
#endif
/* This is in gmon.c but should be in gmon.h */
#define SCALE_1_TO_1 0x10000L

#import "stuff/bool.h"
#import "stuff/best_arch.h"
#import "stuff/bytesex.h"
#import "stuff/round.h"

#ifndef __MACH30__
#import "../profileServer/profileServer.h"
#endif /* __MACH30__ */

#import "images.h"
#import "reloc.h"
#import "symbols.h"
#import "errors.h"
#import "allocate.h"
#import "dyld_init.h"
#import "entry_point.h"
#import "debug.h"
#import "register_funcs.h"
#import "lock.h"

struct object_images object_images;
struct library_images library_images;

/* The name of the executable, argv[0], for error messages */
char *executables_name = NULL;

/*
 * loading_executables_libraries is set when loading libraries for the
 * executable.
 */
static enum bool loading_executables_libraries = TRUE;

/*
 * The names of object file images are stored in a string block (or just
 * malloc()'ed they don't fit).  We need to have a copy of the name for error
 * messages and can't rely on using the name the user gave us when the object
 * file image first came to be.  This is done to avoid malloc()'ing space in
 * the most likely cases (for the executable's name, argv[0] and a reasonable
 * number of bundles).  This is done by the routine save_string().
 */
enum string_block_size { STRING_BLOCK_SIZE = 1024 };
struct string_block {
    unsigned long used;		/* the number of bytes used */
    char strings[STRING_BLOCK_SIZE]; /* the strings */
};
static struct string_block string_block;

/*
 * The module_state's for libraries are allocated from this pool of
 * module_states.  This is is to avoid malloc()'ing space for a reasonable
 * number libraries in the most likely cases (for an executable that uses
 * libsys and a few frameworks).
 * (info taken from a 4.0 Lantern1Y system)
 * Library	Number of modules
 * libsys	941
 * AppKit	289
 * Foundation	81
 * SoundKit	17
 *			subtotal 1328
 * NEXTIME	6
 * EOAccess	37
 * EOInterface	32
 * NIAccess	8
 * NIInterface	7
 * DebugKit	18
 * DevKit	81
 * Interceptor	12
 * NeXTApps	27
 *			total 1556
 */
enum module_state_block_size { MODULE_STATE_BLOCK_SIZE = 2000 };
struct module_state_block {
    unsigned long used;		/* the number of items used */
    module_state module_states[MODULE_STATE_BLOCK_SIZE]; /* the module_states */
};
static struct module_state_block module_state_block;

static module_state *allocate_module_states(
    unsigned long nmodules);

/*
 * The function pointer passed to _dyld_moninit() to do profiling of dyld loaded
 * code.  If this function pointer is not NULL at the time of a map_image()
 * called it is called indirectly to set up the profiling buffers.
 */
void (*dyld_monaddition)(char *lowpc, char *highpc) = NULL;

static struct object_image *new_object_image(
    void);
static struct library_image *new_library_image(
    unsigned long nmodules);
static char *get_framework_name(
    char *name,
    enum bool with_suffix);
static char *look_back_for_slash(
    char *name,
    char *p);
static char *search_for_name_in_path(
    char *name,
    char *path);
static enum bool map_library_image(
    struct dylib_command *dl,
    char *dylib_name,
    int fd,
    char *file_addr,
    unsigned long file_size,
    unsigned long library_offset,
    unsigned long library_size,
    dev_t dev,
    ino_t ino);
static enum bool is_library_loaded(
    char *dylib_name,
    struct dylib_command *dl);
static enum bool set_prebound_state(
    struct prebound_dylib_command *pbdylib);

static enum bool check_image(
    char *name,
    char *image_type,
    unsigned long image_size,
    struct mach_header *mh,
    struct segment_command **linkedit_segment,
    struct segment_command **mach_header_segment,
    struct dysymtab_command **dyst,
    struct symtab_command **st,
    struct dylib_command **dlid,
    unsigned long *low_addr,
    unsigned long *high_addr);
static enum bool check_linkedit_info(
    char *name,
    char *image_type,
    struct segment_command *linkedit_segment,
    struct symtab_command *st,
    struct dysymtab_command *dyst);

static enum bool map_image(
    char *name,
    char *image_type,
    unsigned long image_size,
    int fd,
    char *file_addr,
    unsigned long file_size,
    unsigned long library_offset,
    unsigned long library_size,
    unsigned long low_addr,
    unsigned long high_addr,
    struct mach_header **mh,
    struct segment_command **linkedit_segment,
    struct dysymtab_command **dyst,
    struct symtab_command **st,
    struct dylib_command **dlid,
    enum bool *change_protect_on_reloc,
    enum bool *cache_sync_on_reloc,
    struct section **init,
    struct section **term,
    unsigned long *seg1addr,
    unsigned long *slide_value,
    unsigned long *images_dyld_stub_binding_helper);
static void set_segment_protections(
    char *name,
    char *image_type,
    struct mach_header *mh,
    unsigned long slide_value);
static void load_images_libraries(
    struct mach_header *mh);
static void undo_prebinding_for_library(
    struct library_image *library_image);
static void failed_use_prebound_libraries(
    void);
static void reset_module_states(
    void);

/*
 * The address of these symbols are written in to the (__DATA,__dyld) section
 * at the following offsets:
 *	at offset 0	stub_binding_helper_interface
 *	at offset 4	_dyld_func_lookup
 *	at offset 8	start_debug_thread
 * The 'C' types (if any) for these symbols are ignored here and all are
 * declared as longs so the assignment of their address in to the section will
 * not require a cast.  stub_binding_helper_interface is really a label in the
 * assembly code interface for the stub binding.  It does not have a meaningful 
 * 'C' type.  _dyld_func_lookup is the routine in dyld_libfuncs.c.
 * start_debug_thread is the routine in debug.c.
 *
 * For ppc the image's stub_binding_binding_helper is read from:
 *	at offset 20	the image's stub_binding_binding_helper address
 * and saved into to the image structure.
 */
extern long stub_binding_helper_interface;
extern long _dyld_func_lookup;

/*
 * load_executable_image() loads up the executable into the dynamic linker's
 * data structures (the kernel has loaded the segments into memory).  Then it
 * calls load_images_libraries() for all dynamic shared libraries the executable
 * uses.
 */
void
load_executable_image(
char *name,
struct mach_header *mh,
unsigned long *entry_point)
{
    unsigned long i, j, seg1addr;
    struct load_command *lc, *load_commands;
    struct segment_command *sg, *linkedit_segment;
    struct section *s, *init, *term;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct thread_command *thread_command;
    struct object_image *object_image;
    enum bool change_protect_on_reloc, cache_sync_on_reloc, seg1addr_found;
    char *dylib_name, *p;
#ifdef __ppc__
    unsigned long images_dyld_stub_binding_helper;

	images_dyld_stub_binding_helper =
	    (unsigned long)(&unlinked_lazy_pointer_handler);
#endif

	/* set for error reporting in here */
	executables_name = name;

	/*
	 * Pick up the linkedit segment and dynamic symbol command from the
	 * executable.  We don't check for the error of having more than one of
	 * these but just pick up the first one if any.  Checks to see if the
	 * load commands for an executable are valid are not done as we assume
	 * they are good since the kernel used them and we are running.
	 */
	linkedit_segment = NULL;
	st = NULL;
	dyst = NULL;
	init = NULL;
	term = NULL;
	seg1addr_found = FALSE;
	seg1addr = 0;
	change_protect_on_reloc = FALSE;
	cache_sync_on_reloc = FALSE;
	load_commands = (struct load_command *)
			((char *)mh + sizeof(struct mach_header));
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(strcmp(sg->segname, SEG_LINKEDIT) == 0){
		    if(linkedit_segment == NULL)
			linkedit_segment = sg;
		}
		/*
		 * Pickup the address of the first segment.  Note this may not
		 * be the lowest address, but it is the address relocation
		 * entries are based off of.
		 */
		if(seg1addr_found == FALSE){
		    seg1addr = sg->vmaddr;
		    seg1addr_found = TRUE;
		}
		/*
		 * Stuff the address of the stub_binding_helper_interface into
		 * the first 4 bytes of the (__DATA,__dyld) section if there is
		 * one.  And stuff the address of _dyld_func_lookup in the
		 * second 4 bytes of the (__DATA,__dyld) section.  And stuff the
		 * address of start_debug_thread in the third 4 bytes of the
		 * (__DATA,__dyld) section.
		 */
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0; j < sg->nsects; j++){
		    if(strcmp(s->segname, "__DATA") == 0 &&
		       strcmp(s->sectname, "__dyld") == 0){
			if(s->size >= sizeof(unsigned long)){
			    *((long *)s->addr) =
				(long)&stub_binding_helper_interface;
			}
			if(s->size >= 2 * sizeof(unsigned long)){
			    *((long *)(s->addr + 4)) =
				(long)&_dyld_func_lookup;
			}
			if(s->size >= 3 * sizeof(unsigned long)){
			    *((long *)(s->addr + 8)) =
				(long)&start_debug_thread;
			}
#ifdef __ppc__
			if(s->size >= 5 * sizeof(unsigned long)){
			    images_dyld_stub_binding_helper = 
				*((long *)(s->addr + 20));
			}
#endif
		    }
		    s++;
		}
		/*
		 * If this segment is not to have write protection then check to
		 * see if any of the sections have external relocations and if
		 * so mark the image as needing to change protections when doing
		 * relocation in it.
		 */
		if((sg->initprot & VM_PROT_WRITE) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & S_ATTR_EXT_RELOC)){
			    if((sg->maxprot & VM_PROT_READ) == 0 ||
			       (sg->maxprot & VM_PROT_WRITE) == 0){
	    			error("malformed executable: %s (segment %.16s "
				    "has relocation entries but the max vm "
				    "protection does not allow reading and "
				    "writing)", name, sg->segname);
	    			link_edit_error(DYLD_FILE_FORMAT, EBADMACHO,
				    name);
			    }
			    change_protect_on_reloc = TRUE;
			    break;
			}
			s++;
		    }
		}
		/*
		 * If the image has relocations for instructions then the i
		 * cache needs to sync with d cache on relocation.  A good guess
		 * is made based on the section attributes and section name.
		 */
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0; j < sg->nsects; j++){
		    if(((strcmp(s->segname, "__TEXT") == 0 &&
		         strcmp(s->sectname, "__text") == 0) ||
			 (s->flags & S_ATTR_SOME_INSTRUCTIONS)) &&
			 (s->flags & S_ATTR_EXT_RELOC)){
			cache_sync_on_reloc = TRUE;
			break;
		    }
		    s++;
		}
		/*
		 * If the image has a module init section pick it up.
		 */
		if(init == NULL){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & SECTION_TYPE) ==
			   S_MOD_INIT_FUNC_POINTERS){
			    init = s;
			    break;
			}
			s++;
		    }
		}
		/*
		 * If the image has a module term section pick it up.
		 */
		if(term == NULL){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & SECTION_TYPE) ==
			   S_MOD_TERM_FUNC_POINTERS){
			    term = s;
			    break;
			}
			s++;
		    }
		}
		break;
	    case LC_SYMTAB:
		if(st == NULL)
		    st = (struct symtab_command *)lc;
		break;
	    case LC_DYSYMTAB:
		if(dyst == NULL)
		    dyst = (struct dysymtab_command *)lc;
		break;
	    case LC_UNIXTHREAD:
		thread_command = (struct thread_command *)lc;
		*entry_point = get_entry_point(thread_command);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	/*
	 * We allow anything that make some sense and issue warnings that most
	 * likely are errors for the executable (in libraries these are hard
	 * errors).
	 */
	if(st == NULL && dyst != NULL){
	    error("malformed executable: %s (dynamic symbol table command "
		"but no standard symbol table command)", name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    dyst = NULL;
	}
	else if(linkedit_segment == NULL){
	    if(dyst != NULL){
		error("malformed executable: %s (dynamic symbol table command"
		    "but no " SEG_LINKEDIT "segment)", name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		error("malformed executable: %s (symbol table command but no "
		    SEG_LINKEDIT "segment)", name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    }
	    else{
		error("possible malformed executable: %s (no dynamic symbol "
		    "table command)", name);
		link_edit_error(DYLD_WARNING, EBADMACHO, name);
		error("possible malformed executable: %s (no " SEG_LINKEDIT
		    " segment)", name);
		link_edit_error(DYLD_WARNING, EBADMACHO, name);
	    }
	    /* without a linkedit segment we can't get to the symbol table */
	    st = NULL;
	    dyst = NULL;
	}
	else{
	    if(dyst == NULL){
		error("possible malformed executable: %s (no dynamic symbol "
		    "table command)", name);
		link_edit_error(DYLD_WARNING, EBADMACHO, name);
	    }
	    else{
		check_linkedit_info(name, "executable", linkedit_segment, st,
				    dyst);
	    }
	}

	/*
	 * Set up the executable as the first object image loaded.
	 */
	object_image = new_object_image();
	object_image->image.name = save_string(name);
	executables_name = object_image->image.name;
	object_image->image.vmaddr_slide = 0;
	/* note the executable is not always contiguious in memory and should
	   not be deallocated using vmaddr_size */
	object_image->image.vmaddr_size = 0;
	object_image->image.seg1addr = seg1addr;
	object_image->image.mh = mh;
	object_image->image.st = st;
	object_image->image.dyst = dyst;
	object_image->image.linkedit_segment = linkedit_segment;
	object_image->image.change_protect_on_reloc = change_protect_on_reloc;
	object_image->image.cache_sync_on_reloc = cache_sync_on_reloc;
	object_image->image.init = init;
	object_image->image.term = term;
#ifdef __ppc__
	object_image->image.dyld_stub_binding_helper =
			    images_dyld_stub_binding_helper;
#endif
	object_image->module = BEING_LINKED;

	/*
	 * If DYLD_INSERT_LIBRARIES is set insert the libraries listed.
	 */
	if(dyld_insert_libraries != NULL){
	    if(dyld_print_libraries == TRUE)
		print("loading libraries for DYLD_INSERT_LIBRARIES=%s\n",
			dyld_insert_libraries);
	    dylib_name = dyld_insert_libraries;
	    for(;;){
		p = strchr(dylib_name, ':');
		if(p != NULL)
		    *p = '\0';
		/*
		 * This feature disables prebinding.  We know that launched is
		 * FALSE at this point.
		 */
		if(dyld_prebind_debug != 0 && prebinding == TRUE)
		    print("dyld: %s: prebinding disabled due to "
			   "DYLD_INSERT_LIBRARIES being set\n",
			   executables_name);
		prebinding = FALSE;
		(void)load_library_image(NULL, dylib_name);
		if(p == NULL){
		    break;
		}
		else{
		    *p = ':';
		    dylib_name = p + 1;
		}
	    }
	}

	/*
	 * Now load the library images this executable uses.
	 */
	if(dyld_print_libraries == TRUE)
	    print("loading libraries for image: %s\n",
		   object_image->image.name);
	loading_executables_libraries = TRUE;
	load_images_libraries(mh);
	loading_executables_libraries = FALSE;

	/*
	 * Load the dependent libraries.
	 */
	load_dependent_libraries();
}

/*
 * load_library_image() causes the specified dynamic shared library to be loaded
 * into memory and added to the dynamic link editor data structures to use it.
 * Specifily this routine takes a pointer to a dylib_command for a library and
 * finds and opens the file corresponding to it.  It deals with the file being
 * fat and then calls map_library_image() to have the library's segments mapped
 * into memory. 
 */
enum bool
load_library_image(
struct dylib_command *dl, /* allow NULL for NSAddLibrary() to use this */
char *dylib_name)
{
    char *name, *new_dylib_name;
    int fd, errnum, save_errno;
    struct stat stat_buf;
    unsigned long file_size;
    char *file_addr;
    kern_return_t r;
    struct fat_header *fat_header;
    struct fat_arch *fat_archs, *best_fat_arch;
    struct mach_header *mh;

	/*
	 * If the dylib_command is not NULL this this is not a result of a call
	 * to NSAddLibrary() so searching may take place.
	 */
	if(dl != NULL){
	    new_dylib_name = NULL;
	    dylib_name = (char *)dl + dl->dylib.name.offset;
	    /*
	     * If the dyld_framework_path is set and this dylib_name is a
	     * framework name, use the first file that exists in the framework
	     * path if any.  If there is none go on to search the
	     * dyld_library_path if any.  The first call to get_framework_name()
	     * trys to get a name without a suffix, the second call trys with a
	     * suffix.
	     */
	    if(dyld_framework_path != NULL){
	        if((name = get_framework_name(dylib_name, FALSE)) != NULL){
		    new_dylib_name = search_for_name_in_path(name,
							 dyld_framework_path);
		}
		if(new_dylib_name != NULL){
		    dylib_name = new_dylib_name;
		}
		else{
		    if((name = get_framework_name(dylib_name, TRUE)) != NULL){
			new_dylib_name = search_for_name_in_path(name,
							 dyld_framework_path);
			if(new_dylib_name != NULL)
			    dylib_name = new_dylib_name;
		    }
		}
	    }
	    /*
	     * If the dyld_library_path is set then use the first file that
	     * exists in the path.  If none use the original name. 
	     * The string dyld_library_path points to is "path1:path2:path3" and
	     * comes from the enviroment variable DYLD_LIBRARY_PATH.
	     */
	    if(new_dylib_name == NULL &&
		dyld_library_path != NULL){
		name = strrchr(dylib_name, '/');
		if(name != NULL && name[1] != '\0')
		    name++;
		else
		    name = dylib_name;
		new_dylib_name = search_for_name_in_path(name,
							 dyld_library_path);
		if(new_dylib_name != NULL)
		    dylib_name = new_dylib_name;
	    }
	    /*
	     * Now try to open the dylib_name.  If it fails and we have not
	     * previously tried to search for a name then try searching the
	     * fall back paths (including the default fall back framework path).
	     */
	    fd = open(dylib_name, O_RDONLY, 0);
	    if(fd == -1 && new_dylib_name == NULL){
		save_errno = errno;
		/*
		 * First try the the dyld_fallback_framework_path if that has
		 * been set (first without a suffix and then with a suffix).
		 */
		if(dyld_fallback_framework_path != NULL){
		    if((name = get_framework_name(dylib_name, FALSE)) != NULL){
			new_dylib_name = search_for_name_in_path(name,
						 dyld_fallback_framework_path);
		    }
		    if(new_dylib_name != NULL){
			dylib_name = new_dylib_name;
		    }
		    else{
			if((name = get_framework_name(dylib_name, TRUE)) !=
			   NULL){
			    new_dylib_name = search_for_name_in_path(name,
						 dyld_fallback_framework_path);
			    if(new_dylib_name != NULL)
				dylib_name = new_dylib_name;
			}
		    }
		}
		/*
		 * If no new name is still found try dyld_fallback_library_path
		 * if that was set.
		 */
		if(new_dylib_name == NULL &&
		    dyld_fallback_library_path != NULL){
		    name = strrchr(dylib_name, '/');
		    if(name != NULL && name[1] != '\0')
			name++;
		    else
			name = dylib_name;
		    new_dylib_name = search_for_name_in_path(name,
						 dyld_fallback_library_path);
		    if(new_dylib_name != NULL)
			dylib_name = new_dylib_name;
		}
		/*
		 * If no new name is still use the default fallback framework
		 * path creating it if has not yet been created.
		 */
		if(new_dylib_name == NULL){
		    if(default_fallback_framework_path == NULL){
			default_fallback_framework_path =
			    allocate(strlen(home) +
				sizeof(DEFAULT_FALLBACK_FRAMEWORK_PATH));
			strcpy(default_fallback_framework_path, home);
			strcat(default_fallback_framework_path,
			       DEFAULT_FALLBACK_FRAMEWORK_PATH);
		    }
		    if((name = get_framework_name(dylib_name, FALSE)) != NULL){
			new_dylib_name = search_for_name_in_path(name,
					     default_fallback_framework_path);
		    }
		    if(new_dylib_name != NULL){
			dylib_name = new_dylib_name;
		    }
		    else{
			if((name = get_framework_name(dylib_name, TRUE)) !=
			   NULL){
			    new_dylib_name = search_for_name_in_path(name,
					     default_fallback_framework_path);
			    if(new_dylib_name != NULL)
				dylib_name = new_dylib_name;
			}
		    }
		    if(new_dylib_name == NULL){
			if(default_fallback_library_path == NULL){
			    default_fallback_library_path =
				allocate(strlen(home) +
				    sizeof(DEFAULT_FALLBACK_LIBRARY_PATH));
			    strcpy(default_fallback_library_path, home);
			    strcat(default_fallback_library_path,
				   DEFAULT_FALLBACK_LIBRARY_PATH);
			}
			name = strrchr(dylib_name, '/');
			if(name != NULL && name[1] != '\0')
			    name++;
			else
			    name = dylib_name;
			new_dylib_name = search_for_name_in_path(name,
						 default_fallback_library_path);
			if(new_dylib_name != NULL)
			    dylib_name = new_dylib_name;
		    }
		}
		/*
		 * If the search through the fall back paths found a new path
		 * then open it.  If no name was ever found put back the errno
		 * from the original open that failed.
		 */
		if(new_dylib_name != NULL)
		    fd = open(dylib_name, O_RDONLY, 0);
		else
		    errno = save_errno;
	    }
	}
	/*
	 * The dylib_command is NULL so this is a result of a call
	 * to NSAddLibrary() so just open the name passed in.
	 */
	else{
	    fd = open(dylib_name, O_RDONLY, 0);
	}

	/*
	 * If the library is already loaded just return.
	 */
	if(is_library_loaded(dylib_name, dl) == TRUE){
	    if(fd != -1)
		close(fd);
	    return(TRUE);
	}

	if(dyld_print_libraries == TRUE)
	    print("loading library: %s\n", dylib_name);

	/*
	 * The file name that will be used for this library has been opened.
	 * If that failed report it and return.  For fixed address shared
	 * libraries the kernel maps the library and if it is not present you
	 * get an EBADEXEC, for access permissions you get EACCES ...  Here we
	 * just return whatever the errno from the open call happens to be.
	 * So this won't match what the kernel might have done (but we also
	 * don't allow execute only permissions as we require read permission).
	 */
	if(fd == -1){
	    errnum = errno;
	    system_error(errnum, "can't open library: %s ", dylib_name);
	    link_edit_error(DYLD_FILE_ACCESS, errnum, dylib_name);
	    return(FALSE);
	}
	/*
	 * Now that the dylib_name has been determined and opened get it into
	 * memory by mapping it.
	 */
	if(fstat(fd, &stat_buf) == -1){
	    errnum = errno;
	    system_error(errnum, "can't stat library: %s", dylib_name);
	    link_edit_error(DYLD_FILE_ACCESS, errnum, dylib_name);
	    goto load_library_image_cleanup2;
	}
	file_size = stat_buf.st_size;
	/*
	 * For some reason mapping files with zero size fails so it has to
	 * be handled specially.
	 */
	if(file_size == 0){
	    error("truncated or malformed library: %s (file is empty)",
		dylib_name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
	    goto load_library_image_cleanup2;
	}
	/*
	 * Since directories can be opened but not mapped check to see this
	 * is a plain file.  Which will give a less confusing.
	 */
	if((stat_buf.st_mode & S_IFMT) != S_IFREG){
	    error("file is not a regular file: %s (can't possibly be a "
		  "library)", dylib_name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
	    goto load_library_image_cleanup2;
	}
	if((r = map_fd((int)fd, (vm_offset_t)0, (vm_offset_t *)&file_addr,
	    (boolean_t)TRUE, (vm_size_t)file_size)) != KERN_SUCCESS){
	    mach_error(r, "can't map library: %s", dylib_name);
	    link_edit_error(DYLD_MACH_RESOURCE, r, dylib_name);
	    goto load_library_image_cleanup2;
	}

	/*
	 * Determine what type of file it is fat or thin and if it is even an
	 * object file.
	 */
	if(sizeof(struct fat_header) > file_size){
	    error("truncated or malformed library: %s (file too small to be "
		  "a library)", dylib_name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
	    goto load_library_image_cleanup1;
	}
	fat_header = (struct fat_header *)file_addr;
#ifdef __BIG_ENDIAN__
	if(fat_header->magic == FAT_MAGIC)
#endif
#ifdef __LITTLE_ENDIAN__
	if(fat_header->magic == SWAP_LONG(FAT_MAGIC))
#endif
	{
#ifdef __LITTLE_ENDIAN__
	    swap_fat_header(fat_header, LITTLE_ENDIAN_BYTE_SEX);
#endif
	    if(sizeof(struct fat_header) + fat_header->nfat_arch *
	       sizeof(struct fat_arch) > file_size){
		error("truncated or malformed library: %s (fat file's fat_arch "
		      "structs extend past the end of the file)", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    fat_archs = (struct fat_arch *)(file_addr +
					    sizeof(struct fat_header));
#ifdef __LITTLE_ENDIAN__
	    swap_fat_arch(fat_archs, fat_header->nfat_arch,
			  LITTLE_ENDIAN_BYTE_SEX);
#endif
	    best_fat_arch = cpusubtype_findbestarch(host_basic_info.cpu_type,
						    host_basic_info.cpu_subtype,
						    fat_archs,
						    fat_header->nfat_arch);
	    if(best_fat_arch == NULL){
		error("bad CPU type in library: %s", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADARCH, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    if(sizeof(struct mach_header) > best_fat_arch->size){
		error("truncated or malformed library: %s (file too small to "
		      "be a library)", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    mh = (struct mach_header *)(file_addr + best_fat_arch->offset);
	    if(mh->magic != MH_MAGIC){
		error("malformed library: %s (not a Mach-O file, bad magic "
		    "number)", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    /*
	     * This file has the right magic number so try to map it in. 
	     * map_library_image() will close the file descriptor and
	     * deallocate the mapped in memory.
	     */
	    return(map_library_image(dl, dylib_name, fd, file_addr, file_size,
			             best_fat_arch->offset, best_fat_arch->size,
			             stat_buf.st_dev, stat_buf.st_ino));
	}
	else{
	    if(sizeof(struct mach_header) > file_size){
		error("truncated or malformed library: %s (file too small to "
		      "be a library)", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    mh = (struct mach_header *)file_addr;
	    if(mh->magic == SWAP_LONG(MH_MAGIC)){
		error("bad CPU type in library: %s", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    if(mh->magic != MH_MAGIC){
		error("malformed library: %s (not a Mach-O file, bad magic "
		    "number)", dylib_name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
		goto load_library_image_cleanup1;
	    }
	    /*
	     * This file has the right magic number so try to map it in. 
	     * map_library_image() will close the file descriptor and
	     * deallocate the mapped in memory.
	     */
	    return(map_library_image(dl, dylib_name, fd, file_addr, file_size,
				     0, file_size, stat_buf.st_dev,
				     stat_buf.st_ino));
	}

load_library_image_cleanup1:
	if((r = vm_deallocate(mach_task_self(), (vm_address_t)file_addr,
	    (vm_size_t)file_size)) != KERN_SUCCESS){
	    mach_error(r, "can't vm_deallocate map_fd memory for library: %s",
		dylib_name);
	    link_edit_error(DYLD_MACH_RESOURCE, r, dylib_name);
	}

load_library_image_cleanup2:
	if(close(fd) == -1){
	    errnum = errno;
	    system_error(errnum, "can't close file descriptor for library: %s ",
		 dylib_name);
	    link_edit_error(DYLD_UNIX_RESOURCE, errnum, dylib_name);
	}
	return(FALSE);
}

/*
 * get_framework_name() is passed a name of a dynamic library and returns a
 * pointer to the start of the framework name if one exist or NULL none exists.
 * A framework name can take one of the following two forms:
 *	Foo.framework/Versions/A/Foo
 *	Foo.framework/Foo
 * Where A and Foo can be any string.
 */
static
char *
get_framework_name(
char *name,
enum bool with_suffix)
{
    char *foo, *a, *b, *c, *d, *suffix;
    unsigned long l, s;

	/* pull off the last component and make foo point to it */
	a = strrchr(name, '/');
	if(a == NULL)
	    return(NULL);
	if(a == name)
	    return(NULL);
	foo = a + 1;
	l = strlen(foo);
	
	/* look for suffix if requested starting with a '_' */
	if(with_suffix){
	    suffix = strrchr(foo, '_');
	    if(suffix != NULL){
		s = strlen(suffix);
		if(suffix == foo || s < 2)
		    suffix = NULL;
		else
		    l -= s;
	    }
	}

	/* first look for the form Foo.framework/Foo */
	b = look_back_for_slash(name, a);
	if(b == NULL){
	    if(strncmp(name, foo, l) == 0 &&
	       strncmp(name + l, ".framework/", sizeof(".framework/")-1 ) == 0)
		return(name);
	    else
		return(NULL);
	}
	else{
	    if(strncmp(b+1, foo, l) == 0 &&
	       strncmp(b+1 + l, ".framework/", sizeof(".framework/")-1 ) == 0)
		return(b+1);
	}

	/* next look for the form Foo.framework/Versions/A/Foo */
	if(b == name)
	    return(NULL);
	c = look_back_for_slash(name, b);
	if(c == NULL ||
	   c == name ||
	   strncmp(c+1, "Versions/", sizeof("Versions/")-1) != 0)
	    return(NULL);
	d = look_back_for_slash(name, c);
	if(d == NULL){
	    if(strncmp(name, foo, l) == 0 &&
	       strncmp(name + l, ".framework/", sizeof(".framework/")-1 ) == 0)
		return(name);
	    else
		return(NULL);
	}
	else{
	    if(strncmp(d+1, foo, l) == 0 &&
	       strncmp(d+1 + l, ".framework/", sizeof(".framework/")-1 ) == 0)
		return(d+1);
	    else
		return(NULL);
	}
}

/*
 * look_back_for_slash() is passed a string name and an end point in name to
 * start looking for '/' before the end point.  It returns a pointer to the
 * '/' back from the end point or NULL if there is none.
 */
static
char *
look_back_for_slash(
char *name,
char *p)
{
	for(p = p - 1; p >= name; p--){
	    if(*p == '/')
		return(p);
	}
	return(NULL);
}

/*
 * search_for_name_in_path() is used in searching for name in the
 * DYLD_LIBRARY_PATH or the DYLD_FRAMEWORK_PATH.  It is passed a name and a
 * path and returns the name of the first combination that exist or NULL if
 * none exists.
 */
static
char *
search_for_name_in_path(
char *name,
char *path)
{
    char *dylib_name, *p;
    struct stat stat_buf;

	dylib_name = allocate(strlen(name) + strlen(path) + 2);
	for(;;){
	    p = strchr(path, ':');
	    if(p != NULL)
		*p = '\0';
	    if(*path == '\0')
		goto next_path;
	    strcpy(dylib_name, path);
	    strcat(dylib_name, "/");
	    strcat(dylib_name, name);
	    if(stat(dylib_name, &stat_buf) == 0){
		if(p != NULL)
		    *p = ':';
		return(dylib_name);
	    }
	    if(p == NULL){
		free(dylib_name);
		return(NULL);
	    }
	    else{
next_path:
		*p = ':';
		path = p + 1;
	    }
	}
	/* can't get here */
	return(NULL);
}

/*
 * map_library_image() maps segments of the specified library into memory and
 * adds the library to the list of library images.
 */
static
enum bool
map_library_image(
struct dylib_command *dl, /* allow NULL for NSAddLibrary() to use this */
char *dylib_name,
int fd,
char *file_addr,
unsigned long file_size,
unsigned long library_offset,
unsigned long library_size,
dev_t dev,
ino_t ino)
{
    struct mach_header *mh;
    int errnum;
    kern_return_t r;
    unsigned long low_addr, high_addr, slide_value, seg1addr;
    struct segment_command *linkedit_segment, *mach_header_segment;
    struct dysymtab_command *dyst;
    struct symtab_command *st;
    struct dylib_command *dlid;
    enum bool change_protect_on_reloc, cache_sync_on_reloc;
    struct section *init, *term;
    struct library_image *library_image;
    struct dyld_event event;
    char *name;
    unsigned long images_dyld_stub_binding_helper;

#ifdef __ppc__
	images_dyld_stub_binding_helper =
	    (unsigned long)(&unlinked_lazy_pointer_handler);
#endif
	/*
	 * On entry the only checks that have been done are the file_addr and
	 * file_size have only been checked so that file_size >= sizeof(mach
	 * _header) and the magic number MH_MAGIC is correct.  The caller has
	 * checked that the library_offset to the library_size is contained in
	 * the file_size. file_addr is guaranteed to be on a page boundary as		 * allocated by mach.  All file format errors reported here will be
	 * DYLD_FILE_FORMAT and EBADMACHO.
	 */
	mh = (struct mach_header *)(file_addr + library_offset);
	if(check_image(dylib_name, "library", library_size, mh,
	    &linkedit_segment, &mach_header_segment, &dyst, &st, &dlid,
	    &low_addr, &high_addr) == FALSE)
	    goto map_library_image_cleanup;

	/*
	 * Do the library specific checks on the mach header and load commands.
	 */
	if(mh->filetype != MH_DYLIB){
	    error("malformed library: %s (not a Mach-O library file, bad "
		"filetype value)", dylib_name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
	    goto map_library_image_cleanup;
	}
	if(dlid == NULL){
	    error("malformed library: %s (Mach-O library file, does not have "
		"an LC_ID_DYLIB command)", dylib_name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, dylib_name);
	    goto map_library_image_cleanup;
	}
	/*
	 * If the compatibility version of the user of the library is
	 * greater than the library then it is an error and the
	 * library can't be used.
	 */
	if(dl != NULL &&
	   dl->dylib.compatibility_version >
	   dlid->dylib.compatibility_version){
	    error("version mismatch for library: %s (compatibility "
		"version of user: %lu.%lu.%lu greater than library's version: "
		"%lu.%lu.%lu)", dylib_name,
		dl->dylib.compatibility_version >> 16,
		(dl->dylib.compatibility_version >> 8) & 0xff,
		dl->dylib.compatibility_version & 0xff,
		dlid->dylib.compatibility_version >> 16,
		(dlid->dylib.compatibility_version >> 8) & 0xff,
		dlid->dylib.compatibility_version & 0xff);
	    link_edit_error(DYLD_FILE_FORMAT, ESHLIBVERS, dylib_name);
	    goto map_library_image_cleanup;
	}

	/*
	 * Now that the library checks out map the library in.
	 */
	if(map_image(dylib_name, "library", library_size, fd, file_addr, 
	    file_size, library_offset, library_size, low_addr, high_addr,
	    &mh, &linkedit_segment, &dyst, &st, &dlid, &change_protect_on_reloc,
	    &cache_sync_on_reloc, &init, &term, &seg1addr, &slide_value,
	    &images_dyld_stub_binding_helper) == FALSE){

	    return(FALSE);
	}

	/*
	 * To avoid allocating space for the dylib_name we use the name
	 * from the library's id command when it is the same as the
	 * dylib_name passed in.  As the space for the name in the library's
	 * id command will never go away as libraries are not unloaded but
	 * dylib_name may point at a load library command in an object that
	 * might be unloaded.
	 */
	name = (char *)dlid + dlid->dylib.name.offset;
	if(strcmp(dylib_name, name) == 0)
	    dylib_name = name;

	/*
	 * This library is now successfully mapped in add it to the list of
	 * libraries.
	 */
	library_image = new_library_image(dyst->nmodtab);
	library_image->image.name = dylib_name;
	library_image->image.vmaddr_slide = slide_value;
	library_image->image.vmaddr_size = high_addr - low_addr;
	library_image->image.seg1addr = seg1addr;
	library_image->image.mh = mh;
	library_image->image.st = st;
	library_image->image.dyst = dyst;
	library_image->image.linkedit_segment = linkedit_segment;
	library_image->image.change_protect_on_reloc = change_protect_on_reloc;
	library_image->image.cache_sync_on_reloc = cache_sync_on_reloc;
	library_image->image.init = init;
	library_image->image.term = term;
#ifdef __ppc__
	library_image->image.dyld_stub_binding_helper =
			     images_dyld_stub_binding_helper;
#endif
	library_image->dlid = dlid; 
	library_image->dev = dev;
	library_image->ino = ino;

	/*
	 * Do local relocation if this library was slid.  This also disables 
	 * prebinding and undoes the prebinding
	 */
	if(slide_value != 0){
	    if(dyld_prebind_debug != 0 &&
	       prebinding == TRUE &&
	       launched == FALSE)
		print("dyld: %s: prebinding disabled because library: %s got "
		       "slid\n", executables_name, dylib_name);
	    if(launched == FALSE)
		prebinding = FALSE;
	    local_relocation(&(library_image->image));
	    relocate_symbol_pointers_for_defined_externs(
		&(library_image->image));
	}

	/*
	 * If this library is not prebound then disable prebinding.
	 */
	if((mh->flags & MH_PREBOUND) != MH_PREBOUND){
	    if(dyld_prebind_debug != 0 &&
	       prebinding == TRUE &&
	       launched == FALSE)
		print("dyld: %s: prebinding disabled because library: %s not "
		       "prebound\n", executables_name, dylib_name);
	    if(launched == FALSE)
		prebinding = FALSE;
	}
	else{
	    /*
	     * The library is prebound.  If we have already launched the
	     * program we can't use the prebinding and it must be undone.
	     */
	    if(launched == TRUE)
		undo_prebinding_for_library(library_image);
	}

	/*
	 * Check to see if the time stamps match of the user of this library an
	 * in the id of this library.
	 */
	if(dl != NULL &&
	   dl->dylib.timestamp != dlid->dylib.timestamp){
	    if(prebinding == TRUE && launched == FALSE){
		/*
		 * The timestamps don't match so if we are not loading
		 * libraries from the executable then prebinding is always 
		 * disabled.  If we are loading libraries from the executable
		 * and the executable was prebound then also disable prebinding.
		 * This allows trying to use just prebound libraries.
		 */
		if(loading_executables_libraries == FALSE ||
		   executable_prebound == TRUE){
		    if(dyld_prebind_debug != 0)
			print("dyld: %s: prebinding disabled because time "
			       "stamp of library: %s did not match\n",
			       executables_name, dylib_name);
		    prebinding = FALSE;
		}
	    }
	}

	/*
	 * Set the segment protections on the library now that relocation is
	 * done.
	 */
	set_segment_protections(dylib_name, "library", mh, slide_value);

	/* send the event message that this image was added */
	memset(&event, '\0', sizeof(struct dyld_event));
	event.type = DYLD_IMAGE_ADDED;
	event.arg[0].header = mh;
	event.arg[0].vmaddr_slide = slide_value;
	event.arg[0].module_index = 0;
	send_event(&event);

	return(TRUE);

map_library_image_cleanup:
	/*
	 * The above label is used in error conditions before map_image() is
	 * called.  map_image() does this cleanup.
	 */
	if((r = vm_deallocate(mach_task_self(), (vm_address_t)file_addr,
	    (vm_size_t)file_size)) != KERN_SUCCESS){
	    mach_error(r, "can't vm_deallocate map_fd memory for library: %s",
		dylib_name);
	    link_edit_error(DYLD_MACH_RESOURCE, r, dylib_name);
	}
	if(close(fd) == -1){
	    errnum = errno;
	    system_error(errnum, "can't close file descriptor for library: %s ",
		 dylib_name);
	    link_edit_error(DYLD_UNIX_RESOURCE, errnum, dylib_name);
	}
	return(FALSE);
}

/*
 * map_bundle_image() maps segments of the specified bundle into memory and
 * adds the bundle to the list of object images.  This is used to implement the
 * NSloadModule() api.
 */
struct object_image *
map_bundle_image(
char *name,
char *object_addr,
unsigned long object_size)
{
    struct mach_header *mh;
    unsigned long low_addr, high_addr, slide_value, seg1addr;
    struct segment_command *linkedit_segment, *mach_header_segment;
    struct dysymtab_command *dyst;
    struct symtab_command *st;
    enum bool change_protect_on_reloc, cache_sync_on_reloc;
    struct section *init, *term;
    struct object_image *object_image;
    struct dyld_event event;
    unsigned long images_dyld_stub_binding_helper;
#ifdef __ppc__
	images_dyld_stub_binding_helper =
	    (unsigned long)(&unlinked_lazy_pointer_handler);
#endif

	/*
	 * This routine only deals with MH_BUNDLE files that are on page
	 * boundaries.  The library code for NSloadModule() insures this for
	 * it's call to here and deals with things that are not this type in
	 * the library code.
	 */
	if(((int)object_addr % vm_page_size) != 0){
	    error("malformed object file image: %s (address of image not on a "
		"page boundary)", name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(NULL);
	}
	if(sizeof(struct mach_header) > object_size){
	    error("truncated or malformed object file image: %s (too small to "
		"be an object file image)", name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(NULL);
	}
	mh = (struct mach_header *)object_addr;
	if(mh->magic != MH_MAGIC){
	    error("malformed object file image: %s (not a Mach-O image, bad "
		"magic number)", name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(NULL);
	}
	/*
	 * Now that it looks like it could be an object file image check it.
	 */
	if(check_image(name, "object file image", object_size, mh,
	    &linkedit_segment, &mach_header_segment, &dyst, &st, NULL,
	    &low_addr, &high_addr) == FALSE)
	    return(NULL);
	/*
	 * Do the bundle specific check on the mach header.
	 */
	if(mh->filetype != MH_BUNDLE){
	    error("malformed object file image: %s (not a Mach-O bundle file, "
		"bad filetype value)", name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(NULL);
	}

	/*
	 * Now that the object file image checks out map it in.
	 */
	if(map_image(name, "object file image", object_size, -1, NULL, 
	    0, 0, 0, low_addr, high_addr, &mh, &linkedit_segment, &dyst,
	    &st, NULL, &change_protect_on_reloc, &cache_sync_on_reloc, &init,
	    &term, &seg1addr, &slide_value, &images_dyld_stub_binding_helper)
		== FALSE)
	    return(NULL);

	/*
	 * This object file image is now successfully mapped in add it to the
	 * list of object image loaded.
	 */
	object_image = new_object_image();
	object_image->image.name = save_string(name);
	object_image->image.vmaddr_slide = slide_value;
	object_image->image.vmaddr_size = high_addr - low_addr;
	object_image->image.seg1addr = seg1addr;
	object_image->image.mh = mh;
	object_image->image.st = st;
	object_image->image.dyst = dyst;
	object_image->image.linkedit_segment = linkedit_segment;
	object_image->image.change_protect_on_reloc = change_protect_on_reloc;
	object_image->image.cache_sync_on_reloc = cache_sync_on_reloc;
	object_image->image.init = init;
	object_image->image.term = term;
#ifdef __ppc__
	object_image->image.dyld_stub_binding_helper =
			    images_dyld_stub_binding_helper;
#endif
	object_image->module = UNLINKED;

	/*
	 * Do local relocation if this object file image was slid.
	 */
	if(slide_value != 0){
	    local_relocation(&(object_image->image));
	    relocate_symbol_pointers_for_defined_externs(
		&(object_image->image));
	}

	/*
	 * Set the segment protections on the object file image now that
	 * relocation is done.
	 */
	set_segment_protections(name, "object file image", mh, slide_value);

	/* send the event message that this image was added */
	memset(&event, '\0', sizeof(struct dyld_event));
	event.type = DYLD_IMAGE_ADDED;
	event.arg[0].header = mh;
	event.arg[0].vmaddr_slide = slide_value;
	event.arg[0].module_index = 0;
	send_event(&event);

	/*
	 * Now load each of the libraries this object file image loads.
	 */
	if(dyld_print_libraries == TRUE)
	    print("loading libraries for image: %s\n",
		   object_image->image.name);
	load_images_libraries(mh);

	/*
	 * Return the module.
	 */
	return(object_image);
}

/*
 * unload_bundle_image() is the hack that unlinks a module loaded with
 * NSUnlinkModule().
 */
void
unload_bundle_image(
struct object_image *object_image,
enum bool keepMemoryMapped,
enum bool reset_lazy_references)
{
    kern_return_t r;

	/*
	 * If this is not a private image remove the defined symbols in the
	 * module and create undefined symbols if any of them are currently
	 * referenced.  Then check and report any undefined symbols that may
	 * have been created.
	 */
	if(object_image->image.private == FALSE){
	    unlink_object_module(object_image, reset_lazy_references);
	    check_and_report_undefineds();
	}

	/*
	 * Deallocate the memory for this image if keepMemoryMapped is FALSE.
	 */
	if(keepMemoryMapped == FALSE){
/*
printf("keepMemoryMapped == FALSE doing vm_deallocate() mh = 0x%x size = 0x%x\n",
object_image->image.mh, object_image->image.vmaddr_size);
*/
	    if((r = vm_deallocate(mach_task_self(),
			(vm_address_t)object_image->image.mh,
			(vm_size_t)object_image->image.vmaddr_size)) !=
			KERN_SUCCESS){
		mach_error(r, "can't vm_deallocate memory for module: %s",
			   object_image->image.name);
		link_edit_error(DYLD_MACH_RESOURCE, r,object_image->image.name);
	    }
	}

	/*
	 * Mark this object file image structure unused and clean it up.
	 * TODO: reclaim the storage for the name:
	unsave_string(object_image->image.name);
	 */
	memset(object_image, '\0', sizeof(struct object_image));
	object_image->module = UNUSED;

	return;
}


/*
 * map_image() maps an images' Mach-O segments into memory.  If sucessfull it
 * returns TRUE if not it returns false.  In either case if the image is from
 * a file the file descriptor is closed and the map_fd() memory is deallocated.
 */
static
enum bool
map_image(
    /* input */
char *name,
char *image_type,
unsigned long image_size,
int fd,
char *file_addr,
unsigned long file_size,
unsigned long library_offset,
unsigned long library_size,
unsigned long low_addr,
unsigned long high_addr,
    /* in/out */
struct mach_header **mh,
    /* output */
struct segment_command **linkedit_segment,
struct dysymtab_command **dyst,
struct symtab_command **st,
struct dylib_command **dlid,
enum bool *change_protect_on_reloc,
enum bool *cache_sync_on_reloc,
struct section **init,
struct section **term,
unsigned long *seg1addr,
unsigned long *slide_value,
unsigned long *images_dyld_stub_binding_helper)
{
    vm_address_t address, image_addr;
    vm_size_t size;
#ifdef __MACH30__
    vm_region_info_data_t info;
    mach_msg_type_number_t infoCnt;
#else
    vm_prot_t protection, max_protection;
    vm_inherit_t inheritance;
    boolean_t shared;
    vm_offset_t offset;
#endif
    mach_port_t object_name;
    enum bool slide_it, in_the_way;

    int errnum;
    kern_return_t r;
    unsigned long i, j;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    struct section *s;
    unsigned long mach_header_segment_vmaddr;

	/*
	 * We want this image at low_addr to high_addr so see if those
	 * vmaddresses are available to map in the image or will it have to be
	 * slid to an available address.  If the memory we have is from a
	 * mapped file and is the only thing in the way then we'll move it.
	 */
	slide_it = FALSE;
	in_the_way = FALSE;
	address = low_addr;
#ifdef __MACH30__
	infoCnt = sizeof(vm_region_info_t);
	r = vm_region(mach_task_self(), &address, &size, VM_REGION_BASIC_INFO, 
		      info, &infoCnt, &object_name);
#else
	r = vm_region(mach_task_self(), &address, &size, &protection,
		      &max_protection, &inheritance, &shared, &object_name,
		      &offset);
#endif
	/*
	 * If the return value is KERN_SUCCESS we found a vm_region at covers
	 * the low_addr or is above the low_addr.
	 */
	if(r == KERN_SUCCESS){
	    /*
	     * If the address of the region found is less than the high address
	     * needed for the library this region is where the library wants to
	     * be.
	     */ 
	    if(address < high_addr){
		/*
		 * If we have a file mapped see if the region is the memory
		 * we have the file mapped.
		 */
		if(fd != -1 && address == (vm_offset_t)file_addr){
		    /*
		     * This region is for the memory we have the file mapped so
		     * look for the next region after this one to see if it
		     * covers part of the wanted vmaddresses for the image.
		     */
		    in_the_way = TRUE;
		    address = (vm_offset_t)(file_addr + file_size);
#ifdef __MACH30__
		    infoCnt = sizeof(vm_region_info_t);
		    r = vm_region(mach_task_self(), &address, &size,
				  VM_REGION_BASIC_INFO, info, &infoCnt,
				  &object_name);
#else
		    r = vm_region(mach_task_self(), &address, &size, &protection,
				  &max_protection, &inheritance, &shared,
				  &object_name, &offset);
#endif
		    /*
		     * If we find a region and its address is less than the
		     * high address wanted for the image the image will
		     * have to be slid.
		     */
		    if(r == KERN_SUCCESS && address < high_addr)
			slide_it = TRUE;
		}
		/*
		 * There is some memory other than the memory we have the file
		 * mapped at the address we want for the image so the image
		 * will have to be slid.
		 */
		else{
		    slide_it = TRUE;
		}
	    }
	}

	/*
	 * Now that we know if we will have to slide the image or not, allocate
	 * the memory that will be used for the image's segments.
	 */
	if(slide_it == FALSE){
	    /*
	     * We don't have to slide the image but the map_fd memory for the
	     * file may be in the way and have to be moved so we can allocate
	     * the library where we want it.
	     */
	    *slide_value = 0;
	    if(in_the_way == TRUE){
		if((r = vm_deallocate(mach_task_self(), (vm_address_t)file_addr,
		    (vm_size_t)file_size)) != KERN_SUCCESS){
		    mach_error(r, "can't vm_deallocate map_fd memory for "
			"%s: %s", image_type, name);
		    link_edit_error(DYLD_MACH_RESOURCE, r, name);
		    goto map_image_cleanup2;
		}
	    }
	    if((r = vm_allocate(mach_task_self(), (vm_address_t *)&low_addr,
				high_addr - low_addr, FALSE)) != KERN_SUCCESS){
		slide_it = TRUE;
	    }
	    if(in_the_way == TRUE){
		if((r = map_fd((int)fd, 0, (vm_offset_t *)&file_addr,
		    (boolean_t)TRUE, (vm_size_t)file_size)) != KERN_SUCCESS){
		    mach_error(r, "can't map %s: %s", image_type, name);
		    link_edit_error(DYLD_MACH_RESOURCE, r, name);
		    if(slide_it == FALSE){
			if((r = vm_deallocate(mach_task_self(), (vm_address_t)
			    low_addr, (vm_size_t)(high_addr - low_addr))) !=
			    KERN_SUCCESS){
			    mach_error(r, "can't vm_deallocate memory to load "
				"in %s: %s", image_type, name);
			    link_edit_error(DYLD_MACH_RESOURCE, r, name);
			}
		    }
		    goto map_image_cleanup2;
		}
	    }
	}
	if(slide_it == TRUE){
	    if((r = vm_allocate(mach_task_self(), &address,high_addr - low_addr,
				TRUE)) != KERN_SUCCESS){
		mach_error(r, "can't vm_allocate memory to load in %s: %s",
		    image_type, name);
		link_edit_error(DYLD_MACH_RESOURCE, r, name);

		goto map_image_cleanup1;
	    }
	    *slide_value = address - low_addr;
	}

	/*
	 * Now that we have the memory allocated for the segments of the image
	 * map or vm_copy the parts of the segments from the file or memory
	 * into the memory for the segments.
	 */
	load_commands = (struct load_command *)((char *)*mh +
						sizeof(struct mach_header));
	*linkedit_segment = NULL;
	mach_header_segment_vmaddr = 0;
	lc = load_commands;
	for(i = 0; i < (*mh)->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		address = sg->vmaddr + *slide_value;
		if(fd != -1){
		    if((r = map_fd((int)fd, (vm_offset_t)sg->fileoff +
			library_offset, &address, FALSE,
			(vm_size_t)sg->filesize)) != KERN_SUCCESS){
			mach_error(r, "can't map segment: %.16s for %s: %s",
			    sg->segname, image_type, name);
			link_edit_error(DYLD_MACH_RESOURCE, r, name);
			goto map_image_cleanup0;
		    }
		}
		else{
		    image_addr = (vm_address_t)*mh;
		    if((r = vm_copy(mach_task_self(), image_addr + sg->fileoff,
			round(sg->filesize, vm_page_size), address)) !=
			KERN_SUCCESS){
			mach_error(r, "can't vm_copy segment: %.16s for %s: %s",
			    sg->segname, image_type, name);
			link_edit_error(DYLD_MACH_RESOURCE, r, name);
			goto map_image_cleanup0;
		    }
		}
		if(sg->fileoff == 0)
		    mach_header_segment_vmaddr = sg->vmaddr;
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}

	/*
	 * Cleanup the map_fd memory for the image file and close the file
	 * descriptor if we have one.
	 */
	if(fd != -1){
	    if((r = vm_deallocate(mach_task_self(), (vm_address_t)file_addr,
		(vm_size_t)file_size)) != KERN_SUCCESS){
		mach_error(r, "can't vm_deallocate map_fd memory for %s: %s",
		    image_type, name);
		link_edit_error(DYLD_MACH_RESOURCE, r, name);
	    }
	    if(close(fd) == -1){
		errnum = errno;
		system_error(errnum, "can't close file descriptor for %s: %s ",
		     image_type, name);
		link_edit_error(DYLD_UNIX_RESOURCE, errnum, name);
	    }
	}

	/*
	 * Reset the pointers to the mach_header, linkedit_segment, symbol
	 * table command and  dynamic symbol table command to the memory that
	 * mapped in the segments.
	 *
	 * Also determine the first segment address, if relocation entries are
	 * in read-only segments and if there are relocation entries for
	 * instructions.
	 */
	*mh = (struct mach_header *)((char *)mach_header_segment_vmaddr +
				     *slide_value);
	load_commands = (struct load_command *)((char *)*mh +
						sizeof(struct mach_header));
	lc = load_commands;
	*st = NULL;
	*dyst = NULL;
	if(dlid != NULL)
	    *dlid = NULL;
	*seg1addr = ULONG_MAX;
	*change_protect_on_reloc = FALSE;
	*cache_sync_on_reloc = FALSE;
	*init = NULL;
	*term = NULL;
	for(i = 0; i < (*mh)->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(strcmp(sg->segname, SEG_LINKEDIT) == 0){
		    if(*linkedit_segment == NULL)
			*linkedit_segment = sg;
		}
		/* pickup the address of the first segment */
		if(sg->vmaddr < *seg1addr)
		    *seg1addr = sg->vmaddr;
		/*
		 * Stuff the address of the stub_binding_helper_interface into
		 * the first 4 bytes of the (__DATA,__dyld) section if there is
		 * one.  And stuff the address of _dyld_func_lookup in the
		 * second 4 bytes of the (__DATA,__dyld) section.  And stuff the
		 * address of start_debug_thread in the third 4 bytes of the
		 * (__DATA,__dyld) section.
		 */
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0; j < sg->nsects; j++){
		    if(strcmp(s->segname, "__DATA") == 0 &&
		       strcmp(s->sectname, "__dyld") == 0){
			if(s->size >= sizeof(unsigned long)){
			    *((long *)(s->addr + *slide_value)) =
				(long)&stub_binding_helper_interface;
			}
			if(s->size >= 2 * sizeof(unsigned long)){
			    *((long *)(s->addr + *slide_value + 4)) =
				(long)&_dyld_func_lookup;
			}
			if(s->size >= 3 * sizeof(unsigned long)){
			    *((long *)(s->addr + *slide_value + 8)) =
				(long)&start_debug_thread;
			}
#ifdef __ppc__
			if(s->size >= 5 * sizeof(unsigned long)){
			    *images_dyld_stub_binding_helper = 
				*((long *)(s->addr + *slide_value + 20)) +
				*slide_value;
			}
#endif
		    }
		    /*
		     * If we are doing profiling call monaddition() for the
		     * sections that have instructions.
		     */
		    if(dyld_monaddition != NULL){
			/* TODO this should be based on SOME_INSTRUCTIONS */
			if(strcmp(s->segname, SEG_TEXT) == 0 &&
			   strcmp(s->sectname, SECT_TEXT) == 0){
			    if(s->size != 0){
				release_lock();
				dyld_monaddition(
				    (char *)(s->addr + *slide_value),
				    (char *)(s->addr + *slide_value + s->size));
				set_lock();
			    }
			}
		    }
#ifndef __MACH30__
		    else{
			if(profile_server == TRUE &&
			   strcmp(s->segname, SEG_TEXT) == 0 &&
			   strcmp(s->sectname, SECT_TEXT) == 0)
			    shared_pcsample_buffer(name, s, *slide_value);
		    }
#endif /* __MACH30__ */
		    s++;
		}
		/*
		 * If this segment is not to have write protection then check to
		 * see if any of the sections have external relocations and if
		 * so mark the image as needing to change protections when doing
		 * relocation in it.
		 */
		if((sg->initprot & VM_PROT_WRITE) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & S_ATTR_EXT_RELOC)){
			    *change_protect_on_reloc = TRUE;
			    break;
			}
			s++;
		    }
		}
		/*
		 * If the image has relocations for instructions then the
		 * instruction cache needs to be synchronized with the date
		 * cache on relocation.  A good guess is made based on the
		 * section attributes and section name.
		 */
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0; j < sg->nsects; j++){
		    if(((strcmp(s->segname, "__TEXT") == 0 &&
		         strcmp(s->sectname, "__text") == 0) ||
			 (s->flags & S_ATTR_SOME_INSTRUCTIONS)) &&
			 (s->flags & S_ATTR_EXT_RELOC)){
			*cache_sync_on_reloc = TRUE;
			break;
		    }
		    s++;
		}
		/*
		 * If the image has a module init section pick it up.
		 */
		if(*init == NULL){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & SECTION_TYPE) ==
			   S_MOD_INIT_FUNC_POINTERS){
			    *init = s;
			    break;
			}
			s++;
		    }
		}
		/*
		 * If the image has a module term section pick it up.
		 */
		if(*term == NULL){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0; j < sg->nsects; j++){
			if((s->flags & SECTION_TYPE) ==
			   S_MOD_TERM_FUNC_POINTERS){
			    *term = s;
			    break;
			}
			s++;
		    }
		}
		break;
	    case LC_SYMTAB:
		if(*st == NULL)
		    *st = (struct symtab_command *)lc;
		break;
	    case LC_DYSYMTAB:
		if(*dyst == NULL)
		    *dyst = (struct dysymtab_command *)lc;
		break;
	    case LC_ID_DYLIB:
		if(dlid != NULL && *dlid == NULL)
		    *dlid = (struct dylib_command *)lc;
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	return(TRUE);

map_image_cleanup0:
	if((r = vm_deallocate(mach_task_self(), (vm_address_t)low_addr,
	    (vm_size_t)(high_addr - low_addr))) != KERN_SUCCESS){
	    mach_error(r, "can't vm_deallocate memory to load in "
		"%s: %s", image_type, name);
	    link_edit_error(DYLD_MACH_RESOURCE, r, name);
	}
map_image_cleanup1:
	if(fd != -1){
	    if((r = vm_deallocate(mach_task_self(), (vm_address_t)file_addr,
		(vm_size_t)file_size)) != KERN_SUCCESS){
		mach_error(r, "can't vm_deallocate map_fd memory for %s: %s",
		    image_type, name);
		link_edit_error(DYLD_MACH_RESOURCE, r, name);
	    }
	}
map_image_cleanup2:
	if(fd != -1){
	    if(close(fd) == -1){
		errnum = errno;
		system_error(errnum, "can't close file descriptor for %s: %s ",
		     image_type, name);
		link_edit_error(DYLD_UNIX_RESOURCE, errnum, name);
	    }
	}
	return(FALSE);
}

#ifndef __MACH30__
/*
 * shared_pcsample_buffer() is called with a name of a dynamic library, a
 * section pointer and the slide_value of the library.  If their is a shared
 * pcsample buffer for this library then the buffer file is mapped in shared
 * and a profil(2) call is made.
 */
void
shared_pcsample_buffer(
char *name,
struct section *s,
unsigned long slide_value)
{
    struct stat stat_buf;
    unsigned long size, expected_size;
    kern_return_t r;
    char *buf, *rbuf;
    int fd;
    char gmon_out[MAXPATHLEN];
    static enum bool first_time = TRUE;
#ifdef __OPENSTEP__
    struct phdr profile_header;
#else
    struct gmonhdr profile_header;
#endif

	/*
	 * Contact the server and see if for the shared library "name" we have
	 * a pcsample buffer file.
	 */
	if(dyld_sample_debug == 2)
	    print("calling buffer_for_dylib for: %s\n", name);
	if(buffer_for_dylib(name, gmon_out) == FALSE){
	    if(dyld_sample_debug == 2)
		print("buffer_for_dylib for: %s returned FALSE\n", name);
	    return;
	}
	if(dyld_sample_debug == 2)
	    print("buffer_for_dylib for: %s returned: %s\n", name, gmon_out);
	fd = open(gmon_out, O_RDWR, 0);
	if(fd == -1){
	    if(dyld_sample_debug)
		print("can't open: %s for: %s\n", gmon_out, name);
	    return;
	}
	if(fstat(fd, &stat_buf) == -1){
	    if(dyld_sample_debug)
		print("can't stat: %s for: %s\n", gmon_out, name);
	    (void)close(fd);
	    return;
	}
	size = stat_buf.st_size;
	/*
	 * The size of the pcsample buffer file should be exactly the right
	 * size for SCALE_1_TO_1 mapping.
	 */
	expected_size = round(s->size / 1, sizeof(unsigned short)) +
		        sizeof(profile_header);
	if(size != expected_size){
	    if(dyld_sample_debug)
		print("size of: %s for: %s is %ld, expected %ld\n", gmon_out,
		       name, size, expected_size);
	    (void)close(fd);
	    return;
	}
#ifndef MWATSON
	r = vm_allocate(mach_task_self(), (vm_address_t *)&buf, size, TRUE);
	if(r != KERN_SUCCESS){
	    if(dyld_sample_debug)
		print("can't vm_allocate buffer to map: %s for: %s\n",
		       gmon_out, name);
	    (void)close(fd);
	    return;
	}
	rbuf = (char *)mmap(buf, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#else
	rbuf = (char *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#endif
#ifndef __OPENSTEP__
	if(rbuf == NULL)
#else
	if((int)rbuf == -1)
#endif
	{
	    if(dyld_sample_debug)
		print("can't mmap: %s for: %s\n", gmon_out, name);
	    goto cleanup;
	}
	(void)close(fd);

	if(first_time == TRUE){
	    if(profil(rbuf + sizeof(profile_header),
		      size - sizeof(profile_header),
		      (int)s->addr + slide_value,
		     SCALE_1_TO_1) == -1){
		if(dyld_sample_debug)
		    print("profil failed: %s for: %s\n", gmon_out, name);
		goto cleanup;
	    }
	    first_time = FALSE;
	}
	else{
	    if(add_profil(rbuf + sizeof(profile_header),
			  size - sizeof(profile_header),
			 (int)s->addr + slide_value,
			 SCALE_1_TO_1) == -1){
		if(dyld_sample_debug)
		    print("profil failed: %s for: %s\n", gmon_out, name);
		goto cleanup;
	    }
	}
	if(dyld_sample_debug == 2)
	    print("successfully set up: %s for: %s\n", gmon_out, name);
	return;

cleanup:
	(void)close(fd);
#ifndef MWATSON
	r = vm_deallocate(mach_task_self(), (vm_address_t)buf, (vm_size_t)size);
	if(r != KERN_SUCCESS){
	    mach_error(r, "can't vm_deallocate shared pcsample buffer "
		" memory for %s", name);
	    link_edit_error(DYLD_MACH_RESOURCE, r, name);
	}
#endif
	return;
}
#endif /* __MACH30__ */

static
void
set_segment_protections(
char *name,
char *image_type,
struct mach_header *mh,
unsigned long slide_value)
{
    unsigned long i;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    vm_address_t address;
    kern_return_t r;

	/*
	 * Set the initial protection of the segments.  The maximum protection
	 * is not set in case there is relocation to be done later.
	 */
	load_commands = (struct load_command *)((char *)mh +
						sizeof(struct mach_header));
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		address = sg->vmaddr + slide_value;
		if((r = vm_protect(mach_task_self(), address,
				   (vm_size_t)sg->vmsize,
				   FALSE, sg->initprot)) != KERN_SUCCESS){
		    mach_error(r, "can't vm_protect segment: %.16s for %s:"
			" %s", sg->segname, image_type, name);
		    link_edit_error(DYLD_MACH_RESOURCE, r, name);
		}
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * load_dependent_libraries() loads the dependent libraries for libraries that
 * have not had their dependent libraries loaded.  This is done this way to get
 * the proper order of libraries.  The proper order is to have all the libraries
 * in the executable before any of the dependent libraries.  This allows the
 * executable to over ride a dependent library of a library it uses.
 */
void
load_dependent_libraries(
void)
{
    unsigned long i;
    struct library_images *q;

	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		if(q->images[i].dependent_libraries_loaded == FALSE){
		    if(dyld_print_libraries == TRUE)
			print("loading libraries for image: %s\n",
			       q->images[i].image.name);
		    load_images_libraries(q->images[i].image.mh);
		    q->images[i].dependent_libraries_loaded = TRUE;
		}
	    }
	    q = q->next_images;
	}while(q != NULL);
}

static
void
load_images_libraries(
struct mach_header *mh)
{
    unsigned long i;
    struct load_command *lc, *load_commands;
    struct dylib_command *dl_load;

	/*
	 * Load each of the libraries this image uses.
	 */
	load_commands = (struct load_command *)((char *)mh +
						sizeof(struct mach_header));
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_LOAD_DYLIB:
		dl_load = (struct dylib_command *)lc;
		(void)load_library_image(dl_load, NULL);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * check_linkedit_info() checks the mach_header and load_commands of an image.
 * The image is assumed to be mapped into memory at the mach_header pointer for
 * a sizeof image_size.  The strings name and image_type are used for error
 * messages.  TRUE is returned if everything is ok else FALSE is returned after 
 * link_edit_error() is called.  A bunch of things from the Mach-O image are
 * returned.
 */
static
enum bool
check_image(
    /* inputs */
char *name,
char *image_type,
unsigned long image_size,
struct mach_header *mh,
    /* outputs */
struct segment_command **linkedit_segment,
struct segment_command **mach_header_segment,
struct dysymtab_command **dyst,
struct symtab_command **st,
struct dylib_command **dlid,
unsigned long *low_addr,
unsigned long *high_addr)
{
    unsigned long i, j;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    struct dylib_command *dl_load;
    char *load_dylib_name;

	*linkedit_segment = NULL;
	*mach_header_segment = NULL;
	*st = NULL;
	*dyst = NULL;
	if(dlid != NULL)
	    *dlid = NULL;
	*low_addr = ULONG_MAX;
	*high_addr = 0;

	/*
	 * Do the needed remaining checks on the mach header.  The caller needs
	 * to check the filetype and if it is a library to check the dlid passed
	 * back for compatibility.
	 */
	if(mh->cputype != host_basic_info.cpu_type){
	    error("bad CPU type in %s: %s", image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADARCH, name);
	    return(FALSE);
	}
	if(cpusubtype_combine(host_basic_info.cpu_type,
			  host_basic_info.cpu_subtype, mh->cpusubtype) == -1){
	    error("bad CPU subtype in %s: %s", image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADARCH, name);
	    return(FALSE);
	}
	if(mh->sizeofcmds + sizeof(struct mach_header) > image_size){
	    error("truncated or malformed %s: %s (load commands extend "
		"past the end of the image)", image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(FALSE);
	}

	/*
	 * Cycle through the load commands doing the minimum checks for the
	 * things we need.  Checks for duplicate things are not done and just
	 * the first one found is used and checked.
	 *
	 * Pick up the linkedit segment, the segment mapping the mach header,
	 * the symbol table command, the dynamic symbol command and the dynamic
	 * library identification command from the image.  We don't check for
	 * the error of having more than one of these but just pick up the
	 * first one if any. 
	 *
	 * Determined the lowest and highest address needed to cover the segment
	 * commands.  These images are suppose to be contiguious but that is not
	 * checked here.  If it isn't it might fail to load because the spread
	 * covers more address space than we can allocate.
	 */
	load_commands = (struct load_command *)((char *)mh +
						sizeof(struct mach_header));
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0){
		error("truncated or malformed %s: %s (load command %lu "
		    "size not a multiple of sizeof(long))",image_type, name, i);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(lc->cmdsize == 0){
		error("truncated or malformed %s: %s (load command %lu "
		    "size is equal to zero)", image_type, name, i);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if((char *)lc + lc->cmdsize >
	       (char *)load_commands + mh->sizeofcmds){
		error("truncated or malformed %s: %s (load command %lu "
		    "extends past end of all load commands)", image_type,
		    name, i);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(sg->fileoff > image_size){
		    error("truncated or malformed %s: %s (load command "
			"%lu fileoff extends past end of the library)",
			image_type, name, i);
		    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		    return(FALSE);
		}
		if(sg->fileoff + sg->filesize > image_size){
		    error("truncated or malformed %s: %s (load command "
			"%lu fileoff plus filesize extends past end of the "
			"library)", image_type, name, i);
		    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		    return(FALSE);
		}
		if(sg->cmdsize != sizeof(struct segment_command) +
				     sg->nsects * sizeof(struct section)){
		    error("malformed %s: %s (cmdsize field of load "
			"command %lu is inconsistant for a segment command "
			"with the number of sections it has)", image_type,
			name, i);
		    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		    return(FALSE);
		}
		if(strcmp(sg->segname, SEG_LINKEDIT) == 0){
		    if(*linkedit_segment == NULL)
			*linkedit_segment = sg;
		}
		if(sg->fileoff == 0)
		    *mach_header_segment = sg;
		if(sg->vmaddr < *low_addr)
		    *low_addr = sg->vmaddr;
		if(sg->vmaddr + sg->vmsize > *high_addr)
		    *high_addr = sg->vmaddr + sg->vmsize;
		break;

	    case LC_DYSYMTAB:
		if(*dyst == NULL)
		    *dyst = (struct dysymtab_command *)lc;
		break;

	    case LC_SYMTAB:
		if(*st == NULL)
		    *st = (struct symtab_command *)lc;
		break;

	    case LC_ID_DYLIB:
		if(dlid != NULL && *dlid == NULL){
		    *dlid = (struct dylib_command *)lc;
		    if((*dlid)->cmdsize < sizeof(struct dylib_command)){
			error("truncated or malformed %s: %s (cmdsize of "
			    "load command %lu incorrect for LC_ID_DYLIB)",
			    image_type, name, i);
			link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
			return(FALSE);
		    }
		}
		break;

	    case LC_LOAD_DYLIB:
		dl_load = (struct dylib_command *)lc;
		if(dl_load->cmdsize < sizeof(struct dylib_command)){
		    error("truncated or malformed %s: %s (cmdsize of load "
			"command %lu incorrect for LC_LOAD_DYLIB)", image_type,
			name, i);
		    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		    return(FALSE);
		}
		if(dl_load->dylib.name.offset >= dl_load->cmdsize){
		    error("truncated or malformed %s: %s (name.offset of "
			"load command %lu extends past the end of the load "
			"command)", image_type, name, i); 
		    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		    return(FALSE);
		}
		load_dylib_name = (char *)dl_load + dl_load->dylib.name.offset;
		for(j = 0;
		    j < dl_load->cmdsize - dl_load->dylib.name.offset;
		    j++){
		    if(load_dylib_name[j] == '\0')
			break;
		}
		if(j >= dl_load->cmdsize - dl_load->dylib.name.offset){
		    error("truncated or malformed %s: %s (library name of "
			"load command %lu not null terminated)", image_type,
			name, i);
		}
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	if(*mach_header_segment == NULL){
	    error("malformed %s: %s (no segment command maps the mach_header)",
		image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(FALSE);
	}
	if(*st == NULL){
	    error("malformed %s: %s (no symbol table command)", image_type,
		name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(FALSE);
	}
	if(*dyst == NULL){
	    error("malformed %s: %s (no dynamic symbol table command)",
		image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(FALSE);
	}
	if(*linkedit_segment == NULL){
	    error("malformed %s: %s (no " SEG_LINKEDIT "segment)",
		image_type, name);
	    link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
	    return(FALSE);
	}
	if(check_linkedit_info(name, image_type, *linkedit_segment, *st,
			       *dyst) == FALSE)
	    return(FALSE);

	return(TRUE);
}

/*
 * check_linkedit_info() check to see if the offsets and spans of the linkedit
 * information for the symbol table command and dynamic symbol table command
 * are contained in the link edit segment.  TRUE is returned if everything is
 * ok else FALSE is returned after link_edit_error() is called.  These are the
 * only checks done on the linkedit information.  The individual entries will
 * be assumed to be correct.
 */
static
enum bool
check_linkedit_info(
char *name,
char *image_type,
struct segment_command *linkedit_segment,
struct symtab_command *st,
struct dysymtab_command *dyst)
{
	if(st->nsyms != 0){
	    if(st->symoff < linkedit_segment->fileoff ||
	       st->symoff > linkedit_segment->fileoff +
			    linkedit_segment->filesize){
		error("malformed %s: %s (offset to symbol table not in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(st->symoff + st->nsyms * sizeof(struct nlist) <
	       linkedit_segment->fileoff ||
	       st->symoff + st->nsyms * sizeof(struct nlist) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (symbol table not contained in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(st->strsize != 0){
	    if(st->stroff < linkedit_segment->fileoff ||
	       st->stroff > linkedit_segment->fileoff +
			    linkedit_segment->filesize){
		error("malformed %s: %s (offset to string table not in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(st->stroff + st->strsize < linkedit_segment->fileoff ||
	       st->stroff + st->strsize >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (string table not contained in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nlocalsym != 0){
	    if(dyst->ilocalsym > st->nsyms){
		error("malformed %s: %s (ilocalsym in LC_DYSYMTAB load command "
		    "extends past the end of the symbol table", image_type,
		    name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->ilocalsym + dyst->nlocalsym > st->nsyms){
		error("malformed %s: %s (ilocalsym plus nlocalsym in "
		    "LC_DYSYMTAB load command extends past the end of the "
		    "symbol table", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nextdefsym != 0){
	    if(dyst->iextdefsym > st->nsyms){
		error("malformed %s: %s (iextdefsym in LC_DYSYMTAB load "
		    "command extends past the end of the symbol table",
		    image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->iextdefsym + dyst->nextdefsym > st->nsyms){
		error("malformed %s: %s (iextdefsym plus nextdefsym in "
		    "LC_DYSYMTAB load command extends past the end of the "
		    "symbol table", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nundefsym != 0){
	    if(dyst->iundefsym > st->nsyms){
		error("malformed %s: %s (iundefsym in LC_DYSYMTAB load command "
		    "extends past the end of the symbol table", image_type,
		    name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->iundefsym + dyst->nundefsym > st->nsyms){
		error("malformed %s: %s (iundefsym plus nundefsym in "
		    "LC_DYSYMTAB load command extends past the end of the "
		    "symbol table", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->ntoc != 0){
	    if(dyst->tocoff < linkedit_segment->fileoff ||
	       dyst->tocoff > linkedit_segment->fileoff +
			      linkedit_segment->filesize){
		error("malformed %s: %s (offset to table of contents not in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->tocoff +
	       dyst->ntoc * sizeof(struct dylib_table_of_contents) <
	       linkedit_segment->fileoff ||
	       dyst->tocoff +
	       dyst->ntoc * sizeof(struct dylib_table_of_contents) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (table of contents not contained in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nmodtab != 0){
	    if(dyst->modtaboff < linkedit_segment->fileoff ||
	       dyst->modtaboff > linkedit_segment->fileoff +
			         linkedit_segment->filesize){
		error("malformed %s: %s (offset to module table not in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->modtaboff + dyst->nmodtab * sizeof(struct dylib_module) <
	       linkedit_segment->fileoff ||
	       dyst->modtaboff + dyst->nmodtab * sizeof(struct dylib_module) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (module table not contained in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nextrefsyms != 0){
	    if(dyst->extrefsymoff < linkedit_segment->fileoff ||
	       dyst->extrefsymoff > linkedit_segment->fileoff +
			            linkedit_segment->filesize){
		error("malformed %s: %s (offset to referenced symbol table not "
		    "in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->extrefsymoff +
	       dyst->nextrefsyms * sizeof(struct dylib_reference) <
	       linkedit_segment->fileoff ||
	       dyst->extrefsymoff +
	       dyst->nextrefsyms * sizeof(struct dylib_reference) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (referenced table not contained in "
		    SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nindirectsyms != 0){
	    if(dyst->indirectsymoff < linkedit_segment->fileoff ||
	       dyst->indirectsymoff > linkedit_segment->fileoff +
			              linkedit_segment->filesize){
		error("malformed %s: %s (offset to indirect symbol table not "
		    "in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->indirectsymoff +
	       dyst->nindirectsyms * sizeof(unsigned long) <
	       linkedit_segment->fileoff ||
	       dyst->indirectsymoff +
	       dyst->nindirectsyms * sizeof(unsigned long) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (indirect symbol table not contained "
		    "in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nextrel != 0){
	    if(dyst->extreloff < linkedit_segment->fileoff ||
	       dyst->extreloff > linkedit_segment->fileoff +
			         linkedit_segment->filesize){
		error("malformed %s: %s (offset to external relocation entries "
		    "not in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->extreloff +
	       dyst->nextrel * sizeof(struct relocation_info) <
	       linkedit_segment->fileoff ||
	       dyst->extreloff +
	       dyst->nextrel * sizeof(struct relocation_info) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (external relocation entries not "
		    "contained in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	if(dyst->nlocrel != 0){
	    if(dyst->locreloff < linkedit_segment->fileoff ||
	       dyst->locreloff > linkedit_segment->fileoff +
			         linkedit_segment->filesize){
		error("malformed %s: %s (offset to local relocation entries "
		    "not in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	    if(dyst->locreloff +
	       dyst->nlocrel * sizeof(struct relocation_info) <
	       linkedit_segment->fileoff ||
	       dyst->locreloff +
	       dyst->nlocrel * sizeof(struct relocation_info) >
	       linkedit_segment->fileoff + linkedit_segment->filesize){
		error("malformed %s: %s (local relocation entries not "
		    "contained in " SEG_LINKEDIT " segment)", image_type, name);
		link_edit_error(DYLD_FILE_FORMAT, EBADMACHO, name);
		return(FALSE);
	    }
	}
	return(TRUE);
}

/*
 * save_string() is passed the name of an object file image (or some other
 * string) and returns a pointer to a copy of the name that has been saved
 * away.  The name is saved in either the string table or in some malloc()'ed
 * area.
 */
char *
save_string(
char *string)
{
    unsigned long len;
    char *p;

	len = strlen(string) + 1;
	if(len <= STRING_BLOCK_SIZE - string_block.used){
	    p = string_block.strings + string_block.used;
	    strcpy(p, string);
	    string_block.used += len;
	}
	else{
	    p = allocate(len);
	    strcpy(p, string);
	}
	return(p);
}

/*
 * new_object_image() allocates an object_image structure on the list of object
 * images and returns a pointer to it.
 */
static
struct object_image *
new_object_image(
void)
{
    struct object_images *p;
    unsigned long i;
    enum link_state link_state;

	for(p = &object_images ; ; p = p->next_images){
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused reuse it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    return(p->images + i);
	    }
	    if(p->nimages != NOBJECT_IMAGES){
		return(p->images + p->nimages++);
	    }
	    if(p->next_images == NULL)
		break;
	}
	p->next_images = allocate(sizeof(struct object_images));
	memset(p->next_images, '\0',  sizeof(struct object_images));
	return(p->next_images->images + p->next_images->nimages++);
}

/*
 * new_library_image() allocates a library_image structure on the list of
 * library images and returns a pointer to it.  It also allocates nmodule
 * structures for the library_image and fills in the pointer to the module
 * structure in the library_image and the count of modules.
 */
static
struct library_image *
new_library_image(
unsigned long nmodules)
{
    struct library_images *p;
    struct library_image *library_image;

	for(p = &library_images ; ; p = p->next_images){
	    if(p->nimages != NLIBRARY_IMAGES){
		library_image = p->images + p->nimages++;
		library_image->nmodules = nmodules;
		library_image->modules = allocate_module_states(nmodules);
		memset(library_image->modules, '\0',
		       sizeof(module_state) * nmodules);
		return(library_image);
	    }
	    if(p->next_images == NULL)
		break;
	}
	p->next_images = allocate(sizeof(struct library_images));
	memset(p->next_images, '\0',  sizeof(struct library_images));
	library_image = p->next_images->images + p->next_images->nimages++;
	library_image->nmodules = nmodules;
	library_image->modules = allocate_module_states(nmodules);
	memset(library_image->modules, '\0', sizeof(module_state) * nmodules);
	return(library_image);
}

/*
 * allocate_module_states() is passed the number of modules in a library and
 * returns a pointer to that many module_states.  The module_states either come
 * from the block of module states in the module_state_block or are malloc()'ed.
 */
static
module_state *
allocate_module_states(
unsigned long nmodules)
{
    module_state *p;

	if(nmodules <= MODULE_STATE_BLOCK_SIZE - module_state_block.used){
	    p = module_state_block.module_states + module_state_block.used;
	    module_state_block.used += nmodules;
	}
	else{
	    p = allocate(sizeof(module_state) * nmodules);
	}
	return(p);
}

/*
 * is_library_loaded() returns TRUE if the library is already loaded.  Also it
 * reports a error if the loaded library's compatibility_version is less than
 * the one the load command requires.
 */
static
enum bool
is_library_loaded(
char *dylib_name,
struct dylib_command *dl)
{
    unsigned long i;
    struct library_images *p;
    struct stat stat_buf;

	for(p = &library_images; p != NULL; p = p->next_images){
	    for(i = 0; i < p->nimages; i++){
		if(strcmp(dylib_name, p->images[i].image.name) == 0){
		    if(dl != NULL &&
		       dl->dylib.compatibility_version >
		       p->images[i].dlid->dylib.compatibility_version){
			error("version mismatch for library: %s (compatibility "
			    "version of user: %lu.%lu.%lu greater than "
			    "library's version: %lu.%lu.%lu)", dylib_name,
			    dl->dylib.compatibility_version >> 16,
			    (dl->dylib.compatibility_version >> 8) & 0xff,
			    dl->dylib.compatibility_version & 0xff,
			    p->images[i].dlid->dylib.compatibility_version
				>> 16,
			    (p->images[i].dlid->dylib.compatibility_version
				>> 8) & 0xff,
			    p->images[i].dlid->dylib.compatibility_version &
				0xff);
			link_edit_error(DYLD_FILE_FORMAT,ESHLIBVERS,dylib_name);
		    }
		    /*
		     * If this library's time stamps do not match then disable
		     * prebinding.
		     */
		    if(dl == NULL || 
		       dl->dylib.timestamp !=
		       p->images[i].dlid->dylib.timestamp){
			if(dyld_prebind_debug != 0 &&
			   prebinding == TRUE &&
			   launched == FALSE)
			    print("dyld: %s: prebinding disabled because time "
				   "stamp of library: %s did not match\n",
				   executables_name, dylib_name);
			if(launched == FALSE)
			    prebinding = FALSE;
		    }
		    return(TRUE);
		}
	    }
	}

	/*
	 * This maybe a the same library but as a different name in the file
	 * system.  So try to stat() it and then compare the device and inode
	 * pair for a match.
	 */
	if(stat(dylib_name, &stat_buf) == -1)
	    return(FALSE);
	for(p = &library_images; p != NULL; p = p->next_images){
	    for(i = 0; i < p->nimages; i++){
		if(stat_buf.st_dev == p->images[i].dev &&
		   stat_buf.st_ino == p->images[i].ino){
		    if(dl != NULL &&
		       dl->dylib.compatibility_version >
		       p->images[i].dlid->dylib.compatibility_version){
			error("version mismatch for library: %s (compatibility "
			    "version of user: %lu.%lu.%lu greater than "
			    "library's version: %lu.%lu.%lu)", dylib_name,
			    dl->dylib.compatibility_version >> 16,
			    (dl->dylib.compatibility_version >> 8) & 0xff,
			    dl->dylib.compatibility_version & 0xff,
			    p->images[i].dlid->dylib.compatibility_version
				>> 16,
			    (p->images[i].dlid->dylib.compatibility_version
				>> 8) & 0xff,
			    p->images[i].dlid->dylib.compatibility_version
				& 0xff);
			link_edit_error(DYLD_FILE_FORMAT,ESHLIBVERS,dylib_name);
		    }
		    /*
		     * If this library's time stamps do not match then disable
		     * prebinding.
		     */
		    if(dl == NULL || 
		       dl->dylib.timestamp !=
		       p->images[i].dlid->dylib.timestamp){
			if(dyld_prebind_debug != 0 &&
			   prebinding == TRUE &&
			   launched == FALSE)
			    print("dyld: %s: prebinding disabled because time "
				   "stamp of library: %s did not match\n",
				   executables_name, dylib_name);
			if(launched == FALSE)
			    prebinding = FALSE;
		    }
		    return(TRUE);
		}
	    }
	}

	return(FALSE);
}

/*
 * set_images_to_prebound() is called to set the modules of the images to their
 * prebound state.  If successfull it returns TRUE else FALSE.
 */
enum bool
set_images_to_prebound(
void)
{
    unsigned long i, j;
    struct load_command *lc;
    struct prebound_dylib_command *pbdylib;
    struct library_images *q;

	/*
	 * Walk through the executable's load commands for LC_PREBOUND_DYLIB
	 * commands setting the module's state for the specified library.
	 */
	lc = (struct load_command *)((char *)object_images.images[0].image.mh +
				    sizeof(struct mach_header));
	for(i = 0; i < object_images.images[0].image.mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_PREBOUND_DYLIB:
		pbdylib = (struct prebound_dylib_command *)lc;
		if(set_prebound_state(pbdylib) == FALSE){
		    reset_module_states();
		    return(FALSE);
		}
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}

	/*
	 * Check to see that all the libraries got their state set to the
	 * prebound state.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		if(q->images[i].image.prebound != TRUE){
		    if(dyld_prebind_debug != 0)
			print("dyld: %s: prebinding disabled because no "
			       "LC_PREBOUND_DYLIB for library: %s\n",
				executables_name,
				(char *)q->images[i].dlid +
				q->images[i].dlid->dylib.name.offset);
		    prebinding = FALSE;
		    reset_module_states();
		    return(FALSE);
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	/*
	 * Every thing checks so set the executable to fully linked and all the
	 * library modules that did not get set to fully linked to prebound
	 * unlinked.  Then return TRUE.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		for(j = 0; j < q->images[i].nmodules; j++)
		    if(q->images[i].modules[j] != FULLY_LINKED)
			q->images[i].modules[j] = PREBOUND_UNLINKED;
	    }
	    q = q->next_images;
	}while(q != NULL);

	object_images.images[0].module = FULLY_LINKED;
	call_registered_funcs_for_add_images();
	return(TRUE);
}

/*
 * set_prebound_state() takes a prebound_dylib_command and sets the modules of
 * the specified library to the fully linked state for the linked modules.
 * If the prebound_dylib_command refers to a library that is not loaded or it
 * has the wrong number of modules or it is not the first one referencing the
 * library FALSE is returned.  Otherwise it is successfull and TRUE is returned.
 */
static
enum bool
set_prebound_state(
struct prebound_dylib_command *pbdylib)
{
    unsigned long i, j;
    char *name, *linked_modules, *install_name;
    struct library_images *q;

	name = (char *)pbdylib + pbdylib->name.offset;
	linked_modules = (char *)pbdylib + pbdylib->linked_modules.offset;
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		install_name = (char *)q->images[i].dlid +
				q->images[i].dlid->dylib.name.offset;
		if(strcmp(install_name, name) == 0){
		    if(q->images[i].image.prebound == TRUE ||
		       q->images[i].image.dyst->nmodtab != pbdylib->nmodules){
			if(dyld_prebind_debug != 0)
			    print("dyld: %s: prebinding disabled because "
				   "nmodules in LC_PREBOUND_DYLIB for library: "
				   "%s does not match\n",executables_name,name);
			prebinding = FALSE;
			return(FALSE);
		    }
		    for(j = 0; j < q->images[i].nmodules; j++){
			if((linked_modules[j/8] >> (j%8)) & 1)
			    q->images[i].modules[j] = FULLY_LINKED;
		    }
		    q->images[i].image.prebound = TRUE;
		    return(TRUE);
		}
	    }
	    q = q->next_images;
	}while(q != NULL);
	if(dyld_prebind_debug != 0)
	    print("dyld: %s: prebinding disabled because LC_PREBOUND_DYLIB "
		   "found for library: %s but it was not loaded\n",
		   executables_name, name);
	prebinding = FALSE;
	return(FALSE);
}

/*
 * undo_prebound_images() is called when the prebound state of the images can't
 * be used.  This undoes the prebound state.  Note that if the library image
 * was slid then it's local relocation has already been done.
 */
void
undo_prebound_images(
void)
{
    unsigned long i, j;
    struct object_images *p;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct relocation_info *relocs;
    struct nlist *symbols;
    char *strings;
    struct load_command *lc;
    struct segment_command *sg;
    kern_return_t r;

	/*
	 * First undo the prebinding for the object images.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* if this image was not prebound skip it */
		if((p->images[i].image.mh->flags & MH_PREBOUND) != MH_PREBOUND)
		    continue;

		/*
		 * If the image has relocations in read-only segments and the
		 * protection needs to change change it.
		 */
		if(p->images[i].image.change_protect_on_reloc){
		    lc = (struct load_command *)((char *)p->images[i].image.mh +
			    sizeof(struct mach_header));
		    for(j = 0; j < p->images[i].image.mh->ncmds; j++){
			switch(lc->cmd){
			case LC_SEGMENT:
			    sg = (struct segment_command *)lc;
			    if((r = vm_protect(mach_task_self(), sg->vmaddr +
				p->images[i].image.vmaddr_slide,
				(vm_size_t)sg->vmsize, FALSE, sg->maxprot)) !=
				KERN_SUCCESS){
				mach_error(r, "can't set vm_protection on "
				    "segment: %.16s for object: %s",
				    sg->segname, p->images[i].image.name);
				link_edit_error(DYLD_MACH_RESOURCE, r,
				    p->images[i].image.name);
			    }
			    break;
			}
			lc = (struct load_command *)((char *)lc + lc->cmdsize);
		    }
		}

		/* undo the prebinding of the lazy symbols pointers */
		undo_prebound_lazy_pointers(
		    &(p->images[i].image),
#if defined(m68k) || defined(__i386__)
		    GENERIC_RELOC_PB_LA_PTR);
#endif
#ifdef hppa
		    HPPA_RELOC_PB_LA_PTR);
#endif
#ifdef sparc
		    SPARC_RELOC_PB_LA_PTR);
#endif
#ifdef __ppc__
		    PPC_RELOC_PB_LA_PTR);
#endif
		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment != NULL && st != NULL && dyst != NULL){
		    relocs = (struct relocation_info *)
			(p->images[i].image.vmaddr_slide +
			 linkedit_segment->vmaddr +
			 dyst->extreloff -
			 linkedit_segment->fileoff);
		    symbols = (struct nlist *)
			(p->images[i].image.vmaddr_slide +
			 linkedit_segment->vmaddr +
			 st->symoff -
			 linkedit_segment->fileoff);
		    strings = (char *)
			(p->images[i].image.vmaddr_slide +
			 linkedit_segment->vmaddr +
			 st->stroff -
			 linkedit_segment->fileoff);
		    /* undo the prebinding of the external relocation */
		    undo_external_relocation(
			TRUE, /* undo_prebinding */
			&(p->images[i].image),
			relocs,
			dyst->nextrel,
			symbols,
			strings,
			NULL, /* library_name */
			p->images[i].image.name);
		}

		/*
		 * If the image has relocations in read-only segments and the
		 * protection was changed change it back.
		 */
		if(p->images[i].image.change_protect_on_reloc){
		    lc = (struct load_command *)((char *)p->images[i].image.mh +
			    sizeof(struct mach_header));
		    for(j = 0; j < p->images[i].image.mh->ncmds; j++){
			switch(lc->cmd){
			case LC_SEGMENT:
			    sg = (struct segment_command *)lc;
			    if((r = vm_protect(mach_task_self(), sg->vmaddr +
				p->images[i].image.vmaddr_slide,
				(vm_size_t)sg->vmsize, FALSE,
				sg->initprot)) != KERN_SUCCESS){
				mach_error(r, "can't set vm_protection on "
				    "segment: %.16s for object: %s",
				    sg->segname, p->images[i].image.name);
				link_edit_error(DYLD_MACH_RESOURCE, r,
				    p->images[i].image.name);
			    }
			    break;
			}
			lc = (struct load_command *)((char *)lc + lc->cmdsize);
		    }
		}
	    }
	    p = p->next_images;
	}while(p != NULL);

	/*
	 * Second undo the prebinding for the library images.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		/* if this image was not prebound skip it */
		if((q->images[i].image.mh->flags & MH_PREBOUND) != MH_PREBOUND)
		    continue;
		undo_prebinding_for_library(q->images + i);
	    }
	    q = q->next_images;
	}while(q != NULL);
}

/*
 * undo_prebinding_for_library() undoes the prebinding for the specified
 * library.  We make sure the lazy pointers are reset and then just set the
 * module state to PREBOUND_UNLINKED.
 */
static
void
undo_prebinding_for_library(
struct library_image *library_image)
{
    unsigned long j;

	/*
	 * Undo the prebinding of the lazy symbols pointers if the
	 * library as not been slid.  If it has been slid then this
	 * would have been done in local_relocation().
	 */
	if(library_image->image.vmaddr_slide == 0)
	    undo_prebound_lazy_pointers(
		&(library_image->image),
#if defined(m68k) || defined(__i386__)
		GENERIC_RELOC_PB_LA_PTR);
#endif
#ifdef hppa
		HPPA_RELOC_PB_LA_PTR);
#endif
#ifdef sparc
		SPARC_RELOC_PB_LA_PTR);
#endif
#ifdef __ppc__
		PPC_RELOC_PB_LA_PTR);
#endif

	for(j = 0; j < library_image->nmodules; j++)
	    library_image->modules[j] = PREBOUND_UNLINKED;

}

/*
 * try_to_use_prebound_libraries() is called when the libraries are setup for
 * prebinding but the executable is not.  If if is successfull prebinding is
 * left set to TRUE if not prebinding gets set to FALSE.
 */
void
try_to_use_prebound_libraries(
void)
{
	/*
	 * Check to see this executable does not define any symbols defined and
	 * referenced in the libraries it uses.
	 */
	if(check_executable_for_overrides() == FALSE)
	    return;

	/*
	 * Now put all undefined symbols from the executable on the undefined
	 * list.
	 */
	setup_initial_undefined_list(TRUE);

	/*
	 * Now resolve all symbol references this program has to see it there
	 * will be any undefined symbols and to mark which libraries modules
	 * will be linked.
	 */
	if(resolve_undefineds(TRUE, TRUE) == FALSE){
	    /* a multiply defined error occured */
	    failed_use_prebound_libraries();
	    return;
	}

	/*
	 * If there are undefineds then this try failed.
	 */
	if(undefined_list.next != &undefined_list){
	    if(dyld_prebind_debug != 0 && prebinding == TRUE)
		print("dyld: %s: trying to use prebound libraries failed due "
		       "to undefined symbols\n", executables_name);
	    prebinding = FALSE;
	    failed_use_prebound_libraries();
	    return;
	}

	/*
	 * Now do the relocation of just the executable module and mark the
	 * library modules that were being linked as FULLY_LINKED and the other
	 * library modules as PREBOUND_UNLINKED.
	 */
	relocate_modules_being_linked(TRUE);
	object_images.images[0].module = FULLY_LINKED;

	/*
	 * This just causes all images to be marked as the register funcs (which
	 * there are none at launch time) are called.
	 */
	call_registered_funcs_for_add_images();
}

/*
 * failed_use_prebound_libraries() is called when the try to use prebound
 * libraries failed and things need to be cleaned up.  This clears up the
 * undefined and being linked lists.  Resets all the module_states to unlinked.
 */
static
void
failed_use_prebound_libraries(
void)
{
	/* clear undefined list */
	clear_undefined_list();

	/* clear being linked list */
	clear_being_linked_list();
    
	/* reset all the module_states to unlinked */
	reset_module_states();
}

/*
 * reset_module_states() is used when prebinding fails and all the module states
 * need to be set back to UNKINKED.
 */
static
void
reset_module_states(
void)
{
    unsigned long i, j;
    struct library_images *q;

	object_images.images[0].module = BEING_LINKED;

	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		for(j = 0; j < q->images[i].nmodules; j++)
		    q->images[i].modules[j] = UNLINKED;
	    }
	    q = q->next_images;
	}while(q != NULL);
}
