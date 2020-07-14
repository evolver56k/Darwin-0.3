
#
# package.make: a rule to make packages
#
# Yves, July 1997.
#

# Include this file in your own Makefile and use the following instructions.
#
# Set the directory where you want your packages to go:
#
#    PACKAGEDIR = /Some/Location	# Defaults to $(TMPDIR) which defaults to /tmp
#
# Make targets invoking the package target. A typical package creation target would
# look like:
#
#    my_package:
#            $(MAKE) package PACKAGENAME=MyPackage \
#		  PACKAGECOMPONENTS="/Some/Directory /Some/File/here"
#
# The package targets expects to find SomeLanguage.lproj/$(PACKAGENAME).info, and will
# copy Scripts/$(PACKAGENAME).* into the package (where * is one of the legal names
# for the scripts), changing the script's permissions if needed.
#
# To help making packages for different systems (eg. Yellow and Grail) or from
# installed stable roots, one can also use the following variables when making the
# package:
#
#   DSTROOT		a new name for BUILDROOT
#   BUILDROOT		the root where one built the package (eg. /GrailRoots)
#   INSTALLROOT		the root from where to take the package
#
#   PACKAGENAMESUFFIX	a suffix to add to the name of the pacjage (eg. ForGrail)
#   PREPACKAGETARGET	a target to run before the package is built but after things
#			have been copied (useful to delete extra stuff)
#
# Other variables you may want to control are:
#
#   PACKAGE_FLAGS	additional flags to package (like -traditional)
#
#   CHOWN_ROOT		set to NO to prevent giving all files to root.wheel
#   DELETE_PACKAGEROOT	set to NO to not delete the package root after
#			the package has been created
#   COPYINTO_PACKAGEROOT	set to NO to avoid the copy phase: the
#				makefile suppose they're already installed
#				into $(PACKAGEROOT)
#

TMPDIR = /tmp

PACKAGEDIR = $(TMPDIR)
PACKAGEROOT = $(TMPDIR)

# You shouldn't have to edit the following. Please add your targets below.

DSTROOT = /
BUILDROOT = $(DSTROOT)
INSTALLROOT = /

PACKAGENAMESUFFIX =

REALLY =
MKDIRS = /bin/mkdir -p
CHOWN = /usr/sbin/chown
ifneq "" "$(wildcard /usr/bin/package)"
PACKAGE = /usr/bin/package
else
PACKAGE = /usr/etc/package
endif
SED = /usr/bin/sed

CHMOD = /bin/chmod
MKDIR = /bin/mkdir
RM = /bin/rm -f
CP = /bin/cp
ECHO = /bin/echo
TR = /usr/bin/tr
TAR = gnutar
SILENT = @

BASENAME = /usr/bin/basename
DIRNAME = /usr/bin/dirname

ifneq "" "$(wildcard apple-customize.make)"
include apple-customize.make
endif

# Please, don't mess with my package target!

RELATIVECOMPONENTS = $(patsubst /%,%,$(PACKAGECOMPONENTS))
ifeq "" "$(RELATIVECOMPONENTS)"
RELATIVECOMPONENTS = *
endif

ALL_INFO_FILES = $(shell find *.lproj -name $(PACKAGENAME)$(PACKAGENAMESUFFIX).info -print)

package:
	$(SILENT) if [ -z "$(ALL_INFO_FILES)" ]; then \
	    >&2 $(ECHO) Simon says: It is in your best interest to have a .info file available.; \
	    >&2 $(ECHO) Simon is now terminating this make...; \
	    exit 1; \
	fi
ifneq "NO" "$(COPYINTO_PACKAGEROOT)"
	$(REALLY) $(RM) -r $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot
	$(MKDIRS) $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot
	$(REALLY) $(SHELL) -c "(cd $(BUILDROOT); $(TAR) cf - $(RELATIVECOMPONENTS)) | (cd $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot; $(TAR) xf -)"
	@if [ ! -z "$(PREPACKAGETARGET)" ]; then \
	    $(ECHO) $(REALLY) $(MAKE) $(PREPACKAGE_TARGET) PACKAGEROOT=$(PACKAGEROOT) PACKAGENAME=$(PACKAGENAME) PACKAGENAMESUFFIX=$(PACKAGENAMESUFFIX); \
	    $(REALLY) $(MAKE) $(PREPACKAGETARGET) PACKAGEROOT=$(PACKAGEROOT) PACKAGENAME=$(PACKAGENAME) PACKAGENAMESUFFIX=$(PACKAGENAMESUFFIX); \
	fi
ifneq "NO" "$(CHOWN_ROOT)"
	$(REALLY) $(CHOWN) -R root.wheel $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot
endif
	$(REALLY) $(RM) -r $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg
	$(SILENT) if [ YES = "`$(SED) -n 's/^LongFileNames[ 	]*\([A-Za-z][A-Za-z]*\).*/\1/p' $(firstword $(ALL_INFO_FILES)) | $(TR) '[a-z]' '[A-Z]'`" ]; then \
	    packageArgs="-B"; \
	fi; \
	$(ECHO) $(PACKAGE) $(PACKAGE_FLAGS) $$packageArgs $(PACKAGE_FLAGS) $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot$(INSTALLROOT) $(firstword $(ALL_INFO_FILES)) $(PACKAGEICON) -d $(PACKAGEDIR); \
	$(PACKAGE) $(PACKAGE_FLAGS) $$packageArgs $(PACKAGE_FLAGS) $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot$(INSTALLROOT) $(firstword $(ALL_INFO_FILES)) $(PACKAGEICON) -d $(PACKAGEDIR)
endif
	@for file in `ls *.lproj/$(PACKAGENAME)$(PACKAGENAMESUFFIX).*`; do \
	    lproj=`$(DIRNAME) $$file`; \
	    name=`$(BASENAME) $$file`; \
	    $(RM) $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$$name; \
	    if [ ! -d "$(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$$lproj" ]; then \
	        $(MKDIR) $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$$lproj; \
	    fi; \
	    $(CP) $$file $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$$lproj; \
	done
	@for suffix in pre_install post_install pre_delete post_delete; do \
	    if [ -f Scripts/$(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix ]; then \
		$(CP) Scripts/$(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg; \
		$(CHMOD) 755 $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix; \
		$(ECHO) Copied $(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix script into package; \
	    else \
	    	if [ -f Scripts/$(PACKAGENAME).$$suffix ]; then \
		    $(CP) Scripts/$(PACKAGENAME).$$suffix $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix; \
		    $(CHMOD) 755 $(PACKAGEDIR)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkg/$(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix; \
		    $(ECHO) Copied $(PACKAGENAME).$$suffix script as $(PACKAGENAME)$(PACKAGENAMESUFFIX).$$suffix into package; \
		fi; \
	    fi; \
	done
ifneq "NO" "$(DELETE_PACKAGEROOT)"
	$(REALLY) $(RM) -r $(PACKAGEROOT)/$(PACKAGENAME)$(PACKAGENAMESUFFIX).pkgroot
endif

