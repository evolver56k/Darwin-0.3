# Makefile for `wget' utility for MSVC 4.0
# Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#
# Version: 1.4.4
#

SHELL = command

VPATH       = .
o           = .obj
OUTDIR      = .

CC       = cl.exe
LD       = link.exe

CFLAGS   = /nologo /ML /W2 /GX /O2
CPPFLAGS = 
DEFS     = /D "WINDOWS" /D "_CONSOLE" /D "HAVE_CONFIG_H"
LDFLAGS  = /nologo /subsystem:console /incremental:no /machine:I386 #/out:"$(OUTDIR)/wget.exe
LIBS     = kernel32.lib user32.lib shell32.lib wsock32.lib

INCLUDES = /I "." /I ".."

COMPILE = $(CC) $(INCLUDES) $(CPPFLAGS)  $(DEFS) $(CFLAGS)
LINK    = $(LD) $(LDFLAGS) /out:$@

#INSTALL = @INSTALL@
#INSTALL_PROGRAM = @INSTALL_PROGRAM@

RM      = del

#ETAGS = etags

SRC = connect.c host.c http.c netrc.c ftp-basic.c ftp.c ftp-unix.c \
      mtch.c html.c retr.c recur.c url.c init.c utils.c main.c \
      getopt.c atotm.c version.c wsstartup.c

OBJ = connect$o host$o http$o netrc$o ftp-basic$o ftp$o ftp-unix$o \
      mtch$o html$o retr$o recur$o url$o init$o utils$o main$o \
      getopt$o atotm$o version$o wsstartup$o

.SUFFIXES:
.SUFFIXES: .c .obj

.c.obj:
        $(COMPILE) /c $<

# Dependencies for building

wget: wget.exe

wget.exe: $(OBJ)
	$(LINK) $(OBJ) $(LIBS)

#
# Dependencies for installing
#

##install: install.bin
##
##uninstall: uninstall.bin
##
##install.bin: wget
##        $(top_srcdir)/mkinstalldirs $(bindir)
##        $(INSTALL_PROGRAM) wget $(bindir)/wget
##
##uninstall.bin:
##        $(RM) $(bindir)/wget
##

#
# Dependencies for cleanup
#

clean:
        $(RM) *.obj
        $(RM) *.exe
        $(RM) *.bak

distclean: clean
	$(RM) Makefile

realclean: distclean
	$(RM) TAGS

# Dependencies:

atotm$o:
connect$o: systhings.h wget.h options.h utils.h connect.h host.h windecl.h
ftp-basic$o: wget.h options.h utils.h ftp-basic.h connect.h host.h retr.h
ftp-unix$o: wget.h url.h utils.h ftp.h options.h
ftp$o: wget.h options.h utils.h url.h ftp.h ftp-basic.h html.h connect.h host.h http.h mtch.h retr.h netrc.h
getopt$o: getopt.h
host$o: wget.h options.h utils.h url.h host.h
html$o: wget.h options.h url.h utils.h ftp.h html.h http.h
http$o: wget.h options.h utils.h url.h host.h http.h retr.h connect.h mtch.h netrc.h
init$o: wget.h options.h utils.h init.h url.h ftp.h host.h recur.h netrc.h
main$o: wget.h options.h main.h utils.h url.h init.h retr.h recur.h ftp.h getopt.h wsstartup.h
mtch$o: wget.h mtch.h
netrc$o: wget.h utils.h netrc.h options.h
recur$o: wget.h options.h url.h recur.h utils.h retr.h http.h ftp.h mtch.h host.h
retr$o: wget.h options.h utils.h retr.h url.h recur.h ftp.h html.h http.h host.h connect.h
url$o: wget.h options.h utils.h url.h host.h ftp.h mtch.h html.h
utils$o: wget.h utils.h options.h mtch.h
version$o:
wsstartup$o:

