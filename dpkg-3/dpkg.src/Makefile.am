## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS        = 1.1 foreign

ALL_LINGUAS		= en fr ja_JP.EUC

dselect_SUBDIRS         = @dselect_SUBDIRS@

SUBDIRS                 = libiberty intl lib dpkg dpkg-deb split \
	                  md5sum scripts tests $(dselect_SUBDIRS) po doc

## Directory definitions

pkglocalstatedir        = $(sharedstatedir)/@PACKAGE@
pkglibdir               = $(libdir)/@PACKAGE@

docdir                  = $(prefix)/doc
pkgdocdir               = $(docdir)/@PACKAGE@

## Various options

CFLAGS                  = @CFLAGS@ @CWARNS@ -g $(XCFLAGS)
OPTCFLAGS               = @OPTCFLAGS@ @CWARNS@ -g $(XCFLAGS)
LDFLAGS                 = @LDFLAGS@ $(XLDFLAGS)
LIBS                    = @LIBS@ $(XLIBS)

export CFLAGS OPTCFLAGS LDFLAGS LIBS

## Automake variables

# Can set this to "build" (wrapper found in debmake)
DEBIAN_BUILD            = dpkg-buildpackage

# May want to add -a<arch>?
DEBIAN_FLAGS            =

BUILT_SOURCES           = version.h config.h
EXTRA_DIST              = archtable COPYING INSTALL 

CLEANFILES              = version.h.new
DISTCLEANFILES          = version.h config.h
MAINTAINERCLEANFILES    = libtool

DISTDEBFILES            = 50dpkg-dev.el changelog control copyright \
	                  dev-README postinst preinst prerm rules \
	                  shlibs shlibs.default.i386 shlibs.local

MAINTCLEANFILES         = Makefile.in aclocal.m4 config.guess \
	                  config.h.in config.sub configure install-sh \
	                  ltconfig ltmain.sh mkinstalldirs ABOUT-NLS \
	                  po/Makefile.in.in po/stamp-cat-id po/cat-id-tbl.c \
	                  po/dpkg.pot stamp-h.in missing
## Rules

.PHONY: install-data-local uninstall-local dist-hook maintainer-clean-local

uninstall-local:
	-for d in $(pkglocalstatedir) $(pkglibdir) $(pkgdocdir); do \
	        [ -e $$d ] && rmdir $$d; \
	done

dist-hook:
	$(mkinstalldirs) $(distdir)/debian
	set -e; for i in $(DISTDEBFILES); do \
	        cp -p $(srcdir)/debian/$$i $(distdir)/debian/; \
	done
	$(mkinstalldirs) $(distdir)/automake
	cp -dp $(top_srcdir)/automake/* $(distdir)/automake/

maintainer-clean-local:
	-cd $(srcdir) && rm -f $(MAINTCLEANFILES)
	-$(RM) -r $(srcdir)/intl

# for created package files
DEST := $(top_builddir)/..

# for debian/files
DEBIAN := $(top_builddir)/debian

# scratch directory for package building
TMP := $(DEBIAN)/tmp

ENV = \
	LD_LIBRARY_PATH=$(top_builddir)/lib/.libs \
	PATH=$(top_builddir)/dpkg:$(top_builddir)/dpkg-deb:$(top_builddir)/scripts:${PATH}

binary: binary-arch binary-indep

binary-trees:

	install -d $(TMP)		

	-$(RM) -r $(TMP)/install $(TMP)/install.orig $(TMP)/main \
		  $(TMP)/dpkg-dev $(TMP)/dselect \
		  debian/files debian/substvars

	$(MAKE)	prefix=`cd $(TMP) && pwd`/install/usr \
		sysconfdir=`cd $(TMP) && pwd`/install/etc \
		datadir=`cd $(TMP) && pwd`/install/usr/share \
		sharedstatedir=`cd $(TMP) && pwd`/install/var/lib \
		localstatedir=`cd $(TMP) && pwd`/install/var/lib \
		install

#	mv $(TMP)/install $(TMP)/install.orig
#
#binary-trees:
#
#	-$(RM) -r $(TMP)/install $(TMP)/main $(TMP)/dpkg-dev $(TMP)/dselect debian/files debian/substvars
#	cp -rp $(TMP)/install.orig $(TMP)/install

	set -e; for i in main dpkg-dev dselect; do \
		install -d $(TMP)/$$i/DEBIAN; \
	done

	install -c $(srcdir)/debian/preinst.dpkg $(TMP)/main/DEBIAN/preinst
	install -c $(srcdir)/debian/prerm.dpkg $(TMP)/main/DEBIAN/prerm
	install -c $(srcdir)/debian/postinst.dpkg $(TMP)/main/DEBIAN/postinst

	# default file for dpkg-shlibdeps
	install -d $(TMP)/dpkg-dev/etc/dpkg
	echo '/etc/dpkg/shlibs.default' >> $(TMP)/dpkg-dev/DEBIAN/conffiles
	set -e; \
	if [ -f debian/shlibs.default.$(arch) ]; then \
	  cp debian/shlibs.default.$(arch) $(TMP)/dpkg-dev/etc/dpkg/shlibs.default; \
	else \
	  touch $(TMP)/dpkg-dev/etc/dpkg/shlibs.default; \
	fi

	# emacs/xemacs configuration files
	install -d $(TMP)/dpkg-dev/etc/emacs/site-start.d
	install -c $(srcdir)/debian/50dpkg-dev.el $(TMP)/dpkg-dev/etc/emacs/site-start.d/
	install -d $(TMP)/dpkg-dev/etc/xemacs/site-start-19.d
	install -c $(srcdir)/debian/50dpkg-dev.el $(TMP)/dpkg-dev/etc/xemacs/site-start-19.d/

	# compress manual pages
	find $(TMP)/install/usr/man -type f -print0 | xargs -0 gzip -9f --
	find $(TMP)/install/usr/man -type l -print0 | xargs -0 rm -f --
	set -e; for f in \
		dpkg-buildpackage \
		dpkg-gencontrol \
		dpkg-distaddfile \
		dpkg-parsechangelog \
		dpkg-genchanges \
		dpkg-shlibdeps; \
		do \
		ln -sf dpkg-source.1.gz $(TMP)/install/usr/man/man1/$$f.1.gz ; \
	done

	# copyright, changelog
	gzip -9f $(TMP)/install/usr/doc/dpkg/changelog
	gzip -9f $(TMP)/install/usr/doc/dpkg/changelog.manuals
	set -e; for i in dpkg-dev dselect; do \
		install -d $(TMP)/$$i/usr/doc/$$i; \
		install -c $(srcdir)/debian/copyright $(TMP)/$$i/usr/doc/$$i/copyright; \
		ln -s ../$$i/changelog.gz ../$$i/changelog.manuals.gz $(TMP)/$$i/usr/doc/$$i/; \
	done

	# misc documentation
	set -e; for p in dpkg-dev dselect; do \
		install -c $(srcdir)/debian/README.doc $(TMP)/$$p/usr/doc/$$p/README.doc; \
		gzip -9f $(TMP)/$$p/usr/doc/$$p/README.doc; \
	done

	set -e; for lang in $(ALL_LINGUAS); do \
		install -d $(TMP)/main/usr/share/locale/$$lang/LC_MESSAGES; \
		install -d $(TMP)/dev/usr/share/locale/$$lang/LC_MESSAGES; \
		install -d $(TMP)/dselect/usr/share/locale/$$lang/LC_MESSAGES; \
		mv $(TMP)/install/usr/share/locale/$$lang/LC_MESSAGES/libdpkg.mo \
		   $(TMP)/main/usr/share/locale/$$lang/LC_MESSAGES/libdpkg.mo; \
		mv $(TMP)/install/usr/share/locale/$$lang/LC_MESSAGES/dpkg.mo \
		   $(TMP)/main/usr/share/locale/$$lang/LC_MESSAGES/dpkg.mo; \
		mv $(TMP)/install/usr/share/locale/$$lang/LC_MESSAGES/dpkgdeb.mo \
		   $(TMP)/main/usr/share/locale/$$lang/LC_MESSAGES/dpkgdeb.mo; \
		mv $(TMP)/install/usr/share/locale/$$lang/LC_MESSAGES/dpkgutil.mo \
		   $(TMP)/main/usr/share/locale/$$lang/LC_MESSAGES/dpkgutil.mo; \
		mv $(TMP)/install/usr/share/locale/$$lang/LC_MESSAGES/dselect.mo \
		   $(TMP)/dselect/usr/share/locale/$$lang/LC_MESSAGES/dselect.mo; \
	done

	install -d $(TMP)/main/etc
	install -d $(TMP)/main/usr/bin
	install -d $(TMP)/main/usr/sbin
	install -d $(TMP)/main/usr/man/man1
	install -d $(TMP)/main/usr/man/man8
	install -d $(TMP)/main/usr/lib/dpkg
	install -d $(TMP)/main/var/lib/dpkg
	install -d $(TMP)/main/usr/doc/dpkg
	set -e; for f in \
		etc/alternatives \
		usr/bin/dpkg \
		usr/bin/dpkg-split \
		usr/bin/dpkg-deb \
		usr/bin/md5sum \
		usr/sbin/dpkg-divert \
		usr/sbin/install-info \
		usr/sbin/update-alternatives \
		usr/sbin/update-rc.d \
		usr/doc/dpkg/changelog.gz \
		usr/doc/dpkg/changelog.manuals.gz \
		usr/man/man1/md5sum.1.gz \
		usr/man/man1/dpkg-deb.1.gz \
		usr/man/man8/dpkg.8.gz \
		usr/man/man8/dpkg-split.8.gz \
		usr/man/man8/install-info.8.gz \
		usr/man/man8/start-stop-daemon.8.gz \
		usr/man/man8/update-alternatives.8.gz \
		usr/man/man8/update-rc.d.8.gz \
		usr/include \
		usr/lib/dpkg/mksplit \
		var/lib/dpkg/alternatives \
		; do \
		mv $(TMP)/install/$$f $(TMP)/main/$$f; \
	done
	mv $(TMP)/install/usr/lib/libdpkg* $(TMP)/main/usr/lib/


	install -d $(TMP)/dpkg-dev/usr/doc/dpkg
	install -d $(TMP)/dpkg-dev/usr/bin
	install -d $(TMP)/dpkg-dev/usr/man/man1
	install -d $(TMP)/dpkg-dev/usr/man/man5
	install -d $(TMP)/dpkg-dev/usr/man/man8
	install -d $(TMP)/dpkg-dev/usr/lib/dpkg
	install -d $(TMP)/dpkg-dev/usr/share
	set -e; for f in \
		usr/man/man1/822-date.1.gz \
		usr/man/man1/dpkg-buildpackage.1.gz \
		usr/man/man1/dpkg-distaddfile.1.gz \
		usr/man/man1/dpkg-genchanges.1.gz \
		usr/man/man1/dpkg-gencontrol.1.gz \
		usr/man/man1/dpkg-name.1.gz \
		usr/man/man1/dpkg-parsechangelog.1.gz \
		usr/man/man1/dpkg-shlibdeps.1.gz \
		usr/man/man1/dpkg-source.1.gz \
		usr/man/man5/deb-control.5.gz \
		usr/man/man5/deb-old.5.gz \
		usr/man/man5/deb.5.gz \
		usr/man/man8/dpkg-scanpackages.8.gz \
		usr/doc/dpkg/internals.html \
		usr/doc/dpkg/packaging.html \
		usr/doc/dpkg/developer-keys.pgp \
		usr/bin/dpkg-buildpackage \
		usr/bin/dpkg-distaddfile \
		usr/bin/dpkg-genchanges \
		usr/bin/dpkg-gencontrol \
		usr/bin/dpkg-name \
		usr/bin/dpkg-parsechangelog \
		usr/bin/dpkg-scanpackages \
		usr/bin/dpkg-shlibdeps \
		usr/bin/dpkg-source \
		usr/bin/822-date \
		usr/lib/dpkg/parsechangelog \
		usr/lib/dpkg/controllib.pl \
		; do \
		mv $(TMP)/install/$$f $(TMP)/dpkg-dev/$$f; \
	done

	mv $(TMP)/install/usr/lib/emacs $(TMP)/dpkg-dev/usr/lib/emacs \
	|| mv $(TMP)/install/usr/share/emacs $(TMP)/dpkg-dev/usr/share/emacs \
	|| true

	install -d $(TMP)/dselect/usr/bin
	install -d $(TMP)/dselect/usr/lib/dpkg
	install -d $(TMP)/dselect/var/lib/dpkg
	install -d $(TMP)/dselect/usr/man/man8
	set -e; for f in \
		usr/bin/dselect \
		usr/lib/dpkg/methods \
		var/lib/dpkg/methods \
		usr/man/man8/dselect.8.gz \
		; do mv $(TMP)/install/$$f $(TMP)/dselect/$$f; \
	done	

	install -d $(TMP)/main/sbin
	mv $(TMP)/install/usr/sbin/start-stop-daemon $(TMP)/main/sbin/start-stop-daemon

	-strip $(TMP)/main/usr/bin/* $(TMP)/main/usr/sbin/* $(TMP)/main/sbin/*
	-strip --strip-unneeded debian/tmp-main/usr/lib/libdpkg.so.*
	-strip --strip-debug debian/tmp-main/usr/lib/libdpkg.a

	chmod 755 $(TMP)/main/DEBIAN/postinst $(TMP)/main/DEBIAN/prerm $(TMP)/main/DEBIAN/preinst

	-rmdir $(TMP)/install/*/*/*/*/*
	-rmdir $(TMP)/install/*/*/*/*
	-rmdir $(TMP)/install/*/*/*
	-rmdir $(TMP)/install/*/*
	-rmdir $(TMP)/install/*
	rmdir $(TMP)/install

	chown -R root:root $(TMP) || chown -R root:wheel $(TMP)
	chmod -R g-ws,a+r,u+w $(TMP)

binary-indep: binary-trees

	$(ENV) $(top_builddir)/scripts/dpkg-gencontrol.build \
		-c$(srcdir)/debian/control \
		-l$(srcdir)/debian/changelog \
		-T$(DEBIAN)/substvars \
		-f$(DEBIAN)/files \
		-pdpkg-dev \
		-P$(TMP)/dpkg-dev

	$(ENV) $(top_builddir)/dpkg-deb/dpkg-deb --build $(TMP)/dpkg-dev $(DEST)

	set -e; for f in internals packaging; do \
		cp -p $(srcdir)/doc/$$f.ps $(DEST); \
		gzip -9f $(DEST)/$$f.ps; \
		$(ENV) $(top_builddir)/scripts/dpkg-distaddfile.build -f$(DEBIAN)/files $(DEST)/$$f.ps.gz byhand -; \
		GZIP=-9f tar -C $(srcdir)/doc -zcf $(DEST)/$$f.html.tar.gz $$f.html; \
		$(ENV) $(top_builddir)/scripts/dpkg-distaddfile.build -f$(DEBIAN)/files $(DEST)/$$f.html.tar.gz byhand -; \
	done

binary-arch: binary-trees

	#$(ENV) $(top_builddir)/scripts/dpkg-shlibdeps.build \
	#	-L$(srcdir)/debian/shlibs.local \
	#	-dPre-Depends \
	#	-T$(DEBIAN)/substvars \
	#	$(TMP)/main/usr/bin/* $(TMP)/main/usr/sbin/* $(TMP)/main/sbin/*

	-dpkg-shlibdeps \
		-L$(srcdir)/debian/shlibs.local \
		-dPre-Depends \
		-T$(DEBIAN)/substvars \
		$(TMP)/main/usr/bin/* $(TMP)/main/usr/sbin/* $(TMP)/main/sbin/*

	$(ENV) $(top_builddir)/scripts/dpkg-gencontrol.build \
		-pdpkg \
		-c$(srcdir)/debian/control \
		-l$(srcdir)/debian/changelog \
		-T$(DEBIAN)/substvars \
		-f$(DEBIAN)/files \
		-P$(TMP)/main

	$(ENV) $(top_builddir)/dpkg-deb/dpkg-deb --build $(TMP)/main $(DEST)

	#$(ENV) $(top_builddir)/scripts/dpkg-shlibdeps.build \
	#	-L$(srcdir)/debian/shlibs.local \
	#	-dPre-Depends \
	#	-T$(DEBIAN)/substvars \
	#	$(TMP)/main/usr/bin/*

	-dpkg-shlibdeps \
		-L$(srcdir)/debian/shlibs.local \
		-dPre-Depends \
		-T$(DEBIAN)/substvars \
		$(TMP)/main/usr/bin/*

	$(ENV) $(top_builddir)/scripts/dpkg-gencontrol.build \
		-pdselect \
		-c$(srcdir)/debian/control \
		-l$(srcdir)/debian/changelog \
		-T$(DEBIAN)/substvars \
		-f$(DEBIAN)/files \
		-P$(TMP)/dselect

	$(ENV) $(top_builddir)/dpkg-deb/dpkg-deb --build $(TMP)/dselect $(DEST)

	set -e; \
		version=`sed -n 's/^Version: //p' $(TMP)/main/DEBIAN/control`; \
		file=dpkg_$${version}_$(arch).nondebbin.tar; \
		tar -C $(TMP)/main -cf $(DEST)/$${file} usr var; \
		gzip -9f $(DEST)/$${file}; \
		$(ENV) $(top_builddir)/scripts/dpkg-distaddfile.build -f$(DEBIAN)/files $(DEST)/$${file}.gz byhand -

## End of file.
