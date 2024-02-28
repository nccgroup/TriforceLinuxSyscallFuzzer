CFLAGS= -g
CC=arm-linux-gnueabi-gcc
all : driver

OBJS= aflCall.o driver.o parse.o sysc.o argfd.o
driver: $(OBJS)
	$(CC) $(CFLAGS) -static -o $@ $(OBJS) 
clean:
	rm -f $(OBJS) $(HOBJS) driver
