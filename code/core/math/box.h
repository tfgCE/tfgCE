#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

struct Colour;

namespace Collision
{
	struct CheckCollisionContext;
};

namespace Meshes
{
	class Mesh3D;
	namespace Builders
	{
		class IPU;
	};
};

/**
 *	Load as:
 *		1. set of 4 corners
 *		2. set of 3 corners + thickness
 *			will centre on third axis (axes are calculated using corners)
 */
struct Box
{
public:
	Box();
	Box(Vector3 const & _centre, Vector3 const & _halfSize, Vector3 const & _xAxis, Vector3 const & _yAxis) { setup(_centre, _halfSize, _xAxis, _yAxis); }
	explicit Box(Range3 const & _range) { setup(_range); }
	void setup(Vector3 const & _centre, Vector3 const & _halfSize, Vector3 const & _xAxis, Vector3 const & _yAxis);
	void setup(Vector3 const & _centre, Vector3 const & _halfSize, Vector3 const & _xAxis, Vector3 const & _yAxis, Vector3 const & _zAxis);
	void setup(Range3 const & _range);
	void setup(Vector3 const & _refLoc, Range3 const & _rangeLocal, Vector3 const & _xAxis, Vector3 const & _yAxis, Vector3 const & _zAxis);
	void setup_allow_zero(Range3 const & _range);

	void set_half_size(Vector3 const & _halfSize) { halfSize = _halfSize; }
	
	Vector3 const & get_centre() const { return centre; }
	Vector3 const & get_half_size() const { return halfSize; }
	Vector3 const & get_x_axis() const { return xAxis; }
	Vector3 const & get_y_axis() const { return yAxis; }
	Vector3 const & get_z_axis() const { return zAxis; }
	Vector3 const& get_axis(Axis::Type _axis) const { an_assert(_axis >= Axis::X && _axis <= Axis::Y);  return _axis == Axis::X ? xAxis : (_axis == Axis::Y ? yAxis : zAxis); }
	
	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;
	bool check_segment(Segment & _segment) const;
	bool check_segment_detect_only(Segment const & _segment) const; // will just inform if we hit

	void create_from(::Meshes::Builders::IPU const & _ipu, int _startingAtPolygon = 0, int _polygonCount = NONE);

	void apply_transform(Matrix44 const & _transform);
	void apply_transform(Transform const & _transform);
	Range3 calculate_bounding_box(Transform const & _usingPlacement, bool _quickNotUsed) const;

	Vector3 get_point_at_pt(Vector3 const& _pt) const;

	Transform get_placement() const;

	Vector3 get_centre_offset_on_surface(Vector3 const& _localOffset) const;

	Vector3 location_to_world(Vector3 const & _loc) const;
	Vector3 vector_to_world(Vector3 const & _loc) const;
	Vector3 location_to_local(Vector3 const & _loc) const;
	Vector3 vector_to_local(Vector3 const & _loc) const;

	bool is_inside(Vector3 const& _world) const { return is_inside_local(location_to_local(_world)); }
	bool is_inside_local(Vector3 const & _local) const { return _local.x >= -halfSize.x && _local.x <= halfSize.x  &&  _local.y >= -halfSize.y && _local.y <= halfSize.y  &&  _local.z >= -halfSize.z && _local.z <= halfSize.z; }
	float calculate_outside_distance_local(Vector3 const & _local, OUT_ Vector3 * _dir = nullptr) const;

	bool does_overlap(Range3 const& _range) const;
	
	bool does_overlap(Box const& _box) const;

public:
	bool load_from_xml(IO::XML::Node const * _node);

protected:
	Vector3 centre;
	Vector3 halfSize; // halfSize of each axis
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;

	Vector3 points[8];

	static void query_distance_axis(Vector3 const & _diff, Vector3 const & _axis, float const _halfSize, OUT_ float & _offsetOutsideBox);

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif

};
