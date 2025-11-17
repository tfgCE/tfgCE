#pragma once

#include "..\defaults.h"
#include "..\math\mathUtils.h"
#include "..\other\registeredType.h"

struct Name;

struct Vector3;
struct Vector4;

namespace IO
{
	namespace XML
	{
		class Attribute;
		class Node;
	};
};

struct Colour
{
public:
	static Colour const none;
	static Colour const full;

	static Colour black;
	static Colour blackCold;
	static Colour blackWarm;
	static Colour grey;
	static Colour greyLight;
	static Colour white;
	static Colour whiteCold;
	static Colour whiteWarm;
	static Colour red;
	static Colour green;
	static Colour greenDark;
	static Colour blue;
	static Colour purple;
	static Colour magenta;
	static Colour yellow;
	static Colour orange;
	static Colour cyan;
	static Colour mint;
	static Colour gold;

	static Colour zxBlack;
	static Colour zxBlue;
	static Colour zxRed;
	static Colour zxMagenta;
	static Colour zxGreen;
	static Colour zxCyan;
	static Colour zxYellow;
	static Colour zxWhite;
	//
	static Colour zxBlackBright;
	static Colour zxBlueBright;
	static Colour zxRedBright;
	static Colour zxMagentaBright;
	static Colour zxGreenBright;
	static Colour zxCyanBright;
	static Colour zxYellowBright;
	static Colour zxWhiteBright;

	static Colour c64Black;
	static Colour c64White;
	static Colour c64Red;
	static Colour c64Cyan;
	static Colour c64Violet;
	static Colour c64Green;
	static Colour c64Blue;
	static Colour c64Yellow;
	static Colour c64Orange;
	static Colour c64Brown;
	static Colour c64LightRed;
	static Colour c64Grey1;
	static Colour c64Grey2;
	static Colour c64LightGreen;
	static Colour c64LightBlue;
	static Colour c64Grey3;

	static Colour random_rgb();

	float r;
	float g;
	float b;
	float a;

	inline Colour() {}
	Colour(Vector3 const & _rgb, float _a = 1.0f);
	Colour(Vector4 const & _rgba);
	inline Colour(float _r, float _g, float _b, float _a = 1.0f);
	static Colour alpha(float _a) { return Colour(1.0f, 1.0f, 1.0f, _a); }

	bool is_none() const { return *this == none; }

	String to_string() const;
	Vector3 to_vector3() const;
	Vector4 to_vector4() const;

	Colour with_alpha(float _a) const { return Colour(r, g, b, a * _a); }
	Colour with_set_alpha(float _a) const { return Colour(r, g, b, _a); }

	float & access_element(uint _e) { an_assert(_e <= 3); return _e == 0 ? r : (_e == 1 ? g : (_e == 2 ? b : a)); }

	void set_rgba(float _r, float _g, float _b, float _a);

	inline int32 get_int_r() const { return clamp<int32>((int32)r, 0, 255); }
	inline int32 get_int_g() const { return clamp<int32>((int32)g, 0, 255); }
	inline int32 get_int_b() const { return clamp<int32>((int32)b, 0, 255); }
	inline int32 get_int_a() const { return clamp<int32>((int32)a, 0, 255); }

	inline float get_length() const { return sqrt(r * r + g * g + b * b + a * a); }
	inline float get_length_squared() const { return r * r + g * g + b * b + a * a; }

	inline bool operator ==(Colour const & _a) const { return r == _a.r && b == _a.b && g == _a.g && a == _a.a; }
	inline bool operator !=(Colour const & _a) const { return r != _a.r || b != _a.b || g != _a.g || a != _a.a; }

	inline Colour add_to_rgb(float _a) const { return Colour(r + _a, g + _a, b + _a, a ); }
	inline Colour mul_rgb(float _a) const { return Colour(r * _a, g * _a, b * _a, a); }
	inline Colour operator +(Colour const & _a) const { return Colour(r + _a.r, g + _a.g, b + _a.b, min(1.0f, a + _a.a)); }
	inline Colour & operator +=(Colour const & _a) { r += _a.r; g += _a.g; b += _a.b; a = min(1.0f, a + _a.a); return *this; }
	inline Colour operator -(Colour const & _a) const { return Colour(r - _a.r, g - _a.g, b - _a.b, min(1.0f, a - _a.a)); }
	inline Colour & operator -=(Colour const & _a) { r -= _a.r; g -= _a.g; b -= _a.b; a = min(1.0f, a - _a.a); return *this; }
	inline Colour operator *(Colour const & _a) const { return Colour(r * _a.r, g * _a.g, b * _a.b, min(1.0f, a * _a.a)); }
	inline Colour & operator *=(Colour const & _a) { r *= _a.r; g *= _a.g; b *= _a.b; a = min(1.0f, a * _a.a); return *this; }

	inline Colour operator *(float const & _a) const { return Colour(r * _a, g * _a, b * _a, a * _a); }
	inline Colour & operator *=(float const & _a) { r *= _a; g *= _a; b *= _a; a *= _a; return *this; }
	inline Colour operator /(float const & _a) const { return Colour(r / _a, g / _a, b / _a, a / _a); }
	inline Colour & operator /=(float const & _a) { r /= _a; g /= _a; b /= _a; a /= _a; return *this; }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * _rAttr = TXT("r"), tchar const * _gAttr = TXT("g"), tchar const * _bAttr = TXT("b"), tchar const * _aAttr = TXT("a"));
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _rAttr = TXT("r"), tchar const * _gAttr = TXT("g"), tchar const * _bAttr = TXT("b"), tchar const * _aAttr = TXT("a"));
	bool load_from_xml(IO::XML::Attribute const * _attr);
	bool load_from_xml_child_or_attr(IO::XML::Node const * _node, tchar const * _attrOrChild);
	bool load_from_string(String const & _string);
	bool parse_from_string(String const & _string); // just name
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _rAttr = TXT("r"), tchar const * _gAttr = TXT("g"), tchar const * _bAttr = TXT("b"), tchar const * _aAttr = TXT("a")) const;
	bool save_to_xml_attribute(IO::XML::Node * _node, tchar const * _attr) const;

	inline void blend_to(Colour const & _other, float _useTarget);
	inline Colour blended_to(Colour const & _other, float _useTarget) const;
	inline Colour inverted() const { return Colour(1.0f - r, 1.0f - g, 1.0f - b, a); }

	static inline Colour lerp(float _bWeight, Colour const & _a, Colour const & _b);
	static Colour lerp_smart(float _bWeight, Colour const & _a, Colour const & _b); // mixes rgb using alphas

private:
	bool parse_from_string_internal(String const & _string); // doesn't throw error

};

class RegisteredColours
{
public:
	static void initialise_static();
	static void close_static();

	static void register_colour(Name const & _name, Colour const & _colour);
	static Colour get_colour(Name const & _name);
	static Colour get_colour(String const & _name);
};

inline Colour operator *(float const & _a, Colour const &_b) { return Colour(_b.r * _a, _b.g * _a, _b.b * _a, _b.a * _a); }

#include "colour.inl"

#include "..\values.h"

template <>
inline Colour zero<Colour>()
{
	return Colour::none;
}

DECLARE_REGISTERED_TYPE(Colour);
