#ifndef _LIBTRIX_H_
#define _LIBTRIX_H_

#define TRIX_MESH_NAME_MAX 80
#define TRIX_MESH_NAME_DEFAULT "libtrix"

typedef enum {
	TRIX_OK,
	TRIX_ERR,
	TRIX_ERR_ARG,
	TRIX_ERR_FILE,
	TRIX_ERR_MEM
} trix_result;

typedef enum {
	TRIX_STL_BINARY,
	TRIX_STL_ASCII
} trix_stl_mode;

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

// If name is NULL, default mesh name will be used.
trix_result trixCreate(const char *name, trix_mesh **dst_mesh);

// output mesh to dst_path format indicated by mode. writes to stdout if dst_path is null. returns nonzero on error
trix_result trixWrite(const char *dst_path, const trix_mesh *mesh, trix_stl_mode mode);

// creates mesh read from stl_src; reads from stdin if src_path is null. returns null on error.
trix_result trixRead(const char *src_path, trix_mesh **dst_mesh);

// free memory associated with mesh (disassembles face list)
trix_result trixRelease(trix_mesh *mesh);

trix_result trixResetNormals(trix_mesh *mesh);

trix_result trixRecalculateNormals(trix_mesh *mesh);

// appends a trix_face containing triangle to the end of the mesh list
trix_result trixAddTriangle(trix_mesh *mesh, trix_triangle triangle);

// mesh facecount is updated automatically by trixAddTriangle,
// but as a utility trixFacecount will double-check the count
trix_result trixFacecount(trix_mesh *mesh, unsigned long *dst_count);

#endif
