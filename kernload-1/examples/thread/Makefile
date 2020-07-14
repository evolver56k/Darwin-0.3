NAME=thread

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

SECTFILES=Load_Commands.sect Unload_Commands.sect
CFILES= thread.c
OFILES= thread.o
SOURCEFILES= Makefile $(CFILES) $(SECTFILES) $(HFILES) \
	README $(MIGFILES)
CFLAGS= -g -O -MD -DKERNEL -DKERNEL_FEATURES -DMACH -I../../include ${RC_FLAGS} 
LD=../../cmds/kl_ld/kl_ld


all:	$(NAME)_reloc

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

$(SRCDIR):
	$(MKDIRS) $@

clean:
	-$(RM) -rf $(NAME)_reloc $(NAME)_loadable $(OFILES) *.d *.o \
		Makedep Server_Name.sect Loadable_Name.sect \
		Executable_Name.sect Instance_Var.sect

$(NAME)_reloc : $(OFILES) Load_Commands.sect Unload_Commands.sect
	${LD} -n $(NAME) -l Load_Commands.sect -u Unload_Commands.sect \
		-i instance -d $(NAME)_loadable -o $@ $(OFILES)

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	md -u Makedep -d $*.d

-include Makedep
