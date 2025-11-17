#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

namespace Collision
{
	struct CheckCollisionContext;
};

struct Plane
{
public:
	// storing as normal and anchor improves precision
	Vector3 normal;
	Vector3 anchor;

	static Plane const identity;
	static Plane const forward;
	static Plane const zero;
	static Plane const garbage;

	inline Plane() {}
	//inline Plane(float _x, float _y, float _z, float _d) : { normal = Vector3(_x, _y, _z); anchor =  an_assert(get_normal().is_zero() || get_normal().is_normalised());  }
	//inline Plane(Vector3 const & NORMAL_ _normal, float _d) : x(_normal.x), y(_normal.y), z(_normal.z), d(_d) { an_assert(_normal.is_zero() || _normal.is_normalised()); }
	inline Plane(Vector3 const & NORMAL_ _normal, Vector3 const & LOCATION_ _point) { an_assert(_normal.is_zero() || _normal.is_normalised(), TXT("%S"), _normal.to_string().to_char()); set(_normal, _point); }
	inline Plane(Vector3 const & LOCATION_ _a, Vector3 const & LOCATION_ _b, Vector3 const & LOCATION_ _c) { set(_a, _b, _c); }
	inline Plane(Plane const & _a): normal(_a.normal), anchor(_a.anchor) {}

	bool is_garbage() const { return *this == garbage; }

	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const* _childNode);

	static Vector3 intersect(Plane const & _a, Plane const & _b, Plane const & _c);
	static bool can_intersect(Plane const & _a, Plane const & _b, Plane const & _c);

	inline bool is_valid() const { return ! normal.is_zero(); }
	inline bool is_zero() const { return normal.is_zero() && anchor.is_zero(); }

	inline bool operator ==(Plane const & _other) const { return normal == _other.normal && (anchor == _other.anchor || calculate_d() == _other.calculate_d()); }
	inline bool operator !=(Plane const & _other) const { return !operator==(_other); }

	inline String to_string() const;

	inline Vector3 get_normal() const { return normal; }
	inline Vector3 get_anchor() const { return anchor; }
	inline float calculate_d() const { return -Vector3::dot(normal, anchor); }

	inline Plane negated() const { return Plane(-normal, anchor); }

	inline Vector4 to_vector4_for_rendering() const;

	//inline float calculate_y(float _x, float _z) const { return  -(x * _x + z * _z + d) * (y != 0.0f? 1.0f / y : 0.0f); }
	
	inline void set(Vector3 const & NORMAL_ _normal, Vector3 const & LOCATION_ _point);
	inline void set(Vector3 const & LOCATION_ _a, Vector3 const & LOCATION_ _b, Vector3 const & LOCATION_ _c); // setup using points

	// if positive, in front
	inline float get_in_front(Vector3 const & _point) const { return Vector3::dot(normal, _point - anchor); }
	inline Vector3 get_dropped(Vector3 const & _point) const { return _point - normal * get_in_front(_point); }
	inline Plane get_adjusted_along_normal(float _alongNormal) const { return Plane(normal, anchor + normal * _alongNormal); }

	inline Vector3 get_mirrored_dir(Vector3 const & _dir) const { Vector3 result = _dir; float alongNormal = Vector3::dot(get_normal(), _dir); result -= get_normal() * alongNormal * 2.0f; return result; }
	inline Vector3 get_dropped_dir(Vector3 const & _dir) const { Vector3 result = _dir; float alongNormal = Vector3::dot(get_normal(), _dir); result -= get_normal() * alongNormal; return result; }

	bool check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize = 0.0f) const;
	float calc_intersection_t(Segment const & _segment) const;
};
