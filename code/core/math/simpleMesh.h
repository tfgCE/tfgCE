#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

#include <functional>

template <typename Element> class ArrayStack;

namespace Collision
{
	struct CheckCollisionContext;
};

namespace Meshes
{
	class Mesh3D;
	struct Mesh3DPart;
	namespace Builders
	{
		class IPU;
	};
};

struct SimpleMesh
{
protected:
	struct Triangle;
	
public:
	SimpleMesh();
	SimpleMesh(SimpleMesh const & _other);
	~SimpleMesh();

	SimpleMesh const & operator=(SimpleMesh const & _other);

	void clear();

	bool is_empty() const { return triangles.is_empty(); }

	void make_space_for_triangles(int _count);
	void add(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c);
	void add_just_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c); // has to build afterwards
	void build(); // from triangles
	void copy_from(SimpleMesh const & _other);
	
	int get_triangle_count() const { return triangles.get_size(); }
	bool get_triangle(int _idx, OUT_ Vector3 & _a, OUT_ Vector3 & _b, OUT_ Vector3 & _c) const;

	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;

	void reverse_triangles();
	void apply_transform(Matrix44 const & _transform);

	Range3 calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const;

	void create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon = 0, int _polygonCount = NONE);
	void create_from(::Meshes::Mesh3D const * _mesh, OUT_ OPTIONAL_ int * _skinToBoneIdx = nullptr);
	void create_from(::Meshes::Mesh3DPart const * _meshPart, OUT_ OPTIONAL_ int * _skinToBoneIdx = nullptr);

	Vector3 get_centre() const { return range.centre(); }
	Range3 const & get_range() const { return range; }

	void split(Plane const& _plane, SimpleMesh & _behindIntoMesh, float _threshold = 0.0001f);
	void convexify(std::function<SimpleMesh* (int _idx)> get_mesh); // 0 for this, more for newly created

	float calculate_outside_box_distance(Vector3 const& _loc) const;

public:
	bool load_from_xml(IO::XML::Node const * _node);

protected:
	struct Triangle
	{
		LOCATION_ Vector3 a, b, c;
		CACHED_ Plane abc;
		CACHED_ Plane abIn, bcIn, caIn; // planes pointing inward, to check if point is inside triangle or edge should be considered - perpendicular to abc
		CACHED_ NORMAL_ Vector3 a2b, b2c, c2a; // normals from one point to another
		CACHED_ float a2bDist, b2cDist, c2aDist; // distances
		Triangle() {}
		Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c);
		void build();
		bool does_share_edge_with(Triangle const & _tri) const;
		bool has_same_edge_with(Triangle const & _tri) const;
	};

	Array<Triangle> triangles;
	CACHED_ Range3 range = Range3::empty;
	CACHED_ Array<Vector3> points; // unique points from triangles

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif
};
