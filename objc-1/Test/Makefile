
LDFLAGS = ../libobjc_g.a -lsys_s

CFLAGS = -O -g -Wall -no-precomp -arch m88k

TESTS = ForwardTest ObjcTest StringTableTest StringTest forward-simple typedstreamTest ExceptionTest # ProtocolTest

all: $(TESTS)

ForwardTest: ForwardTest.m
	$(CC) $(CFLAGS) -o ForwardTest ForwardTest.m $(LDFLAGS)

ObjcTest: ObjcTest.m
	$(CC) $(CFLAGS) -o ObjcTest ObjcTest.m $(LDFLAGS)

ProtocolTest: ProtocolTest.m
	$(CC) $(CFLAGS) -o ProtocolTest ProtocolTest.m $(LDFLAGS)

StringTableTest: StringTableTest.m
	$(CC) $(CFLAGS) -o StringTableTest StringTableTest.m $(LDFLAGS)

StringTest: StringTest.m
	$(CC) $(CFLAGS) -o StringTest StringTest.m $(LDFLAGS)

forward-simple: forward-simple.m
	$(CC) $(CFLAGS) -o forward-simple forward-simple.m $(LDFLAGS)

typedstreamTest: typedstreamTest.m
	$(CC) $(CFLAGS) -o typedstreamTest typedstreamTest.m $(LDFLAGS)

ExceptionTest: ExceptionTest.c
	$(CC) $(CFLAGS) -o ExceptionTest ExceptionTest.c $(LDFLAGS)

clean:
	/bin/rm -f $(TESTS) *.o *.s

install:
	cp -p $(TESTS) test.strings /Net/pbs/nrw_root/tests/mself
