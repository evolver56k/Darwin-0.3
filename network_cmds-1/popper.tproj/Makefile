#
# Generated by the NeXT Project Builder.
#
# NOTE: Do NOT change this file -- Project Builder maintains it.
#
# Put all of your customizations in files called Makefile.preamble
# and Makefile.postamble (both optional), and Makefile will include them.
#

NAME = popper

PROJECTVERSION = 2.6
PROJECT_TYPE = Tool
LANGUAGE = English

HFILES = popper.h version.h

CFILES = pop_dele.c pop_dropcopy.c pop_dropinfo.c pop_get_command.c\
         pop_get_subcommand.c pop_head.c pop_init.c pop_last.c\
         pop_list.c pop_log.c pop_lower.c pop_msg.c pop_parse.c\
         pop_pass.c pop_quit.c pop_rset.c pop_send.c pop_stat.c\
         pop_updt.c pop_user.c pop_xmit.c pop_xtnd.c popper.c

OTHERSRCS = Makefile.dist Makefile.preamble PB.project README\
            README.NeXT inetd.conf.popper pop3.rfc1081 pop3e.rfc1082\
            popper.8 services.pop


MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
CODE_GEN_STYLE = DYNAMIC
MAKEFILE = tool.make
NEXTSTEP_INSTALLDIR = /usr/libexec
WINDOWS_INSTALLDIR = /usr/libexec
PDO_UNIX_INSTALLDIR = /usr/libexec
LIBS = 
DEBUG_LIBS = $(LIBS)
PROF_LIBS = $(LIBS)


NEXTSTEP_PB_CFLAGS = -DBIND43 -DHAVE_VSPRINTF -DDEBUG
WINDOWS_PB_CFLAGS = -DBIND43 -DHAVE_VSPRINTF -DDEBUG
PDO_UNIX_PB_CFLAGS = -DBIND43 -DHAVE_VSPRINTF -DDEBUG


include $(MAKEFILEDIR)/platform.make

-include Makefile.preamble

include $(MAKEFILEDIR)/$(MAKEFILE)

-include Makefile.postamble

-include Makefile.dependencies
