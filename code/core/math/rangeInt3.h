#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct RangeInt3
{
public:
	RangeInt x;
	RangeInt y;
	RangeInt z;

	static RangeInt3 const zero;
	static RangeInt3 const empty;

	RangeInt3() {}
	explicit RangeInt3(RangeInt const & _x, RangeInt const & _y, RangeInt const & _z) : x(_x), y(_y), z(_z) {}
	explicit RangeInt3(VectorInt3 const & _a, VectorInt3 const & _b) : x(min(_a.x, _b.x), max(_a.x, _b.x)), y(min(_a.y, _b.y), max(_a.y, _b.y)) {}

	bool operator==(RangeInt3 const& _other) const { return x == _other.x && y == _other.y && z == _other.z; }
	bool operator!=(RangeInt3 const& _other) const { return x != _other.x || y != _other.y || z != _other.z; }

	RangeInt& access_element(uint _e) { an_assert(_e <= 2); return _e == 0 ? x : (_e == 1 ? y : z); }

	static RangeInt3 from_at_and_size(VectorInt3 const & _at, VectorInt3 const & _size) { return RangeInt3(RangeInt(_at.x, _at.x + _size.x - 1), RangeInt(_at.y, _at.y + _size.y - 1), RangeInt(_at.z, _at.z + _size.z - 1)); }
	inline VectorInt3 get_as_at() const { return VectorInt3(x.min, y.min, z.min); }
	inline VectorInt3 get_as_size() const { return VectorInt3(x.length(), y.length(), z.length()); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const* _zAttr = TXT("z"));

	bool save_to_xml(IO::XML::Node * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const* _zAttr = TXT("z")) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const* _zAttr = TXT("z")) const;

	inline String to_string() const;

	inline bool is_empty() const { return x.is_empty() || y.is_empty() || z.is_empty(); }

	inline VectorInt3 centre() const;
	inline VectorInt3 bottom_centre() const { return VectorInt3(x.centre(), y.centre(), z.min); }
	inline VectorInt3 near_bottom_left() const { return VectorInt3(x.min, y.min, z.min); }
	inline VectorInt3 far_top_right() const { return VectorInt3(x.max, y.max, z.max); }

	inline void include(RangeInt3 const & _RangeInt3);
	inline void intersect_with(RangeInt3 const & _RangeInt3);

	/** have valid value or empty */
	inline void include(VectorInt3 const & val);

	inline void expand_by(VectorInt3 const & val);

	inline void offset(VectorInt3 const& _by);

	/** needs at least one common point */
	inline bool overlaps(RangeInt3 const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(RangeInt3 const & _other) const;

	/** needs _other to be completely inside this RangeInt3 */
	inline bool does_contain(RangeInt3 const & _other) const;

	/** needs _point to be inside this RangeInt3 */
	inline bool does_contain(VectorInt3 const & _point) const;

	inline VectorInt3 length() const { return VectorInt3(x.length(), y.length(), z.length()); }
};
