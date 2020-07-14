/* This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "symfile.h"
#include "symtab.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "language.h"

#include "libaout.h"	 	/* FIXME Secret internal BFD stuff for a.out */
#include "aout/aout64.h"
#include "complaints.h"

#include "mach-o.h"
#include "objc-lang.h"

#include <assert.h>
#include <sys/ttycom.h>

struct complaint unknown_macho_symtype_complaint =
  {"unknown Mach-O symbol type %s", 0, 0};

struct complaint unknown_macho_section_complaint =
  {"unknown Mach-O section value %s (assuming DATA)", 0, 0};

struct complaint unsupported_indirect_symtype_complaint =
  {"unsupported Mach-O symbol type %s (indirect)", 0, 0};

#define BFD_GETB16(addr) ((addr[0] << 8) | addr[1])
#define BFD_GETB32(addr) ((((((unsigned long) addr[0] << 8) | addr[1]) << 8) | addr[2]) << 8 | addr[3])
#define BFD_GETL16(addr) ((addr[1] << 8) | addr[0])
#define BFD_GETL32(addr) ((((((unsigned long) addr[3] << 8) | addr[2]) << 8) | addr[1]) << 8 | addr[0])

unsigned char next_symbol_types[256];

static unsigned char next_symbol_type_base (macho_type)
  unsigned char macho_type;
{
  unsigned char mtype = macho_type;
  unsigned char ntype = 0;
  
  if (macho_type & BFD_MACH_O_N_STAB) {
    return macho_type;
  }

  if (mtype & BFD_MACH_O_N_PEXT) {
    mtype &= ~BFD_MACH_O_N_PEXT;
    ntype |= N_EXT;
  }

  if (mtype & BFD_MACH_O_N_EXT) {
    mtype &= ~BFD_MACH_O_N_EXT;
    ntype |= N_EXT;
  }

  switch (mtype & BFD_MACH_O_N_TYPE) {
  case BFD_MACH_O_N_SECT:
    /* should add section here */
    break;

  case BFD_MACH_O_N_PBUD:
    ntype |= N_UNDF;
    break;

  case BFD_MACH_O_N_ABS:
    ntype |= N_ABS;
    break;

  case BFD_MACH_O_N_UNDF:
    ntype |= N_UNDF;
    break;

  case BFD_MACH_O_N_INDR:
    complain (&unsupported_indirect_symtype_complaint, local_hex_string (macho_type));
    return macho_type;

  default:
    complain (&unknown_macho_symtype_complaint, local_hex_string (macho_type));
    return macho_type;
  }
  mtype &= ~BFD_MACH_O_N_TYPE;

  assert (mtype == 0);

  return ntype;
}

static void next_symbol_types_init ()
{
  unsigned int i;
  for (i = 0; i < 256; i++) {
    next_symbol_types[i] = next_symbol_type_base (i);
  }
}

static unsigned char next_symbol_type (macho_type, macho_other)
     unsigned char macho_type;
     unsigned char macho_other;
{
  unsigned char ntype = next_symbol_types[macho_type];

  if ((macho_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_SECT) {
    
    if (macho_other == 1) {
      ntype |= N_TEXT;
    } else {
#if 0
      complain (&unknown_macho_section_complaint, local_hex_string (macho_other));
#endif
      ntype |= N_DATA;
    }
  }
  
  return ntype;
}

void next_internalize_symbol (in, ext, abfd)
     struct internal_nlist *in;
     struct external_nlist *ext;
     bfd *abfd;
{
  if (bfd_header_big_endian (abfd)) {
    in->n_strx = BFD_GETB32 (ext->e_strx);
    in->n_desc = BFD_GETB16 (ext->e_desc);
    in->n_value = BFD_GETB32 (ext->e_value);
  } else if (bfd_header_little_endian (abfd)) {
    in->n_strx = BFD_GETL32 (ext->e_strx);
    in->n_desc = BFD_GETL16 (ext->e_desc);
    in->n_value = BFD_GETL32 (ext->e_value);
  } else {
    error ("unable to internalize symbol (unknown endianness)");
  }

  in->n_type = next_symbol_type (ext->e_type[0], ext->e_other[0]);
  in->n_other = 0;
}

static void info_trampoline_command (char *exp, int from_tty)
{
  struct expression *expr;
  struct value *val;
  CORE_ADDR address;
  CORE_ADDR trampoline;
  CORE_ADDR objc;
  asection *next_section;

  expr = parse_expression (exp);
  val = evaluate_expression (expr);
  if (TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_REF)
    val = value_ind (val);
  /* In rvalue contexts, such as this, functions are coerced into
     pointers to functions. */
  if (TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_FUNC
      && VALUE_LVAL (val) == lval_memory)
    address = VALUE_ADDRESS (val);
  else
    address = value_as_pointer (val);
  if (VALUE_BFD_SECTION (val))
    next_section = VALUE_BFD_SECTION (val);

#if defined (TARGET_POWERPC)
  trampoline = ppc_next_skip_trampoline_code (address);
#elif defined (TARGET_I386)
  trampoline = i386_next_skip_trampoline_code (address);
#else
#error unknown architecture
#endif

  find_objc_msgcall (trampoline, &objc);

  fprintf (stderr, "0x%lx becomes 0x%lx becomes 0x%lx\n",
           (unsigned long) address, (unsigned long) trampoline, (unsigned long) objc);
}

void
next_resize_window (int* lines_per_page, int* chars_per_line)
{
    int new_lines_per_page = UINT_MAX;
    int new_chars_per_line = UINT_MAX;

    if (isatty(fileno(stdout)))
      {
        struct winsize window_size;

        if (-1 != ioctl(fileno(stdout), TIOCGWINSZ, &window_size))
          {
//            new_lines_per_page = window_size.ws_row;
            new_chars_per_line = window_size.ws_col;
          }
      }
    *lines_per_page = new_lines_per_page;
    *chars_per_line = new_chars_per_line;
}

void
_initialize_nextstep_tdep ()
{
  next_symbol_types_init ();

  add_info ("trampoline", info_trampoline_command,
	    "Resolve function for DYLD trampoline stub and/or Objective-C call");
}
