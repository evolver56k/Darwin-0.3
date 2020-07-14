
DIR = boot1
include ../MakePaths.dir

INSTALLDIR = $(DSTROOT)/usr/standalone/i386
FOREINSTALLDIR = $(DSTROOT)/usr/Dos
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

FOREIGNNEXT = boot1 boot1f nullboot1
FOREIGNDOS = gonext.com

NASM = $(SYMROOT)/nasm

VERSIONED_FILES = boot1 boot1f nullboot1
VERS = `vers_string -f 5.0 | tr - .`
NEW_VERS = Rhapsody boot1 v$(VERS)
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

REP_CFLAGS = $(RC_CFLAGS) -Wmost -Wno-precomp -g -I../rcz -traditional-cpp -nostdinc -nostdlib /lib/crt1.o  -I/System/Library/Frameworks/System.framework/Headers -I/System/Library/Frameworks/System.framework/Headers/bsd -I/System/Library/Frameworks/System.framework/Headers/ansi
REP_LDFLAGS = /System/Library/Frameworks/System.framework/System /usr/lib/libcc_dynamic.a

all: $(DIRS_NEEDED) $(VERSIONED_FILES)

boot1: boot1.s Makefile
	$(NASM) -dBOOTDEV=HDISK  -dVERS="'$(NEW_VERS)'" boot1.s -o $(SYMROOT)/$@
boot1f: boot1.s Makefile
	$(NASM) -dBOOTDEV=FLOPPY -dVERS="'$(NEW_VERS)'" boot1.s -o $(SYMROOT)/$@
nullboot1: nullboot1.s Makefile
	$(NASM) nullboot1.s -o $(SYMROOT)/$@

install_i386:: all $(INSTALLDIR) $(FOREINSTALLDIR)
	cp $(SYMROOT)/boot1 $(INSTALLDIR)/
	cp $(SYMROOT)/boot1f $(INSTALLDIR)/
	cp $(SYMROOT)/nullboot1 $(INSTALLDIR)/
	cd $(INSTALLDIR); chmod u+w $(FOREIGNNEXT)
	cp $(FOREIGNDOS) $(FOREINSTALLDIR)
	cd $(FOREINSTALLDIR); chmod u+w $(FOREIGNDOS)

$(FOREINSTALLDIR):
	$(MKDIRS) $@

include ../MakeInc.dir
