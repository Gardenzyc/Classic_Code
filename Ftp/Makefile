all:Ftp_test
CFLAGS=-g -O0 -w

DLIBS=-lpthread -lftp_client
LDFLAGS=-
CC=gcc 
#arm-oe-linux-gnueabi-gcc

libftp_client.so:Ftp_client.cpp
	$(CC) $^ -shared -fPIC -o $@ $(CFLAGS)

Ftp_test:Ftp_test.cpp
	$(CC) -o $@ $^ $(DLIBS) $(CFLAGS)

.PHONY:clean

clean:
	-rm *.o *.so Ftp_test
