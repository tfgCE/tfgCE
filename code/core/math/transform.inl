Transform::Transform()
#ifdef AN_DEVELOPMENT
: translation(Vector3(0.0f, 0.0f, 0.0f))
, scale(0.0f)
, orientation(Quat(0.0f, 0.0f, 0.0f, 0.0f))
#endif
{
}

Transform::Transform(Vector3 _translation, Quat _orientation, float _scale)
: translation(_translation)
, scale(_scale)
, orientation(_orientation)
{
	an_assert(is_ok() || is_zero() || is_acceptably_zero());
	//assert_slow(scale >= 0.0f);
}

Transform::Transform(Transform const& _other)
: translation(_other.translation)
, scale(_other.scale)
, orientation(_other.orientation)
{
	an_assert(_other.is_ok() || _other.is_zero() || _other.is_acceptably_zero());
	//assert_slow(scale >= 0.0f);
}

Transform& Transform::operator=(Transform const& _other)
{
	an_assert(_other.is_ok() || _other.is_zero() || _other.is_acceptably_zero());

	translation = _other.translation;
	scale = _other.scale;
	orientation = _other.orientation;

	return *this;
}

Vector3 Transform::location_to_world(Vector3 const & _a) const
{
	assert_slow(scale >= 0.0f);
	return vector_to_world(_a) + translation;
}

Vector3 Transform::location_to_local(Vector3 const & _a) const
{
	assert_slow(scale > 0.0f);
	return vector_to_local(_a - translation);
}

Vector3 Transform::vector_to_world(Vector3 const & _a) const
{
	assert_slow(scale >= 0.0f);
	return orientation.rotate(_a * scale);
}

Vector3 Transform::vector_to_local(Vector3 const & _a) const
{
	assert_slow(scale > 0.0f);
	return orientation.inverted_rotate(_a) / scale;
}

Transform Transform::to_world(Transform const & _a) const
{
	assert_slow(scale >= 0.0f);
	an_assert(orientation.is_normalised(), TXT("not normalised: %.10f (diff:%.10f) : %S"), orientation.length(), abs(orientation.length() - 1.0f), orientation.to_string().to_char());
	an_assert(_a.orientation.is_normalised(), TXT("not normalised: %.10f (diff:%.10f) : %S"), _a.orientation.length(), abs(_a.orientation.length() - 1.0f), _a.orientation.to_string().to_char());

	Transform result;
	result.orientation = orientation.to_world(_a.orientation);
	result.translation = location_to_world(_a.translation);
	result.scale = _a.scale * scale;
	an_assert(result.is_ok() || result.is_zero() || result.is_acceptably_zero());

	return result;
}

Transform Transform::to_local(Transform const & _a) const
{
	assert_slow(scale > 0.0f);

	Transform result;
	result.orientation = orientation.to_local(_a.orientation);
	result.translation = location_to_local(_a.translation);
	result.scale = _a.scale / scale;
	an_assert(result.is_ok() || result.is_zero() || result.is_acceptably_zero());

	return result;
}

Quat Transform::to_world(Quat const & _a) const
{
	assert_slow(scale > 0.0f);
	an_assert(orientation.is_normalised(), TXT("not normalised: %.3f : %S"), orientation.length(), orientation.to_string().to_char());
	an_assert(_a.is_normalised(), TXT("not normalised: %.3f : %S"), _a.length(), _a.to_string().to_char());

	return orientation.to_world(_a);
}

Quat Transform::to_local(Quat const & _a) const
{
	assert_slow(scale > 0.0f);

	return orientation.to_local(_a);
}

Plane Transform::to_world(Plane const & _a) const
{
	return Plane(vector_to_world(_a.get_normal()).normal(), location_to_world(_a.get_anchor()));
}

Plane Transform::to_local(Plane const & _a) const
{
	assert_slow(scale > 0.0f);

	return Plane(vector_to_local(_a.get_normal()).normal(), location_to_local(_a.get_anchor()));
}

Segment Transform::to_world(Segment const & _a) const
{
	assert_slow(scale > 0.0f);
	an_assert(orientation.is_normalised());

	Segment result(_a);
	result.start = location_to_world(result.start);
	result.end = location_to_world(result.end);
	result.startToEnd = vector_to_world(result.startToEnd);
	result.startToEndDir = vector_to_world(result.startToEndDir);
	result.boundingBoxCalculated = false;

	return result;
}

Segment Transform::to_local(Segment const & _a) const
{
	assert_slow(scale > 0.0f);
	an_assert(orientation.is_normalised());

	Segment result(_a);
	result.start = location_to_local(result.start);
	result.end = location_to_local(result.end);
	result.startToEnd = vector_to_local(result.startToEnd);
	result.startToEndDir = vector_to_local(result.startToEndDir);
	result.boundingBoxCalculated = false;

	return result;
}

void Transform::set_translation(Vector3 const & _a)
{
	an_assert(_a.is_ok());
	translation = _a;
	an_assert(is_ok());
}

void Transform::set_orientation(Quat const & _a)
{
	an_assert(_a.is_ok());
	orientation = _a;
	an_assert(is_ok());
}

void Transform::set_scale(float _a)
{
	an_assert(isfinite(_a));
	scale = _a;
	an_assert(is_ok());
}

void Transform::scale_translation(float _scale)
{
	translation *= _scale;
	an_assert(is_ok());
}

Vector3 Transform::get_axis(Axis::Type _axis) const
{
	if (_axis == Axis::X) return vector_to_world(Vector3::xAxis);
	if (_axis == Axis::Y) return vector_to_world(Vector3::yAxis);
	return vector_to_world(Vector3::zAxis);
}

Matrix44 Transform::to_matrix() const
{
	Matrix44 result;

	assert_slow(scale >= 0.0f);
	an_assert(orientation.is_normalised(), TXT("transform's orientation not normalised %.3f"), orientation.length());

	// scale should go to proper row
	orientation.fill_matrix33(result);

	if (abs(scale - 1.0f) > 0.00001f)
	{
		result.m00 *= scale;
		result.m01 *= scale;
		result.m02 *= scale;

		result.m10 *= scale;
		result.m11 *= scale;
		result.m12 *= scale;

		result.m20 *= scale;
		result.m21 *= scale;
		result.m22 *= scale;
	}

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	result.m30 = translation.x;
	result.m31 = translation.y;
	result.m32 = translation.z;

	return result;
}

Transform Transform::inverted() const
{
	assert_slow(scale > 0.0f);
	an_assert(abs(scale - 1.0f) < 0.00001f, TXT("may not work with other scale"));
	Transform ret;
	ret.orientation = orientation.inverted();
	ret.translation = orientation.to_local(-translation);
	ret.scale = 1.0f / scale;
	an_assert(ret.is_ok() || ret.is_zero() || ret.is_acceptably_zero());
	return ret;
}

Transform Transform::lerp(float _t, Transform const & _a, Transform const & _b)
{
	if (_t == 0.0f) return _a;
	if (_t == 1.0f) return _b;
	assert_slow(_a.scale > 0.0f);
	assert_slow(_b.scale > 0.0f);
	Transform result;
#ifdef AN_DEVELOPMENT
	result = Transform::identity;
#endif
	result.set_translation(Vector3::lerp(_t, _a.get_translation(), _b.get_translation()));
	result.set_orientation(Quat::slerp(_t, _a.get_orientation(), _b.get_orientation()));
	result.set_scale(_a.get_scale() * (1.0f - _t) + _b.get_scale() * _t);
	return result;
}

bool Transform::same_with_orientation(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold, Optional<float> const& _scaleThreshold, Optional<float> const& _orientationThreshold)
{
	return (_a.translation - _b.translation).length_squared() <= sqr(_translationThreshold.get(0.001f)) &&
		   abs(_a.scale - _b.scale) <=  _scaleThreshold.get(0.001f) &&
		   Quat::same_orientation(_a.orientation, _b.orientation, _orientationThreshold);
}

bool Transform::same(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold, Optional<float> const& _scaleThreshold, Optional<float> const& _orientationThreshold)
{
	float tt = _translationThreshold.get(0.0f);
	float st = _scaleThreshold.get(0.0f);
	return (tt > 0.0f? (_a.translation - _b.translation).length_squared() <= sqr(tt) : _a.translation == _b.translation) &&
		   (st > 0.0f? abs(_a.scale - _b.scale) <= st : _a.scale == _b.scale) &&
		   Quat::same(_a.orientation, _b.orientation, _orientationThreshold);
}

bool Transform::same_with_rotation(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold, Optional<float> const& _scaleThreshold, Optional<float> const& _orientationThreshold)
{
	return (_a.translation - _b.translation).length_squared() <= sqr(_translationThreshold.get(0.001f)) &&
		   abs(_a.scale - _b.scale) <=  _scaleThreshold.get(0.001f) &&
		   Quat::same_rotation(_a.orientation, _b.orientation, _orientationThreshold);
}

Transform & Transform::make_sane(bool _includingNonZeroScale)
{
	if (!orientation.is_normalised())
	{
		warn(TXT("scale of transform was 0.0"));
		orientation.normalise();
	}
	if (_includingNonZeroScale && scale == 0.0f)
	{
		warn(TXT("scale of transform was 0.0"));
		scale = 1.0f;
	}
	return *this;
}
