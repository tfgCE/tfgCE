#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct RangeInt2
{
public:
	RangeInt x;
	RangeInt y;

	static RangeInt2 const zero;
	static RangeInt2 const empty;

	RangeInt2() {}
	explicit RangeInt2(RangeInt const & _x, RangeInt const & _y) : x(_x), y(_y) {}
	explicit RangeInt2(VectorInt2 const & _a, VectorInt2 const & _b) : x(min(_a.x, _b.x), max(_a.x, _b.x)), y(min(_a.y, _b.y), max(_a.y, _b.y)) {}

	bool operator==(RangeInt2 const& _other) const { return x == _other.x && y == _other.y; }
	bool operator!=(RangeInt2 const& _other) const { return x != _other.x || y != _other.y; }

	RangeInt & access_element(uint _e) { an_assert(_e <= 1); return _e == 0 ? x : y; }

	static RangeInt2 from_at_and_size(VectorInt2 const & _at, VectorInt2 const & _size) { return RangeInt2(RangeInt(_at.x, _at.x + _size.x - 1), RangeInt(_at.y, _at.y + _size.y - 1)); }
	inline VectorInt2 get_as_at() const { return VectorInt2(x.min, y.min); }
	inline VectorInt2 get_as_size() const { return VectorInt2(x.length(), y.length()); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"));

	bool save_to_xml(IO::XML::Node* _node, tchar const* _xAttr = TXT("x"), tchar const* _yAttr = TXT("y")) const;
	bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childNode, tchar const* _xAttr = TXT("x"), tchar const* _yAttr = TXT("y")) const;

	inline String to_string() const;
	inline Range2 to_range2() const;

	inline bool is_empty() const { return x.is_empty() || y.is_empty(); }

	inline VectorInt2 centre() const;
	inline VectorInt2 bottom_centre() const { return VectorInt2(x.centre(), y.min); }
	inline VectorInt2 bottom_left() const { return VectorInt2(x.min, y.min); }
	inline VectorInt2 top_right() const { return VectorInt2(x.max, y.max); }

	inline void include(RangeInt2 const & _RangeInt2);
	inline void intersect_with(RangeInt2 const & _RangeInt2);

	/** have valid value or empty */
	inline void include(VectorInt2 const & val);

	inline void expand_by(VectorInt2 const & val);

	inline void offset(VectorInt2 const& _by);

	/** needs at least one common point */
	inline bool overlaps(RangeInt2 const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(RangeInt2 const & _other) const;

	/** needs _other to be completely inside this RangeInt2 */
	inline bool does_contain(RangeInt2 const & _other) const;

	/** needs _point to be inside this RangeInt2 */
	inline bool does_contain(VectorInt2 const & _point) const;

	inline VectorInt2 clamp(VectorInt2 const& _value) const { return VectorInt2(x.clamp(_value.x), y.clamp(_value.y)); }
	inline VectorInt2 clamp_extended(VectorInt2 const& _value, VectorInt2 const & _by) const { return VectorInt2(x.clamp_extended(_value.x, _by.x), y.clamp_extended(_value.y, _by.y)); }

	inline VectorInt2 length() const { return VectorInt2(x.length(), y.length()); }
};
