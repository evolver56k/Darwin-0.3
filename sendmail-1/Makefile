##
# Makefile for sendmail
##

# Project info
Project  = sendmail
UserType = Administration
ToolType = Services

# It's a 3rd Party Source project
include $(MAKEFILEPATH)/CoreOS/ReleaseControl/Common.make

build::
	$(_v) $(MAKE) -C $(Sources)/$(Project)	\
		Extra_CC_Flags="$(CFLAGS)"	\
		Extra_LD_Flags="$(LDFLAGS)"	\
		OPTIONS='-o "$(OBJROOT)"'

install:: install-sendmail install-cf install-doc install-contrib

install-sendmail:
	$(_v) $(MAKE) -C $(Sources)/$(Project) install-strip	\
		DESTDIR="$(DSTROOT)"				\
		STDIR="/private/var/log"			\
		FORCE_INSTALL="force-install"			\
		OPTIONS='-o "$(OBJROOT)"'

UNIQUE := $(shell echo $$$$)

USRSBINDIR    = /usr/sbin
ETCDIR        = /private/etc
DATADIR       = /usr/share/sendmail
CONFDIR       = $(DATADIR)/conf
CFDIR         = $(CONFDIR)/cf
DOMAINDIR     = $(CONFDIR)/domain
FEATUREDIR    = $(CONFDIR)/feature
HACKDIR       = $(CONFDIR)/hack
M4DIR         = $(CONFDIR)/m4
MAILERDIR     = $(CONFDIR)/mailer
OSTYPEDIR     = $(CONFDIR)/ostype
SHDIR         = $(CONFDIR)/sh
SITECONFIGDIR = $(CONFDIR)/siteconfig

install-cf:
	@echo "Installing supporting files..."
	$(INSTALL_DIRECTORY) $(DSTROOT)$(ETCDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(DATADIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(CONFDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(CFDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(DOMAINDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(FEATUREDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(HACKDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(M4DIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(MAILERDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(OSTYPEDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(SHDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(SITECONFIGDIR)
	$(INSTALL_FILE) -c $(Project)/cf/README          $(DSTROOT)$(CONFDIR)
	$(INSTALL_FILE) -c $(Project)/cf/cf/*.mc         $(DSTROOT)$(CFDIR)
	$(INSTALL_FILE) -c $(Project)/cf/domain/*.m4     $(DSTROOT)$(DOMAINDIR)
	$(INSTALL_FILE) -c $(Project)/cf/feature/*.m4    $(DSTROOT)$(FEATUREDIR)
	$(INSTALL_FILE) -c $(Project)/cf/hack/*.m4       $(DSTROOT)$(HACKDIR)
	$(INSTALL_FILE) -c $(Project)/cf/m4/*.m4         $(DSTROOT)$(M4DIR)
	$(INSTALL_FILE) -c $(Project)/cf/mailer/*.m4     $(DSTROOT)$(MAILERDIR)
	$(INSTALL_FILE) -c $(Project)/cf/ostype/*.m4     $(DSTROOT)$(OSTYPEDIR)
	$(INSTALL_FILE) -c $(Project)/cf/sh/*.sh         $(DSTROOT)$(SHDIR)
	$(INSTALL_FILE) -c $(Project)/cf/siteconfig/*.m4 $(DSTROOT)$(SITECONFIGDIR)
	$(M4) -D_CF_DIR_=$(DSTROOT)$(CONFDIR)/ $(DSTROOT)$(M4DIR)/cf.m4				\
		$(DSTROOT)$(CFDIR)/rhapsody.mc > /tmp/sendmail.cf.$(UNIQUE)
	$(SED) 's|O AliasFile=|#O AliasFile=|g'							\
		< /tmp/sendmail.cf.$(UNIQUE) > /tmp/sendmail.cf.$(UNIQUE).no-alias
	$(INSTALL_FILE) -c /tmp/sendmail.cf.$(UNIQUE).no-alias $(DSTROOT)$(ETCDIR)/sendmail.cf
	$(INSTALL_FILE) -c /dev/null $(DSTROOT)$(ETCDIR)/sendmail.cw
	$(RM) -f /tmp/sendmail.cf.$(UNIQUE)*

DOCSDIR = /System/Documentation/Administration/Services/$(ProjectName)

install-doc:
	@echo "Installing documentation..."
	$(INSTALL_DIRECTORY) $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/FAQ                    $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/KNOWNBUGS              $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/LICENSE                $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/README                 $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/RELEASE_NOTES          $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/doc/changes/changes.ps $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/doc/intro/intro.ps     $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/doc/op/op.ps           $(DSTROOT)$(DOCSDIR)
	$(INSTALL_FILE) -c $(Project)/doc/usenix/usenix.ps   $(DSTROOT)$(DOCSDIR)

USRBINDIR = /usr/bin

install-contrib:
	@echo "Installing contrib files..."
	$(INSTALL_DIRECTORY) $(DSTROOT)$(USRBINDIR)
	$(INSTALL_DIRECTORY) $(DSTROOT)$(MANDIR)/man1
	$(INSTALL_SCRIPT) -c $(Project)/contrib/expn.pl $(DSTROOT)$(USRBINDIR)/expn
	$(LN) -fs $(USRBINDIR)/expn $(DSTROOT)$(MANDIR)/man1/expn.1
