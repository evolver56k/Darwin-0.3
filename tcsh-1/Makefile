##
# Makefile for tcsh
##

# Project info
Project               = tcsh
Extra_CC_Flags        = -UNeXT -D_PATH_TCSHELL='\"/bin/tcsh\"' -Wno-precomp
Extra_Configure_Flags = --bindir="/bin"
Extra_Install_Flags   = DESTBIN="$(DSTROOT)/bin"
UserType              = Administration
ToolType              = Commands

# It's a GNU Source project
include $(MAKEFILEPATH)/CoreOS/ReleaseControl/GNUSource.make

install::
	$(MKDIR) $(DSTROOT)$(ETCDIR)
	echo "source /usr/share/init/tcsh/rc"     > $(DSTROOT)$(ETCDIR)/csh.cshrc
	echo "source /usr/share/init/tcsh/login"  > $(DSTROOT)$(ETCDIR)/csh.login
	echo "source /usr/share/init/tcsh/logout" > $(DSTROOT)$(ETCDIR)/csh.logout
