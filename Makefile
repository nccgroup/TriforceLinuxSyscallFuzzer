
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

# fuzzRoot boots into the driver
fuzzRoot.cpio.gz : driver heater
	./makeRoot fuzzRoot driver

# testRoot boots into the driver with verbose flag set
testRoot.cpio.gz : driver heater
	./makeRoot testRoot driver -vv

inputs : gen.py
	test -d inputs || mkdir inputs
	./gen.py

clean:
	rm -f $(OBJS) $(HOBJS) testAfl.o
	rm -rf testRoot* fuzzRoot*

