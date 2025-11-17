#pragma once

#include "math.h"

#include <functional>

// blend advancer

template <typename Value>
struct BlendAdvancer
{
	Value curr;
	Value target;
	float timeLeft = 0.0f;
	std::function<void()> on_reached_time;
	void advance(float _deltaTime);
	void init();
	void init(Value const & _t);
	void set_target(Value const & _t, float _timeLeft);
};
