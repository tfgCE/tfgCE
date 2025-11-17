#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

namespace PolygonUtils
{
	struct Triangle
	{
		Vector2 a, b, c;
	};

	void triangulate(Array<Vector2> const& _polygon, OUT_ Array<Triangle>& _triangles);
};
