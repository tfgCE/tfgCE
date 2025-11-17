#include "convhullImpl.h"

#define CONVHULL_3D_ENABLE

#include "convhull_3d.h"

//

using namespace convhullImpl;

//

void build(
	ch_vertex* const in_vertices,
	const int nVert,
	/* output arguments */
	int* out_faces,
	int* nOut_faces)
{
	int* faceIndices = NULL;
	int nFaces;

	convhull_3d_build(in_vertices, nVert, &faceIndices, &nFaces);

	memcpy(out_faces, faceIndices, sizeof(int) * 3 * nFaces);
	
	free(faceIndices);
}
