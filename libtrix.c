
#include "libtrix.h"

unsigned long tmMeshFacecount(tm_mesh *mesh) {
	unsigned long count;
	tm_face *face;
	
	if (mesh == NULL) {
		return 0;
	}
	
	count = 0;
	face = mesh->first;
	while (face != NULL) {
		count += 1;
		face = face->next;
	}
	
	return count;
}

int tmWriteMeshHeaderBinary(FILE *stl_dst, tm_mesh *mesh) {
	char header[80];
	
	if (mesh == NULL) {
		return 1;
	}
	
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
	
	if (mesh == NULL) {
		return 1;
	}
	
	// what are the length and content constraints on name?
	// not specified in http://www.ennex.com/~fabbers/StL.asp
	// so be conservative (or look for conventions elsewhere)
	// (consider using mesh->name instead of fixed name)
	
	if (fprintf(stl_dst, "solid MESH\n") < 0) {
		return 1;
	}
	
	return 0;
}

int tmWriteMeshFooterASCII(FILE *stl_dst, tm_mesh *mesh) {
	
	if (fprintf(stl_dst, "endsolid MESH\n") < 0) {
		return 1;
	}
	
	return 0;
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
	
	if (face == NULL) {
		return 1;
	}
	
	if (fprintf(stl_dst,
			"facet normal %f %f %f\n"
			"outer loop\n"
			"vertex %f %f %f\n"
			"vertex %f %f %f\n"
			"vertex %f %f %f\n"
			"endloop\n"
			"endfacet\n",
			face->triangle.n.x, face->triangle.n.y, face->triangle.n.z,
			face->triangle.a.x, face->triangle.a.y, face->triangle.a.z
			face->triangle.b.x, face->triangle.b.y, face->triangle.b.z
			face->triangle.c.x, face->triangle.c.y, face->triangle.c.z) < 0) {
		return 1;
	}
	
	return 0;
}

int tmWriteFaceToSTLBinary(FILE *stl_dst, tm_face *face) {
	unsigned short attributes = 0;
	
	// triangle struct is 12 floats in sequence needed for output!
	if (fwrite(&face->triangle, 4, 12, stl_dst) != 12) {
		return 1;
	}
	
	if (fwrite(&attributes, 2, 1, stl_dst) != 1) {
		return 1;
	}
	
	return 0;
}

int tmWriteFaceToSTL(FILE *stl_dst, tm_face *face, tm_stl_mode mode) {
	return (mode == TM_STL_ASCII
			? tmWriteFaceToSTLASCII(stl_dst, face)
			: tmWriteFaceToSTLBinary(stl_dst, face));
}

// stl_dst is assumed to be open and ready for writing
int tmWriteMeshToSTL(FILE *stl_dst, tm_mesh *mesh, tm_stl_mode mode) {
	tm_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	if (tmWriteMeshHeader(stl_dst, mesh, mode)) {
		return 1;
	}
	
	face = mesh->first;
	while (face != NULL) {
		
		if (tmWriteFaceToSTL(stl_dst, face, mode)) {
			return 1;
		}
		
		face = face->next;
	}
	
	if (tmWriteMeshFooter(stl_dst, mesh, mode)) {
		return 1;
	}
	
	return 0;
}

int tmAddTriangleToMesh(tm_mesh *mesh, tm_triangle triangle) {
	tm_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	face = (tm_face *)malloc(sizeof(tm_face));
	if (face == NULL) { 
		return 1;
	}
	
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
	
	return 0;
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
