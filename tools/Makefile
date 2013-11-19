# Build PROG from PROG.c using shared library:
# gcc PROG.c -o PROG -ltrix -lm -L/usr/local/lib -Wl,-R/usr/local/lib
#
# Build PROG from PROG.c using static library:
# gcc PROG.c -o PROG -static -ltrix -lm -L/usr/local/lib
#
# Use `file PROG` to confirm executable type.
# Use `ldd PROG` to list used shared libraries.

CFLAGS = -Wall
LDFLAGS = -ltrix -lm -L/usr/local/lib -Wl,-R/usr/local/lib

.PHONY: all clean

all: facecount renormal zeronormals

facecount: facecount.c
	gcc $(CFLAGS) facecount.c -o facecount $(LDFLAGS)

renormal: renormal.c
	gcc $(CFLAGS) renormal.c -o renormal $(LDFLAGS)

zeronormals: zeronormals.c
	gcc $(CFLAGS) zeronormals.c -o zeronormals $(LDFLAGS)

clean:
	rm -f facecount renormal zeronormals
