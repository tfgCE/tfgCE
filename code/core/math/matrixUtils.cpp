#include "math.h"

#include "..\random\random.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

Matrix44 projection_matrix(float const minX, float const maxX, float const minY, float const maxY, float const _nearPlane, float const _farPlane)
{
	float const width = (maxX - minX);
	float const height = (maxY - minY);
	float const offsetZ = _nearPlane; // set to zero for a [0,1] clip space

	Matrix44 out;
	if (_farPlane <= _nearPlane)
	{
		// place the far plane at infinity
		out.m[0][0] = 2.0f * _nearPlane / width;
		out.m[1][0] = 0.0f;
		out.m[2][0] = (maxX + minX) / width;
		out.m[3][0] = 0.0f;

		out.m[0][1] = 0.0f;
		out.m[1][1] = 2.0f * _nearPlane / height;
		out.m[2][1] = (maxY + minY) / height;
		out.m[3][1] = 0.0f;

		out.m[0][2] = 0.0f;
		out.m[1][2] = 0.0f;
		out.m[2][2] = -1.0f;
		out.m[3][2] = -(_nearPlane + offsetZ);

		out.m[0][3] = 0.0f;
		out.m[1][3] = 0.0f;
		out.m[2][3] = -1.0f;
		out.m[3][3] = 0.0f;
	}
	else
	{
		// normal projection
		out.m[0][0] = 2.0f * _nearPlane / width;
		out.m[1][0] = 0.0f;
		out.m[2][0] = (maxX + minX) / width;
		out.m[3][0] = 0.0f;

		out.m[0][1] = 0.0f;
		out.m[1][1] = 2.0f * _nearPlane / height;
		out.m[2][1] = (maxY + minY) / height;
		out.m[3][1] = 0.0f;

		out.m[0][2] = 0.0f;
		out.m[1][2] = 0.0f;
		out.m[2][2] = -(_farPlane + offsetZ) / (_farPlane - _nearPlane);
		out.m[3][2] = -(_farPlane * (_nearPlane + offsetZ)) / (_farPlane - _nearPlane);

		out.m[0][3] = 0.0f;
		out.m[1][3] = 0.0f;
		out.m[2][3] = -1.0f;
		out.m[3][3] = 0.0f;
	}
	return out;
}

Matrix44 projection_matrix_assymetric_fov(float const leftDegrees, float const rightDegrees, float const upDegrees, float const downDegrees, float const _nearPlane, float const _farPlane)
{
	float const minX = -_nearPlane * tan_deg(leftDegrees);
	float const maxX =  _nearPlane * tan_deg(rightDegrees);

	float const minY = -_nearPlane * tan_deg(downDegrees);
	float const maxY =  _nearPlane * tan_deg(upDegrees);

	return projection_matrix(minX, maxX, minY, maxY, _nearPlane, _farPlane);
}

Matrix44 projection_matrix(DEG_ float const _fov, float const _aspectRatio, float const _nearPlane, float const _farPlane)
{
	Matrix44 result = Matrix44::identity;
	Vector2 fovSize;
	// fov relates to y (that's why fovsize is y)
	fovSize.y = _nearPlane * tan_deg(_fov / 2.0f);
	fovSize.x = fovSize.y * _aspectRatio;
	// based on: something found in Internet, but I had to modify it to work properly for x+right, y+up, z+closer
	// and I had to drop 2.0 in few places as it was some crazy way to make up for some other incorrect calculations
	result.m00 = _nearPlane / fovSize.x;
	result.m11 = _nearPlane / fovSize.y;
	result.m22 = -(_farPlane + _nearPlane) / (_farPlane - _nearPlane);
	result.m23 = -1.0f;
	result.m32 = -(2.0f * _farPlane * _nearPlane) / (_farPlane - _nearPlane);
	result.m33 = 0.0f;
	return result;
}

Matrix44 orthogonal_matrix_for_2d(LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, float const _nearPlane, float const _farPlane)
{
	Matrix44 result = Matrix44::identity;
	result.m00 = 2.0f / _size.x;
	result.m11 = 2.0f / _size.y;
	result.m22 = -(_farPlane + _nearPlane) / (_farPlane - _nearPlane);
	result.m30 = -(2.0f * _leftBottom.x + _size.x ) / _size.x;
	result.m31 = -(2.0f * _leftBottom.y + _size.y ) / _size.y;
	result.m32 = -(2.0f * _farPlane * _nearPlane) / (_farPlane - _nearPlane);
	return result;
}

Matrix44 orthogonal_matrix(Range2 const & _range, float const _nearPlane, float const _farPlane)
{
	Matrix44 result = Matrix44::identity;
	result.m00 = 2.0f / (_range.x.max - _range.x.min);
	result.m11 = 2.0f / (_range.y.max - _range.y.min);
	result.m22 = -2.0f / (_farPlane - _nearPlane);
	result.m30 = -(_range.x.min + _range.x.max) / (_range.x.max - _range.x.min);
	result.m31 = -(_range.y.min + _range.y.max) / (_range.y.max - _range.y.min);
	result.m32 = -(_farPlane + _nearPlane) / (_farPlane - _nearPlane);
	//result.m32 = -(2.0f) / (_farPlane - _nearPlane);
	return result;
}

Matrix44 camera_view_matrix(LOCATION_ Vector3 const & _from, LOCATION_ Vector3 const & _at, NORMAL_ Vector3 const & _cameraUp)
{
	an_assert(_cameraUp.is_normalised());

	Matrix44 result;
	Vector3 up, right, viewDir;

	viewDir = _at - _from;
	viewDir.normalise();

	right = Vector3::cross(viewDir, _cameraUp);
	right.normalise();

	up = Vector3::cross(right, viewDir);

	// alright!
	// here's what's going on:
	// by default row is axis, so our matrix would be like this
	// row 0 - right
	// row 1 - viewDir
	// row 2 - up
	// row 3 - location of camera
	// but we want it to be inversed
	// what should we do then?
	// transpose 3x3 matrix, so rows 0-2 become columns
	// calculate location as toLocal from old matrix calculated on -from
	// which means that we use - instead of + and we do toWorld on our result matrix (as toLocal is transposed toWorld)

	// transform to open gl coords is done in stack etc

	// rotation part:
    result.m00 = right.x;
    result.m10 = right.y;
    result.m20 = right.z;
    result.m01 = viewDir.x;
    result.m11 = viewDir.y;
    result.m21 = viewDir.z;
    result.m02 = up.x;
    result.m12 = up.y;
    result.m22 = up.z;

	// location part:
    result.m30 = - result.m00 * _from.x - result.m10 * _from.y - result.m20 * _from.z;
    result.m31 = - result.m01 * _from.x - result.m11 * _from.y - result.m21 * _from.z;
    result.m32 = - result.m02 * _from.x - result.m12 * _from.y - result.m22 * _from.z;

	// fourth column:
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = 0.0f;
    result.m33 = 1.0f;

	return result;
}

Matrix44 matrix_from_up(LOCATION_ Vector3 const& _from, NORMAL_ Vector3 const& _up)
{
	an_assert(_up.is_normalised());
	while (true)
	{
		Vector3 tempDir;
		Random::Generator rg;
		tempDir.x = rg.get_float(-1.0f, 1.0f);
		tempDir.y = rg.get_float(-1.0f, 1.0f);
		tempDir.z = rg.get_float(-1.0f, 1.0f);
		Vector3 lookDir = Vector3::cross(tempDir, _up);
		if (abs(Vector3::dot(lookDir, _up)) > 0.99f)
		{
			continue;
		}
		lookDir = lookDir.normal();
		return look_matrix(_from, lookDir, _up);
	}
}

Vector3 calculate_flat_forward_from(Transform const& _headTransform)
{
	Vector3 lookForward = _headTransform.get_axis(Axis::Y);
	{
		Vector3 lookUp = _headTransform.get_axis(Axis::Z);
		lookForward = lookForward + (0.7f * lookUp * -lookForward.z); // flatten it
		lookForward = (lookForward * Vector3(1.0f, 1.0f, 0.0)).normal();
	}
	return lookForward;
}

