.PHONY: clean install installhdrs installsrc installsrc-check

d = $(DSTROOT)
perl = $(DSTROOT)
python = $(DSTROOT)

clean:
	find . -name \*~ -print0 | xargs -0 $(RM)

install:

	install -d $(python)/usr/lib/python1.5/site-packages/dpkg
	install -d $(python)/usr/doc/dpkg-python

	install -c debian/changelog $(python)/usr/doc/dpkg-python/changelog.Debian
	gzip -9vf $(python)/usr/doc/dpkg-python/*
	install -c debian/copyright $(python)/usr/doc/dpkg-python/copyright

	cp python/dpkg/*.py $(python)/usr/lib/python1.5/site-packages/dpkg/

	install -d $(perl)/usr/lib/perl5/Dpkg
	install -d $(perl)/usr/doc/dpkg-perl

	install -c debian/changelog $(perl)/usr/doc/dpkg-perl/changelog.Debian
	gzip -9vf $(perl)/usr/doc/dpkg-perl/*
	install -c debian/copyright $(perl)/usr/doc/dpkg-perl/copyright

	cp -rp perl5/Dpkg/* $(perl)/usr/lib/perl5/Dpkg/
	find $(perl)/usr/lib/perl5 -type f \! -name \*.pm -print0 | xargs -0 $(RM)

	chown -R root.root $(d) || chown -R root.wheel $(d)
	find $(d) -type f -print0 | xargs -0 chmod 644
	find $(d) -type d -print0 | xargs -0 chmod 755

installhdrs:

installsrc:

	gnutar --exclude=CVS -cf - . | gnutar -C $(SRCROOT) -xf -

installsrc-check:
	[ -z `find . -name \*~` ] || \
		(echo 'Emacs-style backup files present; not copying.' && exit 1)
