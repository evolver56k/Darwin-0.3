/* Declarations for main module.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


/* $Id: main.h,v 1.1.1.1 1999/04/23 02:05:56 wsanchez Exp $ */

#ifndef MAIN_H
#define MAIN_H

/* Function declarations */

#ifdef HAVE_SIGNAL
RETSIGTYPE hangup PARAMS((int));
#endif /* WINDOWS */

int main PARAMS((int, char * const *));
void printhelp PARAMS((void));

#endif /* MAIN_H */
