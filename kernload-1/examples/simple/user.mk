NAME=simple
CFILES= simple.c
MIGCFILES= simple_user.c 
MIGINCLUDES= simple.h
HFILES= simple_types.h

CFLAGS= -g -O -MD -DMACH -I../../include ${RC_CFLAGS}
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)


all:	$(NAME)

$(NAME): $(OFILES)
	cc -o $(NAME) $(CFLAGS) $(OFILES)

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	md -u Makedep -d $*.d

simple.c: simple.h

simple_user.c simple.h: simple.defs 
	mig ${MIGFLAGS} simple.defs -user simple_user.c -header simple.h -handler /dev/null

-include Makedep
