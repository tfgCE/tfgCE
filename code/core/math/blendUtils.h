#pragma once

#include "math.h"

// blends

template <typename Value>
Value blend_to_using_time(Value const & _from, Value const & _to, float const _blendTime, float const _deltaTime);

template <typename Value>
Value blend_to_using_speed(Value const & _from, Value const & _to, float const _speed, float const _deltaTime);

// blendTime is for changing by 1 (eg. 0 to 1)
template <typename Value>
Value blend_to_using_speed_based_on_time(Value const & _from, Value const & _to, float const _blendTime, float const _deltaTime);
