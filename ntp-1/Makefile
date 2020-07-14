##
# Makefile for ntp
##

# Project info
Project           = ntp
UserType          = Administration
ToolType          = Services
Extra_Environment = LIBMATH=""
Extra_CC_Flags    = -DUSE_NETINFO

# It's a GNU Source project
include $(MAKEFILEPATH)/CoreOS/ReleaseControl/GNUSource.make

# Install the HTML docs.
install::
	$(INSTALL_DIRECTORY) $(DSTROOT)$(Install_HTML)
	cd $(Sources)/html && \
	$(PAX) -rw *.htm *.gif *.jpg $(DSTROOT)$(Install_HTML)
