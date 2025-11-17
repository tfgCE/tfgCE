namespace BlendCurve
{
	float linear(float _a)
	{
		return _a;
	}

	float cubic(float _a)
	{
		_a = (_a - 0.5f) * 2.0f;
		float const signA = sign(_a);
		_a = 1.0f - abs(_a);
		_a = sqr(_a);
		_a = (1.0f - _a) * signA;
		_a = _a * 0.5f + 0.5f;
		return _a;
	}
	
	float quadratic(float _a)
	{
		_a = (_a - 0.5f) * 2.0f;
		float const signA = sign(_a);
		_a = 1.0f - abs(_a);
		_a = sqr(_a);
		_a = sqr(_a);
		_a = (1.0f - _a) * signA;
		_a = _a * 0.5f + 0.5f;
		return _a;
	}
	
	float cubic_in(float _a)
	{
		float const signA = sign(_a);
		_a = abs(_a);
		_a = sqr(_a) * signA;
		return _a;
	}

	float cubic_out(float _a)
	{
		float const signA = sign(_a);
		_a = 1.0f - abs(_a);
		_a = sqr(_a);
		_a = (1.0f - _a) * signA;
		return _a;
	}

	float sinus(float _a)
	{
		_a = (_a - 0.5f) * 2.0f;
		_a = sin_deg(90.0f * _a);
		_a = _a * 0.5f + 0.5f;
		return _a;
	}
};
