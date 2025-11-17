#include "math.h"

#include "earcut/earcut.h"

#include <array>
#include <vector>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

void PolygonUtils::triangulate(Array<Vector2> const& _polygon, OUT_ Array<Triangle>& _triangles)
{
	// taken from https://github.com/mapbox/earcut.hpp#usage

	// The number type to use for tessellation
	using Coord = float;

	// The index type. Defaults to uint32_t, but you can also pass uint16_t if you know that your
	// data won't have more than 65536 vertices.
	using N = uint32_t;

	// Create array
	using Point = std::array<Coord, 2>;
	std::vector<std::vector<Point>> polygonDef;

	std::vector<Point> polygon;

	for_every(v, _polygon)
	{
		polygon.push_back({ { v->x, v->y } });
	}

	polygonDef.push_back(polygon);

	std::vector<N> indices = mapbox::earcut<N>(polygonDef);

	_triangles.make_space_for_additional((uint)(indices.size() / 3));
	for (int i = 0; i <= (int)indices.size() - 3; i += 3)
	{
		Triangle t;
		// it seems that earcut unwinds them differently or doesn't care much, hence 0,2,1 order
		t.a = _polygon[indices[i + 0]];
		t.b = _polygon[indices[i + 2]];
		t.c = _polygon[indices[i + 1]];
		// check if clockwise, force clockwise
		if (Vector2::dot((t.b - t.a).rotated_right(), t.c - t.a) < 0.0f)
		{
			swap(t.b, t.c);
		}
		_triangles.push_back(t);
	}
}
