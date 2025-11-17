inline float aspect_ratio(SIZE_ Vector2 const & _size)
{
	return _size.x / _size.y;
}

inline float aspect_ratio(SIZE_ VectorInt2 const & _size)
{
	return (float)_size.x / (float)_size.y;
}

inline Vector2 maintain_aspect_ratio_keep_x(SIZE_ Vector2 const & _size, SIZE_ Vector2 const & _baseOnAspectRatio)
{
	return Vector2(_size.x, _size.x * _baseOnAspectRatio.y / _baseOnAspectRatio.x);
}

inline Vector2 maintain_aspect_ratio_keep_y(SIZE_ Vector2 const & _size, SIZE_ Vector2 const & _baseOnAspectRatio)
{
	return Vector2(_size.y * _baseOnAspectRatio.x / _baseOnAspectRatio.y, _size.y);
}

inline Vector2 apply_aspect_ratio_coef(SIZE_ Vector2 const& _size, float _aspectRatioCoef)
{
	Vector2 vs = _size;
	if (_aspectRatioCoef > 1.0f)
	{
		vs.y /= _aspectRatioCoef;
	}
	else
	{
		vs.x *= _aspectRatioCoef;
	}
	return vs;
}

inline VectorInt2 apply_aspect_ratio_coef(SIZE_ VectorInt2 const& _size, float _aspectRatioCoef)
{
	VectorInt2 vs = _size;
	if (_aspectRatioCoef > 1.0f)
	{
		vs.y = (int)((float)vs.y / _aspectRatioCoef);
	}
	else
	{
		vs.x = (int)((float)vs.x * _aspectRatioCoef);
	}
	return vs;
}

inline Vector2 get_full_for_aspect_ratio_coef(SIZE_ Vector2 const& _size, float _aspectRatioCoef)
{
	Vector2 vs = _size;
	if (_aspectRatioCoef > 1.0f)
	{
		vs.y *= _aspectRatioCoef;
	}
	else
	{
		vs.x /= _aspectRatioCoef;
	}
	return vs;
}

inline VectorInt2 get_full_for_aspect_ratio_coef(SIZE_ VectorInt2 const& _size, float _aspectRatioCoef)
{
	VectorInt2 vs = _size;
	if (_aspectRatioCoef > 1.0f)
	{
		vs.y = (int)((float)vs.y * _aspectRatioCoef);
	}
	else
	{
		vs.x = (int)((float)vs.x / _aspectRatioCoef);
	}
	return vs;
}

inline Matrix44 rotation_xy_matrix(float _deg)
{
	Matrix44 result = Matrix44::identity;
	float s = sin_deg(_deg);
	float c = cos_deg(_deg);
	result.m00 = c;
	result.m01 = s;
	result.m10 = -s;
	result.m11 = c;
	return result;
}

inline Matrix44 translation_matrix(LOCATION_ Vector3 const & _translation)
{
	Matrix44 result = Matrix44::identity;
	result.set_translation(_translation);
	return result;
}

inline Matrix44 scale_matrix(float _scale)
{
	Matrix44 result = Matrix44::identity;
	result.m00 *= _scale;
	result.m11 *= _scale;
	result.m22 *= _scale;
	return result;
}

inline Matrix44 scale_matrix(Vector3 const & _scale)
{
	Matrix44 result = Matrix44::identity;
	result.m00 *= _scale.x;
	result.m11 *= _scale.y;
	result.m22 *= _scale.z;
	return result;
}

inline Matrix44 translation_scale_xy_matrix(LOCATION_ Vector2 const & _translation, Vector2 const & _scale)
{
	Matrix44 result = Matrix44::identity;
	result.m00 = _scale.x;
	result.m11 = _scale.y;
	result.set_translation(Vector3(_translation.x, _translation.y, 0.0f));
	return result;
}

inline Matrix44 look_at_matrix(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _up)
{
	an_assert(_up.is_normalised());

	Matrix44 result;
	Vector3 up, right, viewDir;

	viewDir = _at - _from;
	viewDir.normalise();

	right = Vector3::cross(viewDir, _up);
	right.normalise();

	up = Vector3::cross(right, viewDir);

	// rotation part:
    result.m00 = right.x;
    result.m01 = right.y;
    result.m02 = right.z;
    result.m10 = viewDir.x;
    result.m11 = viewDir.y;
    result.m12 = viewDir.z;
    result.m20 = up.x;
    result.m21 = up.y;
    result.m22 = up.z;

	// location part:
    result.m30 = _from.x;
    result.m31 = _from.y;
    result.m32 = _from.z;

	// fourth column:
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = 0.0f;
    result.m33 = 1.0f;

	return result;
}

inline Matrix44 look_at_matrix_with_right(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _right)
{
	Matrix44 result;
	Vector3 up, right, viewDir;

	viewDir = _at - _from;
	viewDir.normalise();

	up = Vector3::cross(_right, viewDir);
	up.normalise();

	right = Vector3::cross(viewDir, up);
	right.normalise();

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = viewDir.x;
	result.m11 = viewDir.y;
	result.m12 = viewDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	// location part:
	result.m30 = _from.x;
	result.m31 = _from.y;
	result.m32 = _from.z;

	// fourth column:
	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

inline Matrix44 look_matrix(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _up)
{
	an_assert(_up.is_normalised());
	an_assert(_lookDir.is_normalised());

	Matrix44 result;
	Vector3 up, right;

	right = Vector3::cross(_lookDir, _up);
	right.normalise();

	up = Vector3::cross(right, _lookDir);

	// rotation part:
    result.m00 = right.x;
    result.m01 = right.y;
    result.m02 = right.z;
    result.m10 = _lookDir.x;
    result.m11 = _lookDir.y;
    result.m12 = _lookDir.z;
    result.m20 = up.x;
    result.m21 = up.y;
    result.m22 = up.z;

	// location part:
    result.m30 = _from.x;
    result.m31 = _from.y;
    result.m32 = _from.z;

	// fourth column:
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = 0.0f;
    result.m33 = 1.0f;

	return result;
}

inline Matrix44 look_matrix_no_up(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir)
{
	an_assert(_lookDir.is_normalised());

	Matrix44 result;
	Vector3 up, right;

	up = abs(_lookDir.z) < 0.8f ? Vector3::zAxis : Vector3::yAxis;

	right = Vector3::cross(_lookDir, up);
	right.normalise();

	up = Vector3::cross(right, _lookDir);

	// rotation part:
    result.m00 = right.x;
    result.m01 = right.y;
    result.m02 = right.z;
    result.m10 = _lookDir.x;
    result.m11 = _lookDir.y;
    result.m12 = _lookDir.z;
    result.m20 = up.x;
    result.m21 = up.y;
    result.m22 = up.z;

	// location part:
    result.m30 = _from.x;
    result.m31 = _from.y;
    result.m32 = _from.z;

	// fourth column:
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = 0.0f;
    result.m33 = 1.0f;

	return result;
}

inline Matrix44 look_matrix_with_right(LOCATION_ Vector3 const & _from, NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _right)
{
	an_assert(_right.is_normalised());
	an_assert(_lookDir.is_normalised());

	Matrix44 result;
	Vector3 up, right;

	up = Vector3::cross(_right, _lookDir);
	up.normalise();

	right = Vector3::cross(_lookDir, up);
	right.normalise();

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = _lookDir.x;
	result.m11 = _lookDir.y;
	result.m12 = _lookDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	// location part:
	result.m30 = _from.x;
	result.m31 = _from.y;
	result.m32 = _from.z;

	// fourth column:
	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

Matrix33 look_matrix33(NORMAL_ Vector3 const & _lookDir, NORMAL_ Vector3 const & _up)
{
	// as above without "from"

	an_assert(_up.is_normalised());
	an_assert(_lookDir.is_normalised());

	Matrix33 result;
	Vector3 up, right, viewDir;

	right = Vector3::cross(_lookDir, _up);
	right.normalise();

	up = Vector3::cross(right, _lookDir);

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = _lookDir.x;
	result.m11 = _lookDir.y;
	result.m12 = _lookDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	return result;
}

Matrix44 matrix_from_axes_orthonormal_check(NORMAL_ Vector3 const & _xAxis, NORMAL_ Vector3 const & _yAxis, NORMAL_ Vector3 const & _zAxis, LOCATION_ Vector3 const & _location)
{
	an_assert(_xAxis.is_normalised());
	an_assert(_yAxis.is_normalised());
	an_assert(_zAxis.is_normalised());
	an_assert(abs(Vector3::dot(_xAxis, _yAxis)) < 0.01f);
	an_assert(abs(Vector3::dot(_xAxis, _zAxis)) < 0.01f);
	an_assert(abs(Vector3::dot(_yAxis, _zAxis)) < 0.01f);

	Matrix44 result;
	result.m00 = _xAxis.x;
	result.m01 = _xAxis.y;
	result.m02 = _xAxis.z;
	result.m03 = 0.0f;

	result.m10 = _yAxis.x;
	result.m11 = _yAxis.y;
	result.m12 = _yAxis.z;
	result.m13 = 0.0f;

	result.m20 = _zAxis.x;
	result.m21 = _zAxis.y;
	result.m22 = _zAxis.z;
	result.m23 = 0.0f;

	result.m30 = _location.x;
	result.m31 = _location.y;
	result.m32 = _location.z;
	result.m33 = 1.0f;

	an_assert(result.is_orthonormal());

	return result;
}

Matrix44 matrix_from_axes(NORMAL_ Vector3 const & _xAxis, NORMAL_ Vector3 const & _yAxis, NORMAL_ Vector3 const & _zAxis, LOCATION_ Vector3 const & _location)
{
	Matrix44 result;
	result.m00 = _xAxis.x;
	result.m01 = _xAxis.y;
	result.m02 = _xAxis.z;
	result.m03 = 0.0f;

	result.m10 = _yAxis.x;
	result.m11 = _yAxis.y;
	result.m12 = _yAxis.z;
	result.m13 = 0.0f;

	result.m20 = _zAxis.x;
	result.m21 = _zAxis.y;
	result.m22 = _zAxis.z;
	result.m23 = 0.0f;

	result.m30 = _location.x;
	result.m31 = _location.y;
	result.m32 = _location.z;
	result.m33 = 1.0f;

	return result;
}

Matrix33 up_matrix33(NORMAL_ Vector3 const & _up)
{
	an_assert(_up.is_normalised());

	Vector3 lookDir = Vector3::yAxis;
	if (abs(_up.y) > 0.7f) lookDir = Vector3::xAxis;
	
	an_assert(_up.is_normalised());
	an_assert(lookDir.is_normalised());

	Matrix33 result;
	Vector3 up, right, viewDir;

	up = _up;

	right = Vector3::cross(lookDir, up);
	right.normalise();

	lookDir = Vector3::cross(up, right);

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = lookDir.x;
	result.m11 = lookDir.y;
	result.m12 = lookDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	return result;
}

inline Matrix44 matrix_from_up_right(LOCATION_ Vector3 const & _loc, NORMAL_ Vector3 const & _up, NORMAL_ Vector3 const & _right)
{
	Matrix44 result;
	Vector3 up = _up.normal();
	Vector3 right = _right.normal();
	Vector3 viewDir = Vector3::cross(_up, _right).normal();

	// recalculate right
	right = Vector3::cross(viewDir, _up);
	right.normalise();

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = viewDir.x;
	result.m11 = viewDir.y;
	result.m12 = viewDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	// location part:
	result.m30 = _loc.x;
	result.m31 = _loc.y;
	result.m32 = _loc.z;

	// fourth column:
	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

inline Matrix44 matrix_from_up_forward(LOCATION_ Vector3 const & _loc, NORMAL_ Vector3 const & _up, NORMAL_ Vector3 const & _forward)
{
	Matrix44 result;
	Vector3 up = _up.normal();
	Vector3 viewDir = _forward.normal();
	Vector3 right = Vector3::cross(viewDir, up);
	right.normalise();

	// recalculate view dir
	viewDir = Vector3::cross(up, right);
	viewDir.normalise();

	// rotation part:
	result.m00 = right.x;
	result.m01 = right.y;
	result.m02 = right.z;
	result.m10 = viewDir.x;
	result.m11 = viewDir.y;
	result.m12 = viewDir.z;
	result.m20 = up.x;
	result.m21 = up.y;
	result.m22 = up.z;

	// location part:
	result.m30 = _loc.x;
	result.m31 = _loc.y;
	result.m32 = _loc.z;

	// fourth column:
	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

Matrix44 turn_matrix_xy_180(Matrix44 const & mat)
{
	return matrix_from_axes_orthonormal_check(-mat.get_x_axis(), -mat.get_y_axis(), mat.get_z_axis(), mat.get_translation());
}
