## Process this file with automake to produce a Makefile.in

AUTOMAKE_OPTIONS	= 1.1 foreign

## Directory definitions

pkglibdir		= $(libdir)/@PACKAGE@
pkglocalstatedir	= $(sharedstatedir)/@PACKAGE@

methodsdir		= $(pkglibdir)/methods
methodsdatadir		= $(pkglocalstatedir)/methods
methodsmnt		= $(methodsdatadir)/mnt

## Automake variables

MSCRIPTS		= setup update install
METHODS			= disk floppy
DESCS			= disk.desc.harddisk disk.desc.mounted disk.desc.cdrom \
			  disk.desc.nfs floppy.desc.floppy

LINGUAS			= en fr ja_JP.EUC

MAINTAINERCLEANFILES	= $(srcdir)/Makefile.in

## Rules

.PHONY: install-data-local install-methods-data install-descs-data \
	install-exec-local dist-hook uninstall-local

install-data-local: install-methods-data install-descs-data
	$(mkinstalldirs) $(methodsmnt)

install-methods-data:
	set -e; for m in $(METHODS); do \
		set -e; for l in $(LINGUAS); do \
			$(mkinstalldirs) $(methodsdir)/$$l/$$m; \
			$(INSTALL_DATA) $(srcdir)/$$l/$$m.names $(methodsdir)/$$l/$$m/names; \
		done; \
		$(mkinstalldirs) $(methodsdatadir)/$$m; \
	done

install-descs-data:
	set -e; for x in $(DESCS); do \
		set -e; for l in $(LINGUAS); do \
			d=`echo $$x | sed 's:.*/\([^/]*\)$$:\1:; s:\.:/:'`; \
			$(INSTALL_DATA) $(srcdir)/$$l/$$x $(methodsdir)/$$l/$$d; \
		done; \
	done

install-exec-local:
	set -e; for m in $(METHODS); do \
		$(mkinstalldirs) $(methodsdatadir)/$$m; \
		set -e; for l in $(LINGUAS); do \
			$(mkinstalldirs) $(methodsdir)/$$l/$$m; \
			for s in $(MSCRIPTS); do \
				$(INSTALL_PROGRAM) $(srcdir)/$$l/$$m.$$s $(methodsdir)/$$l/$$m/$$s; \
			done; \
		done; \
	done

uninstall-local:
	-for m in $(METHODS); do \
		for l in $(LINGUAS); do \
			rm -f $(methodsdir)/$$l/$$m/names; \
			[ -e $(methodsdir)/$$l/$$m ] && rmdir $(methodsdir)/$$l/$$m; \
		done; \
	done
	-for x in $(DESCS); do \
		for l in $(LINGUAS); do \
			d=`echo $$x | sed 's:\.:/:'`; \
			dir=`echo $$d | sed 's:\(.*\)/[^/]*$$:\1:'`; \
			rm -f $(methodsdir)/$$l/$$d; \
			[ -e $$dir ] && rmdir $$dir; \
		done; \
	done
	-for m in $(METHODS); do \
		for l in $(LINGUAS); do \
			for s in $(MSCRIPTS); do \
				rm -f $(methodsdir)/$$l/$$m/$$s; \
			done; \
			[ -e $(methodsdir)/$$l/$$m ] && rmdir $(methodsdir)/$$l/$$m; \
		done; \
		[ -e $(methodsdir)/$$m ] && rmdir $(methodsdir)/$$m; \
		[ -e $(methodsdatadir)/$$m ] && rmdir $(methodsdatadir)/$$m; \
		[ -e $(methodsmnt)/$$m ] && rmdir $(methodsmnt)/$$m; \
	done
	-[ -e $(methodsmnt) ] && rmdir $(methodsmnt)
	-[ -e $(methodsdir) ] && rmdir $(methodsdir)
	-[ -e $(methodsdatadir) ] && rmdir $(methodsdatadir)


dist-hook:
	set -e; for l in $(LINGUAS); do \
		$(mkinstalldirs) $(distdir)/$$l; \
		set -e; for m in $(METHODS); do \
			cp -p $(srcdir)/$$l/$$m.names $(distdir)/$$l/; \
			for s in $(MSCRIPTS); do \
				cp -p $(srcdir)/$$l/$$m.$$s $(distdir)/$$l/; \
			done; \
		done; \
		for d in $(DESCS); do \
			cp -p $(srcdir)/$$l/$$d $(distdir)/$$l/; \
		done; \
	done

## End of file.
