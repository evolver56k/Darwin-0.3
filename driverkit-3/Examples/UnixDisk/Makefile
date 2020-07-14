#
# 	Makefile for unixDisk
#
BUILD_NAME=unixdisk

#
# Destination of installed binaries and headers.
#
INSTALL_BIN_DIR= 
INSTALL_HDR_DIR= 

#
# Destination of installed headers.
#
HEADER_DIR=

#
# Options to install.
#
INSTALLFLAGS=

#
# Define target architectures.
# MACHINE_LIST is the list of architectures to be build for during "make all"
# 	and "make install" if RC_ARCHS is not specified.
# TARGET_LIST specify the type of build to be performed during "make all" and
#	"make install".
# MACHINE and 'first' specify the architecture and type of build for a default
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
MACHINE_LIST= 		m68k
TARGET_LIST= 		debug release
MACHINE= 		m68k
INSTALL_TARGETS= 
INSTALL_MACHINES=
first: debug

#
# Leave undefined if no machine-dependent subdirectories.
#
#SOURCE_SUBDIRS=

DEBUG_CFLAGS= 	-I../..
RELEASE_CFLAGS= -I../..

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
# For generating kern-loadable.
#
DEBUG_KL_FLAGS=
RELEASE_KL_FLAGS=


#
# Specify the source files common to all machines.
#
COMMON_MFILES= FloppyDisk.m unixDisk.m unixThread.m SCSIDisk.m \
	unixDiskPrivate.m main.m
COMMON_CFILES= 
COMMON_PUBLIC_HFILES=
COMMON_PRIVATE_HFILES= unixDiskPrivate.h unixThread.h unixDiskUxpr.h \
	unixDiskTypes.h FloppyDisk.h unixDisk.h SCSIDisk.h

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
# Other install rules (invoked as dependency, so this executes before 
# the main install rule).
#
other_install: always

# 
# Libraries. These may be overridden, deleted, or appended to.
#
RELEASE_LIBS= 		$(SYMROOT)/../../libDriver/syms/libDriver.a
DEBUG_LIBS=  	 	$(SYMROOT)/../../libDriver/syms/libDriver_g.a

#
# This does the rest.
#
-include ../../MakefileDir/Makefile.device_common
