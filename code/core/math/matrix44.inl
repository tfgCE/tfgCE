Matrix44::Matrix44(float _m00, float _m01, float _m02, float _m03,
				   float _m10, float _m11, float _m12, float _m13,
				   float _m20, float _m21, float _m22, float _m23,
				   float _m30, float _m31, float _m32, float _m33)
{
	m00 = _m00;	m01 = _m01;	m02 = _m02;	m03 = _m03;
	m10 = _m10;	m11 = _m11;	m12 = _m12;	m13 = _m13;
	m20 = _m20;	m21 = _m21;	m22 = _m22;	m23 = _m23;
	m30 = _m30;	m31 = _m31;	m32 = _m32;	m33 = _m33;
}

Matrix44 Matrix44::inverted() const
{
	an_assert(is_orthogonal(), TXT("matrix should be orthogonal"));

	assert_slow(m03 == 0.0f);
	assert_slow(m13 == 0.0f);
	assert_slow(m23 == 0.0f);
	assert_slow(m33 == 1.0f);

	float const scaleSqr = extract_scale_squared();
	float const invScaleSqr = scaleSqr != 0.0f ? 1.0f / scaleSqr : 0.0f;

	float const tm00 = m00 * invScaleSqr;
	float const tm01 = m01 * invScaleSqr;
	float const tm02 = m02 * invScaleSqr;
	float const tm10 = m10 * invScaleSqr;
	float const tm11 = m11 * invScaleSqr;
	float const tm12 = m12 * invScaleSqr;
	float const tm20 = m20 * invScaleSqr;
	float const tm21 = m21 * invScaleSqr;
	float const tm22 = m22 * invScaleSqr;

	// transpose 3x3
	return Matrix44(tm00, tm10, tm20, m03,
					tm01, tm11, tm21, m13,
					tm02, tm12, tm22, m23,
					-m30 * tm00  -  m31 * tm01  -  m32 * tm02,
					-m30 * tm10  -  m31 * tm11  -  m32 * tm12,
					-m30 * tm20  -  m31 * tm21  -  m32 * tm22,
					m33);
}

Matrix44 Matrix44::full_inverted() const
{
	// based on: http://www.cg.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche23.html
	//		and: http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
	float detValue = det();
	an_assert(detValue != 0.0f);
	if (detValue == 0.0f)
	{
		return Matrix44::zero;
	}
	float b00 = m12*m23*m31 - m13*m22*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 + m11*m22*m33;
	float b01 = m03*m22*m31 - m02*m23*m31 - m03*m21*m32 + m01*m23*m32 + m02*m21*m33 - m01*m22*m33;
	float b02 = m02*m13*m31 - m03*m12*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 + m01*m12*m33;
	float b03 = m03*m12*m21 - m02*m13*m21 - m03*m11*m22 + m01*m13*m22 + m02*m11*m23 - m01*m12*m23;
	float b10 = m13*m22*m30 - m12*m23*m30 - m13*m20*m32 + m10*m23*m32 + m12*m20*m33 - m10*m22*m33;
	float b11 = m02*m23*m30 - m03*m22*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 + m00*m22*m33;
	float b12 = m03*m12*m30 - m02*m13*m30 - m03*m10*m32 + m00*m13*m32 + m02*m10*m33 - m00*m12*m33;
	float b13 = m02*m13*m20 - m03*m12*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 + m00*m12*m23;
	float b20 = m11*m23*m30 - m13*m21*m30 + m13*m20*m31 - m10*m23*m31 - m11*m20*m33 + m10*m21*m33;
	float b21 = m03*m21*m30 - m01*m23*m30 - m03*m20*m31 + m00*m23*m31 + m01*m20*m33 - m00*m21*m33;
	float b22 = m01*m13*m30 - m03*m11*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 + m00*m11*m33;
	float b23 = m03*m11*m20 - m01*m13*m20 - m03*m10*m21 + m00*m13*m21 + m01*m10*m23 - m00*m11*m23;
	float b30 = m12*m21*m30 - m11*m22*m30 - m12*m20*m31 + m10*m22*m31 + m11*m20*m32 - m10*m21*m32;
	float b31 = m01*m22*m30 - m02*m21*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 + m00*m21*m32;
	float b32 = m02*m11*m30 - m01*m12*m30 - m02*m10*m31 + m00*m12*m31 + m01*m10*m32 - m00*m11*m32;
	float b33 = m01*m12*m20 - m02*m11*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 + m00*m11*m22;
	return Matrix44(b00, b01, b02, b03,
					b10, b11, b12, b13,
					b20, b21, b22, b23,
					b30, b31, b32, b33) * (1.0f / detValue);
}

Matrix44 Matrix44::transposed() const
{
	return Matrix44(m00, m10, m20, m30,
					m01, m11, m21, m31,
					m02, m12, m22, m32,
					m03, m13, m23, m33);
}

Vector4 Matrix44::mul(Vector4 const & _a) const
{
	return Vector4( m00 * _a.x  +  m10 * _a.y  +  m20 * _a.z  +  m30 * _a.w,
					m01 * _a.x  +  m11 * _a.y  +  m21 * _a.z  +  m31 * _a.w,
					m02 * _a.x  +  m12 * _a.y  +  m22 * _a.z  +  m32 * _a.w,
					m03 * _a.x  +  m13 * _a.y  +  m23 * _a.z  +  m33 * _a.w );
}

Vector3 Matrix44::location_to_world(Vector3 const & _a) const
{
	return Vector3( m00 * _a.x  +  m10 * _a.y  +  m20 * _a.z  +  m30,
					m01 * _a.x  +  m11 * _a.y  +  m21 * _a.z  +  m31,
					m02 * _a.x  +  m12 * _a.y  +  m22 * _a.z  +  m32 );
}

Vector3 Matrix44::location_to_local(Vector3 const & _a) const
{
	an_assert(is_orthogonal(), TXT("matrix should be orthogonal"));

	float const scaleSqr = extract_scale_squared();
	float const invScaleSqr = scaleSqr != 0.0f ? 1.0f / scaleSqr : 0.0f;

	Vector3 b( _a.x - m30, _a.y - m31, _a.z - m32 );

	return Vector3( m00 * b.x  +  m01 * b.y  +  m02 * b.z,
					m10 * b.x  +  m11 * b.y  +  m12 * b.z,
					m20 * b.x  +  m21 * b.y  +  m22 * b.z ) * invScaleSqr;
}

Vector3 Matrix44::vector_to_world(Vector3 const & _a) const
{
	return Vector3( m00 * _a.x  +  m10 * _a.y  +  m20 * _a.z,
					m01 * _a.x  +  m11 * _a.y  +  m21 * _a.z,
					m02 * _a.x  +  m12 * _a.y  +  m22 * _a.z );
}

Vector3 Matrix44::vector_to_local(Vector3 const & _a) const
{
	an_assert(is_orthogonal(), TXT("matrix should be orthogonal"));

	float const scaleSqr = extract_scale_squared();
	float const invScaleSqr = scaleSqr != 0.0f ? 1.0f / scaleSqr : 0.0f;

	return Vector3( m00 * _a.x  +  m01 * _a.y  +  m02 * _a.z,
					m10 * _a.x  +  m11 * _a.y  +  m12 * _a.z,
					m20 * _a.x  +  m21 * _a.y  +  m22 * _a.z ) * invScaleSqr;
}

Matrix44 Matrix44::to_world(Matrix44 const & _a) const
{
	return Matrix44( m00 * _a.m00  +  m10 * _a.m01  +  m20 * _a.m02  +  m30 * _a.m03,
					 m01 * _a.m00  +  m11 * _a.m01  +  m21 * _a.m02  +  m31 * _a.m03,
					 m02 * _a.m00  +  m12 * _a.m01  +  m22 * _a.m02  +  m32 * _a.m03,
					 m03 * _a.m00  +  m13 * _a.m01  +  m23 * _a.m02  +  m33 * _a.m03,
				/* */m00 * _a.m10  +  m10 * _a.m11  +  m20 * _a.m12  +  m30 * _a.m13,
					 m01 * _a.m10  +  m11 * _a.m11  +  m21 * _a.m12  +  m31 * _a.m13,
					 m02 * _a.m10  +  m12 * _a.m11  +  m22 * _a.m12  +  m32 * _a.m13,
					 m03 * _a.m10  +  m13 * _a.m11  +  m23 * _a.m12  +  m33 * _a.m13,
				/* */m00 * _a.m20  +  m10 * _a.m21  +  m20 * _a.m22  +  m30 * _a.m23,
					 m01 * _a.m20  +  m11 * _a.m21  +  m21 * _a.m22  +  m31 * _a.m23,
					 m02 * _a.m20  +  m12 * _a.m21  +  m22 * _a.m22  +  m32 * _a.m23,
					 m03 * _a.m20  +  m13 * _a.m21  +  m23 * _a.m22  +  m33 * _a.m23,
				/* */m00 * _a.m30  +  m10 * _a.m31  +  m20 * _a.m32  +  m30 * _a.m33,
					 m01 * _a.m30  +  m11 * _a.m31  +  m21 * _a.m32  +  m31 * _a.m33,
					 m02 * _a.m30  +  m12 * _a.m31  +  m22 * _a.m32  +  m32 * _a.m33,
					 m03 * _a.m30  +  m13 * _a.m31  +  m23 * _a.m32  +  m33 * _a.m33 );
}

Matrix44 Matrix44::to_local(Matrix44 const & _a) const
{
	// inverted is quite fast
	return inverted().to_world(_a);
}

Plane Matrix44::to_world(Plane const & _a) const
{
	return Plane(vector_to_world(_a.get_normal()).normal(), location_to_world(_a.get_anchor()));
}

Plane Matrix44::to_local(Plane const & _a) const
{
	return Plane(vector_to_local(_a.get_normal()).normal(), location_to_local(_a.get_anchor()));
}

void Matrix44::set_translation(Vector3 const & _a)
{
	m30 = _a.x;
	m31 = _a.y;
	m32 = _a.z;
}

bool Matrix44::is_orthonormal() const
{
	return abs(m00 * m10 + m01 * m11 + m02 * m12) < 0.01f &&
		   abs(m00 * m20 + m01 * m21 + m02 * m22) < 0.01f &&
		   abs(m00 * m00 + m01 * m01 + m02 * m02 - 1.0f) < 0.01f &&
		   abs(m10 * m10 + m11 * m11 + m12 * m12 - 1.0f) < 0.01f &&
		   abs(m20 * m20 + m21 * m21 + m22 * m22 - 1.0f) < 0.01f;
}

bool Matrix44::is_orthogonal() const
{
	return abs(m00 * m10 + m01 * m11 + m02 * m12) < 0.01f &&
		   abs(m00 * m20 + m01 * m21 + m02 * m22) < 0.01f;
}

void Matrix44::remove_scale()
{
	float xAxis = get_x_axis().length();
	float xAxisInv = xAxis != 0.0f ? 1.0f / xAxis : 0.0f;
	m00 *= xAxisInv;
	m01 *= xAxisInv;
	m02 *= xAxisInv;

	float yAxis = get_y_axis().length();
	float yAxisInv = yAxis != 0.0f ? 1.0f / yAxis : 0.0f;
	m10 *= yAxisInv;
	m11 *= yAxisInv;
	m12 *= yAxisInv;

	float zAxis = get_z_axis().length();
	float zAxisInv = zAxis != 0.0f ? 1.0f / zAxis : 0.0f;
	m20 *= zAxisInv;
	m21 *= zAxisInv;
	m22 *= zAxisInv;
}

float Matrix44::extract_scale() const
{
	float xAxis = get_x_axis().length();
	an_assert(length_of(xAxis - get_y_axis().length()) < 0.001f);
	an_assert(length_of(xAxis - get_z_axis().length()) < 0.001f);
	return xAxis;
}

float Matrix44::extract_scale_squared() const
{
	float xAxis = get_x_axis().length_squared();
	an_assert(length_of(xAxis - get_y_axis().length_squared()) < 0.001f);
	an_assert(length_of(xAxis - get_z_axis().length_squared()) < 0.001f);
	return xAxis;
}

float Matrix44::extract_min_scale() const
{
	return min(get_x_axis().length(), min(get_y_axis().length(), get_z_axis().length()));
}

bool Matrix44::has_uniform_scale() const
{
	float xAxis = get_x_axis().length();
	return (length_of(xAxis - get_y_axis().length()) < 0.001f) &&
		   (length_of(xAxis - get_z_axis().length()) < 0.001f);
}

bool Matrix44::has_at_least_one_zero_scale() const
{
	return get_x_axis().is_zero() || get_y_axis().is_zero() || get_z_axis().is_zero();
}

Matrix33 Matrix44::to_matrix33() const
{
	return Matrix33(m00, m01, m02,
					m10, m11, m12,
					m20, m21, m22);
}

Quat Matrix44::to_quat() const
{
	an_assert(is_orthogonal());

	float const scale = extract_scale();
	float const invScale = scale != 0.0f ? 1.0f / scale : 0.0f;

	float const tm00 = m00 * invScale;
	float const tm01 = m01 * invScale;
	float const tm02 = m02 * invScale;
	float const tm10 = m10 * invScale;
	float const tm11 = m11 * invScale;
	float const tm12 = m12 * invScale;
	float const tm20 = m20 * invScale;
	float const tm21 = m21 * invScale;
	float const tm22 = m22 * invScale;

	//	based on: http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
	//			: but with row and column swapped
	float tr = tm00 + tm11 + tm22;

	Quat result;
	if (tr > 0)
	{
		float s = sqrt(tr + 1.0f) * 2.0f; // s=4*qw 
		result.w = 0.25f * s;
		s = 1.0f / s;
		result.x = (tm12 - tm21) * s;
		result.y = (tm20 - tm02) * s;
		result.z = (tm01 - tm10) * s;
	}
	else if (tm00 > tm11 && tm00 > tm22)
	{
		float s = sqrt(1.0f + tm00 - tm11 - tm22) * 2.0f; // s=4*qx 
		result.x = 0.25f * s;
		s = 1.0f / s;
		result.w = (tm12 - tm21) * s;
		result.y = (tm10 + tm01) * s;
		result.z = (tm20 + tm02) * s;
	}
	else if (tm11 > tm22)
	{
		float s = sqrt(1.0f + tm11 - tm00 - tm22) * 2.0f; // s=4*qy
		result.y = 0.25f * s;
		s = 1.0f / s;
		result.w = (tm20 - tm02) * s;
		result.x = (tm10 + tm01) * s;
		result.z = (tm21 + tm12) * s;
	}
	else
	{
		float s = sqrt(1.0f + tm22 - tm00 - tm11) * 2.0f; // s=4*qz
		result.z = 0.25f * s;
		s = 1.0f / s;
		result.w = (tm01 - tm10) * s;
		result.x = (tm20 + tm02) * s;
		result.y = (tm21 + tm12) * s;
	}
	return result;
}

Transform Matrix44::to_transform() const
{
	//Transform t = Transform(get_translation(), to_quat());
	//Matrix44 m = t.to_matrix();
	return Transform(get_translation(), to_quat(), get_x_axis().length());
}

bool Matrix44::operator ==(Matrix44 const & _a) const
{
	return m00 == _a.m00 && m01 == _a.m01 && m02 == _a.m02 && m03 == _a.m03 &&
		   m10 == _a.m10 && m11 == _a.m11 && m12 == _a.m12 && m13 == _a.m13 &&
		   m20 == _a.m20 && m21 == _a.m21 && m22 == _a.m22 && m23 == _a.m23 &&
		   m30 == _a.m30 && m31 == _a.m31 && m32 == _a.m32 && m33 == _a.m33;
}

bool Matrix44::operator !=(Matrix44 const & _a) const
{
	return !operator==(_a);
}

void Matrix44::set_orientation_matrix(Matrix33 const & _orientation)
{
	m00 = _orientation.m00;
	m01 = _orientation.m01;
	m02 = _orientation.m02;
	m10 = _orientation.m10;
	m11 = _orientation.m11;
	m12 = _orientation.m12;
	m20 = _orientation.m20;
	m21 = _orientation.m21;
	m22 = _orientation.m22;
}

Matrix33 Matrix44::get_orientation_matrix() const
{
	return Matrix33(m00, m01, m02,
					m10, m11, m12,
					m20, m21, m22);
}

float Matrix44::det() const
{
	return m03 * m12 * m21 * m30 - m02 * m13 * m21 * m30 -
		   m03 * m11 * m22 * m30 + m01 * m13 * m22 * m30 +
		   m02 * m11 * m23 * m30 - m01 * m12 * m23 * m30 -
		   m03 * m12 * m20 * m31 + m02 * m13 * m20 * m31 +
		   m03 * m10 * m22 * m31 - m00 * m13 * m22 * m31 -
		   m02 * m10 * m23 * m31 + m00 * m12 * m23 * m31 +
		   m03 * m11 * m20 * m32 - m01 * m13 * m20 * m32 -
		   m03 * m10 * m21 * m32 + m00 * m13 * m21 * m32 +
		   m01 * m10 * m23 * m32 - m00 * m11 * m23 * m32 -
		   m02 * m11 * m20 * m33 + m01 * m12 * m20 * m33 +
		   m02 * m10 * m21 * m33 - m00 * m12 * m21 * m33 -
		   m01 * m10 * m22 * m33 + m00 * m11 * m22 * m33;
}

Matrix44 Matrix44::operator *(float const & _a) const
{
	return Matrix44(m00 * _a, m01 * _a, m02 * _a, m03 * _a,
					m10 * _a, m11 * _a, m12 * _a, m13 * _a,
					m20 * _a, m21 * _a, m22 * _a, m23 * _a,
					m30 * _a, m31 * _a, m32 * _a, m33 * _a);
}

Segment Matrix44::to_world(Segment const & _a) const
{
	Segment result(_a);
	result.start = location_to_world(result.start);
	result.end = location_to_world(result.end);
	result.startToEnd = vector_to_world(result.startToEnd);
	result.startToEndDir = vector_to_world(result.startToEndDir);
	result.boundingBoxCalculated = false;

	return result;
}

Segment Matrix44::to_local(Segment const & _a) const
{
	Segment result(_a);
	result.start = location_to_local(result.start);
	result.end = location_to_local(result.end);
	result.startToEnd = vector_to_local(result.startToEnd);
	result.startToEndDir = vector_to_local(result.startToEndDir);
	result.boundingBoxCalculated = false;

	return result;
}
