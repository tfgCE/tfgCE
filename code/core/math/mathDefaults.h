#pragma once

#include "..\defaults.h"

template <>
inline void set_to_default<Vector3>(Vector3 & _object)
{
	_object = Vector3::zero;
}

template <>
inline void set_to_default<Transform>(Transform & _object)
{
	_object = Transform::identity;
}
