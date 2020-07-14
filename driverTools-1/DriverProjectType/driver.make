#
# driver.make
#
# Variable definitions and rules for building driver projects.  A driver
# is a directory which contains a dynamically-loadable executable and any
# resources that executable requires.  See wrapped.make for more information
# about projects whose product is a directory.
#
# PUBLIC TARGETS
#    driver: synonymous with all
#
# IMPORTED VARIABLES
#    none
#
# EXPORTED VARIABLES
#    none
#

.PHONY: driver all

driver: all

PRODUCT = $(PRODUCT_DIR)/$(NAME).$(BUNDLE_EXTENSION)
PRODUCTS = $(PRODUCT)
INNER_PRODUCT = $(PRODUCT)/$(NAME)$(BUILD_TYPE_SUFFIX)
STRIPPED_PRODUCTS = $(INNER_PRODUCT)

PROJTYPE_MFLAGS = -F$(PRODUCT_DIR)
PROJTYPE_RESOURCES = ProjectTypes/Driver.projectType/Resources

# The resource directories are exported so that non-wrapped
# subprojects can copy their resources.
export GLOBAL_RESOURCE_DIR = $(PRODUCT)

HELP_FILE = Help
SOURCE_HELP_DIR = DriverHelp

#
# The code below was grabbed from wrapped-common.make.  We should go back
# to that for IOKit project stuff.
#
WRAPPED = YES

THIN_BINARIES := $(NAME)$(BUILD_TYPE_SUFFIX) $(TOOLS)
THIN_BINARIES := $(THIN_BINARIES:%.tproj=%)
THIN_BINARIES := $(THIN_BINARIES:%.lksproj=%_reloc)

AFTER_BUILD += post_copy_tables
AFTER_INSTALL += thin_architectures

ifndef LOCAL_MAKEFILEDIR
    LOCAL_MAKEFILEDIR = $(LOCAL_DEVELOPER_DIR)/Makefiles/pb_makefiles
endif
-include $(LOCAL_MAKEFILEDIR)/driver.make.preamble

include $(MAKEFILEDIR)/common.make

#
# Try to find the Driver.projectType root directory
#
ifneq "" "$(wildcard $(NEXT_ROOT)$(LOCAL_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES))"
DRIVER_TOOLS_ROOT=$(NEXT_ROOT)$(LOCAL_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES)
else

ifneq "" "$(wildcard $(HOME)/Library/$(PROJTYPE_RESOURCES))"
DRIVER_TOOLS_ROOT=$(HOME)/Library/$(PROJTYPE_RESOURCES)
else
DRIVER_TOOLS_ROOT=$(NEXT_ROOT)$(SYSTEM_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES)
endif
endif

ifeq "$(OS)" "NEXTSTEP"
PROJTYPE_LDFLAGS = $($(OS)_PROJTYPE_LDFLAGS) -bundle -undefined suppress
endif

EXPR = /bin/expr

#
# prebuild rules
#

GENPKGSCRIPT  = $(DRIVER_TOOLS_ROOT)/genpkgfiles.sh
VEREDITSCRIPT = $(DRIVER_TOOLS_ROOT)/veredit.sh
GENTOCSCRIPT  = $(DRIVER_TOOLS_ROOT)/gentoc.sh
THINSCRIPT    = $(DRIVER_TOOLS_ROOT)/thindriver.sh
INFO_ROOT     = $(DSTROOT)/usr/local/InfoFiles

.PHONY: post_copy_tables move_to_architecture thin_architectures	  \
	movehelp gen_table_contents driver_version_edit gen_package_files

movehelp:
	$(SILENT) for lang in $(LANGUAGES); do				  \
	    localRscDir=$(GLOBAL_RESOURCE_DIR)/$$lang.lproj;		  \
	    helpdir=$$localRscDir/$(HELP_FILE);				  \
	    srchlpdir=$$localRscDir/$(SOURCE_HELP_DIR);			  \
	    if test -e "$$helpdir"; then				  \
		$(CHMOD) -R a+w $$helpdir;				  \
		$(RM) -rf $$helpdir;					  \
	    fi;								  \
	    cmd="$(MV) $$srchlpdir $$helpdir"; echo $$cmd; eval $$cmd;	  \
            if test "$(GEN_TABLE_CONTENTS)" = gen_table_contents; then	  \
		cmd="$(GENTOCSCRIPT) $(SRCROOT)$(SRCPATH)";		  \
		cmd="$$cmd > $$helpdir/TableOfContents.rtf";		  \
		echo $$cmd; eval $$cmd;					  \
	    fi;								  \
	done

driver_version_edit:
	$(VEREDITSCRIPT) $(SRCROOT)$(SRCPATH) $(PRODUCT)

gen_package_files:
ifeq "$(DSTROOT)" ""
	$(GENPKGSCRIPT) $(NAME) $(SRCROOT)$(SRCPATH) /tmp
else
	$(SILENT) $(MKDIRS) $(INFO_ROOT)
	$(GENPKGSCRIPT) $(NAME) $(SRCROOT)$(SRCPATH) $(INFO_ROOT)
endif

post_copy_tables:
	$(SILENT) for table in $(GLOBAL_RESOURCE_DIR)/*.table;	       	  \
	do								  \
	    $(CHMOD) +w $$table;					  \
	    $(ECHO) '"Server Name" = "$(NAME)";' >> $$table;		  \
	done

#
# Thin architectures just calls a standard shell script, but before doing so
# it sets up the environment of commands that the shell script is likely to
# need.
#
thin_architectures:
	ARCH_CMD="$(ARCH_CMD)" CAT="$(CAT)" \
	CHGRP="$(CHGRP)" CHOWN="$(CHOWN)" CHMOD="$(CHMOD)" \
	ECHO="$(ECHO)" FASTCP="$(FASTCP)" FIND="$(FIND)" \
	LIPO="$(LIPO)" MKDIRS="$(MKDIRS)" \
	MV="$(MV)" RM="$(RM)" SED="$(SED)" \
            $(SHELL) $(THINSCRIPT) \
		-driver $(INSTALLED_PRODUCTS) \
		-archs $(ADJUSTED_TARGET_ARCHS) -binaries $(THIN_BINARIES) \
		-perms $(INSTALL_PERMISSIONS) -owner $(INSTALL_AS_USER)   \
		-group $(INSTALL_AS_GROUP)

$(PRODUCT): $(INNER_PRODUCT)

$(INNER_PRODUCT): $(DEPENDENCIES)
ifneq "$(words $(LOADABLES))" "0"
	$(SILENT) $(MKDIRS) $(PRODUCT)
	$(LD) $(ALL_LDFLAGS) $(ARCHITECTURE_FLAGS) -o $(INNER_PRODUCT) $(LOADABLES)
endif

-include $(LOCAL_MAKEFILEDIR)/driver.make.postamble

