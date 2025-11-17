#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct VectorInt3
{
public:
	int32 x;
	int32 y;
	int32 z;

	static VectorInt3 const zero;
	static VectorInt3 const one;

	inline VectorInt3() {}
	inline VectorInt3(int32 _x, int32 _y, int32 _z): x(_x), y(_y), z(_z) {}

	int32 & access_element(uint _e) { an_assert(_e <= 2); return _e == 0 ? x : (_e == 1? y : z); }
	int32 get_element(uint _e) const { an_assert(_e <= 2); return _e == 0 ? x : (_e == 1? y : z); }

	inline String to_string() const;
	inline Vector3 to_vector3() const { return Vector3((float)x, (float)y, (float)z); }
	inline VectorInt2 to_vector_int2() const { return VectorInt2(x, y); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"));
	bool load_from_xml_child_or_attr(IO::XML::Node const * _node, tchar const * _attrOrChild);
	bool load_from_string(String const & _string);

	bool save_to_xml(IO::XML::Node * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z")) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z")) const;

	inline VectorInt3 operator -() const { return VectorInt3(-x, -y, -z); }
	inline VectorInt3 operator +(VectorInt3 const & _a) const { return VectorInt3(x + _a.x, y + _a.y, z + _a.z); }
	inline VectorInt3 & operator +=(VectorInt3 const & _a) { x += _a.x; y += _a.y; z += _a.z; return *this; }
	inline VectorInt3 operator -(VectorInt3 const & _a) const { return VectorInt3(x - _a.x, y - _a.y, z - _a.z); }
	inline VectorInt3 & operator -=(VectorInt3 const & _a) { x -= _a.x; y -= _a.y; z -= _a.z; return *this; }
	inline VectorInt3 operator *(int32 const & _a) const { return VectorInt3(x * _a, y * _a, z * _a); }
	inline VectorInt3 & operator *=(int32 const & _a) { x *= _a; y *= _a; z *= _a;  return *this; }
	inline VectorInt3 operator *(VectorInt3 const & _a) const { return VectorInt3(x * _a.x, y * _a.y, z * _a.z); }
	inline VectorInt3 & operator *=(VectorInt3 const & _a) { x *= _a.x; y *= _a.y; z *= _a.z; return *this; }
	inline VectorInt3 operator /(VectorInt3 const & _a) const { return VectorInt3(x / _a.x, y / _a.y, z / _a.z); }
	inline VectorInt3 & operator /=(VectorInt3 const & _a) { x /= _a.x; y /= _a.y; z /= _a.z; return *this; }
	inline VectorInt3 operator /(int _a) const { return VectorInt3(x / _a, y / _a, z / _a); }
	inline VectorInt3 & operator /=(int & _a) { x /= _a; y /= _a; z /= _a; return *this; }
	inline VectorInt3 operator *(Vector3 const & _a) const { return VectorInt3((int32)((float)x * _a.x), (int32)((float)y * _a.y), (int32)((float)z * _a.z)); }
	inline VectorInt3 & operator *=(Vector3 const & _a) { x = (int32)((float)x * _a.x); y = (int32)((float)y * _a.y); z = (int32)((float)z * _a.z); return *this; }
	inline VectorInt3 operator *(float const & _a) const { return VectorInt3((int32)((float)x * _a), (int32)((float)y * _a), (int32)((float)z * _a)); }
	inline VectorInt3 & operator *=(float const & _a) { x = (int32)((float)x * _a); y = (int32)((float)y * _a); z = (int32)((float)z * _a); return *this; }
	
	inline static int dot(VectorInt3 const & _a, VectorInt3 const & _b) { return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z; }

	inline bool operator ==(VectorInt3 const & _a) const { return x == _a.x && y == _a.y && z == _a.z;}
	inline bool operator !=(VectorInt3 const & _a) const { return x != _a.x || y != _a.y || z != _a.z; }

	inline bool is_zero() const { return x == 0 && y == 0 && z == 0; }

	bool serialise(Serialiser & _serialiser);
};

inline VectorInt3 operator *(int32 const & _a, VectorInt3 const &_b) { return VectorInt3(_b.x * _a, _b.y * _a, _b.z * _a); }
inline VectorInt3 operator *(float const & _a, VectorInt3 const &_b) { return VectorInt3((int32)((float)_b.x * _a), (int32)((float)_b.y * _a), (int32)((float)_b.z * _a)); }
