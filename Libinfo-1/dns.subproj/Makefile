#
# Generated by the NeXT Project Builder.
#
# NOTE: Do NOT change this file -- Project Builder maintains it.
#
# Put all of your customizations in files called Makefile.preamble
# and Makefile.postamble (both optional), and Makefile will include them.
#

NAME = dns

PROJECTVERSION = 2.7
PROJECT_TYPE = Component

HFILES = inet.h nameser.h options.h portability.h resolv.h

CFILES = gethnamaddr.c getnetbyaddr.c getnetbyname.c getnetent.c\
         getnetnamadr.c herror.c nsap_addr.c res_comp.c res_data.c\
         res_debug.c res_init.c res_mkquery.c res_query.c res_send.c\
         sethostent.c

OTHERSRCS = Makefile.preamble Makefile Makefile.postamble


MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
CODE_GEN_STYLE = DYNAMIC
MAKEFILE = subproj.make
NEXTSTEP_INSTALLDIR = /usr/local/lib
WINDOWS_INSTALLDIR = /usr/local/lib
PDO_UNIX_INSTALLDIR = /usr/local/lib
LIBS = 
DEBUG_LIBS = $(LIBS)
PROF_LIBS = $(LIBS)


NEXTSTEP_PB_CFLAGS = -DUSE_OPTIONS_H
WINDOWS_PB_CFLAGS = -DUSE_OPTIONS_H
PDO_UNIX_PB_CFLAGS = -DUSE_OPTIONS_H


NEXTSTEP_OBJCPLUS_COMPILER = /usr/bin/cc
WINDOWS_OBJCPLUS_COMPILER = $(DEVDIR)/gcc
PDO_UNIX_OBJCPLUS_COMPILER = $(NEXTDEV_BIN)/gcc
NEXTSTEP_JAVA_COMPILER = /usr/bin/javac
WINDOWS_JAVA_COMPILER = $(JDKBINDIR)/javac.exe
PDO_UNIX_JAVA_COMPILER = $(NEXTDEV_BIN)/javac

include $(MAKEFILEDIR)/platform.make

-include Makefile.preamble

include $(MAKEFILEDIR)/$(MAKEFILE)

-include Makefile.postamble

-include Makefile.dependencies
