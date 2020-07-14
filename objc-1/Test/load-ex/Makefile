
CFLAGS = -O -Wall -g

all: load Bar.o Car.o Jar.o CategoryOfCar.o CategoryOfObject.o

load: load.o
	$(CC) $(CFLAGS) -o load load.o

clean:
	rm -f *.o load
