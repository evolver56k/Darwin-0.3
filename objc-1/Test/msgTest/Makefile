
#
# Define INT_32 if integers are 32 bits
# Remove this define for 16-bit integers
#
CFLAGS = -DINT_32 -g -O -arch m68k -arch m88k

all: rettest

OBJ = ret1.o ret2.o rettest.o

$(OBJ):	ret1.h ret2.h rettest.h

rettest: $(OBJ)
	$(CC) $(CFLAGS) -o rettest $(OBJ) -lsys_s

test:
	rettest

clean:
	/bin/rm -f *.o rettest
