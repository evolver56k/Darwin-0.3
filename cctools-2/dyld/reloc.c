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
#import <stdlib.h>
#import <limits.h>
#import <string.h>
#import <mach/mach.h>
#import "stuff/openstep_mach.h"
#import <mach-o/loader.h>
#import <mach-o/nlist.h>
#import <mach-o/reloc.h>
#ifdef hppa
#import <mach-o/hppa/reloc.h>
#endif
#ifdef sparc
#import <mach-o/sparc/reloc.h>
#endif
#import <mach-o/dyld_debug.h>

#import "stuff/vm_flush_cache.h"

#import "images.h"
#import "symbols.h"
#import "errors.h"
#import "reloc.h"
#import "debug.h"
#import "register_funcs.h"

/*
 * relocate_modules_being_linked() preforms the external relocation for the
 * modules being linked and sets the values of the symbol pointers for the
 * symbols being linked.  When trying to launch with prebound libraries then
 * launching_with_prebound_libraries is TRUE.  Then only the executable gets
 * relocated and all library modules in the BEING_LINKED state get set to the
 * FULLY_LINKED state and the others get set to PREBOUND_UNLINKED state.
 */
void
relocate_modules_being_linked(
enum bool launching_with_prebound_libraries)
{
    unsigned long i, j;
    enum link_state link_state;
    struct object_images *p;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct dylib_module *dylib_modules;
    struct relocation_info *relocs;
    struct nlist *symbols;
    char *strings;
    struct load_command *lc;
    struct segment_command *sg;
    kern_return_t r;
    struct dyld_event event;

	memset(&event, '\0', sizeof(struct dyld_event));
	event.type = DYLD_MODULE_BOUND;
	/*
	 * First relocate object_images modules that are being linked.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;
		relocate_symbol_pointers_in_object_image(&(p->images[i].image));

		/* skip modules that that not in the being linked state */
		if(link_state != BEING_LINKED)
		    continue;

		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment == NULL || st == NULL || dyst == NULL)
		    continue;
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

		p->images[i].module = 
		    external_relocation(
			&(p->images[i].image),
			relocs,
			dyst->nextrel,
			symbols,
			strings,
			NULL, /* library_name */
			p->images[i].image.name);

		if(launching_with_prebound_libraries == TRUE)
		    p->images[i].module = FULLY_LINKED;

		/* send the event message that this module was bound */
		event.arg[0].header = p->images[i].image.mh;
		event.arg[0].vmaddr_slide = p->images[i].image.vmaddr_slide;
		event.arg[0].module_index = 0;
		send_event(&event);

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
	 * Next relocate the library_images.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		if(launching_with_prebound_libraries == FALSE)
		    relocate_symbol_pointers_in_library_image(
			&(q->images[i].image));

		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;
		relocs = (struct relocation_info *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->extreloff -
		     linkedit_segment->fileoff);
		symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		strings = (char *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		dylib_modules = (struct dylib_module *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->modtaboff -
		     linkedit_segment->fileoff);
		/*
		 * If the image has relocations in read-only segments and the
		 * protection needs to change change it.
		 */
		if(q->images[i].image.change_protect_on_reloc){
		    lc = (struct load_command *)((char *)q->images[i].image.mh +
			    sizeof(struct mach_header));
		    for(j = 0; j < q->images[i].image.mh->ncmds; j++){
			switch(lc->cmd){
			case LC_SEGMENT:
			    sg = (struct segment_command *)lc;
			    if((r = vm_protect(mach_task_self(), sg->vmaddr +
				q->images[i].image.vmaddr_slide,
				(vm_size_t)sg->vmsize, FALSE,
				VM_PROT_READ | VM_PROT_WRITE |
				VM_PROT_EXECUTE)) != KERN_SUCCESS){
				mach_error(r, "can't set vm_protection on "
				    "segment: %.16s for library: %s",
				    sg->segname, q->images[i].image.name);
				link_edit_error(DYLD_MACH_RESOURCE, r,
				    q->images[i].image.name);
			    }
			    break;
			}
			lc = (struct load_command *)((char *)lc + lc->cmdsize);
		    }
		}
		for(j = 0; j < dyst->nmodtab; j++){

		    /* skip modules that that not in the being linked state */
		    link_state = q->images[i].modules[j];
		    if(link_state != BEING_LINKED){
			if(launching_with_prebound_libraries == TRUE)
			    q->images[i].modules[j] = PREBOUND_UNLINKED;
			continue;
		    }

		    if(launching_with_prebound_libraries == FALSE)
			q->images[i].modules[j] = 
			    external_relocation(
				&(q->images[i].image),
				relocs + dylib_modules[j].iextrel,
				dylib_modules[j].nextrel,
				symbols,
				strings,
				q->images[i].image.name,
				strings + dylib_modules[j].module_name);
		    else
			q->images[i].modules[j] = FULLY_LINKED;

		    /* send the event message that this module was bound */
		    event.arg[0].header = q->images[i].image.mh;
		    event.arg[0].vmaddr_slide = q->images[i].image.vmaddr_slide;
		    event.arg[0].module_index = j;
		    send_event(&event);
		}
		/*
		 * If the image has relocations in read-only segments and the
		 * protection was changed change it back.
		 */
		if(q->images[i].image.change_protect_on_reloc){
		    lc = (struct load_command *)((char *)q->images[i].image.mh +
			    sizeof(struct mach_header));
		    for(j = 0; j < q->images[i].image.mh->ncmds; j++){
			switch(lc->cmd){
			case LC_SEGMENT:
			    sg = (struct segment_command *)lc;
			    if((r = vm_protect(mach_task_self(), sg->vmaddr +
				q->images[i].image.vmaddr_slide,
				(vm_size_t)sg->vmsize, FALSE,
				sg->initprot)) != KERN_SUCCESS){
				mach_error(r, "can't set vm_protection on "
				    "segment: %.16s for library: %s",
				    sg->segname, q->images[i].image.name);
				link_edit_error(DYLD_MACH_RESOURCE, r,
				    q->images[i].image.name);
			    }
			    break;
			}
			lc = (struct load_command *)((char *)lc + lc->cmdsize);
		    }
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	clear_being_linked_list();
}

/*
 * undo_prebound_lazy_pointers() undoes the prebinding for lazy symbol pointers.
 * This is also done by local_relocation() which is called if the image can't
 * be loaded at the addresses it wants in the file.  This is called when the
 * prebinding can't be used and the image was loaded at the address it wants.
 */
void
undo_prebound_lazy_pointers(
struct image *image,
unsigned long PB_LA_PTR_r_type)
{
    unsigned long i, r_slide, r_address, r_type, r_value, value;
    struct relocation_info *relocs;
    struct scattered_relocation_info *sreloc;
    unsigned long cache_flush_high_addr, cache_flush_low_addr;

	relocs = (struct relocation_info *)
	    (image->vmaddr_slide +
	     image->linkedit_segment->vmaddr +
	     image->dyst->locreloff -
	     image->linkedit_segment->fileoff);

	cache_flush_high_addr = 0;
	cache_flush_low_addr = ULONG_MAX;
	r_slide = image->seg1addr + image->vmaddr_slide;
	r_value = 0;

	for(i = 0; i < image->dyst->nlocrel; i++){
	    if((relocs[i].r_address & R_SCATTERED) != 0){
		sreloc = (struct scattered_relocation_info *)(relocs + i);
		r_address = sreloc->r_address;
		r_type = sreloc->r_type;
		r_value = sreloc->r_value;

		/*
		 * If the relocation entry is for a prebound lazy pointer
		 * (r_type is the passed in value PB_LA_PTR_r_type) then undo
		 * the prebinding by using the value of the lazy pointer saved
		 * in r_value plus the slide amount.
		 */
		if(r_type == PB_LA_PTR_r_type){
		    value = r_value + image->vmaddr_slide;
		    *((long *)(r_address + r_slide)) = value;

		    if(image->cache_sync_on_reloc){
			if(r_address + r_slide < cache_flush_low_addr)
			    cache_flush_low_addr = r_address + r_slide;
			if(r_address + r_slide + (1 << 2) >
			    cache_flush_high_addr)
			    cache_flush_high_addr =
				r_address + r_slide + (1 << 2);
		    }
		}
	    }
#if defined(hppa) || defined(sparc)
	    else{
		r_type = relocs[i].r_type;
	    }
	    /*
	     * If the relocation entry had a pair step over it.
	     */
#ifdef hppa
	    if(r_type == HPPA_RELOC_HI21 ||
	       r_type == HPPA_RELOC_LO14 ||
	       r_type == HPPA_RELOC_BR17)
#endif
#ifdef sparc
	    if(r_type == SPARC_RELOC_HI22 ||
	       r_type == SPARC_RELOC_LO10 )
#endif
		i++;
#endif /* defined(hppa) || defined(sparc) */

	}

	if(image->cache_sync_on_reloc &&
	   cache_flush_high_addr > cache_flush_low_addr)
	    vm_flush_cache(mach_task_self(), cache_flush_low_addr,
			   cache_flush_high_addr - cache_flush_low_addr);
}
