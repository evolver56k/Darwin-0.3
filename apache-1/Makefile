##
# Makefile for apache
##
# Wilfredo Sanchez | wsanchez@apple.com
##

# Project info
Project         = apache
UserType        = Administration
ToolType        = Services
GnuAfterInstall = install-local

# It's a GNU Source project
# Well, not really but we can make it work.
include $(MAKEFILEPATH)/CoreOS/ReleaseControl/GNUSource.make

# Ignore RC_CFLAGS
Extra_CC_Flags = -traditional-cpp

# We put CFLAGS and LDFLAGS into the configure environment directly,
# and not in $(Environment), because the Apache Makefiles don't follow
# GNU guidelines, though configure mostly does.

LocalWebServer   = $(NSLOCALDIR)$(NSLIBRARYSUBDIR)/WebServer
FrameworkVersion = 1.3
DocRoot          = $(LocalWebServer)/Documents
CGIDir           = $(LocalWebServer)/CGI-Executables
Documentation    = $(NSDOCUMENTATIONDIR)/$(UserType)/$(ToolType)/apache
Configure        = cd $(shell pwd)/apache && CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(Sources)/configure
Configure_Flags  = --shadow="$(BuildDirectory)"									\
		   --with-layout=Rhapsody									\
		   --target="apache"										\
		   --enable-shared=max										\
		   --enable-module=most
Environment      = TARGET_STRIPFLAGS='-S "-S"' DSO_STRIPFLAGS='-S "-S"'						\
		   iconsdir="$(NSSYSTEMDIR)$(NSLIBRARYSUBDIR)/Apache/Icons"					\
		   htdocsdir="$(DocRoot)"									\
		   cgidir="$(CGIDir)"
Install_Flags    = root="$(DSTROOT)"
Install_Target   = install

##
# We want to compile all of the modules, so users don't have to, but
# we don't want them all turned on by default, since they add bloat
# to the server and hinder performance.
# Let's disable the ones users aren't likely to need, while leaving
# them the option of re-enabling them as desired.
# Modules listed here are disabled, except modules preceded by '-'
# are not disabled--a hack so we can keep a full list of modules here.
##
Disabled_Modules = 			\
		  -mod_access		\
		  -mod_actions		\
		  -mod_alias		\
		  -mod_asis		\
		  -mod_auth		\
		   mod_auth_anon	\
		   mod_auth_db		\
		   mod_auth_dbm		\
		  -mod_autoindex	\
		  -mod_cgi		\
		   mod_cern_meta	\
		  -mod_dir		\
		   mod_digest		\
		   mod_env		\
		   mod_expires		\
		   mod_headers		\
		  -mod_imap		\
		  -mod_include		\
		   mod_info		\
		   mod_log_agent	\
		  -mod_log_config	\
		   mod_log_referer	\
		  -mod_mime		\
		   mod_mime_magic	\
		  -mod_negotiation	\
		   mod_proxy libproxy	\
		  -mod_rewrite		\
		  -mod_setenvif		\
		   mod_speling		\
		   mod_status		\
		  -mod_userdir		\
		   mod_usertrack	\
		   mod_unique_id

##
# install-local does the following:
# - Install our default doc root.
# - Install our version of printenv. (Need to resubmit to Apache.)
# - Move apache manual to documentation directory, place a symlink to it in the doc root.
# - Add a symlink to the Apache release note in the doc root.
# - Make the server root group writeable.
# - Create framework symlinks. (Should be done by Apache makefiles.)
# - Disable non-"standard" modules.
# - Edit the configuration defaults as needed.
# - Remove -arch foo flags from apxs since module writers may not build for the same architectures(s) as we do.
##

install-local:
	@echo "Fixing up document root"
	$(_v) $(INSTALL_FILE) -c -m 664 "$(SRCROOT)/DocumentRoot/"*.html "$(SRCROOT)/DocumentRoot/"*.gif "$(DSTROOT)$(DocRoot)"
	$(_v) $(INSTALL_FILE) -c -m 664 printenv "$(DSTROOT)$(LocalWebServer)/CGI-Executables"
	$(_v) $(MKDIR) `dirname "$(DSTROOT)$(Documentation)"`
	$(_v) $(RMDIR) "$(DSTROOT)$(Documentation)"
	$(_v) $(MV) "$(DSTROOT)$(DocRoot)/manual" "$(DSTROOT)$(Documentation)"
	$(_v) $(LN) -fs "$(Documentation)" "$(DSTROOT)$(DocRoot)/manual"
	$(_v) $(LN) -fs "$(NSDOCUMENTATIONDIR)/ReadMe/Networking&Services/Apache.html" "$(DSTROOT)$(DocRoot)/ReleaseNotes.html"
	@echo "Fixing up framework"
	$(_v) cd "$(DSTROOT)$(NSFRAMEWORKDIR)/Apache.framework" && $(LN) -fs "Versions/$(FrameworkVersion)/"* .
	@echo "Fixing up configuration"
	$(_v) for module in $(Disabled_Modules); do											\
	          perl -i -pe 's|^(LoadModule\s+.*/'$${module}'\.so)$$|#$${1}|' $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*;	\
	          perl -i -pe 's|^(AddModule\s+'$${module}'\.c)$$|#$${1}|'   $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*;	\
	      done
	$(_v) perl -i -pe 's|^(User\s+).*$$|$${1}www|'                                          $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|^(Group\s+).*$$|$${1}www|'                                         $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|^(MaxRequestsPerChild\s+)\d+$$|$${1}100000|'                       $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|^(UserDir\s+).+$$|$${1}\"Web Documents\"|'                         $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|^(ServerAdmin\s+).*$$|#$${1}webmaster\@mydomain.com|'              $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|^(ServerName\s+).*$$|#$${1}www.mydomain.com|'                      $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|Log "($(LocalWebServer)/Logs/.+)"|Log "\|/usr/sbin/rotatelogs $${1} 86400"|' $(DSTROOT)$(LocalWebServer)/Configuration/apache.conf*
	$(_v) perl -i -pe 's|-arch\s+\S+\s*||g' $(DSTROOT)$(USRSBINDIR)/apxs
	$(_v) $(CHMOD) -R g+w "$(DSTROOT)$(DocRoot)"
	$(_v) $(CHMOD) -R g+w "$(DSTROOT)$(CGIDir)"
