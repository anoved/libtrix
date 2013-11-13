#ifndef _LIBTRIX_H_
#define _LIBTRIX_H_

#define TRIX_MESH_NAME_MAX 80
#define TRIX_MESH_NAME_DEFAULT "libtrix"

typedef struct {
	float x, y, z;
} trix_vertex;

typedef struct {
	trix_vertex n, a, b, c;
} trix_triangle;

struct trix_face_node {
	trix_triangle triangle;
	struct trix_face_node *next;
};
typedef struct trix_face_node trix_face;

typedef struct {
	char name[TRIX_MESH_NAME_MAX];
	trix_face *first, *last;
	unsigned long facecount;
} trix_mesh;

// output stl mode
// binary is more compact; ascii is human readable
typedef enum {
	TRIX_STL_BINARY,
	TRIX_STL_ASCII
} trix_stl_mode;

// or also cw/ccw options
/* instead of different reset/recalculate functions,
 * one normal resetter that takes a mode switch:
typedef enum {
	TRIX_NORMAL_IMPLICIT,
	TRIX_NORMAL_EXPLICIT
} trix_normal_mode;
*/

// If name is NULL, default mesh name will be used.
trix_mesh *trixCreate(const char *name);

// output mesh to dst_path format indicated by mode. writes to stdout if dst_path is null. returns nonzero on error
int trixWrite(const char *dst_path, trix_mesh *mesh, trix_stl_mode mode);

// creates mesh read from stl_src; reads from stdin if src_path is null. returns null on error.
trix_mesh *trixRead(const char *src_path);

// free memory associated with mesh (disassembles face list)
void trixRelease(trix_mesh *mesh);

int trixResetNormals(trix_mesh *mesh);

int trixRecalculateNormals(trix_mesh *mesh);

// appends a trix_face containing triangle to the end of the mesh list
int trixAddTriangle(trix_mesh *mesh, trix_triangle triangle);

// mesh facecount is updated automatically by trixAddTriangle,
// but as a utility trixFacecount will double-check the count
unsigned long trixFacecount(trix_mesh *mesh);

// CSG operations

#endif
