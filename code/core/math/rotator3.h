#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Quat;

namespace RotatorComponent
{
	enum Type
	{
		Yaw,
		Pitch,
		Roll
	};
};

struct Rotator3
{
public:
	float pitch; // + up
	float yaw; // + right
	float roll; // + clockwise

	static Rotator3 const zero;
	static Rotator3 const xAxis;
	static Rotator3 const yAxis;
	static Rotator3 const zAxis;

	static inline float get_yaw(Vector2 const & _dir); // -180,180
	static inline float get_yaw(Vector3 const & _dir); // -180,180
	static inline float get_roll(Vector3 const & _dir); // -180,180
	static inline float normalise_axis(float _value) { _value = mod(_value, 360.0f); return _value <= 180.0f ? _value : _value - 360.0f; }

	inline Rotator3() {}
	inline Rotator3(float _pitch, float _yaw, float _roll): pitch(_pitch), yaw(_yaw), roll(_roll) {}
	inline Rotator3(Rotator3 const & _a): pitch(_a.pitch), yaw(_a.yaw), roll(_a.roll) {}

	inline static Rotator3 lerp(float _t, Rotator3 const& _a, Rotator3 const& _b);

	float & access_element(uint _e) { an_assert(_e <= 2); return _e == 0 ? pitch : (_e == 1 ? yaw : roll); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll"));
	bool load_from_string(String const & _string);

	bool save_to_xml(IO::XML::Node * _node, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll")) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _pitchAttr = TXT("pitch"), tchar const * _yawAttr = TXT("yaw"), tchar const * _rollAttr = TXT("roll")) const;

	inline String to_string() const;
	inline Quat to_quat() const;

	inline bool is_ok() const { return isfinite(pitch) && isfinite(yaw) && isfinite(roll); }

	inline Vector3 get_forward() const; // doesn't have any info about roll of course
	inline Vector3 get_axis(Axis::Type _axis) const; // doesn't have any info about roll of course

	inline float length() const; // when treated as vector
	inline float length_squared() const; // when treated as vector
	inline Rotator3 normal() const; // to 1.0

	inline float get_component(RotatorComponent::Type _type) const { return _type == RotatorComponent::Pitch ? pitch : (_type == RotatorComponent::Yaw ? yaw : roll); }
	inline float & access_component(RotatorComponent::Type _type) { return _type == RotatorComponent::Pitch ? pitch : (_type == RotatorComponent::Yaw ? yaw : roll); }

	inline Rotator3 normal_axes() const;

	//TODO public bool load_from_xml(XML.Node _node, string _pitchAttr = "pitch", string _yawAttr = "yaw", string _rollAttr = "roll")
	//TODO public bool load_from_xml_child_node(XML.Node _node, string _childNode, string _pitchAttr = "pitch", string _yawAttr = "yaw", string _rollAttr = "roll")

	inline Rotator3 operator -() const { return Rotator3(-pitch, -yaw, -roll); }
	inline Rotator3 operator +(Rotator3 const & _a) const { return Rotator3(pitch + _a.pitch, yaw + _a.yaw, roll + _a.roll); }
	inline Rotator3 & operator +=(Rotator3 const & _a) { pitch += _a.pitch; yaw += _a.yaw; roll += _a.roll; return *this; }
	inline Rotator3 operator -(Rotator3 const & _a) const { return Rotator3(pitch - _a.pitch, yaw - _a.yaw, roll - _a.roll); }
	inline Rotator3 & operator -=(Rotator3 const & _a) { pitch -= _a.pitch; yaw -= _a.yaw; roll -= _a.roll; return *this; }
	inline Rotator3 operator *(Rotator3 const & _a) const { return Rotator3(pitch * _a.pitch, yaw * _a.yaw, roll * _a.roll); }
	inline Rotator3 & operator *=(Rotator3 const & _a) { pitch *= _a.pitch; yaw *= _a.yaw; roll *= _a.roll; return *this; }
	
	inline Rotator3 operator *(float const & _a) const { return Rotator3(pitch * _a, yaw * _a, roll * _a); }
	inline Rotator3 & operator *=(float const & _a) { pitch *= _a; yaw *= _a; roll *= _a; return *this; }
	inline Rotator3 operator /(float const & _a) const { return Rotator3(pitch / _a, yaw / _a, roll / _a); }
	inline Rotator3 & operator /=(float const & _a) { pitch /= _a; yaw /= _a; roll /= _a; return *this; }

	inline bool operator ==(Rotator3 const & _a) const { return pitch == _a.pitch && yaw == _a.yaw && roll == _a.roll; }
	inline bool operator !=(Rotator3 const & _a) const { return pitch != _a.pitch || yaw != _a.yaw || roll != _a.roll; }

	inline bool is_zero() const { return pitch == 0.0f && yaw == 0.0f && roll == 0.0f; }

};

inline Rotator3 operator *(float const & _a, Rotator3 const &_b) { return Rotator3(_b.pitch * _a, _b.yaw * _a, _b.roll * _a); }

template <> bool Optional<Rotator3>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Rotator3>::load_from_xml(IO::XML::Attribute const * _attr);

