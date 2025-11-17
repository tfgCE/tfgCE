#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct VectorInt4
{
public:
	int32 x;
	int32 y;
	int32 z;
	int32 w;

	static VectorInt4 const zero;

	inline VectorInt4() {}
	inline VectorInt4(int32 _x, int32 _y, int32 _z, int32 _w) : x(_x), y(_y), z(_z), w(_w) {}

	int32 & access_element(uint _e) { an_assert(_e <= 3); return _e == 0 ? x : (_e == 1 ? y : (_e == 2 ? z : w)); }

	inline String to_string() const;
	inline Vector4 to_vector4() const { return Vector4((float)x, (float)y, (float)z, (float)w); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _wAttr = TXT("w"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), tchar const * _zAttr = TXT("z"), tchar const * _wAttr = TXT("w"));
	bool load_from_string(String const & _string);

	inline VectorInt4 operator -() const { return VectorInt4(-x, -y, -z, -w); }
	inline VectorInt4 operator +(VectorInt4 const & _a) const { return VectorInt4(x + _a.x, y + _a.y, z + _a.z, w + _a.w); }
	inline VectorInt4 & operator +=(VectorInt4 const & _a) { x += _a.x; y += _a.y; z += _a.z; w += _a.w; return *this; }
	inline VectorInt4 operator -(VectorInt4 const & _a) const { return VectorInt4(x - _a.x, y - _a.y, z - _a.z, w - _a.w); }
	inline VectorInt4 & operator -=(VectorInt4 const & _a) { x -= _a.x; y -= _a.y; z -= _a.z; w -= _a.w; return *this; }
	inline VectorInt4 operator *(int32 const & _a) const { return VectorInt4(x * _a, y * _a, z * _a, w * _a); }
	inline VectorInt4 & operator *=(int32 const & _a) { x *= _a; y *= _a; z *= _a; w *= _a; return *this; }
	inline VectorInt4 operator *(VectorInt4 const & _a) const { return VectorInt4(x * _a.x, y * _a.y, z * _a.z, w * _a.w); }
	inline VectorInt4 & operator *=(VectorInt4 const & _a) { x *= _a.x; y *= _a.y; z *= _a.z; w *= _a.w; return *this; }
	inline VectorInt4 operator *(Vector4 const & _a) const { return VectorInt4((int32)((float)x * _a.x), (int32)((float)y * _a.y), (int32)((float)z * _a.z), (int32)((float)w * _a.w)); }
	inline VectorInt4 & operator *=(Vector4 const & _a) { x = (int32)((float)x * _a.x); y = (int32)((float)y * _a.y); z = (int32)((float)z * _a.z); w = (int32)((float)w * _a.w); return *this; }

	inline VectorInt4 operator *(float const & _a) const { return VectorInt4((int32)((float)x * _a), (int32)((float)y * _a), (int32)((float)z * _a), (int32)((float)w * _a)); }
	inline VectorInt4 & operator *=(float const & _a) { x = (int32)((float)x * _a); y = (int32)((float)y * _a); z = (int32)((float)z * _a); w = (int32)((float)w * _a); return *this; }

	inline bool is_zero() const { return x == 0 && y == 0 && z == 0 && w == 0; }

};

inline VectorInt4 operator *(int32 const & _a, VectorInt4 const &_b) { return VectorInt4(_b.x * _a, _b.y * _a, _b.z * _a, _b.w * _a); }
inline VectorInt4 operator *(float const & _a, VectorInt4 const &_b) { return VectorInt4((int32)((float)_b.x * _a), (int32)((float)_b.y * _a), (int32)((float)_b.z * _a), (int32)((float)_b.w * _a)); }
