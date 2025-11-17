#pragma once

#include "..\globalDefinitions.h"

#include <math.h>

#ifdef AN_CLANG
#include "..\debug\debug.h"
#include "..\memory\memory.h"
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

struct String;
struct Vector2;
template <typename Point> struct BezierCurve;

float find_closest_line_curve_point(Vector2 const & _p, Vector2 const & _dir, BezierCurve<Vector2> const & _curve, float _step = SMALL_STEP);

float remap_value(float _value, float _from0, float _from1, float _to0, float _to1, bool _clamp = false);

template <typename Type>
inline String to_string(Type const& _v); // include mathToString.h

template <typename Type>
inline Type replace_negative_zero_positive(Type const & _value, Type const & _onNegative, Type const & _onZero, Type const & _onPositive)
{
	return _value > (Type)0? _onPositive : (_value == (Type)0 ? _onZero : _onNegative);
}

template <typename Type>
inline Type calc_pt(Type const & _0, Type const & _1, Type const & _v)
{
	an_assert((_1 - _0) != 0);
	return (_v - _0) / (_1 - _0);
}

template <typename Type>
inline Type lerp(float _pt, Type const & _0, Type const & _1)
{
	return _0 + (_1 - _0) * _pt;
}

template <typename Type>
inline Type abs(Type const _a)
{
	return (Type)(_a < (Type)0 ? -_a : _a);
}

template <typename Type>
inline Type sign(Type const _a)
{
	return (Type)(_a < (Type)0 ? (Type)-1 : (Type)1);
}

template <typename Type>
inline Type pow(Type const _base, Type const _power)
{
	return (Type)::pow((double)_base, (double)_power);
}

template <typename Type>
inline Type normal(Type const & _a)
{
	todo_important(TXT("specialise"));
	return (Type)0;
}

template <typename Type>
inline Type exp(Type const & _a)
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type exp2(Type const & _a)
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type log(Type const & _a)
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type log2(Type const & _a)
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type round(Type const & _a)
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type ceil(Type const & _a)  // -3.5 should give -3
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type ceil_from_zero(Type const & _a)  // -3.5 should give -4
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type floor(Type const & _a) // -3.5 should give -4
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type floor_to_zero(Type const & _a) // -3.5 should give -3
{
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type fract(Type const& _a)  // -3.2 should give 0.8
{
	return _a - ceil(_a);
}

template <typename Type>
inline Type step(Type const & _edge, Type const& _a)  // -3.2 should give 0.8
{
	return _a < _edge? (Type)0 : (Type)1;
}

template <typename Type>
inline Type is_almost_zero(Type const _a, Type const _epsilon = 0.0001f)
{
	return _a >= -_epsilon && _a <= _epsilon;
}

template <typename Type>
inline bool almost_equal(Type const _a, Type const _b, Type const _epsilon = 0.0001f)
{
	return is_almost_zero(_b - _a, _epsilon);
}

template <typename Type>
inline Type sane_zero(Type const _a, Type const _epsilon = 0.000001f)
{
	return is_almost_zero(_a, _epsilon)? 0.0f : _a;
}

template <typename Type>
inline Type min(Type const _a, Type const _b)
{
	return _a < _b ? _a : _b;
}

template <typename Type>
inline Type max(Type const _a, Type const _b)
{
	return _a < _b ? _b : _a;
}

template <typename Type>
Type reduce_to_zero(Type const _value, Type const _threshold)
{
	if (_value >= 0) return max<Type>(0, _value - _threshold);
	else return min<Type>(0, _value + _threshold);
}

template <typename Type>
inline Type clamp(Type const _a, Type const _min, Type const _max)
{
	return _a < _min? _min : (_a > _max? _max : _a);
}

template <typename Type>
inline Type safe_inv(Type const _a)
{
	return _a != (Type)0? (Type)1 / _a : (Type)0;
}

template <typename Type>
inline Type sqrt(Type const _a)
{
	return (Type)::sqrt((double)_a);
}

template <typename Type>
inline Type inv_sqrt(Type const _a)
{
	return (Type)1 / (Type)::sqrt((double)_a);
}

template <typename Type>
inline Type sqr(Type const _a)
{
	return _a * _a;
}

template <typename Type>
inline Type cbc(Type const _a)
{
	return _a * _a * _a;
}

template <typename Type>
inline Type half_of(Type const _value)
{
	return _value * 0.5f;
}

template <typename Type>
inline Type mod_raw(Type const _a, Type const _b)
{
	// this will go down to 0 from both sides, -1 % 4 will be -1
	// (not that very useful basically it should be used as basic function for actual mod)
	todo_important(TXT("specialise"));
	return _a;
}

template <typename Type>
inline Type mod(Type const _a, Type const _b)
{
	// this will go down as whole thing is continuous:
	//	 . . . . . 
	//	...........
	//	 5 % 4 = 1
	//	 4 % 4 = 0
	//	 3 % 4 = 3
	//	 2 % 4 = 2
	//	 1 % 4 = 1
	//	 0 % 4 = 0
	//	-1 % 4 = 3
	//	-2 % 4 = 2
	//	-3 % 4 = 1
	//	-4 % 4 = 0
	//	-4 % 4 = 3
	//	...........
	//	 . . . . . 
	// specialise if really needed
	if (_a >= (Type)0)
	{
		return mod_raw(_a, _b);
	}
	else
	{
		Type rest = mod_raw(-_a, _b);
		if (rest == (Type)0)
		{
			return (Type)0;
		}
		else
		{
			return _b - rest;
		}
	}
}

template <typename Type>
inline Type mod_halves(Type const _a, Type const _b)
{
	Type modded = mod(_a, _b);
	Type half = _b / (Type)2;
	while (modded > half)
	{
		modded -= _b;
	}
	while (modded < -half)
	{
		modded += _b;
	}
	return modded;
}

template <typename Type>
inline Type round_to(Type const & _value, Type const & _round)
{
	Type roundHalf = half_of(_round);
	Type change = mod(_value + roundHalf, _round) - roundHalf;
	return _value - change;
}

template <typename Type>
inline Type round_down_to(Type const & _value, Type const & _round)
{
	Type change = mod(_value, _round);
	return _value - change;
}

template <typename Type>
inline Type round_up_to(Type const & _value, Type const & _round)
{
	Type change = mod(_value, _round);
	return _value + _round - change;
}

template <typename Type>
inline Type floor_to(Type const& _value, Type const& _round)
{
	Type change = mod(_value, _round);
	return (_value - change);
}

template <typename Type>
inline Type ceil_to(Type const& _value, Type const& _round)
{
	Type change = mod(_value, _round);
	if (change != (Type)0)
	{
		change = _round - change;
	}
	return (_value + change);
}

template <typename Type>
inline Type floor_by(Type const& _value, Type const& _round)
{
	Type change = mod(_value, _round);
	return (_value - change) / _round;
}

template <typename Type>
inline Type ceil_by(Type const& _value, Type const& _round)
{
	Type change = mod(_value, _round);
	if (change != (Type)0)
	{
		change = _round - change;
	}
	return (_value + change) / _round;
}

template <typename Type>
inline void swap(Type & _a, Type & _b)
{
	Type temp = _a;
	_a = _b;
	_b = temp;
}

template <typename Type>
inline void swap_mem(Type & _a, Type & _b)
{
	uint32 size = sizeof(Type);
	void * temp = allocate_stack(size);
	memory_copy(temp, &_a, size);
	memory_copy(&_a, &_b, size);
	memory_copy(&_b, temp, size);
}

template <typename Type>
inline Type pi()
{
	return (Type)3.141592653589793238462643383279f;
}

template <typename Type>
inline Type relative_angle(Type const _a)
{
	Type a = mod(_a, 360.0f);
	return a > 180.0f ? a - 360.0f : a;
}

template <typename Type>
inline Type degree_to_radian(Type const _a)
{
	return _a * pi<Type>() / 180.0f;
}

template <typename Type>
inline Type radian_to_degree(Type const _a)
{
	return _a * 180.0f / pi<Type>();
}

template <typename Type>
inline Type sin_deg(DEG_ Type const _a)
{
	return (Type)::sin(degree_to_radian(_a));
}

template <typename Type>
inline Type cos_deg(DEG_ Type const _a)
{
	return (Type)::cos(degree_to_radian(_a));
}

template <typename Type>
inline Type tan_deg(DEG_ Type const _a)
{
	return (Type)::tan(degree_to_radian(_a));
}

template <typename Type>
inline DEG_ Type atan_deg(Type const _a)
{
	return radian_to_degree((Type)::atan(_a));
}

template <typename Type>
inline Type sin_rad(RAD_ Type const _a)
{
	return (Type)::sin(_a);
}

template <typename Type>
inline Type cos_rad(RAD_ Type const _a)
{
	return (Type)::cos(_a);
}

template <typename Type>
inline RAD_ Type asin_rad(Type const _a)
{
	return (Type)::asinf(_a);
}

template <typename Type>
inline DEG_ Type asin_deg(Type const _a)
{
	return radian_to_degree((Type)::asinf(_a));
}

template <typename Type>
inline RAD_ Type acos_rad(Type const _a)
{
	return (Type)::acosf(_a);
}

template <typename Type>
inline DEG_ Type acos_deg(Type const _a)
{
	return radian_to_degree((Type)::acosf(_a));
}

template <typename Type>
inline RAD_ Type atan_rad(Type const _a)
{
	return (Type)::atanf(_a);
}

template <typename Type>
inline RAD_ Type atan2_rad(Type const _a, Type const _b)
{
	return (Type)::atan2f(_a, _b);
}

template <typename Type>
inline void order(Type & _a, Type & _b)
{
	if (_a > _b)
	{
		swap(_a, _b);
	}
}

template <typename Type>
inline void order(Type & _a, Type & _b, Type & _c)
{
	if (_a > _b)
	{
		// ? b ? a ?
		if (_b > _c)
		{
			// c b a
			swap(_a, _c);
		}
		else
		{
			// b ? ?
			if (_a > _c)
			{
				// b c a (worst case)
				swap(_a, _b);
				swap(_b, _c);
			}
			else
			{
				// b a c
				swap(_a, _b);
			}
		}
	}
	else
	{
		// a ? ?
		if (_b > _c)
		{
			// a c b
			swap(_b, _c);
		}
		else
		{
			// a b c
		}
	}
}

template <typename Type>
inline float length_of(Type const & _a)
{
	return abs((float)_a);
}

template <typename Type>
inline Type normal_of(Type const & _a)
{
	float length = length_of(_a);
	return length != 0.0f ? _a * ((Type)1 / length) : _a;
}

template <typename Type>
inline float length_squared_of(Type const & _a)
{
	return sqr(length_of(_a));
}

template <typename Type>
inline float dot_product(Type const & _a, Type const & _b)
{
	return (float)(_a * _b);
}


// specialisations

template <>
inline float exp<float>(float const & _a)
{
	return ::expf(_a);
}

template <>
inline double exp<double>(double const & _a)
{
	return ::exp(_a);
}

template <>
inline float exp2<float>(float const & _a)
{
	return ::exp2f(_a);
}

template <>
inline double exp2<double>(double const & _a)
{
	return ::exp2(_a);
}
template <>
inline float log<float>(float const& _a)
{
	return ::logf(_a);
}

template <>
inline double log<double>(double const& _a)
{
	return ::log(_a);
}

template <>
inline float log2<float>(float const& _a)
{
	return ::log2f(_a);
}

template <>
inline double log2<double>(double const& _a)
{
	return ::log2(_a);
}

template <>
inline float round<float>(float const & _a)
{
	return ::roundf(_a);
}

template <>
inline double round<double>(double const & _a)
{
	return ::lround(_a);
}

template <>
inline float ceil<float>(float const & _a)
{
	return ::ceilf(_a);
}

template <>
inline double ceil<double>(double const & _a)
{
	return ::ceil(_a);
}

template <>
inline float floor<float>(float const & _a)
{
	return (float)::floor((double)_a);
}

template <>
inline double floor<double>(double const & _a)
{
	return ::floor((double)_a);
}

template <>
inline float ceil_from_zero<float>(float const & _a)
{
	return sign(_a) * ::ceil(abs(_a));
}

template <>
inline double ceil_from_zero<double>(double const & _a)
{
	return sign(_a) * ::ceil(abs(_a));
}

template <>
inline float floor_to_zero<float>(float const & _a)
{
	return sign(_a) * ::floor(abs(_a));
}

template <>
inline double floor_to_zero<double>(double const & _a)
{
	return sign(_a) * ::floor(abs(_a));
}

template <AN_NOT_CLANG_TYPENAME	int>
inline int half_of(int const _value)
{
	return _value / 2;
}

#ifndef AN_CLANG
template <>
inline int half_of(int const _value)
{
	return _value / 2;
}
#endif

template <>
inline int mod_raw<int>(int const _a, int const _b)
{
	return _a % _b;
}

template <>
inline float mod_raw<float>(float const _a, float const _b)
{
	return (float)::fmod(_a, _b);
}

template <>
inline double mod_raw<double>(double const _a, double const _b)
{
	return (float)::fmod(_a, _b);
}

template <>
inline float mod<float>(float const _a, float const _b)
{
	return _a >= 0.0f ? mod_raw(_a, _b) : _b - mod_raw(-_a, _b);
}

template <>
inline double mod<double>(double const _a, double const _b)
{
	return _a >= 0.0f ? mod_raw(_a, _b) : _b - mod_raw(-_a, _b);
}
