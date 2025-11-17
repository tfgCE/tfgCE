#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

template <typename Element> class ArrayStack;

namespace Collision
{
	struct CheckCollisionContext;
}; 

struct ConvexHull
{
protected:
	struct Triangle;
	struct Edge;

public:
	ConvexHull();
	ConvexHull(ConvexHull const & _other);
	~ConvexHull();

	ConvexHull const & operator=(ConvexHull const & _other);

	bool is_empty() const { return points.is_empty(); }

	void clear();
	void clear_keep_points();

	void add(Vector3 const & _point);
	void build();
	void copy_from(ConvexHull const & _other);
	
	int get_vertex_count() const { return points.get_size(); }
	int get_triangle_count() const { return triangles.get_size(); }
	bool get_vertex(int _idx, OUT_ Vector3& _v)const;
	bool get_triangle(int _idx, OUT_ Vector3 & _a, OUT_ Vector3 & _b, OUT_ Vector3 & _c) const;

	// they may touch but can't contain each other's points inside
	bool does_intersect_with(ConvexHull const & _other, Transform const & _otherRelativePlacement) const;
	static bool does_intersect(ConvexHull const & _a, Transform const & _aPlacement, ConvexHull const & _b, Transform const & _bPlacement);

	bool does_contain_inside(Vector3 const & _relativeLocation, OUT_ OPTIONAL_ Vector3 * _normal = nullptr) const;

	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;

	Range3 const & get_range() const { return range; }
	float calculate_size_approximated() const;
	Vector3 const & get_centre() const { return centre; }

	void apply_transform(Matrix44 const & _transform);

	Range3 calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const;

#ifdef AN_DEVELOPMENT
	String debug_build_code() const;
	void debug_visualise(Optional<Vector3> _a = NP, Optional<Vector3> _b = NP, Optional<Vector3> _c = NP, Optional<Vector3> _o = NP) const;
#endif

public:
	bool load_from_xml(IO::XML::Node const * _node, bool _allowLessThanFour = false);

protected:
	struct Triangle
	{
		LOCATION_ Vector3 a, b, c;
		Plane abc;
		Plane abIn, bcIn, caIn; // planes pointing inward, to check if point is inside triangle or edge should be considered - perpendicular to abc
		// these are used by collisions
		CACHED_ NORMAL_ Vector3 a2b, b2c, c2a; // normals from one point to another
		CACHED_ float a2bDist, b2cDist, c2aDist; // distances
		Plane abToTriangle, bcToTriangle, caToTriangle; // separation plane
		Triangle const * abTriangle; // triangles beyond edges
		Triangle const * bcTriangle;
		Triangle const * caTriangle;
		Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c);
		Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, bool _justStore); // store values, we will calculate them using "centre"
		void prepare_using_centre(Vector3 const& _hullCentre);
		bool has_on_outside(Triangle const & _t) const;
		bool has_edge(Vector3 const & _a, Vector3 const & _b) const;
		Plane get_out_separation_plane(Vector3 const & _a, Vector3 const & _b, Plane const & _in, Triangle const * _out) const;
		bool check_point_inside(Vector3 const & _point, OUT_ OPTIONAL_ Vector3 * _normal = nullptr) const;
		int check_points_inside(REF_ Array<Vector3> const & _points, int * _pointsInside) const;
		bool check_edges_intersect(REF_ Array<Edge> const & _edges) const;
		bool switch_to_triangle(Triangle const *& _newTriangle, Triangle const *& _prevTriangle, Vector3 const & _point, float _increaseSize = 0.0f) const;
		static bool is_degenerate(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c);

		void prepare_cached();
	};
	struct Edge
	{
		NORMAL_ Plane inPlus;
		Vector3 a, b;
		Triangle const * trianglePlus;
		Triangle const * triangleMinus;
		Edge();
	};
	struct Bisect
	{
		NORMAL_ Plane inPlus; // plane sharing one edge of a triangle and going through centre of convex hull
		Triangle const * trianglePlus; // point at triangle in triangle array
		Triangle const * triangleMinus; // point at triangle in triangle array
		Bisect* plus;
		Bisect* minus;
		Bisect(Edge const & _edge);
		~Bisect();
		void add(Edge const & _edge);
		float calculate_perpendicular(Edge const & _edge) const;
		Triangle const * find_triangle(Vector3 const & _point) const;
	private:
		static void add_at(Bisect*& _bisect, Edge const & _edge);
		float calculate_perpendicular_at(Bisect const * _bisect, Edge const & _edge) const;
	};

#ifdef AN_DEVELOPMENT
	Array<Vector3> orgPoints; // pre build
#endif
	Array<Vector3> points;
	CACHED_ Range3 range = Range3::empty;
	CACHED_ Vector3 centre;
	CACHED_ Array<Triangle> triangles;
	CACHED_ Array<Edge> edges;
	CACHED_ Bisect* bisect;

	Triangle const * find_triangle(Vector3 const & _point, Triangle const * _startingTriangle) const;

	void add_triangle(int32 _a);
	bool add_triangle(int32 _a, int32 _b);
	bool has_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c) const;
	bool add_triangle_in_gap(int32 _a, int32 _b);
	bool intersects_coplanar_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c) const;
	int32 find_point_index(Vector3 const & _a) const;

	void link_triangles();

	void build_bisect();
	void add_edge(Vector3 const & _a, Vector3 const & _b, const Triangle * _inside);

	bool check_intersection_against_points(Transform const & _otherRelativePlacement, REF_ Array<Vector3> const & _points, OUT_ bool & _intersects) const; // returns true if result found
	bool check_intersection_against_edges(Transform const & _otherRelativePlacement, REF_ Array<Edge> const & _edges, OUT_ bool & _intersects) const; // returns true if result found

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif
};
