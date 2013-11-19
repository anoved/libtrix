// Usage: ./facecount model.stl ...
// Prints number of faces in each STL model named on command line.

#include <stdio.h>
#include <libtrix.h>

void PrintMeshFacecount(const char *path) {
	trix_mesh *m;
	trix_result r;
	
	if ((r = trixRead(&m, path)) != TRIX_OK) {
		fprintf(stderr, "Cannot open %s (%d)\n", path, (int)r);
		return;
	}
	
	printf("%s: %lu\n", path, (unsigned long)m->facecount);
	
	(void)trixRelease(&m);
}

int main(int argc, char **argv) {
	int m;
	for (m = 1; m < argc; m++) {
		PrintMeshFacecount(argv[m]);
	}
	return 0;
}
