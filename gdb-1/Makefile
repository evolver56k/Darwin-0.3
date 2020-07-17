.PHONY: all clean configure build-core build-gdb build-binutils \
	build newversion install installsrc installhdrs \
	install-frameworks-pre install-frameworks-post \
	install-frameworks-pdo install-frameworks-fat \
	install-gdb-pdo install-gdb-fat \
	install-binutils-base install-binutils-pdo install-binutils-fat \
	install-chmod-rhapsody install-chmod-pdo install-clean install-source \
	check

RC_ARCHS=$(shell /usr/bin/arch)

SRCROOT=.
OBJROOT=./obj
SYMROOT=./sym
DSTROOT=./dst

INSTALL=$(SRCROOT)/gdb/install-sh

CANONICAL_ARCHS := $(foreach arch,$(RC_ARCHS),$(foreach os,$(RC_OS),$(foreach release,$(RC_RELEASE),$(os):$(arch):$(release))))

CANONICAL_ARCHS := $(subst teflon:ppc:Rhapsody,powerpc-apple-rhapsody,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst teflon:i386:Rhapsody,i386-apple-rhapsody,$(CANONICAL_ARCHS))

CANONICAL_ARCHS := $(subst teflon:ppc:Darwin,powerpc-apple-rhapsody,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst teflon:i386:Darwin,i386-apple-rhapsody,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst teflon:ppc:Hera,powerpc-apple-rhapsody,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst teflon:i386:Hera,i386-apple-rhapsody,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst macos:ppc:Beaker,powerpc-apple-macos10,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst macos:i386:Beaker,i386-apple-macos10,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst solaris:sparc:Zeus,sparc-nextpdo-solaris2,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst hpux:hppa:Zeus,hppa1.1-nextpdo-hpux10.20,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst solaris:sparc:Hydra,sparc-nextpdo-solaris2,$(CANONICAL_ARCHS))
CANONICAL_ARCHS := $(subst hpux:hppa:Hydra,hppa2.0n-nextpdo-hpux11.0,$(CANONICAL_ARCHS))

SRCTOP = $(shell cd $(SRCROOT) && pwd)
OBJTOP = $(shell (test -d $(OBJROOT) || $(INSTALL) -c -d $(OBJROOT)) && cd $(OBJROOT) && pwd)
SYMTOP = $(shell (test -d $(SYMROOT) || $(INSTALL) -c -d $(SYMROOT)) && cd $(SYMROOT) && pwd)
DSTTOP = $(shell (test -d $(DSTROOT) || $(INSTALL) -c -d $(DSTROOT)) && cd $(DSTROOT) && pwd)

GDB_VERSION=4.18-19990707
BINUTILS_VERSION=4.18-19990705
APPLE_VERSION = $(shell grep ProjectVersion $(SRCROOT)/CVSVersionInfo.txt | awk '{print $$2;}')

GDB_VERSION_STRING=$(GDB_VERSION) (Apple version gdb-$(APPLE_VERSION))
GDB_BINARY=gdb

BINUTILS_VERSION_STRING=$(BINUTILS_VERSION) (Apple version gdb-$(APPLE_VERSION))
BINUTILS_BINARIES= objdump objcopy addr2line nm-new size strings c++filt
# ar strip-new ranlib 
# nlmconv srconv sysdump coffdump

BINUTILS_MANPAGES= objdump.1 objcopy.1 addr2line.1 nm.1 size.1 strings.1 cxxfilt.man

MMALLOC_ADDRESS = 0xb9f00000
LIBERTY_ADDRESS = 0x66b00000
OPCODES_ADDRESS = 0x66d00000
BFD_ADDRESS = 0xb5700000

FRAMEWORKS=mmalloc liberty bfd opcodes

CC = cc
CC_FOR_BUILD = cc
CDEBUGFLAGS = -g
CFLAGS = $(CDEBUGFLAGS) $(RC_CFLAGS)
HARCH = UNKNOWN

RC_CFLAGS_NOARCH = $(shell echo $(RC_CFLAGS) | sed -e 's/-arch [a-z0-9]*//g')

ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
CC = cc -arch $(HARCH) -traditional-cpp
CC_FOR_BUILD = NEXT_ROOT= cc -traditional-cpp
CDEBUGFLAGS = -g -O3
CFLAGS = $(CDEBUGFLAGS) -Wall -Wimplicit $(RC_CFLAGS_NOARCH)
HARCH = $(shell echo $* | sed -e 's/--.*//' -e 's/powerpc/ppc/' -e 's/-apple-rhapsody//' -e 's/-apple-macos.*//')
endif

ifneq ($(findstring hpux,$(CANONICAL_ARCHS)),)
CC = gcc
CC_FOR_BUILD = NEXT_ROOT= cc
CC_FOR_BUILD = NEXT_ROOT= gcc
CDEBUGFLAGS = -g -O3
CFLAGS = $(CDEBUGFLAGS) -D__STDC_EXT__=1 $(RC_CFLAGS_NOARCH)
endif

ifneq ($(findstring solaris,$(CANONICAL_ARCHS)),)
CC = gcc
CC_FOR_BUILD = NEXT_ROOT= cc
CC_FOR_BUILD = NEXT_ROOT= gcc
CDEBUGFLAGS = -g -O3
CFLAGS = $(CDEBUGFLAGS) $(RC_CFLAGS_NOARCH)
endif

CONFIG_DIR=UNKNOWN
DEVEXEC_DIR=UNKNOWN
DOCUMENTATION_DIR=UNKNOWN
LIBEXEC_BINUTILS_DIR=UNKNOWN
LIBEXEC_GDB_DIR=UNKNOWN
MAN_DIR=UNKNOWN
PRIVATE_FRAMEWORKS_DIR=UNKNOWN

NATIVE_TARGET = unknown--unknown

NATIVE_TARGETS = $(foreach arch1,$(CANONICAL_ARCHS),$(arch1)--$(arch1))

CROSS_TARGETS = $(foreach arch1,$(CANONICAL_ARCHS),$(foreach arch2,$(filter-out $(arch1),$(CANONICAL_ARCHS)),$(arch1)--$(arch2)))

PPC_TARGET=UNKNOWN
I386_TARGET=UNKNOWN

CONFIG_VERBOSE=-v
CONFIG_ALL_BFD_TARGETS=--enable-targets=all
CONFIG_64_BIT_BFD=--enable-64-bit-bfd
CONFIG_WITH_MMAP=--with-mmap
CONFIG_WITH_MMALLOC=--with-mmalloc
CONFIG_MAINTAINER_MODE=--enable-maintainer-mode
CONFIG_OTHER_OPTIONS=

ifneq ($(findstring hpux,$(CANONICAL_ARCHS)),)
CONFIG_ALL_BFD_TARGETS=--enable-targets=all
CONFIG_64_BIT_BFD=--enable-64-bit-bfd
CONFIG_WITH_MMAP=--with-mmap
CONFIG_WITH_MMALLOC=
CONFIG_MAINTAINER_MODE=
endif

ifneq ($(findstring hppa1.1-nextpdo-hpux10.20,$(CANONICAL_ARCHS)),)
CONFIG_ALL_BFD_TARGETS=--enable-targets=all
CONFIG_64_BIT_BFD=
CONFIG_WITH_MMAP=
CONFIG_WITH_MMALLOC=
CONFIG_MAINTAINER_MODE=
endif

CONFIGURE_OPTIONS= \
	$(CONFIG_VERBOSE) \
	$(CONFIG_ALL_BFD_TARGETS) \
	$(CONFIG_64_BIT_BFD) \
	$(CONFIG_WITH_MMAP) \
	$(CONFIG_WITH_MMALLOC) \
	$(CONFIG_OTHER_OPTIONS)

MAKE_OPTIONS= \
	WITH_CFM= \
	WITH_FOUNDATION= \
	USE_PTHREADS=

EFLAGS = CFLAGS='$(CFLAGS)' CC='$(CC)' CC_FOR_BUILD='$(CC_FOR_BUILD)' NEXT_ROOT='$(NEXT_ROOT)' $(MAKE_OPTIONS)
MFLAGS = $(EFLAGS)

CONFIGURE_ENV = $(EFLAGS)
MAKE_ENV = $(EFLAGS)

SUBMAKE = $(MAKE_ENV) $(MAKE)

_all: all

$(OBJROOT)/%/stamp-rc-configure-pdo:
	$(RM) -r $(OBJROOT)/$*
	$(INSTALL) -c -d $(OBJROOT)/$*
	(cd $(OBJROOT)/$* && \
		$(CONFIGURE_ENV) $(SRCTOP)/gdb/configure \
			--host=$(shell echo $* | sed -e 's/--.*//') \
			--target=$(shell echo $* | sed -e 's/.*--//') \
			$(CONFIGURE_OPTIONS) \
			)
	touch $@

$(OBJROOT)/%/stamp-rc-configure:
	$(RM) -r $(OBJROOT)/$*
	$(INSTALL) -c -d $(OBJROOT)/$*
	(cd $(OBJROOT)/$* && \
		$(CONFIGURE_ENV) $(SRCTOP)/gdb/configure \
			--host=$(shell echo $* | sed -e 's/--.*//') \
			--target=$(shell echo $* | sed -e 's/.*--//') \
			$(CONFIGURE_OPTIONS) \
			)
	touch $@

$(OBJROOT)/%/stamp-rc-configure-cross:
	$(RM) -r $(OBJROOT)/$*
	$(INSTALL) -c -d $(OBJROOT)/$*/gdb
	(cd $(OBJROOT)/$*/gdb && \
		$(CONFIGURE_ENV) $(SRCTOP)/gdb/gdb/configure \
			--host=$(shell echo $* | sed -e 's/--.*//') \
			--target=$(shell echo $* | sed -e 's/.*--//') \
			$(CONFIGURE_OPTIONS) \
			)
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/libiberty $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/bfd $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/electric-fence $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/readline $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/opcodes $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/mmalloc $(OBJROOT)/$*/
	ln -sf ../$(shell echo $* | sed -e 's/\(.*\)--.*/\1--\1/')/texinfo $(OBJROOT)/$*/
	touch $@

$(OBJROOT)/%/stamp-build-gdb-pdo:
	$(SUBMAKE) -C $(OBJROOT)/$*/readline $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/electric-fence $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/mmalloc $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/libiberty $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/bfd $(MFLAGS) headers all
	$(SUBMAKE) -C $(OBJROOT)/$*/opcodes $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/gdb $(MFLAGS) VERSION='$(GDB_VERSION_STRING)' 
	#touch $@

$(OBJROOT)/%/stamp-build-core:
	$(SUBMAKE) -C $(OBJROOT)/$*/readline $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/electric-fence $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/intl $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/mmalloc $(MFLAGS) FRAMEWORK_ADDRESS=$(MMALLOC_ADDRESS) all stamp-framework 
	$(SUBMAKE) -C $(OBJROOT)/$*/libiberty $(MFLAGS) FRAMEWORK_ADDRESS=$(LIBERTY_ADDRESS) all stamp-framework
	$(SUBMAKE) -C $(OBJROOT)/$*/bfd $(MFLAGS) FRAMEWORK_ADDRESS=$(BFD_ADDRESS) headers all stamp-framework
	$(SUBMAKE) -C $(OBJROOT)/$*/opcodes $(MFLAGS) FRAMEWORK_ADDRESS=$(OPCODES_ADDRESS) all stamp-framework
	#touch $@

$(OBJROOT)/%/stamp-build-gdb:
	$(SUBMAKE) -C $(OBJROOT)/$*/gdb $(MFLAGS) VERSION='$(GDB_VERSION_STRING)' \
		MMALLOC='-F../mmalloc -framework mmalloc' \
		OPCODES='-F../opcodes -framework opcodes' \
		BFD='-F../bfd -framework bfd' \
		LIBIBERTY='-F../libiberty -framework liberty'

$(OBJROOT)/%/stamp-build-gdb-docs:
	$(SUBMAKE) -C $(OBJROOT)/$*/texinfo $(MFLAGS) CC='$(CC_FOR_BUILD)' all
	$(SUBMAKE) -C $(OBJROOT)/$*/gdb/doc $(MFLAGS) VERSION='$(GDB_VERSION_STRING)' \
		gdb.info
	$(SUBMAKE) -C $(OBJROOT)/$*/bfd/doc $(MFLAGS) VERSION='$(GDB_VERSION_STRING)' \
		bfd.info
	#touch $@

$(OBJROOT)/%/stamp-build-testsuite:
	$(SUBMAKE) -C $(OBJROOT)/$*/tcl $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/expect $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/dejagnu $(MFLAGS)

$(OBJROOT)/%/stamp-build-binutils:
	$(SUBMAKE) -C $(OBJROOT)/$*/binutils $(MFLAGS) VERSION='$(BINUTILS_VERSION_STRING)' \
		MMALLOC='-F../mmalloc -framework mmalloc' \
		OPCODES='-F../opcodes -framework opcodes' \
		BFD='-F../bfd -framework bfd' \
		LIBIBERTY='-F../libiberty -framework liberty'
	#touch $@

install-frameworks-pre:

	$(INSTALL) -c -d $(SYMROOT)/$(PRIVATE_FRAMEWORKS_DIR)
	$(INSTALL) -c -d $(DSTROOT)/$(PRIVATE_FRAMEWORKS_DIR)

	set -e;	for i in $(FRAMEWORKS); do \
		for framedir in $(DSTROOT)/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework \
				$(SYMROOT)/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework; do \
			$(RM) -r $${framedir}; \
			$(INSTALL) -c -d $${framedir}/Versions/A/PrivateHeaders; \
			$(INSTALL) -c -d $${framedir}/Versions/A/Headers; \
			$(INSTALL) -c -d $${framedir}/Versions/A/Resources; \
			$(INSTALL) -c -d $${framedir}/Versions/A/Resources/English.lproj; \
			ln -sf A $${framedir}/Versions/Current; \
			ln -sf Versions/Current/$${i} $${framedir}/$${i}; \
			ln -sf Versions/Current/PrivateHeaders $${framedir}/PrivateHeaders; \
			ln -sf Versions/Current/Headers $${framedir}/Headers; \
			ln -sf Versions/Current/Resources $${framedir}/Resources; \
		done; \
	done

install-frameworks-post:

	set -e; for i in $(FRAMEWORKS); do \
		strip -S -o $(DSTROOT)/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework/Versions/A/$${i} \
			 $(SYMROOT)/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework/Versions/A/$${i}; \
	done

install-frameworks-rhapsody:

	$(SUBMAKE) install-frameworks-pre

	set -e; for i in $(FRAMEWORKS); do \
		j=`echo $${i} | sed -e 's/liberty/libiberty/;'`; \
		lipo -create -output $(SYMROOT)/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework/Versions/A/$${i} \
			$(patsubst %,$(OBJROOT)/%/$${j}/$${i}.framework/Versions/A/$${i},$(NATIVE_TARGETS)); \
	done

	$(SUBMAKE) install-frameworks-post

install-frameworks-pdo:

	true

install-gdb-common:

	set -e; for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		$(INSTALL) -c -d $${dstroot}/$(DEVEXEC_DIR); \
		$(INSTALL) -c -d $${dstroot}/$(CONFIG_DIR); \
		$(INSTALL) -c -d $${dstroot}/$(MAN_DIR); \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/gdb; \
		\
		$(INSTALL) -c -d $${docroot}; \
		\
		$(INSTALL) -c -m 644 $(SRCROOT)/gdb/gdb/gdb.1 $${docroot}/gdb.1; \
		\
		$(INSTALL) -c -m 644 $(SRCROOT)/refcard.ps $${docroot}/refcard.ps; \
		$(INSTALL) -c -m 644 $(SRCROOT)/refcard.pdf $${docroot}/refcard.pdf; \
		\
	done;

install-gdb-pdo: install-gdb-common

	set -e; for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/gdb; \
		\
		for i in gdb gdbint stabs; do \
			cp -rp $(SRCROOT)/html/$${i} $${docroot}/; \
		done; \
		\
		$(INSTALL) -c -d $${dstroot}/$(MAN_DIR)/man1; \
		$(INSTALL) -c -m 644 $(SRCROOT)/gdb/gdb/gdb.1 $${dstroot}/$(MAN_DIR)/man1/gdb.1; \
		\
		$(INSTALL) -c -d $${dstroot}/$(CONFIG_DIR); \
		$(INSTALL) -c -m 644 $(SRCROOT)/gdb.conf $${dstroot}/$(CONFIG_DIR)/gdb.conf; \
	done;

	$(INSTALL) -c $(OBJROOT)/$(NATIVE_TARGET)/gdb/gdb $(SYMROOT)/$(DEVEXEC_DIR)/gdb
	$(INSTALL) -c -s $(OBJROOT)/$(NATIVE_TARGET)/gdb/gdb $(DSTROOT)/$(DEVEXEC_DIR)/gdb

install-gdb-rhapsody-common: install-gdb-common

	set -e; for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		$(INSTALL) -c -d $${dstroot}/$(LIBEXEC_GDB_DIR); \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/gdb; \
		\
		for i in gdb gdbint stabs; do \
			$(INSTALL) -c -d $${docroot}/$${i}; \
			(cd $${docroot}/$${i} && \
				$(SRCROOT)/texi2html \
					-split_chapter \
					-I $(OBJROOT)/$(firstword $(NATIVE_TARGETS))/gdb/doc \
					$(SRCROOT)/gdb/gdb/doc/$${i}.texinfo); \
		done; \
		\
		$(INSTALL) -c -d $${dstroot}/$(MAN_DIR)/man1; \
		$(INSTALL) -c -m 644 $(SRCROOT)/gdb/gdb/gdb.1 $${dstroot}/$(MAN_DIR)/man1/gdb.1; \
		\
		$(INSTALL) -c -d $${dstroot}/$(CONFIG_DIR); \
		$(INSTALL) -c -m 644 $(SRCROOT)/gdb.conf $${dstroot}/$(CONFIG_DIR)/gdb.conf; \
		\
		sed -e 's/version=.*/version=$(GDB_VERSION)-$(APPLE_VERSION)/' \
			< $(SRCROOT)/gdb.sh > $${dstroot}/usr/bin/gdb; \
		chmod 755 $${dstroot}/usr/bin/gdb; \
		\
	done;

install-gdb-rhapsody: install-gdb-rhapsody-common

	set -e; for target in $(CANONICAL_ARCHS); do \
		lipo -create $(OBJROOT)/$${target}--$${target}/gdb/gdb \
			-output $(SYMROOT)/$(LIBEXEC_GDB_DIR)/gdb-$${target}; \
	 	strip -o $(DSTROOT)/$(LIBEXEC_GDB_DIR)/gdb-$${target} \
			$(SYMROOT)/$(LIBEXEC_GDB_DIR)/gdb-$${target}; \
	done

install-gdb-fat: install-gdb-rhapsody-common

	lipo -create $(patsubst %,$(OBJROOT)/%/gdb/gdb,$(PPC_TARGET)--$(PPC_TARGET) $(I386_TARGET)--$(PPC_TARGET)) \
		-output $(SYMROOT)/$(LIBEXEC_GDB_DIR)/gdb-$(PPC_TARGET)
	lipo -create $(patsubst %,$(OBJROOT)/%/gdb/gdb,$(PPC_TARGET)--$(I386_TARGET) $(I386_TARGET)--$(I386_TARGET)) \
		-output $(SYMROOT)/$(LIBEXEC_GDB_DIR)/gdb-$(I386_TARGET)

	set -e; for target in $(CANONICAL_ARCHS); do \
	 	strip -o $(DSTROOT)/$(LIBEXEC_GDB_DIR)/gdb-$${target} \
			$(SYMROOT)/$(LIBEXEC_GDB_DIR)/gdb-$${target}; \
	done

install-binutils-common:

	set -e; for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/binutils; \
		\
		$(INSTALL) -c -d $${docroot}/binutils; \
		\
		for i in $(BINUTILS_MANPAGES); do \
			$(INSTALL) -c -m 644 $(SRCROOT)/gdb/binutils/$${i} $${docroot}/`echo $${i} | sed -e 's/cxxfilt\\.man/c++filt.1/`; \
		done; \
	done;

install-binutils-rhapsody-common: install-binutils-common

	set -e; for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		$(INSTALL) -c -d $${dstroot}/$(LIBEXEC_BINUTILS_DIR); \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/binutils; \
		\
		(cd $${docroot}/binutils && \
			$(SRCROOT)/texi2html $(SRCROOT)/gdb/binutils/binutils.texi); \
	done

	set -e; for i in $(BINUTILS_BINARIES); do \
		instname=`echo $${i} | sed -e 's/\\-new//'`; \
		lipo -create $(patsubst %,$(OBJROOT)/%/binutils/$${i},$(NATIVE_TARGETS)) \
			-output $(SYMROOT)/$(LIBEXEC_BINUTILS_DIR)/$${instname}; \
		strip -o $(DSTROOT)/$(LIBEXEC_BINUTILS_DIR)/$${instname} $(SYMROOT)/$(LIBEXEC_BINUTILS_DIR)/$${instname}; \
	done

install-binutils-fat: install-binutils-rhapsody-common

install-binutils-rhapsody: install-binutils-rhapsody-common

install-binutils-pdo: install-binutils-common

	set -e;	for dstroot in $(SYMROOT) $(DSTROOT); do \
		\
		docroot=$${dstroot}/$(DOCUMENTATION_DIR)/binutils; \
		\
		cp -rp $(SRCROOT)/html/binutils $${docroot}/; \
	done

	set -e; for i in $(BINUTILS_BINARIES); do \
		instname=`echo $${i} | sed -e 's/\\.new//'`; \
		$(INSTALL) -c $(patsubst %,$(OBJROOT)/%/binutils/$${i},$(NATIVE_TARGET)) \
			$(SYMROOT)/$(LIBEXEC_BINUTILS_DIR)/$${instname}; \
		$(INSTALL) -c -s $(patsubst %,$(OBJROOT)/%/binutils/$${i},$(NATIVE_TARGET)) \
			$(DSTROOT)/$(LIBEXEC_BINUTILS_DIR)/$${instname}; \
	done

install-chmod-rhapsody:
	set -e;	if [ `whoami` = 'root' ]; then \
		for dstroot in $(SYMROOT) $(DSTROOT); do \
			chown -R root.wheel $${dstroot}; \
			chmod -R  u=rwX,g=rX,o=rX $${dstroot}; \
			for i in $(FRAMEWORKS); do \
				chmod a+x $${dstroot}/$(PRIVATE_FRAMEWORKS_DIR)/$${i}.framework/Versions/A/$${i}; \
			done; \
			chmod a+x $${dstroot}/$(LIBEXEC_GDB_DIR)/*; \
			chmod a+x $${dstroot}/$(LIBEXEC_BINUTILS_DIR)/*; \
			chmod a+x $${dstroot}/$(DEVEXEC_DIR)/*; \
		done; \
	fi

install-chmod-pdo:
	set -e;	if [ `whoami` = 'root' ]; then \
		true; \
	fi

install-source:
	$(INSTALL) -c -d $(DSTROOT)/$(SOURCE_DIR)
	#gnutar --exclude=CVS -C $(SRCROOT) -cf - . | gnutar -C $(DSTROOT)/$(SOURCE_DIR) -xf -

all: build

clean:
	$(RM) -r $(OBJROOT)

newversion:
	$(RM) $(OBJROOT)/*/gdb/version.c
	$(RM) $(OBJROOT)/*/binutils/version.c

check-args:
ifeq "$(CANONICAL_ARCHS)" "i386-apple-rhapsody"
else
ifeq "$(CANONICAL_ARCHS)" "powerpc-apple-rhapsody"
else
ifeq "$(CANONICAL_ARCHS)" "i386-apple-macos10"
else
ifeq "$(CANONICAL_ARCHS)" "powerpc-apple-macos10"
else
ifeq "$(CANONICAL_ARCHS)" "i386-apple-rhapsody powerpc-apple-rhapsody"
else
ifeq "$(CANONICAL_ARCHS)" "powerpc-apple-rhapsody i386-apple-rhapsody"
else
ifeq "$(CANONICAL_ARCHS)" "i386-apple-macos10 powerpc-apple-macos10"
else
ifeq "$(CANONICAL_ARCHS)" "hppa1.1-nextpdo-hpux10.20"
else
ifeq "$(CANONICAL_ARCHS)" "hppa2.0n-nextpdo-hpux11.0"
else
ifeq "$(CANONICAL_ARCHS)" "sparc-nextpdo-solaris2"
else
	echo "Unknown architecture string: \"$(CANONICAL_ARCHS)\""
	exit 1
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

configure: 
ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-rc-configure, $(NATIVE_TARGETS))
endif
ifneq ($(CROSS_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-rc-configure-cross, $(CROSS_TARGETS))
endif
else
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-rc-configure-pdo, $(NATIVE_TARGETS))
endif
endif

build-gdb-pdo:
	$(SUBMAKE) configure
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-gdb-pdo, $(NATIVE_TARGETS)) 
endif

build-core:
	$(SUBMAKE) configure
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-core, $(NATIVE_TARGETS)) 
endif

build-gdb:
	$(SUBMAKE) configure
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-gdb, $(NATIVE_TARGETS))
endif
ifneq ($(CROSS_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-gdb, $(CROSS_TARGETS))
endif

build-gdb-docs:
	$(MAKE) configure
ifneq ($(NATIVE_TARGETS),)
	$(MAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-gdb-docs, $(NATIVE_TARGETS))
endif
ifneq ($(CROSS_TARGETS),)
	$(MAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-gdb-docs, $(CROSS_TARGETS))
endif

build-binutils:
	$(SUBMAKE) configure
ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-binutils, $(NATIVE_TARGETS))
endif
endif

build-testsuite:
ifneq ($(NATIVE_TARGETS),)
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-testsuite, $(NATIVE_TARGETS))
endif

build:
	$(SUBMAKE) check-args
ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
	$(SUBMAKE) build-core
	$(SUBMAKE) build-gdb 
	$(SUBMAKE) build-gdb-docs 
	$(SUBMAKE) build-binutils
#	$(SUBMAKE) build-testsuite 
else
	$(SUBMAKE) build-gdb-pdo
#	$(SUBMAKE) build-binutils
#	$(SUBMAKE) build-testsuite 
endif

install-clean:
	$(RM) -r $(SYMROOT) $(DSTROOT)

install-rhapsody:
	$(SUBMAKE) install-clean
ifeq "$(CANONICAL_ARCHS)" "i386-apple-rhapsody powerpc-apple-rhapsody"
	$(SUBMAKE) install-frameworks-rhapsody NATIVE_TARGET=unknown--unknown
	$(SUBMAKE) install-gdb-fat NATIVE_TARGET=unknown--unknown PPC_TARGET=powerpc-apple-rhapsody I386_TARGET=i386-apple-rhapsody
	$(SUBMAKE) install-binutils-fat NATIVE_TARGET=unknown--unknown
else
ifeq "$(CANONICAL_ARCHS)" "i386-apple-macos10 powerpc-apple-macos10"
	$(SUBMAKE) install-frameworks-rhapsody NATIVE_TARGET=unknown--unknown
	$(SUBMAKE) install-gdb-fat NATIVE_TARGET=unknown--unknown PPC_TARGET=powerpc-apple-macos10 I386_TARGET=i386-apple-macos10
	$(SUBMAKE) install-binutils-fat NATIVE_TARGET=unknown--unknown
else
	$(SUBMAKE) install-frameworks-rhapsody
	$(SUBMAKE) install-gdb-rhapsody
	$(SUBMAKE) install-binutils-rhapsody
endif
endif
	$(SUBMAKE) install-chmod-rhapsody

install-pdo:
	$(SUBMAKE) install-clean
	$(SUBMAKE) install-gdb-pdo NATIVE_TARGET=$(NATIVE_TARGETS)
#	$(SUBMAKE) install-binutils-pdo NATIVE_TARGET=$(NATIVE_TARGETS)
	$(SUBMAKE) install-chmod-pdo


RHAPSODY_FLAGS = \
	CONFIG_DIR=private/etc \
	DEVEXEC_DIR=usr/bin \
	DOCUMENTATION_DIR=System/Documentation/Developer/Commands \
	LIBEXEC_BINUTILS_DIR=usr/libexec/binutils \
	LIBEXEC_GDB_DIR=usr/libexec/gdb \
	MAN_DIR=usr/share/man \
	PRIVATE_FRAMEWORKS_DIR=System/Library/PrivateFrameworks \
	SOURCE_DIR=System/Developer/Source/Commands/gdb

PDO_FLAGS = \
	CONFIG_DIR=Developer/Libraries/gdb \
	DEVEXEC_DIR=Developer/Executables \
	DOCUMENTATION_DIR=Documentation/Developer/Commands \
	LIBEXEC_BINUTILS_DIR=Developer/Libraries/binutils \
	LIBEXEC_GDB_DIR=Developer/Libraries/gdb \
	MAN_DIR=Local/man \
	PRIVATE_FRAMEWORKS_DIR=UNKNOWN \
	SOURCE_DIR=Developer/Source/gdb

install-src:
	$(SUBMAKE) check-args
	$(SUBMAKE) build
ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
	$(SUBMAKE) $(RHAPSODY_FLAGS) install-rhapsody
	$(SUBMAKE) $(RHAPSODY_FLAGS) install-source
	$(SUBMAKE) $(RHAPSODY_FLAGS) install-chmod-rhapsody
else
	$(SUBMAKE) $(PDO_FLAGS) install-pdo
	$(SUBMAKE) $(PDO_FLAGS) install-source
	$(SUBMAKE) $(PDO_FLAGS) install-chmod-pdo
endif

install-nosrc:
	$(SUBMAKE) check-args
	$(SUBMAKE) build
ifneq ($(findstring rhapsody,$(CANONICAL_ARCHS))$(findstring macos10,$(CANONICAL_ARCHS)),)
	$(SUBMAKE) $(RHAPSODY_FLAGS) install-rhapsody
else
	$(SUBMAKE) $(PDO_FLAGS) install-pdo
endif

install:
ifeq ($(INSTALL_SOURCE),no)
	$(SUBMAKE) install-nosrc
else	
	$(SUBMAKE) install-src
endif

installsrc:
	$(SUBMAKE) check
	#gnutar --exclude=CVS -cf - . | gnutar -C $(SRCROOT) -xf -

installhdrs:
	@echo

check:
	[ -z `find . -name \*~` ] || \
		(echo 'Emacs-style backup files present; not copying.' && exit 1)

$(OBJROOT)/%/stamp-build-bare-gdb:
	$(SUBMAKE) -C $(OBJROOT)/$*/electric-fence $(MFLAGS)
	$(SUBMAKE) -C $(OBJROOT)/$*/readline $(MFLAGS) all
	$(SUBMAKE) -C $(OBJROOT)/$*/gdb $(MFLAGS) VERSION='$(GDB_VERSION_STRING)' \
		LDFLAGS=-F$(PRIVATE_FRAMEWORKS_DIR) \
		OPCODES='-framework opcodes' OPCODES_DEP= \
		LIBIBERTY='-framework liberty' LIBIBERTY_DEP= \
		MMALLOC='-framework mmalloc' MMALLOC_DEP= \
		BFD='-framework bfd' BFD_DEP= \
		BFD_DIR=$(PRIVATE_FRAMEWORKS_DIR)/bfd.framework/PrivateHeaders \
		BFD_SRC=$(SRCTOP)/gdb/bfd

build-bare-gdb:
	$(SUBMAKE) $(patsubst %,$(OBJROOT)/%/stamp-build-bare-gdb, $(NATIVE_TARGETS))

bare-gdb:
	$(SUBMAKE) configure build-bare-gdb \
		CONFIG_ALL_BFD_TARGETS= \
		CONFIG_VERBOSE=--silent \
		CONFIG_OTHER_OPTIONS='--only=electric-fence,readline,gdb --config-gdb=--no-recursion' \
		PRIVATE_FRAMEWORKS_DIR=$(NEXT_ROOT)/System/Library/PrivateFrameworks

min-gdb:
	$(SUBMAKE) configure build-core build-gdb \
		CONFIG_ALL_BFD_TARGETS= \
		CONFIG_VERBOSE=--silent \
		CONFIG_OTHER_OPTIONS='--only=electric-fence,mmalloc,readline,libiberty,bfd,opcodes,gdb --config-gdb=--no-recursion --config-bfd=--no-recursion'

