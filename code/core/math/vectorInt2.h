#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct VectorInt2
{
public:
	int32 x;
	int32 y;

	static VectorInt2 const zero;
	static VectorInt2 const one;

	inline VectorInt2() {}
	inline explicit VectorInt2(int32 _v): x(_v), y(_v) {}
	inline VectorInt2(int32 _x, int32 _y): x(_x), y(_y) {}

	int32 & access_element(uint _e) { an_assert(_e <= 1); return _e == 0 ? x : y; }
	int32 get_element(uint _e) const { an_assert(_e <= 1); return _e == 0 ? x : y; }

	inline String to_string() const;
	inline String to_loadable_string() const;
	inline Vector2 to_vector2() const { return Vector2((float)x, (float)y); }
	inline VectorInt3 to_vector_int3(int _z = 0) const;

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"));
	bool load_from_xml_child_or_attr(IO::XML::Node const * _node, tchar const * _attrOrChild);
	bool load_from_string(String const & _string);

	bool save_to_xml(IO::XML::Node * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y")) const;
	bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childNode, tchar const* _xAttr = TXT("x"), tchar const* _yAttr = TXT("y")) const;

	inline VectorInt2 operator -() const { return VectorInt2(-x, -y); }
	inline VectorInt2 operator +(VectorInt2 const & _a) const { return VectorInt2(x + _a.x, y + _a.y); }
	inline VectorInt2 & operator +=(VectorInt2 const & _a) { x += _a.x; y += _a.y; return *this; }
	inline VectorInt2 operator -(VectorInt2 const & _a) const { return VectorInt2(x - _a.x, y - _a.y); }
	inline VectorInt2 & operator -=(VectorInt2 const & _a) { x -= _a.x; y -= _a.y; return *this; }
	inline VectorInt2 operator *(int32 const & _a) const { return VectorInt2(x * _a, y * _a); }
	inline VectorInt2 & operator *=(int32 const & _a) { x *= _a; y *= _a; return *this; }
	inline VectorInt2 operator *(VectorInt2 const & _a) const { return VectorInt2(x * _a.x, y * _a.y); }
	inline VectorInt2 & operator *=(VectorInt2 const & _a) { x *= _a.x; y *= _a.y; return *this; }
	inline VectorInt2 operator /(VectorInt2 const & _a) const { return VectorInt2(x / _a.x, y / _a.y); }
	inline VectorInt2 & operator /=(VectorInt2 const & _a) { x /= _a.x; y /= _a.y; return *this; }
	inline VectorInt2 operator /(int _a) const { return VectorInt2(x / _a, y / _a); }
	inline VectorInt2 & operator /=(int & _a) { x /= _a; y /= _a; return *this; }
	inline VectorInt2 operator *(Vector2 const & _a) const { return VectorInt2((int32)((float)x * _a.x), (int32)((float)y * _a.y)); }
	inline VectorInt2 & operator *=(Vector2 const & _a) { x = (int32)((float)x * _a.x); y = (int32)((float)y * _a.y); return *this; }
	inline VectorInt2 operator *(float const & _a) const { return VectorInt2((int32)((float)x * _a), (int32)((float)y * _a)); }
	inline VectorInt2 & operator *=(float const & _a) { x = (int32)((float)x * _a); y = (int32)((float)y * _a); return *this; }
	
	inline static int dot(VectorInt2 const & _a, VectorInt2 const & _b) { return _a.x * _b.x + _a.y * _b.y; }

	inline bool operator ==(VectorInt2 const & _a) const { return x == _a.x && y == _a.y; }
	inline bool operator !=(VectorInt2 const & _a) const { return x != _a.x || y != _a.y; }

	inline bool is_zero() const { return x == 0 && y == 0; }

	bool serialise(Serialiser & _serialiser);
};

inline VectorInt2 operator *(int32 const & _a, VectorInt2 const &_b) { return VectorInt2(_b.x * _a, _b.y * _a); }
inline VectorInt2 operator *(float const & _a, VectorInt2 const &_b) { return VectorInt2((int32)((float)_b.x * _a), (int32)((float)_b.y * _a)); }

template <> bool Optional<VectorInt2>::load_from_xml(IO::XML::Node const* _node);
template <> bool Optional<VectorInt2>::load_from_xml(IO::XML::Attribute const* _attr);
