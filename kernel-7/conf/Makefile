##
# Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
# 
# Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
# Reserved.  This file contains Original Code and/or Modifications of
# Original Code as defined in and that are subject to the Apple Public
# Source License Version 1.1 (the "License").  You may not use this file
# except in compliance with the License.  Please obtain a copy of the
# License at http://www.apple.com/publicsource and read it before using
# this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
# License for the specific language governing rights and limitations
# under the License.
# 
# @APPLE_LICENSE_HEADER_END@
##

#
# Release build parameters
#
# installhdrs parameters
#
# Machine-dependent headers to install
# (The first of the list is the arch that is used to source the
# machine independent headers.)
#
HEADER_ARCHS = PPC I386

#
# install parameters
#
# Kernel type to install
#
INSTALL_TYPE = RELEASE

#
# Location of tftpboot directory to link kernel to.
#
TFTPDIR = /private/tftpboot

#
# "Post new_vers" parameters
#
BUILD_ARCHS = PPC I386
BUILD_TYPES = RELEASE

# TAG_ARCHS = PPC I386
# TAG_TYPE = RELEASE

#
# Defaults for build all
# Host machine -- Default architecture
#
DEFAULT_TYPE = RELEASE
DEFAULT_ARCH= `arch | awk '/ppc/ { printf("PPC") } /i386/ { printf("I386") }'`

#
# Type and architecture for this build -- often overridden from
# command line on recursive makes
#
TYPE = $(DEFAULT_TYPE)
ARCH = $(DEFAULT_ARCH)
BUILD_DIR= $(TYPE)_$(ARCH)

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

#
# Build all architectures for TYPE
#
all: tools ALWAYS
	@for arch in $(BUILD_ARCHS);					\
	do								\
		echo Making mach_kernel for $(TYPE)_$$arch;		\
		${MAKE} ARCH=$$arch TYPE=$(TYPE) mach_kernel.kernel	\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT};		\
	done

#
# Easy way to build all architectures for a particular type
#
debug release profile DEBUG RELEASE PROFILE: tools ALWAYS
	@for arch in $(BUILD_ARCHS);					\
	do								\
		type=`echo $@ | tr a-z A-Z`;				\
		${MAKE} ARCH=$$arch TYPE=$$type mach_kernel.kernel	\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT};		\
	done

#
# Easy way to build all types for a particular architecture
#
ppc PPC i386 I386 : tools ALWAYS
	@for type in $(BUILD_TYPES);					\
	do								\
		arch=`echo $@ | tr a-z A-Z`;				\
		${MAKE} ARCH=$$arch TYPE=$$type mach_kernel.kernel	\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT};		\
	done

#
# Somethings to say after a new_vers (but say it from ..)
#
kernels: tools ALWAYS
	@for type in $(BUILD_TYPES);					\
	do								\
		${MAKE} -k TYPE=$$type all				\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT};		\
	done
	$(MAKE) tags OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}

tags: tools ALWAYS
	-@for arch in $(TAG_ARCHS);					\
	do								\
		${MAKE} ARCH=$$arch TYPE=$(TAG_TYPE) tags.kernel	\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT};		\
	done


#
# Clean the world
#
clean: ALWAYS
	-@for arch in $(BUILD_ARCHS);					\
	do								\
	    for type in $(BUILD_TYPES);					\
	    do								\
		echo make clean for $${type}_$${arch};			\
	        rm -rf $(OBJROOT)/$${type}_$${arch};			\
	        rm -rf $(OBJROOT)/config.$${type}_$${arch};		\
	        rm -rf $(OBJROOT)/config.$${type}_$${arch}.old;		\
		rm -f $${type}_$${arch} $${type}_$${arch}.old;		\
	    done;							\
	done
	@CWD=`pwd`; \
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	(								\
		cd tools;						\
		${MAKE} clean						\
		  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`		\
		  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`		\
	)

#
# Clobber the world, rm tag files, too
#
clobber: clean ALWAYS
	-@for arch in $(BUILD_ARCHS);					\
	do								\
		rm -f ../tags.$$arch ../TAGS.$$arch;			\
	done;								\
	rm -rf $(OBJROOT)						\
	rm -f ../tags ../TAGS

#
# Install kernel based on RC_ARCHS for all INSTALL_TYPES
# Install kernel header files based on RC_ARCHS
#
install: tools DSTROOT installhdrs mach_kernel
	rm -f ${DSTROOT}/mach_kernel
	$(MKDIRS) ${DSTROOT}${TFTPDIR}
	install -c -m 444 ${SYMROOT}/mach_kernel ${DSTROOT}/mach_kernel
	ln -f ${DSTROOT}/mach_kernel ${DSTROOT}${TFTPDIR}/mach_kernel
	@for arch in $(RC_ARCHS);					\
	do								\
		ARCH=`echo $$arch | tr a-z A-Z`;			\
		${MAKE} ARCH=$$ARCH TYPE=$(INSTALL_TYPE)		\
		  install_md.kernel					\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}			\
		  DSTROOT=${DSTROOT};					\
	done
#
# Build FAT kernel based on RC_ARCHS for INSTALL_TYPE
#
mach_kernel: tools
	@lipo_args="";							\
	for arch in $(RC_ARCHS);					\
	do								\
		ARCH=`echo $$arch | tr a-z A-Z`;			\
		$(MAKE) ARCH=$$ARCH TYPE=$(INSTALL_TYPE)		\
		  mach_kernel.kernel					\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}			\
		  DSTROOT=${DSTROOT};					\
		lipo_args="$$lipo_args ${OBJROOT}/$(INSTALL_TYPE)_$$ARCH/mach_kernel"; \
	done;								\
	$(MKDIRS) $(SYMROOT);						\
	echo rm -f $(SYMROOT)/mach_kernel;				\
	rm -f $(SYMROOT)/mach_kernel;					\
	if [ "`echo $(RC_ARCHS) | wc -w`" -eq 1 ]; then			\
	    echo cp ${OBJROOT}/$(INSTALL_TYPE)_$$ARCH/mach_kernel 	\
		$(SYMROOT)/mach_kernel; 				\
	    cp ${OBJROOT}/$(INSTALL_TYPE)_$$ARCH/mach_kernel 		\
		$(SYMROOT)/mach_kernel; 				\
	else								\
	    echo lipo -create -o $(SYMROOT)/mach_kernel $$lipo_args;	\
	    lipo -create -o $(SYMROOT)/mach_kernel $$lipo_args;		\
	fi

#
# Install kernel header files based on HEADER_ARCHS
#
installhdrs: pre_install DSTROOT
	@set $(HEADER_ARCHS);						\
	ARCH=`echo $$1 | tr a-z A-Z`;					\
	${MAKE} ARCH=$$ARCH TYPE=$(INSTALL_TYPE) DSTROOT=$$DSTROOT	\
	  install_mi_hdrs.kernel					\
	  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}				\
	  DSTROOT=${DSTROOT}
	for arch in $(HEADER_ARCHS);					\
	do								\
		ARCH=`echo $$arch | tr a-z A-Z`;			\
		${MAKE} ARCH=$$ARCH TYPE=$(INSTALL_TYPE) DSTROOT=$$DSTROOT \
		  install_md_hdrs.kernel				\
		  OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}			\
		  DSTROOT=${DSTROOT};					\
	done

#
# Internal rules
#
VERSDIR=..

OBJROOT=$(VERSDIR)/BUILD
SYMROOT=$(VERSDIR)/BUILD

#
# Get a usable version of doconf for the installhdrs target.
#
pre_install:	ALWAYS OBJROOT
	@CWD=`pwd`;							    \
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				    \
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				    \
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				    \
	echo '#################' make $@ for tools '###################';   \
	(								    \
		cd tools;						    \
		${MAKE} pre_install					    \
		  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`		    \
		  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`		    \
		  DSTROOT=`relpath -d $$VERSDIR . $$OBJROOT`		    \
	)

tools:	ALWAYS
	@CWD=`pwd`;							\
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	(								\
		cd tools;						\
		${MAKE} install						\
		  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`		\
		  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`		\
		  DSTROOT=`relpath -d $$VERSDIR . $$OBJROOT`		\
	)

#
# Build something in the build directory making sure that the
# build directory exists!
#
install.kernel install_kernel.kernel install_md.kernel \
installhdrs.kernel install_mi_hdrs.kernel install_md_hdrs.kernel: ALWAYS
	${MAKE} ARCH=$(ARCH) TYPE=$(TYPE) arch=`echo $(ARCH) | tr A-Z a-z` \
	  builddir OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}
	@CWD=`pwd`;							\
	cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD;				\
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	echo cd $(OBJROOT)/$(BUILD_DIR);				\
	cd $(OBJROOT)/$(BUILD_DIR);					\
	echo ${MAKE} ${@:.kernel=}					\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`			\
	  DSTROOT=$$DSTROOT;						\
	${MAKE} ${@:.kernel=}						\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`			\
	  DSTROOT=$$DSTROOT

tags.kernel: ALWAYS
	${MAKE} ARCH=$(ARCH) TYPE=$(TYPE) arch=`echo $(ARCH) | tr A-Z a-z` \
	  builddir OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}
	@CWD=`pwd`;							\
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	echo cd $(OBJROOT)/$(BUILD_DIR);				\
	cd $(OBJROOT)/$(BUILD_DIR);					\
	echo ${MAKE} ${@:.kernel=}					\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`;			\
	${MAKE} ${@:.kernel=}						\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`

mach_kernel.kernel: ALWAYS
	${MAKE} ARCH=$(ARCH) TYPE=$(TYPE) arch=`echo $(ARCH) | tr A-Z a-z` \
	  builddir OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}
	@CWD=`pwd`;							\
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	echo cd $(OBJROOT)/$(BUILD_DIR);				\
	cd $(OBJROOT)/$(BUILD_DIR);					\
	echo ${MAKE} ${@:.kernel=}					\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`;			\
	${MAKE} ${@:.kernel=}						\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`;			\
	if [ $(SYMROOT) != $(OBJROOT) ];				\
	then								\
		$(MKDIRS) $$SYMROOT;					\
		echo cp mach_kernel.sys $$SYMROOT/mach_kernel.$(BUILD_DIR); \
		cp mach_kernel.sys $$SYMROOT/mach_kernel.$(BUILD_DIR);	\
	fi

boot: ALWAYS
	${MAKE} ARCH=$(ARCH) TYPE=$(TYPE) arch=`echo $(ARCH) | tr A-Z a-z` \
	  builddir OBJROOT=${OBJROOT} SYMROOT=${SYMROOT}
	@CWD=`pwd`;							\
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				\
	cd ${SYMROOT}; SYMROOT=`pwd`; cd $$CWD;				\
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				\
	echo cd $(OBJROOT)/$(BUILD_DIR);				\
	cd $(OBJROOT)/$(BUILD_DIR);					\
	echo ${MAKE} $@							\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`;			\
	${MAKE} $@							\
	  OBJROOT=`relpath -d $$VERSDIR . $$OBJROOT`			\
	  SYMROOT=`relpath -d $$VERSDIR . $$SYMROOT`

#
# Make sure the "build directory" for TYPE and ARCH exists
# and the kernel is configured
#
builddir: $(OBJROOT)/$(BUILD_DIR)/Makefile

$(OBJROOT)/$(BUILD_DIR)/Makefile: MASTER MASTER.$(arch)			\
		Makefile.template Makefile.$(arch) files.$(arch) files
	-rm -f $(OBJROOT)/$(BUILD_DIR) $(OBJROOT)/$(BUILD_DIR).old
	-rm -f $(OBJROOT)/$(BUILD_DIR)/Makefile
	$(OBJROOT)/doconf -c -cpu $(arch) -d $(OBJROOT) $(TYPE)

DSTROOT OBJROOT:	ALWAYS
	@if [ -n "${$@}" ];						\
	then								\
		exit 0;							\
	else								\
		echo Must define $@;					\
		exit 1;							\
	fi

ALWAYS:

