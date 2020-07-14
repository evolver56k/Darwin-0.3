/*
 * dpkg-deb - construction and deconstruction of *.deb archives
 * dpkg-deb.h - external definitions for this program
 *
 * Copyright (C) 1994,1995 Ian Jackson <iwj10@cus.cam.ac.uk>
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

#ifndef DPKG_DEB_H
#define DPKG_DEB_H

#include "config.h"

typedef void dofunction(const char *const *argv);
dofunction do_build, do_contents, do_control;
dofunction do_info, do_field, do_extract, do_vextract, do_fsystarfile;

extern int debugflag, nocheckflag, oldformatflag;
extern const struct cmdinfo *cipaction;
extern dofunction *action;

void extracthalf(const char *debar, const char *directory,
                 const char *taroption, int admininfo);

#define DEBMAGIC     "!<arch>\ndebian-binary   "
#define ADMINMEMBER  "control.tar.gz  "
#define DATAMEMBER   "data.tar.gz     "

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(category, locale)
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(text) gettext (text)
# define G_(text) (text)
# define N_(text) (text)
#else
# define bindtextdomain(domain, directory)
# define textdomain(domain)
# define _(text) (text)
# define G_(text) (text)
# define N_(text) (text)
#endif

#define internerr(s) do_internerr (s,__LINE__,__FILE__)

#ifndef S_ISLNK
# define S_ISLNK(mode) ((mode&0xF000) == S_IFLNK)
#endif

#endif /* DPKG_DEB_H */
