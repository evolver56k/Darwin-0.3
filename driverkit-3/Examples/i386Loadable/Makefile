#
# 	Makefile for kernel loadable NullDriver
#
BUILD_NAME=NullDriver
INSTALL_BIN_DIR= /usr/Devices/NullDriver.config
#INSTALL_BIN_DIR= 
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
MACHINE_LIST= 		
TARGET_LIST= 		relocrelease
MACHINE= 		i386
INSTALL_TARGETS= 	relocrelease
#INSTALL_TARGETS=
INSTALL_MACHINES=	i386

first: relocrelease

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
DEBUG_CFLAGS= -I../../
RELEASE_CFLAGS= -I../../

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
KL_DEBUG_FLAGS= -d NullDriver_loadable
KL_RELEASE_FLAGS=

#
# Define these for generating a kern_loadable relocatable.
#
SERVER_NAME= NullDriver
KS_VAR_NAME= NullDriverInstance

MIG_DERIVED= 
#
# The kl_ld run for kern_loadable module depends on this. It is
# typically used to do a mig run.
#
OTHER_DEPENDS= 

#
# Optional targets...
#
prePost: preLoad postLoad

preLoad: preLoad.o
	cc $(RC_CFLAGS) -o $@ $(*F).o -lDriver
postLoad: postLoad.o
	cc $(RC_CFLAGS) -o $@ $(*F).o -lDriver

#
# Specify name of load_commands and unload_commands text files for 
# kern-loadable module. The LOAD_COMMANDS files is mandatory; the 
# UNLOAD_COMMANDS file is optional - leave UNLOAD_COMMANDS commented out
# if no file needed.
#
LOAD_COMMANDS= 		Load_Commands.sect
#UNLOAD_COMMANDS= 	Unload_Commands.sect

#
# Specify the source files common to all machines.
#
COMMON_MFILES= NullDriver.m
COMMON_CFILES= 
COMMON_PUBLIC_HFILES=
COMMON_PRIVATE_HFILES= NullDriver.h

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

OTHER_INSTALL= Instance0.table preLoad postLoad

#
# Other files to installsrc.
#
OTHER_SOURCE= $(OTHER_INSTALL)

#
# Other files to clean.
#
OTHER_CLEAN_FILES=

#
# Other install rules (invoked as dependency, so this executes before 
# the main install rule).
#
other_install: always preLoad postLoad
	install $(INSTALLFLAGS) $(OTHER_INSTALL)			\
		    $(DSTROOT)$(INSTALL_BIN_DIR)

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
# This does the rest.
#
-include ../../MakefileDir/Makefile.device_common
