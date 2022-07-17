CC=gcc
IDIR=/home/crush/Documents/fluff/classicube/source/src/
CFLAGS=-shared -ldl -fPIC -I$(IDIR)

build: fakeUnicodeSupport.c
	$(CC) fakeUnicodeSupport.c -o ../bin/fakeUnicodeSupport.so $(CFLAGS)