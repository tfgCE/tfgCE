#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

template <>
inline bool load_value_from_xml<float>(REF_ float & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a = _node->get_float_attribute(_valueName, _a);
			return true;
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<int>(REF_ int & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a = _node->get_int_attribute(_valueName, _a);
			return true;
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<bool>(REF_ bool & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a = _node->get_bool_attribute(_valueName, _a);
			return true;
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<Range>(REF_ Range & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a.load_from_xml(_node, _valueName);
			return true;
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<RangeInt>(REF_ RangeInt & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a.load_from_xml(_node, _valueName);
			return true;
		}
	}
	return false;
}

template <>
inline Vector2 ceil<Vector2>(Vector2 const & _a)
{
	return Vector2(ceil(_a.x), ceil(_a.y));
}

template <>
inline Vector3 ceil<Vector3>(Vector3 const & _a)
{
	return Vector3(ceil(_a.x), ceil(_a.y), ceil(_a.z));
}

template <>
inline Vector2 floor<Vector2>(Vector2 const & _a)
{
	return Vector2(floor(_a.x), floor(_a.y));
}

template <>
inline Vector3 floor<Vector3>(Vector3 const & _a)
{
	return Vector3(floor(_a.x), floor(_a.y), floor(_a.z));
}

template <>
inline Vector2 fract<Vector2>(Vector2 const& _a)
{
	return Vector2(fract(_a.x), fract(_a.y));
}

template <>
inline Vector3 fract<Vector3>(Vector3 const& _a)
{
	return Vector3(fract(_a.x), fract(_a.y), fract(_a.z));
}

template <>
inline Vector2 zero<Vector2>()
{
	return Vector2::zero;
}

template <>
inline Vector3 zero<Vector3>()
{
	return Vector3::zero;
}

template <>
inline Vector4 zero<Vector4>()
{
	return Vector4::zero;
}

template <>
inline Plane zero<Plane>()
{
	return Plane::zero;
}

template <>
inline Rotator3 zero<Rotator3>()
{
	return Rotator3::zero;
}

template <>
inline Transform zero<Transform>()
{
	return Transform::zero;
}

template <>
inline Range2 zero<Range2>()
{
	return Range2::zero;
}

template <>
inline Range3 zero<Range3>()
{
	return Range3::zero;
}

template <>
inline Sphere zero<Sphere>()
{
	return Sphere::zero;
}

template <>
inline Transform default_value<Transform>()
{
	return Transform::identity;
}

template <>
inline Vector2 round<Vector2>(Vector2 const & _a)
{
	return Vector2(round(_a.x), round(_a.y));
}

template <>
inline Vector2 abs<Vector2>(Vector2 const _a)
{
	return (Vector2(abs(_a.x), abs(_a.y)));
}

template <>
inline Vector2 normal<Vector2>(Vector2 const & _a)
{
	return _a.normal();
}

template <>
inline Vector3 normal<Vector3>(Vector3 const & _a)
{
	return _a.normal();
}

template <>
inline float length_of<Vector2>(Vector2 const & _a)
{
	return _a.length();
}
//
template <>
inline float length_squared_of<Vector2>(Vector2 const & _a)
{
	return _a.length_squared();
}
//
template <>
inline float dot_product<Vector2>(Vector2 const & _a, Vector2 const & _b)
{
	return Vector2::dot(_a, _b);
}

template <>
inline float length_of<Vector3>(Vector3 const & _a)
{
	return _a.length();
}
//
template <>
inline float length_squared_of<Vector3>(Vector3 const & _a)
{
	return _a.length_squared();
}
//
template <>
inline float dot_product<Vector3>(Vector3 const & _a, Vector3 const & _b)
{
	return Vector3::dot(_a, _b);
}

template <>
inline float length_of<Range>(Range const & _a)
{
	return _a.length();
}

template <>
inline int lerp<int>(float _t, int const& _a, int const& _b)
{
	float l = round(lerp<float>(_t, (float)_a, (float)_b));
	if (l >= 0.0f)
	{
		return (int)(l + 0.01f);
	}
	else
	{
		l = -l;
		return -(int)(l + 0.01f);
	}
}

template <>
inline Range2 lerp<Range2>(float _t, Range2 const& _a, Range2 const& _b)
{
	return Range2::lerp(_t, _a, _b);
}

template <>
inline Range3 lerp<Range3>(float _t, Range3 const& _a, Range3 const& _b)
{
	return Range3::lerp(_t, _a, _b);
}

template <>
inline Transform lerp<Transform>(float _t, Transform const& _a, Transform const& _b)
{
	return Transform::lerp(_t, _a, _b);
}

template <>
inline Vector3 lerp<Vector3>(float _t, Vector3 const& _a, Vector3 const& _b)
{
	return Vector3::lerp(_t, _a, _b);
}

template <>
inline Rotator3 lerp<Rotator3>(float _t, Rotator3 const& _a, Rotator3 const& _b)
{
	return Rotator3::lerp(_t, _a, _b);
}
