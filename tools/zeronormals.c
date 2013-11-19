// Usage: ./zeronormals < input.stl > output.stla
// Reads input.stl model, resets face normal vectors to 0 0 0, and prints reset ASCII STL. 

#include <stdio.h>
#include <libtrix.h>

int main(int argc, char **argv) {
	trix_mesh *mesh;
	
	if (trixRead(NULL, &mesh) != TRIX_OK) {
		fprintf(stderr, "input failed\n");
		return 1;
	}
	
	if (trixResetNormals(mesh) != TRIX_OK) {
		fprintf(stderr, "normal vector reset failed\n");
		return 1;
	}
	
	if (trixWrite(NULL, mesh, TRIX_STL_ASCII) != TRIX_OK) {
		fprintf(stderr, "output failed\n");
		return 1;
	}
	
	(void)trixRelease(mesh);
	
	return 0;
}
