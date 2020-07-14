#
# 	Makefile for kernel loadable version of IOStub
#
BUILD_NAME=myHandler
INSTALL_BIN_DIR= 
INSTALL_HDR_DIR= 
INSTALLFLAGS=

#
# Define target architectures.
# MACHINE_LIST is the list of architectures to be build for during "make all"
# 	and "make install" if RC_ARCHS is not specified.
# TARGET_LIST specify the type of build to be performed during "make all" and
#	"make install".
# MACHINE and first specify the architecture and type of build for a default
#	'make' with no arguments. 
#
# Possible values of 'first' and TARGET_LIST are:
#
#	release			-- user executable
#	debug			-- user executable, DEBUG
#	kernrelease		-- .o to be linked into kernel code
#	kerndebug		-- .o to be linked into kernel code, DEBUG
#	relocrelease		-- kern-loadable
#	relocdebug		-- kern_loadable, DEBUG
#
MACHINE_LIST= 		m88k
TARGET_LIST= 		relocdebug
MACHINE= 		m88k
INSTALL_TARGETS= 
INSTALL_MACHINES=

first: relocdebug

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
KL_DEBUG_FLAGS= -d myHandler_loadable
KL_RELEASE_FLAGS=

#
# Define these for generating a kern_loadable relocatable.
#
SERVER_NAME= myHandler
KS_VAR_NAME= myHandlerInstance

MIG_DERIVED= myHandlerHandler.c
#
# The kl_ld run for kern_loadable module depends on this. It is
# typically used to do a mig run.
#
OTHER_DEPENDS= 	$(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED)

MIG_SOURCE=	myHandler.defs

#
# Copy the mig source down the the build directory...
#
$(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED):	$(MIG_SOURCE)
	rm -f $(OBJROOT)/$(BUILD_DIR)/$(MIG_DERIVED) \
		$(OBJROOT)/$(BUILD_DIR)/$(MIG_SOURCE)
	cp $(MIG_SOURCE) $(OBJROOT)/$(BUILD_DIR)
	(cd $(OBJROOT)/$(BUILD_DIR); \
		mig $(MIG_SOURCE) -handler $(MIG_DERIVED) \
		-sheader myHandlerHandler.h -user /dev/null \
		-header myHandler.h )

#
# Specify name of load_commands and unload_commands text files for 
# kern-loadable module.
#
LOAD_COMMANDS= 		Load_Commands.sect
UNLOAD_COMMANDS= 	Unload_Commands.sect

#
# Specify the source files common to all machines.
#
COMMON_MFILES= myHandler.m myDevice.m
COMMON_CFILES= myHandlerHandler.c
COMMON_PUBLIC_HFILES=
COMMON_PRIVATE_HFILES= myHandler.h myDevice.h

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

#
# Other files to installsrc.
#
OTHER_SOURCE= Load_Commands.sect Unload_Commands.sect $(MIG_SOURCE)

#
# Other files to clean.
#
OTHER_CLEAN_FILES= myHandlerHandler.c

#
# Other install rules (invoked as dependency, so this executes before 
# the main install rule).
#
other_install: always

# 
# Libraries. These may be overridden, deleted, or appended to.
#
RELEASE_LIBS=
DEBUG_LIBS=  
#RELEASE_KERN_LIBS=	$(SYMROOT)/../../libDriver/syms/libDriver_kern.o
#DEBUG_KERN_LIBS=	$(SYMROOT)/../../libDriver/syms/libDriver_kern_g.o
RELEASE_KERN_LIBS=
DEBUG_KERN_LIBS=	

#
# Typically there is nothing to install; public header files are in
# ../driverkit and binaries are linked into libDriver. This can be overridden
# to install an executable, though.
#
install: $(TARGETS)

#
# This does the rest.
#
-include ../../../MakefileDir/Makefile.device_common
