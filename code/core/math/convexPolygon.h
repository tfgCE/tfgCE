#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

template <typename Element> class ArrayStack;

struct ConvexPolygon
{
public:
	struct Edge
	{
		// clockwise
		Vector2 a, b;

		CACHED_ float length;
		CACHED_ Vector2 dir;
		CACHED_ Vector2 in;

		Vector2 get_at_pt(float _pt) const;
		float calculate_distance(Vector2 const& _p) const;
		Vector2 drop_to_edge(Vector2 const & _p) const;
	};

public:
	ConvexPolygon();
	ConvexPolygon(ConvexPolygon const & _other);
	~ConvexPolygon();

	ConvexPolygon const & operator=(ConvexPolygon const & _other);

	bool is_empty() const { return points.is_empty(); }

	Vector2 const & get_centre() const { return centre; }

	void clear();
	void clear_keep_points();

	bool is_inside(Vector2 const& _point) const;
	bool does_share_edge_with_triangle(Vector2 const& _a, Vector2 const& _b, Vector2 const& _c, OPTIONAL_ OUT_ Vector2* _other = nullptr) const;

	bool can_add_to_remain_convex(Vector2 const& _point) const;

	void add(Vector2 const & _point);
	void pop_point();
	void build();
	void copy_from(ConvexPolygon const & _other);
	void copy_from_dont_build(ConvexPolygon const & _other);
	
	int get_vertex_count() const { return points.get_size(); }
	int get_edge_count() const { return edges.get_size(); }
	Edge const * get_edge(int _idx) const { return edges.is_index_valid(_idx) ? &edges[_idx] : nullptr; }

	Range2 const & get_range() const { return range; }

	Vector2 move_to_edge(Vector2 const & _p) const;
	Optional<Vector2> move_inside(Vector2 const & _p, float _radius) const; // not set means that can't fit
	
	Edge const * get_closest_edge(Vector2 const & _p, ConvexPolygon::Edge const ** _secondDoClosestEdge = nullptr) const;

	bool check_collides(Vector2 const & _start, Vector2 const & _end) const;

protected:
	Array<Vector2> points;
	CACHED_ Vector2 centre;
	CACHED_ Range2 range = Range2::empty;
	CACHED_ Array<Edge> edges;

	bool internal_build(bool _mayFail); // returns true if built
};
