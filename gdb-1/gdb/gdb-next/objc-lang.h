/* Objective C language support definitions for GDB, the GNU debugger.
   Copyright 1992 Free Software Foundation, Inc.

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

#ifdef __STDC__		/* Forward decls for prototypes */
struct value;
#endif

extern int
objc_parse PARAMS ((void));	/* Defined in c-exp.y */

extern void
objc_error PARAMS ((char *));	/* Defined in c-exp.y */

extern void			/* Defined in c-typeprint.c */
c_print_type PARAMS ((struct type *, char *, GDB_FILE *, int, int));

extern int
c_val_print PARAMS ((struct type *, char *, int, CORE_ADDR, GDB_FILE *, int, int,
		     int, enum val_prettyprint));

extern int
c_value_print PARAMS ((struct value *, GDB_FILE *, int, 
		       enum val_prettyprint));

extern CORE_ADDR lookup_objc_class     PARAMS ((char *classname));
extern int       lookup_child_selector PARAMS ((char *methodname));

char *objc_demangle PARAMS ((const char *mangled));
char *is_objc_demangled PARAMS ((char *name));

int find_objc_msgcall (CORE_ADDR pc, CORE_ADDR *new_pc);
