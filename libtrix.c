#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "libtrix.h"

trix_result trixApply(trix_mesh *mesh, trix_function func) {
	trix_face *face, *next;
	trix_result rr;
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	face = mesh->first;
	while (face != NULL) {
		next = face->next;
		
		if ((rr = (func)(face)) != TRIX_OK) {
			return rr;
		}
		
		face = next;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteHeaderBinary(FILE *stl_dst, const trix_mesh *mesh) {
	char header[80];
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	// for now, just writing mesh name to header.
	// should check that name doesn't begin with "solid"
	strncpy(header, mesh->name, 80);
	header[79] = '\0';
	
	if (fwrite(header, 80, 1, stl_dst) != 1) {
		return TRIX_ERR_FILE;
	}
	
	if (fwrite(&mesh->facecount, 4, 1, stl_dst) != 1) {
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteHeaderASCII(FILE *stl_dst, const trix_mesh *mesh) {
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	if (fprintf(stl_dst, "solid %s\n", mesh->name) < 0) {
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteHeader(FILE *stl_dst, const trix_mesh *mesh, trix_stl_mode mode) {
	return (mode == TRIX_STL_ASCII
			? trixWriteHeaderASCII(stl_dst, mesh)
			: trixWriteHeaderBinary(stl_dst, mesh));
}

static trix_result trixWriteFooterASCII(FILE *stl_dst, const trix_mesh *mesh) {
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	if (fprintf(stl_dst, "endsolid %s\n", mesh->name) < 0) {
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteFooter(FILE *stl_dst, const trix_mesh *mesh, trix_stl_mode mode) {
	if (mode == TRIX_STL_ASCII) {
		return trixWriteFooterASCII(stl_dst, mesh);
	}
	// no footer in binary mode
	return TRIX_OK;
}

static trix_result trixWriteFaceASCII(FILE *stl_dst, trix_face *face) {
	
	if (face == NULL) {
		return TRIX_ERR_ARG;
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
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteFaceBinary(FILE *stl_dst, trix_face *face) {
	unsigned short attributes = 0;
	
	// triangle struct is 12 floats in sequence needed for output!
	if (fwrite(&face->triangle, 4, 12, stl_dst) != 12) {
		return TRIX_ERR_FILE;
	}
	
	if (fwrite(&attributes, 2, 1, stl_dst) != 1) {
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixWriteFace(FILE *stl_dst, trix_face *face, trix_stl_mode mode) {
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
trix_result trixWrite(const char *dst_path, const trix_mesh *mesh, trix_stl_mode mode) {
	trix_face *face;
	FILE *stl_dst;
	trix_result rr;
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	if (dst_path == NULL) {
		stl_dst = stdout;
	} else if ((stl_dst = fopen(dst_path, "w")) == NULL) {
		return TRIX_ERR_FILE;
	}
	
	if ((rr = trixWriteHeader(stl_dst, mesh, mode)) != TRIX_OK) {
		trixCloseOutput(stl_dst);
		return rr;
	}
	
	face = mesh->first;
	while (face != NULL) {
		
		if ((rr = trixWriteFace(stl_dst, face, mode)) != TRIX_OK) {
			trixCloseOutput(stl_dst);
			return rr;
		}
		
		face = face->next;
	}
	
	if ((rr = trixWriteFooter(stl_dst, mesh, mode)) != TRIX_OK) {
		trixCloseOutput(stl_dst);
		return rr;
	}
	
	trixCloseOutput(stl_dst);
	return TRIX_OK;
}

static trix_result trixReadBinary(FILE *stl_src, trix_mesh **dst_mesh) {
	
	uint32_t facecount, f;
	uint16_t attribute;
	
	trix_triangle triangle;
	trix_mesh *mesh;
	trix_result rr;
	
	// fread instead to get header
	if (fseek(stl_src, 80, SEEK_SET) != 0) {
		return TRIX_ERR_FILE;
	}
	
	if (fread(&facecount, 4, 1, stl_src) != 1) {
		return TRIX_ERR_FILE;
	}
	
	if ((rr = trixCreate(NULL, &mesh)) != TRIX_OK) {
		return rr;
	}
	
	// consider succeeding as long as we can read a whole number of faces,
	// even if that number read does not match facecount (only optionally treat it as an error?) 
	
	for (f = 0; f < facecount; f++) {
		
		if (fread(&triangle, 4, 12, stl_src) != 12) {
			(void)trixRelease(mesh);
			return TRIX_ERR_FILE;
		}
		
		if (fread(&attribute, 2, 1, stl_src) != 1) {
			(void)trixRelease(mesh);
			return TRIX_ERR_FILE;
		}
		
		if (trixAddTriangle(mesh, &triangle)) {
			(void)trixRelease(mesh);
			return TRIX_ERR_FILE;
		}
	}
	
	*dst_mesh = mesh;
	return TRIX_OK;
}

static trix_result trixReadTriangleASCII(FILE *stl_src, trix_triangle *triangle) {
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
		return TRIX_ERR_FILE;
	}
	
	return TRIX_OK;
}

static trix_result trixReadASCII(FILE *stl_src, trix_mesh **dst_mesh) {
	trix_mesh *mesh;
	trix_triangle triangle;
	trix_result rr;
	fpos_t p;
	
	if ((rr = trixCreate(NULL, &mesh)) != TRIX_OK) {
		return rr;
	}
	
	// expect but discard a solid name
	fscanf(stl_src, "solid %*100s\n");
	fgetpos(stl_src, &p);
	
	while (trixReadTriangleASCII(stl_src, &triangle) == TRIX_OK) {
		fgetpos(stl_src, &p);
		if ((rr = trixAddTriangle(mesh, &triangle)) != TRIX_OK) {
			(void)trixRelease(mesh);
			return rr;
		}
	}
	
	// read the footer; a bit pedantic since data has already been read
	fsetpos(stl_src, &p);
	if (fscanf(stl_src, "endsolid %*100s\n") == EOF) {
		printf("failed to read footer\n");
	}
	
	*dst_mesh = mesh;
	return TRIX_OK;
}

trix_result trixRead(const char *src_path, trix_mesh **dst_mesh) {
	trix_mesh *mesh;
	FILE *stl_src;
	char header[5];
	trix_result rr;
	
	if (src_path == NULL) {
		stl_src = stdin;
	} else if ((stl_src = fopen(src_path, "r")) == NULL) {
		return TRIX_ERR_FILE;
	}
	
	if (fread(header, 1, 5, stl_src) != 5) {
		trixCloseInput(stl_src);
		return TRIX_ERR_FILE;
	}
	
	rewind(stl_src);
	
	if (strncmp(header, "solid", 5) == 0) {
		rr = trixReadASCII(stl_src, &mesh);
	} else {
		rr = trixReadBinary(stl_src, &mesh);
	}
	
	if (rr != TRIX_OK) {
		trixCloseInput(stl_src);
		return rr;
	}
	
	trixCloseInput(stl_src);
	*dst_mesh = mesh;
	return TRIX_OK;
}

trix_result trixAddTriangle(trix_mesh *mesh, const trix_triangle *triangle) {
	trix_face *face;
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	if ((face = (trix_face *)malloc(sizeof(trix_face))) == NULL) {
		return TRIX_ERR_MEM;
	}
	
	face->triangle = *triangle;
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
	
	// check for maximum facecount here?
	// may only apply to binary output
	mesh->facecount += 1;
	return TRIX_OK;
}

trix_result trixCreate(const char *name, trix_mesh **dst_mesh) {
	trix_mesh *mesh;
	
	if ((mesh = (trix_mesh *)malloc(sizeof(trix_mesh))) == NULL) {
		return TRIX_ERR;
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
	
	*dst_mesh = mesh;
	return TRIX_OK;
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

trix_result trixResetNormals(trix_mesh *mesh) {
	trix_face *face;
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	face = mesh->first;
	while (face != NULL) {
		trixResetTriangleNormal(&face->triangle);
		face = face->next;
	}
	
	return TRIX_OK;
}

// sets result to difference of vectors a and b (b - a)
static void vector_difference(const trix_vertex *a, const trix_vertex *b, trix_vertex *result) {
	result->x = b->x - a->x;
	result->y = b->y - a->y;
	result->z = b->z - a->z;
}

// sets result to cross product of vectors a and b (a x b)
// https://en.wikipedia.org/wiki/Cross_product#Mnemonic
static void vector_crossproduct(const trix_vertex *a, const trix_vertex *b, trix_vertex *result) {
	result->x = a->y * b->z - a->z * b->y;
	result->y = a->z * b->x - a->x * b->z;
	result->z = a->x * b->y - a->y * b->x;
}

// sets result to unit vector codirectional with vector v (v / ||v||)
static void vector_unitvector(const trix_vertex *v, trix_vertex *result) {
	float mag = sqrtf((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
	result->x = v->x / mag;
	result->y = v->y / mag;
	result->z = v->z / mag;
}

static trix_result trixRecalculateFaceNormal(trix_face *face) {
	trix_vertex u, v, cp, n;
	
	if (face == NULL) {
		return TRIX_ERR_ARG;
	}
	
	// vectors u and v are triangle sides ab and bc
	vector_difference(&face->triangle.a, &face->triangle.b, &u);
	vector_difference(&face->triangle.b, &face->triangle.c, &v);
	
	// the cross product of two vectors is perpendicular to both
	// since vectors u and v both lie in the plane of triangle abc,
	// the cross product is perpendicular to the triangle's surface
	vector_crossproduct(&u, &v, &cp);
	
	// normalize the cross product to unit length to get surface normal n
	vector_unitvector(&cp, &n);
	
	face->triangle.n.x = n.x;
	face->triangle.n.y = n.y;
	face->triangle.n.z = n.z;
	return TRIX_OK;
}

trix_result trixRecalculateNormals(trix_mesh *mesh) {
	return trixApply(mesh, (trix_function)trixRecalculateFaceNormal);
}

trix_result trixRelease(trix_mesh *mesh) {

	trix_face *face, *nextface;
	
	if (mesh == NULL) {
		return TRIX_ERR_ARG;
	}
	
	face = mesh->first;
	
	while (face != NULL) {
		nextface = face->next;
		free(face);
		face = nextface;
	}
	
	free(mesh);
	mesh = NULL;
	return TRIX_OK;
}
