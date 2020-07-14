#
# kernelserver.make
#
# Variable definitions and rules for building kernelserver projects.  A
# kernelserver is a fragment of code that can be loaded into the kernel
# as part of a IOKit project.
#
# PUBLIC TARGETS
#    kernelserver: synonymous with all
#
# IMPORTED VARIABLES
#    none
#
# EXPORTED VARIABLES
#    none
#

.PHONY: kernelserver all

kernelserver: all
PROJTYPE = KernelServerProjectType

ifeq "" "$(PRODUCT)"
    PRODUCT = $(PRODUCT_DIR)/$(NAME)_reloc$(EXECUTABLE_EXT)
endif

PRODUCTS = $(PRODUCT)
STRIPPED_PRODUCTS = $(PRODUCT)
KL_LD = /usr/bin/kl_ld

CREATE_INSTANCE_CMD = CreateKLLDInstance.sh
INSTANCE_FILE = $(NAME)_instance.m
INSTANCE_OBJFILE = $(NAME)_instance.o

ifeq "NO" "$(USE_OBJC_KSINSTANCE)"
    INSTANCE_FILE = $(NAME)_instance.c
    CREATE_INSTANCE_CMD := $(CREATE_INSTANCE_CMD) -C
endif

OTHER_GENERATED_SRCFILES += $(INSTANCE_FILE)
OTHER_GENERATED_OFILES += $(INSTANCE_OBJFILE)

ifndef LOAD_SECTION
    LOAD_SECTION = Load_Commands.sect
endif
ifndef UNLOAD_SECTION
    UNLOAD_SECTION = Unload_Commands.sect
endif

#
# Specify name of load_commands and unload_commands text files for 
# kern-loadable module.
#
KL_LDFLAGS_NAME = -n $(NAME)
KL_LDFLAGS_INSTANCE = -i $(NAME)_instance

ifndef KL_LDFLAGS_DEBUG
    ifeq "YES" "$(DEBUG)"
        KL_LDFLAGS_DEBUG = -d $(NAME)_loadable
    endif
endif

ifndef KL_LDFLAGS_LOAD_COMMANDS
    ifneq "" "$(filter $(LOAD_SECTION), $(OTHERSRCS))"
        KL_LDFLAGS_LOAD_COMMANDS = -l $(LOAD_SECTION)
    endif
endif

ifndef KL_LDFLAGS_UNLOAD_COMMANDS
    ifneq "" "$(filter $(UNLOAD_SECTION), $(OTHERSRCS))"
        KL_LDFLAGS_UNLOAD_COMMANDS = -u $(UNLOAD_SECTION)
    endif
endif

SYSTEM_FRAMEWORK = $(NEXT_ROOT)$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework
SYSTEM_HEADERS = $(SYSTEM_FRAMEWORK)/Headers
KERNEL_HEADERS = $(SYSTEM_FRAMEWORK)/PrivateHeaders
ifneq "" "$(wildcard $(KERNEL_HEADERS))"
    KERNEL_HEADERS_INCLUDES = -I$(KERNEL_HEADERS) \
    			      -I$(KERNEL_HEADERS)/ansi \
			      -I$(KERNEL_HEADERS)/bsd
endif

i386_PROJTYPE_CFLAGS = 
ppc_PROJTYPE_CFLAGS = -finline -fno-keep-inline-functions \
		      -force_cpusubtype_ALL \
		      -msoft-float -mcpu=604 -mlong-branch 
ARCH_PROJTYPE_FLAGS = $($(CURRENT_ARCH)_PROJTYPE_CFLAGS) \
		      $(OTHER_$(CURRENT_ARCH)_CFLAGS)

ifneq "NO" "$(USE_MACH_USER)"
    MACH_USER_API = -DMACH_USER_API
endif

PROJTYPE_CFLAGS = $(ARCH_PROJTYPE_FLAGS) -static \
		  -DKERNEL -D_KERNEL $(MACH_USER_API) \
		  -DKERNEL_SERVER_INSTANCE=$(NAME)_instance

ifndef LOCAL_MAKEFILEDIR
    LOCAL_MAKEFILEDIR = $(LOCAL_DEVELOPER_DIR)/Makefiles/pb_makefiles
endif
-include $(LOCAL_MAKEFILEDIR)/kernelserver.make.preamble

include $(MAKEFILEDIR)/common.make

#
# Make the global instance
# First find the Resource root for the CreateKLLDInstance shell script
# then generate and build the instance file itself.
#
PROJTYPE_RESOURCES = ProjectTypes/KernelServer.projectType/Resources

ifneq "" "$(wildcard $(NEXT_ROOT)$(LOCAL_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES))"
LKS_TOOLS_ROOT=$(NEXT_ROOT)$(LOCAL_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES)
else

ifneq "" "$(wildcard $(HOME)/Library/$(PROJTYPE_RESOURCES))"
LKS_TOOLS_ROOT=$(HOME)/Library/$(PROJTYPE_RESOURCES)
else
LKS_TOOLS_ROOT=$(NEXT_ROOT)$(SYSTEM_DEVELOPER_DIR)/$(PROJTYPE_RESOURCES)
endif
endif

KL_LDFLAGS = $(KL_LDFLAGS_NAME) $(KL_LDFLAGS_DEBUG) $(KL_LDFLAGS_INSTANCE) \
	     $(KL_LDFLAGS_LOAD_COMMANDS) $(KL_LDFLAGS_UNLOAD_COMMANDS) \
	     $(OPTIONAL_LDFLAGS)

$(PRODUCT): $(DEPENDENCIES)
	$(KL_LD) -o $(PRODUCT) $(KL_LDFLAGS) $(ARCHITECTURE_FLAGS) $(LOADABLES)

$(INSTANCE_FILE):
	$(SILENT) $(ECHO) Creating $@;
	$(SILENT) sh $(LKS_TOOLS_ROOT)/$(CREATE_INSTANCE_CMD) $(NAME) > $(SFILE_DIR)/$@

-include $(LOCAL_MAKEFILEDIR)/kernelserver.make.postamble
