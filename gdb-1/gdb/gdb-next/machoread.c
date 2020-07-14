/* Mach-O symbol-table support for Mach, for GDB.
   Copyright 1997  Free Software Foundation, Inc.
   Written by Klee Dienes.  Contributed by Apple Computer, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "breakpoint.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "buildsym.h"
#include "obstack.h"
#include "gdb-stabs.h"
#include "stabsread.h"

#include "mach-o.h"

#include <string.h>

#include <assert.h>

struct macho_symfile_info {
  asymbol **syms;
  long nsyms;
};

static void
macho_new_init (objfile)
     struct objfile *objfile;
{
}

static void
macho_symfile_init (objfile)
     struct objfile *objfile;
{
  objfile->sym_stab_info = 
    xmmalloc (objfile->md, sizeof (struct dbx_symfile_info));

  memset ((PTR) objfile->sym_stab_info, 0, sizeof (struct dbx_symfile_info));

  objfile->sym_private = 
    xmmalloc (objfile->md,sizeof (struct macho_symfile_info));

  memset (objfile->sym_private, 0, sizeof (struct macho_symfile_info));

  objfile->flags |= OBJF_REORDERED;
  init_entry_point_info (objfile);
}

static void
macho_symfile_read (objfile, section_offsets, mainline)
     struct objfile *objfile;
     struct section_offsets *section_offsets;
     int mainline;
{
  bfd *abfd = objfile->obfd;

  struct bfd_mach_o_load_command *gsymtab, *gdysymtab;

  struct bfd_mach_o_symtab_command *symtab = NULL;
  struct bfd_mach_o_dysymtab_command *dysymtab = NULL;
  struct bfd_mach_o_section *section = NULL;

  int ret;

  assert (section_offsets != NULL);
  assert (objfile != NULL);
  assert (abfd != NULL);
  assert (abfd->filename != NULL);

  stabsect_build_psymtabs (objfile, section_offsets, mainline,
			   "LC_SYMTAB.stabs", "LC_SYMTAB.stabstr",
			   "LC_SEGMENT.__TEXT.__text",
			   "LC_SEGMENT.__DATA.__data",
			   NULL);

  ret = bfd_mach_o_lookup_command (abfd, BFD_MACH_O_LC_SYMTAB, &gsymtab);
  if (ret == 0) { return; }
  if (ret != 1) {
#if 0
    warning ("Error fetching LC_SYMTAB load command from object file \"%s\"",
	     abfd->filename);
#endif
    return;
  }

  ret = bfd_mach_o_lookup_command (abfd, BFD_MACH_O_LC_DYSYMTAB, &gdysymtab);
  if (ret != 1) {
#if 0
    warning ("Error fetching LC_DYSYMTAB load command from object file \"%s\"",
	     abfd->filename);
#endif
    return;
  }

  assert (gsymtab->type == BFD_MACH_O_LC_SYMTAB);
  assert (gdysymtab->type == BFD_MACH_O_LC_DYSYMTAB);

  symtab = &gsymtab->command.symtab;
  dysymtab = &gdysymtab->command.dysymtab;

  {
    asection *bfdsec = NULL;
    struct bfd_mach_o_section *lsection = NULL;
    struct bfd_mach_o_load_command *lcommand = NULL;

    bfdsec = bfd_get_section_by_name (abfd, "LC_SEGMENT.__TEXT.__picsymbol_stub");
    if (bfdsec == NULL) {
#if 0
      warning ("Error fetching __TEXT.__picsymbol_stub section from object file.");
#endif
      return;
    }

    ret = bfd_mach_o_lookup_section (abfd, bfdsec, &lcommand, &lsection);
    if ((ret != 1) || (lcommand != NULL)) {
#if 0
      warning ("Error fetching __TEXT.__picsymbol_stub section from object file.");
#endif
      return;
    }
    assert (lsection != NULL);

    section = lsection;
  }
    
  assert (DBX_STRINGTAB (objfile) != NULL);
  if (symtab->strtab == NULL) {
    symtab->strtab = DBX_STRINGTAB (objfile);
  }

  if (symtab->strtab == NULL) {
    ret = bfd_mach_o_scan_read_symtab_strtab (abfd, symtab);
    if (ret != 0) { 
      warning ("Unable to read symbol table for \"%s\": %s", 
	       abfd->filename, bfd_errmsg (bfd_get_error ()));
      return;
    }
  }

  {
    unsigned long i, nsyms;
    asymbol sym;

    nsyms = section->size / section->reserved2;
    
    init_minimal_symbol_collection ();
    make_cleanup (discard_minimal_symbols, 0);
    
    for (i = 0; i < nsyms; i++) {
      
      unsigned long cursym = section->reserved1 + i;
      CORE_ADDR stubaddr = section->addr + (i * section->reserved2);
      char *sname = NULL;
      char nname[4096];

      if (cursym >= dysymtab->nindirectsyms) {
	warning ("Indirect symbol entry out of range in \"%s\" (%lu >= %lu)",
		 abfd->filename, cursym, (unsigned long) dysymtab->nindirectsyms);
	return;
      } 
      ret = bfd_mach_o_scan_read_dysymtab_symbol (abfd, dysymtab, symtab, &sym, cursym);
      if (ret != 0) { return; }

      sname = sym.name;
      assert (sname != NULL);
      if (sname[0] == bfd_get_symbol_leading_char (abfd)) {
	sname++;
      }

      assert ((strlen (sname) + sizeof ("dyld_stub_") + 1) < 4096);
      sprintf (nname, "dyld_stub_%s", sname);

      stubaddr += ANOFFSET (section_offsets, SECT_OFF_TEXT);
      prim_record_minimal_symbol_and_info
	(nname, stubaddr, mst_solib_trampoline, NULL, SECT_OFF_TEXT, bfd_get_section (&sym), objfile);
    }

    install_minimal_symbols (objfile);
    discard_minimal_symbols (0);
  }
}

static void
macho_symfile_finish (objfile)
     struct objfile *objfile;
{
}

static struct sym_fns macho_sym_fns =
{
  bfd_target_mach_o_flavour,

  macho_new_init,		/* sym_new_init: init anything gbl to entire symtab */
  macho_symfile_init,		/* sym_init: read initial info, setup for sym_read() */
  macho_symfile_read,		/* sym_read: read a symbol file into symtab */
  macho_symfile_finish,		/* sym_finish: finished with file, cleanup */
  default_symfile_offsets,	/* sym_offsets:  xlate external to internal form */
  NULL				/* next: pointer to next struct sym_fns */
};

void
_initialize_machoread ()
{
  add_symtab_fns (&macho_sym_fns);
}
