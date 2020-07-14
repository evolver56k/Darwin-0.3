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
#include <stdarg.h>
#include <string.h>
#include <mach/mach.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <time.h>
#include "stuff/bool.h"
#include "stuff/bytesex.h"

#include "dylibs.h"
#include "ld.h"
#include "objects.h"
#include "pass1.h"
#include "sections.h"

__private_extern__ enum bool has_dynamic_linker_command = FALSE;

#ifndef RLD

/* the pointer to the head of the dynamicly linked shared library commands */
__private_extern__ struct merged_dylib *merged_dylibs = NULL;

/* the pointer to the merged the dynamic linker command if any */
__private_extern__ struct merged_dylinker *merged_dylinker = NULL;

/* the pointer to the head of the dynamicly linked shared library segments */
__private_extern__ struct merged_segment *dylib_segments = NULL;

static struct merged_dylib *lookup_merged_dylib(
    struct dylib_command *dl);

/*
 * create_dylib_id_command() creates the LC_ID_DYLIB load command from the
 * command line argument values.  It is called from layout() when the output
 * filetype is MH_DYLIB.
 */
__private_extern__
void
create_dylib_id_command(
void)
{
    char *name;
    unsigned long cmdsize;
    struct dylib_command *dl;
    struct merged_dylib *mdl;

	if(dylib_install_name != NULL)
	    name = dylib_install_name;
	else
	    name = outputfile;

	cmdsize = sizeof(struct dylib_command) +
		  round(strlen(name) + 1, sizeof(long));
	dl = allocate(cmdsize);
	memset(dl, '\0', cmdsize);
	dl->cmd = LC_ID_DYLIB;
	dl->cmdsize = cmdsize;
	dl->dylib.name.offset = sizeof(struct dylib_command);
	dl->dylib.timestamp = time(0);
	dl->dylib.current_version = dylib_current_version;
	dl->dylib.compatibility_version = dylib_compatibility_version;
	strcpy((char *)dl + sizeof(struct dylib_command), name);

	mdl = allocate(sizeof(struct merged_dylib));
	memset(mdl, '\0', sizeof(struct merged_dylib));
	mdl->dylib_name = name;
	mdl->dl = dl;
	mdl->output_id = TRUE;
	mdl->next = merged_dylibs;
	merged_dylibs = mdl;
}

/*
 * merge_dylibs() merges in the dylib commands from the current object.
 */
__private_extern__
void
merge_dylibs(void)
{
    unsigned long i;
    struct mach_header *mh;
    struct load_command *lc;
    struct dylib_command *dl;
    struct dylinker_command *dyld;
    char *dyld_name;

	/*
	 * Process all the load commands for the dynamic shared libraries.
	 */
	mh = (struct mach_header *)cur_obj->obj_addr;
	lc = (struct load_command *)((char *)cur_obj->obj_addr +
				     sizeof(struct mach_header));
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmd == LC_LOAD_DYLIB || lc->cmd == LC_ID_DYLIB){
		/*
		 * Do not record dynamic libraries dependencies in the output
		 * file.  Only record the library itself.
		 */
		if(lc->cmd != LC_LOAD_DYLIB || mh->filetype != MH_DYLIB){
		    dl = (struct dylib_command *)lc;
		    (void)lookup_merged_dylib(dl);
		    (void)add_dynamic_lib(DYLIB, dl, cur_obj);
		}
	    }
	    else if(lc->cmd == LC_LOAD_DYLINKER || lc->cmd == LC_ID_DYLINKER){
		dyld = (struct dylinker_command *)lc;
		dyld_name = (char *)dyld + dyld->name.offset;
		if(merged_dylinker == NULL){
		    merged_dylinker = allocate(sizeof(struct merged_dylinker));
		    memset(merged_dylinker, '\0',
			   sizeof(struct merged_dylinker));
		    merged_dylinker->dylinker_name = dyld_name;
		    merged_dylinker->definition_object = cur_obj;
		    merged_dylinker->dyld = dyld;
		    has_dynamic_linker_command = TRUE;
		    if(save_reloc == FALSE)
			output_for_dyld = TRUE;
		}
		else if(strcmp(dyld_name, merged_dylinker->dylinker_name) != 0){
		    error("multiple dynamic linkers loaded (only one allowed)");
		    print_obj_name(merged_dylinker->definition_object);
		    print("loads dynamic linker %s\n",
			  merged_dylinker->dylinker_name);
		    print_obj_name(cur_obj);
		    print("loads dynamic linker %s\n", dyld_name);
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * lookup_merged_dylib() adds a LC_LOAD_DYLIB command to it to the merged list
 * of dynamic shared libraries for the load command passed to it.  It ignores
 * the command if it see the same library twice.
 */
static
struct merged_dylib *
lookup_merged_dylib(
struct dylib_command *dl)
{
    char *dylib_name;
    struct merged_dylib **p, *mdl;

	dylib_name = (char *)dl + dl->dylib.name.offset;
	p = &merged_dylibs;
	while(*p){
	    mdl = *p;
	    if(strcmp(mdl->dylib_name, dylib_name) == 0){
		if(mdl->dl->cmd == LC_ID_DYLIB){
		    /*
		     * If the new one is also a LC_ID_DYLIB use the one with the
		     * highest compatiblity number.  Else if the new one is just
		     * an LC_LOAD_DYLIB ignore it and use the merged one that is
		     * a LC_ID_DYLIB.
		     */
		    if(dl->cmd == LC_ID_DYLIB){
		       if(dl->dylib.compatibility_version >
			  mdl->dl->dylib.compatibility_version){
			    if(strcmp(mdl->definition_object->file_name,
				      cur_obj->file_name) != 0)
				warning("multiple references to dynamic shared "
				    "library: %s (from %s and %s, using %s "
				    "which has higher compatibility_version)",
				    dylib_name,
				    mdl->definition_object->file_name,
				    cur_obj->file_name, cur_obj->file_name);
			    mdl->dylib_name = dylib_name;
			    mdl->dl = dl;
			    mdl->definition_object = cur_obj;
			}
		    }
		}
		else{
		    if(dl->cmd == LC_ID_DYLIB){
			mdl->dylib_name = dylib_name;
			mdl->dl = dl;
			mdl->definition_object = cur_obj;
		    }
		}
		return(mdl);
	    }
	    p = &(mdl->next);
	}
	*p = allocate(sizeof(struct merged_dylib));
	memset(*p, '\0', sizeof(struct merged_dylib));
	mdl = *p;
	mdl->dylib_name = dylib_name;
	mdl->dl = dl;
	mdl->definition_object = cur_obj;
	mdl->output_id = FALSE;
	return(mdl);
}

/*
 * create_dylinker_id_command() creates the LC_ID_DYLINKER load command from the
 * command line argument values.  It is called from layout() when the output
 * filetype is MH_DYLINKER.
 */
__private_extern__
void
create_dylinker_id_command(
void)
{
    char *name;
    unsigned long cmdsize;
    struct dylinker_command *dyld;
    struct merged_dylinker *mdyld;

	if(dylinker_install_name != NULL)
	    name = dylinker_install_name;
	else
	    name = outputfile;

	cmdsize = sizeof(struct dylinker_command) +
		  round(strlen(name) + 1, sizeof(long));
	dyld = allocate(cmdsize);
	memset(dyld, '\0', cmdsize);
	dyld->cmd = LC_ID_DYLINKER;
	dyld->cmdsize = cmdsize;
	dyld->name.offset = sizeof(struct dylinker_command);
	strcpy((char *)dyld + sizeof(struct dylinker_command), name);

	mdyld = allocate(sizeof(struct merged_dylinker));
	memset(mdyld, '\0', sizeof(struct merged_dylinker));
	mdyld->dylinker_name = name;
	mdyld->dyld = dyld;
	merged_dylinker = mdyld;
}

/*
 * add_dylib_segment() adds the specified segment to the list of
 * dylib_segments as comming from the specified dylib_name.
 */
__private_extern__
void
add_dylib_segment(
struct segment_command *sg,
char *dylib_name)
{
    struct merged_segment **p, *msg;

	p = &dylib_segments;
	while(*p){
	    msg = *p;
	    p = &(msg->next);
	}
	*p = allocate(sizeof(struct merged_segment));
	msg = *p;
	memset(msg, '\0', sizeof(struct merged_segment));
	msg->sg = *sg;
	msg->filename = dylib_name;
}
#endif /* !defined(RLD) */
