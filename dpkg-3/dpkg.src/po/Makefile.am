## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS = 1.1 foreign

localedir = $(prefix)/share/locale

XGETTEXT = PATH=../src:$$PATH /usr/bin/xgettext --foreign-user --directory=$(top_srcdir) --directory=$(top_builddir)
MSGMERGE = PATH=../src:$$PATH msgmerge

SOURCES = cat-id-tbl.c

ALL_LINGUAS = en fr ja_JP.EUC
ALL_PACKAGES = libdpkg dpkgdeb dpkgutil dpkg dselect

POGFILES = \
  $(foreach lingua,$(ALL_LINGUAS), \
    $(foreach package,$(ALL_PACKAGES), \
      $(package).$(lingua).pog))

GMOFILES = \
  $(foreach lingua,$(ALL_LINGUAS), \
    $(foreach package,$(ALL_PACKAGES), \
      $(package).$(lingua).gmo))

CATALOGS = $(GMOFILES)

POTFILES = $(addsuffix .pot,$(ALL_PACKAGES))

DISTFILES = ChangeLog stamp-cat-id $(POGFILES) $(POTFILES) $(GMOFILES) $(SOURCES)
SUFFIXES = .c .o .po .pog .pox .gmo .mo .msg .cat

all: 	potfiles catalogs
potfiles: $(addprefix $(srcdir)/,$(POTFILES))
catalogs: $(CATALOGS)

libdpkg_POTFILES = \
	lib/arch.c \
	lib/database.c \
	lib/dbmodify.c \
	lib/dump.c \
	lib/ehandle.c \
	lib/fields.c \
	lib/lock.c \
	lib/mlib.c \
	lib/myopt.c \
	lib/parse.c \
	lib/parsehelp.c \
	lib/showcright.c \
	lib/varbuf.c

dpkg_POTFILES = \
	dpkg/archives.c \
	dpkg/cleanup.c \
	dpkg/configure.c \
	dpkg/depcon.c \
	dpkg/enquiry.c \
	dpkg/errors.c \
	dpkg/filesdb.c \
	dpkg/help.c \
	dpkg/main.c \
	dpkg/packages.c \
	dpkg/processarc.c \
	dpkg/remove.c \
	dpkg/select.c \
	dpkg/update.c

dpkgdeb_POTFILES = \
	dpkg-deb/build.c \
	dpkg-deb/extract.c \
	dpkg-deb/info.c \
	dpkg-deb/main.c

dselect_POTFILES = \
	dselect/basecmds.cc \
	dselect/baselist.cc \
	dselect/basetop.cc \
	dselect/bindings.cc \
	dselect/curkeys.cc \
	dselect/main.cc \
	dselect/methkeys.cc \
	dselect/methlist.cc \
	dselect/method.cc \
	dselect/methparse.cc \
	dselect/pkgcmds.cc \
	dselect/pkgdepcon.cc \
	dselect/pkgdisplay.cc \
	dselect/pkginfo.cc \
	dselect/pkgkeys.cc \
	dselect/pkglist.cc \
	dselect/pkgsublist.cc \
	dselect/pkgtop.cc \
	dselect/helpmsgs.cc

dpkgutil_POTFILES = \
	split/info.c \
	split/join.c \
	split/main.c \
	split/queue.c \
	split/split.c \
	md5sum/md5sum.c \
	scripts/start-stop-daemon.c

%.gmo: %.pog
	echo $@
	#file=$(srcdir)/`echo $* | sed 's,.*/,,'`.gmo \
        #  && rm -f $$file && $(GMSGFMT) -o $$file $<

$(srcdir)/%.pot: $(%_POTFILES)
	$(XGETTEXT) --default-domain=$* \
		--add-comments --keyword=_ --keyword=N_ $($*_POTFILES)
	rm -f $@
	mv $*.po $@

.PHONY: install-data-local update-po

install-data-local:

	set -e; \
	for package in $(ALL_PACKAGES); do \
	  for lang in $(ALL_LINGUAS); do \
	    $(mkinstalldirs) $(localedir)/$$lang/LC_MESSAGES; \
	    cp -p $(srcdir)/$$package.$$lang.gmo $(localedir)/$$lang/LC_MESSAGES/$$package.mo; \
	  done; \
	done

update-po: Makefile $(addprefix $(srcdir)/,$(POTFILES))
	set -e; cd $(srcdir) && \
	for package in $(ALL_PACKAGES); do \
	  for lang in $(ALL_LINGUAS); do \
	    mv $$package.$$lang.pog $$package.$$lang.old.pog; \
	    echo "$$package.$$lang:"; \
	    if $(MSGMERGE) $$package.$$lang.old.pog $$package.pot -o $$package.$$lang.pog; then \
	      rm -f $$package.$$lang.old.pog; \
	    else \
	      echo "msgmerge for $$package.$$lang failed"; \
	      rm -f $$package.$$lang.pog; \
	      mv $$package.$$lang.old.pog $$package.$$lang.pog; \
	    fi; \
	  done \
	done
