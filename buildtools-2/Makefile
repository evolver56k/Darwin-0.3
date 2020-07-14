.PHONY: clean install installhdrs installsrc installsrc-check

d = $(DSTROOT)
p = darwin-buildtools

clean:
	find . -name \*~ -print0 | xargs -0 $(RM)

install:

	rm -rf $(d)

	install -d $(d)/usr/doc/$(p)
	install -d $(d)/usr/lib/perl5/Dpkg/Package
	install -d $(d)/usr/bin

	install -c debian/changelog $(d)/usr/doc/$(p)/changelog.Debian
	gzip -9vf $(d)/usr/doc/$(p)/*
	install -c debian/copyright $(d)/usr/doc/$(p)/copyright

	cp -p lib/*.pm $(d)/usr/lib/perl5/Dpkg/Package
	cp tools/darwin-buildall.pl $(d)/usr/bin/darwin-buildall
	cp tools/darwin-buildpackage.pl $(d)/usr/bin/darwin-buildpackage
	cp tools/darwin-missing.pl $(d)/usr/bin/darwin-missing

	chown -R root.root $(d) || chown -R root.wheel $(d)
	find $(d) -type f -print0 | xargs -0 chmod 644
	find $(d) -type d -print0 | xargs -0 chmod 755

	chmod a+x $(d)/usr/bin/* 

installhdrs:

installsrc:

	gnutar --exclude=CVS -cf - . | gnutar -C $(SRCROOT) -xf -

installsrc-check:
	[ -z `find . -name \*~` ] || \
		(echo 'Emacs-style backup files present; not copying.' && exit 1)
