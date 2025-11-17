#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct RangeInt
{
public:
	int min;
	int max;

	static RangeInt const zero;
	static RangeInt const empty;

	RangeInt() {}
	explicit RangeInt(int _at) : min(_at), max(_at) {}
	explicit RangeInt(int _min, int _max) : min(_min), max(_max) {}

	bool operator==(RangeInt const& _other) const { return min == _other.min && max == _other.max; }
	bool operator!=(RangeInt const& _other) const { return min != _other.min || max != _other.max; }

	inline String to_string() const;
	inline String to_parsable_string() const;
	inline Range to_range() const;

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr = TXT("range"));
	bool load_from_xml_or_text(IO::XML::Node const * _node, tchar const * _attr = TXT("range"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attr = nullptr);
	bool load_from_string(String const & _string);

	bool save_to_xml(IO::XML::Node* _node, tchar const* _attr = TXT("range")) const;

	static RangeInt at(int _at) { return RangeInt(_at, _at); }

	inline bool is_empty() const { return min > max; }

	inline void include(RangeInt const & _range);
	inline void intersect_with(RangeInt const & _range);

	/** have valid value or empty */
	inline void include(int val);
	inline void expand_by(int _by);

	inline void offset(int _by);

	inline void limit_min(int _to); // min can only get bigger
	inline void limit_max(int _to); // max can only get smaller

	inline RangeInt get_limited_to(RangeInt const & _range) const; // will result in non empty

	inline RangeInt operator*(int _by) const;
		
	/** needs at least one common point */
	inline bool overlaps(RangeInt const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(RangeInt const & _other) const;
	inline bool intersects_extended(RangeInt const & _other, int _by) const;

	/** needs _other to be completely inside this RangeInt */
	inline bool does_contain(RangeInt const & _other) const;

	inline int distance_from(int _value) const;
		
	inline int length() const { return ::max<int>(max - min + 1, 0); } // for int it makes sense to have + 1 as range 0,1 contains two values
	inline int centre() const { return (min + max) / 2; }
	inline int get_at(float _pt) const { return (int)((float)min * (1.0f - _pt) + (float)max * _pt); }
	inline bool does_contain(int _value) const { return _value >= min && _value <= max; }
	inline bool does_contain_extended(int _value, int _by) const { return _value >= min - _by && _value <= max + _by; }
	inline int clamp(int _value) const { return min <= max? ::clamp(_value, min, max) : _value; }
	inline int clamp_extended(int _value, int _by) const;
};

template <> bool Optional<RangeInt>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<RangeInt>::load_from_xml(IO::XML::Attribute const * _attr);
