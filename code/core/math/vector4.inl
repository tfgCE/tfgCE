String Vector4::to_string() const
{
	return String::printf(TXT("x:%.10f y:%.10f z:%.10f w:%.10f"), x, y, z, w);
}

Vector3 Vector4::to_vector3() const
{
	return Vector3(x, y, z);
}

bool Vector4::is_normalised() const
{
	return abs(length_squared() - 1.0f) < 0.01f;
}

void Vector4::normalise()
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		x *= lenSq;
		y *= lenSq;
		z *= lenSq;
		w *= lenSq;
	}
}

Vector4 Vector4::normal() const
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		return Vector4(x * lenSq, y * lenSq, z * lenSq, w * lenSq);
	}
	return Vector4::zero;
}

