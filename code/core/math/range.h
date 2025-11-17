#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Range
{
public:
	float min;
	float max;

	static Range const zero;
	static Range const empty;
	static Range const infinite;

	Range() {}
	explicit Range(float _at) : min(_at), max(_at) {}
	explicit Range(float _min, float _max): min( _min ), max( _max ) {}

	static Range of_length(float _v, float _len) { return Range(_v, _v + _len); }

	inline static Range lerp(float _t, Range const& _a, Range const& _b);
	
	bool operator==(Range const & _other) const { return min == _other.min && max == _other.max; }
	bool operator!=(Range const & _other) const { return ! operator==(_other); }

	String to_debug_string() const;
	String to_percent_string(int _decimals = 0) const;
	String to_string(int _decimals = 3, tchar const * _suffix = nullptr) const;

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr = TXT("range"));
	bool load_from_xml_or_text(IO::XML::Node const * _node, tchar const * _attr = TXT("range"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attr = nullptr);
	bool load_from_attribute_or_from_child(IO::XML::Node const * _node, tchar const * _name);
	bool load_from_string(String const & _string);

	static Range at(float _at) { return Range(_at, _at); }

	Range mul_pt(float _minPt, float _maxPt) { return Range(get_at(_minPt), get_at(_maxPt)); }

	inline bool is_empty() const { return min > max; }
	inline bool is_zero() const { return min == 0.0f && max == 0.0f; }

	inline void include(Range const & _range);
	inline void intersect_with(Range const & _range);

	/** have valid value or empty */
	inline void include(float val);
	inline void expand_by(float _by);
	inline Range expanded_by(float _by) const { Range result = *this; result.expand_by(_by); return result; }

	inline void offset(float _by);

	inline void limit_min(float _to);
	inline void limit_max(float _to);

	inline void scale_relative_to_centre(float _by);

	inline Range moved_by(float _by) const;
	inline Range& move_by(float _by);
	inline Range operator*(float _by) const;
	inline Range & operator*=(float _by);
	inline Range operator+(float _by) const;
	inline Range & operator+=(float _by);
	inline Range operator+(Range const & _by) const;
	inline Range operator-(Range const & _by) const;
	Range operator-() const { return Range(-max, -min); }

	/** needs at least one common point */
	inline bool overlaps(Range const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(Range const & _other) const;
	inline bool intersects_extended(Range const & _other, float _by) const;

	/** needs _other to be completely inside this range */
	inline bool does_contain(Range const & _other) const;

	inline float distance_from(float _value) const;
		
	inline float length() const { return ::max<float>(max - min, 0.0f); }
	inline float centre() const { return (min + max) * 0.5f; }
	inline float get_at(float _pt) const { return min * (1.0f - _pt) + max * _pt; }
	inline float get_pt_from_value(float _value) const { return max != min? (_value - min) / (max - min) : (_value < min? 0.0f : 1.0f); }
	inline bool does_contain(float _value) const { return _value >= min && _value <= max; }
	inline bool does_contain_extended(float _value, float _by) const { return _value >= min - _by && _value <= max + _by; }
	inline float clamp(float _value) const { return min <= max? ::clamp(_value, min, max) : _value; }
	inline float clamp_extended(float _value, float _by) const;
	inline float mod_into(float _value) const { return min + mod(_value - min, max - min); }
	inline Range get_part(Range const & _pt) const;

	inline Range clamp_to(Range const& _limit) const { return Range(_limit.clamp(min), _limit.clamp(max)); }

	bool serialise(Serialiser & _serialiser);

	static float transform(float _value, Range const & _from, Range const & _to);
	static float transform_clamp(float _value, Range const & _from, Range const & _to);
};

template <> bool Optional<Range>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Range>::load_from_xml(IO::XML::Attribute const * _attr);
