#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Range3
{
public:
	Range x;
	Range y;
	Range z;

	static Range3 const zero;
	static Range3 const empty;

	Range3() {}
	explicit Range3(Range const & _x, Range const & _y, Range const & _z): x( _x ), y( _y ), z( _z ) {}

	inline static Range3 lerp(float _t, Range3 const& _a, Range3 const& _b);
		
	bool operator==(Range3 const& _other) const { return x == _other.x && y == _other.y && z == _other.z; }
	bool operator!=(Range3 const& _other) const { return !operator==(_other); }

	inline String to_debug_string() const;
	String to_string(int _decimals = 3, tchar const * _suffix = nullptr) const;

	inline bool is_empty() const { return x.is_empty() || y.is_empty() || z.is_empty(); }
	inline bool has_something() const { return ! x.is_empty() || ! y.is_empty() || ! z.is_empty(); }

	inline void include(Range3 const & _Range3);
	inline void intersect_with(Range3 const & _Range3);

	inline void make_non_flat();

	inline Vector3 get_at(Vector3 const& _pt) const { return Vector3(x.get_at(_pt.x), y.get_at(_pt.y), z.get_at(_pt.z)); }

	/** have valid value or empty */
	inline void include(Vector3 const & val);
	inline void expand_by(Vector3 const & _by);

	inline void scale_relative_to_centre(Vector3 const & _by);

	/** needs at least one common point */
	inline bool overlaps(Range3 const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(Range3 const & _other) const;
	inline bool intersects_extended(Range3 const & _other, float _by) const;

	/** needs _other to be completely inside this Range3 */
	inline bool does_contain(Range3 const & _other) const;
	inline bool does_contain(Vector3 const & _point) const;

	bool intersects_triangle(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c) const;

	inline Vector3 centre() const { return Vector3(x.centre(), y.centre(), z.centre()); }
	inline Vector3 length() const { return Vector3(x.length(), y.length(), z.length()); }
	inline Vector3 near_bottom_left() const { return Vector3(x.min, y.min, z.min); }
	inline Vector3 far_top_right() const { return Vector3(x.max, y.max, z.max); }

	inline Vector3 clamp(Vector3 const & _a) const { return Vector3(x.clamp(_a.x), y.clamp(_a.y), z.clamp(_a.z)); }

	inline void construct_from_location_and_axis_offsets(Vector3 const & _location, Vector3 const & _xm, Vector3 const & _xp, Vector3 const & _ym, Vector3 const & _yp, Vector3 const & _zm, Vector3 const & _zp);
	inline void construct_from_placement_and_range3(Transform const & _placement, Range3 const & _range3);

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"));
	bool load_from_string(String const & _string);
};

template <> bool Optional<Range3>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Range3>::load_from_xml(IO::XML::Attribute const * _attr);
