/* Declarations for cmpt.
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


/* $Id: cmpt.h,v 1.1.1.1 1999/04/23 02:05:55 wsanchez Exp $ */

#ifndef CMPT_H
#define CMPT_H

#ifndef HAVE_STRCASECMP
int strcasecmp();
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp();
#endif
#ifndef HAVE_STRSTR
char *strstr();
#endif
#ifndef HAVE_STRPTIME
char *strptime();
#endif

#endif /* CMPT_H */
