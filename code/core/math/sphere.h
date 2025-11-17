#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

namespace Collision
{
	struct CheckCollisionContext;
};

struct Sphere
{
public:
	static Sphere const zero;

	Sphere();
	Sphere(Vector3 const & _location, float _radius) { setup(_location, _radius); }
	void setup(Vector3 const & _location, float _radius);

	float& access_element(uint _e) { an_assert(_e <= 3); return _e == 0 ? location.x : (_e == 1 ? location.y : (_e == 2? location.z : radius)); }

	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;

	void apply_transform(Matrix44 const & _transform);

	Range3 calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const;

	Vector3 const & get_centre() const { return location; }
	float get_radius() const { return radius; }

	float calc_intersection_t(Segment const & _segment) const;

	float calculate_outside_distance(Vector3 const& _loc) const;

	bool does_contain(Vector3 const& _point) const { return (location - _point).length_squared() <= sqr(radius); }

	bool does_overlap(Range3 const& _range) const;

	String to_string() const;

public:
	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_string(String const& _string);

protected:
	Vector3 location;
	float radius;

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif

	friend class CoreRegisteredTypes;
};

template <> bool Optional<Sphere>::load_from_xml(IO::XML::Node const* _node);
template <> bool Optional<Sphere>::load_from_xml(IO::XML::Attribute const* _attr);
