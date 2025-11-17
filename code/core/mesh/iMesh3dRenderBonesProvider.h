#pragma once

#include "..\math\math.h"

namespace Meshes
{
	interface_class IMesh3DRenderBonesProvider
	{
	public:
		/** this should return render bones that take vertex from MS and end up in transformed bones location in MS */
		virtual Matrix44 const * get_render_bone_matrices(OUT_ int & _matricesCount) const = 0;
	};

};
