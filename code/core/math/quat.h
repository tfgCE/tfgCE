#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Quat
{
public:
	float x;
	float y;
	float z;
	float w;

	static Quat const zero;
	static Quat const identity;

	inline Quat() {}
	inline Quat(float _x, float _y, float _z, float _w): x(_x), y(_y), z(_z), w(_w) {}
	inline Quat(Quat const & _a): x(_a.x), y(_a.y), z(_a.z), w(_a.w) {}

	inline static Quat lerp(float _t, Quat const & _a, Quat const & _b);
	inline static Quat slerp(float _t, Quat const & _a, Quat const & _b);

	bool load_from_xml_as_rotator(IO::XML::Node const * _node);
	bool load_from_xml_child_node_as_rotator(IO::XML::Node const * _node, tchar const * _childNode);

	bool save_to_xml_as_rotator(IO::XML::Node * _node) const;
	bool save_to_xml_child_node_as_rotator(IO::XML::Node * _node, tchar const * _childNode) const;

	inline bool is_ok() const { return isfinite(x) && isfinite(y) && isfinite(z) && isfinite(w); }

	inline String to_string() const;

	inline Matrix33 to_matrix_33() const;
	inline Matrix44 to_matrix() const;

	inline Rotator3 to_rotator() const;

	inline void fill_matrix33(Matrix33 & _mat) const;
	inline void fill_matrix33(Matrix44 & _mat) const;

	// but if you need all axis, better change to matrix first
	inline Vector3 get_x_axis() const;
	inline Vector3 get_y_axis() const;
	inline Vector3 get_z_axis() const;
	inline Vector3 get_axis(Axis::Type _axis) const;

	inline static Quat axis_rotation(Vector3 const & _axis, float _angle);
	inline static Quat axis_rotation_normalised(Vector3 const & _axis, float _angle);

	inline Quat inverted() const { an_assert(is_normalised()); return Quat(-x, -y, -z, w); }

	// rotation is if we rotate differently, even if we get same final result - it is more strict
	inline static bool same_rotation(Quat const & _a, Quat const & _b, Optional<float> const & _threshold = NP) { return dot(_a, _b) >= 1.0f - _threshold.get(0.0001f); }
	// orientation is interested only in final result, not how we rotated there
	inline static bool same_orientation(Quat const& _a, Quat const& _b, Optional<float> const& _threshold = NP) { return abs(dot(_a, _b)) >= 1.0f - _threshold.get(0.0001f); } // more strict
	// just same, exactly
	inline static bool same(Quat const& _a, Quat const& _b, Optional<float> const& _threshold = NP) { float t = _threshold.get(0.0f); return t > 0.0f? abs(dot(_a, _b)) >= 1.0f - t : _a == _b; } // more strict

	inline Quat operator -() const { return Quat(-x, -y, -z, -w); }
	inline Quat operator +(Quat const & _a) const { return Quat(x + _a.x, y + _a.y, z + _a.z, w + _a.w); }
	inline Quat & operator +=(Quat const & _a) { x += _a.x; y += _a.y; z += _a.z; w += _a.w; return *this; }
	inline Quat operator -(Quat const & _a) const { return Quat(x - _a.x, y - _a.y, z - _a.z, w - _a.w); }
	inline Quat & operator -=(Quat const & _a) { x -= _a.x; y -= _a.y; z -= _a.z; w -= _a.w; return *this; }
	inline Quat operator *(Quat const & _a) const { return Quat(x * _a.x, y * _a.y, z * _a.z, w * _a.w); }
	inline Quat & operator *=(Quat const & _a) { x *= _a.x; y *= _a.y; z *= _a.z; w *= _a.w; return *this; }
	
	inline Quat operator *(float const & _a) const { return Quat(x * _a, y * _a, z * _a, w * _a); }
	inline Quat & operator *=(float const & _a) { x *= _a; y *= _a; z *= _a; w *= _a; return *this; }
	inline Quat operator /(float const & _a) const { return Quat(x / _a, y / _a, z / _a, w / _a); }
	inline Quat & operator /=(float const & _a) { x /= _a; y /= _a; z /= _a; w /= _a; return *this; }
	
	// blend identity with quat (1.0f is fully this one, 0.0f is identity)
	inline Quat blended_from_identity(float _fromIdentity) const { return Quat(x * _fromIdentity, y * _fromIdentity, z * _fromIdentity, w * _fromIdentity + 1.0f * (1.0f - _fromIdentity)); }
	inline Quat blended_from_identity_normalised(float _fromIdentity) const { return Quat(x * _fromIdentity, y * _fromIdentity, z * _fromIdentity, w * _fromIdentity + 1.0f * (1.0f - _fromIdentity)).normal(); }

	inline bool is_identity() const { return x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f; }
	inline bool is_zero() const { return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f; }

	inline static float dot(Quat const & _a, Quat const & _b) { return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w; }
	
	inline float length() const { return sqrt<float>(length_squared()); }
	inline float length_squared() const { return (sqr(x) + sqr(y) + sqr(z) + sqr(w)); }
		
	inline bool is_normalised(float _margin = 0.01f) const;
	inline void normalise();
	inline Quat normal() const;

	inline Vector3 rotate(Vector3 const & v) const;
	inline Vector3 inverted_rotate(Vector3 const & v) const;
	inline Vector3 to_world(Vector3 const & v) const { return rotate(v); }
	inline Vector3 to_local(Vector3 const & v) const { return inverted_rotate(v); }

	inline Quat rotate_by(Quat const & q) const;
	inline Quat inverted_rotate_by(Quat const & q) const;
	inline Quat to_world(Quat const & q) const { return rotate_by(q); }
	inline Quat to_local(Quat const & q) const { return inverted_rotate_by(q); }

	bool serialise(Serialiser & _serialiser);

private:
	// operators are prohibited! as it is not that clear what to we expect
	inline bool operator ==(Quat const & _a) const { return x == _a.x && y == _a.y && z == _a.z && w == _a.w; }
	inline bool operator !=(Quat const & _a) const { return x != _a.x || y != _a.y || z != _a.z || w != _a.w; }

	friend struct Transform;
};

inline Quat operator *(float const & _a, Quat const &_b) { return Quat(_b.x * _a, _b.y * _a, _b.z * _a, _b.w * _a); }
