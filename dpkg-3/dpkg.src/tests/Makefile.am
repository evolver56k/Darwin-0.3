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

MAINTAINERCLEANFILES	= $(srcdir)/Makefile.in

noinst_PROGRAMS		= star

star_SOURCES		= star.c
star_LDADD 		= ../lib/libdpkg.la

## Rules

star: $(star_OBJECTS) $(star_DEPENDENCIES)
	@rm -f dpkg
	$(LINK) $(star_LDFLAGS) $(star_OBJECTS) $(star_LDADD) $(LIBS)

## End of file.
