String Vector3::to_string() const
{
	return String::printf(TXT("x:%.10f y:%.10f z:%.10f"), x, y, z);
}

VectorInt3 Vector3::to_vector_int3() const
{
	return VectorInt3(TypeConversions::Normal::f_i_closest(x), TypeConversions::Normal::f_i_closest(y), TypeConversions::Normal::f_i_closest(z));
}

Rotator3 Vector3::to_rotator() const
{
	Vector3 dir = normal();
	dir.z = clamp(dir.z, -1.0f, 1.0f);
	Rotator3 ret;
	ret.roll = 0.0f;
	ret.pitch = asin_deg(dir.z);

	if (dir.x == 0.0f && dir.y == 0.0f)
	{
		ret.yaw = 0.0f;
	}
	else
	{
		float const lengthXY = sqrt(1.0f - sqr(dir.z));
		float const multiplyXY = lengthXY != 0.0f ? 1.0f / lengthXY : 0.0f;
		ret.yaw = acos_deg(clamp(dir.y * multiplyXY, -1.0f, 1.0f)) * (dir.x > 0.0f ? 1.0f : -1.0f);
	}

	return ret;
}

Vector2 Vector3::to_vector2() const
{
	return Vector2(x, y);
}

Vector4 Vector3::to_vector4(float _w) const
{
	return Vector4(x, y, z, _w);
}

bool Vector3::is_normalised() const
{
	return abs(length_squared() - 1.0f) < 0.01f;
}

void Vector3::normalise_2d()
{
	z = 0.0f;
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		x *= lenSq;
		y *= lenSq;
		z *= lenSq;
	}
}

void Vector3::normalise()
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		x *= lenSq;
		y *= lenSq;
		z *= lenSq;
	}
}

Vector3 Vector3::normal_2d() const
{
	float lenSq = x * x + y * y;
	if (lenSq != 0.0f)
	{
		lenSq = lenSq != 1.0f ? 1.0f / sqrt(lenSq) : 1.0f;
		return Vector3(x * lenSq, y * lenSq, 0.0f);
	}
	return Vector3::zero;
}

Vector3 Vector3::normal() const
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = lenSq != 1.0f ? 1.0f / sqrt(lenSq) : 1.0f;
		return Vector3(x * lenSq, y * lenSq, z * lenSq);
	}
	return Vector3::zero;
}

void Vector3::normal_and_length(OUT_ Vector3 & _normal, OUT_ float & _length) const
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		_length = sqrt(lenSq);
		lenSq = 1.0f / _length;
		_normal = Vector3(x * lenSq, y * lenSq, z * lenSq);
	}
	else
	{
		_length = 0.0f;
		_normal = Vector3::zero;
	}
}

Vector3 Vector3::blend(Vector3 const & _a, Vector3 const & _b, float _t)
{
	float it = 1.0f - _t;
	return Vector3(_a.x * it + _b.x * _t,
				   _a.y * it + _b.y * _t,
				   _a.z * it + _b.z * _t);
}

Vector3 Vector3::lerp(float _t, Vector3 const & _a, Vector3 const & _b)
{
	return _a * (1.0f - _t) + _b * _t;
}

void Vector3::rotate_by_pitch(float _pitch)
{
	float oy = y;
	float oz = z;
	float sa = sin_deg(_pitch);
	float ca = cos_deg(_pitch);
	y =  ca * oy + sa * oz;
	z = -sa * oy + ca * oz;
}

Vector3 Vector3::rotated_by_pitch(float _pitch) const
{
	float sa = sin_deg(_pitch);
	float ca = cos_deg(_pitch);
	return Vector3(x,
				   ca * y - sa * z,
				   sa * y + ca * z);
}

void Vector3::rotate_by_yaw(float _yaw)
{
	float ox = x;
	float oy = y;
	float sa = sin_deg(_yaw);
	float ca = cos_deg(_yaw);
	x =  ca * ox + sa * oy;
	y = -sa * ox + ca * oy;
}

Vector3 Vector3::rotated_by_yaw(float _yaw) const
{
	float sa = sin_deg(_yaw);
	float ca = cos_deg(_yaw);
	return Vector3(ca * x + sa * y,
				  -sa * x + ca * y,
				   z);
}

void Vector3::rotate_by_roll(float _roll)
{
	float ox = x;
	float oz = z;
	float sa = sin_deg(_roll);
	float ca = cos_deg(_roll);
	x =  ca * ox + sa * oz;
	z = -sa * ox + ca * oz;
}

Vector3 Vector3::rotated_by_roll(float _roll) const
{
	float sa = sin_deg(_roll);
	float ca = cos_deg(_roll);
	return Vector3(ca * x + sa * z,
				   y,
				  -sa * x + ca * z);
}

Vector3 Vector3::drop_using(Vector3 const & _alongDir) const
{
	Vector3 alongDir = _alongDir.normal();
	return *this - alongDir * dot(*this, alongDir);
}

Vector3 Vector3::drop_using_normalised(Vector3 const & _alongDir) const
{
	an_assert(_alongDir.is_normalised());
	return *this - _alongDir * dot(*this, _alongDir);
}

Vector3 Vector3::along(Vector3 const & _alongDir) const
{
	Vector3 alongDir = _alongDir.normal();
	return alongDir * dot(*this, alongDir);
}

Vector3 Vector3::along_normalised(Vector3 const & _alongDir) const
{
	an_assert(_alongDir.is_normalised());
	return _alongDir * dot(*this, _alongDir);
}

Vector3 const & Vector3::axis(Axis::Type _t)
{
	if (_t == Axis::X) return xAxis;
	if (_t == Axis::Y) return yAxis;
	if (_t == Axis::Z) return zAxis;

	return zero;
}

Vector3 Vector3::keep_within_angle(Vector3 const & _otherDir, float _maxAngleDot) const
{
	return keep_within_angle_normalised(_otherDir.normal(), _maxAngleDot);
}

Vector3 Vector3::keep_within_angle_normalised(Vector3 const & _otherDir, float _maxAngleDot) const
{
	an_assert(_otherDir.is_normalised());
	float len = length();
	float minDotNormal = _maxAngleDot;
	float minDot = minDotNormal * len;
	float dot = Vector3::dot(*this, _otherDir);
	if (dot >= minDot)
	{
		return *this;
	}
	else
	{
		Vector3 flat = *this - _otherDir * dot;
		return _otherDir * minDot + flat.normal() * sqrt(max(0.0f, sqr(len) - sqr(minDot)));
	}
}
