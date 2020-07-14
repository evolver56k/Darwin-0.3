#
# Config server Makefile.
#

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

BUILD_NAME= 	Config
BIN_NAME= 	$(BUILD_NAME)
MIGSOURCE= 	Config.defs
MIGHEADER= 	Config.h
INSTALL_BIN_DIR = /usr/local/bin
INSTALL_HDR_DIR = $(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Headers/driverkit
DRIVER_PATH     = /usr/Devices
INSTALLFLAGS= -s -m 555
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

#
# OBJROOT and SYMROOT are overridden by RC tools for builds from read-only
# source.
#
OBJROOT= .
SYMROOT= .

CC = cc
MIG= mig
DEBUG_CFLAGS= -DDEBUG -DDDM_DEBUG
RELEASE_CFLAGS= 
CFLAGS= -g -O -Wall -Wnoformat -I. -I.. -MD

#
# Define target directories.
#
TARGETS=
VPATH = $(BUILD_DIR):$(OBJROOT)

CFILES = Config.c ConfigUtils.c ConfigScan.c ConfigUfs.c

MIGCFILES = ConfigServer.c
PRIVATE_HFILES = ConfigPrivate.h ConfigUtils.h ConfigScan.h ConfigUfs.h
PUBLIC_HFILES =

OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)
LIBOFILES= $(LIBCFILES:.c=.o)

SOURCEFILES= $(CFILES) $(PRIVATE_HFILES) $(PUBLIC_HFILES) Makefile \
	$(MIGSOURCE)

.SUFFIXES: .defs .h
 
# 
# Libraries.
#
RELEASE_LIBS= 		$(SYMROOT)/../libDriver/syms/libDriver.a
DEBUG_LIBS=  	 	$(SYMROOT)/../libDriver/syms/libDriver_g.a

first: 	debug
all:	$(TARGETS)

#
# Note for clean build of other modules in this project, we copy header files
# over to ../Devices, but we still install the header files from this 
# directory.
#
$(HFILES): $(LHFILES)
	cp -p $(*F).h $(INCDIR)

#
# For now, install the debug version...
#
install: DSTROOT $(DSTROOT)$(INSTALL_BIN_DIR) $(DSTROOT)$(DRIVER_PATH)

install_only: DSTROOT $(DSTROOT)$(INSTALL_BIN_DIR) $(DSTROOT)$(DRIVER_PATH)

#
# No installed headers for now.
#
#installhdrs: DSTROOT $(DSTROOT)$(INSTALL_HDR_DIR)
#	install ${INSTFLAGS} $(PUBLIC_HFILES) $(DSTROOT)$(INSTALL_HDR_DIR)
installhdrs:

kern:

installsrc: SRCROOT $(SRCROOT)
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -)	

clean: 
	rm -rf *.[aod] *~ Release Debug  $(BUILD_NAME) $(BUILD_NAME)_g tags

tags:	always
	rm -f tags
	ctags *.[mch]

release: $(OBJROOT) $(SYMROOT) always
	@if [ -n "$(RC_ARCHS)" ]; then					\
	    machines="$(RC_ARCHS)";					\
	else								\
	    machines="`/usr/bin/arch`";					\
	fi;								\
	BUILDDIR=$(OBJROOT)/Release;					\
	echo Building release version for $$machines...;		\
	for machine in $$machines; do					\
	    arch_flags="$$arch_flags -arch $$machine";			\
	done;								\
	$(MAKE) $$BUILDDIR/$(BUILD_NAME)  \
		"BUILD_DIR=$$BUILDDIR"  \
		"OBJROOT=$(OBJROOT)" \
		"OTHER_CFLAGS=$(DEBUG_CFLAGS) $$arch_flags $(RC_CFLAGS)" \
		"LIBS=$(RELEASE_LIBS)"

debug: $(OBJROOT) $(SYMROOT) always
	@if [ -n "$(RC_ARCHS)" ]; then					\
	    machines="$(RC_ARCHS)";					\
	else								\
	    machines="`/usr/bin/arch`";					\
	fi;								\
	BUILDDIR=$(OBJROOT)/Debug;					\
	echo Building debug version for $$machines...;			\
	for machine in $$machines; do					\
	    arch_flags="$$arch_flags -arch $$machine";			\
	done;								\
	$(MAKE) $$BUILDDIR/$(BUILD_NAME)  \
		"BUILD_DIR=$$BUILDDIR"  \
		"OBJROOT=$(OBJROOT)" \
		"OTHER_CFLAGS=$(DEBUG_CFLAGS) $$arch_flags $(RC_CFLAGS)" \
		"LIBS=$(DEBUG_LIBS)"

#
# Copy the defs file to the build directory because mig is so lame. 
# libDriver builds the user side.
#
$(MIGCFILES): $(MIGSOURCE)
	rm -f $(BUILD_DIR)/$(MIGSOURCE)
	cp $(MIGSOURCE) $(BUILD_DIR)
	(cd $(BUILD_DIR); \
		${MIG} ${MIGFLAGS} -user /dev/null $(MIGSOURCE))
	
always:

#
# The executable Config.
#
$(BUILD_DIR)/$(BUILD_NAME) \
$(BUILD_DIR)/$(BUILD_NAME)_g: $(BUILD_DIR) $(MIGCFILES) $(OFILES) $(LIBS) 
	$(CC) $(CFLAGS) $(OTHER_CFLAGS) -o $@ $(OFILES) $(LIBS) -lcs

.c.o:
	$(CC) $(CFLAGS) $(OTHER_CFLAGS) -c $(*F).c \
		-dependency-file $(BUILD_DIR)/$(*F).d -o $(BUILD_DIR)/$(*F).o 
	md -u $(BUILD_DIR)/Makedep -d $(BUILD_DIR)/$(*F).d

.m.o:
	$(CC) $(CFLAGS) $(OTHER_CFLAGS) -c $(*F).m \
		-dependency-file $(BUILD_DIR)/$(*F).d -o $(BUILD_DIR)/$(*F).o 
	md -u $(BUILD_DIR)/Makedep -d $(BUILD_DIR)/$(*F).d

$(SRCROOT) $(DSTROOT)$(INSTALL_HDR_DIR) $(DSTROOT)$(INSTALL_BIN_DIR) $(BUILD_DIR) $(OBJROOT) $(SYMROOT) $(DSTROOT)$(DRIVER_PATH):
	$(MKDIRS) $@

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

-include Makedep
