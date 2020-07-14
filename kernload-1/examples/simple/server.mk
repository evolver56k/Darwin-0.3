NAME=simple
CFILES= simple_server.c
MIGCFILES= simple_handler.c
MIGHFILES= simple_handler.h
HFILES= simple_types.h
LD=../../cmds/kl_ld/kl_ld

CFLAGS= -g -O -MD -DKERNEL -DKERNEL_FEATURES -DMACH -I../../include  ${RC_CFLAGS}

OFILES= simple_server.o simple_handler.o

all:	$(NAME)_reloc

$(NAME)_reloc : $(OFILES) Load_Commands.sect Unload_Commands.sect
	${LD} -n $(NAME) -l Load_Commands.sect -u Unload_Commands.sect \
		-i instance -d $(NAME)_loadable -o $@ $(OFILES) $(RC_CFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	md -u Makedep -d $*.d

simple_server.o: simple_handler.h

$(MIGCFILES) $(MIGHFILES): simple.defs 
	mig ${MIGFLAGS} simple.defs -handler simple_handler.c -sheader simple_handler.h -user /dev/null -header /dev/null

-include Makedep
