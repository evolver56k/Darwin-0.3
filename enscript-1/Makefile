##
# Makefile for enscript
##

# Project info
Project               = enscript
UserType              = Administration
ToolType              = Commands
Extra_Configure_Flags = --sysconfdir=$(Install_Prefix)/share/enscript --with-media=Letter
Extra_LD_Flags        = -read_only_relocs warning
Extra_Install_Flags   = sysconfdir=$(DSTROOT)$(Install_Prefix)/share/enscript

# It's a GNU Source project
include /System/Developer/Makefiles/CoreOS/ReleaseControl/GNUSource.make
