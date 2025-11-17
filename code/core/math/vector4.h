#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Vector4
{
public:
	float x;
	float y;
	float z;
	float w;

	static Vector4 const zero;

	inline Vector4() {}
	inline Vector4(float _x, float _y, float _z, float _w): x(_x), y(_y), z(_z), w(_w) {}

	float & access_element(uint _e) { an_assert(_e <= 3); return _e == 0 ? x : (_e == 1 ? y : (_e == 2 ? z : w)); }

	inline String to_string() const;
	inline Vector3 to_vector3() const;

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _wAttr = TXT("w"), bool _keepExistingValues = false);
	bool load_from_xml(IO::XML::Node const* _node, bool _keepExistingValues) { return load_from_xml(_node, TXT("x"), TXT("y"), TXT("z"), TXT("w"), _keepExistingValues); }
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _wAttr = TXT("w"), bool _keepExistingValues = false);
	bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childNode, bool _keepExistingValues) { return load_from_xml_child_node(_node, _childNode, TXT("x"), TXT("y"), TXT("z"), TXT("w"), _keepExistingValues); }
	bool load_from_string(String const & _string);

	inline Vector4 inverted() const { return Vector4(x != 0.0f ? 1.0f / x : 0.0f, y != 0.0f ? 1.0f / y : 0.0f, z != 0.0f ? 1.0f / z : 0.0f, w != 0.0f ? 1.0f / w : 0.0f); }

	inline Vector4 operator -() const { return Vector4(-x, -y, -z, -w); }
	inline Vector4 operator +(Vector4 const & _a) const { return Vector4(x + _a.x, y + _a.y, z + _a.z, w + _a.w); }
	inline Vector4 & operator +=(Vector4 const & _a) { x += _a.x; y += _a.y; z += _a.z; w += _a.w; return *this; }
	inline Vector4 operator -(Vector4 const & _a) const { return Vector4(x - _a.x, y - _a.y, z - _a.z, w - _a.w); }
	inline Vector4 & operator -=(Vector4 const & _a) { x -= _a.x; y -= _a.y; z -= _a.z; w -= _a.w; return *this; }
	inline Vector4 operator *(Vector4 const & _a) const { return Vector4(x * _a.x, y * _a.y, z * _a.z, w * _a.w); }
	inline Vector4 & operator *=(Vector4 const & _a) { x *= _a.x; y *= _a.y; z *= _a.z; w *= _a.w; return *this; }
	
	inline Vector4 operator *(float const & _a) const { return Vector4(x * _a, y * _a, z * _a, w * _a); }
	inline Vector4 & operator *=(float const & _a) { x *= _a; y *= _a; z *= _a; w *= _a; return *this; }
	inline Vector4 operator /(float const & _a) const { return Vector4(x / _a, y / _a, z / _a, w / _a); }
	inline Vector4 & operator /=(float const & _a) { x /= _a; y /= _a; z /= _a; w /= _a; return *this; }

	inline bool is_zero() const { return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f; }

	inline static float dot(Vector4 const & _a, Vector4 const & _b) { return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w; }

	inline static float distance(Vector4 const & _a, Vector4 const & _b) { return sqrt<float>(distance_squared(_a, _b)); }
	inline static float distance_squared(Vector4 const & _a, Vector4 const & _b) { return (sqr(_a.x - _b.x) + sqr(_a.y - _b.y) + sqr(_a.z - _b.z) + sqr(_a.w - _b.w)); }

	inline float length() const { return sqrt<float>(length_squared()); }
	inline float length_squared() const { return (sqr(x) + sqr(y) + sqr(z) + sqr(w)); }
		
	inline bool is_normalised() const;
	inline void normalise();
	inline Vector4 normal() const;

};

inline Vector4 operator *(float const & _a, Vector4 const &_b) { return Vector4(_b.x * _a, _b.y * _a, _b.z * _a, _b.w * _a); }

