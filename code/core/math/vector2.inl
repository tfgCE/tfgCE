String Vector2::to_string() const
{
	return String::printf(TXT("x:%.10f y:%.10f"), x, y);
}

String Vector2::to_loadable_string() const
{
	return String::printf(TXT("%.10f %.10f"), x, y);
}

VectorInt2 Vector2::to_vector_int2() const
{
	return VectorInt2(TypeConversions::Normal::f_i_closest(x), TypeConversions::Normal::f_i_closest(y));
}

VectorInt2 Vector2::to_vector_int2_cells() const
{
	return VectorInt2(TypeConversions::Normal::f_i_cells(x), TypeConversions::Normal::f_i_cells(y));
}

Vector3 Vector2::to_vector3(float _z) const
{
	return Vector3(x, y, _z);
}

Vector3 Vector2::to_vector3_xz(float _newY) const
{
	return Vector3(x, _newY, y);
}

bool Vector2::is_normalised() const
{
	return abs(length_squared() - 1.0f) < 0.01f;
}

void Vector2::normalise()
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		x *= lenSq;
		y *= lenSq;
	}
}

Vector2 Vector2::normal() const
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		return Vector2(x * lenSq, y * lenSq);
	}
	return Vector2::zero;
}

Vector2 Vector2::normal_in_square() const
{
	if (is_almost_zero())
	{
		return Vector2::zero;
	}

	float absX = abs(x);
	float absY = abs(y);
	if (absX > absY)
	{
		return Vector2(sign(x), y / absX);
	}
	else
	{
		return Vector2(x / absY, sign(y));
	}
}

float Vector2::angle() const
{
	float angle = 0.0f;
	if (x != 0.0f || y != 0.0f)
	{
		if (y == 0.0f)
		{
			if (x > 0.0f)
			{
				angle = 90.0f;
			}
			else
			{
				angle = -90.0f;
			}
		}
		else
		{
			angle = atan_deg(abs(x/y));
			if (y < 0.0f)
			{
				angle = 180.0f - angle;
			}
			if (x < 0.0f)
			{
				angle = -angle;
			}
		}
	}
	return angle;
}
		
void Vector2::rotate_by_angle(float _angle)
{
	float ox = x;
	float oy = y;
	float sa = sin_deg(_angle);
	float ca = cos_deg(_angle);
	x =   ca * ox + sa * oy;
	y = - sa * ox + ca * oy;
}
		
Vector2 Vector2::rotated_by_angle(float _angle) const
{
	float sa = sin_deg(_angle);
	float ca = cos_deg(_angle);
	return Vector2(  ca * x + sa * y,
				   - sa * x + ca * y);
}
/*
void Vector2::rotate_by(Rotator2 _rotator)
{
	rotate_by_angle(_rotator.yaw);
}

Vector2 Vector2::rotated_by(Rotator2 _rotator)
{
	return rotated_by_angle(_rotator.yaw);
}
*/

Vector2 Vector2::drop_using(Vector2 const& _alongDir) const
{
	Vector2 alongDir = _alongDir.normal();
	return *this - alongDir * dot(*this, alongDir);
}

Vector2 Vector2::drop_using_normalised(Vector2 const& _alongDir) const
{
	an_assert(_alongDir.is_normalised());
	return *this - _alongDir * dot(*this, _alongDir);
}

void Vector2::maximise_off_zero(Vector2 const& _other)
{
	if (_other.x > 0.0f && _other.x > x) { x = _other.x; } else
	if (_other.x < 0.0f && _other.x < x) { x = _other.x; }
	if (_other.y > 0.0f && _other.y > y) { y = _other.y; } else
	if (_other.y < 0.0f && _other.y < y) { y = _other.y; }
}
