###############################################################################
#  NeXT common.make
#  Copyright 1992, NeXT Computer, Inc.
#
#  This makefile is common to all project-types (apps, subprojects,
#  bundles, and palettes).  It can also prove useful to custom Makefiles
#  needing generic project-building functionality, but users should be aware
#  that interfaces supported at this level are private to the app makefiles
#  and may change from release to release.
#  
###############################################################################



SUPPORTFILES = IB.nproj Makefile makefile $(NAME).iconheader \
	Makefile.preamble Makefile.postamble *.project

SRCFILES = $(CLASSES) $(MFILES) $(CFILES) \
	$(CCFILES) $(CAPCFILES) $(CAPMFILES) $(CXXFILES) $(CPPFILES) \
	$(HFILES) $(PSWFILES) $(PSWMFILES) $(DBMODELAFILES) \
	$(GLOBAL_RESOURCES) \
	$(OTHERSRCS) $(OTHERLINKED) $(OTHER_SOURCEFILES)

### Compute all the possible derived files and directories for them:

SRCROOT = .
DERIVED_SRC_DIR_NAME = derived_src

# Directory for .o files (can be thrown away) 
OFILE_DIR = $(OBJROOT)/obj
# Directory for all other derived files (contains symbol info. for debugging)
SYM_DIR = $(SYMROOT)/$(DERIVED_SRC_DIR_NAME)
# Directory for all public headers of the entire project tree
LOCAL_HEADER_DIR = $(SYMROOT)/Headers/$(NAME)

# For compatibility:
DERIVED_DIR = $(OFILE_DIR)
DERIVED_SRC_DIR = $(SYM_DIR)

$(OFILE_DIR) $(SYM_DIR) $(PUBLIC_HEADER_DIR) $(DSTROOT)$(INSTALLDIR) $(PRODUCT_ROOT):
	@$(MKDIRS) $@

MSGOFILES = $(MSGFILES:.msg=Speaker.o) $(MSGFILES:.msg=Listener.o)
MSGDERIVEDMFILES = $(MSGFILES:.msg=Speaker.m) $(MSGFILES:.msg=Listener.m)

ALLMIGFILES = $(MIGFILES) $(DEFSFILES)

MIGOFILES = $(MIGFILES:.mig=User.o) $(MIGFILES:.mig=Server.o)
DEFSOFILES = $(DEFSFILES:.defs=User.o) $(DEFSFILES:.defs=Server.o)
ALLMIGOFILES = $(MIGOFILES) $(DEFSOFILES)

MIGDERIVEDCFILES = $(MIGFILES:.mig=User.c) $(MIGFILES:.mig=Server.c)
DEFSDERIVEDCFILES = $(DEFSFILES:.defs=User.c) $(DEFSFILES:.defs=Server.c)
ALLMIGDERIVEDCFILES = $(MIGDERIVEDCFILES) $(DEFSDERIVEDCFILES)

EARLY_HFILES = $(PSWFILES:.psw=.h) $(PSWMFILES:.pswm=.h)
EARLY_OFILES = $(PSWFILES:.psw=.o) $(PSWMFILES:.pswm=.o)

ALL_PRECOMPS = $(PRECOMPILED_HEADERS:.h=.p) $(PRECOMPS)
INITIAL_TARGETS = $(OFILE_DIR) $(SYM_DIR) $(EARLY_HFILES) $(MSGOFILES) $(ALLMIGOFILES) $(ALL_PRECOMPS) refresh_precomps export_headers $(OTHER_INITIAL_TARGETS)

SUBPROJ_OFILES = $(SUBPROJECTS:.subproj=_subproj.o)
SUBPROJ_OFILELISTS = $(SUBPROJECTS:.subproj=_subproj.ofileList)
NON_SUBPROJ_OFILES = $(CLASSES:.m=.o) $(MFILES:.m=.o) $(CFILES:.c=.o) \
	$(CCFILES:.cc=.o) $(CAPCFILES:.C=.o) $(CAPMFILES:.M=.o) \
	$(CXXFILES:.cxx=.o) $(CPPFILES:.cpp=.o) $(EARLY_OFILES) \
	$(OTHERLINKEDOFILES) 
OFILES = $(SUBPROJ_OFILES) $(NON_SUBPROJ_OFILES) 
 
#    Note: It would be nice to put $(OTHERRELOCATABLES) in this list someday
#          when PB provides full paths for the contents of this variable.

# Derived resources:
DBMODELS = $(DBMODELAFILES:.dbmodela=.dbmodel)

HELP_DIRS = Help
HELP_STORES = $(HELP_DIRS:=.store) $(OTHER_HELP_DIRS:=.store)

### Set defaults for many values used throughout the app Makefiles

# Default extension for bundles (directories containing object code and resources)

MAKEFILES = Makefile

PRODUCT_DEPENDS = $(OFILES) $(OTHER_OFILES) $(DBMODELS) \
	$(ICONHEADER) $(APPICON) $(DOCICONS) $(MAKEFILES) \
	$(OTHER_PRODUCT_DEPENDS)

GARBAGE = $(PROJECT_TYPE_SPECIFIC_GARBAGE) \
	$(OBJROOT)/*_obj $(OFILE_DIR) \
	$(SYMROOT)/$(DERIVED_SRC_DIR_NAME) $(SYMROOT)/sym \
	*~ $(LANGUAGE).lproj/*~ $(VERS_FILE) \
	Makefile.dependencies $(SYMROOT)/$(CHANGES_FILE_BASE)* gmon.out \
	$(ALL_PRECOMPS) $(OTHER_INITIAL_TARGETS) $(OTHER_GARBAGE)

GARBAGE_TO_BE = \
	$(DEV_PUBLIC_HEADER_DIR) $(DEV_PROJECT_HEADER_DIR) \
	$(DEV_PRIVATE_HEADER_DIR)

# Default name for file to use as "reference time of last build"
CHANGES_FILE_BASE = .lastBuildTime

# Defaults for who to chown executables to when installing
INSTALL_AS_USER = root
INSTALL_AS_GROUP = wheel

# Compiler flags that may be overridden
OPTIMIZATION_CFLAG = -O
DEBUG_SYMBOLS_CFLAG = -g
WARNING_CFLAGS = -Wall
DEBUG_BUILD_CFLAGS = -DDEBUG
PROFILE_BUILD_CFLAGS = -pg -DPROFILE
POSIX_BUILD_CFLAGS = -D_POSIX_LIB
SHLIB_BUILD_CFLAGS =  -I/LocalDeveloper/Headers/libsys -i/LocalDeveloper/Headers/libsys/shlib.h -DSHLIB

# Default compiler options
ALL_FRAMEWORK_CFLAGS = $(FRAMEWORK_PATHS) $(PROPOGATED_FRAMEWORK_CFLAGS)
PROJECT_SPECIFIC_CFLAGS = $(CFLAGS) $(OTHER_CFLAGS) $(HEADER_PATHS) $(PB_CFLAGS) 
PROJ_CFLAGS = $(PROJECT_SPECIFIC_CFLAGS)
COMMON_CFLAGS = $(DEBUG_SYMBOLS_CFLAG) $(WARNING_CFLAGS) $(PROJECT_SPECIFIC_CFLAGS)   
all_target_CFLAGS = $(COMMON_CFLAGS) $(OPTIMIZATION_CFLAG)
debug_target_CFLAGS = $(COMMON_CFLAGS) $(DEBUG_BUILD_CFLAGS)
profile_target_CFLAGS = $(COMMON_CFLAGS) $(PROFILE_BUILD_CFLAGS) $(OPTIMIZATION_CFLAG) 
app_target_CFLAGS = $(all_target_CFLAGS)
library_target_CFLAGS = $(all_target_CFLAGS)
framework_target_CFLAGS = $(all_target_CFLAGS)
bundle_target_CFLAGS = $(all_target_CFLAGS)
posix_target_CFLAGS = $(COMMON_CFLAGS) $(POSIX_BUILD_CFLAGS) $(OPTIMIZATION_CFLAG) 
shlib_target_CFLAGS = $(COMMON_CFLAGS) $(SHLIB_BUILD_CFLAGS) $(OPTIMIZATION_CFLAG) 
OBJCFLAG = -ObjC

# ...and the actual flags used in compilation (see basicrules.make)
ALL_CFLAGS = $(ALL_FRAMEWORK_CFLAGS) $(PROJ_CFLAGS) $(PROPOGATED_CFLAGS) -I$(DEV_PROJECT_HEADER_DIR) -I$(SYM_DIR) $(RC_CFLAGS)
ALL_PRECOMP_CFLAGS = $(ALL_FRAMEWORK_CFLAGS) $(PROJ_CFLAGS) $(PROPOGATED_CFLAGS) -I$(SYM_DIR) $(ALL_ARCH_FLAGS)

# Link editor options:
CUMULATIVE_LDFLAGS = $(LIBRARY_PATHS) $(PB_LDFLAGS) $(LDFLAGS) $(OTHER_LDFLAGS)
ALL_LDFLAGS = $(CUMULATIVE_LDFLAGS) $(PROPOGATED_LDFLAGS) $(PROJECT_SPECIFIC_LDFLAGS) 

# Yacc options
YFLAGS = -d

# Defaults strip options
INTERFACE_DESC_FILE = /NextDeveloper/Makefiles/project/interface.symbols
#DYLD_EXEC_STRIP_OPTS = -s $(INTERFACE_DESC_FILE) -u -A
DYLD_EXEC_STRIP_OPTS = -S
APP_STRIP_OPTS = $(DYLD_EXEC_STRIP_OPTS)
TOOL_STRIP_OPTS =  $(DYLD_EXEC_STRIP_OPTS) 
LIBRARY_STRIP_OPTS = -S   # Note: -S strips debugging symbols
DYNAMIC_STRIP_OPTS = -S

# Various commands:
SHELL  = /bin/sh
FASTCP = /usr/lib/fastcp
FASTLN = /usr/lib/fastln
CHANGES = /usr/lib/changes
ARCH_TOOL = /usr/lib/arch_tool
OFILE_LIST_TOOL = /usr/lib/ofileListTool -removePrefix ../ -removePrefix $(OBJROOT)/
DEARCHIFY = $(ARCH_TOOL) -dearchify
ARCHIFY = $(ARCH_TOOL) -archify_list
CHOWN  = /etc/chown
CHMOD  = /bin/chmod
TAR    = /usr/bin/gnutar
LIBTOOL = /bin/libtool
STRIP  = /bin/strip
RM     = /bin/rm
LN     = /bin/ln -s
CP     = /bin/cp
INSTALL_HEADERS_CMD = $(CP) -p
ECHO   = /bin/echo
MKDIRS = /bin/mkdirs
TOUCH  = /usr/bin/touch
AWK    = /bin/awk
PSWRAP = /usr/bin/pswrap
MSGWRAP = /usr/bin/msgwrap
MIG    = /usr/bin/mig
DBC    = /NextDeveloper/Apps/DBModeler.app/dbcompiler
COMPRESSHELP = /usr/bin/compresshelp
FIXPRECOMPS = /usr/bin/fixPrecomps
LIBTOOL = /bin/libtool

PUSHD = pushed_dir=`pwd` ; cd 
POPD = cd $$pushed_dir

ADAPTOR_SEARCH_PATH = $(HOME)/Library/Adaptors /LocalLibrary/Adaptors /NextLibrary/Adaptors

DEFAULT_BUNDLE_EXTENSION = bundle

# Set VPATH via a variable so clients of common.make can reuse it when overriding VPATH
NORMAL_VPATH = $(OFILE_DIR):$(SYM_DIR):$(LANGUAGE).lproj:$(PRODUCT_ROOT):$(PRODUCT_ROOT)/$(LANGUAGE).lproj
VPATH = $(VPATH_PREAMBLE)$(NORMAL_VPATH)$(VPATH_POSTAMBLE)


# Generation of a version string if project lives in correct directory name
# To activate this feature, put your source code in a directory named 
# $(NAME).%d[.%d][.%d] and set OTHER_GENERATED_OFILES = $(VERS_OFILES).
VERS_FILE = $(SYM_DIR)/$(NAME)_vers.c
VERS_OFILE = $(OFILE_DIR)/$(NAME)_vers.o
$(VERS_FILE): 
	$(RM) -f $(VERS_FILE) ; \
	vers_string -c $(NAME) \
		| sed s/SGS_VERS/$(NAME)_VERS_STRING/ \
		| sed s/VERS_NUM/$(NAME)_VERS_NUM/ > $@

ALL_OTHER_OFILES = $(OTHER_OFILES) $(OTHER_GENERATED_OFILES)

### Use a set of basic suffix-style rules:

COMMON_APP_MAKEFILE_DIR = /NextDeveloper/Makefiles/project
include $(COMMON_APP_MAKEFILE_DIR)/basicrules.make

### Some utility definitions used throughout the PB Makefiles

process_target_archs = \
	if [ -n "$(TARGET_ARCHS)" ] ; then \
		archs="$(TARGET_ARCHS)" ; \
	else \
	    if [ -n "$(RC_ARCHS)" ] ; then \
		archs="$(RC_ARCHS)" ; \
	    else \
		archs=`/usr/bin/arch` ; \
	    fi ; \
	fi ; \
	if [ -z "$$archs" ] ; then \
		archs=`/usr/bin/arch` ; \
	fi ; \
        archless_rcflags=`$(DEARCHIFY) $(RC_CFLAGS)` ; \
        arch_flags=`$(ARCHIFY) $$archs` ; \
	$(set_build_output_dirs)

set_build_output_dirs = \
	if [ -n "$(BUILD_OUTPUT_DIR)" ] ; then \
		build_output_dir="$(BUILD_OUTPUT_DIR)" ; \
	else \
		build_output_dir="." ; \
	fi ; \
	if [ -n "$(SYMROOT)" ] ; then \
		symroot="$(SYMROOT)" ; \
	else \
		symroot=$$build_output_dir ; \
	fi ; \
	if [ -n "$(OBJROOT)" ] ; then \
		objroot="$(OBJROOT)" ; \
	else \
		objroot=$$build_output_dir ; \
	fi

set_bundle_ext = \
	if [ -z "$(BUNDLE_EXTENSION)" ] ; then \
	   bundle_ext=$(DEFAULT_BUNDLE_EXTENSION) ; \
	else \
	   bundle_ext=$(BUNDLE_EXTENSION) ; \
	fi 

DYNAMIC_CFLAGS = -fno-common
DYNAMIC_LDFLAGS = # -all_load

set_language_flags = \
	if [ -n "$(RC_KANJI)" -o "$(JAPANESE)" = "YES" ] ; then \
		language_cflags='-DKANJI' ; \
		libs="$(LIBS:lNeXT_s=lNeXTJ_s)" ; \
		other_libs="$(OTHER_LIBS:lNeXT_s=lNeXTJ_s)" ; \
		other_japanese_libs="$(OTHER_JAPANESE_LIBS)" ; \
	else \
		language_cflags='' ; \
		libs="$(LIBS)" ; \
		other_libs="$(OTHER_LIBS)" ; \
		other_japanese_libs=""; \
	fi

set_dynamic_flags = \
	if [ "$@" = "shlib" ] ; then \
           dynamic_cflags="-static" ; \
	else \
           if [ "$(CODE_GEN_STYLE)" = "DYNAMIC" ] ; then \
                buildtype="dynamic"                         ; \
                dynamic_cflags="-dynamic $(DYNAMIC_CFLAGS)" ; \
		library_ext="dylib" ; \
           else \
                buildtype="static"          ; \
                dynamic_cflags="-static" ; \
		library_ext="a" ; \
           fi ; \
        fi ; \
	if [ "$@" = "shlib" -o "$@" = "posix" ] ; then \
            buildtype="$@"          ; \
        fi ; \
       	libname="lib$(NAME).$$library_ext"


set_dynamic_link_flags = \
        if [ "$(CODE_GEN_STYLE)" = "DYNAMIC" ]; then \
                dynamic_libtool_flags="-dynamic -install_name $(DYLIB_INSTALL_DIR)/$(DYLIB_INSTALL_NAME)" ; \
                dynamic_ldflags="$(DYNAMIC_LDFLAGS)" ; \
        else \
                dynamic_libtool_flags="-static" ; \
        fi


set_objdir = \
	if [ "$@" = "debug" -o "$@" = "profile" ] ; then \
		objdir="$@_obj" ; \
	else \
		objdir="obj" ; \
	fi

set_build_for_arch = \
	build_for_arch=yes; \
	for excluded_arch in $(EXCLUDED_ARCHS) none ; do \
	    if [ "$$arch" = "$$excluded_arch" ] ; then \
	        build_for_arch=no; \
	    fi ; \
	done ; \
	if [ -n "$(INCLUDED_ARCHS)" ] ; then \
	   build_for_arch=no; \
	   for included_arch in $(INCLUDED_ARCHS) none ; do \
	       if [ "$$arch" = "$$included_arch" ] ; then \
	          build_for_arch=yes; \
	       fi ; \
	   done ; \
	fi


### Define all the targets necessary at every level of the project-hierarchy:

# The following rules and rule fragments do the recursion into "sub" projects
# of this project and does a 'make project' for each one in its
# respective directories.  This insures that we do not rely on the directory
# timestamp or "hack" file to know whether or not something has changed.  

CHANGES_COMMAND = $(CHANGES) $(TOP_PRODUCT_ROOT)/$(CHANGES_FILE_BASE).$(TARGET_ARCH) "$(OFILE_DIR)"

use_default_directory_args = \
	top_prod_root=`echo $(TOP_PRODUCT_ROOT) | sed '/^[^/]/s:^:../:'` ; \
	prod_root=`echo $(PRODUCT_ROOT) | sed '/^[^/]/s:^:../:'` ; \
	ofile_dir=`echo $(OFILE_DIR) | sed '/^[^/]/s:^:../:'` ; \
	sym_dir=`echo $(SYM_DIR) | sed '/^[^/]/s:^:../:'` ; \
	header_base=`echo $(DEV_HEADER_DIR_BASE) | sed '/^[^/]/s:^:../:'` ; \
	project_header_base=`echo $(DEV_PROJECT_HEADER_DIR_BASE) | sed '/^[^/]/s:^:../:'` ; \
        propogated_cflags=`echo $(PROPOGATED_CFLAGS) | sed 's:-I../:-I../../:g'` 

use_absolute_directory_args = \
      $(PUSHD) $(PRODUCT_ROOT) ; abs_prod_root=`pwd` ; $(POPD) ; \
      prod_root=$$abs_prod_root ; \
      $(PUSHD) $(TOP_PRODUCT_ROOT) ; abs_top_prod_root=`pwd` ; $(POPD) ; \
      top_prod_root=$$abs_top_prod_root ; \
      $(PUSHD) $(OFILE_DIR) ; abs_ofile_dir=`pwd` ; $(POPD) ; \
      ofile_dir=$$abs_ofile_dir ; \
      $(PUSHD) $(SYM_DIR) ; abs_sym_dir=`pwd` ; $(POPD) ; \
      sym_dir=$$abs_sym_dir ; \
      $(PUSHD) $(DEV_HEADER_DIR_BASE) ; abs_header_base=`pwd` ; $(POPD) ; \
      header_base=$$abs_header_base ; \
      $(PUSHD) $(DEV_PROJECT_HEADER_DIR_BASE) ; abs_project_header_base=`pwd` ; $(POPD) ; \
      project_header_base=$$abs_project_header_base ; \
      propogated_cflags="$(PROPOGATED_CFLAGS)"


ALL_SUBPROJECTS = $(BUILD_TOOLS) $(SUBPROJECTS) $(BUNDLES) $(FRAMEWORK_SUBPROJECTS) $(LIBRARIES) $(TOOLS) $(LEGACIES)
IS_TOPLEVEL = 

# may not be right if tool is renamed without renaming directory name
TOOL_NAMES = $(TOOLS=.tproj=)

all_subprojects: $(TOP_PRODUCT_ROOT)
	@(if [ -z "$(ONLY_SUBPROJECT)" ] ; then \
	    if [ -n "$(BUILD_ALL_SUBPROJECTS)" ] ; then \
		subdirectories="$(ALL_SUBPROJECTS)" ; \
	    else \
	        subdirectories=`$(CHANGES_COMMAND) $(ALL_SUBPROJECTS)` ; \
	    fi ; \
	else \
	    subdirectories="$(ONLY_SUBPROJECT)" ; \
	fi ; \
	target=project; \
	beginning_msg="Making" ; ending_msg="Finished making" ; \
	actual_prod_root=$(PRODUCT_ROOT) ; \
	$(recurse_on_subdirectories) ; \
	$(check_tools) ; \
	if [ -n "$(IS_TOPLEVEL)" -a -z "$(ONLY_SUBPROJECT)" ] ; then \
	   $(RM) -f $(TOP_PRODUCT_ROOT)/$(CHANGES_FILE_BASE).$(TARGET_ARCH) ; \
	   $(TOUCH) $(TOP_PRODUCT_ROOT)/$(CHANGES_FILE_BASE).$(TARGET_ARCH) ; \
	fi)

$(SUBPROJ_OFILES):
	@(subdirectories="$(SUBPROJECTS)"; \
	target=project; \
	beginning_msg="Making" ; ending_msg="Finished making" ; \
  	$(recurse_on_subdirectories))

TOOL_NAMES = $(TOOLS:.tproj=)
check_tools = \
	for tool in $(TOOL_NAMES) none ; do \
	   if [ $$tool = "none" ] ; then break; fi ; \
	   tool_file="$$actual_prod_root/$$tool.$(TARGET_ARCH)" ; \
	   if [ ! -s $$tool_file ] ; then \
	      echo $$tool_file no longer exists...; \
	      subdirectories=$$tool.tproj; \
	      target=project; \
	      beginning_msg="Making" ; ending_msg="Finished making" ; \
  	      $(recurse_on_subdirectories) ; \
	   fi ; \
	done

recurse_on_subdirectories = \
   	$(use_default_directory_args) ;\
	for sub in $$subdirectories none ; do \
	   if [ $$sub = "none" ] ; then break; fi ; \
	   if [ -h $$sub ] ; then \
		$(use_absolute_directory_args) ; \
	   fi ; \
           $(PUSHD) $$sub; \
	   if [ -n "$$beginning_msg" ] ; then \
		$(ECHO) $$beginning_msg $$sub ; \
	   fi ; \
	   $(MAKE) $$target $(exported_vars) ; \
	   $(POPD) ; \
	   if [ -n "$$ending_msg" ] ; then \
		$(ECHO) $$ending_msg $$sub ; \
	   fi ; \
	done


exported_vars = \
	"PRODUCT_ROOT = $$prod_root" \
	"TOP_PRODUCT_ROOT = $$top_prod_root" \
	"OFILE_DIR = $$ofile_dir/$$sub" \
	"PRODUCT_PREFIX = $$ofile_dir/$$sub" \
	"BUNDLE_DIR = $$prod_root/$$sub" \
	"REL_BUNDLE_DIR = $$sub" \
	"SYM_DIR = $$sym_dir/$$sub" \
	"CODE_GEN_STYLE = $(CODE_GEN_STYLE)" \
	"BUILD_OFILES_LIST_ONLY = $(BUILD_OFILES_LIST_ONLY)" \
	"TARGET_ARCH = $(TARGET_ARCH)" \
	"COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
	"APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
	"MAKEFILEDIR = $(MAKEFILEDIR)" \
	"SRCROOT = $(SRCROOT)" \
	"OBJROOT = $(OBJROOT)" \
	"SYMROOT = $(SYMROOT)" \
	"PROPOGATED_CFLAGS = $(PROJ_CFLAGS) $$propogated_cflags -I$$sym_dir" \
	"PROPOGATED_FRAMEWORK_CFLAGS = $(ALL_FRAMEWORK_CFLAGS)" \
	"PROPOGATED_LDFLAGS = $(PROJECT_SPECIFIC_LDFLAGS) $(PROPOGATED_LDFLAGS)" \
	"DEV_HEADER_DIR_BASE = $$header_base" \
	"DEV_PROJECT_HEADER_DIR_BASE = $$project_header_base" \
	"MULTIPLE_ARCHS = $(MULTIPLE_ARCHS)" \
	"SINGLE_ARCH = $(SINGLE_ARCH)" \
	"DEPENDENCIES =" \
	"ONLY_SUBPROJECT =" \
	"RC_ARCHS = $(RC_ARCHS)" \
	"RC_CFLAGS = $(RC_CFLAGS)" \
	"ALL_ARCH_FLAGS = $(ALL_ARCH_FLAGS)" \
	$(projectType_specific_exported_vars)

configure_for_target_archs_exported_vars = \
	$(exported_vars) \
	$(extra_configure_for_target_archs_exported_vars)

# Finalizing build (e.g. lipo/ln the arch-specific binaries into place)

configure_for_target_archs::
	@(subdirectories="$(ALL_SUBPROJECTS)"; \
	target=configure_for_target_archs ; \
   	$(use_default_directory_args) ;\
	for sub in $$subdirectories none ; do \
	   if [ $$sub = "none" ] ; then break; fi ; \
	   if [ -h $$sub ] ; then \
		$(use_absolute_directory_args) ; \
	   fi ; \
           $(PUSHD) $$sub; \
	   $(MAKE) $$target $(configure_for_target_archs_exported_vars) ; \
	   $(POPD) ; \
	done)



# Finalizing installation

STRIP_ON_INSTALL = YES

finalize_install_exported_vars = \
	"DSTROOT = $(DSTROOT)" \
	"OBJROOT = $(OBJROOT)" \
	"SYMROOT = $(SYMROOT)" \
	"SYM_DIR = $(SYM_DIR)" \
	"DEVROOT = $(DEVROOT)" \
	"INSTALLDIR = $(INSTALLDIR)" \
	"PRODUCT_ROOT = $(PRODUCT_ROOT)" \
	"PRODUCT = $(PRODUCT)" \
	"OFILE_DIR = $(OFILE_DIR)" \
	"PROJ_CFLAGS = $(PROJ_CFLAGS)" \
        "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
	"APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
	"MAKEFILEDIR = $(MAKEFILEDIR)" \
	"RC_CFLAGS = $(RC_CFLAGS)" \
	"RC_ARCHS = $(RC_ARCHS)" \
	$(extra_finalize_install_exported_vars)

# Finalizing build (e.g. lipo/ln the arch-specific binaries into place)

finalize_install:: strip_myself after_install
	@(subdirectories="$(ALL_SUBPROJECTS)"; \
	target=finalize_install ; \
   	$(use_default_directory_args) ;\
	for sub in $$subdirectories none ; do \
	   if [ $$sub = "none" ] ; then break; fi ; \
	   if [ -h $$sub ] ; then \
		$(use_absolute_directory_args) ; \
	   fi ; \
           $(PUSHD) $$sub; \
	   $(MAKE) $$target $(finalize_install_exported_vars) ; \
	   $(POPD) ; \
	done)

strip_myself::

after_install::


# Resources stuff:

GENERATED_RESOURCES = `echo *.info > /dev/null 2>&1`

# The following rule insures that resources for this particular level in the project hierarchy get copied over to the appropriate place in the PRODUCT_ROOT.  Note that we depend on VPATH including $(LANGUAGE).lproj so that the LOCAL_RESOURCES are found correctly.  FASTCP is used to minimize the copying of files, since most resources are likely to be up to date most of the time.

resources:: $(LOCAL_RESOURCES) $(GLOBAL_RESOURCES) $(HELP_STORES) $(OTHER_RESOURCES)
	@(if [ "$(PRODUCT_ROOT)" != "." ] ; then \
		$(MKDIRS) $(PRODUCT_ROOT)/$(LANGUAGE).lproj ; \
	fi ; \
	if [ "$(LOCAL_RESOURCES)" != "" ] ; then \
	   locals="" ; \
	   for resource in $(LOCAL_RESOURCES) none ; do   \
	      insert="true"; \
	      for helpdir in $(HELP_DIRS) $(OTHER_HELP_DIRS) none ; do \
	         if [ "$$resource" = "$$helpdir" ] ; \
   		    then insert="false"; \
		 fi ; \
	      done ; \
	      if [ "$$insert" = "true" ] ; then \
	         locals="$$locals $$resource" ; \
	      fi ; \
	   done ; \
	   $(FASTCP) $$locals $(PRODUCT_ROOT)/$(LANGUAGE).lproj ; \
	fi ; \
	$(FASTCP) $(GLOBAL_RESOURCES) $(GENERATED_RESOURCES) $(PRODUCT_ROOT))


# rules for copying, cleaning and making dependencies

installsrc:: SRCROOT
	@($(MAKE) copy "DEST=$(SRCROOT)" \
	               "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
		       "APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
		       "MAKEFILEDIR = $(MAKEFILEDIR)" )

copy:: $(NAME).copy $(BUNDLES:.bproj=.copy) \
		    $(SUBPROJECTS:.subproj=.copy) \
		    $(TOOLS:.tproj=.copy)

$(NAME).copy:: DEST $(DEST) $(SRCFILES)
	@(if [ "$(SRCFILES)" != "" ] ; then \
	       $(ECHO) "$(TAR) cf - $(SRCFILES) | (cd $(DEST); $(TAR) xf -)" ; \
	       $(TAR) cf - $(SRCFILES) | (cd $(DEST); $(TAR) xf -) ; \
	fi ; \
	$(MKDIRS) $(DEST)/$(LANGUAGE).lproj ; \
	if [ "$(LOCAL_RESOURCES)" != "" ] ; then \
	   $(ECHO) "(cd $(LANGUAGE).lproj; $(TAR) cf - $(LOCAL_RESOURCES)) | (cd $(DEST)/$(LANGUAGE).lproj; $(TAR) xf - )" ; \
	   (cd $(LANGUAGE).lproj; $(TAR) cf - $(LOCAL_RESOURCES)) | (cd $(DEST)/$(LANGUAGE).lproj; $(TAR) xf -) ; \
	fi ; \
	supportfiles="" ; \
	for i in $(SUPPORTFILES) $(APPICON) $(DOCICONS) none ; do \
	    if [ -r $$i -a ! -r $(DEST)/$$i ] ; then \
		supportfiles="$$supportfiles $$i" ; \
	    fi ; \
	done ; \
	if [ "$$supportfiles" != "" ] ; then \
	   $(ECHO) "$(TAR) cf - $$supportfiles | (cd $(DEST); $(TAR) xf -)" ; \
	   $(TAR) cf - $$supportfiles | (cd $(DEST); $(TAR) xf -) ; \
	fi)

.bproj.copy .subproj.copy .tproj.copy:
	@(cd $<; $(MAKE) copy "NAME=$*" "DEST=$(DEST)/$<")

.bproj.clean .subproj.clean .tproj.clean:
	@(echo Cleaning $<... ; cd $<; \
	  $(MAKE) clean "NAME=$*")

.bproj.depend .subproj.depend .tproj.depend:
	@(sym_dir=`echo $(SYM_DIR) | sed '/^[^/]/s:^:../:'` ; \
	cd $<; \
	$(MAKE) depend "NAME=$*" \
		       "PROPOGATED_CFLAGS = $(PROJ_CFLAGS) $(PROPOGATED_CFLAGS) -I$$sym_dir/$<" )



# Build a set of dependencies for current level into Makefile.depndencies 
   
Makefile.dependencies:: $(CLASSES) $(MFILES) $(CFILES) $(CCFILES) $(CAPCFILES) $(CAPMFILES) $(CXXFILES) $(CPPFILES) $(INITIAL_TARGETS)
	@($(RM) -f Makefile.dependencies ; \
	if [ "`$(ECHO) $(CLASSES) $(MFILES) $(CFILES) $(CCFILES) $(CAPCFILES) $(CAPMFILES) $(CXXFILES) $(CPPFILES) | wc -w`" != "       0" ] ; then \
		if [ "$(ENGLISH)" = "YES" -o "$(JAPANESE)" != "YES" ] ; then \
			language_cflags="" ; \
		else \
			language_cflags="-DKANJI" ; \
		fi ; \
		$(ECHO) "$(CC) -MM $(PROJ_CFLAGS) $(PROPOGATED_CFLAGS) -I$(SYM_DIR)  $$language_cflags $(CLASSES) $(MFILES) $(CFILES) $(CCFILES) $(CAPCFILES) $(CAPMFILES) $(CXXFILES) $(CPPFILES) > Makefile.dependencies" ; \
		$(CC) -MM $(PROJ_CFLAGS) $(PROPOGATED_CFLAGS) -I$(SYM_DIR)  $$language_cflags $(CLASSES) $(MFILES) $(CFILES) $(CCFILES) $(CAPCFILES) $(CAPMFILES) $(CXXFILES) $(CPPFILES) > Makefile.dependencies || ($(RM) -f Makefile.dependencies ; exit 1) ; \
	fi );


SRCROOT DEST:
	@if [ -n "${$@}" ]; then exit 0; \
	else $(ECHO) Must define $@; exit 1; fi

$(DEST)::
	-$(RM) -rf $(DEST)
	@$(MKDIRS) $(DEST)

		
# Header stuff:

DEV_HEADER_DIR_BASE = $(SYMROOT)
DEV_PROJECT_HEADER_DIR_BASE = $(SYMROOT)
DEV_PUBLIC_HEADER_DIR  = $(DEV_HEADER_DIR_BASE)/Headers
DEV_PROJECT_HEADER_DIR = $(DEV_PROJECT_HEADER_DIR_BASE)/ProjectHeaders
DEV_PRIVATE_HEADER_DIR = $(DEV_HEADER_DIR_BASE)/PrivateHeaders

installhdrs::  
	@(echo == Making installhdrs for $(NAME) == ; \
	$(MAKE) copy_all_headers \
                "PUBLIC_HEADER_DEST_DIR = $(DSTROOT)$(PUBLIC_HEADER_DIR)" \
                "PROJECT_HEADER_DEST_DIR =" \
                "PRIVATE_HEADER_DEST_DIR = $(DSTROOT)$(PRIVATE_HEADER_DIR)" \
                "HEADER_COPY_CMD = $(FASTCP)" \
                "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
      	  	"APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
		"MAKEFILEDIR = $(MAKEFILEDIR)" \
		"RC_CFLAGS = $(RC_CFLAGS)" \
		"SYMROOT = $(SYMROOT)" \
		"DSTROOT = $(DSTROOT)")

copy_all_headers:: copy_my_headers after_installhdrs
	@(for dir in $(ALL_SUBPROJECTS) none ; do \
	  if [ $$dir = "none" ] ; then break; fi ; \
	  $(PUSHD) $$dir; $(ECHO) Installing headers for $$dir ; \
	  $(MAKE) copy_all_headers \
		   "PUBLIC_HEADER_DEST_DIR = $(PUBLIC_HEADER_DEST_DIR)" \
		   "PROJECT_HEADER_DEST_DIR = $(PROJECT_HEADER_DEST_DIR)" \
		   "PRIVATE_HEADER_DEST_DIR = $(PRIVATE_HEADER_DEST_DIR)" \
		   "HEADER_COPY_CMD = $(HEADER_COPY_CMD)" \
                   "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
		   "APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
		   "MAKEFILEDIR = $(MAKEFILEDIR)" \
		   "DSTROOT = $(DSTROOT)" \
		   "SRCROOT = $(SRCROOT)" \
		   "OBJROOT = $(OBJROOT)" \
		   "SYMROOT = $(SYMROOT)" ; \
	   $(POPD) ; \
	done)


export_headers:
	@(if [ "$(IS_TOPLEVEL)" = "YES" \
	       -a "$(EXPORT_PROJECT_HEADERS)" = "YES" ] ; then \
		$(ECHO) Exporting project headers... ; \
		$(MAKE) export_project_headers \
			"DEV_PUBLIC_HEADER_DIR =" \
			"DEV_PRIVATE_HEADER_DIR =" \
               		"COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)"\
			"APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
			"MAKEFILEDIR = $(MAKEFILEDIR)" \
			"DSTROOT = $(DSTROOT)" \
			"SRCROOT = $(SRCROOT)" \
			"OBJROOT = $(OBJROOT)" \
			"SYMROOT = $(SYMROOT)" ; \
		$(ECHO) Finished exporting project headers. ; \
	fi)

export_project_headers:
	@($(MAKE) copy_my_headers \
		"PUBLIC_HEADER_DEST_DIR = $(DEV_PUBLIC_HEADER_DIR)" \
		"PROJECT_HEADER_DEST_DIR = $(DEV_PROJECT_HEADER_DIR)" \
		"PRIVATE_HEADER_DEST_DIR = $(DEV_PRIVATE_HEADER_DIR)" \
		"HEADER_COPY_CMD = $(FASTLN)" \
                "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
		"APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
		"MAKEFILEDIR = $(MAKEFILEDIR)" \
		"DSTROOT = $(DSTROOT)" \
		"SRCROOT = $(SRCROOT)" \
		"OBJROOT = $(OBJROOT)" \
		"SYMROOT = $(SYMROOT)" ; \
		subdirectories="$(ALL_SUBPROJECTS)"; \
		target=export_project_headers ; \
  		$(recurse_on_subdirectories))
			 
copy_my_headers:: $(PUBLIC_HEADERS) $(OTHER_PUBLIC_HEADERS) $(PROJECT_HEADERS) $(OTHER_PROJECT_HEADERS) $(PRIVATE_HEADERS) $(OTHER_PRIVATE_HEADERS)
	@(if [ \( -n "$(PUBLIC_HEADERS)" -o \
	       -n "$(OTHER_PUBLIC_HEADERS)" \) -a \
	       -n "$(PUBLIC_HEADER_DEST_DIR)" ] ; then \
	   $(MKDIRS) $(PUBLIC_HEADER_DEST_DIR) ; \
	   $(HEADER_COPY_CMD) $(PUBLIC_HEADERS) $(OTHER_PUBLIC_HEADERS) $(PUBLIC_HEADER_DEST_DIR) ; \
	fi ; \
	if [ \( -n "$(PROJECT_HEADERS)" -o \
	     -n "$(OTHER_PROJECT_HEADERS)" \) -a \
	     -n "$(PROJECT_HEADER_DEST_DIR)" ] ; then \
	   $(MKDIRS) $(PROJECT_HEADER_DEST_DIR) ; \
	   $(HEADER_COPY_CMD) $(PROJECT_HEADERS) $(OTHER_PROJECT_HEADERS) $(PROJECT_HEADER_DEST_DIR) ; \
	fi ; \
	if [ \( -n "$(PRIVATE_HEADERS)" -o \
	     -n "$(OTHER_PRIVATE_HEADERS)" \) -a \
	     -n "$(PRIVATE_HEADER_DEST_DIR)" ] ; then \
	   $(MKDIRS) $(PRIVATE_HEADER_DEST_DIR) ; \
	   $(HEADER_COPY_CMD) $(PRIVATE_HEADERS) $(OTHER_PRIVATE_HEADERS) $(PRIVATE_HEADER_DEST_DIR) ; \
	fi)

after_installhdrs::
	@(if [ -n "$(PUBLIC_PRECOMPILED_HEADERS)" -a -n "$(PUBLIC_HEADER_DIR)" ] ; then \
	   cd $(DSTROOT)/$(PUBLIC_HEADER_DIR) ; \
	   for header in $(PUBLIC_PRECOMPILED_HEADERS) none ; do \
	      if [ $$header = "none" ] ; then break; fi ; \
	      cmd="cc -precomp $(PUBLIC_PRECOMPILED_HEADERS_CFLAGS) $(RC_CFLAGS) $$header";\
	      $(ECHO) $$cmd ; $$cmd ; \
	   done ; \
	fi)


# Cleaning stuff:	

clean:: 
	@(echo == Making clean for $(NAME) == ; \
	$(set_bundle_ext) ; \
	$(process_target_archs) ; \
	if [ -n "$(CLEAN_ALL_SUBPROJECTS)" ] ; then \
	   $(MAKE) actual_clean really_clean \
		   "DEV_HEADER_DIR_BASE = $(DEV_HEADER_DIR_BASE)" \
	           "BUNDLE_EXTENSION = $$bundle_ext" \
		   "APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
               	   "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
                   "SYMROOT = $$symroot" \
                   "OBJROOT = $$objroot" ; \
	else \
	   $(MAKE) actual_clean "BUNDLE_EXTENSION = $$bundle_ext" \
		   "DEV_HEADER_DIR_BASE = $(DEV_HEADER_DIR_BASE)" \
		   "APP_MAKEFILE_DIR = $(APP_MAKEFILE_DIR)" \
               	   "COMMON_APP_MAKEFILE_DIR = $(COMMON_APP_MAKEFILE_DIR)" \
                   "SYMROOT = $$symroot" \
                   "OBJROOT = $$objroot" ; \
	fi )
	
actual_clean:: $(NAME).clean 

really_clean:: 	
	@(subdirectories="$(ALL_SUBPROJECTS)" ;\
	target="actual_clean really_clean"; \
	beginning_msg="Cleaning" ; ending_msg="Finished cleaning" ; \
	$(recurse_on_subdirectories))

$(NAME).clean::
	@if [ ! -w . ] ; then $(ECHO) '***' project write-protected; exit 1 ; fi
	$(RM) -rf $(GARBAGE)


# Misc. old stuff:

writable::
	@chmod -R +w *

protected::
	@chmod -R a-w *


