#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct RangeRotator3
{
public:
	Range pitch;
	Range yaw;
	Range roll;

	static RangeRotator3 const zero;
	static RangeRotator3 const empty;

	RangeRotator3() {}
	explicit RangeRotator3(Range const & _pitch, Range const & _yaw, Range const & _roll): pitch( _pitch ), yaw( _yaw ), roll( _roll) {}

	inline static RangeRotator3 lerp(float _t, RangeRotator3 const& _a, RangeRotator3 const& _b);
		
	bool operator==(RangeRotator3 const& _other) const { return pitch == _other.pitch && yaw == _other.yaw && roll == _other.roll; }
	bool operator!=(RangeRotator3 const& _other) const { return !operator==(_other); }

	inline String to_debug_string() const;
	String to_string(int _decimals = 3, tchar const * _suffix = nullptr) const;

	inline bool is_empty() const { return pitch.is_empty() || yaw.is_empty() || roll.is_empty(); }
	inline bool has_something() const { return !pitch.is_empty() || ! yaw.is_empty() || ! roll.is_empty(); }

	inline void include(RangeRotator3 const & _RangeRotator3);
	inline void intersect_with(RangeRotator3 const & _RangeRotator3);

	inline Rotator3 get_at(Rotator3 const& _pt) const { return Rotator3(pitch.get_at(_pt.pitch), yaw.get_at(_pt.yaw), roll.get_at(_pt.roll)); }

	/** have valid value or empty */
	inline void include(Rotator3 const & val);
	inline void expand_by(Rotator3 const & _by);

	inline void scale_relative_to_centre(Rotator3 const & _by);

	/** needs at least one common point */
	inline bool overlaps(RangeRotator3 const & _other) const;
	/** cannot just touch, need to go into another */
	inline bool intersects(RangeRotator3 const & _other) const;
	inline bool intersects_extended(RangeRotator3 const & _other, float _by) const;

	/** needs _other to be completely inside this RangeRotator3 */
	inline bool does_contain(RangeRotator3 const & _other) const;
	inline bool does_contain(Rotator3 const & _point) const;

	inline Rotator3 centre() const { return Rotator3(pitch.centre(), yaw.centre(), roll.centre()); }
	inline Rotator3 length() const { return Rotator3(pitch.length(), yaw.length(), roll.length()); }

	inline Rotator3 clamp(Rotator3 const & _a) const { return Rotator3(pitch.clamp(_a.pitch), yaw.clamp(_a.yaw), roll.clamp(_a.roll)); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll"));
	bool load_from_string(String const & _string);
};

template <> bool Optional<RangeRotator3>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<RangeRotator3>::load_from_xml(IO::XML::Attribute const * _attr);
