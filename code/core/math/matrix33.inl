Matrix33::Matrix33(float _m00, float _m01, float _m02,
				   float _m10, float _m11, float _m12,
				   float _m20, float _m21, float _m22)
{
	m00 = _m00;	m01 = _m01;	m02 = _m02;
	m10 = _m10;	m11 = _m11;	m12 = _m12;
	m20 = _m20;	m21 = _m21;	m22 = _m22;
}

Matrix33 Matrix33::inverted() const
{
	an_assert(is_orthogonal(), TXT("matrix should be orthogonal"));

	float const scale = extract_scale();
	float const invScale = scale != 0.0f ? 1.0f / scale : 0.0f;

	return Matrix33(m00 * invScale, m10 * invScale, m20 * invScale,
					m01 * invScale, m11 * invScale, m21 * invScale,
					m02 * invScale, m12 * invScale, m22 * invScale);
}

Matrix33 Matrix33::full_inverted() const
{
	// based on: http://www.cg.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche23.html
	float detValue = det();
	an_assert(detValue != 0.0f);
	if (detValue == 0.0f)
	{
		return Matrix33::zero;
	}
	return Matrix33(m11 * m22 - m12 * m21, m02 * m21 - m01 * m22, m01 * m12 - m02 * m11,
					m12 * m20 - m10 * m22, m00 * m22 - m02 * m20, m02 * m10 - m00 * m12,
					m10 * m21 - m11 * m20, m01 * m20 - m00 * m21, m00 * m11 - m01 * m10) * (1.0f / detValue);
}

Matrix33 Matrix33::transposed() const
{
	return Matrix33(m00, m10, m20,
					m01, m11, m21,
					m02, m12, m22);
}

Matrix44 Matrix33::to_matrix44() const
{
	return Matrix44(m00, m01, m02, 0.0f,
					m10, m11, m12, 0.0f,
					m20, m21, m22, 0.0f,
					0.0f,0.0f,0.0f,1.0f);
}

Vector2 Matrix33::location_to_world(Vector2 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	return Vector2( m00 * _a.x  +  m10 * _a.y  +  m20,
					m01 * _a.x  +  m11 * _a.y  +  m21 );
}

Vector2 Matrix33::location_to_local(Vector2 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	Vector2 b( _a.x - m20, _a.y - m21 );

	return Vector2( m00 * b.x  +  m01 * b.y,
					m10 * b.x  +  m11 * b.y );
}

Vector2 Matrix33::vector_to_world(Vector2 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	return Vector2( m00 * _a.x  +  m10 * _a.y,
					m01 * _a.x  +  m11 * _a.y );
}

Vector2 Matrix33::vector_to_local(Vector2 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	return Vector2( m00 * _a.x  +  m01 * _a.y,
					m10 * _a.x  +  m11 * _a.y );
}

Vector3 Matrix33::to_world(Vector3 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	return Vector3( m00 * _a.x  +  m10 * _a.y  +  m20 * _a.z,
					m01 * _a.x  +  m11 * _a.y  +  m21 * _a.z,
					m02 * _a.x  +  m12 * _a.y  +  m22 * _a.z );
}

Vector3 Matrix33::to_local(Vector3 const & _a) const
{
	//an_assert(is_orthonormal(), TXT("matrix should be orthonormal"));

	return Vector3( m00 * _a.x  +  m01 * _a.y  +  m02 * _a.z,
					m10 * _a.x  +  m11 * _a.y  +  m12 * _a.z,
					m20 * _a.x  +  m21 * _a.y  +  m22 * _a.z );
}

Matrix33 Matrix33::to_world(Matrix33 const & _a) const
{
	return Matrix33( m00 * _a.m00  +  m10 * _a.m01  +  m20 * _a.m02,
					 m01 * _a.m00  +  m11 * _a.m01  +  m21 * _a.m02,
					 m02 * _a.m00  +  m12 * _a.m01  +  m22 * _a.m02,
				/* */m00 * _a.m10  +  m10 * _a.m11  +  m20 * _a.m12,
					 m01 * _a.m10  +  m11 * _a.m11  +  m21 * _a.m12,
					 m02 * _a.m10  +  m12 * _a.m11  +  m22 * _a.m12,
				/* */m00 * _a.m20  +  m10 * _a.m21  +  m20 * _a.m22,
					 m01 * _a.m20  +  m11 * _a.m21  +  m21 * _a.m22,
					 m02 * _a.m20  +  m12 * _a.m21  +  m22 * _a.m22 );
}

Matrix33 Matrix33::to_local(Matrix33 const & _a) const
{
	// inverted is quite fast
	return inverted().to_world(_a);
}

bool Matrix33::is_orthonormal() const
{
	return abs(m00 * m10 + m01 * m11 + m02 * m12) < 0.01f &&
		   abs(m00 * m20 + m01 * m21 + m02 * m22) < 0.01f &&
		   abs(m00 * m00 + m01 * m01 + m02 * m02 - 1.0f) < 0.01f &&
		   abs(m10 * m10 + m11 * m11 + m12 * m12 - 1.0f) < 0.01f &&
		   abs(m20 * m20 + m21 * m21 + m22 * m22 - 1.0f) < 0.01f;
}

bool Matrix33::is_orthogonal() const
{
	return abs(m00 * m10 + m01 * m11 + m02 * m12) < 0.01f &&
		   abs(m00 * m20 + m01 * m21 + m02 * m22) < 0.01f;
}

void Matrix33::remove_scale()
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

float Matrix33::extract_scale() const
{
	float xAxis = get_x_axis().length();
	an_assert(length_of(xAxis - get_y_axis().length()) < 0.001f);
	an_assert(length_of(xAxis - get_z_axis().length()) < 0.001f);
	return xAxis;
}

float Matrix33::extract_scale_squared() const
{
	float xAxis = get_x_axis().length_squared();
	an_assert(length_of(xAxis - get_y_axis().length_squared()) < 0.001f);
	an_assert(length_of(xAxis - get_z_axis().length_squared()) < 0.001f);
	return xAxis;
}

Quat Matrix33::to_quat() const
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

Transform Matrix33::to_transform() const
{
	//Transform t = Transform(get_translation(), to_quat());
	//Matrix33 m = t.to_matrix();
	return Transform(Vector3::zero, to_quat(), get_x_axis().length());
}

bool Matrix33::operator ==(Matrix33 const & _a) const
{
	return m00 == _a.m00 && m01 == _a.m01 && m02 == _a.m02 &&
		   m10 == _a.m10 && m11 == _a.m11 && m12 == _a.m12 &&
		   m20 == _a.m20 && m21 == _a.m21 && m22 == _a.m22 ;
}

bool Matrix33::operator !=(Matrix33 const & _a) const
{
	return !operator==(_a);
}

float Matrix33::det() const
{
	return m00 * m11 * m22 + m10 * m21 * m02 + m20 * m01 * m12 - m00 * m21 * m12 - m20 * m11 * m02 - m01 * m10 * m22;
}

Matrix33 Matrix33::operator *(float const & _a) const
{
	return Matrix33(m00 * _a, m01 * _a, m02 * _a,
					m10 * _a, m11 * _a, m12 * _a,
					m20 * _a, m21 * _a, m22 * _a);
}
