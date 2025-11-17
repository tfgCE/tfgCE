#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

namespace Collision
{
	struct CheckCollisionContext;
};

struct Capsule
{
public:
	Capsule();
	Capsule(Vector3 const & _locationA, Vector3 const & _locationB, float _radius) { setup(_locationA, _locationB, _radius); }

	void setup(Vector3 const & _locationA, Vector3 const & _locationB, float _radius);

	void debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const;
	void log(LogInfoContext & _context) const;

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;

	void apply_transform(Matrix44 const & _transform);

	Range3 calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const;

	Vector3 get_centre() const { return (locationA + locationB) * 0.5f; }
	Vector3 const& get_location_a() const { return locationA; }
	Vector3 const& get_location_b() const { return locationB; }
	float get_radius() const { return radius; }

	float calculate_outside_distance(Vector3 const& _loc) const;

	bool does_overlap(Range3 const& _range) const;

public:
	bool load_from_xml(IO::XML::Node const * _node);

protected:
	Vector3 locationA;
	Vector3 locationB;
	float radius;

	CACHED_ Vector3 a2bNormal;
	CACHED_ float a2bDist;

#ifdef AN_DEVELOPMENT
	mutable bool highlightDebugDraw = false;
#endif
};
