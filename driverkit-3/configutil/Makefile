#
# Generic Makefile for simple commands
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#
# Who and where
#
# PROGRAM -- what the executable should be named
# BINDIR -- the directory where the executable should be installed
# DSTDIRS -- a list of directories that must exist at install time
#		(preceed all entries with $(DSTROOT)

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

PROGRAM= configutil
BINDIR= /usr/local/bin
CONFDIR=
DSTDIRS= $(DSTROOT)$(BINDIR) $(DSTROOT)$(CONFDIR)

#
# Source files
#
HFILES=
YFILES=
CFILES= $(PROGRAM).c 
SFILES=

#
# Garbage is removed from source directory by "clobber"
#
GARBAGE= errs [Mm]ake.out *~ #*# *.o *.d TAGS tags vers.c Makedep

#
# What to print and how to print it
# Paths are relative to PROGDIR.
#
PRINTFILES = $(HFILES) $(YFILES) $(CFILES) $(SFILES)
TABSIZE=8
ENSCRIPTFLAGS=-2rG

#
# OBJROOT and SYMROOT may optionally be defined on command line
# Used by release control to place objects and executable in particular
# directories.
#
OBJROOT=.
SYMROOT=.

#
# Global options that apply to all types of builds
#
CFLAGS= -I$(DSTROOT)/$(NEXT_ROOT)$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Headers -I../../.. \
	-I$(DSTROOT)/$(NEXT_ROOT)$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/PrivateHeaders
SFLAGS= 
LDFLAGS=
LIBS=-lcs -lDriver -L$(SYMROOT)/../libDriver/syms
IFLAGS= -u -s

#
# Options for release builds
#
STD_COPTS= -O2 -g
STD_LDOPTS=
STD_LIBS=

#
# Options for profiling builds
#
PROF_COPTS= -pg -O2
PROF_LDOPTS= -pg
PROF_LIBS=

#
# Options for debugging builds
#
DBG_COPTS= -g -DDEBUG
DBG_LDOPTS=
DBG_LIBS=

#
# Objects that can't be built by the default rules can have the
# appropriate rules placed at the end of this file.
# EXTRA_SRCS and EXTRA_OBJS should list these source and object files
# if for some reason they could not be listed in HFILES, CFILES, YFILES, ....
# (E.g.: the file isn't a C, yacc, or assembler source.)
#
EXTRA_SRCS=
EXTRA_OBJS=

#
# Include file for simple makefiles
# Note: this is included here, rather than at the bottom of the file
# for a reason: You want all macro defs to proceed all "rules".
# Include MakeInc.simple here insures that any macros needed by the
# both the rules in MakeInc.simple and those that follow in this file
# will be defined.
#
findfile MakeInc.simple

#
# Extra install operations go here
#
install_extra:

installhdrs:

#
# This makefile will recursive invoke itself after cd'ing to OBJROOT.
# When invoked in the OBJROOT will have the following macros defined:
#	SRCDIR -- path to the directory containing this Makefile
#	SYMROOT -- path to the directory that is to contain the result
#		of this build
#
# E.g.
# foo.o:	foo.c foo.h
#	$(CC) -MD $(CFLAGS) $(COPTS) $(CCFLAGS) -WEIRD_OPT -c \
#		-o $@ $(SRCDIR)/foo.c
#	md -d -u Makedep $(*F).d
#
