## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS	= 1.1 foreign

## Directory definitions

docdir			= $(prefix)/doc
pkgdocdir		= $(docdir)/@PACKAGE@

## Automake variables

man_MANS		= deb.5 deb-old.5 deb-control.5
pkgdoc_DATA		= developer-keys.pgp

HTMLDOCSTAMPS		= internals.html-stamp packaging.html-stamp
HTMLDIRS		= internals.html packaging.html
SGMLFILES		= internals.sgml packaging.sgml
HARDCOPY		= internals.ps internals.dvi packaging.ps

CLEANFILES		= $(srcdir)/manuals-version.new \
			  $(srcdir)/*.aux \
			  $(srcdir)/*.cp \
			  $(srcdir)/*.fn \
			  $(srcdir)/*.fns \
			  $(srcdir)/*.ky \
			  $(srcdir)/*.log \
			  $(srcdir)/*.pg \
			  $(srcdir)/*.toc \
			  $(srcdir)/*.tp \
			  $(srcdir)/*.vr \
			  $(srcdir)/*.sgmls \
			  $(srcdir)/*.internals.texi \
			  $(srcdir)/*.bak

EXTRA_DIST		= database-structure.fig \
			  ChangeLog \
			  manuals-version \
			  developer-keys.pgp \
			  $(SGMLFILES) \
			  $(man_MANS) \
			  texinfo.dtd \
			  texinfo.texi_mapping \
			  texinfo.debian_mapping

DISTCLEANFILES		= 

INFOFILES		= internals.info

## Rules

SUFFIXES		= .html-stamp .sgml .ps .fig .monops

.PHONY: all install-data-local uninstall-local dist-hook clean-local

all:
	if [ -f /usr/bin/debiandoc2ps ]; then \
		$(MAKE) $(HTMLDOCSTAMPS) $(HARDCOPY) $(INFOFILES); \
	fi

$(srcdir)/manuals-version: $(top_srcdir)/debian/changelog
	v=`dpkg-parsechangelog -l$< | sed -n 's/^Version: //p'` && \
	echo "<!entity manuals-version \"$$v\">" >$@.new
	echo "<!entity dpkg-version \"$(VERSION)\">" >>$@.new
	(cmp -s $@.new $@ || mv -f $@.new $@) && rm -f $@.new

$(srcdir)/packaging.html-stamp: packaging.sgml manuals-version
	cd $(srcdir) && rm -rf packaging.thml/*.html
	cd $(srcdir) && debiandoc2html packaging.sgml
	touch $@

$(srcdir)/packaging.ps: packaging.sgml manuals-version
	cd $(srcdir) && debiandoc2ps -pletter packaging.sgml

$(srcdir)/internals.texi: internals.sgml texinfo.dtd texinfo.texi_mapping manuals-version
	cd $(srcdir) && nsgmls internals.sgml > internals.sgmls || ($(RM) internals.sgmls && false)
	cd $(srcdir) && sgmlsasp texinfo.texi_mapping < internals.sgmls > internals.texi || ($(RM) internals.texi && false)
	cd $(srcdir) && perl -pi -e 's/printindex FN/printindex fn/' internals.texi || ($(RM) internals.texi && false)

$(srcdir)/internals.ps: internals.dvi
	cd $(srcdir) && dvips -t letter -o internals.ps internals.dvi

$(srcdir)/internals.dvi: internals.texi
	cd $(srcdir) && texi2dvi internals.texi

$(srcdir)/internals.info: internals.texi
	cd $(srcdir) && makeinfo internals.texi

$(srcdir)/internals.html-stamp: internals.texi
	cd $(srcdir) && $(RM) -r internals.html/*.html
	-cd $(srcdir) && mkdir internals.html
	cd $(srcdir)/internals.html && texi2html -monolithic internals.texi
	cd $(srcdir) && mv internals.html/internals.html internals.html/index.html
	touch $@

$(srcdir)/database-structure.ps: database-structure.fig
	fig2dev -L ps -c -l _ -P < $< > ps
	mv -f ps $@

$(srcdir)/database-structure.mono.ps: database-structure.ps
	perl -pe 's:^/(col[0-7]) \{[01 ]*1[01 ]* setrgbcolor\}\
 bind def$$::/$$1 {} bind def:' $< > ps
	mv -f ps $@

$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign doc/Makefile
	perl -ni -e \
		'print; if (/p in \$$\$$list\; do/) { print "\t  p=`basename \$$\$$p`; \\\n"; }' \
		$(srcdir)/Makefile.in

install-data-local:
	if [ -f /usr/bin/debiandoc2ps ]; then \
		$(MAKE) $(HTMLDOCSTAMPS); \
	fi
	$(mkinstalldirs) $(pkgdocdir)
	$(INSTALL_DATA) $(top_srcdir)/debian/changelog $(pkgdocdir)/changelog
	$(INSTALL_DATA) $(srcdir)/ChangeLog $(pkgdocdir)/changelog.manuals
	set -e; for f in $(HTMLDOCSTAMPS); do \
		g=`echo $$f | sed -e 's/-stamp$$//'`; \
		cp -r $(srcdir)/$$g $(pkgdocdir)/`basename $$g`; \
	done

uninstall-local:
	-$(RM) $(pkgdocdir)/changelog.manuals $(pkgdocdir)/changelog
	-cd $(pkgdocdir) && rm -rf $(HTMLDIRS)

dist-hook:
	if [ -f /usr/bin/debiandoc2ps ]; then \
		$(MAKE) $(HTMLDOCSTAMPS) $(HARDCOPY) internals.texi internals.info; \
	fi
	cp -p $^ $(distdir)
	for f in `echo $^ | tr ' \t' '\n\n' | sed -ne 's:.html-stamp::gp'`; do set -e; \
		$(RM) -r $(distdir)/`basename $$f.html`; \
		mkdir $(distdir)/`basename $$f.html`; \
		cp -p $$f.html/* $(distdir)/`basename $$f.html`; \
	done
	$(RM) -r $(distdir)/obsolete
	cp -rp $(srcdir)/obsolete $(distdir)/
	cp -p $(HARDCOPY) $(distdir)/

maintainer-clean-local: doc-clean
	-$(RM) $(srcdir)/Makefile.in 

doc-clean:
	cd $(srcdir) && $(RM) *.aux *.cp *.dvi *.fn *.ky *.log *.pg *.toc *.tp *.vr *.bak
	cd $(srcdir) && $(RM) internals.FN internals.fns internals.sgmls internals.texi internals.info*
	cd $(srcdir) && $(RM) packaging.texi packaging.html packaging.html-stamp
	cd $(srcdir) && $(RM) packaging.text packaging.ps packaging.lout
	cd $(srcdir) && $(RM) internals.texi internals.html internals.html-stamp
	cd $(srcdir) && $(RM) internals.text internals.ps packaging.lout
	cd $(srcdir) && $(RM) manuals-version*

## End of file.
