#include "nextstep-nat-dyld-process.h"

#include "nextstep-nat-dyld-info.h"
#include "nextstep-nat-dyld-path.h"
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

struct symbol_file_info {
  bfd *abfd;
  int from_tty;
  CORE_ADDR addr;
  int addrisoffset;
  int mainline;
  int mapped;
  CORE_ADDR mapaddr;
  int readnow;
  int user_loaded;
  int is_solib;
  const char *prefix;
  struct objfile *result;
};  

int symbol_file_add_bfd_helper (char *v)
{
  struct symbol_file_info *s = (struct symbol_file_info *) v;
  s->result = symbol_file_add_bfd
    (s->abfd, s->from_tty, s->addr, s->addrisoffset, s->mainline, s->mapped, s->mapaddr, s->readnow, s->user_loaded, s->is_solib, s->prefix);
  return 1;
}

struct objfile *symbol_file_add_bfd_safe
(bfd *abfd, int from_tty, CORE_ADDR addr, 
 int addrisoffset, int mainline, int mapped, CORE_ADDR mapaddr,
 int readnow, int user_loaded, int is_solib, const char *prefix)
{
  struct symbol_file_info s;
  int ret;

  s.abfd = abfd;
  s.from_tty = from_tty;
  s.addr = addr;
  s.addrisoffset = addrisoffset;
  s.mainline = mainline;
  s.mapped = mapped;
  s.mapaddr = mapaddr;
  s.readnow = readnow;
  s.user_loaded = user_loaded;
  s.is_solib = is_solib;
  s.prefix = prefix;
  s.result = NULL;

  ret = catch_errors
    (symbol_file_add_bfd_helper, &s, "unable to load symbol file: ", RETURN_MASK_ALL);
  
  return s.result;
}

struct bfd_file_info {
  const char *filename;
  bfd *result;
};  

int symfile_bfd_open_helper
(char *v)
{
  struct bfd_file_info *s = (struct bfd_file_info *) v;
  s->result = symfile_bfd_open (s->filename);
  return 1;
}

bfd *symfile_bfd_open_safe
(const char *filename)
{
  struct bfd_file_info s;
  int ret;

  s.filename = filename;
  s.result = NULL;

  ret = catch_errors
    (symfile_bfd_open_helper, &s, "unable to open symbol file: ", RETURN_MASK_ALL);

  return s.result;
}

struct dyld_map_info {
  bfd_vma addr;
  bfd_vma offset;
};

bfd_size_type dyld_map_read
(PTR iodata, PTR data, bfd_size_type size, bfd_size_type nitems, bfd *abfd, bfd_vma where)
{
  struct dyld_map_info *iptr = (struct dyld_map_info *) iodata;
  unsigned int i;
  int ret;

  assert (iptr != NULL);
  
  if (strcmp (current_target.to_shortname, "next-child") != 0) {
    bfd_set_error (bfd_error_no_contents);
    return 0;
  }

  if (abfd->tdata.any == NULL) {
    bfd_vma infaddr = iptr->addr + where;
    ret = current_target.to_xfer_memory
      (infaddr, data, (size * nitems), 0, &current_target);
    if (ret <= 0) {
      bfd_set_error (bfd_error_system_call);
      return 0;
    }
    return ret;
  }
  
  if ((strcmp (bfd_get_target (abfd), "mach-o-be") != 0)
      && (strcmp (bfd_get_target (abfd), "mach-o-le") != 0)
      && (strcmp (bfd_get_target (abfd), "mach-o") != 0)) {
    bfd_set_error (bfd_error_invalid_target);
    return 0;
  }
  
  {
    struct mach_o_data_struct *mdata = NULL;
    assert (bfd_mach_o_valid (abfd));
    mdata = abfd->tdata.mach_o_data;
    for (i = 0; i < mdata->header.ncmds; i++) {
      struct bfd_mach_o_load_command *cmd = &mdata->commands[i];
      if (cmd->type == BFD_MACH_O_LC_SEGMENT) {
	struct bfd_mach_o_segment_command *segment = &cmd->command.segment;
	if ((where >= segment->fileoff)
	    && (where < (segment->fileoff + segment->filesize))) {
	  bfd_vma infaddr = (segment->vmaddr + iptr->offset + (where - segment->fileoff));
	  ret = current_target.to_xfer_memory
	    (infaddr, data, (size * nitems), 0, &current_target);
	  if (ret <= 0) {
	    bfd_set_error (bfd_error_system_call);
	    return 0;
	  }
	  return ret;
	}
      }
    }
  }

  bfd_set_error (bfd_error_no_contents);
  return 0;
}

bfd_size_type dyld_map_write
(PTR iodata, const PTR data, 
 bfd_size_type size, bfd_size_type nitems,
 bfd *abfd, bfd_vma where)
{
  error ("unable to write to in-memory images");
}

int dyld_map_flush (PTR iodata, bfd *abfd)
{
  return 0;
}

boolean dyld_map_close (PTR iodata, bfd *abfd)
{
  free (iodata);
  return 1;
}

bfd_vma extend_vma (unsigned long n)
{
  return (- ((bfd_vma) (- ((long) n))));
}

bfd *dyld_map_image
(CORE_ADDR addr, CORE_ADDR offset)
{
  struct dyld_map_info *iptr = NULL;
  struct bfd_io_functions fdata;
  char *filename = NULL;
  bfd *ret = NULL;
  int iret = 0;

  iptr = (struct dyld_map_info *) xmalloc (sizeof (struct dyld_map_info));
  iptr->addr = addr;
  iptr->offset = extend_vma (offset);

  fdata.iodata = iptr;
  fdata.read_func = &dyld_map_read;
  fdata.write_func = &dyld_map_write;
  fdata.flush_func = &dyld_map_flush;
  fdata.close_func = &dyld_map_close;

  iret = asprintf (&filename, "[memory at 0x%lx]", (unsigned long) addr);
  if (iret == 0) {
    warning ("unable to allocate memory for filename for memory region at 0x%lx", (unsigned long) addr);
    return NULL;
  }

  ret = bfd_funopenr (filename, NULL, &fdata);
  if (ret == NULL) { 
    warning ("Unable to open memory image for address 0x%lx; skipping", addr);
    return NULL;
  }
  
  if (bfd_check_format (ret, bfd_archive))
    {
      bfd *abfd = NULL;
#if defined (__ppc__)
      const bfd_arch_info_type *thisarch = bfd_lookup_arch (bfd_arch_powerpc, 0);
#elif defined (__i386__)
      const bfd_arch_info_type *thisarch = bfd_lookup_arch (bfd_arch_i386, 0);
#else
      const bfd_arch_info_type *thisarch = bfd_lookup_arch (bfd_arch_powerpc, 0);
#endif
      for (;;) {
	abfd = bfd_openr_next_archived_file (ret, abfd);
	if (abfd == NULL) { break; }
	if (! bfd_check_format (abfd, bfd_object)) { abfd = NULL; break; }
	if (thisarch == NULL) { abfd = NULL; break; }

	if (bfd_default_compatible (bfd_get_arch_info (abfd), thisarch)) { break; } 
      }
      if (abfd != NULL) { 
	ret = abfd;
      }
    }

  /* FIXME: should check for errors from bfd_close (for one thing, on
     error it does not free all the storage associated with the bfd).  */

  if (! bfd_check_format (ret, bfd_object)) {
    warning ("Unable to read symbols from %s: %s.", bfd_get_filename (ret), bfd_errmsg (bfd_get_error ()));
    bfd_close (ret);
    return NULL;
  }

  if ((strcmp (bfd_get_target (ret), "mach-o-be") != 0)
      && (strcmp (bfd_get_target (ret), "mach-o-le") != 0)
      && (strcmp (bfd_get_target (ret), "mach-o") != 0)) {
    warning ("Unable to read symbols from %s: invalid file format \"%s\".",
	     bfd_get_filename (ret), bfd_get_target (ret));
    bfd_close (ret);
    return NULL;
  }
  
  return ret;
}
