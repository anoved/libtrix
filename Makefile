PREFIX = /usr/local
CFLAGS = -Wall

.PHONY: default lib install uninstall clean

default: lib

libtrix.o: libtrix.c libtrix.h
	gcc $(CFLAGS) -c libtrix.c

lib: libtrix.a

libtrix.a: libtrix.o
	ar r libtrix.a libtrix.o

install: libtrix.a
	cp libtrix.a $(PREFIX)/lib
	cp libtrix.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/libtrix.a
	rm -f $(PREFIX)/include/libtrix.h

clean:
	rm -f libtrix.o
	rm -f libtrix.a
