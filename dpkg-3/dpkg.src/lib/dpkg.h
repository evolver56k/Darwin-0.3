/*
 * libdpkg - Debian packaging suite library routines
 * dpkg.h - general header for Debian package handling
 *
 * Copyright (C) 1994, 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 * Copyright (C) 1997, 1998 Klee Dienes <klee@debian.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef DPKG_H
#define DPKG_H

struct cmdinfo;

extern const char *thisname;
extern const char *const printforhelp;

#include "dpkg-db.h"

#include "arch.h"
#include "database.h"
#include "dbmodify.h"
#include "dpkg-db.h"
#include "dpkg-var.h"
#include "dpkg.h"
#include "dump.h"
#include "ehandle.h"
#include "fnmatch.h"
#include "lock.h"
#include "mlib.h"
#include "myopt.h"
#include "nfmalloc.h"
#include "parse.h"
#include "parsedump.h"
#include "parsehelp.h"
#include "tarfn.h"
#include "varbuf.h"
#include "vercmp.h"

void showcopyright (const struct cmdinfo*, const char*);

#endif /* DPKG_H */
