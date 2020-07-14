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

int scatterload_hack = 0;

void
_initialize_nextstep_xdep ()
{
  struct cmd_list_element *cmd;

  cmd = add_set_cmd ("scatterload-hack", class_obscure, var_boolean, 
		     (char *) &scatterload_hack,
		     "Set if GDB should include hack for scatterloaded executables",
		     &setlist);
  add_show_from_set (cmd, &showlist);		
}
