
#include "libtrix.h"

unsigned long tmMeshFacecount(tm_mesh *mesh) {
	unsigned long count;
	tm_face *face;
	
	// sanity check mesh
	
	facecount = 0;
	face = mesh->first;
	while (face != NULL) {
		facecount += 1;
		face = face->next;
	}
	
	return facecount;
}

int tmWriteMeshHeaderBinary(FILE *stl_dst, tm_mesh *mesh) {
	char header[80];
	
	// could put some mesh metadata in header (facecount, provenance, etc)
	strncpy(header, "Binary STL", 80);
	
	if (fwrite(header, 80, 1, stl_dst) != 1) {
		// failed to write header
		return 1;
	}
	
	if (fwrite(&mesh->facecount, 4, 1, stl_dst) != 1) {
		// failed to write face count
		return 1;
	}
	
	return 0;
}

int tmWriteMeshHeaderASCII(FILE *stl_dst, tm_mesh *mesh) {
	// what are the length and content constraints on name?
	// not specified in http://www.ennex.com/~fabbers/StL.asp
	// so be conservative (or look for conventions elsewhere)
	// (consider using mesh->name instead of fixed name)
	fprintf(stl_dst, "solid MESH\n");
}

int tmWriteMeshFooterASCII(FILE *stl_dst, tm_mesh *mesh) {
	fprintf(stl_dst, "endsolid MESH\n");
}

int tmWriteMeshHeader(FILE *stl_dst, tm_mesh *mesh, tm_stl_mode mode) {
	return (mode == TM_STL_ASCII
			? tmWriteMeshHeaderASCII(stl_dst, mesh)
			: tmWriteMeshHeaderBinary(stl_dst, mesh));
}

int tmWriteMeshFooter(FILE *stl_dst, tm_mesh *mesh, tm_stl_mode mode) {
	if (mode == TM_STL_ASCII) {
		return tmWriteMeshFooterASCII(stl_dst, mesh);
	}
	// no footer in binary mode
	return 0;
}


int tmWriteFaceToSTLASCII(FILE *stl_dst, tm_face *face) {
	tm_triangle t = face->triangle;
	fprintf(stl_dst,
			"facet normal %f %f %f\n"
			"outer loop\n"
			"vertex %f %f %f\n"
			"vertex %f %f %f\n"
			"vertex %f %f %f\n"
			"endloop\n"
			"endfacet\n",
			t.n.x, t.n.y, t.n.z,
			t.a.x, t.a.y, t.a.z
			t.b.x, t.b.y, t.b.z
			t.c.x, t.c.y, t.c.z);
}

int tmWriteFaceToSTLBinary(FILE *stl_dst, tm_face *face) {
	unsigned short attributes = 0;
	
	
	// triangle struct is 12 floats in sequence needed for output!
	fwrite(&face->triangle, 4, 12, stl_dst);
	
	fwrite(&attributes, 2, 1, stl_dst);
}

int tmWriteFaceToSTL(FILE *stl_dst, tm_face *face, tm_stl_mode mode) {
	return (mode == TM_STL_ASCII
			? tmWriteFaceToSTLASCII(stl_dst, face)
			: tmWriteFaceToSTLBinary(stl_dst, face));
}

// stl_dst is assumed to be open and ready for writing
int tmWriteMeshToSTL(FILE *stl_dst, tm_mesh *mesh, tm_stl_mode mode) {
	tm_face *face;
	
	// sanity check stl_dst and mesh
	
	tmWriteMeshHeader(stl_dst, mesh, mode);
	
	face = mesh->first;
	while (face != NULL) {
		
		tmWriteFaceToSTL(stl_dst, face, mode);
		
		face = face->next;
	}
	
	tmWriteMeshFooter(stl_dst, mesh, mode);
	
	return 0;
}

int tmAddTriangleToMesh(tm_mesh *mesh, tm_triangle triangle) {

	// sanity check mesh
	
	tm_face *face;
	
	face = (tm_face *)malloc(sizeof(tm_face));
	face->triangle = triangle;
	face->next = NULL;
	
	if (mesh->last == NULL) {
		// this is the first face
		mesh->first = face;
		mesh->last = face;
	} else {
		mesh->last->next = face;
		mesh->last = face;
	}
	
	
	// perhaps useful to return pointer to new face or something? 
}

int tmReleaseMesh(tm_mesh *mesh) {

	tm_face *face, *nextface;
	
	// sanity check mesh
	
	face = mesh->first;
	
	while (face != NULL) {
		nextface = face->next;
		free(face);
		face = nextface;
	}
}
