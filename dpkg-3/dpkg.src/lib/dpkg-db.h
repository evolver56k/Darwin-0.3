/*
 * libdpkg - Debian packaging suite library routines
 * dpkg-db.h - declarations for in-core package database management
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

#ifndef DPKG_DB_H
#define DPKG_DB_H

#include <stdio.h>
#include <stdlib.h>

#include "varbuf.h"

struct versionrevision {
  unsigned long epoch;
  const char *version;
  const char *revision;
};  

enum deptype {
  dep_suggests,
  dep_recommends,
  dep_depends,
  dep_predepends,
  dep_conflicts,
  dep_provides,
  dep_replaces
};

enum depverrel {
  dvrf_earlier =      0x0001,
  dvrf_later =        0x0002,
  dvrf_strict =       0x0008,
  dvrf_orequal =      0x0010,
  dvrf_builtup =      0x0040,
  dvr_none =          0x0080,
  dvr_earlierequal =  dvrf_builtup | dvrf_earlier | dvrf_orequal,
  dvr_earlierstrict = dvrf_builtup | dvrf_earlier | dvrf_strict,
  dvr_laterequal =    dvrf_builtup | dvrf_later   | dvrf_orequal,
  dvr_laterstrict =   dvrf_builtup | dvrf_later   | dvrf_strict,
  dvr_exact =         0x0100
};

struct dependency {
  struct pkginfo *up;
  struct dependency *next;
  struct deppossi *list;
  enum deptype type;
};

struct deppossi {
  struct dependency *up;
  struct pkginfo *ed;
  struct deppossi *next, *nextrev, *backrev;
  struct versionrevision version;
  enum depverrel verrel;
  int cyclebreak;
};

struct arbitraryfield {
  struct arbitraryfield *next;
  char *name;
  char *value;
};

struct conffile {
  struct conffile *next;
  char *name;
  char *hash;
};

struct filedetails {
  struct filedetails *next;
  char *name;
  char *msdosname;
  char *size;
  char *md5sum;
};

struct pkginfoperfile {
  int valid;
  struct dependency *depends;
  struct deppossi *depended;
  int essential; /* The `essential' flag, 1=yes, 0=no (absent) */
  char *description, *maintainer, *source, *architecture, *installedsize;
  struct versionrevision version;
  struct conffile *conffiles;
  struct arbitraryfield *arbs;
};

/* used by dselect only, but we keep a pointer here */

struct perpackagestate;

struct pkginfo {
  struct pkginfo *next;
  char *name;
  enum pkgwant {
    want_unknown, want_install, want_hold, want_deinstall, want_purge,
    want_sentinel /* Not allowed except as special sentinel value
                     in some places */
  } want;
  enum pkgeflag {
    eflagf_reinstreq    = 01,
    eflagf_obsoletehold = 02,
    eflagv_ok           = 0,
    eflagv_reinstreq    =    eflagf_reinstreq,
    eflagv_obsoletehold =                       eflagf_obsoletehold,
    eflagv_obsoleteboth =    eflagf_reinstreq | eflagf_obsoletehold
  } eflag; /* bitmask, but obsoletehold no longer used except when reading */
  enum pkgstatus {
    stat_notinstalled, stat_unpacked, stat_halfconfigured,
    stat_installed, stat_halfinstalled, stat_configfiles
  } status;
  enum pkgpriority {
    pri_required, pri_important, pri_standard, pri_recommended,
    pri_optional, pri_extra, pri_contrib,
    pri_other, pri_unknown, pri_unset=-1
  } priority;
  char *otherpriority;
  char *section;
  struct versionrevision configversion;
  struct filedetails *files;
  struct pkginfoperfile installed;
  struct pkginfoperfile available;
  struct perpackagestate *clientdata;
};

#endif /* DPKG_DB_H */
