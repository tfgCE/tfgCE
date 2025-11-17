#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Vector3
{
public:
	float x;
	float y;
	float z;

	static Vector3 const zero;
	static Vector3 const one;
	static Vector3 const half;
	static Vector3 const xAxis;
	static Vector3 const yAxis;
	static Vector3 const zAxis;
	static Vector3 const xy;
	static Vector3 const xz;

	inline Vector3() {}
	inline explicit Vector3(float _v) : x(_v), y(_v), z(_v) {}
	inline Vector3(float _x, float _y, float _z): x(_x), y(_y), z(_z) {}
	inline Vector3(Vector3 const & _a): x(_a.x), y(_a.y), z(_a.z) {}
	//TODO inline Vector3(Rotator3 const & _rot);

	inline static Vector3 const & axis(Axis::Type _t);

	inline static Vector3 lerp(float _t, Vector3 const & _a, Vector3 const & _b);
	
	float & access_element(uint _e) { an_assert(_e <= 2); return _e == 0 ? x : (_e == 1 ? y : z); }
	float get_element(uint _e) const { an_assert(_e <= 2); return _e == 0 ? x : (_e == 1 ? y : z); }

	inline String to_string() const;
	inline Rotator3 to_rotator() const;
	inline Vector2 to_vector2() const;
	inline Vector4 to_vector4(float _w = 0.0f) const;
	inline VectorInt3 to_vector_int3() const;

	inline bool is_ok() const { return isfinite(x) && isfinite(y) && isfinite(z); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _axisAttr = TXT("axis"), bool _keepExistingValues = false);
	bool load_from_xml(IO::XML::Node const* _node, bool _keepExistingValues) { return load_from_xml(_node, TXT("x"), TXT("y"), TXT("z"), TXT("axis"), _keepExistingValues); }
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _axisAttr = TXT("axis"), bool _keepExistingValues = false);
	bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childNode, bool _keepExistingValues) { return load_from_xml_child_node(_node, _childNode, TXT("x"), TXT("y"), TXT("z"), TXT("axis"), _keepExistingValues); }
	bool load_from_string(String const & _string);
	
	bool save_to_xml(IO::XML::Node * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z")) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z")) const;

	static Vector3 load_axis_from_string(String const & _string);

	inline Vector3 inverted() const { return Vector3(x != 0.0f? 1.0f / x : 0.0f, y!= 0.0f? 1.0f / y : 0.0f, z!= 0.0f? 1.0f / z : 0.0f); }

	inline bool operator ==(Vector3 const & _a) const { return x == _a.x && y == _a.y && z == _a.z; }
	inline bool operator !=(Vector3 const & _a) const { return x != _a.x || y != _a.y || z != _a.z; }

	inline Vector3 operator -() const { return Vector3(-x, -y, -z); }
	inline Vector3 operator +(Vector3 const & _a) const { return Vector3(x + _a.x, y + _a.y, z + _a.z); }
	inline Vector3 & operator +=(Vector3 const & _a) { x += _a.x; y += _a.y; z += _a.z; return *this; }
	inline Vector3 operator -(Vector3 const & _a) const { return Vector3(x - _a.x, y - _a.y, z - _a.z); }
	inline Vector3 & operator -=(Vector3 const & _a) { x -= _a.x; y -= _a.y; z -= _a.z; return *this; }
	inline Vector3 operator *(Vector3 const & _a) const { return Vector3(x * _a.x, y * _a.y, z * _a.z); }
	inline Vector3 & operator *=(Vector3 const & _a) { x *= _a.x; y *= _a.y; z *= _a.z; return *this; }
	inline Vector3 operator /(Vector3 const& _a) const { return Vector3(x / _a.x, y / _a.y, z / _a.z); }
	inline Vector3& operator /=(Vector3 const& _a) { x /= _a.x; y /= _a.y; z /= _a.z; return *this; }

	inline Vector3 operator *(float const & _a) const { return Vector3(x * _a, y * _a, z * _a); }
	inline Vector3 & operator *=(float const & _a) { x *= _a; y *= _a; z *= _a; return *this; }
	inline Vector3 operator /(float const & _a) const { return Vector3(x / _a, y / _a, z / _a); }
	inline Vector3 & operator /=(float const & _a) { x /= _a; y /= _a; z /= _a; return *this; }

	inline bool is_zero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }
	inline bool is_almost_zero(float const _epsilon = 0.0001f) const { return abs(x) <= _epsilon && abs(y) <= _epsilon && abs(z) <= _epsilon; }
	inline static bool are_almost_equal(Vector3 const & _a, Vector3 const & _b, float const _epsilon = 0.0001f) { return abs(_a.x - _b.x) <= _epsilon && abs(_a.y - _b.y) <= _epsilon && abs(_a.z - _b.z) <= _epsilon; }

	inline static float dot(Vector3 const & _a, Vector3 const & _b) { return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z; }
	// xAxis = Vector3::cross(yAxis, zAxis);
	// yAxis = Vector3::cross(zAxis, xAxis);
	// zAxis = Vector3::cross(xAxis, yAxis);
	inline static Vector3 cross(Vector3 const & _a, Vector3 const & _b) { return Vector3(_a.y * _b.z - _a.z * _b.y,				// x = y (x) z
																						 _a.z * _b.x - _a.x * _b.z,				// y = z (x) x
																						 _a.x * _b.y - _a.y * _b.x); }			// z = x (x) y

	inline static float distance(Vector3 const & _a, Vector3 const & _b) { return sqrt<float>(distance_squared(_a, _b)); }
	inline static float distance_squared(Vector3 const & _a, Vector3 const & _b) { return (sqr(_a.x - _b.x) + sqr(_a.y - _b.y) + sqr(_a.z - _b.z)); }

	inline float length() const { return sqrt<float>(length_squared()); }
	inline float length_squared() const { return (sqr(x) + sqr(y) + sqr(z)); }

	inline float length_2d() const { return sqrt<float>(length_squared_2d()); }
	inline float length_squared_2d() const { return (sqr(x) + sqr(y)); }

	inline bool is_normalised() const;
	inline void normalise();
	inline void normalise_2d();
	inline Vector3 normal() const;
	inline Vector3 normal_2d() const;

	Vector3 get_any_perpendicular() const; // perpendicular to us

	inline Vector3 drop_using(Vector3 const & _alongDir) const; // will be dropped onto plane perpendicular to alongDir
	inline Vector3 drop_using_normalised(Vector3 const & _alongDir) const; // will be dropped onto plane perpendicular to alongDir
	inline Vector3 along(Vector3 const & _alongDir) const; // will be dropped onto along dir
	inline Vector3 along_normalised(Vector3 const & _alongDir) const; // will be dropped onto along dir

	inline Vector3 keep_within_angle(Vector3 const & _otherDir, float _maxAngleDot) const;
	inline Vector3 keep_within_angle_normalised(Vector3 const & _otherDir, float _maxAngleDot) const;

	inline void normal_and_length(OUT_ Vector3 & _normal, OUT_ float & _length) const;

	inline void rotate_right() { float temp = x; x = y; y = -temp; }
	inline Vector3 rotated_right() const { return Vector3(y, -x, z); }

	inline static Vector3 blend(Vector3 const & _a, Vector3 const & _b, float _t);

	inline void rotate_by_pitch(float _pitch);
	inline Vector3 rotated_by_pitch(float _pitch) const;

	inline void rotate_by_yaw(float _yaw);
	inline Vector3 rotated_by_yaw(float _yaw) const;

	inline void rotate_by_roll(float _roll);
	inline Vector3 rotated_by_roll(float _roll) const;

	bool serialise(Serialiser & _serialiser);
};

inline Vector3 operator *(float const & _a, Vector3 const &_b) { return Vector3(_b.x * _a, _b.y * _a, _b.z * _a); }

template <> bool Optional<Vector3>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Vector3>::load_from_xml(IO::XML::Attribute const * _attr);
