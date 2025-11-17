#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Vector2
{
public:
	float x;
	float y;

	static Vector2 const zero;
	static Vector2 const one;
	static Vector2 const half;
	static Vector2 const xAxis;
	static Vector2 const yAxis;

	inline Vector2() {}
	inline explicit Vector2(float _v): x(_v), y(_v) {}
	inline Vector2(float _x, float _y): x(_x), y(_y) {}
	//TODO inline Vector2(Rotator2 const & _rot);

	float & access_element(uint _e) { an_assert(_e <= 1); return _e == 0 ? x : y; }
	float get_element(uint _e) const { an_assert(_e <= 1); return _e == 0 ? x : y; }

	inline String to_string() const;
	inline String to_loadable_string() const;
	inline VectorInt2 to_vector_int2() const;
	inline VectorInt2 to_vector_int2_cells() const;
	inline Vector3 to_vector3(float _newZ = 0.0f) const;
	inline Vector3 to_vector3_xz(float _newY = 0.0f) const;

	//TODO public Rotator2 asRotator { get { return Rotator2.from_vector2(this); } } 

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), bool _keepExistingValues = false);
	bool load_from_xml(IO::XML::Node const* _node, bool _keepExistingValues) { return load_from_xml(_node, TXT("x"), TXT("y"), _keepExistingValues); }
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y"), bool _keepExistingValues = false);
	bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childNode, bool _keepExistingValues) { return load_from_xml_child_node(_node, _childNode, TXT("x"), TXT("y"), _keepExistingValues); }
	bool load_from_string(String const & _string);

	bool save_to_xml(IO::XML::Node * _node, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y")) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _xAttr = TXT("x"), tchar const * _yAttr = TXT("y")) const;

	inline Vector2 inverted() const { return Vector2(x != 0.0f? 1.0f / x : 0.0f, y!= 0.0f? 1.0f / y : 0.0f); }

	inline bool operator ==(Vector2 const & _a) const { return x == _a.x && y == _a.y; }
	inline bool operator !=(Vector2 const & _a) const { return x != _a.x || y != _a.y; }

	inline Vector2 operator -() const { return Vector2(-x, -y); }
	inline Vector2 operator +(Vector2 const & _a) const { return Vector2(x + _a.x, y + _a.y); }
	inline Vector2 & operator +=(Vector2 const & _a) { x += _a.x; y += _a.y; return *this; }
	inline Vector2 operator -(Vector2 const & _a) const { return Vector2(x - _a.x, y - _a.y); }
	inline Vector2 & operator -=(Vector2 const & _a) { x -= _a.x; y -= _a.y; return *this; }
	inline Vector2 operator *(Vector2 const & _a) const { return Vector2(x * _a.x, y * _a.y); }
	inline Vector2 & operator *=(Vector2 const & _a) { x *= _a.x; y *= _a.y; return *this; }
	inline Vector2 operator /(Vector2 const & _a) const { return Vector2(x / _a.x, y / _a.y); }
	inline Vector2 & operator /=(Vector2 const & _a) { x /= _a.x; y /= _a.y; return *this; }

	inline Vector2 operator *(float const & _a) const { return Vector2(x * _a, y * _a); }
	inline Vector2 & operator *=(float const & _a) { x *= _a; y *= _a; return *this; }
	inline Vector2 operator /(float const & _a) const { return Vector2(x / _a, y / _a); }
	inline Vector2 & operator /=(float const & _a) { x /= _a; y /= _a; return *this; }

	inline bool is_zero() const { return x == 0.0f && y == 0.0f; }
	inline bool is_almost_zero(float const _epsilon = 0.0001f) const { return abs(x) <= _epsilon && abs(y) <= _epsilon; }
	inline bool is_one() const { return x == 1.0f && y == 1.0f; }

	inline static float dot(Vector2 const & _a, Vector2 const & _b) { return _a.x * _b.x + _a.y * _b.y; }
	inline static float dot_perp(Vector2 const & _a, Vector2 const & _b) { return dot(_a, _b.rotated_right()); }

	inline static float distance(Vector2 const & _a, Vector2 const & _b) { return sqrt<float>(distance_squared(_a, _b)); }
	inline static float distance_squared(Vector2 const & _a, Vector2 const & _b) { return (sqr(_a.x - _b.x) + sqr(_a.y - _b.y)); }

	inline float length() const { return sqrt<float>(length_squared()); }
	inline float length_squared() const { return (sqr(x) + sqr(y)); }
		
	inline bool is_normalised() const;
	inline void normalise();
	inline Vector2 normal() const;

	inline Vector2 normal_in_square() const; // x or y has to be 1 or -1

	inline void rotate_right() { float temp = x; x = y; y = -temp; }
	inline Vector2 rotated_right() const { return Vector2(y, -x); }
	inline Vector2 axis_aligned() const { return abs(x) > abs(y)? Vector2::xAxis * sign(x) : Vector2::yAxis * sign(y); }
	inline bool is_axis_aligned(float _threshold = 0.001f) const { return abs(x) <= _threshold || abs(y) <= _threshold; }

	inline Vector2 drop_using(Vector2 const& _alongDir) const; // will be dropped onto plane perpendicular to alongDir
	inline Vector2 drop_using_normalised(Vector2 const& _alongDir) const; // will be dropped onto plane perpendicular to alongDir

	inline float angle() const;
	inline void rotate_by_angle(float _angle);
	inline Vector2 rotated_by_angle(float _angle) const;
	//TODO inline void rotate_by(Rotator2 _rotator);
	//TODO public Vector2 rotated_by(Rotator2 _rotator)

	bool serialise(Serialiser & _serialiser);

	// direction from a, direction from b in which lines go
	static bool calc_intersection(Vector2 const & _a, Vector2 const & _aDir, Vector2 const & _b, Vector2 const & _bDir, OUT_ Vector2 & _result);

	inline void maximise_off_zero(Vector2 const& _other); // if other is further from zero, its value will be used
};

inline Vector2 operator *(float const & _a, Vector2 const &_b) { return Vector2(_b.x * _a, _b.y * _a); }

template <> bool Optional<Vector2>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Vector2>::load_from_xml(IO::XML::Attribute const * _attr);
