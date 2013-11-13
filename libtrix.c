#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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
	
	// for now, just writing mesh name to header.
	// should check that name doesn't begin with "solid"
	strncpy(header, mesh->name, 80);
	header[79] = '\0';
	
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
	
	if (fprintf(stl_dst, "solid %s\n", mesh->name) < 0) {
		return 1;
	}
	
	return 0;
}

static int trixWriteHeader(FILE *stl_dst, trix_mesh *mesh, trix_stl_mode mode) {
	return (mode == TRIX_STL_ASCII
			? trixWriteHeaderASCII(stl_dst, mesh)
			: trixWriteHeaderBinary(stl_dst, mesh));
}

static int trixWriteFooterASCII(FILE *stl_dst, trix_mesh *mesh) {
	
	if (mesh == NULL) {
		return 1;
	}
	
	if (fprintf(stl_dst, "endsolid %s\n", mesh->name) < 0) {
		return 1;
	}
	
	return 0;
}

static int trixWriteFooter(FILE *stl_dst, trix_mesh *mesh, trix_stl_mode mode) {
	if (mode == TRIX_STL_ASCII) {
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
	return (mode == TRIX_STL_ASCII
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
	
	mesh = trixCreate(NULL);
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
	
	mesh = trixCreate(NULL);
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
	
	// consider using trixRecalculateTriangleNormal here
	
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

trix_mesh *trixCreate(const char *name) {
	trix_mesh *mesh;
	
	mesh = (trix_mesh *)malloc(sizeof(trix_mesh));
	if (mesh == NULL) {
		return NULL;
	}
	
	if (name == NULL) {
		strncpy(mesh->name, TRIX_MESH_NAME_DEFAULT, TRIX_MESH_NAME_MAX);
	} else {
		strncpy(mesh->name, name, TRIX_MESH_NAME_MAX);
		mesh->name[TRIX_MESH_NAME_MAX - 1] = '\0';
	}
	
	mesh->first = NULL;
	mesh->last = NULL;
	mesh->facecount = 0;
	
	return mesh;
}

// returns pointer to the reset normal vector
static void trixResetTriangleNormal(trix_triangle *triangle) {
	
	if (triangle == NULL) {
		return;
	}
	
	triangle->n.x = 0.0;
	triangle->n.y = 0.0;
	triangle->n.z = 0.0;
}

int trixResetNormals(trix_mesh *mesh) {
	trix_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	face = mesh->first;
	while (face != NULL) {
		trixResetTriangleNormal(&face->triangle);
		face = face->next;
	}
	
	return 0;
}

// iteratemeshface function that takes callback function pointer to apply to each face/tri?
// so clear how appropriate OOP is for some tasks, like these geometry operations

static void vertexDifference(trix_vertex *a, trix_vertex *b, trix_vertex *result) {
	result->x = b->x - a->x;
	result->y = b->y - a->y;
	result->z = b->z - a->z;
}

static void vertexCrossProduct(trix_vertex *a, trix_vertex *b, trix_vertex *result) {
	result->x = a->y * b->z - a->z * b->y;
	result->y = a->z * b->x - a->x * b->z;
	result->z = a->x * b->y - a->y * b->x;
}

static float vertexMagnitude(trix_vertex *v) {
	return sqrtf((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}

static void trixRecalculateTriangleNormal(trix_triangle *triangle) {
	
	trix_vertex u, v, n;
	float mag;
	
	if (triangle == NULL) {
		return;
	}

	vertexDifference(&triangle->a, &triangle->b, &u);
	vertexDifference(&triangle->b, &triangle->c, &v);
	vertexCrossProduct(&u, &v, &n);
	
	mag = vertexMagnitude(&n);
	n.x /= mag;
	n.y /= mag;
	n.z /= mag;
	
	triangle->n.x = n.x;
	triangle->n.y = n.y;
	triangle->n.z = n.z;
}

int trixRecalculateNormals(trix_mesh *mesh) {
	trix_face *face;
	
	if (mesh == NULL) {
		return 1;
	}
	
	face = mesh->first;
	while (face != NULL) {
		trixRecalculateTriangleNormal(&face->triangle);
		face = face->next;
	}
	
	return 0;
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
