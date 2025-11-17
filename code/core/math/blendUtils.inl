template <typename Value>
Value blend_to_using_time(Value const & _from, Value const & _to, float const _blendTime, float const _deltaTime)
{
	float atTarget = _blendTime != 0.0f ? clamp(_deltaTime / _blendTime, 0.0f, 1.0f) : 1.0f;
	return lerp(atTarget, _from, _to);
}

template <typename Value>
Value blend_to_using_speed(Value const & _from, Value const & _to, float const _speed, float const _deltaTime)
{
	float maxChangeThisFrame = _speed * _deltaTime;
	return _from + clamp(_to - _from, -maxChangeThisFrame, maxChangeThisFrame);
}

template <typename Value>
Value blend_to_using_speed_based_on_time(Value const & _from, Value const & _to, float const _blendTime, float const _deltaTime)
{
	if (_blendTime == 0.0f) { return _to; }
	float maxChangeThisFrame = ((Value)1) * _deltaTime / _blendTime;
	return _from + clamp(_to - _from, -maxChangeThisFrame, maxChangeThisFrame);
}

//

template <>
inline Vector3 blend_to_using_speed(Vector3 const & _from, Vector3 const & _to, float const _speed, float const _deltaTime)
{
	if (_from == _to)
	{
		return _to;
	}
	else
	{
		Vector3 const f2t = _to - _from;
		float const maxChangeThisFrame = _speed * _deltaTime;
		float const dist = f2t.length();
		return _from + f2t.normal() * min(dist, maxChangeThisFrame);
	}
}
