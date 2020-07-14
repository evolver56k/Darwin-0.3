NAME=simple

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

SECTFILES=Load_Commands.sect Unload_Commands.sect
CFILES= simple.c simple_server.c
MIGFILES= simple.defs
MIGCFILES= simpleUser.c 
MIGINCLUDES= simpleServer.c simple.h
HFILES= simple_types.h
SOURCEFILES= Makefile $(CFILES) $(SECTFILES) $(HFILES) \
	README server.mk user.mk $(MIGFILES)

all:	$(NAME)_reloc $(NAME)

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

$(SRCDIR):
	$(MKDIRS) $@

clean:
	-$(RM) -rf $(NAME) $(NAME)_reloc $(NAME)_loadable $(SOFILES) $(UOFILES) \
		$(MIGCFILES) $(MIGINCLUDES) *.d *.o Makedep

$(NAME)_reloc: FRC
	$(MAKE) -f server.mk $(NAME)_reloc

$(NAME): FRC
	$(MAKE) -f user.mk $(NAME)

FRC:

