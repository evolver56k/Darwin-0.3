## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS	= 1.1 foreign

## Directory definitions

localedir		= $(datadir)/locale

## Various options

CFLAGS			= @CFLAGS@ @CWARNS@ -g $(XCFLAGS)
OPTCFLAGS		= @CFLAGS@ @OPTCFLAGS@ @CWARNS@ -g $(XCFLAGS)
LDFLAGS			= @LDFLAGS@ $(XLDFLAGS)
LIBS			= @INTLLIBS@ @LIBS@ $(XLIBS)

CXXFLAGS		= @CXXFLAGS@ @CWARNS@ -g $(XCXXFLAGS)
OPTCXXFLAGS		= @CXXFLAGS@ @OPTCFLAGS@ @CWARNS@ -g $(XCXXFLAGS)

INCLUDES		= -I. -I$(srcdir) -I$(top_builddir) -I$(top_srcdir)/lib \
			  -I$(top_srcdir)/intl  -I$(top_builddir)/intl
DEFS			= @DEFS@ -DLOCALEDIR=\"$(localedir)\"

## Automake variables

bin_PROGRAMS		= dpkg
man_MANS		= dpkg.8

dpkg_SOURCES		= main.c enquiry.c filesdb.c archives.c processarc.c \
			  cleanup.c select.c packages.c configure.c remove.c \
			  help.c depcon.c errors.c update.c main.h filesdb.h \
			  archives.h dpkg-int.h

dpkg_LDADD		= ../lib/libdpkg.la ../libiberty/libiberty.a ../lib/myopt.o

DISTCLEANFILES		= updates status available *.old
MAINTAINERCLEANFILES	= $(srcdir)/Makefile.in

EXTRA_DIST		= dpkg.8

## Rules

filesdb.o: filesdb.c
	$(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(OPTCFLAGS) -c $<

## End of file.
