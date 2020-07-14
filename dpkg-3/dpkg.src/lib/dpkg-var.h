/*
 * libdpkg - Debian packaging suite library routines
 * dpkg.h - general header for Debian package handling
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

#ifndef DPKG_VAR_H
#define DPKG_VAR_H

#include <stdlib.h>

extern const char *const ARCHIVEVERSION;
extern const char *const SPLITVERSION;
extern const char *const OLDARCHIVEVERSION;

enum { SPLITPARTDEFMAX = 450 * 1024 };
enum { MAXFIELDNAME = 200 };
enum { MAXCONFFILENAME = 1000 };
enum { MAXDIVERTFILENAME = 1024 };
enum { MAXCONTROLFILENAME = 100 };

extern const char *const BUILDCONTROLDIR;
extern const char *const EXTRACTCONTROLDIR;
extern const char *const DEBEXT;
extern const char *const OLDDBEXT;
extern const char *const NEWDBEXT;
extern const char *const OLDOLDDEBDIR;
extern const char *const OLDDEBDIR;

/* extern const char *const ARCHBINFMT; */

extern const char *const DPKG_VERSION_ARCH;

extern const char *const NEWCONFFILEFLAG;
extern const char *const NONEXISTENTFLAG;

extern const char *const DPKGTEMPEXT;
extern const char *const DPKGNEWEXT;
extern const char *const DPKGOLDEXT;
extern const char *const DPKGDISTEXT;

extern const char *const REMOVECONFFEXTS[];

extern const char *const CONTROLFILE;
extern const char *const CONFFILESFILE;
extern const char *const PREINSTFILE;
extern const char *const POSTINSTFILE;
extern const char *const PRERMFILE;
extern const char *const POSTRMFILE;
extern const char *const LISTFILE;

extern const char *const ADMINDIR;
extern const char *const STATUSFILE;
extern const char *const AVAILFILE;
extern const char *const LOCKFILE;
extern const char *const CMETHOPTFILE;
extern const char *const METHLOCKFILE;
extern const char *const DIVERSIONSFILE;
extern const char *const UPDATESDIR;
extern const char *const INFODIR;
extern const char *const PARTSDIR;

extern const char *const CONTROLDIRTMP;
extern const char *const IMPORTANTTMP;
extern const char *const REASSEMBLETMP;
extern const size_t IMPORTANTMAXLEN;

extern const char *const IMPORTANTFMT;
extern const size_t MAXUPDATES;

extern const char *const LIBDIR;
extern const char *const LOCALLIBDIR;
extern const char *const METHODSDIR;

extern const char *const COPYINGFILE;

extern const char *const NOJOBCTRLSTOPENV;
extern const char *const SHELLENV;
extern const char *const DEFAULTSHELL;

enum { IMETHODMAXLEN = 50 };
enum { IOPTIONMAXLEN = 50 };

extern const char *const METHODOPTIONSFILE;
extern const char *const METHODSETUPSCRIPT;
extern const char *const METHODUPDATESCRIPT;
extern const char *const METHODINSTALLSCRIPT;
extern const char *const OPTIONSDESCPFX;

enum { OPTIONINDEXMAXLEN = 5 };

enum { PKGSCRIPTMAXARGS = 10 };
enum { MD5HASHLEN = 32 };

extern const char *const ARCHIVE_FILENAME_PATTERN;

extern const char *const BACKEND;
extern const char *const SPLITTER;
extern const char *const MD5SUM;
extern const char *const DSELECT;
extern const char *const DPKG;

extern const char *const TAR;
extern const char *const GZIP;
extern const char *const CAT;
extern const char *const RM;
extern const char *const FIND;
extern const char *const SHELL;

extern const char *const SHELLENVIR;

extern const char *const FIND_EXPRSTARTCHARS;

enum { TARBLKSZ = 512 };

#endif /* DPKG_VAR_H */
