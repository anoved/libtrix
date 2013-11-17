CC = gcc
PREFIX = /usr/local
CFLAGS = -fPIC -Wall

.PHONY: default install uninstall clean test

default: libtrix.a libtrix.so

libtrix.o: libtrix.c libtrix.h
	$(CC) $(CFLAGS) -c libtrix.c -o libtrix.o

# static library
libtrix.a: libtrix.o
	ar r libtrix.a libtrix.o

# shared library
libtrix.so: libtrix.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,libtrix.so -o libtrix.so libtrix.o

clean:
	rm -f libtrix.so libtrix.a libtrix.o

install: libtrix.so
	cp libtrix.a $(PREFIX)/lib
	cp libtrix.so $(PREFIX)/lib
	cp libtrix.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/libtrix.a
	rm -f $(PREFIX)/lib/libtrix.so
	rm -f $(PREFIX)/include/libtrix.h

# run tests, including static analyses
test:
	tclsh test/all.tcl -constraint static
