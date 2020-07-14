## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS	= 1.1 foreign

## Directory definitions

pkglibdir		= $(libdir)/@PACKAGE@
pkglocalstatedir	= $(sharedstatedir)/@PACKAGE@

parsechangelogdir	= $(pkglibdir)/parsechangelog
altssysconfdir		= $(pkgsysconfdir)/alternatives
altslocalstatedir	= $(pkglocalstatedir)/alternatives

# This should be $(pkgsysconfdir), but uses $(sysconfdir) for backwards compatibility.
altssysconfdir		= $(sysconfdir)/alternatives

## Automake variables

INCLUDES		= -I$(top_srcdir)/include -I$(top_srcdir)/libiberty-include \
			  -I$(top_srcdir) -I$(srcdir) \
			  -I$(top_builddir) -I$(top_builddir)/include -I. \
			  -I$(top_srcdir)/intl -I$(top_builddir)/intl
DEFS			= @DEFS@

bin_SCRIPTS		= dpkg-name dpkg-source dpkg-genchanges dpkg-gencontrol \
			  dpkg-shlibdeps dpkg-buildpackage dpkg-parsechangelog \
			  dpkg-distaddfile 822-date dpkg-scanpackages

sbin_PROGRAMS		= start-stop-daemon
start_stop_daemon_SOURCES	= start-stop-daemon.c
start_stop_daemon_LDADD = ../libiberty/libiberty.a
sbin_SCRIPTS		= update-rc.d update-alternatives \
			  install-info dpkg-divert

pkglib_SCRIPTS		= controllib.pl

lisp_LISP		= debian-changelog-mode.el
ELCFILES		= debian-changelog-mode.elc

man_MANS		= dpkg-name.1 dpkg-source.1 822-date.1 update-rc.d.8 \
			  start-stop-daemon.8 update-alternatives.8 \
			  install-info.8 dpkg-scanpackages.8

EXTRA_DIST		= update-rc.d.pl \
			  dpkg-name.sh dpkg-source.pl dpkg-genchanges.pl \
			  dpkg-gencontrol.pl dpkg-shlibdeps.pl \
			  dpkg-buildpackage.sh dpkg-parsechangelog.pl \
			  dpkg-distaddfile.pl 822-date.pl dpkg-scanpackages.pl \
			  update-alternatives.pl install-info.pl dpkg-divert.pl \
			  controllib.pl debian-changelog-mode.el \
			  cl-debian.pl \
			  $(man_MANS)

noinst_SCRIPTS		= cl-debian

LSMANL			= dpkg-gencontrol dpkg-genchanges dpkg-buildpackage \
			  dpkg-distaddfile dpkg-parsechangelog dpkg-shlibdeps

CLEANFILES		= $(bin_SCRIPTS) $(sbin_SCRIPTS) $(noinst_DATA) $(ELCFILES)
MAINTAINERCLEANFILES	= $(srcdir)/Makefile.in $(srcdir)/elisp-comp

## Rules

.PHONY: install-data-local uninstall-local

SUFFIXES = .pl .sh

%: %.pl
	sed -e "s:^#\![ 	]*/usr/bin/perl:#\! $(perlpath):; \
		s:\$$dpkglibdir[ 	]*=[ 	]*['\"][^'\"]*['\"]:\$$dpkglibdir=\"$(pkglibdir)\":; \
		s:\$$version[ 	]*=[ 	]*['\"][^'\"]*['\"]:\$$version=\"$(VERSION)\":" \
		< $< > $@.dest
	sed -e "s:^#\![ 	]*/usr/bin/perl:#\! $(perlpath):; \
		s:\$$dpkglibdir[ 	]*=[ 	]*['\"][^'\"]*['\"]:\$$dpkglibdir=\"./scripts\":; \
		s:\$$version[ 	]*=[ 	]*['\"][^'\"]*['\"]:\$$version=\"$(VERSION)\":" \
		< $< > $@.build
	chmod 755 $@.build $@.dest
	if test "$@" = "cl-debian"; then \
	  (mkdir parsechangelog || true); \
	  cp -p $@.build parsechangelog/debian; \
	  ln -sf $(srcdir)/controllib.pl controllib.pl; \
	fi
	mv $@.dest $@

%: %.sh 
	sed -e "s:version=\"[^\"]*\":version=\"$(VERSION)\":" \
		< $< > $@.dest
	sed -e "s:version=\"[^\"]*\":version=\"$(VERSION)\":" \
		< $< > $@.build
	chmod 755 $@.build $@.dest
	mv $@.build $@

$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign scripts/Makefile
	perl -ni -e \
		'print; if (/p in \$$\$$list\; do/) { print "\t  p=`basename \$$\$$p`; \\\n"; }' \
		$(srcdir)/Makefile.in

install-data-local: cl-debian
	$(mkinstalldirs) $(altslocalstatedir) $(altssysconfdir)
	set -e; for f in $(LSMANL); do ln -sf dpkg-source.1 $(mandir)/man1/$$f.1; done
	$(mkinstalldirs) $(parsechangelogdir)
	$(INSTALL_PROGRAM) cl-debian $(parsechangelogdir)/debian

uninstall-local:
	$(RM) $(parsechangelogdir)/debian
	-[ -e $(parsechangelogdir) ] && rmdir $(parsechangelogdir)
	set -e; for f in $(LSMANL); do $(RM) $(mandir)/man1/$$f.1; done
	-[ -e $(altssysconfdir) ] && rmdir $(altssysconfdir)
	-[ -e $(altslocalstatedir) ] && rmdir $(altslocalstatedir)

## End of file.
