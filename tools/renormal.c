// Usage: ./renormal < input.stl > output.stla
// Reads input.stl model, recomputes normal vectors of each face, and prints ASCII STL including updated normals. 

#include <stdio.h>
#include <libtrix.h>

int main(int argc, char **argv) {
	trix_mesh *mesh;
	
	if (trixRead(NULL, &mesh) != TRIX_OK) {
		fprintf(stderr, "input failed\n");
		return 1;
	}
	
	if (trixRecalculateNormals(mesh) != TRIX_OK) {
		fprintf(stderr, "surface normals calculation failed\n");
		return 1;
	}
	
	if (trixWrite(NULL, mesh, TRIX_STL_ASCII) != TRIX_OK) {
		fprintf(stderr, "output failed\n");
		return 1;
	}
	
	(void)trixRelease(mesh);
	
	return 0;
}
