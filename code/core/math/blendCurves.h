#pragma once

#include "math.h"

namespace BlendCurve
{
	inline float linear(float _a);

	inline float cubic(float _a);
	inline float cubic_in(float _a);
	inline float cubic_out(float _a);

	inline float quadratic(float _a);

	inline float sinus(float _a);

	float apply(Name const & _type, float _a);
};
