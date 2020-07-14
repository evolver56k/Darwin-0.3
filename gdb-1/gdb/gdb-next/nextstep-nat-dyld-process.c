#include "nextstep-nat-dyld-process.h"

#include "nextstep-nat-dyld-info.h"
#include "nextstep-nat-dyld-path.h"
#include "nextstep-nat-dyld-io.h"
#include "nextstep-nat-inferior.h"
#include "nextstep-nat-mutils.h"

#include "defs.h"
#include "inferior.h"
#include "symfile.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "objfiles.h"

#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_debug.h>

#include <assert.h>
#include <string.h>

#include "mach-o.h"

#include "gdbcore.h"		/* for core_ops */
extern struct target_ops exec_ops;

static int dyld_allow_resolve_filenames_flag = 1;
static int dyld_always_resolve_filenames_flag = 0;
static int dyld_always_read_from_memory_flag = 0;
static int dyld_remove_overlapping_basenames_flag = 1;
static char *dyld_symbols_prefix = "dyld_";
static int dyld_load_dyld_symbols_flag = 1;
static int dyld_load_shlib_symbols_flag = 1;

#if WITH_CFM
extern int inferior_auto_start_cfm_flag;
#endif /* WITH_CFM */

extern next_inferior_status *next_status;

void dyld_add_inserted_libraries
  (struct dyld_objfile_info *info, const struct dyld_path_info *d)
{
  const char *s1, *s2;

  assert (info != NULL);
  assert (d != NULL);

  s1 = d->insert_libraries;
  if (s1 == NULL) { return; }

  while (*s1 != '\0') {

    struct dyld_objfile_entry *e = NULL;

    s2 = strchr (s1, ':');
    if (s2 == NULL) {
      s2 = strchr (s1, '\0');
    }
    assert (s2 != NULL);

    e = dyld_objfile_entry_alloc (info);

    e->user_name = savestring (s1, (s2 - s1));
    e->load_flag = 1;
    e->reason = dyld_reason_init;

    s1 = s2; 
  }
}

void dyld_add_image_libraries
  (struct dyld_objfile_info *info, bfd *abfd)
{
  struct mach_o_data_struct *mdata = NULL;
  unsigned int i;

  assert (info != NULL);

  if (abfd == NULL) { return; }

  assert (bfd_mach_o_valid (abfd));
  mdata = abfd->tdata.mach_o_data;

  if (NULL == mdata)
    {
      dyld_debug("dyld_add_image_libraries: mdata == NULL\n");
      return;
    }

  for (i = 0; i < mdata->header.ncmds; i++) {
    struct bfd_mach_o_load_command *cmd = &mdata->commands[i];
    switch (cmd->type) {
    case BFD_MACH_O_LC_LOAD_DYLINKER:
    case BFD_MACH_O_LC_ID_DYLINKER:
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB: {

      struct dyld_objfile_entry *e = NULL;
      char *name = NULL;

      switch (cmd->type) {
      case BFD_MACH_O_LC_LOAD_DYLINKER:
      case BFD_MACH_O_LC_ID_DYLINKER: {
	bfd_mach_o_dylinker_command *dcmd = &cmd->command.dylinker;

	name = xmalloc (dcmd->name_len + 1);
            
	bfd_seek (abfd, dcmd->name_offset, SEEK_SET);
	if (bfd_read (name, 1, dcmd->name_len, abfd) != dcmd->name_len) {
	  warning ("Unable to find library name for LC_LOAD_DYLINKER or LD_ID_DYLINKER command; ignoring");
	  free (name);
	  continue;
	}
	break;
      }
      case BFD_MACH_O_LC_LOAD_DYLIB:
      case BFD_MACH_O_LC_ID_DYLIB: {
	bfd_mach_o_dylib_command *dcmd = &cmd->command.dylib;

	name = xmalloc (dcmd->name_len + 1);
            
	bfd_seek (abfd, dcmd->name_offset, SEEK_SET);
	if (bfd_read (name, 1, dcmd->name_len, abfd) != dcmd->name_len) {
	  warning ("Unable to find library name for LC_LOAD_DYLIB or LD_ID_DYLIB command; ignoring");
	  free (name);
	  continue;
	}
	break;
      }
      default:
	abort ();
      }
      
      if (name[0] == '\0') {
	warning ("No image name specified by LC_LOAD or LC_ID command; ignoring");
	free (name);
	name = NULL;
      }

      e = dyld_objfile_entry_alloc (info);

      e->text_name = name;
      e->text_name_valid = 1;
      e->load_flag = 1;
      e->reason = dyld_reason_init;

      switch (cmd->type) {
      case BFD_MACH_O_LC_LOAD_DYLINKER:
      case BFD_MACH_O_LC_ID_DYLINKER:
	e->prefix = dyld_symbols_prefix;
	break;
      case BFD_MACH_O_LC_LOAD_DYLIB:
      case BFD_MACH_O_LC_ID_DYLIB:
	break;
      default:
	abort ();
      };
    }
    default:
      break;
    }
  }
}

void dyld_resolve_filename_image
(const struct next_inferior_status *s, struct dyld_objfile_entry *e)
{
  struct mach_header header;

  CORE_ADDR curpos;
  unsigned int i;

  assert (e->allocated);
  if (e->image_name_valid) { return; }

  if (! e->dyld_valid) { return; }

  target_read_memory (e->dyld_addr, (char *) &header, sizeof (struct mach_header));

  switch (header.filetype) {
  case MH_DYLINKER:
  case MH_DYLIB:
    break;
  case MH_BUNDLE:
    break;
  default:
    return;
  }

  curpos = ((unsigned long) e->dyld_addr) + sizeof (struct mach_header);
  for (i = 0; i < header.ncmds; i++) {

    struct load_command cmd;
    struct dylib_command dcmd;
    struct dylinker_command dlcmd;
    char name[256];

    target_read_memory (curpos, (char *) &cmd, sizeof (struct load_command));
    if (cmd.cmd == LC_ID_DYLIB) {
      target_read_memory (curpos, (char *) &dcmd, sizeof (struct dylib_command));
      target_read_memory (curpos + dcmd.dylib.name.offset, name, 256);
      e->image_name = strsave (name);
      e->image_name_valid = 1;
      break;
    } else if (cmd.cmd == LC_ID_DYLINKER) {
      target_read_memory (curpos, (char *) &dlcmd, sizeof (struct dylinker_command));
      target_read_memory (curpos + dlcmd.name.offset, name, 256);
      e->image_name = strsave (name);
      e->image_name_valid = 1;
      break;
    }

    curpos += cmd.cmdsize;
  }

  if (e->image_name == NULL) {
    dyld_debug ("Unable to determine filename for loaded object (no LC_ID load command)\n");
  }
}

void dyld_resolve_filenames_image
(const struct next_inferior_status *s, struct dyld_objfile_info *new)
{
  unsigned int i;

  assert (s != NULL);
  assert (new != NULL);

  for (i = 0; i < new->nents; i++) {
    struct dyld_objfile_entry *e = &new->entries[i];
    if (! e->allocated) { continue; }
    dyld_resolve_filename_image (s, e);
  }
}

void dyld_resolve_filename_dyld
(const struct next_inferior_status *s, struct dyld_objfile_entry *e)
{
  char *iname, *mname;
  unsigned long inamelen, mnamelen;
  enum dyld_debug_return ret;
  struct dyld_debug_module module;
  kern_return_t kret;
    
  assert (e->allocated);
      
  if (e->dyld_name_valid) { return; }

  if (! dyld_allow_resolve_filenames_flag) { return; }
  if ((e->image_name != NULL) && (! dyld_always_resolve_filenames_flag)) { return; }
    
  if (! e->dyld_valid) {
    dyld_debug ("Unable to determine name for dynamic library; skipping "
		"(dyld information not valid for library)\n");
    return;
  }

  if (s->dyld_status.state != dyld_started) {
    dyld_debug ("Unable to determine name for dynamic library at 0x%lx "
		"(offset 0x%lx); skipping (no dyld thread present in inferior)\n",
		(unsigned long) e->dyld_addr, 
		(unsigned long) e->dyld_slide);
  }

  module.header = (struct mach_header *) (((unsigned char *) 0) + e->dyld_addr);
  module.vmaddr_slide = e->dyld_slide;
  module.module_index = e->dyld_index;

  assert (s->dyld_status.state == dyld_started);
  ret = _dyld_debug_module_name (s->task, 120000, 120000, 0, module,
				 &iname, &inamelen, &mname, &mnamelen);

  if (ret != DYLD_SUCCESS) {
    warning ("Unable to determine name for dynamic library at 0x%lx "
	     "(offset 0x%lx): %s (%d); skipping",
	     (unsigned long) e->dyld_addr, 
	     (unsigned long) e->dyld_slide,
	     dyld_debug_error_string (ret), ret);
    return;
  }

  if ((iname == NULL) || (iname[0] == '\0')) {
    warning ("Empty filename returned from DYLD; ignoring");
  } else {
    e->dyld_name = strsave (iname);
  }
  e->dyld_name_valid = 1;

  kret = vm_deallocate (task_self(), iname - ((char *) 0), inamelen);
  if (kret != KERN_SUCCESS) {
    warning ("Unable to deallocate memory used to read module name: %s",
	     mach_error_string (kret));
  }
  kret = vm_deallocate (task_self(), mname - ((char *) 0), mnamelen);
  if (kret != KERN_SUCCESS) {
    warning ("Unable to deallocate memory used to read module name: %s",
	     mach_error_string (kret));
  }
}

void dyld_resolve_filenames_dyld
(const struct next_inferior_status *s, struct dyld_objfile_info *new)
{
  unsigned int i;

  assert (s != NULL);
  assert (new != NULL);

  for (i = 0; i < new->nents; i++) {
    struct dyld_objfile_entry *e = &new->entries[i];
    if (! e->allocated) { continue; }
    dyld_resolve_filename_dyld (s, e);
  }
}

static CORE_ADDR library_offset (struct dyld_objfile_entry *e)
{
  assert (e != NULL);
  if (e->image_addr_valid && e->dyld_valid) {
    assert (e->dyld_addr == ((e->image_addr + e->dyld_slide) & 0xffffffff));
  }

  if (e->dyld_valid) {
    return (unsigned long) e->dyld_addr;
  } else if (e->image_addr_valid) {
    return (unsigned long) e->image_addr;
  } else {
    return 0;
  }
}

void dyld_load_library (const struct dyld_path_info *d, struct dyld_objfile_entry *e)
{
  int read_from_memory = 0;
  const char *name = NULL;
  const char *name2 = NULL;

  assert (e->allocated);

  if (e->loaded_flag) { return; }
  if (e->loaded_error) { return; }

  if (dyld_always_read_from_memory_flag) {
    read_from_memory = 1;
  }

  if (! read_from_memory) {
    name = dyld_entry_source_filename (e);
    if (name == NULL) {
      char *s = dyld_entry_string (e);
      warning ("No image filename available for %s; reading from memory", s);
      free (s);
      read_from_memory = 1;
    }
    name2 = dyld_resolve_image (d, name);
    if (name2 == NULL) {
      char *s = dyld_entry_string (e);
      warning ("Unable to resolve source pathname for %s; reading from memory", s);
      free (s);
      read_from_memory = 1;
    }      
  }

  if (read_from_memory && (! e->dyld_valid)) {
      char *s = dyld_entry_string (e);
      warning ("Unable to read symbols from %s (not yet mapped into memory); skipping", s);
      free (s);
      return;
  }    

  if (read_from_memory) {
    assert (e->dyld_valid);
    e->abfd = dyld_map_image (e->dyld_addr, e->dyld_slide);
    e->loaded_memaddr = e->dyld_addr;
    e->loaded_from_memory = 1;
  } else {
    assert (name2 != NULL);
    e->abfd = symfile_bfd_open_safe (name2);
    e->loaded_name = name2;
    e->loaded_from_memory = 0;
  }

  if (e->abfd == NULL) {
    char *s = dyld_entry_string (e);
    e->loaded_error = 1;
    warning ("Unable to read symbols from %s; skipping.", s);
    free (s);
    return;
  }

  {
    asection *text_sect = bfd_get_section_by_name (e->abfd, "LC_SEGMENT.__TEXT");
    if (text_sect != NULL) {
      e->image_addr = bfd_section_vma (e->abfd, text_sect);
      e->image_addr_valid = 1;
    } else {
      char *s = dyld_entry_string (e);
      warning ("Unable to locate text section for %s (no section \"%s\")\n", 
	       s, "LC_SEGMENT.__TEXT");
      free (e);
    }
  }
}

void dyld_load_libraries (const struct dyld_path_info *d, struct dyld_objfile_info *result)
{
  unsigned int i;
  assert (result != NULL);

  for (i = 0; i < result->nents; i++) {
    struct dyld_objfile_entry *e = &result->entries[i];
    if (! e->allocated) { continue; }
    if (e->load_flag) {
      dyld_load_library (d, e);
    }
  }
}

void dyld_load_symfile (struct dyld_objfile_entry *e) 
{
  char *name = NULL;
  char *leaf = NULL;
  int load_symbols = 0;

  assert (e->allocated);

  name = dyld_entry_string (e);
  
  if (e->abfd == NULL) { return; }
  if (e->objfile != NULL)
    {
      return;
    }

  if ((! e->dyld_valid) && (e->image_addr == 0)) {
    printf_filtered ("Deferring load of %s (not pre-bound)\n", name);
    free (name);
    return;
  }

  e->loaded_flag = 1;

  if (e->dyld_valid) { 
    e->loaded_addr = e->dyld_addr;
    e->loaded_addrisoffset = 0;
  } else {
    e->loaded_addr = e->image_addr;
    e->loaded_addrisoffset = 0;
  }

  free (name);
  name = dyld_entry_string (e);

  leaf = strrchr (name, '/');
  leaf = ((leaf != NULL) ? leaf : name);

#if WITH_CFM
  if (inferior_auto_start_cfm_flag && (strstr (leaf, "CarbonCore") != NULL)) {
    load_symbols = 1;
  }
#endif /* WITH_CFM */

  if ((strstr (leaf, "dyld") != NULL) && dyld_load_dyld_symbols_flag) {
    load_symbols = 1;
  }

  if (dyld_load_shlib_symbols_flag) {
    load_symbols = 1;
  }

  if (load_symbols) {
    printf_filtered ("Reading symbols from %s...", name);
    gdb_flush (gdb_stdout);
    
    e->objfile = symbol_file_add_bfd_safe
      (e->abfd, 0, e->loaded_addr, e->loaded_addrisoffset, 0, 0, 0, 0, 0, 1, e->prefix);
    
    if (e->objfile == NULL) {
      e->loaded_error = 1;
      e->abfd = NULL;
      free (name);
      return;
    }

    printf_filtered ("done\n");
    gdb_flush (gdb_stdout);
  }

#if WITH_CFM
  if (inferior_auto_start_cfm_flag && (strstr (leaf, "CarbonCore") != NULL)) {

    struct expression* expr = parse_expression ("&gPCFMInfoHooks");
    if (expr != NULL) {
      value_ptr val = evaluate_expression (expr);

      if (val != NULL) {

	dyld_debug (stderr, "gPCFMInfoHooks in CarbonCore @ %#x\n", val->aligner.contents[0]);
	next_status->cfm_info.info_api_cookie = (void *) val->aligner.contents[0];
	  
	/* At one time gPCFMInfoHooks was in libbase.dylib which nothing was directly
	   linked against. By the time GDB noticed the library was loaded the program
	   was already running and so the CFM API had to be initialized then. Currently
	   LauchCFMApp is linked directly against CarbonCore and so the location of
	   gPCFMInfoHooks is known when the symbol table is read (before the program
	   starts) - you can't initialize the CFM API before the program is running
	   though since the initialization reads memory from the inferior. This init
	   call here probably isn't needed, but is being left in case some CFM launching
	   app loads CarbonCore indirectly. The routine checks to make sure the inferior
	   exists and also that it doesn't initialize itself twice, so having it here
	   and being called twice won't hurt anything. */

	next_init_cfm_info_api (next_status);
      }
    }
  }
#endif /* WITH_CFM */
  
  if (e->objfile == NULL) { 
    return;
  }

  assert (e->objfile->obfd != NULL);

  if (build_objfile_section_table (e->objfile)) {
    warning ("Unable to build section table for %s.", name);
    free (name);
    return;
  }

  free (name);
}

void dyld_load_symfiles (struct dyld_objfile_info *result)
{
  unsigned int i;
  assert (result != NULL);

  for (i = 0; i < result->nents; i++) {
    struct dyld_objfile_entry *e = &result->entries[i];
    if (! e->allocated) { continue; }
    dyld_load_symfile (e);
  }
}

void dyld_merge_libraries
(struct dyld_objfile_info *old, struct dyld_objfile_info *new, struct dyld_objfile_info *result)
{
  unsigned int i;

  assert (old != NULL);
  assert (old != new);
  assert (old != result);
  assert (new != result);

  /* remaining files in 'old' will be cached for future use */
  for (i = 0; i < old->nents; i++) {

      struct dyld_objfile_entry *o = &old->entries[i];
      struct dyld_objfile_entry *e = NULL;

      if (! o->allocated) { continue; }

      e = dyld_objfile_entry_alloc (result);
      *e = *o;

      e->reason = dyld_reason_cached;

      dyld_objfile_entry_clear (o);
  }
    
  /* all remaining files in 'new' will need to be loaded */
  for (i = 0; i < new->nents; i++) {

    struct dyld_objfile_entry *n = &new->entries[i];
    struct dyld_objfile_entry *e = NULL;
    
    if (! n->allocated) { continue; }
    
    e = dyld_objfile_entry_alloc (result);
    *e = *n;
    
    dyld_objfile_entry_clear (n);
  }
}

static int dyld_objfile_allocated (struct objfile *o)
{
  struct objfile *objfile, *temp;

  ALL_OBJFILES_SAFE (objfile, temp) {
    if (o == objfile) {
      return 1;
    }
  }
  return 0;
}

void dyld_remove_objfile (struct dyld_objfile_entry *e)
{
  char *s = NULL;

  assert (e->allocated);

  if (e->load_flag) { return; }

  if (e->objfile == NULL) { return; }

  assert (dyld_objfile_allocated (e->objfile));
  assert (e->objfile->obfd != NULL);

  s = dyld_entry_string (e);
  printf_filtered ("Removing symbols for %s...", s);
  free (s);
  gdb_flush (gdb_stdout);
  free_objfile (e->objfile);
  e->objfile = NULL;
  e->abfd = NULL;
  printf_filtered ("done\n");
  gdb_flush (gdb_stdout);
}

void dyld_remove_objfiles (struct dyld_objfile_info *result)
{
  unsigned int i;
  assert (result != NULL);

  for (i = 0; i < result->nents; i++) {
    struct dyld_objfile_entry *e = &result->entries[i];
    if (! e->allocated) { continue; }
    if ((! e->load_flag) && e->loaded_flag) {
      dyld_remove_objfile (e);
    }
  }
}

static int dyld_libraries_similar
(struct dyld_objfile_entry *f, struct dyld_objfile_entry *l)
{
  const char *fname = NULL;
  const char *lname = NULL;

  const char *fbase = NULL;
  const char *lbase = NULL;
  unsigned int flen = 0;
  unsigned int llen = 0;

  assert (f != NULL);
  assert (l != NULL);

  fname = dyld_entry_source_filename (f);
  lname = dyld_entry_source_filename (l);

  if ((lname != NULL) && (fname != NULL)) {
    dyld_library_basename (fname, &fbase, &flen, NULL);
    dyld_library_basename (lname, &lbase, &llen, NULL);
    if ((flen == llen) && (strncmp (fbase, lbase, llen) == 0)) {
      return 1;
    }
  }
  
  if (library_offset (f) == library_offset (l)
      && (library_offset (f) != 0)
      && (library_offset (l) != 0)) {
    return 1;
  }

  return 0;
}

static int dyld_libraries_compatible
(struct dyld_path_info *d,
 struct dyld_objfile_entry *f, struct dyld_objfile_entry *l)
{
  const char *fname = NULL;
  const char *lname = NULL;

  const char *fres = NULL;
  const char *lres = NULL;

  assert (f != NULL);
  assert (l != NULL);

  fname = dyld_entry_source_filename (f);
  lname = dyld_entry_source_filename (l);

  if (strcmp (f->prefix, l->prefix) != 0) {
    return 0;
  }

  if (library_offset (f) != library_offset (l)
      && (library_offset (f) != 0)
      && (library_offset (l) != 0)) {
    return 0;
  }

  if ((fname != NULL) && (lname != NULL)) {
    if (f->loaded_name == NULL) {
      fres = dyld_resolve_image (d, fname);
    } else {
      fres = fname;
    }
    if (l->loaded_name == NULL) {
      lres = dyld_resolve_image (d, lname);
    } else {
      lres = lname;
    }
    if ((fres == NULL) != (lres == NULL)) {
      return 0;
    }
    if ((fres == NULL) && (lres == NULL)) {
      if (strcmp (fname, lname) != 0) {
	return 0;
      }
    }
    assert ((fres != NULL) && (lres != NULL));
    if (strcmp (fres, lres) != 0) {
      return 0;
    }
  }

  if (dyld_always_read_from_memory_flag) {
    if (f->loaded_from_memory != l->loaded_from_memory) {
      return 0;
    }
  }

  return 1;
}
	  
void dyld_objfile_move_load_data
(struct dyld_objfile_entry *f, struct dyld_objfile_entry *l)
{
  l->objfile = f->objfile;
  l->abfd = f->abfd;
  
  l->load_flag = 1;

  l->loaded_name = f->loaded_name;
  l->loaded_memaddr = f->loaded_memaddr;
  l->loaded_addr = f->loaded_addr;
  l->loaded_offset = f->loaded_offset;
  l->loaded_addrisoffset = f->loaded_addrisoffset;
  l->loaded_from_memory = f->loaded_from_memory;
  l->loaded_error = f->loaded_error;
  l->loaded_flag = f->loaded_flag;

  f->objfile = NULL;
  f->abfd = NULL;
  
  f->load_flag = 0;

  f->loaded_name = NULL;
  f->loaded_memaddr = 0;
  f->loaded_addr = 0;
  f->loaded_offset = 0;
  f->loaded_addrisoffset = -1;
  f->loaded_from_memory = -1;
  f->loaded_error = 0;
  f->loaded_flag = 0;
}

void dyld_remove_duplicates (struct dyld_path_info *d, struct dyld_objfile_info *result)
{
  unsigned int i, j;

  assert (result != NULL);

  for (i = 0; i < result->nents; i++) {
    for (j = i + 1; j < result->nents; j++) {

      struct dyld_objfile_entry *f = &result->entries[i];
      struct dyld_objfile_entry *l = &result->entries[j];

      if ((! f->allocated) || (! l->allocated)) { continue; }

      if ((f->objfile != NULL) && (f->objfile == l->objfile)) {
	dyld_objfile_move_load_data (f, l);
      }
    }
  }

  /* remove libraries in 'old' already loaded by something in 'result' */
  for (i = 0; i < result->nents; i++) {
    for (j = i + 1; j < result->nents; j++) {

      struct dyld_objfile_entry *f = &result->entries[i];
      struct dyld_objfile_entry *l = &result->entries[j];

      if (! f->allocated) { continue; }
      if (! l->allocated) { continue; }

      if (((dyld_remove_overlapping_basenames_flag == 1) ||
	  (f->reason == dyld_reason_cached))
	  && dyld_libraries_similar (f, l)) {
	
	if (f->objfile != NULL) {
	  
	  char *s = dyld_entry_string (f);
	  assert (dyld_objfile_allocated (f->objfile));

	  if (dyld_libraries_compatible (d, f, l)) {
	    dyld_debug ("Symbols for %s already loaded; not re-processing\n", s);
	    dyld_objfile_move_load_data (f, l);
	  } else {
	    f->load_flag = 0;
	    dyld_remove_objfile (f);
	  }

	  free (s);
	}
	
	dyld_objfile_entry_clear (f);
      }
    }
  }
}

void dyld_check_discarded (struct dyld_objfile_info *info)
{
  unsigned int j;
  for (j = 0; j < info->nents; j++) {
    struct dyld_objfile_entry *e = &info->entries[j];
    if (! dyld_objfile_allocated (e->objfile)) {
      dyld_objfile_entry_clear (e);
    }
  }
}

void dyld_update_shlibs
(const struct next_inferior_status *s,
 struct dyld_path_info *d,
 struct dyld_objfile_info *old, 
 struct dyld_objfile_info *new, 
 struct dyld_objfile_info *result)
{
  assert (old != NULL);
  assert (new != NULL);
  assert (result != NULL);

  dyld_debug ("dyld_update_shlibs: updating shared library information\n");

  dyld_check_discarded (old);
  dyld_merge_libraries (old, new, result);

  dyld_resolve_filenames_image (s, result);
  dyld_resolve_filenames_dyld (s, result);
  dyld_remove_objfiles (result);
  dyld_remove_duplicates (d, result);

  dyld_objfile_info_pack (result);
  dyld_load_libraries (d, result);
  dyld_load_symfiles (result);

  dyld_merge_section_tables (result);
  dyld_update_section_tables (result, &current_target);
  dyld_update_section_tables (result, &exec_ops);

  reread_symbols ();
  breakpoint_re_set ();
}

static void dyld_process_image_event
(struct dyld_objfile_info *info,
 const struct dyld_event *event)
{
  struct dyld_debug_module module;
  struct mach_header header;
  CORE_ADDR addr, slide;

  assert (info != NULL);
  assert (event != NULL);

  module = event->arg[0];

  addr = (unsigned long) (((unsigned char *) module.header) - ((unsigned char *) 0));
  slide = (unsigned long) module.vmaddr_slide;
  
  target_read_memory (addr, (char *) &header, sizeof (struct mach_header));
  
  switch (header.filetype) {
  case MH_EXECUTE:
    dyld_debug ("Ignored executable at 0x%lx (offset 0x%lx)\n", (unsigned long) addr, slide);
    break;
  case MH_FVMLIB:
    dyld_debug ("Ignored fixed virtual memory shared library at 0x%lx (offset 0x%lx)\n",
		(unsigned long) addr, slide);
    break;
  case MH_PRELOAD:
    dyld_debug ("Ignored preloaded executable at 0x%lx (offset 0x%lx)\n", (unsigned long) addr, slide);
    break;
  case MH_DYLIB: {
    struct dyld_objfile_entry *e = dyld_objfile_entry_alloc (info);
    e->dyld_addr = addr;
    e->dyld_slide = slide;
    e->dyld_valid = 1;
    e->dyld_index = module.module_index;
    e->load_flag = 1;
    e->reason = dyld_reason_dyld;
    dyld_debug ("Noted dynamic library at 0x%lx (offset 0x%lx)\n", (unsigned long) addr, slide);
    break;
  }
  case MH_DYLINKER: {
    struct dyld_objfile_entry *e = dyld_objfile_entry_alloc (info);
    e->dyld_addr = addr;
    e->dyld_slide = slide;
    e->dyld_valid = 1;
    e->dyld_index = module.module_index;
    e->prefix = dyld_symbols_prefix;
    e->load_flag = 1;
    e->reason = dyld_reason_dyld;
    dyld_debug ("Noted dynamic link editor at 0x%lx (offset 0x%lx)\n", (unsigned long) addr, slide);
    break;
  }
  case MH_BUNDLE: {
    struct dyld_objfile_entry *e = dyld_objfile_entry_alloc (info);
    e->dyld_addr = addr;
    e->dyld_slide = slide;
    e->dyld_valid = 1;
    e->dyld_index = module.module_index;
    e->load_flag = 1;
    e->reason = dyld_reason_dyld;
    dyld_debug ("Noted bundle at 0x%lx (offset 0x%lx)\n", (unsigned long) addr, slide);
    break;
  }
  default:
    warning ("Ignored unknown object module at 0x%lx (offset 0x%lx) with type 0x%lx\n",
	     (unsigned long) addr, (unsigned long) slide, (unsigned long) header.filetype);
    break;
  }
}

void dyld_process_event
(struct dyld_objfile_info *info,
 const struct dyld_event *event)
{
  assert (info != NULL);
  assert (event != NULL);

  switch (event->type) {
  case DYLD_IMAGE_ADDED:
    dyld_process_image_event (info, event);
    break;
  case DYLD_MODULE_BOUND:
    dyld_debug ("DYLD module bound: 0x%08lx -- 0x%08lx -- %4u\n", 
		 (unsigned long) event->arg[0].header, 
		 (unsigned long) event->arg[0].vmaddr_slide, 
		 (unsigned int) event->arg[0].module_index); 
    break;
  case DYLD_MODULE_REMOVED:
    dyld_debug ("DYLD module removed.\n");
    break;
  case DYLD_MODULE_REPLACED:
    dyld_debug ("DYLD module replaced.\n");
    break;
  case DYLD_PAST_EVENTS_END:
    dyld_debug ("DYLD past events end.\n");
    break;
  default:
    dyld_debug ("Unknown DYLD event type 0x%x; ignoring\n", (unsigned int) event->type);
  }
}

void dyld_purge_cached_libraries (struct dyld_objfile_info *info)
{
  unsigned int i;
  assert (info != NULL);

  for (i = 0; i < info->nents; i++) {
    struct dyld_objfile_entry *e = &info->entries[i];
    if (! e->allocated) { continue; }
    if (e->reason == dyld_reason_cached) {
      e->load_flag = 0;
      dyld_remove_objfile (e);
      dyld_objfile_entry_clear (e);
    }
  }

  dyld_objfile_info_pack (info);
  dyld_merge_section_tables (info);
  dyld_update_section_tables (info, &current_target);
  dyld_update_section_tables (info, &exec_ops);

  reread_symbols ();
  breakpoint_re_set ();
}

void
_initialize_nextstep_nat_dyld_process ()
{
  struct cmd_list_element *cmd = NULL;
  char *temp = NULL;

  cmd = add_set_cmd ("dyld-load-dyld-symbols", class_obscure, var_boolean, 
		     (char *) &dyld_load_dyld_symbols_flag,
		     "Set if GDB should load symbol information for the dynamic linker.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-load-shlib-symbols", class_obscure, var_boolean, 
		     (char *) &dyld_load_shlib_symbols_flag,
		     "Set if GDB should load symbol information for DYLD-based shared libraries.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-symbols-prefix", class_obscure, var_string, 
		     (char *) &dyld_symbols_prefix,
		     "Set the prefix that GDB should prepend to all symbols for the dynamic linker.",
		     &setlist);
  add_show_from_set (cmd, &showlist);
  temp = xmalloc (strlen (dyld_symbols_prefix) + 1);
  strcpy (temp, dyld_symbols_prefix);
  dyld_symbols_prefix = temp;

  cmd = add_set_cmd ("dyld-always-resolve-filenames", class_obscure, var_boolean, 
		     (char *) &dyld_always_resolve_filenames_flag,
		     "Set if GDB should use the DYLD interface to determine the names of loaded images.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-allow-resolve-filenames", class_obscure, var_boolean, 
		     (char *) &dyld_allow_resolve_filenames_flag,
		     "Set if GDB should use the DYLD interface to determine the names of loaded images.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-always-read-from-memory", class_obscure, var_boolean, 
		     (char *) &dyld_always_read_from_memory_flag,
		     "Set if GDB should always read loaded images from the inferior's memory.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-remove-overlapping-basenames", class_obscure, var_boolean, 
		     (char *) &dyld_remove_overlapping_basenames_flag,
		     "Set if GDB should remove shared library images with similar basenames.",
		     &setlist);
  add_show_from_set (cmd, &showlist);
}
