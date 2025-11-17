Colour::Colour(float _r, float _g, float _b, float _a)
: r( _r )
, g( _g )
, b( _b )
, a( _a )
{
}

void Colour::blend_to(Colour const & _other, float _useTarget)
{
	_useTarget = clamp(_useTarget, 0.0f, 1.0f);
	float useOld = 1.0f - _useTarget;
	r = r * useOld + _other.r * _useTarget;
	g = g * useOld + _other.g * _useTarget;
	b = b * useOld + _other.b * _useTarget;
	a = a * useOld + _other.a * _useTarget;
}

Colour Colour::blended_to(Colour const & _other, float _useTarget) const
{
	Colour result = *this;
	result.blend_to(_other, _useTarget);
	return result;
}

Colour Colour::lerp(float _bWeight, Colour const & _a, Colour const & _b)
{
	return _a * (1.0f - _bWeight) + _bWeight * _b;
}

template <>
inline void set_to_default<Colour>(Colour & _object)
{
	_object = Colour::black;
}
