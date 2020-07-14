## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS	= 1.1 foreign

## Directory definitions

docdir			= $(prefix)/doc
pkgdocdir		= $(docdir)/@PACKAGE@
copyingfile		= $(pkgdocdir)/copyright

pkgincludedir		= $(includedir)/@PACKAGE@
pkglocalstatedir	= $(localstatedir)/@PACKAGE@
pkgsysconfdir		= $(sysconfdir)/@PACKAGE@

methods_suffix		= methods
pinfo_suffix		= info
pupdates_suffix		= updates
parts_suffix		= parts

## Various options

CFLAGS			= @CFLAGS@ @CWARNS@ -g $(XCFLAGS)
OPTCFLAGS		= @CFLAGS@ @OPTCFLAGS@ @CWARNS@ -g $(XCFLAGS)
LDFLAGS			= @LDFLAGS@ $(XLDFLAGS)
LIBS			= @LIBS@ $(XLIBS)

CXXFLAGS		= @CXXFLAGS@ @CWARNS@ -g $(XCXXFLAGS)
OPTCXXFLAGS		= @CXXFLAGS@ @OPTCFLAGS@ @CWARNS@ -g $(XCXXFLAGS)

INCLUDES		= -I. -I$(srcdir) -I$(top_builddir) \
			  -I$(top_srcdir)/intl  -I$(top_builddir)/intl
DEFS			= @DEFS@

## Automake variables

pkginclude_HEADERS	= dpkg.h dpkg-db.h tarfn.h
noinst_HEADERS		= myopt.h

lib_LTLIBRARIES		= libdpkg.la
noinst_OBJECTS		= myopt.o

libdpkg_la_SOURCES	= ehandle.c mlib.c parse.c parsehelp.c fields.c dump.c \
			  nfmalloc.c varbuf.c database.c vercmp.c \
			  lock.c dbmodify.c showcright.c arch.c \
			  tarfn.c dpkg-var.c
libdpkg_la_LDFLAGS	= -version-info 0:0:0 -lc

EXTRA_DIST		= myopt.c
BUILT_SOURCES		=
CLEANFILES		= stamp-dpkg-var.c dpkg-var.c

all: myopt.o

MAINTAINERCLEANFILES	= $(srcdir)/Makefile.in $(srcdir)/stamp-h.in

## Rules

dpkg-var.c: stamp-dpkg-var.c
stamp-dpkg-var.c: dpkg-var.c.in Makefile
	sed	' \
		s:\([ 	]*[^A-Z]ADMINDIR[ 	]*=[ 	]*"\).*\("\):\1$(pkglocalstatedir)\2:; \
		s:\([ 	]*[^A-Z]UPDATESDIR[ 	]*=[ 	]*"\).*\("\):\1$(pupdates_suffix)\2:; \
		s:\([ 	]*[^A-Z]INFODIR[ 	]*=[ 	]*"\).*\("\):\1$(pinfo_suffix)\2:; \
		s:\([ 	]*[^A-Z]PARTSDIR[ 	]*=[ 	]*"\).*\("\):\1$(parts_suffix)\2:; \
		s:\([ 	]*[^A-Z]LIBDIR[ 	]*=[ 	]*"\).*\("\):\1$(pkglibdir)\2:; \
		s:\([ 	]*[^A-Z]METHODSDIR[ 	]*=[ 	]*"\).*\("\):\1$(methods_suffix)\2:; \
		s:\([ 	]*[^A-Z]COPYINGFILE[ 	]*=[ 	]*"\).*\("\):\1$(copyingfile)\2:; \
		' \
		< $< > dpkg-var.c.new
	(cmp -s dpkg-var.c.new dpkg-var.c) || mv -f dpkg-var.c.new dpkg-var.c
	touch $@

## End of file.
