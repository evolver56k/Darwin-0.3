/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach-o/loader.h>
#include <objc/objc-runtime.h>
#include <objc/NXString.h>
#include <sys/types.h>
#include "Protocol.h"

/* These are in libc in later updates */
void
swap_mach_header(
struct mach_header *mh,
enum NXByteOrder target_byte_order)
{
	mh->magic = NXSwapLong(mh->magic);
	mh->cputype = NXSwapLong(mh->cputype);
	mh->cpusubtype = NXSwapLong(mh->cpusubtype);
	mh->filetype = NXSwapLong(mh->filetype);
	mh->ncmds = NXSwapLong(mh->ncmds);
	mh->sizeofcmds = NXSwapLong(mh->sizeofcmds);
	mh->flags = NXSwapLong(mh->flags);
}

void
swap_load_command(
struct load_command *lc,
enum NXByteOrder target_byte_order)
{
	lc->cmd = NXSwapLong(lc->cmd);
	lc->cmdsize = NXSwapLong(lc->cmdsize);
}

void
swap_segment_command(
struct segment_command *sg,
enum NXByteOrder target_byte_order)
{
	/* segname[16] */
	sg->cmd = NXSwapLong(sg->cmd);
	sg->cmdsize = NXSwapLong(sg->cmdsize);
	sg->vmaddr = NXSwapLong(sg->vmaddr);
	sg->vmsize = NXSwapLong(sg->vmsize);
	sg->fileoff = NXSwapLong(sg->fileoff);
	sg->filesize = NXSwapLong(sg->filesize);
	sg->maxprot = NXSwapLong(sg->maxprot);
	sg->initprot = NXSwapLong(sg->initprot);
	sg->nsects = NXSwapLong(sg->nsects);
	sg->flags = NXSwapLong(sg->flags);
}

void
swap_section(
struct section *s,
unsigned long nsects,
enum NXByteOrder target_byte_order)
{
    unsigned long i;

	for(i = 0; i < nsects; i++){
	    /* sectname[16] */
	    /* segname[16] */
	    s[i].addr = NXSwapLong(s[i].addr);
	    s[i].size = NXSwapLong(s[i].size);
	    s[i].offset = NXSwapLong(s[i].offset);
	    s[i].align = NXSwapLong(s[i].align);
	    s[i].reloff = NXSwapLong(s[i].reloff);
	    s[i].nreloc = NXSwapLong(s[i].nreloc);
	    s[i].flags = NXSwapLong(s[i].flags);
	    s[i].reserved1 = NXSwapLong(s[i].reserved1);
	    s[i].reserved2 = NXSwapLong(s[i].reserved2);
	}
}

void
swap_symtab_command(
struct symtab_command *st,
enum NXByteOrder target_byte_order)
{
	st->cmd = NXSwapLong(st->cmd);
	st->cmdsize = NXSwapLong(st->cmdsize);
	st->symoff = NXSwapLong(st->symoff);
	st->nsyms = NXSwapLong(st->nsyms);
	st->stroff = NXSwapLong(st->stroff);
	st->strsize = NXSwapLong(st->strsize);
}

void
swap_symseg_command(
struct symseg_command *ss,
enum NXByteOrder target_byte_order)
{
	ss->cmd = NXSwapLong(ss->cmd);
	ss->cmdsize = NXSwapLong(ss->cmdsize);
	ss->offset = NXSwapLong(ss->offset);
	ss->size = NXSwapLong(ss->size);
}

/*
 * For system call errors the error messages allways contains
 * sys_errlist[errno] as part of the message.
 */
extern char *sys_errlist[];
extern int errno;

/* Name of this program for error messages (argv[0]) */
char *progname;

extern void _sel_writeHashTable(
	int start_addr, 
	char *myselectorstraddr, 
	char *shlibselectorstraddr,
	void **stuff, 
	int *stuffsize
);

/*
 * Declarations for the static routines in this file.
 */
static void usage();

static Module get_objc(
	long fd, 
	char *filename, 
	struct section *objcsects, 
	int nsects
);
static void print_method_list(
	struct objc_method_list *mlist_before_reloc, 
	struct section *firstobjcsect, 
	int nsects
);
static void print_method_list2(
	struct objc_method_description_list *mlist_before_reloc, 
	struct section *firstobjcsect, 
	int nsects
);
static void getObjcSections(
	struct mach_header *mh,
	struct load_command *lc, 
	struct section **objcsects, 
	int *nsects
);
static void readObjcData(
	long fd,
	char *filename,
    	struct section *objcsects, 
	int nsects
);
static void swapObjcData(
    	struct section *objcsects, 
	int nsects
);
static void swap_objc_modules(
	struct objc_module *modules,
	unsigned long size
);
static void swap_objc_symtabs(
	struct objc_symtab *symtabs,
	unsigned long size
);
static void swap_objc_classes(
	struct objc_class *classes,
	unsigned long size
);
static void swap_objc_categories(
	struct objc_category *categories,
	unsigned long size
);
static void swap_objc_method_lists(
	struct objc_method_list *method_lists,
	unsigned long size
);
static void swap_objc_protocols(
	Protocol *protocols,
	unsigned long size
);
static void swap_objc_ivar_lists(
	struct objc_ivar_list *ivar_lists,
	unsigned long size
);
static void swap_objc_protocol_lists(
	struct objc_protocol_list *protocol_lists,
	unsigned long size
);
static void swap_objc_refs(
	unsigned long *refs,
	unsigned long size
);
static void swap_string_objects(
	NXConstantString *s,
	unsigned long size
);
static void swap_objc_method_description_lists(
	struct objc_method_description_list *mdls,
	unsigned long size
);
struct _hashEntry {
    struct _hashEntry *next;
    char *sel;
};
static void swap_hashEntries(
	struct _hashEntry *_hashEntries,
	unsigned long size
);
static void *getObjcData(
    	struct section *objcsects, 
	int nsects,
	const void *addr
);
static struct section *getObjcSection(
    	struct section *objcsects, 
	int nsects,
	char *name	
);

int swapped = 0;

void
main(argc, argv)
int argc;
char *argv[];
{
	long fd;
	char *filename;
	struct mach_header mh;
	struct load_command *lcp;

	progname = argv[0];

	if(argc == 1) {
	    fprintf(stderr, "%s: At least one file must be specified\n",
		    progname);
	    usage();
	    exit(1);
	}
	else if (argc == 2)
	  filename = argv[1];
	else
	  {
	    usage();
	    exit(1);
	  }
	
	fd = open(filename, O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "%s : Can't open %s (%s)\n", progname,
			filename, sys_errlist[errno]);
	}
	lseek(fd, 0, 0);
	if(read(fd, (char *)&mh, sizeof(mh)) != sizeof(mh)){
	    fprintf(stderr, "%s : Can't read mach header of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    return;
	}
	if(mh.magic == MH_MAGIC){
		if((lcp = (struct load_command *)malloc(mh.sizeofcmds))
		    == (struct load_command *)0){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n",
			    progname, sys_errlist[errno]);
		    exit(1);
		}
		if(read(fd, (char *)lcp, mh.sizeofcmds) != mh.sizeofcmds){
		    fprintf(stderr,"%s : Can't read load commands of %s (%s)\n",
			       progname, filename, sys_errlist[errno]);
		    lcp = (struct load_command *)0;
		}
	}
	else if(mh.magic == NXSwapLong(MH_MAGIC)){
		if((lcp = (struct load_command *)
				malloc(NXSwapLong(mh.sizeofcmds)))
		    		== (struct load_command *)0){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n",
			    progname, sys_errlist[errno]);
		    exit(1);
		}
		if(read(fd, (char *)lcp, NXSwapLong(mh.sizeofcmds)) !=
		   NXSwapLong(mh.sizeofcmds)){
		    fprintf(stderr,"%s : Can't read load commands of %s (%s)\n",
			       progname, filename, sys_errlist[errno]);
		    lcp = (struct load_command *)0;
		}
	}

	{
	struct section *firstobjcsect;
	int nsects, size;
	long vm_hashaddr;
	void *table; 
	struct section *sel, *tmp;
	char *selectors, tempfilename[] = "objcoptXXXXXX";

	getObjcSections(&mh, lcp, &firstobjcsect, &nsects);
	get_objc(fd, filename, firstobjcsect, nsects);

	if (strcmp(firstobjcsect[nsects-1].sectname, "__runtime_setup") == 0)
		// we are doing a `replace'
		vm_hashaddr = firstobjcsect[nsects-1].addr;
	else  
		vm_hashaddr = round(firstobjcsect[nsects-1].addr + 
			    	firstobjcsect[nsects-1].size, sizeof(long));

	sel = getObjcSection(firstobjcsect, nsects, "__meth_var_names");
	
	tmp = getObjcSection(firstobjcsect, nsects, "__selector_strs");
	
	if (sel && tmp)
	  {
	    fprintf (stderr,
		     "Cannot objcopt mixture of new and old strings\n");
	    exit (1);
	  }
	
	if (!sel)
	  sel = tmp;	// Use old-style strings
	
	selectors = (char *)sel->reserved1;

	_sel_writeHashTable(vm_hashaddr, (char *)selectors, (char *)sel->addr,
				 &table, &size);
	if(swapped)
	    swap_hashEntries((struct _hashEntry *)table, size);

	mktemp(tempfilename);
	add_objc_runtime_setup(filename,tempfilename,table, size);

	close(fd);
	unlink(filename);
	if (rename(tempfilename, filename) == -1) {
		fprintf(stderr, "%s : Cannot rename temporary file from: "
				"%s to %s\n", progname, tempfilename, filename);
		exit(1);
	}
	}

	exit(0);
}

static void getObjcSections(
	struct mach_header *mh,
	struct load_command *lc, 
	struct section **objcsects, 
	int *nsects
)
{
	int i,j;
    	struct segment_command *sg;
	struct section *s;
    	struct load_command l, *initlc;

	swapped = mh->magic == NXSwapLong(MH_MAGIC);
	if(swapped)
	    swap_mach_header(mh, NXHostByteOrder());
	initlc = lc;

	for(i = 0 ; i < mh->ncmds; i++){
	    l = *lc;
	    if(swapped)
		swap_load_command(&l, NXHostByteOrder());
	    if(l.cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + l.cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(l.cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(swapped)
		    swap_segment_command(sg, NXHostByteOrder());
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, SEG_OBJC) == 0){

		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));

		    *objcsects = s;
		    *nsects = sg->nsects;

		    for(j = 0 ; j < sg->nsects ; j++){
			if(swapped)
			    swap_section(s, 1, NXHostByteOrder());
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds){
			    printf("section structure command extends past end "
				   "of load commands\n");
			}
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds)
			    break;
			s++;
		    }
		}
	    }
	    if(l.cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + l.cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		break;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

static void readObjcData(
	long fd,
	char *filename,
    	struct section *objcsects, 
	int nsects
)
{
	int i;

	for (i = 0; i < nsects; i++) {

		struct section *s;

		s = objcsects + i;

		if (s->size > 0) {
	    	  if((s->reserved1 = (long)malloc(s->size)) == 0) {
		    fprintf(stderr, "%s : Ran out of memory (%s)\n", 
			progname, sys_errlist[errno]);
		    exit(1);
	          }
	          lseek(fd, s->offset, 0);
	          if((read(fd, s->reserved1, s->size)) != s->size){
		    fprintf(stderr, "%s : Can't read modules of %s (%s)\n",
		    	progname, filename, sys_errlist[errno]);
		    free((void *)s->reserved1);
		    return;
	          }
		}
	}
}

struct objc_protocol
{
    @defs(Protocol)
};

struct objc_string_object
{
    @defs(NXConstantString)
};

static void swapObjcData(
    	struct section *objcsects, 
	int nsects
)
{
	int i, categories_size = 0, protocols_size = 0;
	struct objc_category *categories = NULL;
	struct objc_protocol *protocols = NULL;
	struct section *s, *cat_inst = NULL, *cat_cls = NULL;
	struct objc_method_list *method_list;
	struct objc_protocol_list *protocol_list;
	struct objc_method_description_list *mdl;

	for (i = 0; i < nsects; i++) {

		s = objcsects + i;
		if(strncmp(s->sectname, "__class", 16) == 0 ||
		   strncmp(s->sectname, "__meta_class", 16) == 0){
		    swap_objc_classes((struct objc_class *)s->reserved1,
				      s->size);
		}
		else if(strncmp(s->sectname, "__string_object", 16) == 0){
		    swap_string_objects((NXConstantString *)s->reserved1,
				        s->size);
		}
		else if(strncmp(s->sectname, "__protocol", 16) == 0){
		    protocols = (struct objc_protocol *)s->reserved1;
		    protocols_size = s->size;
		    swap_objc_protocols((Protocol *)s->reserved1,
				        s->size);
		}
		else if(strncmp(s->sectname, "__cat_cls_meth", 16) == 0){
		    /* handled special below */
		    cat_cls = s;
		}
		else if(strncmp(s->sectname, "__cat_inst_meth", 16) == 0){
		    /* handled special below */
		    cat_inst = s;
		}
		else if(strncmp(s->sectname, "__cls_meth", 16) == 0 ||
		        strncmp(s->sectname, "__inst_meth", 16) == 0){
		    swap_objc_method_lists((struct objc_method_list *)
					   s->reserved1, s->size);
		}
		else if(strncmp(s->sectname, "__message_refs", 16) == 0 ||
		        strncmp(s->sectname, "__selector_refs", 16) == 0 ||
			strncmp(s->sectname, "__cls_refs", 16) == 0){
		    swap_objc_refs((unsigned long *)s->reserved1, s->size);
		}
		else if(strncmp(s->sectname, "__class_names", 16) == 0 ||
			strncmp(s->sectname, "__meth_var_names", 16) == 0 ||
			strncmp(s->sectname, "__meth_var_types", 16) == 0 ||
		        strncmp(s->sectname, "__selector_strs", 16) == 0){
			; /* character strings, no swapping */
		}
		else if(strncmp(s->sectname, "__module_info", 16) == 0){
		    swap_objc_modules((struct objc_module *)s->reserved1,
				      s->size);
		}
		else if(strncmp(s->sectname, "__symbols", 16) == 0){
		    swap_objc_symtabs((struct objc_symtab *)s->reserved1,
				      s->size);
		}
		else if(strncmp(s->sectname, "__category", 16) == 0){
		    categories = (struct objc_category *)s->reserved1;
		    categories_size = s->size;
		    swap_objc_categories((struct objc_category *)s->reserved1,
				      s->size);
		}
		else if(strncmp(s->sectname, "__class_vars", 16) == 0 ||
			strncmp(s->sectname, "__instance_vars", 16) == 0){
		    swap_objc_ivar_lists((struct objc_ivar_list *)s->reserved1,
				         s->size);
		}
		else if(strncmp(s->sectname, "__runtime_setup", 16) == 0){
		    ; /* do nothing it will be tossed */
		}
                else if(strncmp(s->sectname, "__cstring_object", 16)) {
                   ; /* do nothing */
                }
                else if(strncmp(s->sectname, "__sel_fixup", 16)) {
                   ; /* do nothing */
                }
		else{
		    fprintf(stderr, "%s : unknown __OBJC section: %s\n",
			    progname, s->sectname);
		    exit(1);
		}
	}

	for (i = 0; i < categories_size; i += sizeof(struct objc_category)){
	    if(categories->instance_methods != 0){
		method_list = (struct objc_method_list *)
				(cat_inst->reserved1 + 
				((unsigned long)categories->instance_methods -
				(unsigned long)cat_inst->addr));
		swap_objc_method_lists(method_list,
		    sizeof(struct objc_method_list) -
			   sizeof(struct objc_method));
	    }

	    if(categories->class_methods != 0){
		method_list = (struct objc_method_list *)
				(cat_cls->reserved1 + 
				((unsigned long)categories->class_methods -
				(unsigned long)cat_cls->addr));
		swap_objc_method_lists(method_list,
		    sizeof(struct objc_method_list) -
		    sizeof(struct objc_method));
	    }

	    if(categories->protocols != 0){
		protocol_list = (struct objc_protocol_list *)
				(cat_cls->reserved1 + 
				((unsigned long)categories->protocols -
				(unsigned long)cat_cls->addr));
		swap_objc_protocol_lists(protocol_list, 
					 sizeof(struct objc_protocol_list) -
					 sizeof(struct objc_protocol *));
	    }
	    categories++;
	}
	for (i = 0; i < protocols_size; i += sizeof(struct objc_protocol)){
	    if(protocols->instance_methods != 0){
		mdl = (struct objc_method_description_list *)
			    (cat_inst->reserved1 + 
			    ((unsigned long)protocols->instance_methods -
			    (unsigned long)cat_inst->addr));
		swap_objc_method_description_lists(mdl,
			sizeof(struct objc_method_description_list) -
			sizeof(struct objc_method_description));
	    }

	    if(protocols->class_methods != 0){
		mdl = (struct objc_method_description_list *)
			    (cat_cls->reserved1 + 
			    ((unsigned long)protocols->class_methods -
			    (unsigned long)cat_cls->addr));
		swap_objc_method_description_lists(mdl,
			sizeof(struct objc_method_description_list) -
			sizeof(struct objc_method_description));
	    }
	    protocols++;
	}
}

static
void
swap_objc_module(
struct objc_module *module,
enum NXByteOrder target_byte_order)
{
	module->version = NXSwapLong(module->version);
	module->size = NXSwapLong(module->size);
	module->name = (char *) NXSwapLong((long)module->name);
	module->symtab = (Symtab) NXSwapLong((long)module->symtab);
}

static
void
swap_objc_symtab(
struct objc_symtab *symtab,
enum NXByteOrder target_byte_order)
{
	symtab->sel_ref_cnt = NXSwapLong(symtab->sel_ref_cnt);
	symtab->refs = (SEL *) NXSwapLong((long)symtab->refs);
	symtab->cls_def_cnt = NXSwapShort(symtab->cls_def_cnt);
	symtab->cat_def_cnt = NXSwapShort(symtab->cat_def_cnt);
}

static
void
swap_objc_class(
struct objc_class *objc_class,
enum NXByteOrder target_byte_order)
{
	objc_class->isa = (struct objc_class *)
		NXSwapLong((long)objc_class->isa);
	objc_class->super_class = (struct objc_class *)
		NXSwapLong((long)objc_class->super_class);
	objc_class->name = (const char *)
		NXSwapLong((long)objc_class->name);		
	objc_class->version =
		NXSwapLong(objc_class->version);
	objc_class->info =
		NXSwapLong(objc_class->info);
	objc_class->instance_size =
		NXSwapLong(objc_class->instance_size);
	objc_class->ivars = (struct objc_ivar_list *)
		NXSwapLong((long)objc_class->ivars);
	objc_class->methods = (struct objc_method_list *)
		NXSwapLong((long)objc_class->methods);
	objc_class->cache = (struct objc_cache *)
		NXSwapLong((long)objc_class->cache);
	objc_class->protocols = (struct objc_protocol_list *)
		NXSwapLong((long)objc_class->protocols);
}

static
void
swap_objc_category(
struct objc_category *objc_category,
enum NXByteOrder target_byte_order)
{
	objc_category->category_name = (char *)
		NXSwapLong((long)objc_category->category_name);
	objc_category->class_name = (char *)
		NXSwapLong((long)objc_category->class_name);
	objc_category->instance_methods = (struct objc_method_list *)
		NXSwapLong((long)objc_category->instance_methods);
	objc_category->class_methods = (struct objc_method_list *)
		NXSwapLong((long)objc_category->class_methods);
	objc_category->protocols = (struct objc_protocol_list *)
		NXSwapLong((long)objc_category->protocols);
}

static
void
swap_objc_ivar_list(
struct objc_ivar_list *objc_ivar_list,
enum NXByteOrder target_byte_order)
{
	objc_ivar_list->ivar_count = NXSwapLong(objc_ivar_list->ivar_count);
}

static
void
swap_objc_ivar(
struct objc_ivar *objc_ivar,
enum NXByteOrder target_byte_order)
{
	objc_ivar->ivar_name = (char *)
		NXSwapLong((long)objc_ivar->ivar_name);
	objc_ivar->ivar_type = (char *)
		NXSwapLong((long)objc_ivar->ivar_type);
	objc_ivar->ivar_offset = 
		NXSwapLong(objc_ivar->ivar_offset);
}

static
void
swap_objc_method_list(
struct objc_method_list *method_list,
enum NXByteOrder target_byte_order)
{
	method_list->method_next = (struct objc_method_list *)
		NXSwapLong((long)method_list->method_next);
	method_list->method_count = 
		NXSwapLong(method_list->method_count);
}

static
void
swap_objc_method(
struct objc_method *method,
enum NXByteOrder target_byte_order)
{
	method->method_name = (SEL)
		NXSwapLong((long)method->method_name);
	method->method_types = (char *)
		NXSwapLong((long)method->method_types);
	method->method_imp = (IMP)
		NXSwapLong((long)method->method_imp);
}

static
void
swap_objc_protocol_list(
struct objc_protocol_list *protocol_list,
enum NXByteOrder target_byte_order)
{
	protocol_list->next = (struct objc_protocol_list *)
		NXSwapLong((long)protocol_list->next);
	protocol_list->count =
		NXSwapLong(protocol_list->count);
}

static
void
swap_objc_protocol(
Protocol *p,
enum NXByteOrder target_byte_order)
{
    struct objc_protocol *protocol;

	protocol = (struct objc_protocol *)p;

	protocol->isa = (struct objc_class *)
		NXSwapLong((long)protocol->isa);
	protocol->protocol_name = (char *)
		NXSwapLong((long)protocol->protocol_name);
	protocol->protocol_list = (struct objc_protocol_list *)
		NXSwapLong((long)protocol->protocol_list);
	protocol->instance_methods = (struct objc_method_description_list *)
		NXSwapLong((long)protocol->instance_methods);
	protocol->class_methods = (struct objc_method_description_list *)
		NXSwapLong((long)protocol->class_methods);

}

static
void
swap_objc_method_description_list(
struct objc_method_description_list *mdl,
enum NXByteOrder target_byte_order)
{
	mdl->count = NXSwapLong(mdl->count);
}

static
void
swap_objc_method_description(
struct objc_method_description *md,
enum NXByteOrder target_byte_order)
{
	md->name = (SEL)NXSwapLong((long)md->name);
	md->types = (char *)NXSwapLong((long)md->types);
}

void
swap_string_object(
NXConstantString *p,
enum NXByteOrder target_byte_order)
{
    struct objc_string_object *string_object;

	string_object = (struct objc_string_object *)p;

	string_object->isa = (struct objc_class *)
		NXSwapLong((long)string_object->isa);
	string_object->characters = (char *)
		NXSwapLong((long)string_object->characters);
	string_object->_length =
		NXSwapLong(string_object->_length);
}

static
void
swap_hashEntry(
struct _hashEntry *_hashEntry,
enum NXByteOrder target_byte_order)
{
	_hashEntry->next = (struct _hashEntry *)
		NXSwapLong((long)_hashEntry->next);
	_hashEntry->sel = (char *)
		NXSwapLong((long)_hashEntry->sel);
}

static void swap_objc_modules(
	struct objc_module *modules,
	unsigned long size
)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += sizeof(struct objc_module), j++)
		swap_objc_module(modules + j, NXHostByteOrder());
}

static void swap_objc_symtabs(
	struct objc_symtab *symtabs,
	unsigned long size
)
{
	int i, j;
	struct objc_symtab *symtab;

	symtab = symtabs;
	while ((unsigned long)symtab - (unsigned long)symtabs < size){
		swap_objc_symtab(symtab, NXHostByteOrder());
		for(j = 0; j < symtab->cls_def_cnt; j++){
		    symtab->defs[j] = (struct objc_class *)
			NXSwapLong((long)symtab->defs[j]);
		}
		for(j = 0; j < symtab->cat_def_cnt; j++){
		    symtab->defs[j + symtab->cls_def_cnt] =
			(struct objc_class *)
			NXSwapLong((long)symtab->defs[j + symtab->cls_def_cnt]);
		}
		symtab = (struct objc_symtab *)
		    (&(symtab->defs[symtab->cls_def_cnt +
				    symtab->cat_def_cnt]));
	}
}

static void swap_objc_classes(
	struct objc_class *classes,
	unsigned long size
)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += sizeof(struct objc_class), j++)
		swap_objc_class(classes + j, NXHostByteOrder());
}

static void swap_objc_categories(
	struct objc_category *categories,
	unsigned long size
)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += sizeof(struct objc_category), j++)
		swap_objc_category(categories + j, NXHostByteOrder());
}

static void swap_objc_method_lists(
	struct objc_method_list *method_lists,
	unsigned long size
)
{
	int i, j;
	struct objc_method_list *method_list;

	method_list = method_lists;
	while ((unsigned long)method_list - (unsigned long)method_lists < size){
		swap_objc_method_list(method_list, NXHostByteOrder());
		for (j = 0; j < method_list->method_count; j++){
		    swap_objc_method(&(method_list->method_list[j]),
				     NXHostByteOrder());
		}
		method_list = (struct objc_method_list *)
		    (&(method_list->method_list[method_list->method_count]));
	}
}

static void swap_objc_protocols(
	Protocol *protocols,
	unsigned long size
)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += sizeof(struct objc_protocol), j++)
		swap_objc_protocol(protocols + j, NXHostByteOrder());
}

static void swap_objc_ivar_lists(
	struct objc_ivar_list *ivar_lists,
	unsigned long size
)
{
	int i, j;
	struct objc_ivar_list *ivar_list;

	ivar_list = ivar_lists;
	while ((unsigned long)ivar_list - (unsigned long)ivar_lists < size){
		swap_objc_ivar_list(ivar_list, NXHostByteOrder());
		for (j = 0; j < ivar_list->ivar_count; j++){
		    swap_objc_ivar(&(ivar_list->ivar_list[j]),
				     NXHostByteOrder());
		}
		ivar_list = (struct objc_ivar_list *)
		    (&(ivar_list->ivar_list[ivar_list->ivar_count]));
	}
}

static void swap_objc_protocol_lists(
	struct objc_protocol_list *protocol_lists,
	unsigned long size
)
{
	int i, j;
	struct objc_protocol_list *protocol_list;

	protocol_list = protocol_lists;
	while ((unsigned long)protocol_list -
	       (unsigned long)protocol_lists < size){
		swap_objc_protocol_list(protocol_list, NXHostByteOrder());
		for (j = 0; j < protocol_list->count; j++){
		    protocol_list->list[j] = (Protocol *)
			NXSwapLong((long)(protocol_list->list[j]));
		}
		protocol_list = (struct objc_protocol_list *)
		    (&(protocol_list->list[protocol_list->count]));
	}
}

static void swap_objc_method_description_lists(
	struct objc_method_description_list *mdls,
	unsigned long size
)
{
	int i, j;
	struct objc_method_description_list *mdl;

	mdl = mdls;
	while ((unsigned long)mdl - (unsigned long)mdls < size){
		swap_objc_method_description_list(mdl, NXHostByteOrder());
		for (j = 0; j < mdl->count; j++){
		    swap_objc_method_description( &(mdl->list[j]),
				     		 NXHostByteOrder());
		}
		mdl = (struct objc_method_description_list *)
			(&(mdl->list[mdl->count]));
	}
}

static void *getObjcData(
    	struct section *objcsects, 
	int nsects,
	const void *addr
)
{
	int i;

	if (addr == 0)
		return 0;

	for (i = 0; i < nsects; i++) {
		struct section *s;

		s = objcsects + i;
		if (((long)addr >= s->addr) && 
			((long)addr < (s->addr + s->size)))
		  return (void *)(s->reserved1 + ((long)addr - s->addr));
	}
    	fprintf(stderr, "%s : Could not `getObjcData'\n", progname);
	exit(1);
}

static void swap_objc_refs(
	unsigned long *refs,
	unsigned long size
)
{
	int i,j;

	for (i = 0, j = 0; i < size; i += sizeof(unsigned long), j++)
		refs[j] = NXSwapLong(refs[j]);
}

static struct section *getObjcSection(
    	struct section *objcsects, 
	int nsects,
	char *name	
)
{
	int i;

	for (i = 0; i < nsects; i++) {
		struct section *s;

		s = objcsects + i;
		if (strncmp(s->sectname,name,16) == 0)
		  return s;
	}
        return 0;
}

static void swap_string_objects(
	NXConstantString *s,
	unsigned long size
)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += sizeof(struct objc_string_object), j++)
		swap_string_object(s + j, NXHostByteOrder());
}

static void swap_hashEntries(
	struct _hashEntry *_hashEntries,
	unsigned long size
)
{
	int i, j;
	enum NXByteOrder target_byte_order;

	target_byte_order = NXHostByteOrder() == NX_BigEndian ?
			    NX_LittleEndian : NX_BigEndian;

	for (i = 0, j = 0; i < size; i += sizeof(struct _hashEntry), j++)
		swap_hashEntry(_hashEntries + j, target_byte_order);
}

/*
 * Print the objc segment.
 */
static
struct objc_module *
get_objc(fd, filename, firstobjcsect, nsects)
long fd;
char *filename;
struct section *firstobjcsect;
int nsects;
{
	long i, j;
	struct section *modsect, *msgsect, *selsect, *protosect;
	struct objc_module *modules, *m;
	struct objc_symtab *t;

	readObjcData(fd, filename, firstobjcsect, nsects);
	if(swapped)
	    swapObjcData(firstobjcsect, nsects);

	modsect = getObjcSection(firstobjcsect, nsects, "__module_info");
	msgsect = getObjcSection(firstobjcsect, nsects, "__message_refs");
  	if (msgsect) {
		int i, cnt = msgsect->size/sizeof(SEL);
   		SEL *sels = (SEL *)msgsect->reserved1;

		/* overwrite the string with a unique identifier */
		for (i = 0; i < cnt; i++) 
			sels[i] = (SEL)sel_registerName(
   				getObjcData(firstobjcsect, nsects,
						sels[i]));
	}
	selsect = getObjcSection(firstobjcsect, nsects, "__selector_refs");
  	if (selsect) {
		int i, cnt = selsect->size/sizeof(SEL);
   		SEL *sels = (SEL *)selsect->reserved1;

		/* overwrite the string with a unique identifier */
		for (i = 0; i < cnt; i++) 
			sels[i] = (SEL)sel_registerName(
   				getObjcData(firstobjcsect, nsects,
						sels[i]));
	}
	modules = (Module)modsect->reserved1;

	for(m = modules ;
	    (char *)m < (char *)modules + modsect->size;
	    m = (struct objc_module *)((char *)m + m->size) ){

		/* relocate fields in module structure */

		m->name = (char *)getObjcData(firstobjcsect, nsects, 
						m->name);
		m->symtab = t = (Symtab)getObjcData(firstobjcsect, nsects, 
						m->symtab);
						
		if (t) {

		  /* Simulate map! */
		  if (m->version == 1) { 
		    int i, cnt = m->symtab->sel_ref_cnt;
   		    SEL *sels = (SEL *)getObjcData(firstobjcsect, nsects,
						m->symtab->refs);

		    /* overwrite the string with a unique identifier */
		    for (i = 0; i < cnt; i++) 
			  sels[i] = (SEL)sel_registerName(
   				getObjcData(firstobjcsect, nsects,
						sels[i]));
		  }

		  for(i = 0; i < t->cls_def_cnt; i++){

    			struct objc_class *objc_class;

			t->defs[i] = objc_class = 
   				(Class)getObjcData(firstobjcsect, nsects,
						t->defs[i]);
print_objc_class:
			/* relocate class structure */

			if(CLS_GETINFO(objc_class, CLS_META)){
				objc_class->isa = (Class)getObjcData(
					firstobjcsect, nsects, objc_class->isa);
			}

			objc_class->super_class = (Class)getObjcData(
						firstobjcsect, nsects, 
						objc_class->super_class);

			objc_class->name = (char *)getObjcData(
						firstobjcsect, nsects, 
						objc_class->name);

			if (objc_class->ivars) {
    				struct objc_ivar_list *ilist;
    				struct objc_ivar *ivar;

				objc_class->ivars = ilist = 
					(struct objc_ivar_list *)getObjcData(
						firstobjcsect, nsects, 
						objc_class->ivars);

				ivar = ilist->ivar_list;
				for(j = 0; j < ilist->ivar_count; j++, ivar++){
					ivar->ivar_name= (char *)getObjcData(
						firstobjcsect, nsects, 
						ivar->ivar_name);

					ivar->ivar_type= (char *)getObjcData(
						firstobjcsect, nsects, 
						ivar->ivar_type);
				}
			}

			if (objc_class->methods) {
				print_method_list(objc_class->methods, 
						firstobjcsect, nsects);
			}

			if(CLS_GETINFO(objc_class, CLS_CLASS)){
			    objc_class->isa = objc_class = 
   					(Class)getObjcData(
						firstobjcsect, nsects,
						objc_class->isa);
			    goto print_objc_class;
			}
		  }

		  for(i = 0; i < t->cat_def_cnt; i++){

    			struct objc_category *objc_category;

			t->defs[i + t->cls_def_cnt] = objc_category = 
   				(Category)getObjcData(firstobjcsect, nsects,
						t->defs[i+t->cls_def_cnt]);

			objc_category->category_name = (char *)getObjcData(
						firstobjcsect, nsects, 
				       		objc_category->category_name);

			objc_category->class_name = (char *)getObjcData(
						firstobjcsect, nsects, 
				       		objc_category->class_name);

			if (objc_category->instance_methods) {
			    print_method_list(objc_category->instance_methods, 
						firstobjcsect, nsects);
			}

			if (objc_category->class_methods) {
			    print_method_list(objc_category->class_methods, 
						firstobjcsect, nsects);
			}
		  }
	        }
	}
	protosect = getObjcSection(firstobjcsect, nsects, "__protocol");
  	if (protosect) {
		int i, cnt = protosect->size/sizeof(Protocol);
		struct proto_template { @defs(Protocol) } *protos;
   		
		protos = (struct proto_template *) protosect->reserved1;

		for (i = 0; i < cnt; i++)
		  {
		    if (protos[i].instance_methods) {
			print_method_list2(protos[i].instance_methods, 
					    firstobjcsect, nsects);
		    }

		    if (protos[i].class_methods) {
			print_method_list2(protos[i].class_methods, 
					    firstobjcsect, nsects);
		    }
		  }
	}
	return modules;
}

static
void
print_method_list(struct objc_method_list *mlist_before_reloc, 
		  struct section *firstobjcsect, 
		  int nsects)
{
    long i;
    struct objc_method *method;
    struct objc_method_list *mlist;

	mlist = (struct objc_method_list *)getObjcData(firstobjcsect, nsects,
				    		mlist_before_reloc);
	method = mlist->method_list;
	for(i = 0; i < mlist->method_count; i++, method++){

	    method->method_name = (SEL)getObjcData(firstobjcsect, nsects, 
		           			method->method_name);

	    method->method_types = (char *)getObjcData(firstobjcsect, nsects, 
		           			method->method_types);

	    method->method_name = 
		(SEL)sel_registerName((STR)method->method_name);

	}
}

static
void
print_method_list2(struct objc_method_description_list *mlist_before_reloc, 
		  struct section *firstobjcsect, 
		  int nsects)
{
    long i;
    struct objc_method_description *method;
    struct objc_method_description_list *mlist;

	mlist = (struct objc_method_description_list *)getObjcData(firstobjcsect, nsects,
				    		mlist_before_reloc);
	method = mlist->list;
	for(i = 0; i < mlist->count; i++, method++){

	    method->name = (SEL)getObjcData(firstobjcsect, nsects, 
		           			method->name);
	    method->name = 
		(SEL)sel_registerName((STR)method->name);

	}
}

/*
 * Print the current usage message.
 */
static
void
usage()
{
	fprintf(stderr,
		"Usage: %s <shlib file>\n",
		progname);
}
