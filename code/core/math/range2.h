#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Range2
{
public:
	Range x;
	Range y;

	static Range2 const zero;
	static Range2 const empty;

	Range2() {}
	explicit Range2(Vector2 const & _a): x(_a.x), y(_a.y) {}
	explicit Range2(Range const & _x, Range const & _y): x( _x ), y( _y ) {}

	Range& access_element(uint _e) { an_assert(_e <= 1); return _e == 0 ? x : y; }
	Range get_element(uint _e) const { an_assert(_e <= 1); return _e == 0 ? x : y; }

	inline static Range2 lerp(float _t, Range2 const& _a, Range2 const& _b);

	bool operator==(Range2 const & _other) const { return x == _other.x && y == _other.y; }
	bool operator!=(Range2 const& _other) const { return !operator==(_other); }
	Range2 & operator*=(Vector2 const & _scale) { x *= _scale.x; y *= _scale.y; return *this; }
	
	inline String to_debug_string() const;
	String to_string(int _decimals = 3, tchar const * _suffix = nullptr) const;

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrX = TXT("x"), tchar const * _attrY = TXT("y"));
	bool load_from_string(String const & _string);

	inline bool is_empty() const { return x.is_empty() || y.is_empty(); }

	inline Vector2 centre() const { return Vector2(x.centre(), y.centre()); }
	inline Vector2 bottom_centre() const { return Vector2(x.centre(), y.min); }
	inline Vector2 bottom_left() const { return Vector2(x.min, y.min); }
	inline Vector2 top_right() const { return Vector2(x.max, y.max); }

	Range2 mul_pt(float _minPtX, float _maxPtX, float _minPtY, float _maxPtY) { return Range2(x.mul_pt(_minPtX, _maxPtX), y.mul_pt(_minPtY, _maxPtY)); }

	inline Range2 & include(Range2 const & _Range2);
	inline void intersect_with(Range2 const & _Range2);

	/** have valid value or empty */
	inline Range2 & include(Vector2 const & val);

	/** needs at least one common point */
	inline bool overlaps(Range2 const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(Range2 const & _other) const;
	inline bool intersects_extended(Range2 const & _other, float _by) const;

	/** needs _other to be completely inside this Range2 */
	inline bool does_contain(Range2 const & _other) const;
	inline bool does_contain(Vector2 const & _other) const;

	inline void expand_by(Vector2 const & _size);
	inline Range2 expanded_by(Vector2 const & _size) const { Range2 result = *this; result.expand_by(_size); return result; }

	inline Vector2 length() const { return Vector2(x.length(), y.length()); }

	inline Vector2 get_at(Vector2 const & _pt) const { return Vector2(x.get_at(_pt.x), y.get_at(_pt.y)); }
	inline Vector2 get_pt_from_value(Vector2 const & _value) const { return Vector2(x.get_pt_from_value(_value.x), y.get_pt_from_value(_value.y)); }

	inline Range2 moved_by(Vector2 const & _by) const;
	inline Range2& move_by(Vector2 const& _by);
};

template <> bool Optional<Range2>::load_from_xml(IO::XML::Node const* _node);
template <> bool Optional<Range2>::load_from_xml(IO::XML::Attribute const* _attr);
