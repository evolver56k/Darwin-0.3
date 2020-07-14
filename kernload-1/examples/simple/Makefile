NAME=simple

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

SECTFILES=Load_Commands.sect Unload_Commands.sect
CFILES= simple.c
MFILES= simple_server.m
MIGFILES= simple.defs
MIGCFILES= simple_user.c simple_handler.c
MIGINCLUDES= simple_handler.h simple.h
HFILES= simple_types.h
SOURCEFILES= Makefile $(CFILES) $(MFILES) $(SECTFILES) $(HFILES) \
	README server.mk user.mk $(MIGFILES)

all:	$(NAME)_reloc $(NAME)

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

$(SRCDIR):
	$(MKDIRS) $@

clean:
	-$(RM) -rf $(NAME) $(NAME)_reloc $(NAME)_loadable $(SOFILES) $(UOFILES) \
		$(MIGCFILES) $(MIGINCLUDES) *.d *.o \
		Makedep Server_Name.sect Loadable_Name.sect \
		Executable_Name.sect Instance_Var.sect

$(NAME)_reloc: FRC
	$(MAKE) -f server.mk $(NAME)_reloc

$(NAME): FRC
	$(MAKE) -f user.mk $(NAME)

FRC:
