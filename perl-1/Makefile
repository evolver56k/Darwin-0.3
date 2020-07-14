##
# Makefile for perl
##
# Wilfredo Sanchez | wsanchez@apple.com
# I wish they'd just use autoconf. This is hairy.
##

# Project info
Project             = perl
UserType            = Developer
ToolType            = Commands
Configure           = $(BuildDirectory)/Configure
Extra_Environment   = HTMLDIR="$(Install_HTML)"						\
		      AR="$(SRCROOT)/ar.sh"  DYLD_LIBRARY_PATH=$(BuildDirectory)
Extra_Install_Flags = HTMLDIR="$(RC_Install_HTML)" HTMLROOT=$(Install_HTML)
GnuAfterInstall     = undo-dstroot-hack

# It's a GNU Source project
# Well, not really but we can make it work.
include $(MAKEFILEPATH)/CoreOS/ReleaseControl/GNUSource.make

Install_Target  = install
CC_Optimize     = 
Extra_CC_Flags  = 
Configure_Flags = -ds -e -Dprefix='$(Install_Prefix)' -Dccflags='$(CFLAGS)' -Dldflags='$(LDFLAGS)'

##
# Eit. Getting perl to compile outside of the source tree is a severe pain.
# I got some of it working, but it's pretty bothersome, so we'll cheat here and
# create a 'shadow tree' instead and build there.
##
lazy_install_source::
	$(_v) if [ ! -f $(Configure) ]; then					\
	  echo "Creating pseudo-copy of sources in OBJROOT...";			\
	  mkdir -p $(OBJROOT);							\
	  for dir in $$( cd $(Sources) && $(FIND) . -type d ); do		\
	    cd $(OBJROOT) && if [ ! -d $$dir ]; then $(MKDIR) $$dir; fi;	\
	  done;									\
	  for file in $$( cd $(Sources) && $(FIND) . -type f ); do		\
	    cd $(OBJROOT) && $(LN) -fs $(Sources)/$$file $$file;		\
	  done;									\
	fi

##
# Eit again.
# There's no easy way (well I don't see one, offhand) to sneak $(DSTROOT) into the
# location variables at "make install" time because make doesn't do the install.
# So we create config.sh, and then do a string replace for the install locations
# in there, which are meant for AFS, or we'd have configure options for it maybe.
# But we don't, so we hack some more.
##
UNIQUE := $(shell echo $$$$)

CONFIGSH = $(OBJROOT)/config.sh

ConfigStamp2 = $(ConfigStamp)2

configure:: $(ConfigStamp2)

$(ConfigStamp2): $(ConfigStamp)
	$(_v) cat $(CONFIGSH) | sed 's@^\(install[^=]*='\''\)@\1$(DSTROOT)@' > /tmp/build.perl.$(UNIQUE)
	$(_v) mv /tmp/build.perl.$(UNIQUE) $(CONFIGSH)
	$(_v) touch $(ConfigStamp2)

##
# Triple eit.
# Having whacked values in config.sh effects Config.pm, and we don't want $(DSTROOT) in there.
# So we need to undo the damage once it is installed.
##
CONFIGPM = $(DSTROOT)/System/Library/Perl/rhapsody/Config.pm

undo-dstroot-hack:
	$(_v) cat $(CONFIGPM) |	sed 's@^\(install[^=]*='\''\)$(DSTROOT)@\1@' > /tmp/build.perl.$(UNIQUE)
	$(_v) mv -f /tmp/build.perl.$(UNIQUE) $(CONFIGPM)
