# Makefile for GNU tar sources.
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

AUTOMAKE_OPTIONS = gnits ansi2knr
bin_PROGRAMS = tar
libexec_PROGRAMS = @RMT@
EXTRA_PROGRAMS = rmt

noinst_HEADERS = arith.h common.h rmt.h system.h tar.h
rmt_SOURCES = rmt.c
tar_SOURCES = arith.c buffer.c compare.c create.c delete.c extract.c \
incremen.c list.c mangle.c misc.c names.c open3.c rtapelib.c tar.c update.c

INCLUDES = -I../intl -I$(top_srcdir)/lib
LDADD = ../lib/libtar.a @INTLLIBS@
localedir = $(prefix)/@DATADIRNAME@/locale

tar.o: tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/tar.c

tar._o: tar._c
	@rm -f _tar.c
	@ln tar._c _tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _tar.c
	@mv _tar.o $@
	@rm _tar.c

rmt.o: rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c $(srcdir)/rmt.c

rmt._o: rmt._c
	@rm -f _rmt.c
	@ln rmt._c _rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c _rmt.c
	@mv _rmt.o $@
	@rm _rmt.c
