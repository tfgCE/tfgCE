String Rotator3::to_string() const
{
	return String::printf(TXT("p:%.10f y:%.10f r:%.10f"), pitch, yaw, roll);
}

Quat Rotator3::to_quat() const
{
	// based on: http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles

	// just randomly changed few things as when I tried to figure it out, I had something off a little bit

	float const pitchRadHalf = -degree_to_radian(pitch) * 0.5f;
	float const yawRadHalf = -degree_to_radian(yaw) * 0.5f;
	float const rollRadHalf = -degree_to_radian(roll) * 0.5f;
	float const cp = cos_rad(pitchRadHalf);
	float const cy = cos_rad(yawRadHalf);
	float const cr = cos_rad(rollRadHalf);
	float const sp = sin_rad(pitchRadHalf);
	float const sy = sin_rad(yawRadHalf);
	float const sr = sin_rad(rollRadHalf);

	Quat result;
	result.w = cp*cr*cy - sp*sr*sy;
	result.x = -sp*cr*cy + cp*sr*sy;
	result.y = -cp*sr*cy - sp*cr*sy;
	result.z = cp*cr*sy + sp*sr*cy;

	return result;
}

float Rotator3::get_yaw(Vector2 const & _dir)
{
	if (_dir.is_zero())
	{
		return 0.0f;
	}
	else
	{
		Vector2 dir = _dir.normal();
		return acos_deg(dir.y) * (dir.x > 0.0f ? 1.0f : -1.0f);
	}
}

float Rotator3::get_yaw(Vector3 const & _dir)
{
	Vector3 dir = (_dir * Vector3(1.0f, 1.0f, 0.0f));
	if (dir.is_zero())
	{
		return 0.0f;
	}
	else
	{
		dir = dir.normal();
		return acos_deg(dir.y) * (dir.x > 0.0f ? 1.0f : -1.0f);
	}
}

float Rotator3::get_roll(Vector3 const & _dir)
{
	Vector3 dir = (_dir * Vector3(1.0f, 0.0f, 1.0f));
	if (dir.is_zero())
	{
		return 0.0f;
	}
	else
	{
		dir = dir.normal();
		return acos_deg(dir.z) * (dir.x > 0.0f ? 1.0f : -1.0f);
	}
}

Vector3 Rotator3::get_forward() const
{
	float cp = cos_deg(pitch);
	float sp = sin_deg(pitch);
	float cy = cos_deg(yaw);
	float sy = sin_deg(yaw);

	return Vector3(cp * sy, cp * cy, sp);
}

Vector3 Rotator3::get_axis(Axis::Type _axis) const
{
	if (_axis == Axis::Y)
	{
		float cp = cos_deg(pitch);
		float sp = sin_deg(pitch);
		float cy = cos_deg(yaw);
		float sy = sin_deg(yaw);

		return Vector3(cp * sy, cp * cy, sp);
	}
	else if (_axis == Axis::Z)
	{
		todo_note(TXT("calculate it properly!"));
		return to_quat().get_z_axis();
	}
	else
	{
		an_assert(_axis == Axis::X);
		todo_note(TXT("calculate it properly!"));
		return to_quat().get_x_axis();
	}
}

float Rotator3::length() const
{
	return sqrt(sqr(pitch) + sqr(yaw) + sqr(roll));
}

float Rotator3::length_squared() const
{
	return sqr(pitch) + sqr(yaw) + sqr(roll);
}

Rotator3 Rotator3::normal_axes() const
{
	Rotator3 result = *this;
	result.pitch = normalise_axis(result.pitch);
	result.yaw = normalise_axis(result.yaw);
	result.roll = normalise_axis(result.roll);
	return result;
}

Rotator3 Rotator3::normal() const
{
	float len = length();
	return len != 0.0f ? *this / len : Rotator3::zero;
}

Rotator3 Rotator3::lerp(float _t, Rotator3 const& _a, Rotator3 const& _b)
{
	return _a * (1.0f - _t) + _b * _t;
}

