##
# Makefile for Bash
##

# Project info
Project               = bash
Extra_Configure_Flags = --bindir=/bin
Extra_Install_Flags   = bindir=$(DSTROOT)/bin
UserType              = Administration
ToolType              = Commands

# It's a GNU Source project
include /System/Developer/Makefiles/CoreOS/ReleaseControl/GNUSource.make

# Bash makefiles are a bit screwy...
# Setting CCFLAGS upsets bash, so we override Environment
# so that it doesn't.
Environment =   CFLAGS="$(CFLAGS)"	\
	       LDFLAGS="$(LDFLAGS)"	\
	      $(Extra_Environment)
