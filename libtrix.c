#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libtrix.h"

unsigned long trixFacecount(trix_mesh *mesh) {
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

static void trixCloseOutput(FILE *dst) {
	if (dst != NULL && dst != stdout) {
		fclose(dst);
	}
}

static void trixCloseInput(FILE *src) {
	if (src != NULL && src != stdin) {
		fclose(src);
	}
}

// stl_dst is assumed to be open and ready for writing
int trixWrite(const char *dst_path, trix_mesh *mesh, trix_stl_mode mode) {
	trix_face *face;
	FILE *stl_dst;
	
	if (mesh == NULL) {
		return 1;
	}
	
	if (dst_path == NULL) {
		stl_dst = stdout;
	} else if ((stl_dst = fopen(dst_path, "w")) == NULL) {
		// failed to open dst
		return 1;
	}
	
	if (trixWriteHeader(stl_dst, mesh, mode) != 0) {
		trixCloseOutput(stl_dst);
		return 1;
	}
	
	face = mesh->first;
	while (face != NULL) {
		
		if (trixWriteFace(stl_dst, face, mode) != 0) {
			trixCloseOutput(stl_dst);
			return 1;
		}
		
		face = face->next;
	}
	
	if (trixWriteFooter(stl_dst, mesh, mode) != 0) {
		trixCloseOutput(stl_dst);
		return 1;
	}
	
	trixCloseOutput(stl_dst);
	return 0;
}

static trix_mesh *trixReadBinary(FILE *stl_src) {
	
	unsigned long facecount, f;
	trix_triangle triangle;
	unsigned short attribute;
	trix_mesh *mesh;
	
	// fread instead to get header
	if (fseek(stl_src, 80, SEEK_SET) != 0) {
		// seek failure
		return NULL;
	}
	
	if (fread(&facecount, 4, 1, stl_src) != 1) {
		// facecount read failure
		return NULL;
	}
	
	mesh = trixCreate();
	if (mesh == NULL) {
		return NULL;
	}
	
	// consider succeeding as long as we can read a whole number of faces,
	// even if that number read does not match facecount (only optionally treat it as an error?) 
	
	for (f = 0; f < facecount; f++) {
		
		if (fread(&triangle, 4, 12, stl_src) != 12) {
			trixRelease(mesh);
			return NULL;
		}
		
		if (fread(&attribute, 2, 1, stl_src) != 1) {
			trixRelease(mesh);
			return NULL;
		}
		
		if (trixAddTriangle(mesh, triangle)) {
			trixRelease(mesh);
			return NULL;
		}
	}
	
	return mesh;
}

static int trixReadTriangleASCII(FILE *stl_src, trix_triangle *triangle) {
	// fragile!
	if (fscanf(stl_src,
			"facet normal %20f %20f %20f\n"
			"outer loop\n"
			"vertex %20f %20f %20f\n"
			"vertex %20f %20f %20f\n"
			"vertex %20f %20f %20f\n"
			"endloop\n"
			"endfacet\n",
			&triangle->n.x, &triangle->n.y, &triangle->n.z,
			&triangle->a.x, &triangle->a.y, &triangle->a.z,
			&triangle->b.x, &triangle->b.y, &triangle->b.z,
			&triangle->c.x, &triangle->c.y, &triangle->c.z) != 12) {
		return 1;
	}
	
	return 0;
}

static trix_mesh *trixReadASCII(FILE *stl_src) {
	trix_mesh *mesh;
	trix_triangle triangle;
	fpos_t p;
	
	mesh = trixCreate();
	if (mesh == NULL) {
		return NULL;
	}
	
	// expect but discard a solid name
	fscanf(stl_src, "solid %*100s\n");
	fgetpos(stl_src, &p);
	
	while (trixReadTriangleASCII(stl_src, &triangle) == 0) {
		fgetpos(stl_src, &p);
		if (trixAddTriangle(mesh, triangle)) {
			trixRelease(mesh);
			return NULL;
		}
	}
	
	// read the footer; a bit pedantic since data has already been read
	fsetpos(stl_src, &p);
	if (fscanf(stl_src, "endsolid %*100s\n") == EOF) {
		printf("failed to read footer\n");
	}
	
	return mesh;
}

trix_mesh *trixRead(const char *src_path) {
	trix_mesh *mesh;
	FILE *stl_src;
	char header[5];
	
	if (src_path == NULL) {
		stl_src = stdin;
	} else if ((stl_src = fopen(src_path, "r")) == NULL) {
		return NULL;
	}
	
	if (fread(header, 1, 5, stl_src) != 5) {
		trixCloseInput(stl_src);
		return NULL;
	}
	
	rewind(stl_src);
	
	if (strncmp(header, "solid", 5) == 0) {
		mesh = trixReadASCII(stl_src);
	} else {
		mesh = trixReadBinary(stl_src);
	}
	
	trixCloseInput(stl_src);
	return mesh;
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
