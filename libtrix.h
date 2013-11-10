#ifndef _LIBTRIX_H_
#define _LIBTRIX_H_

// rudimentary triangle mesh library
// basic features shared between stl generating toys

// have some simple error logging/lookup system
// if functions return an error, they just stash
// the message in a variable which caller can query?
// unless standard error functions cover that use

#include <stdio.h>

typedef struct {
	float x, y, z;
} tm_vertex;

typedef struct {
	tm_vertex a, b, c, n;
} tm_triangle;

struct tm_face_node {
	tm_triangle triangle;
	struct tm_face *next;
};
typedef struct tm_face_node tm_face;

typedef struct {
	// name, to be used for ascii solid name
	tm_face *first, *last;
	// facecount should be maintained as mesh is assembled,
	// but can be recalculated by traversing list
	unsigned long facecount;
} tm_mesh;

// output stl mode
// binary is more compact; ascii is human readable
typedef enum {
	TM_STL_BINARY,
	TM_STL_ASCII
} tm_stl_mode;

// output mesh to stl_dst in format indicated by mode; returns nonzero on error
int tmWriteMeshToSTL(FILE *stl_dst, tm_mesh *mesh, tm_stl_mode mode);

// populate mesh face list with triangles read from stl-src
int tmReadMeshFromSTL(FILE *stl_src, tm_mesh *mesh);

// free memory associated with mesh (disassembles face list)
int tmReleaseMesh(tm_mesh *mesh);

// updates triangle's n vector as unit normal of vertices a, b, c
int tmComputeTriangleNormal(tm_triangle *triangle);

// appends a tm_face containing triangle to the end of the mesh list
int tmAddTriangleToMesh(tm_mesh *mesh, tm_triangle triangle);


// CSG operations

#endif
