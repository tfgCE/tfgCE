template <typename Value>
void BlendAdvancer<Value>::advance(float _deltaTime)
{
	float prev = timeLeft;
	timeLeft = max(0.0f, timeLeft - _deltaTime);
	if (prev != 0.0f && timeLeft == 0.0f &&
		on_reached_time)
	{
		on_reached_time();
		prev = timeLeft;
	}
	float useTarget = prev != 0.0f ? clamp(1.0f - timeLeft / prev, 0.0f, 1.0f) : 1.0f;
	curr = lerp(useTarget, curr, target);
}

template <typename Value>
void BlendAdvancer<Value>::init()
{
	if (on_reached_time)
	{
		on_reached_time();
	}
	curr = target;
}

template <typename Value>
void BlendAdvancer<Value>::init(Value const& _t)
{
	target = _t;
	curr = _t;
}

template <typename Value>
void BlendAdvancer<Value>::set_target(Value const& _t, float _timeLeft)
{
	target = _t;
	timeLeft = _timeLeft;
}
