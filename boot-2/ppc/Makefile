SYMROOT = sym/ppc

default:
	mkdir -p $(SYMROOT)
	cp bin/macho-to-xcoff $(SYMROOT)
	chmod 755 $(SYMROOT)/macho-to-xcoff
	@(cd ppcMac ;  make )
	@(cd SecondaryLoader ; make )

install:	
	make default
	make install_ppc

clean:
	-/bin/rm -r obj sym
	@-(cd SecondaryLoader ; make clean )
	@-(cd ppcMac/sarld ; make clean )

install_ppc:
	mkdir -p $(DSTROOT)/usr/standalone/ppc
	cp $(SYMROOT)/SecondaryLoader $(DSTROOT)/usr/standalone/ppc/boot0
	chmod 444 $(DSTROOT)/usr/standalone/ppc/boot0
	cp $(SYMROOT)/sarld $(DSTROOT)/usr/standalone/ppc/sarld
	chmod 444 $(DSTROOT)/usr/standalone/ppc/sarld

installsrc:
	gnutar cpf - . | (cd ${SRCROOT}; gnutar xpf -)


installhdrs:
