/* i386 target support for NeXTStep, for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1989, 1992 Free Software Foundation, Inc.

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

#ifndef TM_I386NEXT_H
#define TM_I386NEXT_H

#include "config/i386/tm-i386.h"
#include "config/tm-nextstep.h"

extern CORE_ADDR i386_next_skip_trampoline_code (CORE_ADDR pc);
#define	SKIP_TRAMPOLINE_CODE(pc) i386_next_skip_trampoline_code (pc)

extern int i386_next_in_solib_call_trampoline (CORE_ADDR pc, char *name);
#define IN_SOLIB_CALL_TRAMPOLINE(pc, name) i386_next_in_solib_call_trampoline (pc, name)

extern int i386_next_in_solib_return_trampoline (CORE_ADDR pc, char *name);
#define IN_SOLIB_RETURN_TRAMPOLINE(pc,name) i386_next_in_solib_return_trampoline (pc, name)

#endif /* TM_I386NEXT_H */
