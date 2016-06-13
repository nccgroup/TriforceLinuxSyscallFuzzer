
CFLAGS= -g -Wall

all : driver testAfl heater

OBJS= aflCall.o driver.o parse.o sysc.o argfd.o
driver: $(OBJS)
	$(CC) $(CFLAGS) -static -o $@ $(OBJS)

HOBJS= heater.o parse.o sysc.o argfd.o
heater: $(HOBJS)
	$(CC) $(CFLAGS) -static -o $@ $(HOBJS)

testAfl : testAfl.o
	$(CC) $(CFLAGS) -o $@ testAfl.o

testRoot.cpio.gz : driver heater
	./makeRoot testRoot driver -vv

fuzzRoot.cpio.gz : driver heater
	./makeRoot fuzzRoot driver

inputs/ex1 : gen.py
	test -d inputs || mkdir inputs
	./gen.py

clean:
	rm -f $(OBJS) $(HOBJS) testAfl.o
	rm -rf testRoot* fuzzRoot*

