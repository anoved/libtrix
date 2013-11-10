#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libtrix.h"

static unsigned long trixFacecount(trix_mesh *mesh) {
	unsigned long count;
	trix_face *face;
	
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

static int trixWriteHeaderBinary(FILE *stl_dst, trix_mesh *mesh) {
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

static int trixWriteHeaderASCII(FILE *stl_dst, trix_mesh *mesh) {
	
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

static int trixWriteHeader(FILE *stl_dst, trix_mesh *mesh, trix_stl_mode mode) {
	return (mode == TM_STL_ASCII
			? trixWriteHeaderASCII(stl_dst, mesh)
			: trixWriteHeaderBinary(stl_dst, mesh));
}

static int trixWriteFooterASCII(FILE *stl_dst, trix_mesh *mesh) {
	
	if (mesh == NULL) {
		return 1;
	}
	
	if (fprintf(stl_dst, "endsolid MESH\n") < 0) {
		return 1;
	}
	
	return 0;
}

static int trixWriteFooter(FILE *stl_dst, trix_mesh *mesh, trix_stl_mode mode) {
	if (mode == TM_STL_ASCII) {
		return trixWriteFooterASCII(stl_dst, mesh);
	}
	// no footer in binary mode
	return 0;
}

static int trixWriteFaceASCII(FILE *stl_dst, trix_face *face) {
	
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
			face->triangle.a.x, face->triangle.a.y, face->triangle.a.z,
			face->triangle.b.x, face->triangle.b.y, face->triangle.b.z,
			face->triangle.c.x, face->triangle.c.y, face->triangle.c.z) < 0) {
		return 1;
	}
	
	return 0;
}

static int trixWriteFaceBinary(FILE *stl_dst, trix_face *face) {
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

static int trixWriteFace(FILE *stl_dst, trix_face *face, trix_stl_mode mode) {
	return (mode == TM_STL_ASCII
			? trixWriteFaceASCII(stl_dst, face)
			: trixWriteFaceBinary(stl_dst, face));
}

// stl_dst is assumed to be open and ready for writing
int trixWrite(FILE *stl_dst, trix_mesh *mesh, trix_stl_mode mode) {
	trix_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	if (trixWriteHeader(stl_dst, mesh, mode) != 0) {
		return 1;
	}
	
	face = mesh->first;
	while (face != NULL) {
		
		if (trixWriteFace(stl_dst, face, mode) != 0) {
			return 1;
		}
		
		face = face->next;
	}
	
	if (trixWriteFooter(stl_dst, mesh, mode) != 0) {
		return 1;
	}
	
	return 0;
}

int trixAddTriangle(trix_mesh *mesh, trix_triangle triangle) {
	trix_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	face = (trix_face *)malloc(sizeof(trix_face));
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
	
	mesh->facecount += 1;
	
	return 0;
}

trix_mesh *trixCreate(void) {
	trix_mesh *mesh;
	
	mesh = (trix_mesh *)malloc(sizeof(trix_mesh));
	if (mesh == NULL) {
		return NULL;
	}
	
	mesh->first = NULL;
	mesh->last = NULL;
	mesh->facecount = 0;
	
	return mesh;
}

void trixRelease(trix_mesh *mesh) {

	trix_face *face, *nextface;
	
	if (mesh == NULL) {
		return;
	}
	
	face = mesh->first;
	
	while (face != NULL) {
		nextface = face->next;
		free(face);
		face = nextface;
	}
	
	free(mesh);
	mesh = NULL;
}
