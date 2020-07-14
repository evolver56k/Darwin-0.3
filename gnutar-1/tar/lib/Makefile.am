# Makefile for GNU tar library.
# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AUTOMAKE_OPTIONS = gnits

noinst_LIBRARIES = libtar.a

EXTRA_DIST = \
alloca.c fileblocks.c fnmatch.c ftruncate.c execlp.c gmalloc.c \
memset.c mkdir.c modechange.h rename.c rmdir.c stpcpy.c strstr.c

noinst_HEADERS = \
argmatch.h backupfile.h error.h fnmatch.h getopt.h getdate.h getpagesize.h \
pathmax.h

libtar_a_SOURCES = \
argmatch.c backupfile.c error.c getdate.y getopt.c getopt1.c getversion.c \
modechange.c msleep.c xgetcwd.c xmalloc.c xstrdup.c

INCLUDES = -I.. -I$(srcdir) -I../intl

libtar_a_LIBADD = @ALLOCA@ @LIBOBJS@
libtar_a_DEPENDENCIES = $(libtar_a_LIBADD)

# Say $(srcdir), so GNU make does not report an ambiguity with the .y.c rule.
$(srcdir)/getdate.c: getdate.y
	@echo Expect 13 shift/reduce conflicts...
	cd $(srcdir) && \
	  $(YACC) $(YFLAGS) getdate.y; \
	  mv -f y.tab.c getdate.c
