#
# Copyright (C) 1994-1996 NeXT Software, Inc.
# All Rights Reserved
#

#
# These ROOTs will get overridden by RC
#
OBJROOT=./obj
SYMROOT=./obj
DSTROOT=
SRCROOT:=$(shell pwd)

PDO_SRC_DIR = $(DSTROOT)/Developer/Source/GNU/make
GNU_SRC_DIR = $(DSTROOT)/System/Developer/Source/GNU/make
PDO_DOC_DIR = $(DSTROOT)/Documentation/Developer/Commands/make
DOC_DIR = $(DSTROOT)/System/Documentation/Developer/Commands/make

#
# Even though configure knows when it's cross-compiling, it still
# will fail because it cannot run some test programs. The workaround
# is to hard-code certain values by setting them in the environment
# before we invoke configure.
#
# We need to reset RC_OS, or else we'll pick up the wrong
# paths....
# We also have to pass RC_CFLAGS into configure, else it'll
# just go ahead and build a nextstep one.
#
#
# We only do this when RC_RELEASE is Grail (for the Titan train)...
#

CONFIGURE_ENVIRON =

ifeq ($(RC_OS), teflon)
ifeq ($(RC_RELEASE), Grail)
    override RC_OS := nextstep
    CONFIGURE_ENVIRON = ac_cv_func_setvbuf_reversed=yes ac_cv_func_closedir_void=no CC='cc $(RC_CFLAGS)'
endif
endif

ifeq ($(RC_OS), winnt)
   GNU_SRC_DIR = $(DSTROOT)/Developer/Source/GNU/make
   UTIL_BIN =  $(NEXT_ROOT)/Developer/Executables/Utilities

   SHELL = $(NEXT_ROOT)/Library/Executables/sh.exe
   PERL = $(NEXT_ROOT)/Local/Developer/Executables/perl.exe
   EXEC_SUFFIX = .exe
   STRIP = echo Warning: Not stripping 
   RM = $(UTIL_BIN)/rm
   INSTALL = $(UTIL_BIN)/cp
   SYMLINK=$(UTIL_BIN)/ln.exe -s
   MKDIRS = $(UTIL_BIN)/mkdirs
   TAR = $(UTIL_BIN)/tar
   CHMOD = $(UTILDIR)/chmod
else
   # Assume basic Unix-like commands
   SHELL   = /bin/sh
ifneq "" "$(wildcard /usr/bin/perl)"
   PERL    = /usr/bin/perl
else
   PERL    = /usr/local/bin/perl
endif
   RM      = /bin/rm
   INSTALL = /bin/cp
   SYMLINK = /bin/ln -s
   TAR     = /usr/bin/gnutar

ifeq ($(RC_OS), nextstep)
   STRIP = /bin/strip
ifneq "" "$(wildcard /bin/mkdirs)"
   MKDIRS = /bin/mkdirs
else
   MKDIRS = /bin/mkdir -p
endif
   CHOWN = /etc/chown
   CHGRP = /bin/chgrp
endif

ifeq ($(RC_OS), solaris)
   CHMOD = /bin/chmod -f
else
   CHMOD = /bin/chmod
endif

ifeq ($(RC_OS), teflon)
   STRIP = /usr/bin/strip
   MKDIRS = /bin/mkdir -p
   CHOWN = /usr/sbin/chown
   CHGRP = /usr/bin/chgrp
endif

ifeq ($(RC_OS), mach)
   STRIP = /bin/strip 
   MKDIRS = /bin/mkdirs	
   CHOWN = /etc/chown
   CHGRP = /bin/chgrp
endif

ifndef RC_OS
   MKDIRS = mkdir -p
endif

export RC_OS

PDO_DEV_BIN = $(NEXT_ROOT)/Developer/Executables
INSTALL_GROUP = wheel

ifeq ($(RC_OS), solaris)
   TAR     = /bin/tar
   INSTALL_GROUP = bin
   MKDIRS = /usr/bin/mkdir -p
   CC = "$(PDO_DEV_BIN)/gcc -nopdolib -O"
   LD = /usr/ccs/bin/ld
   AR = /usr/ccs/bin/ar  
   STRIP = /usr/ccs/bin/strip  
   CHOWN = /usr/ucb/chown
   CHGRP = /bin/chgrp
   GNU_SRC_DIR = $(PDO_SRC_DIR)
   DOC_DIR = $(PDO_DOC_DIR)
endif

ifeq ($(RC_OS), hpux)
   TAR     = /bin/tar
   INSTALL_GROUP = bin
   MKDIRS = /usr/bin/mkdir -p
   CC = "$(PDO_DEV_BIN)/gcc -nopdolib -O"
   LD = /bin/ld
   AR = /bin/ar  
   STRIP = /bin/strip  
   CHOWN = /bin/chown
   CHGRP = /bin/chgrp
   GNU_SRC_DIR = $(PDO_SRC_DIR)
   DOC_DIR = $(PDO_DOC_DIR)
endif

endif



NEXTSTEP_MAKE_NAME = gnumake
OLD_NEXTSTEP_MAKE_NAME = make
GNU_MAKE_NAME = make
PDO_MAKE_NAME = make
NEXTSTEP_BINDIR = $(DSTROOT)/bin
PDO_BINDIR   = $(DSTROOT)/Developer/Executables
WINNT_BINDIR = $(DSTROOT)/Developer/Executables

INFODIR=$(DSTROOT)/info
INFOFILES=make.info*
LOCALDIR=$(DSTROOT)/usr/local/bin
MAKEMAN=make.man
MANDIR=$(DSTROOT)/usr/man
TEXI_FLAGS = -split_chapter -split_node -number



# /Developer/Executables relative to /usr/local/bin
#RBINDIR=../../Developer/Executables

MAKE_SRC_DIR = make-3.74
GNU_SOURCE_FILES = $(MAKE_SRC_DIR)
SOURCE_FILES     = $(GNU_SOURCE_FILES) Makefile PB.project texi2html
MAKEDIR=$(OBJROOT)/$(MAKE_SRC_DIR)
HTML_DIR=$(SYMROOT)/html

ifeq ($(RC_OS), winnt)
all: build_make_into_makedir 
else
all: build_make_into_makedir update_docs
endif

ifeq ($(RC_OS), winnt)
install: $(RC_OS)_install installGNUsrc 
else
install: $(RC_OS)_install installGNUsrc install_docs
endif

solaris_CFLAGS = -D__EXTENSIONS__=1
hpux_CFLAGS = -D__EXTENSIONS__=1
build_make_into_makedir: $(MAKEDIR) $(MAKEDIR)/Makefile
	@echo BUILDING WITH $(RC_CFLAGS) $($(RC_OS)_CFLAGS) on $(RC_OS)
	cd $(MAKEDIR); $(MAKE) "CFLAGS = $(RC_CFLAGS) $($(RC_OS)_CFLAGS)" \
	                       "LDFLAGS= $(RC_CFLAGS)"

update_docs: $(HTML_DIR) $(HTML_DIR)/texi2html $(HTML_DIR)/make_toc.html $(HTML_DIR)/make-stds_toc.html

install_docs: update_docs
	$(RM) -rf $(DOC_DIR)
	$(MKDIRS) $(DOC_DIR)
	cp -p $(HTML_DIR)/*.html $(DOC_DIR)
ifdef CHOWN
	-$(CHOWN) -R root $(DOC_DIR)
endif
ifdef CHGRP
	-$(CHGRP) -R $(INSTALL_GROUP) $(DOC_DIR)
endif
ifdef CHMOD
	-$(CHMOD) -R a-w $(DOC_DIR)
	-$(CHMOD) -R a+rX $(DOC_DIR)
endif

$(HTML_DIR)/texi2html: texi2html
	cp -p texi2html $(HTML_DIR)/texi2html

$(HTML_DIR)/make-stds_toc.html: $(SRCROOT)/$(MAKE_SRC_DIR)/make-stds.texi
	cd $(HTML_DIR) && $(PERL) -- texi2html $(TEXI_FLAGS) $<
#	d=`pwd` && cd $(HTML_DIR) && $(PERL) -- texi2html $(TEXI_FLAGS) $$d/$<

$(HTML_DIR)/make_toc.html: $(SRCROOT)/$(MAKE_SRC_DIR)/make.texinfo
	cd $(HTML_DIR) && $(PERL) -- texi2html $(TEXI_FLAGS) $<
#	d=`pwd` && cd $(HTML_DIR) && $(PERL) -- texi2html $(TEXI_FLAGS) $$d/$<

debug: $(MAKEDIR) $(MAKEDIR)/Makefile
	cd $(MAKEDIR); $(MAKE) "CFLAGS = $(RC_CFLAGS) $($(RC_OS)_CFLAGS) -DDEBUG -g -coffonly" \
	                       "LDFLAGS= $(RC_CFLAGS) -g -coffonly"

# PDO cannot use the build_make_into_makedir target since VPATH is not supported on
# some PDO platforms, i.e. SUNOS

copy_source_to_makedir: $(OBJROOT)
	$(TAR) cf - $(MAKE_SRC_DIR) | (cd $(OBJROOT); $(TAR) xf -)


installGNUsrc: $(GNU_SRC_DIR)
	$(TAR) cf - $(MAKE_SRC_DIR) | (cd $(GNU_SRC_DIR); $(TAR) xf -)
	@if [ "winnt" != "$(RC_OS)" ]; then \
	    echo $(CHOWN) -R root $(GNU_SRC_DIR) ;\
	    ($(CHOWN) -R root $(GNU_SRC_DIR) || echo "(ignored)") ;\
	    echo $(CHGRP) -R $(INSTALL_GROUP) $(GNU_SRC_DIR) ;\
	    ($(CHGRP) -R $(INSTALL_GROUP) $(GNU_SRC_DIR) || echo "(ignored)") ;\
	fi

teflon_install nextstep_install mach_install _install: build_make_into_makedir $(NEXTSTEP_BINDIR) # build_info $(MANDIR) $(INFODIR)
	$(INSTALL) $(MAKEDIR)/$(GNU_MAKE_NAME) $(SYMROOT)/$(NEXTSTEP_MAKE_NAME)
	$(INSTALL) $(MAKEDIR)/$(GNU_MAKE_NAME) $(NEXTSTEP_BINDIR)/$(NEXTSTEP_MAKE_NAME)
	$(STRIP) $(NEXTSTEP_BINDIR)/$(NEXTSTEP_MAKE_NAME)
	-$(CHOWN) -R root $(NEXTSTEP_BINDIR)/$(NEXTSTEP_MAKE_NAME)
	-$(CHGRP) -R $(INSTALL_GROUP) $(NEXTSTEP_BINDIR)/$(NEXTSTEP_MAKE_NAME)
	$(RM) -f $(NEXTSTEP_BINDIR)/$(OLD_NEXTSTEP_MAKE_NAME)
	$(SYMLINK) $(NEXTSTEP_MAKE_NAME) $(NEXTSTEP_BINDIR)/$(OLD_NEXTSTEP_MAKE_NAME)
#	$(INSTALL) $(MAKEDIR)/$(MAKEMAN) $(MANDIR)
#	$(INSTALL) $(MAKEDIR)/$(INFOFILES) $(INFODIR)


winnt_install win95_install: build_make_into_makedir $(SYMROOT) $(WINNT_BINDIR) # build_info $(MANDIR) $(INFODIR)
	$(INSTALL) $(MAKEDIR)/$(GNU_MAKE_NAME)$(EXEC_SUFFIX) $(SYMROOT)/$(PDO_MAKE_NAME)$(EXEC_SUFFIX)
	$(INSTALL) $(MAKEDIR)/$(GNU_MAKE_NAME)$(EXEC_SUFFIX) $(WINNT_BINDIR)/$(PDO_MAKE_NAME)$(EXEC_SUFFIX)
	$(STRIP) $(WINNT_BINDIR)/$(PDO_MAKE_NAME)$(EXEC_SUFFIX)
#	$(INSTALL) $(MAKEDIR)/$(MAKEMAN) $(MANDIR)
#	$(INSTALL) $(MAKEDIR)/$(INFOFILES) $(INFODIR)

sunos_install solaris_install hpux_install osf_install: pdo_install

pdo_install: copy_source_to_makedir
	(cd $(MAKEDIR) ; CC=$(CC) ; LD=$(LD) ; AR=$(AR) ; export CC LD AR ; \
	     ./configure; $(MAKE) all "CFLAGS=-DNeXT_PDO $($(RC_OS)_CFLAGS)" "AR=$(AR)")
	$(MKDIRS) $(PDO_BINDIR)
	$(INSTALL) $(MAKEDIR)/$(GNU_MAKE_NAME) $(PDO_BINDIR)/$(PDO_MAKE_NAME)
	$(STRIP) $(PDO_BINDIR)/$(PDO_MAKE_NAME)
#	($(MKDIRS) $(LOCALDIR) ; cd $(LOCALDIR) ; \
#		 $(SYMLINK) $(RBINDIR)/$(PDO_MAKE_NAME) $(PDO_MAKE_NAME) )

build_info: $(MAKEDIR)/Makefile
	cd $(MAKEDIR); make info

$(MAKEDIR)/Makefile:
	@(PWD=`pwd` ; \
	cd $(MAKEDIR); \
	if [ -z "$(RC_OS)" ] ; then \
	   echo Hey buddy!  You need to set RC_OS. ; \
	   exit 1 ; \
	fi ; \
	if [ "$(RC_OS)" = "winnt" ] ; then \
	   echo RC_OS=$(RC_OS) ; \
	   $(CONFIGURE_ENVIRON) $$PWD/$(MAKE_SRC_DIR)/nt-configure ; \
        else \
	   echo $(CONFIGURE_ENVIRON) $$PWD/$(MAKE_SRC_DIR)/configure ; \
	   $(CONFIGURE_ENVIRON) $$PWD/$(MAKE_SRC_DIR)/configure ; \
	fi)


# If set, RC_OS will be set to something other than NEXTSTEP for NEXT's PDO builds

# FIXME: 1. Installation of info pages is disabled due to lack of
#             makeinfo tool on all platforms
#        2. Man pages should be submitted through a man page project and 
#             renamed to gnumake


installsrc: $(SRCROOT)
	$(TAR) cf - $(SOURCE_FILES) | (cd $(SRCROOT); $(TAR) xf -)
	-cd $(SRCROOT); find . -name '*~' -exec rm '{}' ';' -print -prune
	-cd $(SRCROOT); find . -type d -name 'CVS' -exec rm -r '{}' ';' -print -prune

installhdrs:

clean:
	-cd $(MAKEDIR); make distclean
	-rm -rf obj

really_clean:
	$(RM) -rf $(MAKEDIR) $(HTML_DIR)

$(MAKEDIR) $(HTML_DIR) $(SYMROOT) $(OBJROOT) $(SRCROOT) $(LOCALDIR) $(INFODIR) $(NEXTSTEP_BINDIR) $(WINNT_BINDIR) $(MANDIR) $(PRODSRCROOT) $(GNU_SRC_DIR):
	$(MKDIRS) $@

