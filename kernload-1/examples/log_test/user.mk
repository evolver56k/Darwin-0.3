NAME=log
CFILES= log.c
MIGCFILES= logUser.c 
MIGINCLUDES= log.h
HFILES= log_types.h

CFLAGS= -g -O -MD -DMACH -I../../include  ${RC_CFLAGS}
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)


all:	$(NAME)

$(NAME): $(OFILES)
	cc -o $(NAME) $(CFLAGS) $(OFILES)

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	md -u Makedep -d $*.d

logUser.c log.h: log.defs 
	mig ${MIGFLAGS} log.defs

-include Makedep


