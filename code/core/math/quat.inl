#include "quat.h"

String Quat::to_string() const
{
	return String::printf(TXT("x:%.10f y:%.10f z:%.10f w:%.10f"), x, y, z, w);
}

bool Quat::is_normalised(float _margin) const
{
	return abs(length() - 1.0f) < _margin;
}

void Quat::normalise()
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

Quat Quat::normal() const
{
	float lenSq = length_squared();
	if (lenSq != 0.0f)
	{
		lenSq = 1.0f / sqrt(lenSq);
		return Quat(x * lenSq, y * lenSq, z * lenSq, w * lenSq);
	}
	return Quat::zero;
}

Vector3 Quat::rotate(Vector3 const & _vec) const
{
	//	based on: http://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
	Vector3 const vecPart(x, y, z);
	return ((2.0f * Vector3::dot(vecPart, _vec)) * vecPart)
		 + ((w * w - Vector3::dot(vecPart, vecPart)) * _vec)
		 + 2.0f * w * Vector3::cross(vecPart, _vec);
}

Vector3 Quat::inverted_rotate(Vector3 const & _vec) const
{
	//	based on: http://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
	Vector3 const vecPart(-x, -y, -z);
	return ((2.0f * Vector3::dot(vecPart, _vec)) * vecPart)
		+ ((w * w - Vector3::dot(vecPart, vecPart)) * _vec)
		+ 2.0f * w * Vector3::cross(vecPart, _vec);
}

Quat Quat::rotate_by(Quat const & q) const
{
	//	based on: http://www.cprogramming.com/tutorial/3d/quaternions.html
	Quat result;
	result.w = w * q.w - x * q.x - y * q.y - z * q.z;
	result.x = w * q.x + x * q.w + y * q.z - z * q.y;
	result.y = w * q.y - x * q.z + y * q.w + z * q.x;
	result.z = w * q.z + x * q.y - y * q.x + z * q.w;
	return result;
}

Quat Quat::inverted_rotate_by(Quat const & q) const
{
	//	based on: http://www.cprogramming.com/tutorial/3d/quaternions.html
	Quat result;
	result.w = w * q.w + x * q.x + y * q.y + z * q.z;
	result.x = w * q.x - x * q.w - y * q.z + z * q.y;
	result.y = w * q.y + x * q.z - y * q.w - z * q.x;
	result.z = w * q.z - x * q.y + y * q.x - z * q.w;
	return result;
}

void Quat::fill_matrix33(Matrix33 & _mat) const
{
	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	//	based on: http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/

	{
		float const sqx = sqr(x);
		float const sqy = sqr(y);
		float const sqz = sqr(z);
		float const sqw = sqr(w);

		_mat.m00 = (sqx - sqy - sqz + sqw);
		_mat.m11 = (-sqx + sqy - sqz + sqw);
		_mat.m22 = (-sqx - sqy + sqz + sqw);
	}

	{
		float const xy = x * y;
		float const zw = z * w;
		_mat.m01 = 2.0f * (xy + zw);
		_mat.m10 = 2.0f * (xy - zw);
	}

	{
		float const xz = x * z;
		float const yw = y * w;
		_mat.m02 = 2.0f * (xz - yw);
		_mat.m20 = 2.0f * (xz + yw);
	}

	{
		float const yz = y * z;
		float const xw = x * w;
		_mat.m12 = 2.0f * (yz + xw);
		_mat.m21 = 2.0f * (yz - xw);
	}
}

void Quat::fill_matrix33(Matrix44 & _mat) const
{
	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	//	based on: http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/
	
	{
		float const sqx = sqr(x);
		float const sqy = sqr(y);
		float const sqz = sqr(z);
		float const sqw = sqr(w);

		_mat.m00 = ( sqx - sqy - sqz + sqw);
		_mat.m11 = (-sqx + sqy - sqz + sqw);
		_mat.m22 = (-sqx - sqy + sqz + sqw);
	}

	{
		float const xy = x * y;
		float const zw = z * w;
		_mat.m01 = 2.0f * (xy + zw);
		_mat.m10 = 2.0f * (xy - zw);
	}

    {
		float const xz = x * z;
		float const yw = y * w;
		_mat.m02 = 2.0f * (xz - yw);
		_mat.m20 = 2.0f * (xz + yw);
	}

    {
		float const yz = y * z;
		float const xw = x * w;
		_mat.m12 = 2.0f * (yz + xw);
		_mat.m21 = 2.0f * (yz - xw);  
	}
}

Vector3 Quat::get_axis(Axis::Type _axis) const
{
	if (_axis == Axis::X) return get_x_axis();
	if (_axis == Axis::Y) return get_y_axis();
	if (_axis == Axis::Z) return get_z_axis();
	return Vector3::zero;
}

Vector3 Quat::get_x_axis() const
{
	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	//	based on: fill_matrix3() above
	Vector3 result;
	{
		float const sqx = sqr(x);
		float const sqy = sqr(y);
		float const sqz = sqr(z);
		float const sqw = sqr(w);

		result.x = ( sqx - sqy - sqz + sqw);
	}

	{
		float const xy = x * y;
		float const zw = z * w;
		result.y = 2.0f * (xy + zw);
	}

    {
		float const xz = x * z;
		float const yw = y * w;
		result.z = 2.0f * (xz - yw);
	}

	return result;
}

Vector3 Quat::get_y_axis() const
{
	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	//	based on: fill_matrix3() above
	Vector3 result;
	{
		float const sqx = sqr(x);
		float const sqy = sqr(y);
		float const sqz = sqr(z);
		float const sqw = sqr(w);

		result.y = (-sqx + sqy - sqz + sqw);
	}

	{
		float const xy = x * y;
		float const zw = z * w;
		result.x = 2.0f * (xy - zw);
	}

    {
		float const yz = y * z;
		float const xw = x * w;
		result.z = 2.0f * (yz + xw);
	}

	return result;
}

Vector3 Quat::get_z_axis() const
{
	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	//	based on: fill_matrix3() above
	Vector3 result;
	{
		float const sqx = sqr(x);
		float const sqy = sqr(y);
		float const sqz = sqr(z);
		float const sqw = sqr(w);

		result.z = (-sqx - sqy + sqz + sqw);
	}

    {
		float const xz = x * z;
		float const yw = y * w;
		result.x = 2.0f * (xz + yw);
	}

    {
		float const yz = y * z;
		float const xw = x * w;
		result.y = 2.0f * (yz - xw);
	}

	return result;
}

Matrix33 Quat::to_matrix_33() const
{
	Matrix33 result;

	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	// scale should go to proper row
	fill_matrix33(result);

	return result;
}

Matrix44 Quat::to_matrix() const
{
	Matrix44 result;

	an_assert(is_normalised(), TXT("transform's orientation not normalised"));

	// scale should go to proper row
	fill_matrix33(result);

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	result.m30 = 0.0f;
	result.m31 = 0.0f;
	result.m32 = 0.0f;

	return result;
}

Rotator3 Quat::to_rotator() const
{
	//	based on: http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

	// again - randomly changed few bits
	Rotator3 result;
	float singularityTest = z*y + w*x;

	result.yaw = radian_to_degree(atan2_rad(2.0f * (-w*z + x*y), (1.0f - 2.0f * (x*x + z*z))));

	#define SINGULARITY_THRESHOLD	0.4999995f

	if (singularityTest < -SINGULARITY_THRESHOLD)
	{
		// TODO test it
		result.roll = -result.yaw - radian_to_degree(2.0f * atan2_rad(x, -w));
		result.pitch = -90.0f;
	}
	else if (singularityTest > SINGULARITY_THRESHOLD)
	{
		// TODO test it
		result.roll = result.yaw - radian_to_degree(2.0f * atan2_rad(x, -w));
		result.pitch = 90.0f;
	}
	else
	{
		result.roll = radian_to_degree(atan2_rad(-2.0f*(-w*y + x*z), (1 - 2.0f*(x*x + y*y))));
		result.pitch = radian_to_degree(asin_rad(2 * (singularityTest)));
	}

	// TODO normalise?

	return result;
}

Quat Quat::lerp(float _t, Quat const & _a, Quat const & _b)
{
	an_assert(_a.is_normalised());
	an_assert(_b.is_normalised());

	float dot = Quat::dot(_a, _b);

	if (dot < 0.0f)
	{
		return (_a * (1.0f - _t) - _b * _t).normal();
	}
	else
	{
		return (_a * (1.0f - _t) + _b * _t).normal();
	}
}

Quat Quat::slerp(float _t, Quat const & _a, Quat const & _b)
{
	an_assert(_a.is_normalised());
	an_assert(_b.is_normalised());

	float dot = Quat::dot(_a, _b);

	float threshold = 0.9995f;
	float absDot = abs(dot);
	Quat c = dot < 0.0f ? -_b : _b;
	if (absDot < threshold)
	{
		float angle = acos_rad(absDot);
		Quat result = (_a * sin_rad(angle * (1 - _t)) + c * sin_rad(angle * _t)) / sin_rad(angle);
		an_assert(result.is_normalised());
		return result;
	}
	else
	{
		Quat result = (_a * (1.0f - _t) + c * _t).normal();
		an_assert(result.is_normalised());
		return result;
	}
}

Quat Quat::axis_rotation(Vector3 const & _axis, float _angle)
{
	return axis_rotation_normalised(_axis.normal(), _angle);
}

Quat Quat::axis_rotation_normalised(Vector3 const & _axis, float _angle)
{
	an_assert(_axis.is_normalised());
	float sa = sin_deg(_angle);
	float ca = cos_deg(_angle);
	Quat result;
	result.x = _axis.x * sa;
	result.y = _axis.y * sa;
	result.z = _axis.z * sa;
	result.w = ca;
	return result;
}
