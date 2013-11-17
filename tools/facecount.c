// Usage: ./facecount model.stl ...
// Prints number of faces in each STL model named on command line.

#include <stdio.h>
#include <libtrix.h>

void PrintMeshFacecount(const char *path) {
	trix_mesh *m;
	
	if (trixRead(path, &m) != TRIX_OK) {
		fprintf(stderr, "Cannot open %s\n", path);
		return;
	}
	
	printf("%s: %lu\n", path, m->facecount);
	
	(void)trixRelease(m);
}

int main(int argc, char **argv) {
	int m;
	for (m = 1; m < argc; m++) {
		PrintMeshFacecount(argv[m]);
	}
	return 0;
}
