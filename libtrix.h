#ifndef _LIBTRIX_H_
#define _LIBTRIX_H_

// rudimentary triangle mesh library
// basic features shared between stl generating toys

// have some simple error logging/lookup system
// if functions return an error, they just stash
// the message in a variable which caller can query?
// unless standard error functions cover that use

typedef struct {
	float x, y, z;
} trix_vertex;

typedef struct {
	trix_vertex a, b, c, n;
} trix_triangle;

struct trix_face_node {
	trix_triangle triangle;
	struct trix_face_node *next;
};
typedef struct trix_face_node trix_face;

typedef struct {
	// name, to be used for ascii solid name
	trix_face *first, *last;
	// facecount should be maintained as mesh is assembled,
	// but can be recalculated by traversing list
	unsigned long facecount;
} trix_mesh;

// output stl mode
// binary is more compact; ascii is human readable
typedef enum {
	TM_STL_BINARY,
	TM_STL_ASCII
} trix_stl_mode;

// creates empty new mesh
trix_mesh *trixCreate(void);

// output mesh to stl_dst in format indicated by mode; returns nonzero on error
int trixWrite(const char *dst_path, trix_mesh *mesh, trix_stl_mode mode);

// creates mesh read from stl_src
trix_mesh *trixRead(const char *src_path);

// free memory associated with mesh (disassembles face list)
void trixRelease(trix_mesh *mesh);

// updates triangle's n vector as unit normal of vertices a, b, c
//int tmComputeTriangleNormal(trix_triangle *triangle);

// appends a trix_face containing triangle to the end of the mesh list
int trixAddTriangle(trix_mesh *mesh, trix_triangle triangle);

// mesh facecount is updated automatically by trixAddTriangle,
// but as a utility trixFacecount will double-check the count
unsigned long trixFacecount(trix_mesh *mesh);


// CSG operations

#endif
