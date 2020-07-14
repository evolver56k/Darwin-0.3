#
# 	Makefile template for use by driverkit-based driver projects.
#

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

BUILD_NAME= myDriver

#
# Destination of installed binaries and headers.
#
INSTALL_BIN_DIR= 
INSTALL_HDR_DIR= 

#
# Destination of installed headers.
#
HEADER_DIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Headers/drivers/

#
# Options to install.
#
INSTALLFLAGS=

#
# Define target architectures.
# MACHINE_LIST is the list of architectures to be build for during "make all"
# 	and "make install" if RC_ARCHS is not specified in the Make command
#	line and RC_ARCHS_LOCAL is not specified in this Makefile.
# RC_ARCHS_LOCAL, if specified, overrides RC_ARCHS. This allows builds
#	to be limited to a particular architecture in case of fat RC 
#	builds.
# TARGET_LIST specifies the types of build to be performed during 
#	"make all" and "make install".
# MACHINE and 'first' specify the architecture and type of build for a default
#	'make' with no arguments. 
# INSTALL_TARGETS and INSTALL_MACHINES specify the actual target and machine 
#	combinations installed in INSTALL_DIR by the 'make install' target.
#
# Note that INSTALL_TARGETS and INSTALL_MACHINES must be subsets of 
# TARGET_LIST and MACHINE_LIST, respectively. Either one (or both) 
# can be null, resulting in no installation.
#
# Possible values of 'first', TARGET_LIST, and INSTALL_TARGETS are:
#
#	release			-- user executable
#	debug			-- user executable, DEBUG
#	relocrelease		-- kern-loadable
#	relocdebug		-- kern_loadable, DEBUG
#
MACHINE_LIST= 		m88k
TARGET_LIST= 		debug
MACHINE= 		m88k
INSTALL_TARGETS= 
INSTALL_MACHINES=
first: 			debug

#
# Define machine-dependent subdirectories. Leave commented out if none.
#
#SOURCE_SUBDIRS=

#
# Optional CFLAGS for cc runs.
#
#DEBUG_CFLAGS=
#RELEASE_CFLAGS=
#
# These are for local build within the driverkit project...
#
DEBUG_CFLAGS= -I../../..
RELEASE_CFLAGS= -I../../..

#
# Specify LD and *_LDFLAGS for either executable or relocatable.
#
# For generating relocatable .o:
#
#LD= /bin/ld
#DEBUG_LDFLAGS= -r
#RELEASE_LDFLAGS= -r

#
# For generating executable:
#
LD= /bin/cc
DEBUG_LDFLAGS= 
RELEASE_LDFLAGS=

#
# Options to kl_ld for generating kern-loadable.
#
KL_DEBUG_FLAGS= 
KL_RELEASE_FLAGS=

#
# Specify name of load_commands and unload_commands text files for 
# kern-loadable module.
#
LOAD_COMMANDS=
UNLOAD_COMMANDS= 

#
# Define these for generating a kern_loadable relocatable.
#
SERVER_NAME=
KS_VAR_NAME=

OTHER_DEPENDS=

#
# Additional files to be cleaned.
#
OTHER_CLEAN_FILES=

#
# Specify the source files common to all machines.
#
COMMON_MFILES= kl_com.m
COMMON_CFILES= myDriver.c myHandlerUser.c
COMMON_PUBLIC_HFILES= 
COMMON_PRIVATE_HFILES= kl_com.h

#
# one set of source files sharing a common name in the machine-dependent 
# build directories. 
#
MACHINE_COMMON_CFILES=
MACHINE_COMMON_MFILES=

#
# optionally, additional source files specific to each machine.
#
# 88k specific...
m88k_CFILES=
m88k_MFILES=
# 68k specific...
m68k_CFILES=
m68k_MFILES=
# i386 specific...
i386_CFILES=
i386_MFILES=

#
# User, kernel, and kern-loadable specific source.
#
USER_MFILES=
USER_CFILES=
KERNEL_MFILES=
KERNEL_CFILES=
LOADABLE_MFILES=
LOADABLE_CFILES=

MIG_DERIVED= myHandlerUser.c

OTHER_DEPENDS= 	$(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED)

MIG_SOURCE=	myHandler.defs

#
# Copy the mig source down the the build directory...
#
$(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED):	../Kernel/$(MIG_SOURCE)
	rm -f $(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED) \
		$(OBJROOT)/$(BUILD_DIR)/$(MIG_SOURCE)
	cp ../Kernel/$(MIG_SOURCE) $(OBJROOT)/$(BUILD_DIR)
	(cd $(OBJROOT)/$(BUILD_DIR); \
		mig $(MIG_SOURCE) -user $(MIG_DERIVED) \
		-sheader /dev/null -handler /dev/null)


#
# Other files to installsrc.
#
OTHER_SOURCE=

#
# Other install rules (invoked as dependency, so this executes before 
# the main install rule).
#
other_install: always

# 
# Libraries. These may be overridden, deleted, or appended to.
#
#LIB_DIR= /usr/lib/
LIB_DIR= $(SYMROOT)/../../../libDriver/syms/

RELEASE_LIBS= 		$(LIB_DIR)/libDriver.a -lkernload
DEBUG_LIBS=  	 	$(LIB_DIR)/libDriver_g.a -lkernload
RELEASE_KERN_LIBS=
DEBUG_KERN_LIBS=	

#
# This does the rest.
#
-include ../../../MakefileDir/Makefile.device_common
