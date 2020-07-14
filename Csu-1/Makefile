OFLAG = -O
CFLAGS = $(OFLAG) -g -Wall
IFLAGS = -S -S -c -m 444

SRCROOT = .
SYMROOT = .
OBJROOT = .

LIBDIR = /lib
LOCLIBDIR = /usr/local/lib
DSTDIRS = $(DSTROOT)$(LIBDIR) $(DSTROOT)$(LOCLIBDIR)
DYLD = $(NEXT_ROOT)/usr/lib/dyld
CC = /usr/bin/cc
INDR = /usr/local/bin/indr
ifneq "$(wildcard /bin/pax)" ""
PAX = /bin/pax -rw
else
PAX = /bin/cp -p
endif
ifneq "$(wildcard /bin/mkdirs)" ""
MKDIR = /bin/mkdirs
else
MKDIR = /bin/mkdir -p
endif
CHMOD = /bin/chmod
INSTALL = /usr/bin/install

CFILES = crt.c init_shlibs.c
SFILES = start.s dyld.s dylib1.s bundle1.s
INSTALL_FILES = $(CFILES) $(SFILES) Makefile PB.project indr_list notes

all: crt0.o gcrt0.o pscrt0.o crt1.o gcrt1.o pscrt1.o dylib1.o bundle1.o

crt0.o: sstart.o xcrt0.o sdyld.o sinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) \
		-r -static -nostdlib \
		$(OBJROOT)/sstart.o $(OBJROOT)/xcrt0.o \
		$(OBJROOT)/sinit_shlibs.o $(OBJROOT)/sdyld.o \
		-o $(OBJROOT)/icrt0.o
	$(INDR) -arch all indr_list $(OBJROOT)/icrt0.o $(SYMROOT)/crt0.o

sstart.o: start.s
	$(CC) -static -DCRT0 $(RC_CFLAGS) \
		-c -o $(OBJROOT)/sstart.o $(SRCROOT)/start.s

xcrt0.o: crt.c
	$(CC) -static -DCRT0 $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xcrt0.o $(SRCROOT)/crt.c

sdyld.o: dyld.s
	$(CC) -static -DCRT0 $(RC_CFLAGS) \
		-c -o $(OBJROOT)/sdyld.o $(SRCROOT)/dyld.s

sinit_shlibs.o: init_shlibs.c
	$(CC) -static $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/sinit_shlibs.o $(SRCROOT)/init_shlibs.c

gcrt0.o: sstart.o xgcrt0.o sdyld.o sinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) \
		-r -static -nostdlib \
		$(OBJROOT)/sstart.o $(OBJROOT)/xgcrt0.o \
		$(OBJROOT)/sinit_shlibs.o $(OBJROOT)/sdyld.o \
		-o $(OBJROOT)/igcrt0.o
	$(INDR) -arch all indr_list $(OBJROOT)/igcrt0.o $(SYMROOT)/gcrt0.o

xgcrt0.o: crt.c
	$(CC) -static -DCRT0 -DGCRT $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xgcrt0.o $(SRCROOT)/crt.c

pscrt0.o: sstart.o xpscrt0.o sdyld.o sinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) \
		-r -static -nostdlib \
		$(OBJROOT)/sstart.o $(OBJROOT)/xpscrt0.o \
		$(OBJROOT)/sinit_shlibs.o $(OBJROOT)/sdyld.o \
		-o $(OBJROOT)/ipscrt0.o
	$(INDR) -arch all indr_list $(OBJROOT)/ipscrt0.o $(SYMROOT)/pscrt0.o

xpscrt0.o: crt.c
	$(CC) -static -DCRT0 -DPOSTSCRIPT $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xpscrt0.o $(SRCROOT)/crt.c

crt1.o: dstart.o xcrt1.o ddyld.o dinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) $(DYLD) \
		-r -dynamic -nostdlib -keep_private_externs \
		$(OBJROOT)/dstart.o $(OBJROOT)/xcrt1.o \
		$(OBJROOT)/ddyld.o $(OBJROOT)/dinit_shlibs.o \
		-o $(OBJROOT)/icrt1.o
	$(INDR) -arch all indr_list $(OBJROOT)/icrt1.o $(SYMROOT)/crt1.o

dstart.o: start.s
	$(CC) -dynamic -DCRT1 $(RC_CFLAGS) \
		-c -o $(OBJROOT)/dstart.o $(SRCROOT)/start.s

xcrt1.o: crt.c
	$(CC) -dynamic -DCRT1 $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xcrt1.o $(SRCROOT)/crt.c

ddyld.o: dyld.s
	$(CC) -dynamic -DCRT1 $(RC_CFLAGS) \
		-c -o $(OBJROOT)/ddyld.o $(SRCROOT)/dyld.s

dinit_shlibs.o: init_shlibs.c
	$(CC) -dynamic $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/dinit_shlibs.o $(SRCROOT)/init_shlibs.c

gcrt1.o: dstart.o xgcrt1.o ddyld.o dinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) $(DYLD) \
		-r -dynamic -nostdlib -keep_private_externs \
		$(OBJROOT)/dstart.o $(OBJROOT)/xgcrt1.o $(OBJROOT)/ddyld.o \
		$(OBJROOT)/dinit_shlibs.o -o $(OBJROOT)/igcrt1.o
	$(INDR) -arch all indr_list $(OBJROOT)/igcrt1.o $(SYMROOT)/gcrt1.o

xgcrt1.o: crt.c
	$(CC) -dynamic -DCRT1 -DGCRT $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xgcrt1.o $(SRCROOT)/crt.c

pscrt1.o: dstart.o xpscrt1.o ddyld.o dinit_shlibs.o
	$(CC) $(CFLAGS) $(RC_CFLAGS) $(DYLD) \
		-r -dynamic -nostdlib -keep_private_externs \
		$(OBJROOT)/dstart.o $(OBJROOT)/xpscrt1.o $(OBJROOT)/ddyld.o \
		$(OBJROOT)/dinit_shlibs.o -o $(OBJROOT)/ipscrt1.o
	$(INDR) -arch all indr_list $(OBJROOT)/ipscrt1.o $(SYMROOT)/pscrt1.o

xpscrt1.o: crt.c
	$(CC) -dynamic -DCRT1 -DPOSTSCRIPT $(CFLAGS) $(RC_CFLAGS) \
		-c -o $(OBJROOT)/xpscrt1.o $(SRCROOT)/crt.c

dylib1.o: dylib1.s
	$(CC) $(RC_CFLAGS) -c -o $(SYMROOT)/dylib1.o $(SRCROOT)/dylib1.s

bundle1.o: bundle1.s
	$(CC) $(RC_CFLAGS) -c -o $(SYMROOT)/bundle1.o $(SRCROOT)/bundle1.s

clean:
	rm -f $(OBJROOT)/sstart.o $(OBJROOT)/dstart.o \
	      $(OBJROOT)/sdyld.o $(OBJROOT)/ddyld.o \
	      $(OBJROOT)/sinit_shlibs.o $(OBJROOT)/dinit_shlibs.o 
	rm -f $(OBJROOT)/xcrt0.o \
	      $(OBJROOT)/xgcrt0.o \
	      $(OBJROOT)/xpscrt0.o \
	      $(OBJROOT)/xcrt1.o \
	      $(OBJROOT)/xgcrt1.o \
	      $(OBJROOT)/xpscrt1.o
	rm -f $(OBJROOT)/icrt0.o \
	      $(OBJROOT)/igcrt0.o \
	      $(OBJROOT)/ipscrt0.o \
	      $(OBJROOT)/icrt1.o \
	      $(OBJROOT)/igcrt1.o \
	      $(OBJROOT)/ipscrt1.o
	rm -f $(SYMROOT)/crt0.o \
	      $(SYMROOT)/gcrt0.o \
	      $(SYMROOT)/pscrt0.o \
	      $(SYMROOT)/crt1.o \
	      $(SYMROOT)/gcrt1.o \
	      $(SYMROOT)/pscrt1.o
	rm -f $(SYMROOT)/dylib1.o $(SYMROOT)/bundle1.o

install: all $(DSTDIRS)
	$(INSTALL) $(IFLAGS) $(SYMROOT)/crt1.o    $(DSTROOT)$(LIBDIR)/crt1.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/gcrt1.o   $(DSTROOT)$(LIBDIR)/gcrt1.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/dylib1.o  $(DSTROOT)$(LIBDIR)/dylib1.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/bundle1.o $(DSTROOT)$(LIBDIR)/bundle1.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/crt0.o    $(DSTROOT)$(LOCLIBDIR)/crt0.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/gcrt0.o   $(DSTROOT)$(LOCLIBDIR)/gcrt0.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/pscrt0.o  $(DSTROOT)$(LOCLIBDIR)/pscrt0.o
	$(INSTALL) $(IFLAGS) $(SYMROOT)/pscrt1.o  $(DSTROOT)$(LOCLIBDIR)/pscrt1.o

installhdrs:

installsrc:
	$(MKDIR) $(SRCROOT)
	$(CHMOD) 755 $(SRCROOT)
	$(PAX) $(INSTALL_FILES) $(SRCROOT)
	$(CHMOD) 444 $(SRCROOT)/*

$(OJBROOT) $(SYMROOT) $(DSTDIRS):
	$(MKDIR) $@
