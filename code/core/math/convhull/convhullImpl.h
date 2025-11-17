#pragma once

#include "convhull_3d.h"

namespace convhullImpl
{
	void build(
		ch_vertex* const in_vertices, // preallocated memory
		const int nVert, // number of vertices
		/* output arguments */
		int* out_faces, // preallocated memory to output data, at least nVert * 3 */
		int* nOut_faces // number of faces
	);
};
